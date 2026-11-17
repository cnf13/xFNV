// idakit sync: src pushed into the open IDA database, deterministically and idempotently. Every entity src has
// (records, vtables, enums, function prototypes, globals, names) overwrites IDA's; what src lacks is touched only
// with exe facts (exefacts). Nothing per function is sync's: the stack points and the user lvar settings are the
// locals pass's (idakit frames, Frames.hpp, then pdb_locals.py), the other Hex-Rays per-function stores
// (user-defined calls, operand types) Unfold's, built in each emit and decompile session (Unfold.hpp). The save
// check holds sync to that: recordFrames before the first phase, changedFrames after the last.
//
// The phases run in the order of the sections below; each section's members are that part of the sync. The result
// depends on no order, and a second run changes nothing (writeState's state.tsv is equal).
#pragma once

#include "idakit/AsmFacts.hpp"
#include "idakit/SrcPush.hpp"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace idakit
{

class Sync : public SrcPush
{
public:
	// the database must be open, with Hex-Rays
	Sync(const SrcModel& model, const ExeVtables& exe, exefacts::Facts& facts, SrcTypes& types)
		: SrcPush("sync", model, exe, facts, types)
	{
	}

	// the save check (SyncCheck.cpp)

	// Records every game function's user lvar settings, stack points (IDA's own included) and purged bytes.
	void recordFrames();
	// The functions whose user lvar settings or stack points differ from recordFrames', in address order, each as
	// `0xADDR: -ROW ... +ROW ...` (functionState's rows only before, only after), each row logged `save-check`;
	// with handLocalsToPass, but those of localsLost (logged `locals-lost` instead). Each function whose purged bytes
	// (func_t.argsize) changed is logged `purge-changed` (counted `purges changed`), and each whose purged bytes
	// differ from the exe's pops (exefacts' pops, where known) counted `purges not the exe's`.
	std::vector<std::string> changedFrames(bool handLocalsToPass);
	// changedFrames' functions whose prototype writePrototype changed, or which call such a function directly, and
	// whose difference is in their lvar settings alone (Hex-Rays drops the settings of a retyped function's own
	// locals, and its callers' at the call, as it re-derives them), in address order, counted `locals lost`: the
	// locals pass's to make again (`pdb_locals.py --fresh --only`, the command's DIR/locals-lost.txt). Never a
	// stack point.
	const std::vector<ea_t>& localsLost() const { return m_localsLost; }

	// types (SyncTypes.cpp)

	// Each hand-made IDA type ida_type_aliases.tsv names for a src record or enum is replaced by a typedef of src's
	// spelling (created where absent), so that its users read src's type. Not a rename: IDA's globals and prototypes
	// keep the old name of a renamed type. A row naming a type src declares itself is a table defect: records() and
	// enums() write src's type over the typedef.
	void aliases();
	// Every src enum written over IDA's enum of its name (or created): src's width and enumerators, each named
	// `Enum::enumerator`.
	void enums();
	// Every src record written over its IDA struct (pushName) at src's size: src's bases, vptr, fields (anonymous
	// structs flattened, anonymous unions as `___u<idx>` members, bit-fields) and nothing else, each record after its
	// bases and by-value member records.
	void records();
	// Every src vtable as `X_vtbl` (primary) or `X_%04X_vtbl` (at offset N): one function pointer per src slot.
	void vtables();

	// functions (SyncFunctions.cpp)

	// Every game function src declares gets its src prototype (prototype(declared(a), a)). Where the exe's code
	// contradicts it (a register input it does not pass, ExePrototype's contradicted, a void return whose callers read
	// eax, an integer return narrower than every return path writes and the callers read), it is still pushed and
	// logged `src-contradicted`, a src fix. A function src declares through a usercall forwarder or registration
	// (usercallDeclared) gets its usercallPrototype, src's types at the locations src records; where that does not
	// match the database's usercall type it keeps IDA's (logged `usercall-refused` with why). The folds are found
	// first (findFolds): a typed fold's address is left to foldPrototypes.
	void prototypes();
	// The folds, each given its class-neutral prototype.
	void foldPrototypes();
	// Every data address src declares a global or static member at gets its src type, as one item of its size, and
	// its name: the first ref by not file-local, static member, then file; IDA's items in the way go (logged
	// `global-retyped` with IDA's item); an address inside code (a function src overlays) is left alone, and one
	// inside the global before it is logged `global-overlap`.
	void globals();
	// Every game function src declares gets IDA's spelling of its src name (idaFunctionName), a fold its registered
	// body's; a fold without one keeps IDA's name.
	void names();

	// exe

	// Pins, on every game function src does not declare (srcDeclares), what the exe's code says of its interface: a
	// C-variadic function's `...`, register inputs as a usercall, a void function's return, a return's width and the
	// register arguments the code never reads dropped (SyncExe.cpp).
	void pins();
	// Clears IDA's outlined-code mark (FUNC_OUTLINE, set by Lumina here) on every game function, the one exe phase
	// not taken from exefacts: Hex-Rays inlines an outlined function into the code that calls or jumps to it, while
	// MSVC 2008 outlines nothing, so each is a real function (SyncExe.cpp).
	void outlined();

private:
	// types

	// the record has an IDA name of its own: it is not TU-local (srcdb's `Name @file.cpp`) or an anonymous member's
	bool idaNamed(const SrcModel::Record& record);
	// the record's layout written under its IDA name, after its bases' and by-value member records' (done: those
	// visited; before: IDA's size of each struct it had before the layouts)
	void writeAfterParts(const SrcModel::Record& record, std::set<std::string>& done,
						 const std::map<std::string, int64_t>& before);
	// the record's layout as an unnamed IDA udt of src's size; nullopt where IDA rejects it (logged)
	std::optional<tinfo_t> layout(const SrcModel::Record& record);
	// the record's fields as members of the holder's udt at offsetBits (an anonymous struct's flattened into it)
	void addFields(const SrcModel::Record& record, int64_t offsetBits, const std::string& holder,
				   udt_type_data_t& udt);
	void writeVtable(const SrcModel::Record& record, const SrcModel::Vtable& table, const std::string& ida);

	// IDA struct name -> the src record records() writes there (the first in name order)
	std::map<std::string, std::string> m_pushed;

	// functions: IDA's name at each address set to the wanted one (globals, names), logged under `<kind>-`
	void setNames(const std::map<ea_t, std::string>& wanted, const std::string& kind);
	// The function's type made the given one (prototypes, folds, pins), applied only where the stored type is not it
	// (counted `prototype-unchanged`: it serializes equal, or differs only in spelling a reference to the same named
	// type by its tag, `enum X` for `X`): Hex-Rays drops the function's lvar settings at its arguments whenever a type
	// is applied, and the analysis an applied type queues drops the callers' at the stack slots it types. An applied
	// one is logged `prototype-changed`, and the save check leaves out the function's lvar settings at its arguments;
	// false where IDA rejects it (the stored type kept). A function IDA has as never returning (its stored type's
	// `__noreturn` or FUNC_NORET) gets the type as `__noreturn` (logged `noreturn-kept` where the given one is not).
	bool writePrototype(ea_t address, const tinfo_t& written);

	// the pins on one function (SyncExe.cpp)
	void pinVariadic(ea_t address);
	void pinUsercall(ea_t address);
	void pinReturn(ea_t address);
	void pinReturnWidth(ea_t address);
	void pinDeadRegisters(ea_t address);

	// the save check: functionState's lvar and every-point rows, and the purged bytes, of each game function; the
	// functions whose prototype writePrototype changed
	std::map<ea_t, std::vector<std::string>> m_frames;
	std::map<ea_t, uint64_t> m_purges;
	std::set<ea_t> m_prototypeChanged;
	std::vector<ea_t> m_localsLost;

	AsmFacts m_asm;
};

}  // namespace idakit
