// idakit functions: the database's function list (writeFunctionList) and its names (writeNameList) for the harness
// tools, by default env/cache/ida/functions.tsv and env/cache/ida/ida-names.tsv, which idakit sync of the canonical
// database also rewrites.
// idakit decompile: the Hex-Rays text of functions, each decompiled afresh after it is unfolded as emit unfolds
// them (Unfold; never saved): "// 0xADDR NAME" then the pseudocode, on stdout one after another, or
// with --out one file each, DIR/<ADDR8>.c. An address inside a function gives that function. A failure is a line on
// stderr (with --out also DIR/<ADDR8>.err, ADDR as given) and exit status 1. Run from the repository root (exefacts
// reads its env/exe).

#include "idakit/Address.hpp"
#include "idakit/Command.hpp"
#include "idakit/Database.hpp"
#include "idakit/ExeSlotTargets.hpp"
#include "idakit/ExeVtables.hpp"
#include "idakit/FunctionList.hpp"
#include "idakit/Ida.hpp"
#include "idakit/SrcModel.hpp"
#include "idakit/SrcTypes.hpp"
#include "idakit/Unfold.hpp"

#include <Facts.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>

namespace idakit
{

namespace
{

void writeFile(const std::filesystem::path& path, const std::string& text)
{
	std::filesystem::path partial = path;
	partial += ".partial";
	{
		std::ofstream file(partial, std::ios::binary | std::ios::trunc);
		file << text;
		if (!file)
			throw Error("idakit: cannot write " + partial.string());
	}
	std::filesystem::rename(partial, path);
}

// The function's text after it is unfolded (Unfold), or why there is none (failed).
std::string decompiled(Unfold& unfold, uint64_t ea, bool& failed)
{
	failed = true;
	func_t* function = get_func(ea);
	if (function == nullptr)
		return "no function at " + hexText(ea);
	uint64_t start = function->start_ea;
	std::string why;
	cfuncptr_t cfunc = unfold.rebuild(function, why);
	if (cfunc == nullptr)
		return hexText(start) + ": " + why;
	qstring body;
	qstring_printer_t printer(cfunc, body, false);
	cfunc->print_func(printer);
	failed = false;
	return "// 0x" + hex8(start) + " " + functionName(start) + "\n" + body.c_str() + "\n";
}

}  // namespace

int runFunctions(Args& args)
{
	std::string idb = args.required("idb");
	std::string out = args.option("out").value_or(kFunctionListPath);
	std::string names = args.option("names").value_or(kNameListPath);
	Database::Options options;
	options.hexrays = false;
	Database database(idb, options);
	size_t functions = writeFunctionList(out);
	size_t named = writeNameList(names);
	std::fprintf(stderr, "idakit functions: %zu functions -> %s, %zu names -> %s\n", functions, out.c_str(), named,
				 names.c_str());
	return 0;
}

int runDecompile(Args& args)
{
	std::string idb = args.required("idb");
	std::string modelPath = args.option("model").value_or("env/cache/srcmodel.sqlite");
	std::string aliases = args.option("aliases").value_or("harness/tools/ida_type_aliases.tsv");
	std::optional<std::filesystem::path> out;
	if (auto dir = args.option("out"))
		out = *dir;
	std::vector<uint64_t> addresses = parseAddresses(args.positional());
	if (out)
		std::filesystem::create_directories(*out);

	SrcModel model(modelPath);
	exefacts::Program program(".");
	exefacts::Facts facts(program);
	Database database(idb);
	ExeVtables vtables;
	ExeSlotTargets slotTargets(vtables, facts);
	SrcTypes types(model, aliases);
	Unfold unfold(model, vtables, facts, types);
	int failures = 0;
	for (uint64_t ea : addresses)
	{
		bool failed = false;
		std::string text = decompiled(unfold, ea, failed);
		if (failed)
		{
			std::fprintf(stderr, "idakit decompile: %s\n", text.c_str());
			++failures;
			if (out)
				writeFile(*out / (hex8(ea) + ".err"), text + "\n");
		}
		else if (out)
			writeFile(*out / (hex8(get_func(ea)->start_ea) + ".c"), text);
		else
			std::fputs(text.c_str(), stdout);
	}
	return failures ? 1 : 0;
}

}  // namespace idakit
