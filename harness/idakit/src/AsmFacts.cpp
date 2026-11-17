#include "idakit/AsmFacts.hpp"

#include "idakit/Asm.hpp"

#include <algorithm>
#include <optional>
#include <set>
#include <string_view>

namespace idakit
{

namespace
{

// IDA's x86 register numbers: eax..edi are 0-7, al, cl, dl, bl, ah, ch, dh, bh are 16-23
int family(uint16 reg)
{
	if (reg < 8)
		return reg;
	if (reg >= 16 && reg < 24)
		return (reg - 16) % 4;
	return -1;
}

constexpr int kEax = 0, kEcx = 1, kEdx = 2, kEsp = 4;

bool isRegister(const op_t& op, int reg)
{
	return op.type == o_reg && op.reg == reg;
}

// the frame slot `[ebp+d]` an operand addresses (base ebp, no index register), else nullopt
std::optional<int64_t> frameSlot(const insn_t& insn, const op_t& op)
{
	if ((op.type != o_displ && op.type != o_phrase) || x86_base_reg(insn, op) != R_bp ||
		x86_index_reg(insn, op) != R_none)
		return std::nullopt;
	return op.type == o_displ ? static_cast<int64_t>(static_cast<int32_t>(op.addr)) : 0;
}

}  // namespace

RegisterUse::RegisterUse(const insn_t& insn)
{
	auto read = [this](int f, int bytes) {
		if (f >= 0)
			reads[f] = std::max(reads[f], bytes);
	};
	auto write = [this](int f) {
		if (f >= 0)
			writes[f] = true;
	};
	const op_t* ops = insn.ops;
	// `xor eax, eax` and the like only write
	if ((insn.itype == NN_xor || insn.itype == NN_sub || insn.itype == NN_sbb) && ops[0].type == o_reg &&
		isRegister(ops[1], ops[0].reg) && ops[2].type == o_void)
	{
		write(family(ops[0].reg));
		return;
	}
	uint32 feature = insn.get_canon_feature(PH);
	for (int n = 0; n < UA_MAXOP && ops[n].type != o_void; ++n)
	{
		const op_t& op = ops[n];
		if (op.type == o_reg)
		{
			int f = family(op.reg);
			// `push ecx` allocates a stack slot (MSVC's prologues); it is no use of `this`
			if (has_cf_use(feature, n) && !(insn.itype == NN_push && f == kEcx))
				read(f, static_cast<int>(get_dtype_size(op.dtype)));
			if (has_cf_chg(feature, n))
				write(f);
		}
		else if (op.type == o_phrase || op.type == o_displ)
		{
			if (op.specflag1)
			{
				// a SIB byte: its base and, unless 4 (none), its index
				int sib = op.specflag2;
				read(family(sib & 7), 4);
				if (((sib >> 3) & 7) != 4)
					read(family((sib >> 3) & 7), 4);
			}
			else
				read(family(op.reg), 4);
		}
	}
	switch (insn.itype)
	{
	case NN_cdq:
		read(kEax, 4);
		write(kEdx);
		break;
	case NN_div:
	case NN_idiv:
		read(kEax, 4);
		read(kEdx, 4);
		write(kEax);
		write(kEdx);
		break;
	case NN_mul:
	case NN_imul:
		// the one-operand forms multiply eax into edx:eax
		if (ops[1].type == o_void)
		{
			read(kEax, 4);
			write(kEax);
			write(kEdx);
		}
		break;
	case NN_loop:
	case NN_loope:
	case NN_loopne:
	case NN_jecxz:
		read(kEcx, 4);
		break;
	case NN_call:
		write(kEax);
		write(kEcx);
		write(kEdx);
		break;
	default:
		break;
	}
}

std::optional<PushedArgument> pushedArgument(ea_t callSite, int64_t offset)
{
	func_t* pfn = get_func(callSite);
	insn_t insn;
	ea_t at = callSite;
	if (!pfn || decode_insn(&insn, at) <= 0 || (insn.itype != NN_call && insn.itype != NN_callni))
		return std::nullopt;
	// the slot as the sp trace places it: a push writes 4 bytes below the sp before it
	sval_t slot = get_spd(pfn, callSite) + static_cast<sval_t>(offset);
	bool afterCall = false;
	int reg = -1;
	for (int steps = 0; steps < 64; ++steps)
	{
		// another path reaches the instruction: what runs before it is not the call's alone
		if (get_first_fcref_to(at) != BADADDR)
			return std::nullopt;
		at = decode_prev_insn(&insn, at);
		if (at == BADADDR || is_basic_block_end(insn, false))
			return std::nullopt;
		afterCall |= insn.itype == NN_call || insn.itype == NN_callni;
		if (reg < 0)
		{
			if (insn.itype == NN_push && get_spd(pfn, at) - 4 == slot)
			{
				// a constant pushed: the push is its own source
				if (insn.ops[0].type == o_imm)
					return PushedArgument{insn, afterCall};
				if (insn.ops[0].type != o_reg || insn.ops[0].reg >= 8 || get_dtype_size(insn.ops[0].dtype) != 4)
					return std::nullopt;
				reg = insn.ops[0].reg;
			}
			continue;
		}
		if (RegisterUse(insn).writes[reg])
			return PushedArgument{insn, afterCall};
	}
	return std::nullopt;
}

bool pushesExtendedBool(ea_t callSite, int64_t offset)
{
	std::optional<PushedArgument> pushed = pushedArgument(callSite, offset);
	if (!pushed)
		return false;
	const insn_t& source = pushed->source;
	const op_t& to = source.ops[0];
	if (to.type != o_reg || family(to.reg) < 0)
		return false;
	int reg = family(to.reg);
	// a call between the source and the call (a nested call building another argument or the receiver) runs after
	// the push only where the push follows the source at once
	insn_t next;
	if (pushed->afterCall && (decode_insn(&next, source.ea + source.size) <= 0 || next.itype != NN_push ||
							  next.ops[0].type != o_reg || family(next.ops[0].reg) != reg))
		return false;
	if (source.itype == NN_movzx)
		return get_dtype_size(source.ops[1].dtype) == 1;
	auto previous = [](ea_t& at, insn_t& insn) {
		if (get_first_fcref_to(at) != BADADDR)
			return false;
		at = decode_prev_insn(&insn, at);
		return at != BADADDR && !is_basic_block_end(insn, false);
	};
	ea_t at = source.ea;
	insn_t insn;
	// `sbb r, r` leaves 0 or -1, which `neg r` or `add r, 1` makes 1 or 0
	if ((source.itype == NN_neg || (source.itype == NN_add && source.ops[1].type == o_imm &&
									source.ops[1].value == 1)) &&
		to.reg < 8)
		return previous(at, insn) && insn.itype == NN_sbb && isRegister(insn.ops[0], to.reg) &&
			   isRegister(insn.ops[1], to.reg);
	if (source.itype < NN_seta || source.itype > NN_setz || to.reg < 16 || to.reg >= 20)
		return false;
	for (int steps = 0; steps < 16 && previous(at, insn); ++steps)
	{
		if (insn.itype == NN_xor && isRegister(insn.ops[0], reg) && isRegister(insn.ops[1], reg))
			return true;
		if (RegisterUse(insn).writes[reg])
			return false;
	}
	return false;
}

namespace
{

// A value an instruction of a call's block sees in a register, as the instruction defining it: the last write before
// `at` that is no copy (`mov r, s`) or address arithmetic (`lea r, [s+N]`, `add r, N`, `sub r, N`: followed to s
// with N added), else the register at the block's start (writer: the block's first instruction, defined false); or
// the dword a `push [s+N]` pushes (reg -1). Two values are the same (sameValue) where writer, register and offset
// are, or where both are dword loads, at one offset, from the same address with no store between them.
struct BlockValue
{
	ea_t writer = BADADDR;
	int reg = -1;
	int64_t offset = 0;
	bool defined = false;
	insn_t insn;  // the defining instruction (defined)
};

// the base register of a memory operand without an index (`[r+N]`, `[r]`), and its displacement; -1 for any other
int memoryBase(const insn_t& insn, const op_t& op, int64_t& displacement)
{
	if ((op.type != o_displ && op.type != o_phrase) || x86_index_reg(insn, op) != R_none)
		return -1;
	int base = x86_base_reg(insn, op);
	if (base < 0 || base >= 8)
		return -1;
	displacement = op.type == o_displ ? static_cast<int64_t>(static_cast<int32_t>(op.addr)) : 0;
	return base;
}

// esp is followed to the frame: its value at an instruction is the function's start (writer) at the sp trace's
// offset there, which pushes and calls change without RegisterUse writing esp
std::optional<BlockValue> blockValue(ea_t at, int reg)
{
	BlockValue value;
	insn_t insn;
	for (int steps = 0; steps < 64; ++steps)
	{
		if (reg == kEsp)
		{
			func_t* pfn = get_func(at);
			if (!pfn)
				return std::nullopt;
			value.writer = pfn->start_ea;
			value.reg = reg;
			value.offset += get_spd(pfn, at);
			return value;
		}
		ea_t previous = get_first_fcref_to(at) != BADADDR ? BADADDR : decode_prev_insn(&insn, at);
		if (previous == BADADDR || is_basic_block_end(insn, false))
		{
			value.writer = at;
			value.reg = reg;
			return value;
		}
		at = previous;
		if (!RegisterUse(insn).writes[reg])
			continue;
		const op_t* ops = insn.ops;
		int64_t displacement = 0;
		if (insn.itype == NN_mov && isRegister(ops[0], reg) && ops[1].type == o_reg && ops[1].reg < 8 &&
			get_dtype_size(ops[1].dtype) == 4)
		{
			reg = ops[1].reg;
			continue;
		}
		if (int base = insn.itype == NN_lea && isRegister(ops[0], reg) ? memoryBase(insn, ops[1], displacement) : -1;
			base >= 0)
		{
			value.offset += displacement;
			reg = base;
			continue;
		}
		if ((insn.itype == NN_add || insn.itype == NN_sub) && isRegister(ops[0], reg) && ops[1].type == o_imm)
		{
			int64_t n = static_cast<int32_t>(ops[1].value);
			value.offset += insn.itype == NN_add ? n : -n;
			continue;
		}
		value.writer = at;
		value.reg = reg;
		value.defined = true;
		value.insn = insn;
		return value;
	}
	return std::nullopt;
}

// the dword memory operand a definition loads: `mov r, [s+N]` or `push [s+N]`; nullptr for any other
const op_t* loadOperand(const BlockValue& v)
{
	if (!v.defined)
		return nullptr;
	const op_t* op = v.insn.itype == NN_mov && v.reg >= 0 && isRegister(v.insn.ops[0], v.reg) ? &v.insn.ops[1]
					 : v.insn.itype == NN_push && v.reg < 0									  ? &v.insn.ops[0]
																							  : nullptr;
	return op && get_dtype_size(op->dtype) == 4 ? op : nullptr;
}

// the value a dword load defining v reads its address from: s's value plus N, or s's value alone with N in
// *displacement where that is given; nullopt for any other definition
std::optional<BlockValue> loadedFrom(const BlockValue& v, int64_t* displacement = nullptr)
{
	int64_t n = 0;
	const op_t* op = v.offset == 0 ? loadOperand(v) : nullptr;
	int base = op ? memoryBase(v.insn, *op, n) : -1;
	if (base < 0)
		return std::nullopt;
	std::optional<BlockValue> address = blockValue(v.writer, base);
	if (address && displacement)
		*displacement = n;
	else if (address)
		address->offset += n;
	return address;
}

// no instruction in [from, to) of straight-line code writes memory: no call and no store (a push writes below the
// frame's locals)
bool noStoreBetween(ea_t from, ea_t to)
{
	insn_t insn;
	for (ea_t at = from; at < to; at += insn.size)
	{
		if (decode_insn(&insn, at) <= 0 || insn.itype == NN_call || insn.itype == NN_callni)
			return false;
		uint32 feature = insn.get_canon_feature(PH);
		for (int n = 0; n < UA_MAXOP && insn.ops[n].type != o_void; ++n)
		{
			optype_t type = insn.ops[n].type;
			if ((type == o_mem || type == o_displ || type == o_phrase) && has_cf_chg(feature, n))
				return false;
		}
	}
	return true;
}

bool sameValue(const BlockValue& a, const BlockValue& b, int depth = 0)
{
	if (a.writer == b.writer && a.reg == b.reg && a.offset == b.offset)
		return true;
	if (depth > 4 || a.offset != b.offset || !loadOperand(a) || !loadOperand(b))
		return false;
	BlockValue x = a, y = b;
	x.offset = y.offset = 0;
	std::optional<BlockValue> from = loadedFrom(x), to = loadedFrom(y);
	return from && to && sameValue(*from, *to, depth + 1) &&
		   noStoreBetween(std::min(a.writer, b.writer), std::max(a.writer, b.writer));
}

}  // namespace

bool callsOnVtableObject(ea_t callSite, std::optional<int64_t> stackOffset)
{
	func_t* pfn = get_func(callSite);
	insn_t call;
	if (!pfn || decode_insn(&call, callSite) <= 0 || call.itype != NN_callni)
		return false;
	// the vtable pointer: the base of the slot's operand, `call [vtbl+N]`, or of the load of the called register
	std::optional<BlockValue> table;
	int64_t displacement = 0;
	if (int base = memoryBase(call, call.ops[0], displacement); base >= 0)
		table = blockValue(callSite, base);
	else if (call.ops[0].type == o_reg && call.ops[0].reg < 8)
		if (std::optional<BlockValue> slot = blockValue(callSite, call.ops[0].reg))
			table = loadedFrom(*slot, &displacement);
	std::optional<BlockValue> object = table ? loadedFrom(*table) : std::nullopt;
	if (!object)
		return false;
	// the `this` the call passes: ecx, or what is pushed into its stack slot (a register, or a dword from memory)
	std::optional<BlockValue> self;
	if (!stackOffset)
		self = blockValue(callSite, kEcx);
	else
	{
		sval_t slot = get_spd(pfn, callSite) + static_cast<sval_t>(*stackOffset);
		insn_t insn;
		ea_t at = callSite;
		for (int steps = 0; steps < 64 && !self; ++steps)
		{
			if (get_first_fcref_to(at) != BADADDR)
				return false;
			at = decode_prev_insn(&insn, at);
			if (at == BADADDR || is_basic_block_end(insn, false))
				return false;
			if (insn.itype != NN_push || get_spd(pfn, at) - 4 != slot)
				continue;
			if (insn.ops[0].type == o_reg && insn.ops[0].reg < 8 && get_dtype_size(insn.ops[0].dtype) == 4)
				self = blockValue(at, insn.ops[0].reg);
			else
			{
				BlockValue pushed;
				pushed.writer = at;
				pushed.defined = true;
				pushed.insn = insn;
				if (!loadOperand(pushed))
					return false;
				self = pushed;
			}
			if (!self)
				return false;
		}
	}
	return self && sameValue(*self, *object);
}


bool AsmFacts::readsBeforeWrite(ea_t function, int family) const
{
	func_t* f = get_func(function);
	if (!f)
		return false;
	FlowChart chart(f);
	const std::vector<FlowChart::Block>& blocks = chart.blocks();
	std::vector<bool> seen(blocks.size());
	std::vector<int> pending{0};
	int budget = 5000;
	while (!pending.empty() && budget > 0)
	{
		int k = pending.back();
		pending.pop_back();
		if (k < 0 || static_cast<size_t>(k) >= blocks.size() || seen[k])
			continue;
		seen[k] = true;
		bool written = false;
		for (ea_t ea : heads(blocks[k].start, blocks[k].end))
		{
			insn_t insn;
			if (--budget <= 0 || decode_insn(&insn, ea) <= 0)
				break;
			RegisterUse use(insn);
			if (use.reads[family])
				return true;
			if (use.writes[family])
			{
				written = true;
				break;
			}
		}
		if (!written)
			pending.insert(pending.end(), blocks[k].succs.begin(), blocks[k].succs.end());
	}
	return false;
}

const std::vector<ea_t>& AsmFacts::callSites(ea_t function)
{
	auto [found, added] = m_callSites.try_emplace(function);
	if (added)
	{
		xrefblk_t xb;
		for (bool ok = xb.first_to(function, XREF_NOFLOW); ok; ok = xb.next_to())
		{
			insn_t insn;
			if (xb.iscode && decode_insn(&insn, xb.from) > 0 && insn.itype == NN_call &&
				insn.ops[0].type == o_near && insn.ops[0].addr == function)
				found->second.push_back(xb.from);
		}
	}
	return found->second;
}

std::vector<int64_t> AsmFacts::cleanup(ea_t function)
{
	std::vector<int64_t> found;
	for (ea_t site : callSites(function))
	{
		ea_t ea = next_head(site, BADADDR);
		insn_t insn;
		if (decode_insn(&insn, ea) <= 0)
			continue;
		if (insn.itype == NN_add && isRegister(insn.ops[0], kEsp) && insn.ops[1].type == o_imm)
			found.push_back(static_cast<int64_t>(insn.ops[1].value));
		else if (insn.itype == NN_pop && isRegister(insn.ops[0], kEcx))
		{
			int pops = 0;
			do
			{
				++pops;
				ea = next_head(ea, BADADDR);
			} while (pops < 4 && decode_insn(&insn, ea) > 0 && insn.itype == NN_pop && isRegister(insn.ops[0], kEcx));
			found.push_back(4 * pops);
		}
	}
	return found;
}

std::vector<int> AsmFacts::resultReads(ea_t function)
{
	std::vector<int> found;
	for (ea_t site : callSites(function))
	{
		int result = 0;
		ea_t ea = site;
		for (int k = 0; k < 12; ++k)
		{
			ea = next_head(ea, BADADDR);
			insn_t insn;
			if (decode_insn(&insn, ea) <= 0)
				break;
			std::string_view mnem = insn.get_canon_mnem(PH);
			if (mnem.starts_with('f') && insn.itype != NN_fnstsw)
			{
				result = kSt0;
				break;
			}
			RegisterUse use(insn);
			if (use.reads[kEax])
			{
				result = use.reads[kEax];
				break;
			}
			// any call writes eax, an indirect one too (`call dword ptr [...]`); the flow leaves at any jmp
			if (use.writes[kEax] || insn.itype == NN_jmp || insn.itype == NN_jmpni || insn.itype == NN_retn ||
				insn.itype == NN_call || insn.itype == NN_callni || insn.itype == NN_callfi)
				break;
		}
		found.push_back(result);
	}
	return found;
}

bool AsmFacts::passesOwnThis(ea_t callSite) const
{
	func_t* f = get_func(callSite);
	if (!f)
		return false;
	FlowChart chart(f);
	const std::vector<FlowChart::Block>& blocks = chart.blocks();
	auto blockOf = [&](ea_t ea) {
		auto found = std::find_if(blocks.begin(), blocks.end(),
								  [&](const FlowChart::Block& b) { return b.start <= ea && ea < b.end; });
		return found == blocks.end() ? -1 : static_cast<int>(found - blocks.begin());
	};
	int entry = blockOf(f->start_ea), site = blockOf(callSite);
	if (entry < 0 || site < 0)
		return false;
	// where the entry block keeps ecx before writing it: frame slots and registers
	std::set<int64_t> slots;
	std::set<int> registers;
	bool ecxWritten = false;
	for (ea_t ea : heads(blocks[entry].start, blocks[entry].end))
	{
		insn_t insn;
		if (ea >= callSite || decode_insn(&insn, ea) <= 0)
			break;
		RegisterUse use(insn);
		if (insn.itype == NN_mov && isRegister(insn.ops[1], kEcx))
		{
			if (std::optional<int64_t> slot = frameSlot(insn, insn.ops[0]))
				slots.insert(*slot);
			else if (insn.ops[0].type == o_reg)
				registers.insert(family(insn.ops[0].reg));
		}
		if (use.writes[kEcx])
		{
			ecxWritten = true;
			break;
		}
	}
	if (site == entry && !ecxWritten)
		return true;
	// the last write to ecx in the call's block
	insn_t last;
	bool found = false;
	for (ea_t ea = prev_head(callSite, blocks[site].start); ea != BADADDR && ea >= blocks[site].start;
		 ea = prev_head(ea, blocks[site].start))
	{
		insn_t insn;
		if (decode_insn(&insn, ea) > 0 && RegisterUse(insn).writes[kEcx])
		{
			last = insn;
			found = true;
			break;
		}
	}
	if (!found || last.itype != NN_mov)
		return false;
	std::optional<int64_t> slot = frameSlot(last, last.ops[1]);
	bool fromSlot = slot && slots.count(*slot);
	bool fromRegister = last.ops[1].type == o_reg && registers.count(family(last.ops[1].reg));
	if (!fromSlot && !fromRegister)
		return false;
	// the home is written once, by the entry block's save
	int writes = 0;
	for (ea_t ea : heads(f->start_ea, f->end_ea))
	{
		insn_t insn;
		if (decode_insn(&insn, ea) <= 0)
			continue;
		if (fromSlot && insn.ops[0].type != o_reg && insn.itype == NN_mov && frameSlot(insn, insn.ops[0]) == slot)
			++writes;
		if (fromRegister && RegisterUse(insn).writes[family(last.ops[1].reg)])
			++writes;
	}
	return writes == 1;
}

}  // namespace idakit
