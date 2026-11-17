// The exe's functions as the open database lists them, for the harness tools (fn.py, exefacts): every function, in
// address order (the game's code ends at 0xEC30E8; the libraries, the CRT and the import thunks follow).
#pragma once

#include <cstdint>
#include <string>

namespace idakit
{

// The canonical database's lists (relative to the checkout, as idakit runs from its root), where fn.py and exefacts
// read them. idakit sync writes its database's under its --out with these file names, the canonical's default --out
// being their folder.
inline constexpr const char* kFunctionListPath = "env/cache/ida/functions.tsv";
inline constexpr const char* kNameListPath = "env/cache/ida/ida-names.tsv";

// The function's name as the tools show it: IDA's name demangled in full, else IDA's name as it is.
std::string functionName(uint64_t start);

// Writes path (through a temporary file of this process, renamed over it): `0xADDR<TAB>0xEND<TAB>NAME` per function,
// END being the end of the function's entry chunk. Returns the number of rows; throws Error when the file cannot be
// written.
size_t writeFunctionList(const std::string& path);

// Writes path (through a temporary file of this process, renamed over it): `0xADDR<TAB>NAME` per named address of the
// database (IDA's names as they are, in address order). Returns the number of rows; throws Error when the file cannot
// be written.
size_t writeNameList(const std::string& path);

}  // namespace idakit
