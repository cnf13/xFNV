// The calls whose arguments come from a stack-slot model of the caller's code: each direct call of a C-variadic
// function typed with the arguments the exe passes there (Hex-Rays otherwise guesses the count from the stack and the
// cleanup: InitializationFlow's callers, an empty variadic stub), and each indirect call Hex-Rays passes more stack
// bytes than the exe pushes there (a local stored at the top of the frame taken for an argument).

#include "idakit/Unfold.hpp"

#include "IdaSupport.hpp"

#include "idakit/Asm.hpp"
#include "idakit/AsmFacts.hpp"

#include <Facts.hpp>

#include <algorithm>
#include <set>

namespace idakit
{

namespace
{

// One stack slot of the model: what one push or `sub esp, N` put there, and whose it is: a call's argument until a
// call takes it, or the frame's (what the prologue puts there, a saved register). A C-variadic call takes every
// argument slot on top, which may hold an enclosing call's arguments too: those it takes are its own kind
struct Slot
{
	enum Owner : uint8_t
	{
		kArgument,
		kTaken,
		kVariadicTaken,
		kFrame
	};
	int64_t size;
	Owner owner;
	bool operator==(const Slot&) const = default;
};

// the model's stack, the top last; unknown where esp moved in a way the model does not follow
struct Stack
{
	bool known = true;
	std::vector<Slot> slots;
	// the doubles stored into the slots, by their position (bytes from the bottom)
	std::set<int64_t> doubles;
	std::string why;  // unknown: the first reason

	bool operator==(const Stack& other) const
	{
		return known == other.known && slots == other.slots && doubles == other.doubles;
	}

	int64_t bytes() const
	{
		int64_t n = 0;
		for (const Slot& slot : slots)
			n += slot.size;
		return n;
	}

	// the slots' sizes bottom up, a taken one marked `*`, one a C-variadic call took `+`, a frame one `^`
	std::string text() const
	{
		std::string out;
		for (const Slot& slot : slots)
			out += (out.empty() ? "" : " ") + std::to_string(slot.size) +
				   (slot.owner == Slot::kTaken			? "*"
					: slot.owner == Slot::kVariadicTaken ? "+"
					: slot.owner == Slot::kFrame		 ? "^"
														 : "");
		return out;
	}

	void unknown(const std::string& reason)
	{
		if (known)
			why = reason;
		known = false;
		slots.clear();
		doubles.clear();
	}

	// n bytes off the top (a slot split where n ends inside it)
	void remove(int64_t n, ea_t at)
	{
		while (known && n > 0)
		{
			if (slots.empty())
				return unknown("esp above the entry at " + hex(at));
			Slot& top = slots.back();
			if (top.size > n)
			{
				top.size -= n;
				break;
			}
			n -= top.size;
			slots.pop_back();
		}
		std::erase_if(doubles, [top = bytes()](int64_t position) { return position + 8 > top; });
	}

	void push(int64_t size, bool frame) { slots.push_back({size, frame ? Slot::kFrame : Slot::kArgument}); }

	// esp moved by n (`add esp, n`; a negative n allocates a slot, as `sub esp, -n` does)
	void adjust(int64_t n, bool frame, ea_t at)
	{
		if (n > 0)
			remove(n, at);
		else if (n < 0)
			push(-n, frame);
	}

	// the slots holding the top n bytes taken by a call, as owner
	void take(int64_t n, Slot::Owner owner = Slot::kTaken)
	{
		for (auto slot = slots.rbegin(); slot != slots.rend() && n > 0; ++slot)
		{
			if (slot->owner == Slot::kArgument)
				slot->owner = owner;
			n -= slot->size;
		}
	}

	// A store of size bytes at [esp+offset]: a slot a call took holds an argument again (MSVC reuses a cdecl call's
	// argument slot for the next call's), and a double stored there is one
	void store(int64_t offset, int64_t size, bool isDouble)
	{
		int64_t end = bytes() - offset, start = end - size;
		if (start < 0 || offset < 0)
			return;
		int64_t position = 0;
		for (Slot& slot : slots)
		{
			if ((slot.owner == Slot::kTaken || slot.owner == Slot::kVariadicTaken) && position < end &&
				position + slot.size > start)
				slot.owner = Slot::kArgument;
			position += slot.size;
		}
		std::erase_if(doubles, [&](int64_t at) { return at < end && at + 8 > start; });
		if (isDouble && size == 8)
			doubles.insert(start);
	}
};

// The end of the function's prologue in its entry block: what a push or `sub esp` puts there before it is the
// frame's (saved registers, locals, the exception registration), never a call's argument. It is the leading run of
// MSVC's frame setup (pushes of the entry's ebp, ebx, esi, edi and ecx, the last a local's slot; `mov` into ebp, ebx
// or memory; `sub`, `and`, `add` on esp with an immediate), extended to the exception registration's `mov fs:[0]`
// where the entry block holds one.
ea_t prologueEnd(exefacts::Facts& facts, func_t* function, ea_t blockEnd)
{
	ea_t end = function->start_ea;
	std::set<uint16> written;
	for (ea_t ea = function->start_ea; ea < blockEnd; ea = get_item_end(ea))
	{
		insn_t insn;
		if (decode_insn(&insn, ea) <= 0)
			break;
		const op_t& op0 = insn.ops[0];
		const op_t& op1 = insn.ops[1];
		bool frameRegister = op0.type == o_reg && (op0.reg == R_bp || op0.reg == R_bx || op0.reg == R_si ||
												   op0.reg == R_di || op0.reg == R_cx);
		bool setup = false;
		if (insn.itype == NN_push)
			setup = frameRegister && !written.contains(op0.reg);
		else if (insn.itype == NN_mov)
			setup = (op0.type == o_reg && (op0.reg == R_bp || op0.reg == R_bx)) || op0.type == o_displ ||
					op0.type == o_phrase || op0.type == o_mem;
		else if (insn.itype == NN_sub || insn.itype == NN_and || insn.itype == NN_add)
			setup = op0.type == o_reg && op0.reg == R_sp && op1.type == o_imm;
		if (!setup)
			break;
		if (op0.type == o_reg && insn.itype != NN_push)
			written.insert(op0.reg);
		end = get_item_end(ea);
	}
	try
	{
		const std::optional<exefacts::EhFrame>& eh = facts.ehFrame(static_cast<uint32_t>(function->start_ea));
		if (eh && eh->kind != exefacts::EhFrame::kNone && !eh->registration.empty())
			if (ea_t last = *std::max_element(eh->registration.begin(), eh->registration.end()); last < blockEnd)
				end = std::max(end, get_item_end(last));
	}
	catch (const exefacts::Raised&)
	{
	}
	return end;
}

}  // namespace

std::optional<Unfold::CallBytes> Unfold::callBytes(func_t* function, ea_t call, const std::map<ea_t, int64_t>& indirect)
{
	insn_t insn;
	decode_insn(&insn, call);
	const op_t& target = insn.ops[0];
	if (target.type == o_near || target.type == o_far)
	{
		uint32_t callee = static_cast<uint32_t>(target.addr);
		try
		{
			// its `ret N`, through a short thunk or an import thunk
			if (uint32_t pops = m_facts.calleePops(callee))
				return CallBytes{pops, 0};
			const std::vector<int32_t>& args = m_facts.abi(callee).args;
			return CallBytes{0, args.empty() ? 0 : 4 * (int64_t(*std::max_element(args.begin(), args.end())) + 1)};
		}
		catch (const exefacts::Raised&)
		{
		}
		if (std::optional<tinfo_t> type = srcPrototype(target.addr))
			if (func_type_data_t fd; type->get_func_details(&fd))
			{
				int purged = type->calc_purged_bytes();
				return purged > 0 ? CallBytes{purged, 0} : CallBytes{0, static_cast<int64_t>(fd.stkargs)};
			}
		return std::nullopt;
	}
	// an import, `call ds:[IAT slot]`
	if (target.type == o_mem)
		if (std::optional<uint32_t> pops = m_facts.program().importPops(static_cast<uint32_t>(target.addr)))
			return CallBytes{*pops, 0};
	if (std::optional<int64_t> pops = indirectPops(function, call))
		return CallBytes{*pops, 0};
	if (auto pops = indirect.find(call); pops != indirect.end())
		return CallBytes{pops->second, 0};
	return std::nullopt;
}

// The stack-slot model: a forward walk over the flow chart carries the stack of slots, a push one of 4 bytes, a
// `sub esp, N` one of N; `add esp, N`, `lea esp, [esp+N]` and `pop` remove bytes from the top; a call the exe shows
// the bytes of removes what its callee pops and marks what a cdecl callee reads taken; a store into a taken slot
// makes it an argument again (MSVC reuses the slot for the next call); a call whose bytes neither the exe nor src
// shows takes nothing where no argument slot is on top; any other esp write, any other such call, and a join of
// differing stacks leave the stack unknown. A call's arguments are the slots on top down to the first that is no
// argument: one taken, or the frame's (those the prologue puts there, and a push of a callee-saved register no path
// to it has touched yet and the function does not take as an input: a late save, as exefacts' stack walk reads it).
// A C-variadic callee takes all of them, so another call's count that stops at slots one took (they may be that
// call's too) is unknown. The slots pending for an enclosing call count as the call's own (`f(g(), x)`: g's count
// holds x).
std::map<ea_t, Unfold::StackArguments> Unfold::stackArguments(func_t* function, const std::set<ea_t>& calls,
															  const std::map<ea_t, int64_t>& indirect)
{
	std::set<ea_t> variadic;
	for (ea_t item : itemHeads(function))
	{
		insn_t insn;
		if (decode_insn(&insn, item) <= 0 || !is_call_insn(insn))
			continue;
		xrefblk_t xb;
		for (bool ok = xb.first_from(item, XREF_NOFLOW); ok; ok = xb.next_from())
			if (xb.iscode && m_variadic.contains(xb.to))
				variadic.insert(item);
	}

	FlowChart chart(function);
	const std::vector<FlowChart::Block>& blocks = chart.blocks();
	ea_t prologue = prologueEnd(m_facts, function, blocks[0].end);
	// the register families each block's entry may see touched (read or written) on a path from the function's entry,
	// and those the function writes outside a push or pop (the registers it has a reason to save)
	std::vector<uint8_t> touches(blocks.size()), touchedIn(blocks.size());
	uint8_t written = 0;
	for (size_t k = 0; k < blocks.size(); ++k)
		for (ea_t ea : heads(blocks[k].start, blocks[k].end))
			if (insn_t insn; decode_insn(&insn, ea) > 0)
			{
				RegisterUse use(insn);
				bool stackOp = insn.itype == NN_push || insn.itype == NN_pop;
				for (int f = 0; f < 8; ++f)
				{
					if (use.reads[f] || use.writes[f])
						touches[k] |= uint8_t(1 << f);
					if (use.writes[f] && !stackOp)
						written |= uint8_t(1 << f);
				}
			}
	// a register input (exefacts' regArgs; its family bits are IDA's register numbers) is touched at the entry, so its
	// push is an argument, not a late save
	std::vector<bool> reached(blocks.size());
	reached[0] = true;
	touchedIn[0] = m_facts.regArgs(static_cast<uint32_t>(function->start_ea));
	for (std::vector<size_t> pending{0}; !pending.empty();)
	{
		size_t k = pending.back();
		pending.pop_back();
		uint8_t out = touchedIn[k] | touches[k];
		for (int succ : blocks[k].succs)
			if (get_func(blocks[succ].start) == function &&
				(!reached[succ] || (touchedIn[succ] | out) != touchedIn[succ]))
			{
				reached[succ] = true;
				touchedIn[succ] |= out;
				pending.push_back(size_t(succ));
			}
	}
	// the stack after the block from the stack before it; at each call asked for, its arguments to found
	auto walk = [&](size_t k, Stack stack, std::map<ea_t, StackArguments>* found) {
		const FlowChart::Block& block = blocks[k];
		uint8_t touched = touchedIn[k];
		for (ea_t ea : heads(block.start, block.end))
		{
			if (!stack.known)
			{
				// the calls asked for after the stack went unknown in this block: unknown for its reason
				if (found)
					for (auto call = calls.lower_bound(ea); call != calls.end() && *call < block.end; ++call)
						found->emplace(*call, StackArguments{false, {}, stack.why});
				break;
			}
			insn_t insn;
			if (decode_insn(&insn, ea) <= 0)
				continue;
			const op_t& op0 = insn.ops[0];
			const op_t& op1 = insn.ops[1];
			// the prologue's range in the entry block (a function chunk below the entry is none of it)
			bool frame = ea >= function->start_ea && ea < prologue;
			bool esp0 = op0.type == o_reg && op0.reg == R_sp;
			// a late save: the push of a callee-saved register untouched so far that the function writes; one it never
			// writes still holds the caller's value, an argument (0xCBC0E7's `push esi`, the hkOstream passed in)
			bool lateSave = insn.itype == NN_push && op0.type == o_reg &&
							(op0.reg == R_bx || op0.reg == R_bp || op0.reg == R_si || op0.reg == R_di) &&
							!(touched & (1 << op0.reg)) && (written & (1 << op0.reg));
			RegisterUse use(insn);
			for (int f = 0; f < 8; ++f)
				if (use.reads[f] || use.writes[f])
					touched |= uint8_t(1 << f);
			switch (insn.itype)
			{
			case NN_push:
			case NN_pushf:
			case NN_pushfd:
				stack.push(4, frame || lateSave);
				continue;
			case NN_pop:
			case NN_popf:
			case NN_popfd:
				stack.remove(4, ea);
				continue;
			case NN_sub:
			case NN_add:
				if (!esp0)
					break;
				if (op1.type != o_imm)
				{
					stack.unknown("esp changed by a register at " + hex(ea));
					continue;
				}
				stack.adjust(insn.itype == NN_add ? signed32(op1.value) : -signed32(op1.value), frame, ea);
				continue;
			case NN_lea:
				if (!esp0)
					break;
				if ((op1.type == o_displ || op1.type == o_phrase) && x86_base_reg(insn, op1) == R_sp &&
					x86_index_reg(insn, op1) == R_none)
				{
					stack.adjust(op1.type == o_displ ? signed32(op1.addr) : 0, frame, ea);
					continue;
				}
				stack.unknown("esp loaded at " + hex(ea));
				continue;
			case NN_and:
				// the prologue's alignment keeps the slots as they are, relative to each other
				if (esp0 && !frame)
					stack.unknown("esp aligned at " + hex(ea));
				continue;
			default:
				break;
			}
			if (is_call_insn(insn))
			{
				if (variadic.contains(ea) || (found && calls.contains(ea)))
				{
					// the arguments: the bytes of the slots on top down to the first one not an argument
					int64_t top = stack.bytes(), n = 0;
					auto slot = stack.slots.rbegin();
					for (; slot != stack.slots.rend() && slot->owner == Slot::kArgument; ++slot)
						n += slot->size;
					if (found && calls.contains(ea))
					{
						StackArguments arguments{true, {}, {}};
						// below a C-variadic call's: what it took may be this call's arguments too
						if (!variadic.contains(ea) && slot != stack.slots.rend() &&
							slot->owner == Slot::kVariadicTaken)
							arguments = StackArguments{false, {}, "below a variadic call's arguments at " + hex(ea)};
						else
							for (int64_t offset = 0; offset < n; offset += arguments.pieces.back())
								arguments.pieces.push_back(offset + 8 <= n && stack.doubles.contains(top - offset - 8)
															   ? 8
															   : std::min<int64_t>(4, n - offset));
						found->emplace(ea, arguments);
					}
					if (variadic.contains(ea))
					{
						stack.take(n, Slot::kVariadicTaken);
						continue;
					}
				}
				std::optional<CallBytes> bytes = callBytes(function, ea, indirect);
				// a call with no argument slot on top has no stack argument: it takes and pops nothing, whatever its
				// callee
				if (!bytes && (stack.slots.empty() || stack.slots.back().owner != Slot::kArgument))
					continue;
				if (!bytes)
				{
					stack.unknown("call " + hex(ea) + " takes unknown bytes");
					continue;
				}
				stack.take(bytes->takes);
				stack.remove(bytes->pops, ea);
				continue;
			}
			uint32 feature = insn.get_canon_feature(PH);
			if (esp0 && has_cf_chg(feature, 0))
				stack.unknown("esp written at " + hex(ea));
			else if (insn.itype == NN_pusha || insn.itype == NN_pushad || insn.itype == NN_popa ||
					 insn.itype == NN_popad || insn.itype == NN_enter || insn.itype == NN_leave)
				stack.unknown("esp written at " + hex(ea));
			else
				// the memory operand an instruction writes (an x87 store's is its second, after ST0)
				for (int n = 0; n < UA_MAXOP && insn.ops[n].type != o_void; ++n)
				{
					const op_t& op = insn.ops[n];
					if (has_cf_chg(feature, n) && (op.type == o_displ || op.type == o_phrase) &&
						x86_base_reg(insn, op) == R_sp && x86_index_reg(insn, op) == R_none)
						stack.store(op.type == o_displ ? signed32(op.addr) : 0, int64_t(get_dtype_size(op.dtype)),
									op.dtype == dt_double);
				}
		}
		return stack;
	};

	// the stack before each block: a flat lattice (a block whose predecessors disagree is unknown), so the fixed
	// point does not depend on the visiting order
	std::map<size_t, Stack> before{{0, Stack{}}};
	std::vector<size_t> pending{0};
	while (!pending.empty())
	{
		size_t k = pending.back();
		pending.pop_back();
		Stack after = walk(k, before.at(k), nullptr);
		for (int succ : blocks[k].succs)
		{
			if (get_func(blocks[succ].start) != function)
				continue;
			auto [at, added] = before.try_emplace(size_t(succ), after);
			if (!added)
			{
				if (!at->second.known || (after.known && at->second == after))
					continue;
				if (after.known)
					at->second.unknown("stacks differ at the join " + hex(blocks[succ].start) + ": [" +
									   at->second.text() + "] and [" + after.text() + "]");
				else
					at->second = after;
			}
			pending.push_back(size_t(succ));
		}
	}
	std::map<ea_t, StackArguments> arguments;
	for (const auto& [k, stack] : before)
	{
		if (!stack.known)
		{
			for (ea_t call : calls)
				if (call >= blocks[k].start && call < blocks[k].end)
					arguments.emplace(call, StackArguments{false, {}, stack.why});
			continue;
		}
		walk(k, stack, &arguments);
	}
	for (ea_t call : calls)
		arguments.emplace(call, StackArguments{false, {}, "unreached from the entry"});
	return arguments;
}

// A variadic call's type from the stack-slot model: the callee's fixed parameters come first, then a `double` where a
// double was stored (an x87 `fstp qword [esp+N]`) and an `int` per other dword; a non-variadic cdecl. nullopt where the
// model does not know the exe's bytes or they do not fit, with the log kind and detail in why.
std::optional<tinfo_t> Unfold::variadicSiteType(const tinfo_t& type, const StackArguments& found,
												std::pair<std::string, std::string>& why)
{
	if (!found.known)
	{
		why = {"variadic-count-unknown", found.why};
		return std::nullopt;
	}
	func_type_data_t fd;
	type.get_func_details(&fd);
	int64_t fixed = 0;
	for (const funcarg_t& arg : fd)
		fixed += stackSize(arg.type);
	int64_t passed = 0;
	for (int64_t piece : found.pieces)
		passed += piece;
	if (passed < fixed)
	{
		why = {"variadic-count-short", std::to_string(passed) + " bytes for " + text(type)};
		return std::nullopt;
	}
	func_type_data_t site;
	site.rettype = fd.rettype;
	for (const funcarg_t& arg : fd)
	{
		funcarg_t copy;
		copy.type = arg.type;
		copy.name = arg.name;
		site.push_back(copy);
	}
	// the pieces past the fixed parameters' bytes (the rest of a double they end inside is a dword)
	int64_t skip = fixed;
	bool whole = true;
	for (int64_t piece : found.pieces)
	{
		int64_t size = piece - std::min(skip, piece);
		skip -= piece - size;
		if (size == 0)
			continue;
		if (size % 4 != 0)
		{
			whole = false;
			break;
		}
		funcarg_t extra;
		extra.type = tinfo_t(size == 8 ? BTF_DOUBLE : BT_INT32);
		site.push_back(extra);
	}
	site.set_cc(CM_CC_CDECL);
	// create_func takes the arguments out of site
	tinfo_t typed;
	if (!whole || !typed.create_func(site))
	{
		why = {"variadic-count-unknown", "slots not in dwords for " + text(type)};
		return std::nullopt;
	}
	return typed;
}

// Each direct call of a C-variadic function, typed with the arguments the exe passes there (variadicSiteType) as a
// user-defined call
udcall_map_t Unfold::variadicSites(func_t* function)
{
	std::map<ea_t, ea_t> sites;
	std::set<ea_t> calls;
	for (ea_t item : itemHeads(function))
	{
		insn_t insn;
		if (decode_insn(&insn, item) <= 0 || !is_call_insn(insn))
			continue;
		xrefblk_t xb;
		for (bool ok = xb.first_from(item, XREF_NOFLOW); ok; ok = xb.next_from())
			if (xb.iscode && m_variadic.contains(xb.to))
			{
				sites.emplace(item, xb.to);
				calls.insert(item);
			}
	}
	if (sites.empty())
		return {};
	std::map<ea_t, StackArguments> arguments = stackArguments(function, calls, {});

	udcall_map_t typed;
	for (const auto& [call, callee] : sites)
	{
		tinfo_t type;
		get_tinfo(&type, callee);
		std::string key = hex(function->start_ea) + " " + hex(call);
		std::pair<std::string, std::string> why;
		std::optional<tinfo_t> site = variadicSiteType(type, arguments.at(call), why);
		if (!site)
		{
			count(why.first);
			log(why.first, key, why.second);
			continue;
		}
		func_type_data_t fd;
		type.get_func_details(&fd);
		udcall_t udc;
		udc.name = get_name(callee);
		udc.tif = *site;
		count("variadic calls typed");
		log("variadic-call", key, std::to_string(site->get_nargs() - fd.size()) + " extra: " + text(udc.tif));
		typed[call] = udc;
	}
	return typed;
}

}  // namespace idakit
