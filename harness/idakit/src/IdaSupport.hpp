// Helpers on IDA's view of the exe that sync, emit, declare and the exe facts share (IdaSupport.cpp; internal to
// the idakit library).
#pragma once

#include "idakit/Ida.hpp"
#include "idakit/SrcModel.hpp"

#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace idakit
{

// the end of the game's code (the CRT and libraries after it are not src's)
inline constexpr uint64_t kGameEnd = 0xEC30E8;

// the start of every function below kGameEnd, ascending
std::vector<ea_t> gameFunctions();
// a function starts at the address
bool functionStart(ea_t address);
// IDA has the function as one that never returns: its stored type's `__noreturn`, or the function's FUNC_NORET mark
// (IDA's analysis of its code, which a stored type carries once written)
bool idaNoreturn(ea_t address);
// the heads (instructions and data) of every chunk of the function, chunk by chunk
std::vector<ea_t> itemHeads(func_t* function);
// the instructions of every chunk of the function, by address
std::vector<ea_t> codeItems(func_t* function);
// a Hex-Rays expression past its casts
const cexpr_t* stripCasts(const cexpr_t* e);
// the lvars whose address the body takes: a `&` of the local or of a part of it (a member, an element), or an array
// local anywhere but as the array of a subscript
std::set<int> addressTakenLocals(cfunc_t& cfunc);

std::string hex(uint64_t value);
// the last component of a qualified name (`A<B::C>::f` -> f; `A::operator<` -> operator<)
std::string lastComponent(const std::string& name);
// the qualified name without its last component
std::string scopeOf(const std::string& name);
// the function's IDA name as the PDB spells names: a mangled one demangled without its argument list
std::string pdbSpelling(ea_t address);
// a constructor's name as the PDB spells it: `X::X` (a class template's `X<T>::X<T>` too)
bool constructorName(const std::string& name);
// a name IDA gave the `_N` suffix (another address holds the name): the name without it
std::optional<std::string> withoutSuffix(const std::string& name);
std::string withoutBlanks(std::string text);
// a Hex-Rays helper that reads or writes part of its argument: LOBYTE, HIWORD, BYTE2, WORD1, SLODWORD, ...
bool isPartHelper(std::string_view helper);

std::string text(const tinfo_t& type);
// the two print alike, `struct`/`enum`/`union`/`class` tags aside
bool sameType(const tinfo_t& a, const tinfo_t& b);
tinfo_t pointerTo(const tinfo_t& target);
tinfo_t voidType();
// `unsigned __int8[size]`
tinfo_t byteArray(int64_t size);
// the bytes the type takes on the stack (4 for an unsized one)
int64_t stackSize(const tinfo_t& type);
// the stack bytes a function type (or a pointer to one, a vtbl member's) passes, for a thiscall or stdcall one
std::optional<int64_t> stackBytes(const tinfo_t& member);
// The stack bytes each `ret` reachable from the function's entry pops (0 for a plain `ret`): the blocks only the C++
// EH dispatcher enters (catch funclets, which end in a plain `ret` to it) are not the function's returns.
std::set<int64_t> reachableRets(func_t* function);
// an integer, or a pointer to nothing in particular (void, an integer; char only for a `this`): IDA's guess, not a
// type anyone declared
bool untyped(const tinfo_t& t, bool self = false);
// a reason's bucket for the counts: the text before the first of the stop characters, trailing blanks dropped
std::string bucket(const std::string& why, const char* stops);

// IDA's x86 registers 0-7 (eax ... edi), as fnvd::original::UserRegs names them; nullptr for esp and ebp
extern const char* const kUserRegisters[8];
// the register family (0 eax ... 7 edi) of a register location UserRegs can load: a whole dword, its low word (IDA's
// ax ... di share the dword's number) or its low byte (IDA's al, cl, dl, bl: 16-19); -1 for another location
int userFamily(const argloc_t& where);
// the UserRegs member of a location userFamily can load ("" for another location)
std::string userRegister(const argloc_t& where);
// a location by IDA's register name ("a register pair", "a location IDA cannot name" for others)
std::string locationText(const argloc_t& where, size_t size);
// a __usercall/__userpurge's result where the seam's usercall adapter leaves it, by its type: "" for void, "eax" (al,
// ax), "edx:eax" or "st0" (a float or double); nullopt for a record, an unsized type or another location
std::optional<std::string> userResult(const func_type_data_t& fd);

}  // namespace idakit
