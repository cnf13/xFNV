// IDA's types as src's type keys (SrcModel's: `builtin:int`, `ptr(K)`, `record:NAME`, ...), and keys as C++.
//
// An IDA type is a src key when every part of it is: a builtin by size, sign and float-ness; a typedef src declares
// under IDA's name; an enum src has by IDA's name, or that harness/tools/ida_type_aliases.tsv names src's enum for at
// IDA's size; a struct or union that is a src record's IDA struct
// (IdaStructNames), or an IDA struct that harness/tools/ida_type_aliases.tsv names src's record for, else whose name
// is a src instance's C++ name (`BSListBase<void*>`), else whose flattened template instance name
// (`BSSimpleArray_UInt16`) fits one src instance (flattened), or an IDA struct named after a src typedef of a record
// (`NiRefObjectPtr`), at src's size; a compiler vector type IDA has as a union (`__m128`) as the compiler's typedef;
// pointers to and arrays of those, and pointers to functions of those (`ptr(fn(cc;ret;params;))`); a pointer to IDA's
// struct or forward declaration named as a record src declares and defines nowhere, as a pointer to that incomplete
// record. Hex-Rays' untyped stand-ins (`_DWORD`, `_BYTE`, ...) as typedefs and anything else are not.
#pragma once

#include "idakit/Ida.hpp"
#include "idakit/SrcIda.hpp"
#include "idakit/SrcModel.hpp"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>

namespace idakit
{

class SrcTypes
{
public:
	// a key's kind and children: `ptr(K)` is {"ptr", of K}, `array(K,4)` {"array", of K, count 4}, `record:X`
	// {"record", name X}
	struct Parts
	{
		std::string kind, of, name;
		int64_t count = 0;
	};
	// a function type key's parts: srcdb's `fn(cc;ret;params;[const])`, a variadic one's params ending in `...`
	struct Function
	{
		std::string cc, ret;
		std::vector<std::string> params;
		bool variadic = false, isConst = false;
	};

	// aliases: the ida_type_aliases.tsv to read (Error when it cannot be); the database must be open (IDA's struct
	// names are looked up in it)
	SrcTypes(const SrcModel& model, const std::string& aliases);

	const SrcModel& model() const { return m_model; }

	// the src key of IDA's type; nullopt where src has none, why: the part that is missing
	std::optional<std::string> keyOf(const tinfo_t& type, std::string& why);
	// the src record whose IDA struct the udt is
	const SrcModel::Record* recordOf(const tinfo_t& udt);
	// The src record a C++ class name stands for (as the PDB or the demangler spells it, `SettingCollectionMap<class
	// Setting>`), through the names IDA's struct for it has or would have: IDA's spellings of it, then its flattened
	// names least cost first (`SettingCollectionMap_Setting`, the template's bare name), each looked up as recordOf
	// does (an IDA struct of the name, else the name's alias or C++ spelling). From a flattened name only a record src
	// models flat or the instance itself, never another instance; nullptr for none.
	const SrcModel::Record* recordOfClass(const std::string& cppName);
	// The src enum an IDA or PDB enum name stands for: src's of the name, else the one ida_type_aliases.tsv names
	// (IDA's `SEX`: src's nested `TESActorBase::SEX`); nullptr for none.
	const SrcModel::Enum* enumOf(const std::string& name) const;
	// ida_type_aliases.tsv's rows: IDA's type name -> src's type as the table spells it
	const std::map<std::string, std::string>& aliases() const { return m_aliases; }
	// The src record a C++ spelling of src's names stands for (the table's `BSSimpleList<void*>`,
	// `ListBox<uint32_t>`): the record as named or as IDA spells it, its typedefs of builtins read as the builtins, or
	// the record a src typedef of the name holds; nullptr for none.
	const SrcModel::Record* recordSpelled(const std::string& spelling) const;

	Parts parts(const std::string& key) const;
	// the function type key's parts; nullopt for another key
	static std::optional<Function> function(const std::string& key);
	static std::string functionKey(const Function& f);
	// The key as a C++ declaration of name (`int* p`, `char (*p)[4]`, `int (FNVD_THISCALL* p)(uint32_t)`; the type
	// alone when name is empty); nullopt where src cannot spell it (member pointer types, clang's names of unnamed
	// records).
	std::optional<std::string> spell(const std::string& key, const std::string& name = "") const;
	// the key with its builtins as RULES' fixed-width names (a typedef key of that name: `typedef:int32_t` for
	// `builtin:int`), at every level, a function's parameters and return included
	std::string fixedWidthKey(const std::string& key) const;
	// typedefs and cv dropped at every level, `long` as `int` (one type on the exe's target): keys that are the same
	// type to a call's argument and to the ABI are equal
	std::string canonical(const std::string& key) const;
	// the src headers (relative to src/) declaring the named types in the key
	void headers(const std::string& key, std::set<std::string>& out) const;
	// the record is base or derives from it with base's subobject at offset 0 (a pointer to it is a pointer to base)
	bool derivesAtZero(const std::string& record, const std::string& base) const;
	// the key with its typedefs resolved at the top (cv kept)
	std::string bare(const std::string& key) const;
	// a pointer or reference to const (the pointer itself const or not)
	bool constPointee(const std::string& key) const;
	// the src record of an instance of the class template pattern (`NiPointer`) the key is, or (pointer) points to;
	// nullptr for any other key
	const SrcModel::Record* templateRecord(const std::string& key, const std::string& pattern, bool pointer) const;

private:
	std::optional<std::string> declarator(const std::string& key, const std::string& inner) const;
	// the src record of IDA's struct (its IdaStructNames record, else its alias's, the record IDA's name spells in
	// C++ (`BSListBase<void*>`), or its flattened name's, at the same size), else nullptr with why set
	const SrcModel::Record* record(const tinfo_t& udt, std::string& why);
	// the same by IDA's struct name; udt: the struct where IDA has one (its size checked, its flattened name read),
	// nullptr where it has none
	const SrcModel::Record* record(const std::string& name, const tinfo_t* udt, std::string& why);
	// The src template instance IDA's flattened struct name stands for, in IDA's words for the arguments
	// (`BSSimpleList_bhkConstraint`, `NiTArray_NiAVObjectPtr`, `hkArray_UInt32`, `NiTMapItem_IntInt`,
	// `NiTMapItem_NiObjectP_Bool`, `X_tList`, `std::vector_ul`), or a bare template name whose struct's member types
	// name the arguments. The name spells a run of the arguments (the leading ones, IDA leaving defaults out, or a
	// later run: NiTMapBase's key and value). Of the instances at the struct's size, the one whose arguments up to its
	// defaulted ones the name spells, at the least cost (argumentWords). "" unless exactly one is left: instances alike
	// to the name (NiTMapBase_IntInt, the DFALL and the NiTPointerAllocator one) stay unresolved.
	std::string flattened(const std::string& name, const tinfo_t& udt);

	const SrcModel& m_model;
	IdaStructNames m_structNames;
	std::map<std::string, std::string> m_aliases;  // IDA's type name -> src's type as the table spells it
	std::unordered_map<std::string, std::string> m_spellings;  // IDA's spelling of a record's name, blanks dropped -> it
	// IDA's spelling of a record src declares and defines nowhere, blanks dropped -> it (keyOf: a pointer to it)
	std::unordered_map<std::string, std::string> m_incomplete;
	std::unordered_map<std::string, std::string> m_flattened;  // flattened's answers
};

// A calling convention's name as src's model spells it (its ABI rows): cdecl, stdcall, fastcall, thiscall, vararg,
// usercall, userpurge, usercall_vararg, pascal, voidarg; "unknown" for another.
const char* conventionName(cm_t cc);
// The IDA convention a src convention name stands for: cdecl, stdcall, thiscall and fastcall; nullopt for another.
std::optional<callcnv_t> conventionCode(const std::string& name);

}  // namespace idakit
