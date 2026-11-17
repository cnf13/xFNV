// srcdb's model of src (`srcdb index`; the schema is harness/srcdb/Model.hpp), read-only and whole into memory:
// what idakit sync builds IDA types from. Type columns hold type keys (`builtin:int`, `ptr(K)`, `record:NAME`, ...).
#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace idakit
{

class SrcModel
{
public:
	// the model format this reader understands (srcdb's kSchemaVersion)
	static constexpr int kFormatVersion = 22;

	// What a src function's declaration rests on (srcdb's function.provenance): the PDB's name (Pdb), src's own
	// (Src), idakit infer's types (Inferred: `// Inferred type` above it), a Havok library symbol's signature
	// (Library: `// Havok SDK signature (...)` above it, idakit declare --hk-symbols'), or IDA's guesses (Guess: a
	// name marker of RULES.md, IDA's parameter names, IDA's `sub_` name). Nested: EmitPrinter has a Provenance of its
	// own.
	enum class Provenance
	{
		Pdb,
		Src,
		Inferred,
		Library,
		Guess,
	};

	struct Type
	{
		std::string key, kind, name, of;
		int64_t count = 0, size = 0;
		bool isSigned = false, isFloat = false, variadic = false, isConst = false;
		std::string cc, ret, cls;
		std::vector<std::string> params;
	};
	struct Field
	{
		std::string name, type;
		int64_t offsetBits = 0;
		std::optional<int64_t> bits;  // a bit-field's width
	};
	struct Base
	{
		std::string record;  // the base record's name
		int64_t offset = 0;
		bool isVirtual = false;
	};
	struct Slot
	{
		std::string kind;      // method | deleting
		std::string function;  // mangled
		std::string name;      // qualified
		bool pure = false;
	};
	struct Vtable
	{
		int64_t offset = 0;  // of its vfptr in the complete object
		std::vector<Slot> slots;
	};
	struct Record
	{
		std::string name, file, tag, pattern;  // pattern: the class template of an instance
		std::string spelled;                   // an instance's name without its default arguments ("": the same)
		int64_t size = 0;
		int64_t align = 0;  // alignof
		int64_t line = 0, endLine = 0;  // its name's line and its closing brace's (0: none, as a declaration)
		bool vfptr = false;   // its own vfptr at 0
		bool pinned = false;  // a static_assert (FNVD_LAYOUT) pins its sizeof: src's size is the exe's
		bool trivialCopy = false;  // its copy assignment is a byte copy (no user-declared or member operator=)
		bool abstract = false;	   // it has a pure virtual: C++ has no object, array or value of it
		std::vector<Field> fields;
		std::vector<Base> bases;
		std::vector<Vtable> vtables;
		// the base members (mangled) its using-declarations bring into its scope (srcdb's using_member)
		std::vector<std::string> usingMembers;

		// its own field of the name (not a base's); nullptr for none
		const Field* field(const std::string& fieldName) const
		{
			for (const Field& f : fields)
				if (f.name == fieldName)
					return &f;
			return nullptr;
		}
	};
	struct Enumerator
	{
		std::string name;
		int64_t value = 0;
	};
	struct Enum
	{
		std::string name, underlying, file;
		int64_t line = 0;  // its name's
		bool scoped = false;
		std::vector<Enumerator> enumerators;  // in declaration order
	};
	struct Typedef
	{
		std::string name, file, target;
	};
	struct Function
	{
		std::string mangled, name, type, cls, file;  // file: the declaration's
		int64_t line = 0;                            // the declaration's
		std::string structor;  // "ctor" or "dtor" for a C++ constructor or destructor
		bool empty = false;	   // a structor with no code (srcdb's emptiness: no vptr, statements or initialisers)
		bool isStatic = false, isVirtual = false;
		bool noreturn = false;  // `[[noreturn]]` (clang's isNoReturn): the function never returns to its caller
		std::vector<std::string> params;  // names (the declaration's; an unnamed one its definition's)
		// the arguments a call must pass where default arguments let it pass fewer than the parameters; -1: all
		int64_t required = -1;
		// a member's access from namespace scope: the narrowest of its own and its enclosing classes' (public,
		// protected, private); "" for a free function
		std::string access;
		Provenance provenance = Provenance::Src;
		// srcdb's function_slot: the slots a declaration's evidence covers (`ret`, a parameter's index from 0) and
		// those an `// Inferred type` clause names (inferred, each evidence of its own)
		std::vector<std::string> slots;
		std::set<std::string> inferredSlots;

		// the declaration is evidence of the exe's types: the PDB's, src's or a library symbol's, not inferred or
		// IDA's guess
		bool evidence() const
		{
			return provenance == Provenance::Pdb || provenance == Provenance::Src || provenance == Provenance::Library;
		}
		// the slot's type is evidence: the declaration's, inferred as a whole or this slot by its clause
		bool slotEvidence(const std::string& slot) const
		{
			return evidence() || provenance == Provenance::Inferred || inferredSlots.count(slot) != 0;
		}
	};
	// where a function is defined
	struct Definition
	{
		std::string file;
		int64_t line = 0, endLine = 0;
		bool instance = false;  // a template's instance: its template's definition (no specialization)
		bool emitted = false;   // inside the emitter's guard (`#if FNVD_HYBRID  // emitted` ... `#endif  // emitted`)
	};
	// one declaration src spells of an explicit specialization (a class template instance's member, a function
	// template's specialization)
	struct Specialization
	{
		std::string file;
		int64_t line = 0, endLine = 0;  // its `template <>` line, its last
		bool definition = false;        // it has the body
		bool isInline = false;          // `inline` written
	};
	struct Abi
	{
		std::string cc, ret;  // ret: CodeGen's ABI kind (direct, extend, indirect, ignore, ...)
		int64_t stackBytes = 0;
	};
	// an inline trivial accessor: a method whose header body is `return m_x;` (load), `return m_x[param];` (index) or
	// `return &m_x;` (address), m_x a field of this
	struct Accessor
	{
		const Function* function = nullptr;
		std::string kind, field;
		int64_t param = -1;  // index: the parameter's index
	};
	// one argument of a forwarder's fnvd::original call
	struct ForwardedArgument
	{
		std::string name;      // the forwarding function's parameter it passes on; empty where none
		bool literal = false;  // a constant passed without a cast: its type says nothing of the exe's
		std::optional<int64_t> value;  // an integer constant's value, as the forwarder passes it
	};
	struct Ref
	{
		std::string file;
		int64_t line = 0, col = 0;
		std::string kind, form;
		uint64_t address = 0;
		std::string target, function, type;  // type: a global's, or a forwarder's call (an fn key)
		bool local = false;
		// a fold's owner as its `Same function as` comment spells it (srcdb's context); empty for any other ref
		std::string context;
		// a forwarder in a class template's own member (srcdb's `pattern:` key), one ref per instance src completes
		// from it: the template's text, none of an instance's own
		bool generic = false;
		std::vector<ForwardedArgument> arguments;  // a forwarder's
		// a usercall forwarder's or registration's (srcdb's ref_location): each parameter's register (eax, ecx, edx,
		// ebx, esi, edi) or `stack`, by its index (`this` -1); empty for any other ref
		std::map<int64_t, std::string> locations;
	};
	// an expression of src naming a function by its identifier (srcdb's function_use): the name's spelling
	struct Use
	{
		std::string file;
		int64_t line = 0, col = 0;  // 1-based; col in bytes
	};
	// a function of the Xbox debug PDB: its qualified name, class (a member's) and type as a model key
	struct PdbFunction
	{
		std::string name, cls, type;
		bool isStatic = false;
		int64_t codeSize = 0;
		std::vector<std::string> params;  // names, `this` aside
	};
	// a data member of a record of the Xbox debug PDB (srcdb's pdb_field): its name and type as a model key
	struct PdbField
	{
		std::string name, type;
	};

	// Throws Error when the file is missing or of another format.
	explicit SrcModel(const std::string& path);

	const Type* type(const std::string& key) const { return find(m_types, key); }
	const Record* record(const std::string& name) const { return find(m_records, name); }
	const Enum* enumeration(const std::string& name) const { return find(m_enums, name); }
	const Typedef* typedefNamed(const std::string& name) const { return find(m_typedefs, name); }
	const Function* function(const std::string& mangled) const { return find(m_functions, mangled); }
	const Abi* abi(const std::string& mangled) const { return find(m_abis, mangled); }
	// the declarations of a qualified name (its overloads)
	const std::vector<const Function*>& functionsNamed(const std::string& name) const;
	// the user-declared C++ constructors and destructors of a class
	const std::vector<const Function*>& structorsOf(const std::string& cls) const;
	// the conversion functions a class declares itself (`operator float`, `operator T*`)
	const std::vector<const Function*>& conversionsOf(const std::string& cls) const;
	// the inline trivial accessors of a class (its own methods)
	const std::vector<Accessor>& accessorsOf(const std::string& cls) const;
	// the inline steps of a class template instance's inline member (srcdb's steps row, its step language), or null
	const std::string* steps(const std::string& mangled) const { return find(m_steps, mangled); }
	const Definition* definition(const std::string& mangled) const { return find(m_definitions, mangled); }
	// every definition by mangled name
	const std::unordered_map<std::string, Definition>& definitions() const { return m_definitions; }
	// the declarations src spells of each explicit specialization, by mangled name, each in (file, line) order
	const std::map<std::string, std::vector<Specialization>>& specializations() const { return m_specializations; }
	// the src .cpp of a class: the one holding the most of its out-of-line member definitions, else its header's
	// sibling .cpp when that is a TU; "" for none
	std::string classSource(const std::string& cls) const;
	// whether the index compiled the .cpp (srcdb's `tu`): a src .cpp that is not one is newer than the model
	bool isTu(const std::string& path) const { return m_tus.count(path) != 0; }

	// every record by name
	const std::map<std::string, Record>& records() const { return m_records; }
	// every enum and typedef by name
	const std::unordered_map<std::string, Enum>& enums() const { return m_enums; }
	const std::unordered_map<std::string, Typedef>& typedefs() const { return m_typedefs; }
	// every type by key
	const std::unordered_map<std::string, Type>& types() const { return m_types; }
	// every declared function by mangled name
	const std::unordered_map<std::string, Function>& functions() const { return m_functions; }
	// the refs by address, each address's in (file, line, col) order; a forwarder in a class template's member once
	// per instance's member made from it (its target and function the instance's), without a function where src
	// completes no instance
	const std::map<uint64_t, std::vector<Ref>>& refs() const { return m_refs; }
	const std::vector<Ref>& refsAt(uint64_t address) const;
	// src registers a body for the address (any FNVD_REPLACES* macro: a body, constructor or destructor ref)
	bool registers(uint64_t address) const;
	// src registers a body for the address that is hand-ported: its definition lies outside the emitter's guard
	bool handPorted(uint64_t address) const;
	// the addresses a forwarder's src function is its own body at: those of the function itself (its mangled name),
	// or for a forwarder ref without one, of every function of its qualified name. A function whose body stands at
	// another address is that address's, not its forwarders' callee's: chooseRef counts its forwarders as calls.
	const std::vector<uint64_t>& bodyAddresses(const Ref& forwarder) const;
	// the addresses the function (its mangled name) is its own body at: those src registers it at (FNVD_REPLACES*,
	// a C++ structor's included) and a kept body's FNVD_ADDRESS
	const std::vector<uint64_t>& bodyAddresses(const std::string& mangled) const;
	// the PDB's functions by qualified name, each name's overloads
	const std::unordered_map<std::string, std::vector<PdbFunction>>& pdbFunctions() const { return m_pdbFunctions; }
	// the PDB's records' data members by the PDB's record name (one per distinct name and type)
	const std::unordered_map<std::string, std::vector<PdbField>>& pdbFields() const { return m_pdbFields; }
	// The PDB's type of a src declaration whose src type is the one the PDB declares (srcdb's pdb_typed, its match:
	// harness/srcdb/Index.cpp): kind "param" (key: a function's mangled name; item: a parameter's index from 0, `this`
	// aside, "ret", or "this": a member of the PDB's class of its name), "field" (key: the record; item: the field's
	// name), "global" (key: a global's or static member's qualified name; item ""), "local" (key: a function's mangled
	// name; item: a local of its PDB function, which has no src declaration: the type is the PDB's alone); "pair",
	// "recorded" and "field_recorded" (the `// PDB:` comment line: a function's slot, a record's field) as
	// harness/srcdb/Model.hpp has them. nullptr for none.
	const std::string* pdbTyped(const std::string& kind, const std::string& key, const std::string& item) const;
	// The expressions naming the function (its mangled name) by its identifier, in (file, line, col) order: read
	// from the model's file on each call (only a renaming tool asks)
	std::vector<Use> uses(const std::string& mangled) const;

	// clang's size of a type in bytes; nullopt for an incomplete or unknown type
	std::optional<int64_t> sizeOf(const std::string& key) const;
	// clang's alignment of a type in bytes (alignof); nullopt for an incomplete or unknown type
	std::optional<int64_t> alignOf(const std::string& key) const;
	// the key with typedefs, cv and arrays peeled off (the record a field's storage holds)
	std::string storage(const std::string& key) const;
	// C++'s empty class: no data, no vfptr, every base empty
	bool isEmpty(const Record& record) const;
	// C++ copies the record as its bytes, constructing and assigning alike: its copy assignment is a byte copy, it
	// declares no destructor and no constructor of one reference parameter, and so do its bases and record members
	bool byteCopied(const Record& record) const;
	// An explicit specialization of a class template (`template <> class ctype<char> {`), whose members are declared
	// and defined as any class's: `template <>` on the record's line or the one before it in its file (srcdb does not
	// record the specialization kind). False for a record that is no template instance.
	static bool explicitSpecialization(const Record& record);
	// The `template <>` heads an explicit specialization of a function (its mangled name, its class) takes: one per
	// class template instance among its class and the classes it is nested in, except an explicit specialization of
	// a class template (its members are defined as any class's), and one for a function template's specialization
	// (MSVC's mangled name of one starts `??$`).
	int templateHeads(const std::string& mangled, const std::string& cls) const;

private:
	template <class Map>
	static const typename Map::mapped_type* find(const Map& map, const std::string& key)
	{
		auto found = map.find(key);
		return found == map.end() ? nullptr : &found->second;
	}

	std::string m_path;
	std::unordered_map<std::string, Type> m_types;
	std::map<std::string, Record> m_records;
	std::unordered_map<std::string, Enum> m_enums;
	std::unordered_map<std::string, Typedef> m_typedefs;
	std::unordered_map<std::string, Function> m_functions;
	std::unordered_map<std::string, std::vector<const Function*>> m_functionsByName;
	std::unordered_map<std::string, Abi> m_abis;
	std::map<uint64_t, std::vector<Ref>> m_refs;
	std::unordered_map<std::string, std::vector<uint64_t>> m_bodyAddresses;        // by mangled name
	std::unordered_map<std::string, std::vector<uint64_t>> m_bodyAddressesByName;  // by qualified name
	std::unordered_map<std::string, std::vector<PdbFunction>> m_pdbFunctions;
	std::unordered_map<std::string, std::vector<PdbField>> m_pdbFields;
	static std::string pdbTypedKey(const std::string& kind, const std::string& key, const std::string& item);
	std::unordered_map<std::string, std::string> m_pdbTyped;  // by pdbTypedKey
	std::unordered_map<std::string, std::string> m_classSources;  // from the definitions
	std::unordered_set<std::string> m_tus;
	std::unordered_map<std::string, std::vector<const Function*>> m_structors;
	std::unordered_map<std::string, std::vector<const Function*>> m_conversions;  // by class
	std::unordered_map<std::string, std::vector<Accessor>> m_accessors;  // by class
	std::unordered_map<std::string, std::string> m_steps;  // by mangled name
	std::unordered_map<std::string, Definition> m_definitions;
	std::map<std::string, std::vector<Specialization>> m_specializations;
};

}  // namespace idakit
