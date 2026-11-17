// idakit frames: every game function's sp trace made right (Frames), its steps in order, then under --out the log,
// the functions whose trace moved (frames-changed.txt, one 0xADDR per line: pdb_locals.py --only's input) and the
// state dump. The function list is written first, on open, so that the exe facts see the database's function bounds.
// The database is saved once, after the last step: a run that fails leaves it as it was.

#include "idakit/Command.hpp"
#include "idakit/Database.hpp"
#include "idakit/ExeSlotTargets.hpp"
#include "idakit/ExeVtables.hpp"
#include "idakit/Frames.hpp"
#include "idakit/FunctionList.hpp"
#include "idakit/SrcModel.hpp"
#include "idakit/SrcTypes.hpp"

#include "IdaSupport.hpp"

#include <Facts.hpp>

#include <chrono>
#include <cstdio>
#include <fstream>

namespace idakit
{

int runFrames(Args& args)
{
	std::string idb = args.required("idb");
	std::string modelPath = args.option("model").value_or("env/cache/srcmodel.sqlite");
	std::string aliases = args.option("aliases").value_or("harness/tools/ida_type_aliases.tsv");
	std::filesystem::path out = args.option("out").value_or("env/cache/frames");

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
	Frames frames(model, exe, facts, types);

	auto c = [&](const char* key) {
		auto found = frames.counts().find(key);
		return static_cast<long long>(found == frames.counts().end() ? 0 : found->second);
	};
	auto lap = std::chrono::steady_clock::now();
	auto seconds = [&] {
		auto now = std::chrono::steady_clock::now();
		double s = std::chrono::duration<double>(now - lap).count();
		lap = now;
		return s;
	};

	frames.record();
	std::printf("recorded: the sp at every instruction of every game function (%.1fs)\n", seconds());
	frames.purges();
	std::printf("purged bytes fixed: %lld (%.1fs)\n", c("purges fixed"), seconds());
	frames.retrace();
	std::printf("traced again: %lld (%.1fs)\n", c("traced again"), seconds());
	frames.realignedFrames();
	std::printf("realigned frames: fixed %lld, kept %lld (%.1fs)\n", c("realigned frames fixed"),
				c("realigned frames kept"), seconds());
	std::vector<ea_t> changed = frames.changed();
	std::printf("frames changed: %lld, lvar settings cleared %lld (%.1fs)\n", c("frames changed"),
				c("lvar settings cleared"), seconds());

	frames.writeLogs(out);
	{
		std::ofstream list(out / "frames-changed.txt");
		for (ea_t function : changed)
			list << hex(function) << '\n';
		if (!list)
			throw Error("idakit frames: cannot write " + (out / "frames-changed.txt").string());
	}
	frames.writeState(out);
	std::printf("state: %s (%.1fs)\n", (out / "state.tsv").c_str(), seconds());
	database.save();
	std::printf("saved %s (%.1fs)\n", idb.c_str(), seconds());
	return 0;
}

}  // namespace idakit
