// A function's prototype against what the exe's code says about it (exefacts): the test whether the code
// contradicts it, and the prototypes built from the code where it does. idakit sync pins them on the functions src does
// not declare (SyncExe.cpp) and tests src's prototypes with them (SyncFunctions.cpp).
#pragma once

#include "idakit/Ida.hpp"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace exefacts
{
class Facts;
}

namespace idakit
{

class AsmFacts;

// ecx and edx (exefacts' kEcx, kEdx) as the code reads them, a contradiction's evidence: exefacts' abi reads less
// those its registerInputs does not count (a `push ecx` that only reserves a slot), so that no test passes again a
// register the dead-register pin (SyncExe.cpp) dropped. exefacts::Raised where abi raises.
uint8_t ecxEdxReads(exefacts::Facts& facts, ea_t function);

// The exe's code contradicts the prototype: ecx or edx read but not passed, a stack slot read past the arguments of a
// function that pops nothing (cdecl, or a callee-pops type at a bare `ret`: 0x690890 reads [esp+8]), or other popped
// bytes than the prototype's. Only cdecl, stdcall, thiscall and fastcall are judged; a function exefacts cannot read
// is not contradicted, nor are the pops of one whose pops exefacts cannot prove (`pops` unknown: no tail-jump target).
bool contradicted(exefacts::Facts& facts, ea_t function, const tinfo_t& type);

// A prototype the exe agrees with, built over the given one: thiscall when the code reads ecx (a `void *this` first
// where the type has no register argument), else stdcall when it pops bytes, else cdecl. The stack arguments are the
// type's own while they fit the bytes the code pops (cdecl: the highest slot it reads), then `int`s. nullopt where the
// code reads edx (fastcall register arguments are not guessed), its pops are unknown (exefacts' `pops`), a fastcall
// type has two register arguments, or the type's stack arguments overrun the popped bytes mid-argument.
std::optional<tinfo_t> exePrototype(exefacts::Facts& facts, ea_t function, const tinfo_t& type);

// The prototype with a return value, for one that returns void where some return path sets eax and every direct
// caller that reads eax after the call reads it at one width: 1 (unsigned __int8, so the bits stay exact), 2 (__int16)
// or 4 (int). nullopt otherwise; why: the reason when callers read the result but not at one width.
std::optional<tinfo_t> exeReturn(exefacts::Facts& facts, AsmFacts& asmFacts, ea_t function, const tinfo_t& type,
								 std::string& why);

// The return width rule on the prototype: whether its integer return is as wide as the bytes of eax every return
// path writes or as the most of eax a direct caller reads (1, 2 or 4 bytes), so that no caller reads a written byte
// the type leaves undefined; nullopt where the rule does not apply (no integer return, no width every path writes, no
// caller reading eax). widerReturn widens a return that does not.
std::optional<bool> returnHoldsWrites(exefacts::Facts& facts, AsmFacts& asmFacts, ea_t function, const tinfo_t& type);

// The prototype with an integer return widened to the bytes of eax every return path writes, where it is narrower
// and a direct caller reads more of eax than it holds (a bool's callers' `test al, al` read no more than its byte):
// signed for a movsx, unsigned for a movzx, else as it was. Only widened: a partial write (`mov al`) may sit on an
// earlier whole one the walk does not see. nullopt otherwise.
std::optional<tinfo_t> widerReturn(exefacts::Facts& facts, AsmFacts& asmFacts, ea_t function, const tinfo_t& type);

// The registers outside every standard convention (eax, ebx, edi, esi, in that order) the code reads before writing
// them (exefacts' regArgs), as IDA's register numbers with their names.
std::vector<std::pair<int, std::string>> inputRegisters(exefacts::Facts& facts, ea_t function);

// The arguments as a __usercall (__userpurge where the function pops its stack arguments): the arguments keep their
// locations (those given a register in it), the stack ones laid out again in order from offset 0, and the return
// location is fd's. nullopt for an unsized stack argument or a type IDA rejects.
std::optional<tinfo_t> usercall(func_type_data_t fd, bool pops);

// The register inputs the code reads that the prototype does not pass (inputRegisters', ecx and edx), by name, ""
// for none.
std::string unpassedRegisters(exefacts::Facts& facts, ea_t function, const tinfo_t& type);

// Where the exe's code contradicts a __usercall/__userpurge prototype, why ("" where it fits): a register the code
// reads before writing it that the type does not pass (unpassedRegisters), one the type passes that the code never
// takes (exefacts' registerInputs), fewer stack bytes than the slots the code reads, or other bytes than the code
// pops (a __usercall's: none). Exe facts a decode cannot give are a contradiction too.
std::string usercallMisfit(exefacts::Facts& facts, ea_t function, const tinfo_t& type);

}  // namespace idakit
