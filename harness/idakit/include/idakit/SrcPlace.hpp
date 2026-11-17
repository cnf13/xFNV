// Where src code goes, for the commands that write src (idakit declare, idakit merge): the .cpp a function's
// out-of-line definition joins, and the home of an explicit specialization's declaration (porting.md "Bodies"), with
// the include graph both are decided on.
#pragma once

#include "idakit/SrcModel.hpp"

#include <map>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace idakit
{

// src's #include graph: each file's includes as read (once), plus the includes a run adds.
class IncludeGraph
{
public:
	// Whether file includes target, directly or through the src headers it includes (paths under src/).
	bool reaches(const std::string& file, const std::string& target);
	// An include of header (a path under src/) this run adds to file.
	void add(const std::string& file, const std::string& header) { m_added[file].push_back(header); }

private:
	std::map<std::string, std::vector<std::string>> m_includes, m_added;
};

// The PDB's source file of each function (env/cache/pdb-function-sources.tsv: qualified name, path under src/).
class PdbSources
{
public:
	explicit PdbSources(const std::string& path);
	// the src .cpp the PDB names for the function, when src has it; "" otherwise
	std::string source(const std::string& name) const;

private:
	std::unordered_map<std::string, std::string> m_sources;  // by ExeVtables::classKey
};

// The .cpp a function's out-of-line definition joins when it leaves the header it is written in: the function's PDB
// source, else its class's .cpp, else the header's same-stem .cpp (which may not exist yet).
std::string definitionSource(const SrcModel& model, const PdbSources& sources, const SrcModel::Function& f,
							 const std::string& header);

// The header a file that needs file's declarations includes: for an `.inl` (no include guard; its header includes it)
// its same-stem `.h` when there is one, else file itself.
std::string includedHeader(const std::string& file);

// The namespaces open at a line of a file's lines (`namespace a {` ... `}`), outermost first: each one's name and
// the line it opens on.
std::vector<std::pair<std::string, int64_t>> namespacesAt(const std::vector<std::string>& lines, int64_t line);

// Where an explicit specialization is declared and defined: its declaration goes into header after line `after`
// (header including the src headers `includes` names, under src/, for the types it needs complete), its definition
// (when it is not already in a .cpp) into source.
struct SpecializationHome
{
	std::string header;
	int64_t after = 0;
	std::string source;
	std::vector<std::string> includes;
};

// The homes of explicit specializations (porting.md "Bodies"): the template's header right after the template's
// definition (the outermost class's, for a template nested in a class), its argument classes forward declared and
// the ones it holds by value defined (their headers included); or, when an argument names a type the template's
// header cannot see there (a nested type, an enum, a type whose header includes the template's), the header defining
// that type (its outermost class), right after that definition. Arguments of that kind from headers none of which
// includes the others have no home. A home inside a namespace that does not enclose the template moves past it.
class SpecializationHomes
{
public:
	using Result = std::variant<SpecializationHome, std::string>;  // the home, or why there is none

	SpecializationHomes(const SrcModel& model, IncludeGraph& includes);

	// The home of src's explicit specialization (a function the model has as a class template instance's member, a
	// member of a class nested in one, or a function template's specialization).
	Result specializationHome(const std::string& mangled);
	// The home of a specialization of a member of owner (a class template instance, or a class nested in one) whose
	// signature names the type keys.
	Result specializationHome(const SrcModel::Record& owner, const std::vector<std::string>& keys);

	// The line of a declaration's or definition's end in a file: from line, its first `;` outside braces, or the
	// brace closing its first `{`.
	int64_t blockEnd(const std::string& file, int64_t line);

	// A type name a specialization's signature or template arguments spell, without its cv, pointer and reference
	// parts; complete: held by value through every enclosing template argument from the instance specialized (the
	// instance needs it defined)
	struct TypeName
	{
		std::string name;
		bool complete = false;
	};

private:
	struct Place
	{
		std::string file;
		int64_t after = 0;
	};
	// at: the template's place; scope: the namespace the template is a member of
	Result home(const Place& at, const std::string& scope, const std::string& ownerName,
				const std::vector<TypeName>& names);
	// the outermost record whose name is a prefix of name's scopes (name itself included); nullptr for none
	const SrcModel::Record* outermost(const std::string& name) const;
	// the namespaces of a qualified name: its leading components that are no records
	std::string namespaceOf(const std::string& name) const;
	// what the type name requires of a home: pinned (a place it must follow) or required (a header it needs)
	void classify(const TypeName& name, const std::string& ownerName, std::vector<Place>& pinned,
				  std::vector<Place>& required, int depth);
	// owner's template arguments (templateArguments' names, every one taken as held by value) as its layout holds
	// them: a name no field or base holds by value at any depth (NiPointer<T> holds a T*) is not complete; a record
	// with no layout the model has keeps them all
	void heldArguments(const SrcModel::Record& owner, std::vector<TypeName>& names) const;
	// the record holds the named type by value: as a base or a field (through cv, arrays and typedefs), or inside a
	// base or field record it holds so
	bool holds(const SrcModel::Record& record, const std::string& name, int depth) const;
	const std::vector<std::string>& lines(const std::string& file);

	const SrcModel& m_model;
	IncludeGraph& m_includes;
	std::map<std::string, Place> m_patterns;  // a class template's name -> its definition
	std::map<std::string, std::vector<std::string>> m_lines;
};

}  // namespace idakit
