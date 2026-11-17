// idakit infer: the types of the slots (return, parameters) of functions src declares with IDA's guesses, from the
// exe's evidence alone (docs/IDAKIT.md "idakit infer"): what the callers pass (each argument whose value comes from an
// evidence declaration: a bound from below) and what the body does with each value (member accesses, virtual calls,
// values passed on to an evidence slot, stored into or compared with an evidence field or global, returned: bounds
// from above), read from the emitter's analysis of the function and of its callers (`analyze`), then solved in
// rounds: a slot proven in one round is evidence in the next, so round 1 rests on the PDB's and src's declarations
// only.
#pragma once

#include "idakit/Emit.hpp"

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace idakit
{

class Infer
{
public:
	// a slot of a function: its return, `this` or a src parameter (index from 0)
	static constexpr int kReturn = -2;
	static constexpr int kThis = -1;

	// Where a value comes from, evaluated in each round: a parameter or `this` of a function, a callee's result, a
	// field read through a pointer (receiver: a source; offset and width in bytes), the address of a member (offset;
	// width: the bytes of what is addressed), a global (its src ref) or its address, the address of an object a
	// constructor builds (key: its class's record key), a number. adjust: the value is the source's plus adjust bytes.
	struct Source
	{
		enum class Kind
		{
			none,
			parameter,
			self,
			result,
			field,
			address,
			global,
			globalAddress,
			object,
			number,
		};
		Kind kind = Kind::none;
		uint64_t function = 0;  // parameter, self: the function; result: the callee
		int slot = 0;			// parameter
		std::string key;		// object: the record key
		const SrcModel::Ref* global = nullptr;
		std::shared_ptr<const Source> receiver;  // field, address
		int64_t offset = 0, width = 0;
		int64_t adjust = 0;
		uint64_t at = 0;  // the instruction it is read at
	};

	// What a body does with a value: reads or writes width bytes at offset through it (width 0: at an unknown
	// offset); calls the virtual at byte slot of the vtable at offset; passes it (plus adjust) to a callee's slot;
	// stores it into or compares it with a destination; returns it from function; uses it as an integer (signedness:
	// 1 signed, -1 unsigned, 0 neither; width: its bytes) or a float.
	struct Use
	{
		enum class Kind
		{
			access,
			vcall,
			pass,
			store,
			compare,
			returned,
			integer,
			floating,
			typeTest,
		};
		Kind kind = Kind::access;
		uint64_t at = 0;
		int64_t offset = 0, width = 0, slotOffset = 0, adjust = 0;
		bool write = false;
		uint64_t callee = 0;  // pass: the callee; returned: the function
		const SrcModel::Function* function = nullptr;  // pass: the callee's src function
		int slot = 0;								   // pass: its slot
		std::shared_ptr<const Source> destination;	   // store, compare
		int signedness = 0;
	};

	struct Slot
	{
		std::vector<Source> below;  // the values passed (a parameter) or returned (the return)
		std::vector<Use> uses;		// what the body (a parameter) or the callers (the return) do with it
		bool rebound = false;		// the body writes the parameter: its uses are not the argument's
	};

	// one row of infer.tsv
	struct Row
	{
		uint64_t address = 0;
		std::string function, mangled, slot, current, proposed, verdict, evidence, truth, kind;
		// a proven slot's clause of the `// Inferred type: ...` comment (`<slot>: <evidence>`)
		std::string clause;
		int round = 0;
	};

	Infer(Context& context, std::set<uint64_t> hidden);

	// a function whose slots are inferred, with its src declaration
	void target(uint64_t address, const SrcModel::Function& declaration);
	bool isTarget(uint64_t address) const { return m_targets.count(address) != 0; }
	// the analysed function's facts: its calls of targets, and its own body's where it is a target
	void read(const Function& function);
	// a target the analysis could not read (no ctree)
	void failed(uint64_t address, const std::string& why) { m_targets.at(address).why = why; }
	// the rows, rounds solved to their fixpoint; truth: compare each proven proposal with the declaration's own
	// type (the hidden functions' PDB types)
	std::vector<Row> solve(bool truth);

private:
	struct Target
	{
		const SrcModel::Function* declaration = nullptr;
		std::map<int, Slot> slots;
		std::set<uint64_t> callers;
		std::string why;
	};
	// a slot's proposal once proven, by the round that proved it
	struct Proven
	{
		std::string key;
		int round = 0;
	};
	struct Bound;
	struct Solution;
	// A src field at an offset of a record, through its bases, member records and arrays: innermost, the innermost
	// member starting there of width bytes (a pointer inside a smart pointer); outermost, the outermost (the member
	// an address names); covering, any one covering those bytes. type: its type (an array's element's); evidence:
	// every member on the way is evidence (fieldEvidence).
	enum class Find
	{
		innermost,
		outermost,
		covering,
	};
	struct FieldAt
	{
		const SrcModel::Field* field = nullptr;
		std::string owner, type;
		bool evidence = false;
	};

	// the src type of a source this round: the key, and what it rests on (empty: no evidence)
	std::string typeOf(const Source& source, std::string& rests, int depth = 0) const;
	// the type of a target's or an evidence declaration's slot this round ("" for none)
	std::string slotType(uint64_t address, const SrcModel::Function* function, int slot) const;
	// the declaration's slot is evidence (not hidden, no fold): an evidence or inferred declaration's, a slot an
	// `// Inferred type` clause names (a target's only so)
	bool evidence(uint64_t address, const SrcModel::Function* function, int slot) const;
	// the address is a linker fold: src names several functions there, or keeps a `Same function as` comment for it
	bool folded(uint64_t address) const;
	std::optional<FieldAt> fieldAt(const std::string& record, int64_t offset, int64_t width, Find find) const;
	// a field's type is evidence: the PDB's, or a field of src's own record (no field the PDB types) that is no
	// IDA placeholder
	bool fieldEvidence(const std::string& owner, const SrcModel::Field& field) const;
	// the record has base's subobject at offset (itself at 0)
	bool derivesAt(const std::string& record, const std::string& base, int64_t offset) const;
	// "" where the class has every upper bound's base and every member and virtual the uses name; else the first it
	// lacks
	std::string lacks(const std::string& cls, const std::vector<Bound>& uppers, const std::vector<Use>& uses) const;
	// truth: the declaration's own type to compare the proposal with ("" for none)
	Solution solveSlot(uint64_t address, const Target& target, int slot, const Slot& facts,
					   const std::string& truth) const;

	Context& m_context;
	std::set<uint64_t> m_hidden;
	std::map<uint64_t, Target> m_targets;
	std::map<std::pair<uint64_t, int>, Proven> m_proven;
	int m_round = 0;
	mutable std::map<std::string, bool> m_pdbRecords;  // a record has a field the PDB types
	mutable std::map<uint64_t, bool> m_folded;
};

}  // namespace idakit
