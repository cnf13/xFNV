// What the exe's code says about a function's interface, for typing the functions src does not declare: which
// registers a function reads before writing them (ecx: a `this`, edx: fastcall/usercall), the stack bytes its callers
// release after a call (a cdecl's arguments) and what callers read of its result.
//
// Registers are counted by family (0 eax, 1 ecx, 2 edx, ... 7 edi), a family's 8- and 16-bit parts included.
#pragma once

#include "idakit/Ida.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <unordered_map>

namespace idakit
{

// The register families one instruction reads (the widest read, in bytes) and writes.
struct RegisterUse
{
	std::array<int, 8> reads{};  // 0: not read
	std::array<bool, 8> writes{};

	explicit RegisterUse(const insn_t& insn);
};

// The push of a call's 4-byte stack argument: the register a push writes into the argument's slot (offset: its stack
// offset at the call; IDA's sp trace places the push) and that register's last write before the push (source), all
// in the call's block (no join between them and the call); a push of a constant is its own source. afterCall: a call
// runs between source and the call (a nested call building another argument or the receiver, which pushes and pops
// below the slot). nullopt where that cannot be shown.
struct PushedArgument
{
	insn_t source;
	bool afterCall = false;
};
std::optional<PushedArgument> pushedArgument(ea_t callSite, int64_t offset);

// The call's 4-byte stack argument (offset) is a bool the exe extended to the whole dword before the push
// (pushedArgument's source; a call between it and the call only where the push follows it at once): a movzx of a
// byte, a setcc of the register's low byte after an `xor r, r` with no other write to r between them in the block,
// or the 0 or 1 of `sbb r, r; neg r` or `sbb r, r; add r, 1`. False where that cannot be shown.
bool pushesExtendedBool(ea_t callSite, int64_t offset);

// An indirect call through a vtable slot passes as its `this` the pointer whose vtable it reads the slot from: the
// callee is `[vtbl+N]` (or a register a dword load from there wrote), vtbl a dword load from `[p+M]`, and `this` (ecx
// where stackOffset is nullopt, else the register pushed into the stack argument at that offset) is p+M, each value
// traced through register copies and `lea` within the call's block. False where that cannot be shown.
bool callsOnVtableObject(ea_t callSite, std::optional<int64_t> stackOffset);

class AsmFacts
{
public:
	// a call site's first use of the result: an x87 instruction (the result in ST0)
	static constexpr int kSt0 = -1;

	// A path from the function's entry reads the register family before writing it (a call writes eax, ecx, edx).
	bool readsBeforeWrite(ea_t function, int family) const;
	// The direct calls to the function.
	const std::vector<ea_t>& callSites(ea_t function);
	// Per call site that releases stack right after the call (`add esp, N`, or a run of `pop ecx`): the bytes.
	std::vector<int64_t> cleanup(ea_t function);
	// Per call site: the eax bytes the caller reads first after the call (1, 2, 4), kSt0, or 0 (eax written first,
	// or the flow leaves).
	std::vector<int> resultReads(ea_t function);
	// The call passes its caller's own `this` in ecx, unchanged: the last write to ecx before it loads the frame slot
	// or copies the register the caller's entry block saved ecx in (and nothing else writes), or ecx is not written
	// at all on the way from the entry to a call in the entry block. False where that cannot be shown.
	bool passesOwnThis(ea_t callSite) const;

private:
	std::unordered_map<ea_t, std::vector<ea_t>> m_callSites;
};

}  // namespace idakit
