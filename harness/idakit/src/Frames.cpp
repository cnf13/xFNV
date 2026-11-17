// idakit frames' steps but the realigned frames (FramesRealigned.cpp): the sp traces recorded, the purged bytes
// made the exe's, the stale traces done again, and the functions whose trace moved found and their lvar settings
// cleared.

#include "idakit/Frames.hpp"

#include "IdaSupport.hpp"

#include <Facts.hpp>

namespace idakit
{

void Frames::record()
{
	m_spd.clear();
	for (ea_t function : gameFunctions())
	{
		func_t* f = get_func(function);
		std::vector<std::pair<ea_t, sval_t>>& spd = m_spd[function];
		for (ea_t ea : codeItems(f))
			spd.emplace_back(ea, get_spd(f, ea));
	}
}

void Frames::purges()
{
	for (ea_t function : gameFunctions())
	{
		std::optional<uint32_t> pops;
		try
		{
			pops = m_facts.pops(static_cast<uint32_t>(function));
		}
		catch (const exefacts::Raised&)
		{
		}
		func_t* f = get_func(function);
		if (!pops || *pops == f->argsize)
			continue;
		count("purges fixed");
		log("purge-fixed", hex(function), std::to_string(f->argsize) + " -> " + std::to_string(*pops));
		f->argsize = *pops;
		update_func(f);
	}
}

// IDA records a direct call's sp change, a point at the call's end, from the callee's purged bytes when it traces
// the caller, and leaves it when the callee's purged bytes change later.
bool Frames::staleCall(func_t* function, std::string& why)
{
	for (ea_t ea : codeItems(function))
	{
		insn_t insn;
		if (decode_insn(&insn, ea) <= 0 || !is_call_insn(insn) ||
			(insn.ops[0].type != o_near && insn.ops[0].type != o_far))
			continue;
		ea_t target = insn.ops[0].addr;
		func_t* callee = get_func(target);
		if (callee == nullptr || callee->start_ea != target || target >= kGameEnd)
			continue;
		if (sval_t recorded = get_sp_delta(function, get_item_end(ea)); recorded != sval_t(callee->argsize))
		{
			why = "call " + hex(ea) + " of " + hex(target) + " records " + std::to_string(recorded) +
				  ", the callee purges " + std::to_string(callee->argsize);
			return true;
		}
	}
	return false;
}

void Frames::retrace()
{
	for (ea_t function : gameFunctions())
	{
		func_t* f = get_func(function);
		std::string why;
		for (ea_t item : itemHeads(f))
			if (is_usersp(get_item_end(item)))
			{
				why = "user stack point at " + hex(get_item_end(item));
				break;
			}
		if (why.empty())
			try
			{
				if (uint32_t align = m_facts.frameAlign(static_cast<uint32_t>(function)))
					why = "frame aligned to " + std::to_string(align);
			}
			catch (const exefacts::Raised&)
			{
			}
		if (why.empty() && !staleCall(f, why))
			continue;
		count("traced again");
		log("traced-again", hex(function), why);
		// a point sits at the end of the instruction whose sp change it records; a user point's mark is cleared
		// with it
		for (ea_t item : itemHeads(f))
		{
			ea_t end = get_item_end(item);
			if (is_usersp(end))
				clr_usersp(end);
			del_stkpnt(f, end);
		}
		f->flags &= ~FUNC_SP_READY;
		update_func(f);
		reanalyze_function(f);
	}
	auto_wait();
}

std::vector<ea_t> Frames::changed()
{
	std::vector<ea_t> out;
	for (const auto& [function, spd] : m_spd)
	{
		func_t* f = get_func(function);
		int64_t moved = 0;
		for (const auto& [ea, before] : spd)
			moved += get_spd(f, ea) != before;
		if (!moved)
			continue;
		out.push_back(function);
		lvar_uservec_t settings;
		bool held = restore_user_lvar_settings(&settings, function) && !settings.empty();
		if (held)
		{
			save_user_lvar_settings(function, lvar_uservec_t());
			count("lvar settings cleared");
		}
		count("frames changed");
		log("frame-changed", hex(function),
			std::to_string(moved) + " instructions moved" + (held ? ", lvar settings cleared" : ""));
	}
	return out;
}

}  // namespace idakit
