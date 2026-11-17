// idakit sync: src pushed into an IDA database (Sync), its phases in order, then the log, the state dump and the
// database's function and name lists under --out (the canonical's default --out is where the harness tools read
// them: kFunctionListPath, kNameListPath). The function list is written first, on open, so that the exe facts see
// the database's function bounds, and again with the name list after the sync. The database is saved once, after
// the last phase, and only where the save check passes (every game function's lvar settings and stack points as
// they were before the first phase; with --hand-locals-to-pass, but the lvar settings of a retyped function and its
// direct callers, which DIR/locals-lost.txt lists for the locals pass with or without the flag): a run that fails
// leaves it as it was.

#include "idakit/Command.hpp"
#include "idakit/Database.hpp"
#include "idakit/ExeSlotTargets.hpp"
#include "idakit/ExeVtables.hpp"
#include "idakit/FunctionList.hpp"
#include "idakit/SrcModel.hpp"
#include "idakit/SrcTypes.hpp"
#include "idakit/Sync.hpp"

#include "IdaSupport.hpp"

#include <Facts.hpp>

#include <chrono>
#include <cstdio>
#include <fstream>

namespace idakit
{

int runSync(Args& args)
{
	std::string idb = args.required("idb");
	std::string modelPath = args.option("model").value_or("env/cache/srcmodel.sqlite");
	std::string aliases = args.option("aliases").value_or("harness/tools/ida_type_aliases.tsv");
	std::filesystem::path out = args.option("out").value_or("env/cache/ida");
	bool handLocalsToPass = args.flag("hand-locals-to-pass");

	SrcModel model(modelPath);
	Database::Options options;
	options.hexrays = true;
	Database database(idb, options);
	std::filesystem::create_directories(out);
	const std::string functionList = (out / std::filesystem::path(kFunctionListPath).filename()).string();
	writeFunctionList(functionList);
	ExeVtables exe;
	exefacts::Program program(".", functionList);
	exefacts::Facts facts(program);
	ExeSlotTargets slotTargets(exe, facts);
	SrcTypes types(model, aliases);
	Sync sync(model, exe, facts, types);

	auto c = [&](const char* key) {
		auto found = sync.counts().find(key);
		return static_cast<long long>(found == sync.counts().end() ? 0 : found->second);
	};
	auto lap = std::chrono::steady_clock::now();
	auto seconds = [&] {
		auto now = std::chrono::steady_clock::now();
		double s = std::chrono::duration<double>(now - lap).count();
		lap = now;
		return s;
	};

	// the save check's record
	sync.recordFrames();
	std::printf("recorded: lvar settings and stack points of every game function (%.1fs)\n", seconds());

	// types
	sync.aliases();
	std::printf("aliases: typedef %lld, absent %lld, not src %lld (%.1fs)\n", c("aliases: typedef"),
				c("aliases: absent"), c("aliases: not src"), seconds());
	sync.enums();
	std::printf("enums: written %lld (%.1fs)\n", c("enums written"), seconds());
	sync.records();
	std::printf("records: written %lld, resized %lld, members unmapped %lld, empty bases left out %lld (%.1fs)\n",
				c("records written"), c("structs resized"), c("members unmapped"), c("empty bases left out"),
				seconds());
	sync.vtables();
	std::printf("vtables: written %lld (%.1fs)\n", c("vtables written"), seconds());

	// functions
	sync.prototypes();
	std::printf("prototypes: %lld (usercalls %lld, exe-decided %lld, refused %lld, unmapped %lld, failed %lld, "
				"src-contradicted %lld) (%.1fs)\n",
				c("prototypes"), c("usercalls"), c("usercalls exe-decided"), c("usercalls refused"),
				c("prototypes unmapped"), c("prototypes failed"), c("src-contradicted"), seconds());
	sync.foldPrototypes();
	std::printf("folds: %lld (typed %lld, untyped %lld, failed %lld) (%.1fs)\n", c("folds"), c("folds typed"),
				c("folds untyped"), c("folds failed"), seconds());
	sync.globals();
	std::printf("globals: %lld (in code %lld, overlapping %lld, unmapped %lld, typed over IDA's items %lld (strings "
				"%lld), type failed %lld; names set %lld, unchanged %lld, failed %lld) (%.1fs)\n",
				c("globals"), c("globals in code"), c("globals overlapping"), c("globals unmapped"),
				c("globals typed over IDA's items"), c("globals typed over IDA's strings"), c("globals type failed"),
				c("global names set"), c("global names unchanged"), c("global names failed"), seconds());
	sync.names();
	std::printf("names: set %lld, unchanged %lld, failed %lld (%.1fs)\n", c("function names set"),
				c("function names unchanged"), c("function names failed"), seconds());

	// exe
	sync.pins();
	std::printf("pins (pinned/already): variadic %lld/%lld, usercall %lld/%lld, return %lld/%lld, return width "
				"%lld/%lld, dead registers %lld/%lld (ecx kept: vtable slot %lld, member's name %lld; pops unknown "
				"%lld, stack arguments not the exe's pops %lld, unforceable %lld) (%.1fs)\n",
				c("variadic pinned"), c("variadic already"), c("usercall pinned"), c("usercall already"),
				c("return pinned"), c("return already"), c("return width widened"), c("return width already"),
				c("dead registers dropped"), c("dead registers already"), c("dead ecx kept: a vtable slot"),
				c("dead ecx kept: a member's name"), c("dead registers: pops unknown"),
				c("dead registers: stack arguments not the exe's pops"), c("dead registers unforceable"), seconds());
	sync.outlined();
	std::printf("outlined marks: cleared %lld, failed %lld (%.1fs)\n", c("outlined marks cleared"),
				c("outlined marks failed"), seconds());
	std::printf("function types (prototypes, folds, pins): changed %lld, unchanged %lld\n", c("prototype-changed"),
				c("prototype-unchanged"));

	// the save check
	std::vector<std::string> changed = sync.changedFrames(handLocalsToPass);
	sync.writeLogs(out);
	// the retyped functions and direct callers whose locals alone lost their settings: the locals pass's input
	// (clearlvars.py, then `pdb_locals.py --fresh --only`), written whether the run saves or not
	const std::filesystem::path lostPath = out / "locals-lost.txt";
	{
		std::ofstream lost(lostPath);
		for (ea_t function : sync.localsLost())
			lost << hex(function) << '\n';
		if (!lost)
			throw Error("idakit sync: cannot write " + lostPath.string());
	}
	if (!changed.empty())
	{
		std::printf("save check: lvar settings or stack points changed in %zu functions; not saved\n",
					changed.size());
		for (size_t k = 0; k < changed.size() && k < 10; ++k)
			std::printf("  %s\n", changed[k].size() > 400 ? (changed[k].substr(0, 400) + " ...").c_str()
														  : changed[k].c_str());
		if (!handLocalsToPass && !sync.localsLost().empty())
			std::printf("save check: %zu of them retyped or calling a retyped function, their lvar settings alone "
						"lost (%s): by hand, env/scratch/clearlvars.py IDB ADDR... on them, the sync, then "
						"pdb_locals.py --fresh --only %s; or sync --hand-locals-to-pass\n",
						sync.localsLost().size(), lostPath.c_str(), lostPath.c_str());
		return 1;
	}
	if (handLocalsToPass)
		std::printf("save check: lvar settings and stack points unchanged but the lvar settings of %zu retyped "
					"functions and direct callers (%s: run pdb_locals.py --fresh --only it after) (%.1fs)\n",
					sync.localsLost().size(), lostPath.c_str(), seconds());
	else
		std::printf("save check: lvar settings and stack points unchanged (%.1fs)\n", seconds());
	sync.writeState(out);
	writeFunctionList(functionList);
	writeNameList((out / std::filesystem::path(kNameListPath).filename()).string());
	std::printf("state: %s (%.1fs)\n", (out / "state.tsv").c_str(), seconds());
	database.save();
	std::printf("saved %s (%.1fs)\n", idb.c_str(), seconds());
	std::printf("purged bytes: changed %lld (logged purge-changed: run idakit frames and the locals pass after), "
				"not the exe's pops %lld\n",
				c("purges changed"), c("purges not the exe's"));
	return 0;
}

}  // namespace idakit
