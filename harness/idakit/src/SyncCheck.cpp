// The save check: sync writes nothing per function (the stack points and user lvar settings are the locals pass's,
// idakit frames and pdb_locals.py), so every game function's lvar settings and stack points, IDA's own points
// included, are the same after the last phase as before the first; the command saves only then. A function whose
// prototype sync changed, or which calls one directly, may lose lvar settings: Hex-Rays re-derives its locals from
// the new type and may drop the settings of its own locals (0x49C170's uiDepth) and its callers' at the call
// (0x4FF980's pkTmp). These are listed (localsLost) for the locals pass to make again; they fail the check unless
// the run hands them to the pass (sync --hand-locals-to-pass). A stack point is never exempted. The purged bytes
// are the prototypes' (IDA takes them from a pushed type): a change is logged, since it leaves the callers' traces
// for idakit frames.

#include "idakit/Sync.hpp"

#include "IdaSupport.hpp"

#include <Facts.hpp>

#include <algorithm>
#include <iterator>

namespace idakit
{

namespace
{

std::vector<std::string> frameRows(ea_t function)
{
	std::vector<std::string> rows;
	SrcPush::functionState(function, SrcPush::kLvarSettings | SrcPush::kAllPoints, rows);
	std::sort(rows.begin(), rows.end());
	return rows;
}

// The rows but the lvar settings at the function's arguments (definition address the function's start), and but
// the settings' function-wide row (`maps`) where no other setting is left.
std::vector<std::string> withoutArguments(ea_t function, const std::vector<std::string>& rows)
{
	const std::string key = hex(function);
	std::vector<std::string> out;
	bool settings = false;
	for (const std::string& r : rows)
	{
		std::vector<std::string> cells;
		for (size_t from = 0;;)
		{
			size_t tab = r.find('\t', from);
			cells.push_back(r.substr(from, tab - from));
			if (tab == std::string::npos)
				break;
			from = tab + 1;
		}
		bool lvar = cells[0] == "lvar" && cells.size() > 3;
		bool functionWide = lvar && cells[2].starts_with("maps");
		if (lvar && !functionWide && cells[3] == key)
			continue;
		settings = settings || (lvar && !functionWide);
		out.push_back(r);
	}
	if (!settings)
		std::erase_if(out, [](const std::string& r) { return r.starts_with("lvar\t") && r.find("\tmaps") != r.npos; });
	return out;
}

}  // namespace

void Sync::recordFrames()
{
	m_frames.clear();
	m_purges.clear();
	for (ea_t function : gameFunctions())
	{
		m_frames.emplace(function, frameRows(function));
		m_purges.emplace(function, get_func(function)->argsize);
	}
}

std::vector<std::string> Sync::changedFrames(bool handLocalsToPass)
{
	std::vector<std::string> changed;
	m_localsLost.clear();
	std::vector<ea_t> now = gameFunctions();
	std::set<ea_t> functions(now.begin(), now.end());
	for (const auto& [function, rows] : m_frames)
		functions.insert(function);
	// the retyped functions and their direct callers: the analysis a written type queues re-derives the callers'
	// stack slots at the call too (their arguments' types and names), where Hex-Rays drops the callers' settings
	std::set<ea_t> retyped(m_prototypeChanged.begin(), m_prototypeChanged.end());
	for (ea_t callee : m_prototypeChanged)
	{
		xrefblk_t xb;
		for (bool ok = xb.first_to(callee, XREF_CODE); ok; ok = xb.next_to())
			if (xb.type == fl_CN || xb.type == fl_CF)
				if (func_t* caller = get_func(xb.from))
					retyped.insert(caller->start_ea);
	}
	for (ea_t function : functions)
	{
		auto before = m_frames.find(function);
		std::vector<std::string> after = get_func(function) ? frameRows(function) : std::vector<std::string>();
		const std::vector<std::string> none;
		std::vector<std::string> was = before == m_frames.end() ? none : before->second;
		// arguments belong to the prototype: Hex-Rays drops their settings where sync changes it
		if (m_prototypeChanged.contains(function))
		{
			was = withoutArguments(function, was);
			after = withoutArguments(function, after);
		}
		if (was != after)
		{
			std::vector<std::string> gone, added;
			std::set_difference(was.begin(), was.end(), after.begin(), after.end(), std::back_inserter(gone));
			std::set_difference(after.begin(), after.end(), was.begin(), was.end(), std::back_inserter(added));
			// a function retyped, or calling one, whose difference is its lvar settings alone: the locals pass's to
			// make again, which fails the check unless the run hands it to the pass
			auto lvar = [](const std::string& r) { return r.starts_with("lvar\t"); };
			bool locals = retyped.contains(function) && std::all_of(gone.begin(), gone.end(), lvar) &&
						  std::all_of(added.begin(), added.end(), lvar);
			bool exempt = locals && handLocalsToPass;
			std::string category = exempt ? "locals-lost" : "save-check";
			std::string detail = hex(function) + ":";
			for (const std::string& r : gone)
			{
				detail += " -" + r;
				log(category, hex(function), "-" + r);
			}
			for (const std::string& r : added)
			{
				detail += " +" + r;
				log(category, hex(function), "+" + r);
			}
			if (locals)
			{
				count("locals lost");
				m_localsLost.push_back(function);
			}
			if (!exempt)
				changed.push_back(detail);
		}
		func_t* f = get_func(function);
		if (f == nullptr || f->start_ea != function)
			continue;
		auto purged = m_purges.find(function);
		std::optional<uint32_t> pops;
		try
		{
			pops = m_facts.pops(static_cast<uint32_t>(function));
		}
		catch (const exefacts::Raised&)
		{
		}
		std::string exe = pops ? std::to_string(*pops) : std::string("unknown");
		if (purged != m_purges.end() && purged->second != f->argsize)
		{
			count("purges changed");
			log("purge-changed", hex(function),
				std::to_string(purged->second) + " -> " + std::to_string(f->argsize) + ", the exe pops " + exe);
		}
		if (pops && *pops != f->argsize)
			count("purges not the exe's");
	}
	return changed;
}

}  // namespace idakit
