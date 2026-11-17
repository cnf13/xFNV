// The functions a vtable tail jump can reach (exefacts' SlotTargets, which exefacts' pops reads): the receiver's
// class from the database's types, its slot's functions from the exe's vtables (ExeVtables::slotFunctions). Each
// command that reads the exe's pops installs one on its exefacts::Facts for its run (setSlotTargets).
//
// The receiver starts at the jump's root, the function's own object: for the entry's ecx, each vtable table that
// holds the function (ExeVtables::holders) gives its class and the subobject the table is for, those whose class
// derives from another holder's class left out (that class's slot covers theirs); without a holder, and for a stack
// argument root, the function's database type gives the argument's pointer. Each load reads the pointer member at
// its displacement (bases and embedded members descended into), or an element of an array of pointers; the receiver
// is the outermost class (a base or an embedded member) starting at its displacement in the last pointee. Every root
// must resolve; the targets are every root's receiver class's slot functions (a pure virtual's `_purecall`
// included: its pops, unknown, prove nothing).
#pragma once

#include "idakit/ExeVtables.hpp"
#include "idakit/Ida.hpp"

#include <Facts.hpp>

#include <optional>
#include <string>
#include <vector>

namespace idakit
{

class ExeSlotTargets : public exefacts::SlotTargets
{
public:
	// the database must be open; installs itself on facts, and removes itself when destroyed
	ExeSlotTargets(const ExeVtables& exe, exefacts::Facts& facts);
	~ExeSlotTargets() override;
	ExeSlotTargets(const ExeSlotTargets&) = delete;
	ExeSlotTargets& operator=(const ExeSlotTargets&) = delete;

	std::optional<std::vector<uint32_t>> targets(uint32_t function, const exefacts::VtableJump& jump) override;

private:
	// the class the receiver has, from a pointer to the root object and the subobject offset the root points to
	std::optional<std::string> receiverClass(tinfo_t pointer, int64_t at, const exefacts::VtableJump& jump) const;

	const ExeVtables& m_exe;
	exefacts::Facts& m_facts;
};

}  // namespace idakit
