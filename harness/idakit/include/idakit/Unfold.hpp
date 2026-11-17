// One function unfolded in the open IDA database right before its decompile is read (idakit emit, idakit decompile):
// user lvar settings of the locals the steps have evidence for, user-defined call types and operand types at its
// calls, from what sync pushed (types, prototypes), the exe's code and the function's own decompiles, in a session
// that is never saved, so that emit and the porters read the same decompile. A rebuild writes on top of what the
// database holds (the locals pass's lvar settings) and clears nothing. The local steps read src (constructor
// registrations, record types, callee prototypes), so they run in every session; idakit locals (locals) also saves
// the lvar settings they give, and nothing else, for the GUI's decompile.
#pragma once

#include "idakit/SrcPush.hpp"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace idakit
{

class Unfold : public SrcPush
{
public:
	// The database must be open, with Hex-Rays; finds the folds and the C-variadic functions (before any decompile
	// of the session). localsSession: the session is idakit locals' (locals), whose log (locals-log.tsv) holds the
	// local steps' rows alone; else unfold-log.tsv, every row.
	Unfold(const SrcModel& model, const ExeVtables& exe, exefacts::Facts& facts, SrcTypes& types,
		   bool localsSession = false);

	// The function unfolded, then its decompile: its variadic call sites typed (variadicSites, merged into its
	// user-defined calls), then up to kRounds decompiles each followed by the steps below (the local steps, then the
	// call-site steps), until no step changes anything; the rows of what the last round's steps left as they are
	// logged. The decompile returned reflects the stores (one more where the last round changed them); null where
	// Hex-Rays fails (why: its reason).
	cfuncptr_t rebuild(func_t* function, std::string& why);
	// idakit locals: rebuild, then the function's user-defined calls and operand types put back as they were, so
	// that the function gains user lvar settings alone. False where a decompile fails (why: its reason, logged
	// `locals-decompile-failed`), the lvar settings put back as they were too.
	bool locals(func_t* function, std::string& why);
	// What the occurrences of each local but the arguments ask of its type, by lvar index, as value locals weighs
	// them (whatever set the local's type): the types a store, a read or its address passed on asks (an IDA sized
	// unknown as the unsigned integer of its size), and whether no use passes its address on (bounded: the local may
	// take a smaller type). The local trials' candidates (LocalTrials.cpp).
	struct Asked
	{
		std::vector<tinfo_t> types;
		bool bounded = true;
	};
	std::map<int, Asked> askedTypes(cfunc_t& cfunc);

private:
	// SrcPush::log, but for a locals session only the local steps' rows (and its own `locals-` rows)
	void log(const std::string& category, const std::string& key, const std::string& detail = "");

	// the session is idakit locals' (log)
	const bool m_localsSession;
	// the calls whose operand type this session wrote (vtableCalls, indirectCalls): the only ones indirectCalls takes
	// back
	std::set<ea_t> m_operandTyped;

	// locals (UnfoldLocals.cpp)

	// the decompile without Hex-Rays' cache, after which the item types it stored or changed are put back
	// (restoreTypes); null where it fails (why: Hex-Rays' reason)
	cfuncptr_t decompile(func_t* function, std::string& why);
	// An item's stored type and who set it (its AFL_TYPE_GUESSED bits).
	struct StoredType
	{
		std::optional<tinfo_t> type;
		aflags_t source = 0;
	};
	static StoredType storedType(ea_t address);
	// the item's type and its source put back as stored
	static void putBack(ea_t address, const StoredType& stored);
	// Hex-Rays stores the types it guesses (a function's, a global's, its callees') and merges them over the types
	// IDA or it guessed before. Every item whose type or type source a decompile changed is put back as it was
	// (logged `guess-restored`), so that no function's decompile depends on another's: the items recorded before it
	// (the function and every address its items reference) and every item whose type changed during it (IDA's
	// changing_ti event, m_before).
	void restoreTypes(func_t* function);
	// A fold's own body is its owner's (UnfoldFoldOwners.cpp): in an emit or decompile session (not a locals one,
	// which saves), the fold's type for this rebuild is its owner's src prototype, the declared function's
	// (`prototype(declared(fold), fold)`) in place of the class-neutral one sync writes. Returns the type stored
	// before, which rebuild puts back after its last decompile; nullopt where the function is no fold or nothing was
	// written (logged).
	std::optional<StoredType> ownerPrototype(func_t* function);
	// The steps of a round, each true where it changed the function's stores: the local steps (narrowMergedArrays,
	// frameLocals, passedObjects, constructedLocals, valueLocals, reusedArguments: lvar settings), then the call-site
	// steps (vtableCalls, indirectCalls, foldOwners: operand types, user-defined calls). What a step leaves as it is (a
	// kept candidate, a failed write) goes to m_kept.
	bool narrowMergedArrays(func_t* function, cfunc_t& cfunc);
	bool frameLocals(func_t* function, cfunc_t& cfunc);
	bool passedObjects(func_t* function, cfunc_t& cfunc);
	bool constructedLocals(func_t* function, cfunc_t& cfunc);
	bool valueLocals(func_t* function, cfunc_t& cfunc);
	bool reusedArguments(func_t* function, cfunc_t& cfunc);
	bool vtableCalls(func_t* function, cfunc_t& cfunc);
	// Each other indirect call (a function pointer, a global one's included, a vtable slot no exe implementation tells
	// the pops of) that Hex-Rays passes more stack bytes than the exe pushes there (the stack-slot model): typed with
	// the bytes the model counts, as the callee's own type under Hex-Rays' casts where that passes them, else Hex-Rays'
	// type without its last stack arguments; none where src's member type passes other bytes (logged).
	bool indirectCalls(func_t* function, cfunc_t& cfunc);
	// Each call of a fold (folds()) typed as its owner's src function: a user-defined call with the owner's
	// prototype, named with its IDA name (UnfoldFoldOwners.cpp).
	bool foldOwners(func_t* function, cfunc_t& cfunc);

	// this round's rows of what the steps left as they are: category, detail
	std::vector<std::pair<std::string, std::string>> m_kept;
	// the items' types before this decompile (decompile, restoreTypes)
	std::map<ea_t, StoredType> m_before;

	// the class whose constructor or destructor the callee is: src's (it registers the address with
	// FNVD_REPLACES_CONSTRUCT or FNVD_REPLACES_DESTRUCT, or as a C++ constructor or destructor), else the PDB's (IDA's
	// name for it is a PDB X::X or X::~X); cls empty for none, or for a fold (fold: the src functions naming it)
	struct Structor
	{
		std::string cls;
		bool destructor = false;
		std::string fold;
	};
	Structor structorOf(ea_t callee);
	// src's record of a Structor's class (its name, else IDA's struct of the name: a template instance spelled as
	// the PDB does); null for none
	const SrcModel::Record* recordOfStructor(const std::string& cls);
	// the callee is a constructor or destructor of a class template's instance (its record has a pattern): it proves
	// the template, not the instance (constructedLocals, valueLocals)
	bool templateStructor(ea_t callee);
	// src's declaration of the callee returns `void *` (its prototype as sync would push it): untyped memory, such as
	// an allocation's (valueLocals), memoised in m_voidResults
	bool untypedResult(ea_t callee);
	std::map<ea_t, bool> m_voidResults;
	// A direct callee's prototype is evidence of the objects passed to it (frameLocals, passedObjects): src declares
	// it and its body dereferences something (an empty body folded with other classes' fits any parameter type).
	bool typeIsEvidence(ea_t callee);
	// The first call whose stack arguments (callBytes, from esp at the call) hold the IDA frame bytes [start, end):
	// an object built there is that call's by-value argument, built in place (constructedLocals); nullopt for none
	std::optional<ea_t> argumentsAt(func_t* function, sval_t start, sval_t end);

	// PDB X::X and X::~X by classKey(name) -> X (structorOf)
	std::map<std::string, std::string> m_pdbStructors;
	// The local split (LVINF_SPLIT) at the definition at `at` (a constructor call, a store), the piece typed `type`
	// where it is not empty (else the next round's steps type it), for the step (constructedLocals, valueLocals: the
	// log's `<step>-split`); called where the step finds the piece starting at `at` still merged with others. A split
	// this rebuild made there before left them merged: it is undone (`<step>-undone`) and not made again in this
	// rebuild; a split the database held (the debug build's) is left as it is (`<step>-kept`). True where the
	// function's stores changed.
	bool splitLocal(ea_t start, lvar_uservec_t& saved, const lvar_t& local, ea_t at, const tinfo_t& type,
					const std::string& step, const std::string& reason);
	// (function, definition): a split undone in this rebuild (splitLocal)
	std::set<std::pair<ea_t, ea_t>> m_undone;
	// (function, definition): a split this rebuild wrote, the only ones splitLocal undoes
	std::set<std::pair<ea_t, ea_t>> m_split;

	// the stack-slot model (UnfoldVariadicCalls.cpp)

	// The stack arguments the exe passes at a call: their bytes in argument order, 8 for a double stored there, else
	// 4 (less for a last partial dword); unknown with the model's reason
	struct StackArguments
	{
		bool known = false;
		std::vector<int64_t> pieces;
		std::string why;
	};
	// The arguments the exe passes at each of the calls, from the stack-slot model; indirect: the bytes indirect calls
	// pop where the model's own evidence (the exe's code, the operand's vtbl member) does not show them
	std::map<ea_t, StackArguments> stackArguments(func_t* function, const std::set<ea_t>& calls,
												  const std::map<ea_t, int64_t>& indirect);
	// A call of the C-variadic function type typed with the arguments the exe passes there (found: the stack-slot
	// model's), as a non-variadic cdecl; nullopt with the log kind and detail in why
	std::optional<tinfo_t> variadicSiteType(const tinfo_t& type, const StackArguments& found,
											std::pair<std::string, std::string>& why);
	// Each direct call of a C-variadic function (m_variadic) in the function, typed so (variadicSiteType) as a
	// user-defined call
	udcall_map_t variadicSites(func_t* function);
	// the stack bytes a call takes: those its callee pops (removed) and those a cdecl callee reads (taken, the caller
	// cleans them up); nullopt where neither the exe nor src shows them
	struct CallBytes
	{
		int64_t pops = 0;
		int64_t takes = 0;
	};
	std::optional<CallBytes> callBytes(func_t* function, ea_t call, const std::map<ea_t, int64_t>& indirect);

	// the functions whose stored type is C-variadic when the session started
	std::set<ea_t> m_variadic;
};

}  // namespace idakit
