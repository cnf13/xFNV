// What the parts that push src into the open IDA database share (idakit sync; Unfold, the per-function rebuild emit
// and decompile run): srcdb's model, the converter from src's types to IDA's, the src declaration behind each
// address, the folds, the exe's vtable-slot purges, the run's log and the state dump.
#pragma once

#include "idakit/ExeVtables.hpp"
#include "idakit/Ida.hpp"
#include "idakit/SrcIda.hpp"
#include "idakit/SrcModel.hpp"
#include "idakit/SrcTypes.hpp"

#include <cstdint>
#include <filesystem>
#include <map>
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

class SrcPush
{
public:
	// A fold: one exe function that is several src functions (a `Same function as` comment, or two or more functions
	// src registers or forwards there), each of its members typed by its own prototype.
	struct Fold
	{
		std::vector<const SrcModel::Function*> members;  // by mangled name
		std::optional<tinfo_t> type;					 // the class-neutral prototype; nullopt: none
	};

	// the folds by address, as findFolds found them
	const std::map<ea_t, Fold>& folds() const { return m_folds; }
	// The src functions a user-defined call (a fold owner's) at a call of the address stands for: every src function
	// whose callName is the call's name and whose prototype there is the call's type; several for a name and type
	// src declares more than once.
	std::vector<const SrcModel::Function*> calledAs(ea_t address, const std::string& name, const tinfo_t& type);

	// DIR/<command>-log.tsv: `category key detail`, this run's rows in the order they were logged
	void writeLogs(const std::filesystem::path& dir) const;
	// DIR/state.tsv: the database's state, sorted `kind key text` rows (SrcPush.cpp); two runs are equal exactly
	// where their state.tsv files are
	void writeState(const std::filesystem::path& dir) const;
	// The kinds of a game function's state rows: its user-defined calls (`udcall`), user lvar settings (`lvar`),
	// operand types (`optype`), user stack points (`stkpnt ADDR DELTA`), and with kAllPoints IDA's own points too
	// (`stkpnt ADDR DELTA auto`).
	enum StateKinds : unsigned
	{
		kUdcalls = 1,
		kLvarSettings = 2,
		kOperandTypes = 4,
		kUserPoints = 8,
		kAllPoints = 16,
	};
	// The function's state rows of those kinds, writeState's, appended to rows (unsorted).
	static void functionState(ea_t function, unsigned kinds, std::vector<std::string>& rows);
	const std::map<std::string, int64_t>& counts() const { return m_counts; }

protected:
	// the database must be open, with Hex-Rays; command: the command's name, for its log and errors
	SrcPush(std::string command, const SrcModel& model, const ExeVtables& exe, exefacts::Facts& facts,
			SrcTypes& types);

	void log(const std::string& category, const std::string& key, const std::string& detail = "");
	void count(const std::string& key, int64_t n = 1) { m_counts[key] += n; }
	// the name a user-defined call of the function carries: its IDA name (idaFunctionName), else (a DeletingDestruct,
	// which has no IDA spelling) its src name
	static std::string callName(const SrcModel::Function& function);

	// The src declaration pushed for an address: chooseRef's ref where it is a body or a forwarder (a structor
	// registration's ref is its Construct / Destruct forwarder), else the function of an `address` ref; null for
	// none.
	const SrcModel::Function* declared(ea_t address) const;
	// Whether src declares the function at an address: a declared function or a fold (one without a registered body
	// has no declared function). Every "src declares it" test is this one; valid after findFolds.
	bool srcDeclares(ea_t address) const { return declared(address) || m_folds.contains(address); }
	// Whether src declares the function at an address through a `fnvd::original::usercall` forwarder (srcdb's form
	// `usercall`) or registers its body through FNVD_REPLACES_USERCALL/_USERPURGE: a function with register arguments
	// outside thiscall and fastcall, whose locations C++ cannot spell in a prototype but src's model records (the
	// forwarder's UserRegs, the registration's location list: srcdb's ref_location).
	bool usercallDeclared(ea_t address) const;
	// The prototype sync pushes for a usercallDeclared function: src's declaration as written (prototype(), `this`
	// first for a member) with each parameter at the location the model records for it, the stack ones in their
	// order, the result where its type's standard return puts it, `__userpurge` where the database's type is one
	// (it pops its stack arguments), else `__usercall`. It is matched against the database's usercall type by
	// location, never by position: the same registers, as many stack arguments of the same sizes in the same order,
	// the result in the same place. Where the two disagree on a count or a location, the exe decides (user,
	// 2026-11-11): src's form (a `__userpurge` where the code pops) where the code's facts fit it and contradict the
	// database's (ExePrototype's usercallMisfit), with why the facts that decided it. nullopt, with why, where they
	// fit both, neither or only the database's, where the two disagree on a stack argument's size or the result's
	// place, where the database's type is no usercall, where the model records no location for a parameter or the
	// hidden result pointer of a record src returns by value, or where src's prototype is C-variadic or unmappable.
	std::optional<tinfo_t> usercallPrototype(ea_t address, std::string& why);
	// The exe addresses a call of the function in src reaches: those src registers its body at (bodyAddresses), else
	// the exe addresses its definition calls through forwarders (a whole-body forwarder's is its function's exe body);
	// empty for none (an inline src body that calls no exe function).
	std::vector<ea_t> definedAt(const SrcModel::Function& function);
	// The prototype sync pushes at an address: a fold's class-neutral one, a usercall's usercallPrototype (IDA's
	// type where src's does not match it), else its declared function's; nullopt for none (or one the converter
	// cannot build).
	std::optional<tinfo_t> srcPrototype(ea_t address);
	// Finds the folds and builds each one's class-neutral prototype over its members': a `void *` this, and a return
	// or argument the members disagree on as `void *` (a pointer) or its integer (an enum).
	void findFolds();

	// The converter (SrcPushConvert.cpp). A src type key as an IDA type, cached by key: a record is IDA's struct under
	// pushName (created as an empty fixed struct of src's size where absent or a typedef; one src does not define, a
	// forward declaration), an enum IDA's enum of its name (created from src's enumerators where absent), a typedef its
	// target (one outside src/ and gen/ IDA's type of its name where that is the same type); nullopt for an
	// unmappable key, logged `unmapped-type`.
	std::optional<tinfo_t> typeOf(const std::string& key);
	// The function's src prototype from its declaration: `this` as a pointer to entryClass for a non-static member
	// (shifted, `B *__shifted(D, N)`, where that is a base subobject of D at N), the hidden result pointer where its ABI
	// returns indirectly, its parameters (an array decayed to a pointer) and
	// declared convention. At an address (BADADDR: none), the exe facts translate two of src's spellings: a free
	// `R FNVD_FASTCALL f(T* apThis, void* apEdx, ...)` whose code reads no edx is the exe's thiscall without apEdx, and
	// a static `X::Construct(void* apMemory, ...)` whose code returns `this` a thiscall with `X* this`. nullopt,
	// logged `prototype-unmapped`, where a part is unmappable.
	std::optional<tinfo_t> prototype(const SrcModel::Function& function, ea_t address = BADADDR);
	// the src enum written as IDA's enum of its name (replacing any), false where IDA rejects it (logged)
	bool writeEnum(const SrcModel::Enum& e);

	// The exe's vtable-slot purges (SrcPushPops.cpp). The bytes every exe implementation of the vtable slot (the
	// class's and its derived classes', ExeVtables::slotFunctions) whose pops the exe proves pops, where they agree.
	std::optional<int64_t> slotPops(const std::string& cls, int64_t offset);
	// the bytes an indirect call pops: its vtable slot's implementations, else the vtbl member type's stack bytes
	std::optional<int64_t> indirectPops(func_t* function, ea_t call);
	// the converter's parts: a builtin's IDA type from its C spelling (kPrimitive's), a record's struct, an enum
	std::optional<tinfo_t> primitive(const std::string& cName);
	std::optional<tinfo_t> recordType(const std::string& name);
	std::optional<tinfo_t> enumType(const std::string& name);

	const std::string m_command;
	const SrcModel& m_model;
	const ExeVtables& m_exe;
	exefacts::Facts& m_facts;
	SrcTypes& m_types;
	IdaStructNames m_structNames;

private:
	std::optional<tinfo_t> buildType(const SrcModel::Type& type);

	std::map<std::string, std::optional<tinfo_t>> m_typeCache;
	std::map<std::string, std::vector<std::pair<int64_t, ea_t>>> m_forwarders;  // definedAt: (line, address) by file
	std::map<ea_t, Fold> m_folds;
	std::map<std::string, std::vector<const SrcModel::Function*>> m_byCallName;  // calledAs, built on first use
	std::map<std::pair<std::string, int64_t>, std::optional<int64_t>> m_slotPops;  // slotPops
	std::map<std::string, int64_t> m_counts;
	std::vector<std::vector<std::string>> m_log;
};

}  // namespace idakit
