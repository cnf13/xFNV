// srcdb: clang's view of src for the harness's tools (the system's libclang-cpp, the same clang the hybrid build's
// clang-cl is).
//   srcdb probe < REQUEST.json > RESULT.json   srcindex's layout probe (Probe.cpp)
//   srcdb index [options]                      the src model database (Index.cpp, Model.hpp)
//   srcdb index-tu ...                         one TU of an index (index runs it)
//   srcdb rename-refs ...                      where one TU spells the names of a rename table (Rename.cpp)
//   srcdb fold-refs ...                        the constants one TU has that only name a function, and their uses
//                                              (Fold.cpp)
//   srcdb globals-tu ...                       one TU's variables with static storage (Globals.cpp)

#include "Store.hpp"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>

namespace srcdb
{
int runProbe();
int runIndex(int argc, char** argv);
int runIndexTu(int argc, char** argv);
int runRenameRefs(int argc, char** argv);
int runFoldRefs(int argc, char** argv);
int runGlobalsTu(int argc, char** argv);
}  // namespace srcdb

int main(int argc, char** argv)
{
	llvm::StringRef command = argc > 1 ? argv[1] : "";
	try
	{
		if (command == "probe" && argc == 2)
			return srcdb::runProbe();
		if (command == "index")
			return srcdb::runIndex(argc, argv);
		if (command == "index-tu")
			return srcdb::runIndexTu(argc, argv);
		if (command == "rename-refs")
			return srcdb::runRenameRefs(argc, argv);
		if (command == "fold-refs")
			return srcdb::runFoldRefs(argc, argv);
		if (command == "globals-tu")
			return srcdb::runGlobalsTu(argc, argv);
	}
	catch (const srcdb::Error& error)
	{
		llvm::errs() << error.what() << "\n";
		return 2;
	}
	llvm::errs() << "usage: srcdb probe < REQUEST.json > RESULT.json\n"
					"       srcdb index [--root DIR] [--compile-commands FILE] [--out DB] [--jobs N] [--limit N]\n";
	return 64;
}
