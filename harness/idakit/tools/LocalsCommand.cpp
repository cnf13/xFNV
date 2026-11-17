// idakit locals: the local steps of Unfold (merged arrays, frame locals, passed objects, constructed locals, value
// locals) run on functions (default: every game function), on top of the lvar settings the database holds, and their
// lvar settings saved (Unfold::locals): each function gains user lvar settings and nothing else. DIR (default
// env/cache/ida/locals) gets locals-log.tsv (the steps' rows) and state.tsv (the database's state after the run).
// Saves the database once, at the end. The step of the locals pass after pdb_locals.py (docs/IDAKIT.md, Unfold). Run
// from the repository root (exefacts reads its env/exe).

#include "IdaSupport.hpp"

#include "idakit/Address.hpp"
#include "idakit/Command.hpp"
#include "idakit/Database.hpp"
#include "idakit/ExeSlotTargets.hpp"
#include "idakit/ExeVtables.hpp"
#include "idakit/SrcModel.hpp"
#include "idakit/SrcTypes.hpp"
#include "idakit/Unfold.hpp"

#include <Facts.hpp>

#include <chrono>
#include <cstdio>
#include <filesystem>

namespace idakit
{

int runLocals(Args& args)
{
	std::string idb = args.required("idb");
	std::string modelPath = args.option("model").value_or("env/cache/srcmodel.sqlite");
	std::string aliases = args.option("aliases").value_or("harness/tools/ida_type_aliases.tsv");
	std::filesystem::path out = args.option("out").value_or("env/cache/ida/locals");
	auto started = std::chrono::steady_clock::now();

	SrcModel model(modelPath);
	exefacts::Program program(".");
	exefacts::Facts facts(program);
	Database database(idb);
	ExeVtables vtables;
	ExeSlotTargets slotTargets(vtables, facts);
	SrcTypes types(model, aliases);
	Unfold unfold(model, vtables, facts, types, true);
	// no addresses: every game function; an empty @FILE: none
	std::vector<uint64_t> addresses;
	if (args.positional().empty())
		for (ea_t ea : gameFunctions())
			addresses.push_back(ea);
	else
		addresses = parseAddresses(args.positional());

	size_t done = 0, failures = 0;
	for (uint64_t ea : addresses)
	{
		++done;
		func_t* function = get_func(ea);
		if (function == nullptr)
		{
			std::fprintf(stderr, "idakit locals: no function at %s\n", hexText(ea).c_str());
			++failures;
			continue;
		}
		std::string why;
		if (!unfold.locals(function, why))
		{
			std::fprintf(stderr, "idakit locals: %s: %s\n", hexText(function->start_ea).c_str(), why.c_str());
			++failures;
		}
		if (done % 500 == 0)
			std::fprintf(stderr, "locals: %zu/%zu\n", done, addresses.size());
	}
	unfold.writeLogs(out);
	unfold.writeState(out);
	database.save();
	std::printf("idakit locals: %zu functions, %zu failed; %s, %s; saved %s (%.1fs)\n", addresses.size(), failures,
				(out / "locals-log.tsv").c_str(), (out / "state.tsv").c_str(), idb.c_str(),
				std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count());
	return failures ? 1 : 0;
}

}  // namespace idakit
