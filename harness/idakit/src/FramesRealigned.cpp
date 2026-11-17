// The realigned frames (idakit frames): the stack points of a frame aligned by `and esp`, from a flow walk of the
// real esp depths, for every game function.

#include "IdaSupport.hpp"

#include "idakit/Asm.hpp"
#include "idakit/Frames.hpp"

#include <Facts.hpp>

#include <regex>
#include <set>

namespace idakit
{

namespace
{

// The purge of an indirect call not resolved through its vtable slot (createMapping 0xCE7D5C `mov edx, [ecx]; mov
// edx, [edx+10h]; ...; call edx`), by its shape: P the bytes pushed since the previous call in its block, 0 when `add
// esp, P` follows (a cdecl call through a pointer, 0xCE8490), else P. A guess: the walk's merge and epilog checks
// reject a wrong one. nullopt when esp moves otherwise between the previous call and this one.
std::optional<int64_t> shapePops(ea_t call, ea_t blockStart)
{
	int64_t pushed = 0;
	for (ea_t ea = prevHead(call, blockStart); ea != BADADDR && ea >= blockStart; ea = prevHead(ea, blockStart))
	{
		Insn insn(ea);
		if (insn.mnem() == "call")
			break;
		if (insn.mnem() == "pop" || (insn.mnem() != "push" && insn.opText(0) == "esp"))
			return std::nullopt;
		pushed += insn.mnem() == "push" ? 4 : 0;
	}
	Insn next(nextHead(call));
	bool cleans = next.mnem() == "add" && next.opText(0) == "esp" && next.opType(1) == o_imm;
	if (cleans && next.opValue(1) != pushed)
		return std::nullopt;
	return cleans ? 0 : pushed;
}

}  // namespace

void Frames::realignedFrames()
{
	for (ea_t address : gameFunctions())
		realignedFrame(get_func(address));
}

std::map<ea_t, int64_t> Frames::realignedDepths(func_t* function, std::vector<ea_t>& shaped, std::string& why)
{
	FlowChart chart(function);
	std::map<ea_t, size_t> byStart;
	for (size_t k = 0; k < chart.blocks().size(); ++k)
		byStart.emplace(chart.blocks()[k].start, k);
	std::map<std::string, int64_t> saved;
	std::map<ea_t, int64_t> depth;
	std::map<ea_t, int64_t> entry{{function->start_ea, 0}};
	std::vector<ea_t> pending{function->start_ea};
	static const std::regex espDisplacement(R"(\[esp(\+[^*\]]*)?\])");
	while (!pending.empty())
	{
		ea_t start = pending.back();
		pending.pop_back();
		const FlowChart::Block& block = chart.blocks()[byStart.at(start)];
		int64_t d = entry[start];
		for (ea_t ea : heads(block.start, block.end))
		{
			if (auto [at, added] = depth.emplace(ea, d); !added && at->second != d)
			{
				why = "depths disagree at " + hex(ea);
				return depth;
			}
			Insn insn(ea);
			const std::string& m = insn.mnem();
			std::string op0 = insn.opText(0), op1 = insn.opText(1);
			if (m == "push")
				d -= 4;
			else if (m == "pop")
				d += 4;
			else if ((m == "add" || m == "sub") && op0 == "esp")
			{
				if (insn.opType(1) != o_imm)
				{
					why = "esp changed by a register at " + hex(ea);
					return depth;
				}
				int64_t imm = insn.opValue(1) & 0xFFFFFFFF;
				d += m == "add" ? imm : -imm;
			}
			else if (m == "mov" && (op0 == "ebp" || op0 == "ebx") && op1 == "esp")
				saved.try_emplace(op0, d);
			else if (m == "mov" && op0 == "esp")
			{
				auto restored = saved.find(op1);
				if (restored == saved.end())
				{
					why = "esp restored from " + op1 + " at " + hex(ea);
					return depth;
				}
				d = restored->second;
			}
			else if (m == "lea" && op0 == "esp" && std::regex_match(withoutBlanks(op1), espDisplacement))
				// `lea esp, [esp+disp]` adds disp; `lea esp, [esp]` is an alignment no-op (0xCE9369)
				d += insn.opType(1) == o_displ ? signed32(insn.opValue(1)) : 0;
			else if (m == "call")
			{
				int kind = insn.opType(0);
				std::optional<int64_t> pops;
				if (kind == o_reg || kind == o_displ || kind == o_phrase)
				{
					pops = indirectPops(function, ea);
					if (!pops && (pops = shapePops(ea, block.start)))
						shaped.push_back(ea);
					if (!pops)
					{
						why = "indirect call " + hex(ea) + " unresolved";
						return depth;
					}
				}
				else
				{
					// a near call to a function's start: its `ret N` (IDA's purge can be off: hkQuaternion::setMul
					// 0xC66320 is `ret 8`, IDA purges 4 at createMapping's calls). An import (`call ds:[IAT slot]`)
					// keeps IDA's purge
					ea_t target = static_cast<ea_t>(insn.opValue(0));
					func_t* callee = get_func(target);
					if ((kind == o_near || kind == o_far) && callee != nullptr && callee->start_ea == target)
					{
						try
						{
							pops = m_facts.pops(static_cast<uint32_t>(target));
						}
						catch (const exefacts::Raised&)
						{
						}
					}
					if (!pops)
						pops = get_sp_delta(function, get_item_end(ea));
				}
				d += *pops;
			}
			else if ((op0 == "esp" && m != "and") || m == "leave" || m == "pushal" || m == "popal" || m == "enter")
			{
				why = "unmodelled esp write at " + hex(ea);
				return depth;
			}
		}
		for (int succ : block.succs)
		{
			ea_t next = chart.blocks()[succ].start;
			if (auto [at, added] = entry.emplace(next, d); !added)
			{
				if (at->second != d)
				{
					why = "depths disagree at " + hex(next);
					return depth;
				}
				continue;
			}
			pending.push_back(next);
		}
	}
	return depth;
}

// A frame aligned by `and esp` (`push ebx; mov ebx, esp; ...; and esp, -N; ...; push ebp; mov ebp, [ebx+4]`, or
// `push ebp; mov ebp, esp; and esp, -N` with esp-relative locals): IDA follows neither the epilog's `mov esp, ebp`
// nor its `mov esp, ebx` and balances the function by spreading sp changes over indirect calls
// (PlayerCharacter::Update 0x93E860: +1112 at `call eax` 0x93EBCA, whose purge is 4). Hex-Rays then decompiles
// pushes as stores into locals ("bad sp value at call"). The stack points are rebuilt from the walk where every
// indirect call is resolved and the body is back at its depth after the prolog's `sub esp, N` before each `mov esp,
// ebp`; else, or where IDA's spd then still differs, IDA's points stay.
void Frames::realignedFrame(func_t* function)
{
	ea_t address = function->start_ea;
	uint32_t align = 0;
	try
	{
		align = m_facts.frameAlign(static_cast<uint32_t>(address));
	}
	catch (const exefacts::Raised&)
	{
	}
	if (!align)
		return;
	std::vector<ea_t> code = codeItems(function);
	std::vector<ea_t> shaped;
	std::string why;
	std::map<ea_t, int64_t> depth;
	// the points below make IDA's spd, linear by address, equal the walk: only for a function in one chunk
	if (function->tailqty)
		why = std::to_string(function->tailqty) + " tail chunks";
	else
		depth = realignedDepths(function, shaped, why);
	if (why.empty())
	{
		// back at the depth after the locals' `sub esp, N` (the first after `mov ebp, esp`) before each `mov esp,
		// ebp`: no call's purge absorbs another's error
		auto is = [](ea_t ea, const char* m, const char* op0, const char* op1) {
			Insn insn(ea);
			return insn.mnem() == m && insn.opText(0) == op0 && (!op1 || insn.opText(1) == op1);
		};
		std::optional<ea_t> movEbp;
		for (size_t k = 0; k < code.size() && k < 24 && !movEbp; ++k)
			if (is(code[k], "mov", "ebp", "esp"))
				movEbp = code[k];
		std::optional<int64_t> after;
		for (size_t k = 0; k < code.size() && k < 32 && movEbp; ++k)
			if (code[k] > *movEbp && is(code[k], "sub", "esp", nullptr))
			{
				auto found = depth.find(next_head(code[k], function->end_ea));
				if (found != depth.end())
					after = found->second;
				break;
			}
		std::string depths;
		bool balanced = after.has_value();
		size_t restores = 0;
		for (ea_t ea : code)
			if (depth.contains(ea) && is(ea, "mov", "esp", "ebp"))
			{
				++restores;
				depths += (depths.empty() ? "" : ", ") + std::to_string(depth[ea]);
				balanced = balanced && depth[ea] == *after;
			}
		if (!balanced || !restores)
			why = "body depth before `mov esp, ebp` [" + depths + "] != " + (after ? std::to_string(*after) : "None") +
				  " after the prolog";
	}
	if (!why.empty())
	{
		count("realigned frames kept");
		log("realigned-frame-sp-kept", hex(address), why);
		return;
	}
	// IDA's spd is linear by address: each instruction's change takes the depth to the next instruction's (a dead one
	// continues linearly; a jump or ret changes nothing)
	std::map<ea_t, int64_t> old;
	for (ea_t ea : code)
		old[ea] = get_sp_delta(function, get_item_end(ea));
	std::vector<ea_t> setAt;
	std::string changed;
	int64_t linear = 0;
	for (size_t k = 0; k < code.size(); ++k)
	{
		ea_t ea = code[k];
		int64_t here = depth.contains(ea) ? depth[ea] : linear;
		int64_t want = old[ea];
		if (k + 1 < code.size())
		{
			const std::string m = Insn(ea).mnem();
			int64_t natural = m.starts_with('j') || m.starts_with("ret") ? 0 : old[ea];
			auto next = depth.find(code[k + 1]);
			want = (next != depth.end() ? next->second : here + natural) - here;
		}
		if (want != old[ea])
		{
			add_user_stkpnt(get_item_end(ea), want);
			setAt.push_back(ea);
			changed += (changed.empty() ? "" : "; ") + hex(ea) + " " + std::to_string(old[ea]) + "->" +
					   std::to_string(want);
		}
		linear = here + want;
	}
	recalc_spd(address);
	std::vector<ea_t> wrong;
	for (const auto& [ea, d] : depth)
		if (spDelta(ea) != d)
			wrong.push_back(ea);
	std::string userKept;
	if (!wrong.empty())
	{
		// not expected (the points are built so the linear spd equals the walk): the points set here are deleted and
		// IDA's values put back as automatic points; IDA refuses those on a jump (0x9408DD's +8 in Update), where
		// IDA's value goes back as a user point
		for (ea_t ea : setAt)
		{
			del_stkpnt(function, get_item_end(ea));
			if (old[ea] && !add_auto_stkpnt(function, get_item_end(ea), old[ea]))
			{
				add_user_stkpnt(get_item_end(ea), old[ea]);
				userKept += (userKept.empty() ? "" : ", ") + hex(ea);
			}
		}
		recalc_spd(address);
	}
	bool fixed = wrong.empty() && !changed.empty();
	std::string shapes;
	for (ea_t ea : std::set<ea_t>(shaped.begin(), shaped.end()))
		shapes += (shapes.empty() ? "" : ", ") + hex(ea);
	count(fixed ? "realigned frames fixed" : "realigned frames kept");
	log(fixed ? "realigned-frame-sp-fixed" : "realigned-frame-sp-kept", hex(address),
		(wrong.empty() ? ""
					   : "spd still differs at " + std::to_string(wrong.size()) + ", first " + hex(wrong[0]) + "; ") +
			(userKept.empty() ? "" : "IDA values restored as user points at " + userKept + "; ") +
			(shapes.empty() ? "" : "purges by shape at " + shapes + "; ") + changed);
	// the function's lvar settings are built over these points (the locals pass), and each run of idakit frames
	// rebuilds the same points; IDA's analysis of the function follows the new points
	if (fixed)
	{
		plan_range(function->start_ea, function->end_ea);
		auto_wait();
	}
}

}  // namespace idakit
