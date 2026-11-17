// Text edits of src/ files for the commands that write src (idakit declare, idakit merge): a function's lines in a
// file, edits collected per file and written once, an include added where a file keeps its includes, the fixed
// width scalar names (RULES "Types from the PDB") of a new declaration's text, and a declaration's texts with its
// `// Not reconstructed yet.` forwarder.
#pragma once

#include "idakit/SrcModel.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace idakit
{

// Lines first to last of a file, 1-based and inclusive.
struct LineRange
{
	int64_t first, last;
};

// The lines of definition d in a file's lines (d.file's): the model's line is the name's, so the range starts above
// it at the declaration's first line (its return type, `template <>`), up to a blank line, a comment or the end of
// what precedes it, and runs to d.endLine. Throws Error ("model and file disagree") for lines the file does not have.
LineRange definitionSpan(const std::vector<std::string>& lines, const SrcModel::Definition& d);

// definitionSpan's lines when they call ea (a hex literal of the address, with or without leading zeros), else
// nullopt.
std::optional<LineRange> definitionLines(const std::vector<std::string>& lines, const SrcModel::Definition& d,
										 uint64_t ea);

// The emitter's guarded block around line `line` (1-based) of a file's lines: from its opener
// `#if FNVD_HYBRID  // emitted` to its closer `#endif  // emitted` (a body with unresolved spots, hybrid-only in src,
// the forwarder it replaces under `#else`), else nullopt. Only those two marker lines count, so the body's own
// #if/#endif inside the block does not end it.
struct EmittedBlock
{
	LineRange whole;					 // opener through closer
	std::optional<LineRange> forwarder;  // the lines of the block's own #else branch; nullopt without one
};
std::optional<EmittedBlock> emittedBlock(const std::vector<std::string>& lines, int64_t line);

// The emitter's guard lines (idakit merge writes them; Reconstruction.hpp defines FNVD_HYBRID as 1 or 0): a marked
// body's block (the hybrid's definition, then under #else the standalone build's forwarder) and the block of what a
// file's marked bodies alone need, each closed by kEmittedEnd.
extern const std::string kEmittedBodyIf;
extern const std::string kEmittedNeedsIf;
extern const std::string kEmittedEnd;

// The first line of the declaration whose name is on line `line`: up from it while the line above continues it (not
// blank, a comment or a directive, and not ending a statement, a block or an access label).
int64_t declarationFirst(const std::vector<std::string>& lines, int64_t line);

// The first line of the comment lines directly above line `first` (first itself when there are none).
int64_t commentTop(const std::vector<std::string>& lines, int64_t first);

// An inline definition's text split at the `{ ... }` after the parameter list of `name`: the declaration alone
// (that block made `;`, an `inline` before it dropped, as the definition moves to a .cpp) and the block's
// statements, one per line without their indentation. nullopt when the text has no such shape.
struct InlineDefinition
{
	std::string declaration;
	std::vector<std::string> statements;
};
std::optional<InlineDefinition> inlineDefinition(const std::string& text, const std::string& name);

// The text with the `inline` keyword taken off each definition head in it (a line starting `inline`, after an
// optional `template <>` and attributes such as `[[noreturn]]`): the definition goes to a .cpp.
std::string withoutInline(const std::string& text);

// An emitted file-local global: its `static FNVD_GLOBAL(type, name, address);` line's name and type (an alias line
// `using name_t = T;` before it: T).
struct EmittedGlobal
{
	std::string name, type;
};
// The global a `static FNVD_GLOBAL(...)` line declares (with the alias line before it, if any); nullopt for any other
// line.
std::optional<EmittedGlobal> emittedGlobal(const std::string& line, const std::string& alias = "");

// One static FNVD_GLOBAL per name in a file: of a body's globals (each entry its alias line, if any, then its
// FNVD_GLOBAL line), those the file's lines declare as another type are the file's object, which the body reads as
// its own type. Removes those entries from globals and returns them.
std::vector<EmittedGlobal> takeRedeclaredGlobals(std::vector<std::vector<std::string>>& globals,
												 const std::vector<std::string>& lines);
// The text with each use of the global read as its type: `gen::as<T>(name)` (GenSupport.hpp, which a body with an
// emitted global includes).
std::string readAs(const std::string& text, const EmittedGlobal& global);

// Edits of files, each file read once (when first asked for) and written once by apply(). A text is one or more
// whole lines (a final newline is optional). Lines are 1-based and refer to the file as read, whatever the edits
// collected before. Throws Error when an edit overlaps one already collected for its file. insert and replace return
// the edit's id, which placed() takes after apply() to say where its text landed.
class FileEdits
{
public:
	// The file's lines as read (without their newlines); none for a file that does not exist yet.
	const std::vector<std::string>& lines(const std::string& path);
	// Inserts text before line `line` (lines(path).size() + 1: at the end). Inserts at one line keep their order.
	size_t insert(const std::string& path, int64_t line, const std::string& text);
	// Replaces the range's lines with text (empty: deletes them). Throws Error for lines the file does not have.
	size_t replace(const std::string& path, LineRange range, const std::string& text);
	// Whether the file, with the edits collected so far, has a line that reads `line` after leading blanks.
	// `unguarded`: only a line outside the emitted hybrid-only blocks (kEmittedBodyIf/kEmittedNeedsIf .. kEmittedEnd),
	// the ones code the standalone build compiles can rely on.
	bool has(const std::string& path, const std::string& line, bool unguarded = false);
	// Writes every edited file, applying its edits from the last line to the first; a file that does not exist is
	// created with the text of its inserts. The edits are dropped afterwards, their placements kept.
	void apply();
	// The lines an edit's text has in its file as apply() wrote it; nullopt before apply(), for a text of no lines,
	// and for an include line a new file's include hoisting dropped as a repeat.
	std::optional<LineRange> placed(size_t id) const;
	// The files with edits collected (before apply()).
	std::vector<std::string> paths() const;

private:
	struct Edit
	{
		int64_t first, last;  // an insert: first is its line, last = first - 1
		std::vector<std::string> text;
		size_t order;
	};
	struct File
	{
		bool exists = false, newlineAtEnd = true;
		std::vector<std::string> lines;
		std::vector<Edit> edits;
	};
	File& file(const std::string& path);
	size_t add(const std::string& path, Edit edit);

	std::map<std::string, File> m_files;
	std::map<size_t, LineRange> m_placed;  // by edit id, after apply()
	size_t m_order = 0;
};

// The include line of a header: `#include "header"`, or for a compiler header named in angle brackets
// (`<xmmintrin.h>`) `#include <xmmintrin.h>`.
std::string includeLine(const std::string& header);

// Adds the header's include line to the file unless it has it: after the last line of the file's first run of #include
// lines, else after its `#pragma once`, else at line 1. Returns the insert's id, nullopt when the file has it.
std::optional<size_t> ensureInclude(FileEdits& edits, const std::string& path, const std::string& header);

// The text with its scalar type names made fixed width (RULES): unsigned __int64 -> uint64_t, int/long -> int32_t,
// DWORD -> uint32_t, BYTE -> uint8_t and the rest, whole words.
std::string fixedWidth(std::string text);

// --- declarations and their forwarders ----------------------------------------------------------------------------

class SrcTypes;

// A function a writer leaves alone, and why (declare.tsv's or merge.tsv's reason).
struct Skip
{
	std::string reason;
};

struct Param
{
	std::string key, name;
};

// A line of text inserted before a line of a src file.
struct LineInsert
{
	std::string file;
	int64_t line = 0;
	std::string text;
};

// What one address gets: a new declaration with its definition (declared), the definition of src's declaration
// (defined), the explicit specialization of a class template instance's member (specialized), or src's unregistered
// (KEEP) body tied to it by an FNVD_ADDRESS assertion after its definition (addressed).
struct Decl
{
	uint64_t ea = 0;
	std::string outcome, kind;
	const SrcModel::Record* owner = nullptr;  // a member's class
	std::string scope;                        // a free function's namespace ("": global)
	std::string member;                       // the declared name
	std::string marker;                       // the name's marker comment, "" for a PDB name
	std::string ret = "builtin:void";
	std::vector<Param> params;  // `this` aside; a structor's apMemory / apThis are its first
	std::string cc;             // thiscall, cdecl, stdcall, fastcall, or usercall/userpurge (IDA's register prototype)
	std::string selfReg;        // a usercall member: the register `this` travels in
	// a usercall: its register parameters by name -> the register (eax, ecx, edx, ebx, esi, edi); the rest on the stack
	std::map<std::string, std::string> registers;
	bool isStatic = false, isConst = false;
	bool exact = false;  // src's declaration: its types are spelled as they are (fixed width only for new text)
	bool indirect = false;  // src's declaration returns a record by value (the exe through a hidden pointer)
	// the function never returns: `[[noreturn]]` on its texts, and `__builtin_unreachable()` after the forwarder's
	// call (src's declaration's, or IDA's for a new one)
	bool noreturn = false;
	std::string templateDeclaration;  // a structor declared in the template for this instance's specialization
	std::string self;	 // the call's first argument before the parameters: this, a base's cast of it, or ""
	std::string header, source;  // header "" when src declares it already
	// a specialization: the header line its declaration follows (its home's); registered: the source line of the
	// instance's registration it follows (0: the file's end)
	int64_t after = 0;
	// a specialization: the headers its home needs for it (SpecializationHome); registered: the headers its .cpp
	// needs for the instance's member (its home's, the template's definition)
	std::vector<std::string> homeIncludes;
	std::string mangled;         // src's declaration's, when it has one
	std::string pdbName;         // the name --sources is looked up under
	// addressed: what ties src's body to the address; registered: its form and place (the declare.tsv reason column)
	std::string proof;
	// registered (a class template instance's member from the template's body): the FNVD_REPLACES macro's suffix
	// ("" for FNVD_REPLACES itself; VIRTUAL, VIRTUAL_VIA, MEMBER, CONSTRUCT, DESTRUCT, DELETING_DESTRUCT)
	std::string form;
	// a virtual several classes' src functions name (a fold, porting rules "Folds"), defined for its owner: the
	// `// Same function as` comment each other member's declaration gets, right above it
	std::vector<LineInsert> foldCopies;
	// a class template instance's member src declares as a fold's copy (`declare --folds`, porting rules "Folds"): the
	// porting rules' `Same function as Owner::f (0xADDR), registered there.` text, written above its specialization's
	// declaration and definition; "" for none
	std::string fold;
};

// `0x405A00`, as src writes exe addresses
std::string address(uint64_t ea);

// C++'s keywords
const std::set<std::string>& keywords();

// A C++ identifier: no keyword, and no name reserved to the implementation (`_C`, `__x`: macros may hold them).
bool identifier(const std::string& s);

// srcdb's names hold the i686 build's 64-bit builtin (`BSTask<long long>`): src spells it int64_t, `long` on x64
std::string fixed64(const std::string& text);

// Each line of the text after indent, a line longer than 120 columns (a tab 4) broken after the commas of its
// argument lists (inside parentheses, outside template arguments), its continuations one unit deeper.
std::string wrapped(const std::string& text, const std::string& indent, const std::string& unit);

// The text's own indentation unit: a tab when a line of it starts with one, else four blanks.
std::string indentUnit(const std::vector<std::string>& lines, int64_t first, int64_t last);

// d as src's declaration f, as it is (exact): its class (src's record), name and scope, its type's result, constness
// and parameters (the declaration's names, `aN` for one no identifier names), static or not, the model's ABI
// convention and whether it returns indirectly, and self (`this`, a base's cast of it) as a member's receiver. The
// address, outcome and place stay the caller's. Throws Skip without an ABI row or type for f.
void srcDeclaration(Decl& d, const SrcModel& model, const SrcModel::Function& f, const std::string& self);

// A Decl's texts in src's spelling: a type key as C++ (src's own types, `exact`, as they are; a new declaration's
// builtins fixed width, RULES), its signature and its `// Not reconstructed yet.` forwarder (porting rules "Code not
// reconstructed yet"). Each throws Skip on a type src cannot spell.
class Forwarders
{
public:
	explicit Forwarders(const SrcTypes& types) : m_types(types) {}

	// builtins as RULES' fixed-width names (a typedef key spelled as that name), at every level
	std::string fixedKey(const std::string& key) const;
	std::string spelled(const Decl& d, const std::string& key, const std::string& name = "") const;
	// The definition head (`R X::f(params) const`, definition) or the declaration in its class or header (`static`
	// where it is), each type key read through keyAt first (a specialization's home spells some through their
	// typedefs' targets). The owner and scope as the TU spells them (tuSpelling); a result that is a pointer or
	// reference to a function or an array trails the parameters (`auto X::f(params) -> char (&)[512]`).
	std::string signature(const Decl& d, bool definition,
						  const std::function<std::string(const std::string&)>& keyAt = {}) const;
	// the forwarder's statements: d.ea's fnvd::original call (a usercall's register arguments in UserRegs) of d.self
	// and the parameters, returned as the declaration returns (a record by value built through the hidden pointer)
	std::vector<std::string> statements(const Decl& d) const;
	// the `template <>` lines an explicit specialization of d takes (at least one)
	std::string templateHeads(const Decl& d) const;
	// The out-of-line forwarder, indented by unit: heads (templateHeads, when given), the definition head (its keys
	// through keyAt), `{`, the `// Not reconstructed yet.` line (unless comment is false: the lines above say it),
	// the statements, `}`; no final newline.
	std::string definition(const Decl& d, bool heads, const std::string& unit, bool comment = true,
						   const std::function<std::string(const std::string&)>& keyAt = {}) const;

private:
	const SrcTypes& m_types;
};

// A type's text (SrcTypes::spell's) as src writes it: the i686 build's 64-bit builtins of srcdb's names fixed width
// (`long long` as int64_t) and no blank before a `*` or `&` (clang's `BSSimpleList<TESHair *>` as
// `BSSimpleList<TESHair*>`).
std::string srcSpelling(const std::string& text);

}  // namespace idakit
