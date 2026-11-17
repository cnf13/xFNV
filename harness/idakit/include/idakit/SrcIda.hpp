// Where src (SrcModel) and the IDA database name the same thing: the src ref that declares an exe address, and the
// IDA struct of a src record. idakit sync and idakit emit share them.
#pragma once

#include "idakit/Ida.hpp"
#include "idakit/SrcModel.hpp"

#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace idakit
{

// the MSVC demangler's spellings of a C++ type name (`X<A *,B>`, `> >`), as IDA names its structs
std::vector<std::string> idaSpellings(const std::string& name);
// src's msvc9:: is the exe's std:: (IDA's std:: classes) or the CRT's unqualified names (_s__ThrowInfo): the spellings
// with it replaced, none when there is none
std::vector<std::string> msvc9Variants(const std::string& spelling);
// the other way: the exe's (the PDB's, IDA's) name with each std:: as src's msvc9::
std::string msvc9Names(std::string name);
// clang's name of a record without one: an anonymous member's type, or a named member's unnamed type
bool unnamedRecord(const std::string& name);
// gen/Prototypes.hpp holds stand-ins (forwarders and address overlays for functions and data src has no home for
// yet), derived from IDA's own names and decompiled prototypes: not src's declarations
bool placeholder(const SrcModel::Ref& ref);

// The ref whose declaration types (and names) an address: the registered body, else a forwarder; an
// FNVD_ORIGINAL_FUNCTION constant (the address as a value) is none. None for a fold (several src functions name the
// address) without a registered body. An address src
// registers as a C++ constructor or destructor (FNVD_REPLACES_CONSTRUCTOR/_DESTRUCTOR) has that registration as
// structor, its body; ref is then the address's other declaration (its `X::Construct`/`X::Destruct` forwarder, which
// callers call), if any. A forwarder in a copy src marks `Same function as` the address names nothing (it calls the
// owner's code); a class template member's own forwarder names it for its instances only where no body and no
// explicit specialization does.
struct RefChoice
{
	const SrcModel::Ref* ref = nullptr;
	std::set<std::string> names;  // the src functions whose body or forwarder names the address
	std::vector<const SrcModel::Ref*> named;  // the bodies and forwarders behind names
	const SrcModel::Ref* structor = nullptr;  // the C++ constructor or destructor src registers there
};
RefChoice chooseRef(const SrcModel& model, uint64_t address);

// The offset of the secondary vtable that alone holds the virtual in src's layout of the record: nullopt when the
// primary vtable holds it, or no vtable does.
std::optional<int64_t> secondaryVtable(const SrcModel::Record& record, const std::string& mangled);
// the base subobject of the record at the offset (a direct base there, else one inside the base holding it); "" for
// none
std::string baseAt(const SrcModel& model, const SrcModel::Record& record, int64_t offset);
// the offset of the base subobject a member's this points to when the exe enters it: 0, or for a virtual that only a
// secondary vtable holds in src's layout, that vtable's offset in the class (nullopt where the class is not in the
// model)
std::optional<int64_t> entryOffset(const SrcModel& model, const SrcModel::Function& member);
// the class whose object a member's this is when the exe enters it: its own, or for a virtual that only a secondary
// vtable holds in src's layout, the base subobject there ("" for none)
std::string entryClass(const SrcModel& model, const SrcModel::Function& member);
// a constructor or destructor written as a static member (porting rules): Construct, Destruct, DeletingDestruct, or
// one of them with idakit declare's `_<ADDR8>` suffix (a second exe copy); "" for any other name
std::string structorName(const std::string& name);
// IDA's name for a src function (its qualified name), in the exe's spelling: std:: for src's msvc9::, no anonymous
// namespace, and `X::X` / `X::~X` for a Construct / Destruct. "" for a DeletingDestruct: whether the exe's is the
// scalar or the vector deleting destructor is not in src.
std::string idaFunctionName(const std::string& qualified);

// a header src includes (`.h`, `.hpp`, `.inl`)
inline bool isHeader(const std::string& file)
{
	return file.ends_with(".h") || file.ends_with(".hpp") || file.ends_with(".inl");
}

// src registers a body at the address outside a header (a .cpp's out-of-line definition, an explicit specialization's
// included): the exe's function there is no transparent function's copy for the checker (porting rules "Transparent
// functions"; checkbody.py `transparent`: no exact name of the address defined out of line in a .cpp), so a src call
// of a class template instance's member made from the template's inline body is no call of it
inline bool bodyOutsideHeaders(const SrcModel& model, uint64_t address)
{
	const std::vector<SrcModel::Ref>& refs = model.refsAt(address);
	return std::any_of(refs.begin(), refs.end(),
					   [](const SrcModel::Ref& r) { return r.kind == "body" && !isHeader(r.file); });
}

// the src header declaring the explicit specialization of the mangled function (its home, which a TU calling it
// includes), "" for none
inline std::string specializationHeader(const SrcModel& model, const std::string& mangled)
{
	auto found = model.specializations().find(mangled);
	if (found != model.specializations().end())
		for (const SrcModel::Specialization& s : found->second)
			if (isHeader(s.file) && s.file.starts_with("src/"))
				return s.file;
	return "";
}

// The exe's thiscall src declares as a free fastcall function whose second parameter is the unused edx,
// `R FNVD_FASTCALL f(T* apThis, [[maybe_unused]] void* apEdx, ...)`: where the database's convention (cc, as
// conventionName spells it) is thiscall, that parameter's index (1), which no exe argument fills; -1 for any other.
inline int edxParameter(const SrcModel::Function& f, const SrcModel::Type& type, const std::string& cc)
{
	bool free = f.cls.empty() || f.isStatic;
	return free && cc == "thiscall" && type.cc == "fastcall" && type.params.size() >= 2 && f.params.size() >= 2 &&
				   f.params[1] == "apEdx"
			   ? 1
			   : -1;
}

// A src name as C++ spells it in its TU: clang's `(anonymous namespace)::` is no qualifier C++ can write (unqualified
// lookup finds the namespace's names in its TU), nor is srcdb's ` @file.cpp` after a TU-local record's name.
std::string tuSpelling(std::string name);

// IDA's struct name for each src record (in the open database), and the reverse.
class IdaStructNames
{
public:
	explicit IdaStructNames(const SrcModel& model);

	// The name IDA's struct for the record has, or is created under. Candidates in order: IDA's spellings of MSVC's
	// name for an unnamed member type, of the instance without its default arguments (source's and IDA's usual
	// spelling), of the record's own name, then each with src's msvc9:: as std:: or as nothing. The first of the
	// first three is the name a missing struct is created under. An IDA struct under the record's own spelling is
	// taken (same size, or no template arguments to tell instances apart); under any other spelling only at src's
	// size. rejected: on the first lookup of the record, each struct passed over for its size.
	const std::string& of(const std::string& record, std::vector<std::string>* rejected = nullptr);
	// The name sync pushes the record's layout under (src's record, with or without a definition): of's candidates,
	// those without msvc9:: (the exe's spellings: IDA's std:: classes, the CRT's names) before those with it, the
	// first that names an IDA struct at any size, else the first candidate. Another src record's first candidate
	// (the name sync creates that record's struct under) is never one: which struct a record takes does not depend
	// on the structs an earlier sync created (`X<unsigned int>`'s `X<unsigned long>` spelling is src's
	// X<unsigned long>'s).
	const std::string& pushName(const std::string& record);

	// The src record whose IDA struct is the named one, nullptr where none (every record looked up once).
	const SrcModel::Record* recordOf(const std::string& idaName);
	// A TU-local record (srcdb's `Name @file.cpp`) with no IDA name: its name without the file and the anonymous
	// namespace is another src record's too. Any other TU-local record is the exe's class of that name (RTTI's
	// `anonymous namespace'::X, IDA's X), its struct's candidates that name's spellings.
	bool unnamedLocal(const SrcModel::Record& record) const;

private:
	struct Candidate
	{
		std::string spelling;
		bool own;  // a spelling of the record's own name
	};
	// of's candidates, in order; the first is the name a missing struct is created under
	std::vector<Candidate> candidates(const std::string& record) const;

	const SrcModel& m_model;
	std::unordered_map<std::string, std::string> m_names;
	std::unordered_map<std::string, std::string> m_pushNames;
	// unnamed records (a named member's unnamed struct or union) -> MSVC's name for them
	std::unordered_map<std::string, std::string> m_unnamedNames;
	// TU-local records with an IDA name (unnamedLocal) -> that name
	std::unordered_map<std::string, std::string> m_localNames;
	std::unordered_map<std::string, const SrcModel::Record*> m_records;  // recordOf, filled on first use
	bool m_reverseFilled = false;
	// each src record's first candidate -> the record (the first in name order), filled on first use
	std::unordered_map<std::string, std::string> m_creationNames;
};

}  // namespace idakit
