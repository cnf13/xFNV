// Floats: a floating constant the exe reads as a float, from a 4-byte memory operand of an x87 or SSE instruction
// (`fld dword`, `fcomp dword`, `fmul dword`, ..., `movss`), whose value is exactly that float, is a float: src writes
// it as its shortest `f` literal (`0.6f` for 0.6000000238418579; .omp/rules/porting.md "x87"). A constant the exe
// reads from a qword is a double, even where its value is a float's, printed as the qword's value where Hex-Rays
// rounded it to a float (a qword 0.2 compared with a float). The instruction is the constant's own where Hex-Rays
// keeps its address; most constants have no address of their own (a compare's, a store's): the address is the nearest
// enclosing item's, else the statement's. The instruction there where it reads the value from memory; else, where it
// takes the value from a register, the load into that register (`movss xmm0, [c]` ... `mulss xmm0, xmm2`, an `fld`
// whose st register a later op consumes); else the one nearest to the address reading its value in its block. That
// width always wins. A constant no instruction of its block reads (fld1, fldz, a pushed or stored immediate, a constant
// Hex-Rays computed) is a float where its value is exactly a float's and Hex-Rays types it float or puts it in a
// float's place (cast to a float, assigned to one, compared with one, returned as one, passed for a src float
// parameter); another call argument keeps the parameter's type (an overloaded callee takes no other) where that is
// exactly its value; any other takes the width at which all the function's instructions reading its value read it,
// else it is C++'s unsuffixed literal, a double. No value that is not exactly a float's is ever a float.

#include "EmitPasses.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <optional>
#include <vector>

#include <gdl.hpp>

namespace idakit
{

namespace
{

// the x87 and SSE instructions that read a float operand from memory as a floating value (not fild's integer)
bool readsFloat(uint16_t itype)
{
	switch (itype)
	{
	case NN_fld:
	case NN_fadd:
	case NN_fsub:
	case NN_fsubr:
	case NN_fmul:
	case NN_fdiv:
	case NN_fdivr:
	case NN_fcom:
	case NN_fcomp:
	case NN_movss:
	case NN_addss:
	case NN_subss:
	case NN_mulss:
	case NN_divss:
	case NN_minss:
	case NN_maxss:
	case NN_comiss:
	case NN_ucomiss:
	case NN_sqrtss:
	case NN_movsd:
	case NN_addsd:
	case NN_subsd:
	case NN_mulsd:
	case NN_divsd:
	case NN_comisd:
	case NN_ucomisd:
	case NN_cvtss2sd:
	case NN_cvtsd2ss:
		return true;
	default:
		return false;
	}
}

// the xmm register op names (IDA's o_reg with an R_xmm number), else -1
int xmmOf(const op_t& op)
{
	return (op.type == o_reg || op.type == o_xmmreg) && is_xmmreg(op.reg) ? static_cast<int>(op.reg) : -1;
}

// j of the st(j) op names (IDA's o_fpreg with j, or an R_st register), else -1
int stOf(const op_t& op)
{
	if (op.type == o_fpreg)
		return static_cast<int>(op.reg);
	return op.type == o_reg && is_fpureg(op.reg) ? static_cast<int>(op.reg) - R_st0 : -1;
}

// the instruction has a memory operand
bool memoryOperand(const insn_t& insn)
{
	for (const op_t& op : insn.ops)
		if (op.type == o_mem || op.type == o_phrase || op.type == o_displ)
			return true;
	return false;
}

// j of an x87 instruction's st(j) register operand (IDA lists the implicit st(0) beside it: the highest), else
// fallback
int stArgument(const insn_t& insn, int fallback)
{
	int j = -1;
	for (const op_t& op : insn.ops)
		j = std::max(j, stOf(op));
	return j >= 0 ? j : fallback;
}

// SSE instructions that only read their first xmm operand (they set flags)
bool comparesOnly(uint16_t itype)
{
	return itype == NN_comiss || itype == NN_ucomiss || itype == NN_comisd || itype == NN_ucomisd;
}

// SSE register copies: the destination's low lane is the source's
bool copiesXmm(uint16_t itype)
{
	return itype == NN_movss || itype == NN_movsd || itype == NN_movaps || itype == NN_movapd || itype == NN_movups ||
		   itype == NN_movupd;
}

// SSE instructions that only write their first operand (their value comes from the second)
bool writesOnly(uint16_t itype)
{
	switch (itype)
	{
	case NN_sqrtss:
	case NN_sqrtsd:
	case NN_rsqrtss:
	case NN_rcpss:
	case NN_cvtss2sd:
	case NN_cvtsd2ss:
	case NN_cvtsi2ss:
	case NN_cvtsi2sd:
	case NN_movd:
	case NN_movq:
		return true;
	default:
		return copiesXmm(itype);
	}
}

// x87 instructions that push a value they do not take from the stack
bool pushesNew(uint16_t itype)
{
	switch (itype)
	{
	case NN_fild:
	case NN_fbld:
	case NN_fldz:
	case NN_fld1:
	case NN_fldpi:
	case NN_fldl2t:
	case NN_fldl2e:
	case NN_fldlg2:
	case NN_fldln2:
		return true;
	default:
		return false;
	}
}

// an x87 instruction (every x87 mnemonic starts with `f`)
bool touchesX87(const insn_t& insn)
{
	qstring mnemonic;
	return print_insn_mnem(&mnemonic, insn.ea) && !mnemonic.empty() && mnemonic[0] == 'f';
}

// the floating value the instruction at ea reads from a 4- or 8-byte memory operand (one of readsFloat's), whether it
// is a float, and whether the instruction compares (fcom, fcomp, comiss, ...). The operand's size is the width: IDA
// types an x87 operand dt_float/dt_double but an SSE one dt_dword/dt_qword. A `movss`/`movsd` storing to memory reads
// none.
struct Operand
{
	double value = 0;
	bool single = false;
	bool compare = false;
};
std::optional<Operand> floatOperand(ea_t ea)
{
	insn_t insn;
	if (ea == BADADDR || decode_insn(&insn, ea) <= 0 || !readsFloat(insn.itype))
		return std::nullopt;
	bool compare = insn.itype == NN_fcom || insn.itype == NN_fcomp || insn.itype == NN_comiss ||
				   insn.itype == NN_ucomiss || insn.itype == NN_comisd || insn.itype == NN_ucomisd;
	bool store = (insn.itype == NN_movss || insn.itype == NN_movsd) && insn.ops[0].type == o_mem;
	for (const op_t& op : insn.ops)
		if (op.type != o_mem || (store && &op == &insn.ops[0]))
			continue;
		else if (size_t size = get_dtype_size(op.dtype); size == 4)
		{
			uint32_t bits = get_dword(op.addr);
			float value;
			std::memcpy(&value, &bits, sizeof value);
			return Operand{value, true, compare};
		}
		else if (size == 8)
		{
			uint64_t bits = get_qword(op.addr);
			double value;
			std::memcpy(&value, &bits, sizeof value);
			return Operand{value, false, compare};
		}
	return std::nullopt;
}

// the same floating value: equal, or NaNs of the same bits (a NaN never equals itself)
bool same(double a, double b)
{
	return a == b || (std::isnan(a) && std::isnan(b) && std::bit_cast<uint64_t>(a) == std::bit_cast<uint64_t>(b));
}

// the operand's value is the constant's: the same, or a double Hex-Rays rounded to a float (a qword 0.2 compared
// with a float shows as 0.2000000029802322)
bool reads(const Operand& read, double value)
{
	return same(read.value, value) || (!read.single && same(static_cast<double>(static_cast<float>(read.value)), value));
}

// how many blocks back from a comparison's block the compare reading its constant is looked for (a compare whose
// result is stored, `v = a < 0.5`: compare and jump, the two stores of 1 and 0, the block Hex-Rays gives the address)
constexpr int kCompareBlocks = 3;

// the instructions of the function that read a floating value from memory, the values it loads or stores without
// reading memory (fldz, fld1, a dword immediate stored or pushed: its bits as a float), and its blocks
struct Reads
{
	std::vector<std::pair<ea_t, Operand>> at;  // by address
	std::vector<std::pair<ea_t, float>> immediates;
	qflow_chart_t chart;

	explicit Reads(ea_t start)
	{
		func_t* f = get_func(start);
		if (!f)
			return;
		func_item_iterator_t items;
		for (bool ok = items.set(f); ok; ok = items.next_code())
		{
			ea_t ea = items.current();
			if (std::optional<Operand> read = floatOperand(ea))
				at.emplace_back(ea, *read);
			else if (insn_t insn; decode_insn(&insn, ea) > 0)
			{
				if (insn.itype == NN_fldz || insn.itype == NN_fld1)
					immediates.emplace_back(ea, insn.itype == NN_fldz ? 0.0f : 1.0f);
				else if ((insn.itype == NN_push && insn.ops[0].type == o_imm) ||
						 (insn.itype == NN_mov && insn.ops[0].dtype == dt_dword && insn.ops[1].type == o_imm &&
						  (insn.ops[0].type == o_mem || insn.ops[0].type == o_phrase || insn.ops[0].type == o_displ)))
				{
					uint32_t bits = static_cast<uint32_t>(insn.ops[insn.itype == NN_push ? 0 : 1].value);
					float value;
					std::memcpy(&value, &bits, sizeof value);
					immediates.emplace_back(ea, value);
				}
			}
		}
		std::sort(at.begin(), at.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
		std::sort(immediates.begin(), immediates.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
		chart.create("", f, BADADDR, BADADDR, FC_NOEXT);  // with predecessors (no FC_NOPREDS)
	}

	// the block holding ea, else -1
	int blockOf(ea_t ea) const
	{
		for (int k = 0; k < chart.size(); ++k)
			if (ea >= chart.blocks[k].start_ea && ea < chart.blocks[k].end_ea)
				return k;
		return -1;
	}

	// ea's block loads or stores value without reading memory
	bool immediate(ea_t ea, double value) const
	{
		int k = blockOf(ea);
		if (k < 0)
			return false;
		auto first = std::lower_bound(immediates.begin(), immediates.end(), chart.blocks[k].start_ea,
									  [](const auto& a, ea_t b) { return a.first < b; });
		for (auto i = first; i != immediates.end() && i->first < chart.blocks[k].end_ea; ++i)
			if (same(static_cast<double>(i->second), value))
				return true;
		return false;
	}

	// the instruction nearest to ea in block k reading value (only a compare where compare)
	std::optional<Operand> inBlock(int k, ea_t ea, double value, bool compare) const
	{
		const qbasic_block_t& block = chart.blocks[k];
		std::optional<Operand> best;
		ea_t distance = BADADDR;
		auto first = std::lower_bound(at.begin(), at.end(), block.start_ea,
									  [](const auto& a, ea_t b) { return a.first < b; });
		for (auto read = first; read != at.end() && read->first < block.end_ea; ++read)
			if (ea_t d = read->first < ea ? ea - read->first : read->first - ea;
				reads(read->second, value) && (!compare || read->second.compare) && d < distance)
				best = read->second, distance = d;
		return best;
	}

	// walks back from ea through its block, then while a block has one predecessor (kCompareBlocks blocks), calling
	// step on each instruction until it returns false
	template <class Step>
	void walkBack(ea_t ea, Step step) const
	{
		int k = blockOf(ea);
		for (int blocks = 0; k >= 0 && blocks <= kCompareBlocks; ++blocks)
		{
			for (ea_t at = prev_head(ea, chart.blocks[k].start_ea); at != BADADDR;
				 at = prev_head(at, chart.blocks[k].start_ea))
				if (insn_t insn; decode_insn(&insn, at) <= 0 || !step(insn))
					return;
			if (chart.npred(k) != 1)
				return;
			k = chart.pred(k, 0);
			ea = chart.blocks[k].end_ea;
		}
	}

	// the memory read of value an xmm register holds at ea: a movss/movsd load into it, through register copies
	std::optional<Operand> sseLoad(ea_t ea, int reg, double value) const
	{
		std::optional<Operand> found;
		walkBack(ea, [&](const insn_t& insn) {
			if (insn.itype == NN_call)
				return false;
			if (xmmOf(insn.ops[0]) != reg || comparesOnly(insn.itype))
				return true;
			if ((insn.itype == NN_movss || insn.itype == NN_movsd) && insn.ops[1].type == o_mem)
			{
				if (std::optional<Operand> read = floatOperand(insn.ea); read && reads(*read, value))
					found = read;
				return false;
			}
			if (copiesXmm(insn.itype) && xmmOf(insn.ops[1]) >= 0)
				return reg = xmmOf(insn.ops[1]), true;
			return false;
		});
		return found;
	}

	// the memory read of value the x87 register st(slot) holds at ea: the fld that pushed it, through the stack's
	// pushes, pops, exchanges and copies; any instruction computing it, or one whose effect is not modelled, ends it
	std::optional<Operand> x87Load(ea_t ea, int slot, double value) const
	{
		std::optional<Operand> found;
		walkBack(ea, [&](const insn_t& insn) {
			// st(j) of the instruction's operand n, else st(fallback)
			auto st = [&](int n, int fallback) {
				int j = stOf(insn.ops[n]);
				return j >= 0 ? j : fallback;
			};
			if (pushesNew(insn.itype))
				return slot > 0 && (--slot, true);
			switch (insn.itype)
			{
			case NN_fld:
				if (slot > 0)
					return --slot, true;
				if (!memoryOperand(insn))
					return slot = stArgument(insn, 1), true;
				if (std::optional<Operand> read = floatOperand(insn.ea); read && reads(*read, value))
					found = read;
				return false;
			case NN_fst:
				if (!memoryOperand(insn) && slot == stArgument(insn, 1))
					slot = 0;
				return true;
			case NN_fstp:
				if (int j = memoryOperand(insn) ? 0 : stArgument(insn, 1); j > 0 && slot == j - 1)
					return slot = 0, true;
				return ++slot, true;
			case NN_fistp:
			case NN_fbstp:
			case NN_fcomp:
			case NN_fucomp:
			case NN_ficomp:
			case NN_fcomip:
			case NN_fucomip:
				return ++slot, true;
			case NN_fcompp:
			case NN_fucompp:
				return slot += 2, true;
			case NN_fcom:
			case NN_fucom:
			case NN_ficom:
			case NN_fcomi:
			case NN_fucomi:
			case NN_ftst:
			case NN_fxam:
			case NN_fist:
			case NN_fstsw:
			case NN_fnstsw:
			case NN_fldcw:
			case NN_fstcw:
			case NN_fnstcw:
				return true;
			case NN_fadd:
			case NN_fsub:
			case NN_fsubr:
			case NN_fmul:
			case NN_fdiv:
			case NN_fdivr:
			case NN_fiadd:
			case NN_fisub:
			case NN_fisubr:
			case NN_fimul:
			case NN_fidiv:
			case NN_fidivr:
				return slot != st(0, 0);
			case NN_faddp:
			case NN_fsubp:
			case NN_fsubrp:
			case NN_fmulp:
			case NN_fdivp:
			case NN_fdivrp:
				return slot != st(0, 1) - 1 && (++slot, true);
			case NN_fsqrt:
			case NN_fabs:
			case NN_fchs:
			case NN_frndint:
			case NN_fsin:
			case NN_fcos:
			case NN_fscale:
			case NN_fprem:
			case NN_fprem1:
			case NN_f2xm1:
				return slot != 0;
			case NN_fxch:
				if (int j = stArgument(insn, 1); slot == 0 || slot == j)
					slot = slot == 0 ? j : 0;
				return true;
			default:
				return insn.itype != NN_call && !touchesX87(insn);
			}
		});
		return found;
	}

	// the memory read of value the instruction at ea takes from a register: an SSE instruction's xmm source operand
	// loaded by a movss/movsd, an x87 instruction's st(0) or st(j) operand pushed by an fld
	std::optional<Operand> loaded(ea_t ea, double value) const
	{
		insn_t insn;
		if (decode_insn(&insn, ea) <= 0)
			return std::nullopt;
		// an x87 instruction takes st(0) and its st(j) operand; a push takes nothing from the stack but `fld st(j)`
		if (touchesX87(insn))
		{
			if (insn.itype == NN_fld || pushesNew(insn.itype))
				return insn.itype == NN_fld && !memoryOperand(insn) ? x87Load(ea, stArgument(insn, 1), value)
																	: std::nullopt;
			if (std::optional<Operand> read = x87Load(ea, 0, value))
				return read;
			for (const op_t& op : insn.ops)
				if (int j = stOf(op); j > 0)
					if (std::optional<Operand> read = x87Load(ea, j, value))
						return read;
			return std::nullopt;
		}
		for (const op_t& op : insn.ops)
			if (int reg = xmmOf(op); reg >= 0 && (&op != &insn.ops[0] || !writesOnly(insn.itype)))
				if (std::optional<Operand> read = sseLoad(ea, reg, value))
					return read;
		return std::nullopt;
	}

	// the instruction reading value whose width decides the constant at ea: the one at ea where it reads value from
	// memory, else the load of the register it takes value from (loaded), else the nearest to ea in ea's block reading
	// value; for a comparison's constant, else a compare reading it in the blocks before (kCompareBlocks back, the
	// nearest first)
	std::optional<Operand> nearest(ea_t ea, double value, bool comparison) const
	{
		int home = blockOf(ea);
		if (home < 0)
			return std::nullopt;
		if (std::optional<Operand> own = floatOperand(ea); own && reads(*own, value))
			return own;
		if (std::optional<Operand> read = loaded(ea, value))
			return read;
		if (std::optional<Operand> read = inBlock(home, ea, value, false); read || !comparison)
			return read;
		std::vector<int> level{home}, seen{home};
		for (int depth = 0; depth < kCompareBlocks && !level.empty(); ++depth)
		{
			std::vector<int> next;
			for (int k : level)
				for (int p = 0; p < chart.npred(k); ++p)
					if (int b = chart.pred(k, p); std::find(seen.begin(), seen.end(), b) == seen.end())
						next.push_back(b), seen.push_back(b);
			for (int k : next)
				if (std::optional<Operand> read = inBlock(k, chart.blocks[k].end_ea, value, true))
					return read;
			level = std::move(next);
		}
		return std::nullopt;
	}

	// the one width at which all the function's instructions reading value read it (a double: its value)
	std::optional<Operand> all(double value) const
	{
		std::optional<Operand> one;
		for (const auto& [ea, read] : at)
			if (reads(read, value))
			{
				if (one && one->single != read.single)
					return std::nullopt;
				one = read;
			}
		return one;
	}
};

}  // namespace

void resolveFloatConstants(Function& function, Context& context)
{
	std::unordered_map<const cexpr_t*, const citem_t*> parents = parentsOf(function);
	Reads exeReads(function.ea);
	// a call's argument (through casts): Hex-Rays types it as the callee's parameter
	auto argument = [&](const cexpr_t* e) {
		auto parent = parents.find(e);
		while (parent != parents.end() && parent->second->op == cot_cast)
			parent = parents.find(static_cast<const cexpr_t*>(parent->second));
		return parent != parents.end() && parent->second->op == cot_call &&
			   static_cast<const cexpr_t*>(parent->second)->x != e;
	};
	// a float place for e: a cast to a float on its way up (Hex-Rays' `(float)1.0` for a raw call's argument), past
	// its casts the right side of an assignment to a float, an operand of a comparison whose other operand is a float
	// (itself or past any of its casts), the value a function returning a float returns, or an argument of a call the
	// Calls pass resolved whose src parameter is a float
	auto floatPlace = [&](const cexpr_t* e) {
		const cexpr_t* value = e;
		auto parent = parents.find(value);
		while (parent != parents.end() && parent->second->op == cot_cast)
		{
			value = static_cast<const cexpr_t*>(parent->second);
			if (value->type.is_float())
				return true;
			parent = parents.find(value);
		}
		if (parent == parents.end())
			return false;
		if (tinfo_t type; parent->second->op == cit_return)
			return function.cfunc->get_func_type(&type) && type.get_rettype().is_float();
		if (!parent->second->is_expr())
			return false;
		const cexpr_t* use = static_cast<const cexpr_t*>(parent->second);
		if (use->op >= cot_asg && use->op <= cot_asgumod && use->y == value)
			return use->x->type.is_float();
		if (use->op == cot_eq || use->op == cot_ne || (use->op >= cot_sge && use->op <= cot_ult))
		{
			const cexpr_t* other = use->x == value ? use->y : use->x;
			while (other->op == cot_cast && !other->type.is_float())
				other = other->x;
			return other->type.is_float();
		}
		auto resolved = function.meaning.find(use);
		const Call* call = resolved != function.meaning.end() ? std::get_if<Call>(&resolved->second) : nullptr;
		if (use->op != cot_call || !call || !call->function)
			return false;
		std::optional<SrcTypes::Function> type = SrcTypes::function(call->function->type);
		for (size_t k = 0; type && k < use->a->size(); ++k)
			if (&(*use->a)[k] == value)
			{
				int parameter = parameterIndex(*call, k);
				return parameter >= 0 && parameter < static_cast<int>(type->params.size()) &&
					   context.types.canonical(type->params[parameter]) == "builtin:float";
			}
		return false;
	};
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t* statement) {
		if (e->op != cot_fnum)
			return;
		double value = 0;
		if (e->fpc->fnum.to_double(&value) != REAL_ERROR_OK)
			return;
		auto mean = [&](const Operand& read) {
			function.meaning[e] = std::string(read.single ? "builtin:float" : "builtin:double");
			if (!same(read.value, value))
				function.floatValues[e] = read.value;
		};
		// the constant is the one an instruction reads (not one Hex-Rays computed from it: a negation, a fold): the
		// constant's own, else (Reads::nearest) the one at its address or the load of the register that one takes it
		// from, else the nearest one reading its value in the block of its address (Hex-Rays has none for most
		// constants: the nearest item above it with one, else its statement's); a comparison's, else a compare
		// reading it in the blocks before
		if (std::optional<Operand> read = floatOperand(e->ea); read && reads(*read, value))
			return mean(*read);
		ea_t anchor = e->ea;
		bool comparison = false, direct = true;  // e an operand of a comparison; e reached through casts only
		for (auto parent = parents.find(e); parent != parents.end();)
		{
			int op = parent->second->op;
			if (direct && (op == cot_eq || op == cot_ne || (op >= cot_sge && op <= cot_ult)))
				comparison = true;
			if (op != cot_cast)
				direct = false;
			if (anchor == BADADDR)
				anchor = parent->second->ea;
			if (anchor != BADADDR && !direct)
				break;
			parent = parent->second->is_expr() ? parents.find(static_cast<const cexpr_t*>(parent->second))
											   : parents.end();
		}
		if (anchor == BADADDR && statement)
			anchor = statement->ea;
		if (std::optional<Operand> read = anchor != BADADDR ? exeReads.nearest(anchor, value, comparison)
															: std::nullopt)
			return mean(*read);
		// none in its block: unless its block makes the value without reading memory (fldz, fld1, an immediate), the
		// width at which all the function's instructions reading the value read it (one loaded once and kept on the
		// x87 stack); else a float where it is exactly a float's value and Hex-Rays types it float or puts it in a
		// float place; a call's argument the parameter's literal (resolveTypes' type: an overloaded callee,
		// `pow(float, float)`, takes no double) where that is exactly its value; else C++'s unsuffixed literal, a
		// double
		if (anchor == BADADDR || !exeReads.immediate(anchor, value))
			if (std::optional<Operand> read = exeReads.all(value))
				return mean(*read);
		bool exact = same(static_cast<double>(static_cast<float>(value)), value);
		if (exact && (e->type.is_float() || floatPlace(e)))
			return mean(Operand{value, true});
		if (argument(e))
		{
			auto typed = function.meaning.find(e);
			const std::string* key = typed != function.meaning.end() ? std::get_if<std::string>(&typed->second) : nullptr;
			if (!exact && key && *key == "builtin:float")
				mean(Operand{value, false});
			return;
		}
		mean(Operand{value, false});
	});
}

}  // namespace idakit
