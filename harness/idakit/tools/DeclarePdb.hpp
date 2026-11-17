// The PDB's types as src's, shared by declare --pdb-signatures (DeclareSignatures.cpp) and --pdb-fields
// (DeclareFields.cpp): a PDB type key in src's names, keys compared as srcdb compares them, their kinds and sizes,
// and the exe addresses src's functions declare. And the checks a table of function slots passes before --retype
// writes it (DeclareSlotRows.cpp), shared by --pdb-signatures and idakit infer (InferCommand.cpp).
#pragma once

#include "idakit/SrcModel.hpp"
#include "idakit/SrcTypes.hpp"

#include <functional>
#include <map>
#include <optional>
#include <ostream>
#include <regex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace exefacts
{
class Facts;
}

namespace idakit
{

// srcdb's compare of names (Index.cpp's PdbTyped::named): the PDB's std:: as msvc9::, `T const` as `const T`, no
// elaborated `enum`/`struct`/`class`/`union`, `long` as `int`, `__int64` as `long long`, no blanks
std::string comparedName(const std::string& name);

// A record or enum of the Xbox 360 build's platform in a PDB type key, which the PC exe has in another form (src's):
// group 2 its name
const std::regex& platformPattern();

// tabs and newlines out of a table cell
std::string cell(std::string text);
// a TSV line's cells (a trailing tab's empty one included)
std::vector<std::string> cells(const std::string& line);
// applied.tsv's text escaping: \\ \t \n
std::string escaped(const std::string& text);
std::string unescaped(const std::string& text);
// the file's bytes; Error where it cannot be read
std::string readFile(const std::string& path);
bool identifierChar(char c);
std::string trim(const std::string& s);
// A `// PDB: ...` (or any `//`) comment as the lines to insert above a declaration at indent: past 120 columns it
// goes on in `//` lines, from after its `; the exe ` (srcdb reads the form and the slot from the first line)
std::string commentBlock(const std::string& comment, const std::string& indent);

// A file's text with its line starts (0-based offsets of 1-based lines)
struct Text
{
	std::string text;
	std::vector<size_t> starts;

	explicit Text(std::string t);
	size_t lineCount() const { return starts.size(); }
	size_t at(int64_t line) const { return line >= 1 && size_t(line) <= starts.size() ? starts[line - 1] : text.size(); }
	int64_t lineOf(size_t offset) const;
	std::string line(int64_t number) const;
	std::vector<std::string> lines() const;
};

// One edit of a file's text: [offset, offset + length) replaced by text, for a table row (comment: a comment line
// inserted above a declaration, ordered after a change at the same offset)
struct Change
{
	size_t offset = 0, length = 0;
	std::string text, row;
	bool comment = false;
};

// The src files --retype edits: their texts as read, and the changes to make
class SrcChanges
{
public:
	Text& text(const std::string& file);
	void add(const std::string& file, Change change) { m_changes[file].push_back(std::move(change)); }
	// the changes, written into the files where apply; applied.tsv's lines (file, line, old, new, row, the row's
	// family from families); perFile: each file's rows
	void write(bool apply, std::ostream& applied, const std::map<std::string, std::string>& families,
			   std::map<std::string, std::vector<std::string>>& perFile);

private:
	std::map<std::string, Text> m_texts;
	std::map<std::string, std::vector<Change>> m_changes;
};

// src's include graph, read from the files' `#include "X"` lines (X from src/, else beside the including file)
class Includes
{
public:
	// the repo-relative files the file includes, directly or not
	std::set<std::string> closure(const std::string& file);

private:
	const std::vector<std::string>& direct(const std::string& file);

	std::map<std::string, std::vector<std::string>> m_direct;
};

// What a declaration of the key's type needs declared (complete: held by value): the headers (relative to src/) of
// what it holds by value, and the records it only points to that a forward declaration can declare (namespace scope,
// no template instance)
void typeNeeds(const SrcModel& model, const SrcTypes& types, const std::string& key, bool complete,
			   std::set<std::string>& headers, std::set<const SrcModel::Record*>& forwards);

// What file (its text t) needs to name the key's type that it does not see yet (complete: by value): an include of
// each header typeNeeds names (none the file includes already, none that includes the file back: Error), a forward
// declaration of each record it only points to, as one insertion for row after the file's first run of #include
// lines (else after its #pragma once, else at its top); none where it needs nothing. inserted: the lines this run
// added, per file, none twice.
std::optional<Change> declarationNeeds(const SrcModel& model, const SrcTypes& types, Includes& includes,
									   const std::string& file, const Text& t, const std::string& key, bool complete,
									   std::map<std::string, std::set<std::string>>& inserted, const std::string& row);

class PdbTypes
{
public:
	PdbTypes(const SrcModel& model, SrcTypes& types);

	// The PDB's type key in src's names: a builtin integer fixed width, a record by src's name (compared as srcdb
	// compares, else ida_type_aliases.tsv's spelling), an enum src has (SrcTypes::enumOf), a function's parts;
	// nullopt with the missing name where src has none
	std::optional<std::string> toSrc(const std::string& pdbKey, std::string& missing) const;
	// the key as srcdb compares it: typedefs resolved, integers by size and sign, cv kept
	std::string normal(const std::string& key, int depth = 0) const;
	// the key as src spells it (fixed widths), as a declaration of name where one is given
	std::string spelled(const std::string& key, const std::string& name = "") const;
	// the key with typedefs and its own cv resolved at the top
	std::string resolved(const std::string& key) const;
	// ptr, ref (a reference), int, bool, char, wchar_t, float, void, enum, record, fn, array
	std::string kindOf(const std::string& key) const;
	// the key's size in bytes (a pointer or reference 4, an array its element's times its bound); nullopt unknown
	std::optional<int64_t> size(const std::string& key) const;
	// its alignment in bytes (a pointer or reference 4, an array its element's); nullopt unknown
	std::optional<int64_t> align(const std::string& key) const;
	// the record is base or derives from it
	bool derives(const std::string& record, const std::string& base) const;

protected:
	const SrcModel& m_model;
	SrcTypes& m_types;
	std::unordered_map<std::string, std::string> m_records;  // comparedName -> src record
	// the exe address each src function declares: its registered body, constructor or destructor, an `address`
	// tie, a fold comment, or a forwarder of a function whose body is at no other address (SrcModel::bodyAddresses)
	std::unordered_map<std::string, uint64_t> m_address;              // mangled -> the exe function
	std::unordered_map<uint64_t, std::vector<std::string>> m_declaring;  // address -> the src functions declaring it
};

// A function slot's row of a --retype table (DeclareRetype.cpp reads its columns by name)
struct SlotRow
{
	// slot: `ret`, `this` or a parameter's index from 0; src: src's type as spelled; proposed: the table's (the PDB's,
	// or infer's) as spelled; key: the proposed type's key, which --retype writes
	std::string id, address, function, mangled, slot, src, proposed, verdict, reason, comment, key;
	std::string family;  // a virtual's: `<root>#<slot>/<rows>`, the rows the family's change takes
};

// The checks of a table's `apply` rows before --retype writes them: a virtual's slot changes in its whole family
// (a method slot of a record's vtable joined with the same slot of each base's vtable at the base's offset) or not at
// all, no change makes a function's parameters another overload's of its name, and none narrows a width the exe
// keeps (narrowing). A row that fails becomes `open` with the reason.
class SlotRowChecks : public PdbTypes
{
public:
	SlotRowChecks(const SrcModel& model, SrcTypes& types, exefacts::Facts& facts);

	// Whether a family member without a row takes the family's type (its own type `own` being another): "" where it
	// does (the check adds its row), else why not
	using Takes = std::function<std::string(const SrcModel::Function& member, const std::string& slot,
											const std::string& own)>;
	// Each (family, slot) with apply rows: they agree on one key; every other row of it is `spelling`, or `keep` with
	// its own type the key; every member without a row has that type or takes it (`takes`, and no narrowing of the
	// member's: a row added). Else every apply row of it is open. Each row of the change gets the family's tag.
	void judgeFamilies(std::vector<SlotRow>& rows, const Takes& takes) const;
	// an apply row is open where its function's parameters, so changed, would be another overload's of its name
	void judgeOverloads(std::vector<SlotRow>& rows) const;
	// Whether key narrows src's integer type of the slot (declarations.md "Return widths", "Parameter widths"): a
	// narrower integer or bool, or a bool for a byte
	bool narrows(const std::string& srcKey, const std::string& key) const;
	// The exe's fact against narrowing function's slot (`ret` or a parameter's index) to key (narrows): a return
	// path that writes more bytes of eax than key, or (for a bool) a byte that may be other than 0 or 1; a parameter
	// the function reads wider than key, passes on whole to a callee (a call's or tail jump's argument), or that a
	// direct caller passes its own argument's whole dword in. "" where none is; unshown says why the exe's code does
	// not show it (no address, a register convention, an address taken, a return value not traced) where it does not
	std::string narrowing(const SrcModel::Function& function, uint64_t ea, const std::string& slot,
						  const std::string& key, std::string& unshown) const;
	// The offset past [entry esp + 4] where the exe passes the function's parameter k: the stack parameters before
	// it, each a multiple of 4 bytes, after the hidden result pointer of an indirect return (a usercall's: its stack
	// parameters before it); nullopt for a register convention or a usercall's register parameter
	std::optional<int64_t> stackOffset(const SrcModel::Function& function, size_t k) const;
	// a usercall's parameter locations by index (`this` -1; srcdb's ref_location: a register or `stack`), null for a
	// function src declares no usercall of
	const std::map<int64_t, std::string>* userLocations(const std::string& mangled) const;
	// an apply row whose key narrows src's type is open where the exe keeps src's width or does not show it
	// (narrowing), for a table whose tool has no `// PDB:` comment to keep it with (idakit infer's)
	void judgeWidths(std::vector<SlotRow>& rows) const;

	// The members (mangled) of the virtual family the function belongs to, itself included; empty for a function no
	// method slot holds
	std::vector<std::string> familyOf(const std::string& mangled) const
	{
		auto found = m_family.find(mangled);
		return found == m_family.end() ? std::vector<std::string>() : m_members.at(found->second);
	}
protected:
	exefacts::Facts& m_facts;

private:
	std::unordered_map<std::string, std::string> m_family;  // mangled -> its virtual family's root
	std::map<std::string, std::vector<std::string>> m_members;  // root -> the family's members
	std::unordered_map<std::string, std::map<int64_t, std::string>> m_locations;  // mangled -> userLocations
};

}  // namespace idakit
