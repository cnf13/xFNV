// idakit merge: the writer of emitted bodies into src/. Every printed emit.tsv row (form src or marked: a member
// definition emit wrote, DIR/src/<ADDR8>.cpp, as the printer wrote it) goes into src:
// - its address's forwarder is replaced: an out-of-line forwarder in a .cpp in its place; a header's forwarder never
//   in the header: an out-of-class one leaves the header (the header keeps the declaration) and an in-class one
//   becomes its declaration alone, the definition going to the end of the .cpp the placement rule picks (an explicit
//   specialization's home's .cpp, else the function's PDB source, else its class's .cpp, else the header's same-stem
//   .cpp: SrcPlace);
// - a marked row (emit.tsv's form; a src row's `/* review: ... */` notes do not make it one) is written for the hybrid
//   build only, the forwarder kept for the standalone build, in a guarded block (kEmittedBodyIf ... #else forwarder
//   ... kEmittedEnd); what only it needs (includes new to the file, its file-local globals) goes in one guarded block
//   per .cpp after the file's includes (a header never gets one); a file declares each global name once: a row's
//   global the file (or a row before it) declares as another type is that declaration, read in the row's text as
//   its own type (takeRedeclaredGlobals, readAs);
// - an address src already registers is re-emitted only when its body is such a guarded block (replaced as a whole);
//   a plain src body is never touched, unless --exclude lists the address (below);
// - a row the run does not merge (form none, excluded, skipped) keeps no guarded block: one its src text still is
//   (an earlier emit's, which a header change can leave naming members or signatures src no longer has) becomes the
//   forwarder it replaced, as idakit declare writes one (Forwarders) with src's declaration as the model has it now,
//   outside any guard (StaleBlocks); so does an excluded address that is no row of the run (merge-gate.py's: a stale
//   block whose errors it attributed to the address). A block whose function src no longer declares, of an address
//   src defines elsewhere, is a second definition of it and is deleted;
// - an excluded address that is no row of the run and whose src body is a plain definition (clean emitted or written
//   by hand: the list is the caller's request to emit it again, pipeline.sh's reemit.list) gets the same forwarder in
//   place of the definition, the comment above it kept, and its registration goes (with a `using` alias line above
//   it only it needs); one that cannot be (registered more than once or at another address too, a C++ structor, a
//   header's inline body, a template's definition its instances share, a fold registered elsewhere) is refused.
// Writes DIR/merge.tsv (address, outcome, file, reason; outcome merged, guarded, skipped, excluded, forwarded, deleted,
// kept: a row not merged whose block could be neither forwarded nor deleted, the reason saying why, forwarded-body or
// refused), DIR/merged.txt (the merged and guarded addresses) and DIR/merge-files.txt (the files edited); src/ only
// with --apply, all edits at once, and then DIR/merge-spans.tsv (address, file, first, last, what: the lines each
// row's text took, a forwarder's included, for harness/tools/merge-gate.py to tell whose text a compile error is in).
// --exclude leaves rows out.
// Needs no database: the model (srcdb index of the tree emit ran on) says where each forwarder or body is. Run from
// the repository root (the model's paths are relative to it).

#include "idakit/Address.hpp"
#include "idakit/Command.hpp"
#include "idakit/Database.hpp"
#include "idakit/SrcEdit.hpp"
#include "idakit/SrcIda.hpp"
#include "idakit/SrcModel.hpp"
#include "idakit/SrcPlace.hpp"
#include "idakit/SrcTypes.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <unordered_map>
#include <variant>

namespace idakit
{

namespace
{

const std::string kNotReconstructed = "Not reconstructed yet.";
std::string trimmedLeft(const std::string& line)
{
	size_t start = line.find_first_not_of(" \t");
	return start == std::string::npos ? std::string() : line.substr(start);
}

bool endsWith(const std::string& text, const std::string& suffix)
{
	return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// A src path (src/X) as an include names it (X).
std::string includeName(const std::string& path)
{
	return path.substr(path.rfind("src/", 0) == 0 ? 4 : 0);
}

std::string joined(const std::vector<std::string>& lines, LineRange range)
{
	std::string text;
	for (int64_t number = range.first; number <= range.last; ++number)
		text += lines[number - 1] + "\n";
	return text;
}

// The parts of an emitted member definition (EmitPrinter's file()): the headers it includes, without the class's
// .cpp (the check file's stand-in for the file the definition joins), and GenSupport.hpp unless the body is marked
// (the raw spellings of unresolved items need it); its file-local globals, each one entry (a `using X_t = ...;` line
// with the FNVD_GLOBAL line after it); the definition from its notes through its FNVD_REPLACES line, without the
// note naming what it replaces (emit's, not the code's: `// replaces the forwarder at` or `// replaces the emitted
// body at`, a line that would change on every re-merge); that definition without its FNVD_REPLACES line, and the
// line alone; the definition's head (`R X::f(...)`). Whether the body is marked is emit.tsv's (its form column).
struct Split
{
	std::vector<std::string> includes;
	std::vector<std::vector<std::string>> globals;
	std::string body, definition, registration, head;
};

std::optional<Split> split(const std::string& text, bool marked)
{
	std::vector<std::string> lines;
	std::istringstream in(text);
	for (std::string line; std::getline(in, line);)
		lines.push_back(line);
	Split out;
	size_t k = 0;
	// `#include "X"` and `#include <X>` (a system header: an intrinsic's), the latter kept as `<X>` (includeLine)
	for (; k < lines.size() &&
		   (lines[k].empty() || lines[k].starts_with("#include \"") || lines[k].starts_with("#include <"));
		 ++k)
	{
		if (lines[k].empty())
			continue;
		std::string header = lines[k][9] == '<' ? lines[k].substr(9) : lines[k].substr(10, lines[k].rfind('"') - 10);
		if (!endsWith(header, ".cpp") && (header != "GenSupport.hpp" || marked))
			out.includes.push_back(header);
	}
	for (; k < lines.size(); ++k)
	{
		if (lines[k].empty())
			continue;
		if (lines[k].starts_with("using ") && k + 1 < lines.size() &&
			lines[k + 1].starts_with("static FNVD_GLOBAL("))
		{
			out.globals.push_back({lines[k], lines[k + 1]});
			++k;
		}
		else if (lines[k].starts_with("static FNVD_GLOBAL("))
			out.globals.push_back({lines[k]});
		else
			break;
	}
	size_t last = lines.size();
	while (last > k && lines[last - 1].empty())
		--last;
	if (k == last || !lines[last - 1].starts_with("FNVD_REPLACES"))
		return std::nullopt;
	bool headDone = false;
	for (; k < last; ++k)
	{
		if (lines[k].starts_with("// replaces the forwarder at ") ||
			lines[k].starts_with("// replaces the emitted body at "))
			continue;
		// the head: from the first line after the notes (a `template <>` line included) to the one opening the body
		if (!headDone && (!out.head.empty() || !lines[k].starts_with("//")))
		{
			headDone = endsWith(lines[k], " {");
			out.head += headDone ? lines[k].substr(0, lines[k].size() - 2) : lines[k] + "\n";
		}
		out.body += lines[k] + "\n";
		if (k + 1 < last)
			out.definition += lines[k] + "\n";
		else
			out.registration = lines[k] + "\n";
	}
	if (!headDone)
		return std::nullopt;
	return out;
}

// The marked body's guarded block: the hybrid's definition, else the forwarder.
std::string guarded(const std::string& body, const std::string& forwarder)
{
	return kEmittedBodyIf + "\n" + body + "#else\n" + forwarder + kEmittedEnd + "\n";
}

// The body without those of its leading note lines (the comment lines before its head) that the comment kept right
// above where it goes already says (kept: those comment lines, as the file keeps them): a forwarder an earlier body
// was made into keeps that body's notes above it, which the body emitted again carries too.
std::string withoutKeptNotes(const std::string& body, const std::string& kept)
{
	auto trimmed = [](const std::string& line) {
		std::string text = trimmedLeft(line);
		return text.substr(0, text.find_last_not_of(" \t") + 1);
	};
	std::set<std::string> said;
	std::istringstream above(kept);
	for (std::string line; std::getline(above, line);)
		said.insert(trimmed(line));
	std::istringstream in(body);
	std::string out;
	bool notes = true;
	for (std::string line; std::getline(in, line);)
	{
		notes = notes && trimmedLeft(line).starts_with("//");
		if (!notes || !said.count(trimmed(line)))
			out += line + "\n";
	}
	return out;
}

// the comment lines right above line `first`, as one text
std::string keptAbove(const std::vector<std::string>& lines, int64_t first)
{
	int64_t top = commentTop(lines, first);
	return top < first ? joined(lines, {top, first - 1}) : "";
}

// The comment lines right above line `first` with "Not reconstructed yet." taken out (a line left with nothing
// after its `//` dropped): the range from the topmost changed line to first - 1 and its new text; nullopt when no
// comment line there says it.
struct CommentFix
{
	int64_t first;
	std::string text;
};

std::optional<CommentFix> commentFix(const std::vector<std::string>& lines, int64_t first)
{
	int64_t top = first;
	while (top >= 2 && trimmedLeft(lines[top - 2]).starts_with("//"))
		--top;
	std::optional<CommentFix> fix;
	for (int64_t number = top; number < first; ++number)
	{
		const std::string& line = lines[number - 1];
		size_t at = line.find(kNotReconstructed);
		if (!fix)
		{
			if (at == std::string::npos)
				continue;
			fix = CommentFix{number, ""};
		}
		std::string changed = line;
		if (at != std::string::npos)
		{
			size_t length = kNotReconstructed.size();
			if (at + length < changed.size() && changed[at + length] == ' ')
				++length;
			changed.erase(at, length);
			std::string rest = trimmedLeft(changed).substr(2);
			if (rest.find_first_not_of(" \t") == std::string::npos)
				continue;
			// "//  text" after the words went from the middle of "// Not reconstructed yet. text"
			size_t end = changed.find_last_not_of(" \t");
			changed.erase(end + 1);
		}
		fix->text += changed + "\n";
	}
	return fix;
}

// Where a .cpp's globals go: after the last #include of its top (the includes, comments, blank lines and directives
// before its first declaration; an include in a conditional group: after that group's #endif; an emitted needs block,
// whose globals are no declaration of the file's own: after it, so that what an earlier block includes, GenSupport.hpp
// among it, comes before the new lines); line 1 for none.
int64_t afterIncludes(const std::vector<std::string>& lines)
{
	int64_t at = 1;
	int depth = 0;
	bool pending = false;
	bool needs = false;
	for (int64_t number = 1; number <= static_cast<int64_t>(lines.size()); ++number)
	{
		std::string line = trimmedLeft(lines[number - 1]);
		if (depth == 0 && lines[number - 1] == kEmittedNeedsIf)
		{
			needs = true;
			++depth;
		}
		else if (needs)
		{
			if (lines[number - 1] == kEmittedEnd)
			{
				needs = false;
				depth = 0;
				at = number + 1;
			}
		}
		else if (line.starts_with("#if"))
			++depth;
		else if (line.starts_with("#endif") && depth > 0 && --depth == 0 && pending)
		{
			at = number + 1;
			pending = false;
		}
		else if (line.starts_with("#include"))
		{
			if (depth == 0)
				at = number + 1;
			else
				pending = true;
		}
		else if (!line.empty() && !line.starts_with("//") && !line.starts_with("#"))
			break;
	}
	return at;
}

std::string readFile(const std::filesystem::path& path)
{
	std::ifstream in(path, std::ios::binary);
	if (!in)
		throw Error("idakit: cannot read " + path.string());
	std::ostringstream text;
	text << in.rdbuf();
	return text.str();
}

// What a file's marked bodies alone need, written as one guarded block after its includes: each line with the rows
// that need it.
struct Needs
{
	std::vector<std::pair<std::string, std::vector<uint64_t>>> includes, globals;
};

// A row's text in a file, for merge-spans.tsv once applied: the edit's id and what the text is; a needs block's line
// is the edit's line `offset` alone.
struct Span
{
	uint64_t ea;
	std::string file, what;
	size_t id;
	std::optional<int64_t> offset;
};

// The top-level, comma-separated arguments of the bracketed list opening at text[open] (`(`, `<`), blanks trimmed;
// nullopt where it does not close.
std::optional<std::vector<std::string>> listArguments(const std::string& text, size_t open)
{
	std::vector<std::string> out;
	std::string current;
	int depth = 0;
	auto push = [&] {
		size_t first = current.find_first_not_of(" \t\n"), last = current.find_last_not_of(" \t\n");
		out.push_back(first == std::string::npos ? "" : current.substr(first, last - first + 1));
		current.clear();
	};
	for (size_t k = open; k < text.size(); ++k)
	{
		char c = text[k];
		if (c == '(' || c == '<' || c == '[' || c == '{')
		{
			if (depth++ == 0)
				continue;
		}
		else if (c == ')' || c == '>' || c == ']' || c == '}')
		{
			if (--depth == 0)
			{
				push();
				return out;
			}
		}
		else if (c == ',' && depth == 1)
		{
			push();
			continue;
		}
		current += c;
	}
	return std::nullopt;
}

// The rows of a run that it does not merge (form none, excluded, skipped) keep no emitted block: one their src text
// is (an earlier emit's, which may name members or signatures src no longer has) is replaced by its forwarder, as
// idakit declare writes one, with the signature of src's declaration as the model has it now. A second signature only
// the block defined (a free function's overload beside the declared one) goes with it. An --exclude'd address whose
// src body is a plain definition (a clean emitted body or one written by hand: the list is the caller's request to
// emit it again) gets its forwarder the same way.
class StaleBlocks
{
public:
	StaleBlocks(const SrcModel& model, const SrcTypes& types, FileEdits& edits)
		: m_model(model), m_types(types), m_forwarders(types), m_edits(edits)
	{
	}

	// What became of ea's emitted block: its forwarder written in its place, or (no src declaration of its function
	// left, and another src definition of the address: a registration or the address's own forwarder) the block
	// deleted, as a second definition of the address.
	struct Done
	{
		std::string file;
		size_t id = 0;			// the edit's
		std::string elsewhere;	// for a deleted block: the other definition (file:line)
	};
	// The block's Done; Skip with the reason where src's text at ea is an emitted block that can be neither
	// forwarded nor deleted; nullopt where it is none.
	std::optional<Done> forward(uint64_t ea);
	// ea's plain src body (no emitted block) made its forwarder: the definition's lines (the comment above them kept)
	// replaced by the forwarder of src's declaration of it, the address's registration removed, and the `using` alias
	// line right above that registration where no later line of the file needs it. Throws Skip, before any edit, where
	// src's text at ea is no such body (none registered, registered more than once or also at another address, a C++
	// structor, a header's inline body, a template's definition its instances share).
	Done forwardBody(uint64_t ea);

private:
	struct Found
	{
		std::string file;
		EmittedBlock block;
		const SrcModel::Ref* ref = nullptr;  // the model's registration in it, when the model has one
		int64_t registration = 0;			 // its FNVD_REPLACES line
	};
	std::optional<Found> find(uint64_t ea);
	const std::map<uint64_t, std::vector<std::pair<std::string, int64_t>>>& registrations();
	const SrcModel::Function* declared(uint64_t ea, const Found& found, std::string& listed);
	std::optional<std::string> definedElsewhere(uint64_t ea, const Found& found);
	// d's forwarder text (from srcDeclaration, its convention settled) to stand at line `first` of file
	std::string text(Decl& d, const SrcModel::Function& f, const std::string& file, int64_t first);

	const SrcModel& m_model;
	const SrcTypes& m_types;
	Forwarders m_forwarders;
	FileEdits& m_edits;
	// src's FNVD_REPLACES lines in .cpp files by address (file, line), read once where the model has no ref at an
	// address (a registration that no longer compiles: an _OVERLOAD form naming the old signature)
	std::optional<std::map<uint64_t, std::vector<std::pair<std::string, int64_t>>>> m_registrations;
	// the model's definitions by file: line, mangled name
	std::optional<std::map<std::string, std::vector<std::pair<int64_t, std::string>>>> m_definitions;
};

// The address a registration line names (`FNVD_REPLACES...(0xADDR, ...`), else 0.
uint64_t registrationAddress(const std::string& line)
{
	size_t open = line.find('(');
	if (!line.starts_with("FNVD_REPLACES") || open == std::string::npos)
		return 0;
	size_t comma = line.find(',', open);
	try
	{
		return std::stoull(line.substr(open + 1, comma - open - 1), nullptr, 16);
	}
	catch (const std::exception&)
	{
		return 0;
	}
}

std::optional<StaleBlocks::Found> StaleBlocks::find(uint64_t ea)
{
	auto at = [&](const std::string& file, int64_t line) -> std::optional<EmittedBlock> {
		std::optional<EmittedBlock> block = emittedBlock(m_edits.lines(file), line);
		return block && block->forwarder ? block : std::nullopt;
	};
	for (const SrcModel::Ref& ref : m_model.refsAt(ea))
		if (ref.kind == "body")
			if (std::optional<EmittedBlock> block = at(ref.file, ref.line))
				return Found{ref.file, *block, &ref, ref.line};
	if (auto lines = registrations().find(ea); lines != registrations().end())
		for (const auto& [file, line] : lines->second)
			if (std::optional<EmittedBlock> block = at(file, line))
				return Found{file, *block, nullptr, line};
	return std::nullopt;
}

const std::map<uint64_t, std::vector<std::pair<std::string, int64_t>>>& StaleBlocks::registrations()
{
	if (!m_registrations)
	{
		m_registrations.emplace();
		for (const auto& entry : std::filesystem::recursive_directory_iterator("src"))
		{
			if (!entry.is_regular_file() || entry.path().extension() != ".cpp")
				continue;
			std::ifstream in(entry.path());
			int64_t number = 0;
			for (std::string line; std::getline(in, line);)
				if (++number; uint64_t address = registrationAddress(line))
					(*m_registrations)[address].emplace_back(entry.path().generic_string(), number);
		}
	}
	return *m_registrations;
}

// Another src definition of ea outside the block (file:line): a registration of a body at it (the model's, a C++
// structor's included, else a registration line the model has none at), or the address's own forwarder (chooseRef's:
// one in no function src registers at another address, which would be a call from that body).
std::optional<std::string> StaleBlocks::definedElsewhere(uint64_t ea, const Found& found)
{
	auto outside = [&](const std::string& file, int64_t line) {
		return file != found.file || line < found.block.whole.first || line > found.block.whole.last;
	};
	for (const SrcModel::Ref& r : m_model.refsAt(ea))
	{
		bool own = r.kind == "body" || r.kind == "constructor" || r.kind == "destructor";
		if (r.kind == "forwarder" && !r.target.empty())
		{
			const std::vector<uint64_t>& registered = m_model.bodyAddresses(r);
			own = std::all_of(registered.begin(), registered.end(), [&](uint64_t a) { return a == ea; });
		}
		if (own && outside(r.file, r.line))
			return r.file + ":" + std::to_string(r.line);
	}
	if (auto lines = registrations().find(ea); lines != registrations().end())
		for (const auto& [file, line] : lines->second)
			if (outside(file, line))
				return file + ":" + std::to_string(line);
	return std::nullopt;
}

// The declaration the block's forwarder takes: the model's function of the block's registration, where src declares
// it outside the block; else, of the functions of the names the block defines or registers, the one declared outside
// it without a definition elsewhere (where several are, the one of as many parameters as the block's forwarder, then
// of its parameter names, then const as it is or not). Null for none (`listed`: the names looked up); throws Skip for
// several.
const SrcModel::Function* StaleBlocks::declared(uint64_t ea, const Found& found, std::string& listed)
{
	auto inBlock = [&](const std::string& file, int64_t line) {
		return file == found.file && line >= found.block.whole.first && line <= found.block.whole.last;
	};
	const SrcModel::Function* registered =
		found.ref && !found.ref->function.empty() ? m_model.function(found.ref->function) : nullptr;
	if (registered && !inBlock(registered->file, registered->line))
		return registered;
	std::set<std::string> names;
	if (registered)
		names.insert(registered->name);
	if (found.ref && !found.ref->target.empty())
		names.insert(found.ref->target);
	if (!m_definitions)
	{
		m_definitions.emplace();
		for (const auto& [mangled, d] : m_model.definitions())
			(*m_definitions)[d.file].emplace_back(d.line, mangled);
	}
	if (auto defined = m_definitions->find(found.file); defined != m_definitions->end())
		for (const auto& [line, mangled] : defined->second)
			if (const SrcModel::Function* f = m_model.function(mangled); f && inBlock(found.file, line))
				names.insert(f->name);
	// the registration's own words: `FNVD_REPLACES(ADDR, X::f)`, `FNVD_REPLACES_VIRTUAL(ADDR, X, f)`, a class through
	// the block's `using` alias (`using X = C<T, N>;` above either form)
	const std::vector<std::string>& lines = m_edits.lines(found.file);
	auto unaliased = [&](std::string cls) {
		for (int64_t k = found.block.whole.first; k <= found.block.whole.last; ++k)
			if (lines[k - 1].starts_with("using " + cls + " = "))
				return lines[k - 1].substr(9 + cls.size(), lines[k - 1].rfind(';') - 9 - cls.size());
		return cls;
	};
	const std::string& registration = lines[found.registration - 1];
	if (std::optional<std::vector<std::string>> words = listArguments(registration, registration.find('(')))
	{
		std::string macro = registration.substr(0, registration.find('('));
		bool scoped = macro.find("VIRTUAL") != std::string::npos || macro == "FNVD_REPLACES_MEMBER";
		size_t member = macro.find("_VIA") != std::string::npos ? 3 : 2;
		if (scoped && words->size() > member)
			names.insert(unaliased((*words)[1]) + "::" + (*words)[member]);
		else if (!scoped && words->size() > 1)
		{
			const std::string& named = (*words)[1];
			size_t scope = named.rfind("::");
			names.insert(named);
			if (scope != std::string::npos)
				names.insert(unaliased(named.substr(0, scope)) + named.substr(scope));
		}
	}
	std::vector<const SrcModel::Function*> candidates;
	for (const std::string& name : names)
		for (const SrcModel::Function* f : m_model.functionsNamed(name))
		{
			const SrcModel::Definition* d = m_model.definition(f->mangled);
			if (!inBlock(f->file, f->line) && (!d || inBlock(d->file, d->line)) &&
				std::find(candidates.begin(), candidates.end(), f) == candidates.end())
				candidates.push_back(f);
		}
	// the block's forwarder's parameters, by name: the head's list (the last one before its `{`), and whether a
	// `const` follows it
	std::string head;
	for (int64_t k = found.block.forwarder->first; k <= found.block.forwarder->last; ++k)
	{
		if (trimmedLeft(lines[k - 1]).starts_with("{"))
			break;
		head += lines[k - 1] + "\n";
	}
	std::vector<std::string> params;
	bool isConst = false;
	int depth = 0;
	for (size_t k = head.size(); k-- > 0;)
		if (head[k] == ')')
		{
			size_t after = head.find_first_not_of(" \t\n", k + 1);
			if (depth++ == 0)
				isConst = after != std::string::npos && head.compare(after, 5, "const") == 0;
		}
		else if (head[k] == '(' && --depth == 0)
		{
			for (std::string p : listArguments(head, k).value_or(std::vector<std::string>{}))
			{
				while (p.ends_with(']') && p.find('[') != std::string::npos)
					p.erase(p.rfind('['));
				size_t end = p.find_last_not_of(" \t");
				size_t start = end == std::string::npos ? 0 : end + 1;
				while (start > 0 && (std::isalnum(static_cast<unsigned char>(p[start - 1])) || p[start - 1] == '_'))
					--start;
				if (!p.empty())
					params.push_back(p.substr(start, end + 1 - start));
			}
			break;
		}
	auto keep = [&](auto fits) {
		std::vector<const SrcModel::Function*> kept;
		std::copy_if(candidates.begin(), candidates.end(), std::back_inserter(kept), fits);
		if (!kept.empty())
			candidates = kept;
	};
	if (candidates.size() > 1)
		keep([&](const SrcModel::Function* f) { return f->params.size() == params.size(); });
	if (candidates.size() > 1)
		keep([&](const SrcModel::Function* f) { return f->params == params; });
	if (candidates.size() > 1)
		keep([&](const SrcModel::Function* f) {
			const SrcModel::Type* type = m_model.type(f->type);
			return type && type->isConst == isConst;
		});
	if (candidates.size() == 1)
		return candidates.front();
	// no declaration outside the block: the block's own definition is src's only declaration
	if (candidates.empty() && registered)
		return registered;
	for (const std::string& name : names)
		listed += (listed.empty() ? "" : ", ") + name;
	if (candidates.empty())
		return nullptr;
	throw Skip{std::to_string(candidates.size()) + " src declarations fit the emitted block of " + hexText(ea) + " (" +
			   listed + ")"};
}

// A usercall's registers by parameter index (`this` -1) from its registration: the model's (srcdb's ref_location),
// else the registration line's location list (`FNVD_REPLACES_USERCALL(ADDR, f, eax, stack)`, a member's `this`
// first). Throws Skip for none.
std::map<int64_t, std::string> userLocations(const SrcModel::Ref* ref, const std::string& registration,
											 const SrcModel::Function& f)
{
	std::map<int64_t, std::string> locations;
	if (ref)
		locations = ref->locations;
	if (locations.empty())
	{
		std::optional<std::vector<std::string>> words = listArguments(registration, registration.find('('));
		bool member = !f.cls.empty() && !f.isStatic;
		if (words && registration.starts_with("FNVD_REPLACES_USER"))
			for (size_t k = 2; k < words->size(); ++k)
				locations[static_cast<int64_t>(k) - 2 - (member ? 1 : 0)] = (*words)[k];
	}
	if (locations.empty())
		throw Skip{"a usercall forwarder whose registers neither the model nor the registration gives"};
	return locations;
}

// The forwarder's Decl of ea: src's declaration f as it is (srcDeclaration), self its receiver.
Decl forwarderDecl(const SrcModel& model, uint64_t ea, const SrcModel::Function& f, const std::string& self)
{
	if (!f.cls.empty() && !model.record(f.cls))
		throw Skip{"no src record of " + f.cls};
	const SrcModel::Type* type = model.type(f.type);
	if (type && type->variadic)
		throw Skip{"variadic"};
	Decl d;
	d.ea = ea;
	srcDeclaration(d, model, f, self);
	return d;
}

// d made a usercall (cc: usercall or userpurge) with the registers of locations; the parameters past them are on the
// stack.
void makeUsercall(Decl& d, const SrcModel::Function& f, const std::map<int64_t, std::string>& locations,
				  const std::string& cc)
{
	d.cc = cc;
	if (auto self = locations.find(-1); self != locations.end() && self->second != "stack")
		d.selfReg = self->second;
	else if (!f.cls.empty() && !f.isStatic)
		throw Skip{"a usercall member whose `this` is in no register"};
	for (size_t k = 0; k < d.params.size(); ++k)
		if (auto at = locations.find(static_cast<int64_t>(k)); at != locations.end() && at->second != "stack")
			d.registers[d.params[k].name] = at->second;
}

std::optional<StaleBlocks::Done> StaleBlocks::forward(uint64_t ea)
{
	std::optional<Found> found = find(ea);
	if (!found)
		return std::nullopt;
	const std::vector<std::string>& lines = m_edits.lines(found->file);
	// a block whose opener lies inside another emitted block (src's text broken by an earlier edit): what either
	// branch holds is no longer the emitter's
	for (int64_t k = found->block.whole.first - 1; k >= 1 && lines[k - 1] != kEmittedEnd; --k)
		if (lines[k - 1] == kEmittedBodyIf || lines[k - 1] == kEmittedNeedsIf)
			throw Skip{"the emitted block lies inside the one at " + found->file + ":" + std::to_string(k) +
					   ": src's to mend"};
	std::string listed;
	const SrcModel::Function* declaration = declared(ea, *found, listed);
	if (!declaration)
	{
		// src no longer declares the block's function: a second definition of an address defined elsewhere goes;
		// else src renamed or moved the function (a declaration carries no address to find it by), src's to mend
		std::optional<std::string> elsewhere = definedElsewhere(ea, *found);
		if (!elsewhere)
			throw Skip{"renamed or moved: no declaration of " + listed + "; address not defined elsewhere"};
		// the block's comment goes with it, and one of the blank lines around it
		LineRange gone{commentTop(lines, found->block.whole.first), found->block.whole.last};
		if (gone.last < static_cast<int64_t>(lines.size()) && trimmedLeft(lines[gone.last]).empty() &&
			(gone.first == 1 || trimmedLeft(lines[gone.first - 2]).empty()))
			++gone.last;
		return Done{found->file, m_edits.replace(found->file, gone, ""), *elsewhere};
	}
	const SrcModel::Function& f = *declaration;
	// the old forwarder's call: its receiver (`this`, or a base's cast of it) is the new one's
	std::string old = joined(lines, *found->block.forwarder);
	size_t call = old.find("fnvd::original::");
	if (call == std::string::npos)
		throw Skip{"the emitted block's #else holds no fnvd::original call"};
	bool user = old.compare(call, 25, "fnvd::original::usercall<") == 0;
	std::map<int64_t, std::string> locations =
		user ? userLocations(found->ref, lines[found->registration - 1], f) : std::map<int64_t, std::string>{};
	// the call's arguments, past its template arguments' closing `>`
	std::string self = "this";
	int depth = 0;
	size_t k = old.find('<', call);
	for (; k < old.size(); ++k)
		if (old[k] == '<')
			++depth;
		else if (old[k] == '>' && --depth == 0)
			break;
	std::optional<std::vector<std::string>> passed =
		k + 1 < old.size() && old[k + 1] == '(' ? listArguments(old, k + 1) : std::nullopt;
	if (passed && passed->size() > 1 && (*passed)[1].starts_with("static_cast<"))
	{
		self = (*passed)[1];
		// srcDeclaration makes a const member's cast const itself
		if (self.starts_with("static_cast<const "))
			self.erase(12, 6);
	}
	Decl d = forwarderDecl(m_model, ea, f, self);
	if (user)
		makeUsercall(d, f, locations, "usercall");
	else if (!conventionCode(d.cc))
		throw Skip{"src convention " + d.cc};
	else
	{
		// the old forwarder's call is the exe's convention: where src's declaration now says another (a constructor
		// helper declared cdecl the exe calls thiscall), a forwarder of src's would enter the exe with the arguments
		// elsewhere. Where the block holds src's only declaration the forwarder declares the function anew, in the
		// exe's convention; a declaration outside it is src's, and one of the two is wrong.
		size_t open = old.find('<', call);
		std::string called = old.substr(call + 16, open == std::string::npos ? 0 : open - call - 16);
		if (called == "cdecl_")
			called = "cdecl";
		bool own = f.file == found->file && f.line >= found->block.whole.first && f.line <= found->block.whole.last;
		if ((called == "cdecl" || called == "thiscall" || called == "stdcall" || called == "fastcall") &&
			called != d.cc)
		{
			if (!own)
				throw Skip{"src declares " + f.name + " " + d.cc + ", the old forwarder calls the exe " + called +
						   ": src's declaration or the old forwarder to mend"};
			d.cc = called;
		}
	}
	std::string forwarder = text(d, f, found->file, found->block.whole.first);
	return Done{found->file, m_edits.replace(found->file, found->block.whole, forwarder), ""};
}

std::string StaleBlocks::text(Decl& d, const SrcModel::Function& f, const std::string& file, int64_t first)
{
	const std::vector<std::string>& lines = m_edits.lines(file);
	const SrcModel::Type* type = m_model.type(f.type);
	if (!d.indirect && type && m_types.parts(m_types.canonical(type->ret)).kind == "record")
		throw Skip{"a record returned in registers"};
	// a free function in the namespace open at the block takes its unqualified name there (an anonymous namespace is
	// none namespacesAt lists, nor any the writer spells: tuSpelling)
	std::string opened;
	for (const auto& [name, line] : namespacesAt(lines, first))
		opened += name + "::";
	if (!d.owner && !d.scope.empty() && tuSpelling(d.scope + "::") == opened)
		d.scope.clear();
	// the comment above says "Not reconstructed yet." already (merge kept the forwarder's)
	bool commented = false;
	for (int64_t k = commentTop(lines, first); k < first; ++k)
		commented = commented || lines[k - 1].find(kNotReconstructed) != std::string::npos;
	bool heads = m_model.templateHeads(f.mangled, f.cls) > 0;
	d.outcome = heads ? "specialized" : "defined";
	return m_forwarders.definition(d, heads, indentUnit(lines, 1, static_cast<int64_t>(lines.size())), !commented) +
		   "\n";
}

StaleBlocks::Done StaleBlocks::forwardBody(uint64_t ea)
{
	auto at = [](const std::string& file, int64_t line) { return file + ":" + std::to_string(line); };
	// the address's registrations: the model's (a body's, a C++ structor's) and src's FNVD_REPLACES lines naming it
	const SrcModel::Ref* ref = nullptr;
	std::set<std::pair<std::string, int64_t>> registered;
	for (const SrcModel::Ref& r : m_model.refsAt(ea))
		if (r.kind == "body" || r.kind == "constructor" || r.kind == "destructor")
		{
			registered.emplace(r.file, r.line);
			ref = &r;
		}
	if (auto lines = registrations().find(ea); lines != registrations().end())
		registered.insert(lines->second.begin(), lines->second.end());
	if (registered.empty())
	{
		for (const SrcModel::Ref& r : m_model.refsAt(ea))
			if (r.kind == "fold")
				throw Skip{"a fold: `// Same function as " + r.context + "` at " + at(r.file, r.line) +
						   ", and src registers no body at the address"};
		throw Skip{"src registers no body at the address"};
	}
	if (registered.size() > 1)
	{
		std::string where;
		for (const auto& [file, line] : registered)
			where += (where.empty() ? "" : ", ") + at(file, line);
		throw Skip{"registered more than once (" + where + ")"};
	}
	auto [registrationFile, registrationLine] = *registered.begin();
	if (!ref)
		throw Skip{"the model has no registration at " + at(registrationFile, registrationLine) +
				   " (it names no declaration src has)"};
	if (ref->kind != "body")
		throw Skip{"a C++ " + ref->kind + " (FNVD_REPLACES_CONSTRUCTOR or _DESTRUCTOR) has no forwarder form"};
	if (!registrationFile.ends_with(".cpp"))
		throw Skip{"the registration is in a header (" + registrationFile + ")"};
	const SrcModel::Function* f = ref->function.empty() ? nullptr : m_model.function(ref->function);
	if (!f)
		throw Skip{"the registration's function " + ref->target + " is not in the model"};
	const SrcModel::Definition* definition = m_model.definition(f->mangled);
	if (!definition)
		throw Skip{"no definition of " + f->name};
	if (definition->emitted)
		throw Skip{"an emitted block without its #else forwarder at " + at(definition->file, definition->line)};
	if (definition->instance)
		throw Skip{"the body is a template's definition, its instances' too (" +
				   at(definition->file, definition->line) + ")"};
	if (!definition->file.ends_with(".cpp"))
		throw Skip{"an inline body in a header (" + at(definition->file, definition->line) + ")"};
	for (uint64_t other : m_model.bodyAddresses(f->mangled))
		if (other != ea)
			throw Skip{"its body is registered at " + hexText(other) + " too"};

	// the registration's lines: to the one closing its argument list with `;`
	const std::vector<std::string>& registrationLines = m_edits.lines(registrationFile);
	LineRange registration{registrationLine, registrationLine};
	std::string registrationText;
	for (int depth = 0;; ++registration.last)
	{
		if (registration.last > static_cast<int64_t>(registrationLines.size()))
			throw Skip{"the registration at " + at(registrationFile, registrationLine) + " does not end"};
		const std::string& line = registrationLines[registration.last - 1];
		registrationText += line + "\n";
		for (char c : line)
			depth += c == '(' ? 1 : c == ')' ? -1 : 0;
		if (depth <= 0 && line.find_last_not_of(" \t") != std::string::npos &&
			line[line.find_last_not_of(" \t")] == ';')
			break;
	}
	std::string macro = trimmedLeft(registrationText).substr(0, trimmedLeft(registrationText).find('('));
	std::optional<std::vector<std::string>> words =
		listArguments(registrationText, registrationText.find('('));
	if (!macro.starts_with("FNVD_REPLACES") || !words || words->size() < 2)
		throw Skip{"no FNVD_REPLACES registration at " + at(registrationFile, registrationLine)};

	const std::vector<std::string>& lines = m_edits.lines(definition->file);
	LineRange range;
	try
	{
		range = definitionSpan(lines, *definition);
	}
	catch (const Error& error)
	{
		throw Skip{error.what()};
	}
	if (registrationFile == definition->file && registration.first <= range.last && registration.last >= range.first)
		throw Skip{"the registration at " + at(registrationFile, registrationLine) + " lies in the definition"};

	// the receiver: an override registered through Class's vtable for its Base subobject is the exe's on Base
	std::string self = "this";
	if (macro.find("_VIA") != std::string::npos && words->size() > 2)
		self = "static_cast<" + (*words)[2] + "*>(this)";
	Decl d = forwarderDecl(m_model, ea, *f, self);
	if (macro.starts_with("FNVD_REPLACES_USER"))
		makeUsercall(d, *f, userLocations(ref, registrationText, *f),
					 macro.starts_with("FNVD_REPLACES_USERPURGE") ? "userpurge" : "usercall");
	else if (!conventionCode(d.cc))
		throw Skip{"src convention " + d.cc};
	std::string forwarder = text(d, *f, definition->file, range.first);

	// the `using` alias line right above the registration, where the registration names it and no later line of the
	// file depends on it (each use after it follows another declaration of the alias after it: one before it may be
	// in another scope)
	LineRange gone = registration;
	static const std::regex alias(R"(^using (\w+) = .*;\s*$)");
	std::smatch match;
	if (registration.first >= 2 &&
		std::regex_match(registrationLines[registration.first - 2], match, alias) &&
		std::regex_search(registrationText, std::regex("\\b" + match[1].str() + "\\b")))
	{
		std::string name = match[1];
		std::regex word("\\b" + name + "\\b");
		int64_t own = registration.first - 1;
		bool redeclared = false, needed = false;
		for (int64_t k = registration.last + 1; k <= static_cast<int64_t>(registrationLines.size()) && !needed; ++k)
		{
			const std::string& line = registrationLines[k - 1];
			if (trimmedLeft(line).starts_with("using " + name + " = "))
				redeclared = true;
			else if (!redeclared && std::regex_search(line, word))
				needed = true;
		}
		if (!needed)
			gone.first = own;
	}
	// one of the blank lines around the registration goes with it, where it stood between two
	if (gone.last < static_cast<int64_t>(registrationLines.size()) && trimmedLeft(registrationLines[gone.last]).empty() &&
		(gone.first == 1 || trimmedLeft(registrationLines[gone.first - 2]).empty()))
		++gone.last;

	size_t id;
	try
	{
		id = m_edits.replace(definition->file, range, forwarder);
	}
	catch (const Error& error)
	{
		throw Skip{error.what()};
	}
	// past the first edit an Error would leave the address half edited: it ends the run (runMerge)
	m_edits.replace(registrationFile, gone, "");
	return Done{definition->file, id, ""};
}

}  // namespace

int runMerge(Args& args)
{
	// Disabled with the emitter (the user, 2026-11-14): no emitted body goes into src and no body is turned back
	// into a forwarder. Only the user takes this guard out.
	std::fprintf(stderr, "idakit merge: disabled with the emitter (the user, 2026-11-14)\n");
	return 2;

	SrcModel model(args.required("model"));
	std::filesystem::path dir = args.required("emitted");
	std::string sourcesPath = args.required("sources");
	bool apply = args.flag("apply");
	// given addresses: only those rows (e.g. the ones whose check compiled)
	std::vector<uint64_t> only = parseAddresses(args.positional());
	// --exclude ADDR or @FILE: rows left out (merge-gate.py's list of bodies that do not compile)
	std::vector<uint64_t> excluded;
	if (std::optional<std::string> exclude = args.option("exclude"))
		excluded = parseAddresses({*exclude});
	auto listed = [&](uint64_t ea) { return only.empty() || std::find(only.begin(), only.end(), ea) != only.end(); };

	PdbSources sources(sourcesPath);
	IncludeGraph includes;
	SpecializationHomes homes(model, includes);
	// the forwarders' types (no database: spelling keys reads only the model)
	SrcTypes types(model, args.option("aliases").value_or("harness/tools/ida_type_aliases.tsv"));

	// The rows this run does not merge (form none, excluded, skipped), each with its merge.tsv outcome (none for a
	// row merge.tsv lists only once its block is forwarded) and reason. Those whose src text is an emitted block get
	// their forwarder (StaleBlocks); so does an excluded address that is no row of the run (merge-gate.py's: an old
	// block of it that no longer compiles, or pipeline.sh's reemit.list). An excluded address whose src body is a
	// plain definition gets its forwarder too (forwardBody), else is listed as refused.
	struct Unmerged
	{
		uint64_t ea;
		std::string outcome, file, reason;
		bool requested = false;  // an --exclude'd address that is no row of the run: forwarded, block or plain body
	};
	std::vector<Unmerged> unmerged;
	// the printed rows (every form but none) and whether each is marked: emit.tsv's form, not a grep of the text
	// (a src body keeps its `/* review: ... */` notes and is clean)
	std::vector<std::pair<uint64_t, bool>> rows;
	{
		std::ifstream in(dir / "emit.tsv");
		if (!in)
			throw Error("idakit: cannot read " + (dir / "emit.tsv").string());
		std::string line;
		std::getline(in, line);
		while (std::getline(in, line))
		{
			std::vector<std::string> cells;
			std::istringstream row(line);
			for (std::string cell; std::getline(row, cell, '\t');)
				cells.push_back(cell);
			if (cells.size() < 2 || cells[1].empty())
				continue;
			uint64_t ea = std::stoull(cells[0], nullptr, 16);
			if (!listed(ea))
				continue;
			if (cells[1] != "none")
				rows.emplace_back(ea, cells[1] != "src");
			else if (std::find(excluded.begin(), excluded.end(), ea) == excluded.end())
				unmerged.push_back({ea, "", "", "form none"});
		}
	}
	for (uint64_t ea : excluded)
		if (listed(ea) && std::none_of(rows.begin(), rows.end(), [&](const auto& row) { return row.first == ea; }))
			unmerged.push_back({ea, "", "", "--exclude", true});

	FileEdits edits;
	std::map<std::string, Needs> needs;
	std::vector<Span> spans;
	// the includes this run adds, by file and header: every row that needs one owns it
	std::map<std::pair<std::string, std::string>, size_t> includeEdits;
	// a definition for a file this run creates goes in after the file's needs block, which is written last: inserts
	// at one line keep their order
	struct Deferred
	{
		uint64_t ea;
		std::string file, text, what;
	};
	std::vector<Deferred> deferred;
	std::ofstream table(dir / "merge.tsv", std::ios::trunc);
	std::ofstream merged(dir / "merged.txt", std::ios::trunc);
	table << "address\toutcome\tfile\treason\n";
	int counts[4] = {};
	for (auto [ea, marked] : rows)
	{
		if (std::find(excluded.begin(), excluded.end(), ea) != excluded.end())
		{
			++counts[3];
			unmerged.push_back({ea, "excluded", "", "--exclude"});
			continue;
		}
		std::string file, reason;
		// the row's first edit replaces its forwarder or guarded block: an Error from it (overlapping another row's)
		// skips the row; one after it would leave the row half edited, so it ends the run with nothing written
		bool edited = false;
		auto skip = [&](const std::string& why) { reason = why; };
		auto record = [&](size_t id, const std::string& path, const std::string& what) {
			spans.push_back(Span{ea, path, what, id, std::nullopt});
		};
		// a definition at the end of a .cpp
		auto append = [&](const std::string& path, const std::string& text, const std::string& what) {
			if (!std::filesystem::exists(path))
				return deferred.push_back({ea, path, text, what});
			const std::vector<std::string>& target = edits.lines(path);
			record(edits.insert(path, static_cast<int64_t>(target.size()) + 1,
								(!target.empty() && target.back().empty() ? "" : "\n") + text),
				   path, what);
		};
		// the declaration's comment (away from the definition: outside the lines `replaced` of file) no longer saying
		// "Not reconstructed yet." once the body is plain: the function's declaration, or an explicit
		// specialization's own (the template's declaration of the member is every instance's)
		bool specialization = false;
		auto fixDeclarationComment = [&](const SrcModel::Function& f, LineRange replaced) {
			std::vector<std::pair<std::string, int64_t>> declarations;
			if (!specialization)
				declarations.emplace_back(f.file, f.line);
			else if (auto rows = model.specializations().find(f.mangled); rows != model.specializations().end())
				for (const SrcModel::Specialization& s : rows->second)
					if (!s.definition)
						declarations.emplace_back(s.file, s.line);
			for (const auto& [path, line] : declarations)
			{
				const std::vector<std::string>& declared = edits.lines(path);
				if (line < 1 || line > static_cast<int64_t>(declared.size()))
					continue;
				int64_t first = specialization ? line : declarationFirst(declared, line);
				if (path == file && first <= replaced.last && line >= replaced.first)
					continue;
				if (std::optional<CommentFix> fix = commentFix(declared, first))
					record(edits.replace(path, {fix->first, first - 1}, fix->text), path, "comment");
			}
		};
		auto merge = [&] {
			std::filesystem::path emitted = dir / "src" / (hex8(ea) + ".cpp");
			if (!std::filesystem::exists(emitted))
				return skip("no member definition emitted");
			RefChoice choice = chooseRef(model, ea);
			if (!choice.ref)
				return skip("no src declaration");
			const SrcModel::Ref& ref = *choice.ref;
			const SrcModel::Function* f = model.function(ref.function);
			if (!f)
				return skip("src declaration " + ref.target + " not in the model");
			std::optional<Split> parts = split(readFile(emitted), marked);
			if (!parts)
				return skip("the emitted text is no member definition ending in its FNVD_REPLACES line");
			specialization = parts->head.starts_with("template <>");
			// one static FNVD_GLOBAL per name in a file: a global of the row's that the file (or a row merged into it
			// before) declares as another type is that declaration, which the row's text reads as its own type
			auto unify = [&](const std::string& target) {
				std::vector<std::string> lines = edits.lines(target);
				for (const auto& [line, needers] : needs[target].globals)
					lines.push_back(line);
				for (const EmittedGlobal& global : takeRedeclaredGlobals(parts->globals, lines))
				{
					parts->body = readAs(parts->body, global);
					parts->definition = readAs(parts->definition, global);
				}
			};

			if (ref.kind == "body")
			{
				// re-emitted: only a guarded block of an earlier emit is replaced, as a whole (it holds the
				// registration the ref is)
				std::optional<EmittedBlock> block = emittedBlock(edits.lines(ref.file), ref.line);
				if (!block || !block->forwarder)
					return skip("src body");
				file = ref.file;
				unify(file);
				const std::vector<std::string>& lines = edits.lines(file);
				std::optional<CommentFix> comment = marked ? std::nullopt : commentFix(lines, block->whole.first);
				LineRange replaced{comment ? comment->first : block->whole.first, block->whole.last};
				std::string prefix = comment ? comment->text : "";
				std::string body = withoutKeptNotes(parts->body, keptAbove(lines, replaced.first) + prefix);
				record(edits.replace(file, replaced,
									 prefix + (marked ? guarded(body, joined(lines, *block->forwarder)) : body)),
					   file, "body");
				edited = true;
				if (!marked)
					fixDeclarationComment(*f, replaced);
			}
			else if (ref.kind == "forwarder")
			{
				const SrcModel::Definition* d = model.definition(f->mangled);
				if (!d)
					return skip("no forwarder definition for " + ref.target);
				if (d->instance)
					return skip("the forwarder is a template's definition");
				const std::vector<std::string>& lines = edits.lines(d->file);
				std::optional<LineRange> range = definitionLines(lines, *d, ea);
				if (!range)
					return skip("forwarder lines do not call the address");
				// a marked body leaves the standalone build's forwarder, and so its comment, as they are
				std::optional<CommentFix> comment = marked ? std::nullopt : commentFix(lines, range->first);
				LineRange replaced{comment ? comment->first : range->first, range->last};
				std::string prefix = comment ? comment->text : "";
				if (endsWith(d->file, ".cpp"))
				{
					// an out-of-line forwarder: the definition takes its place
					file = d->file;
					unify(file);
					std::string body = withoutKeptNotes(parts->body, keptAbove(lines, replaced.first) + prefix);
					record(edits.replace(file, replaced,
										 prefix + (marked ? guarded(body, joined(lines, *range)) : body)),
						   file, "body");
					edited = true;
					if (!marked)
						fixDeclarationComment(*f, replaced);
				}
				else if (d->file != f->file || d->line != f->line)
				{
					// a header's out-of-class forwarder: it leaves the header, its comment lines with it, for the .cpp
					// the placement rule picks (an explicit specialization's home's, else the function's), where the
					// definition takes its place (for a marked body, the guard around it with the forwarder, no longer
					// inline); the header keeps the declaration
					if (specialization)
					{
						auto rows = model.specializations().find(f->mangled);
						if (rows == model.specializations().end() ||
							std::none_of(rows->second.begin(), rows->second.end(),
										 [](const SrcModel::Specialization& s) { return !s.definition; }))
							return skip("the specialization has no declaration besides its header definition (idakit "
										"declare --place-specializations)");
						SpecializationHomes::Result home = homes.specializationHome(f->mangled);
						if (const std::string* why = std::get_if<std::string>(&home))
							return skip("no home for the specialization: " + *why);
						file = std::get<SpecializationHome>(home).source;
					}
					else
						file = definitionSource(model, sources, *f, d->file);
					unify(file);
					int64_t top = commentTop(lines, range->first);
					LineRange removed{top, range->last};
					std::string comments = top < replaced.first ? joined(lines, {top, replaced.first - 1}) : "";
					record(edits.replace(d->file, removed, ""), d->file, "declaration");
					edited = true;
					if (std::optional<size_t> id = ensureInclude(edits, file, includeName(includedHeader(d->file))))
						record(*id, file, "include");
					std::string body = withoutKeptNotes(parts->body, comments + prefix);
					append(file,
						   comments + prefix +
							   (marked ? guarded(body, withoutInline(joined(lines, *range))) : body),
						   "body");
					if (!marked)
						fixDeclarationComment(*f, removed);
				}
				else
				{
					// the declaration in the class stays, without its body; the definition goes to a .cpp (for a
					// marked body with the forwarder made an out-of-line definition in its #else)
					size_t scope = f->name.rfind("::");
					std::optional<InlineDefinition> inlined = inlineDefinition(
						joined(lines, *range), scope == std::string::npos ? f->name : f->name.substr(scope + 2));
					if (!inlined)
						return skip("no inline body after the parameter list at " + d->file + ":" +
									std::to_string(d->line));
					file = definitionSource(model, sources, *f, f->file);
					unify(file);
					record(edits.replace(d->file, replaced, prefix + inlined->declaration), d->file, "declaration");
					edited = true;
					std::string definition = parts->body;
					if (marked)
					{
						std::string forwarder = parts->head + "\n{\n";
						for (const std::string& statement : inlined->statements)
							forwarder += "\t" + statement + "\n";
						definition = guarded(parts->body, forwarder + "}\n");
					}
					if (std::optional<size_t> id = ensureInclude(edits, file, includeName(includedHeader(f->file))))
						record(*id, file, "include");
					append(file, definition, "body");
				}
			}
			else
				return skip("no forwarder: " + ref.kind + " " + ref.target);

			// what the body needs: in the file's includes and globals, or (an include new to the file, a global of
			// a marked body) in the file's guarded block
			Needs& guardedNeeds = needs[file];
			auto listed = [](const std::vector<std::pair<std::string, std::vector<uint64_t>>>& list,
							 const std::string& line) {
				return std::find_if(list.begin(), list.end(), [&](const auto& entry) { return entry.first == line; }) !=
					   list.end();
			};
			auto needed = [&](std::vector<std::pair<std::string, std::vector<uint64_t>>>& list,
							  const std::string& line) {
				auto found =
					std::find_if(list.begin(), list.end(), [&](const auto& entry) { return entry.first == line; });
				if (found == list.end())
					list.emplace_back(line, std::vector<uint64_t>{ea});
				else if (std::find(found->second.begin(), found->second.end(), ea) == found->second.end())
					found->second.push_back(ea);
			};
			for (const std::string& include : parts->includes)
			{
				if (include == includeName(file))
					continue;
				std::string line = includeLine(include);
				if (auto added = includeEdits.find({file, include}); added != includeEdits.end())
					record(added->second, file, "include");
				else if (!marked)
				{
					if (std::optional<size_t> id = ensureInclude(edits, file, include))
					{
						includeEdits[{file, include}] = *id;
						record(*id, file, "include");
					}
				}
				else if (!edits.has(file, line) || listed(guardedNeeds.includes, line))
					needed(guardedNeeds.includes, line);
			}
			std::string globals;
			for (const std::vector<std::string>& global : parts->globals)
			{
				if (edits.has(file, global.back(), !marked) && !(marked && listed(guardedNeeds.globals, global.back())))
					continue;
				for (const std::string& line : global)
				{
					if (marked)
						needed(guardedNeeds.globals, line);
					else
						globals += line + "\n";
				}
			}
			if (!globals.empty())
				record(edits.insert(file, afterIncludes(edits.lines(file)), "\n" + globals), file, "global");
		};
		try
		{
			merge();
		}
		catch (const Error& error)
		{
			if (edited)
				throw;
			skip(error.what());
		}
		if (!reason.empty())
		{
			++counts[2];
			unmerged.push_back({ea, "skipped", file, reason});
			continue;
		}
		++counts[marked ? 1 : 0];
		table << hexText(ea) << '\t' << (marked ? "guarded" : "merged") << '\t' << file << "\t\n";
		merged << hexText(ea) << '\n';
	}
	StaleBlocks stale(model, types, edits);
	int forwarded = 0, deleted = 0, kept = 0, bodies = 0, refused = 0;
	for (Unmerged& row : unmerged)
	{
		std::string why;
		std::optional<StaleBlocks::Done> done;
		try
		{
			done = stale.forward(row.ea);
			if (done)
			{
				row.file = done->file;
				if (done->elsewhere.empty())
				{
					spans.push_back(Span{row.ea, done->file, "forwarder", done->id, std::nullopt});
					row.outcome = "forwarded";
					++forwarded;
				}
				else
				{
					row.outcome = "deleted";
					row.reason += "; its emitted block deleted: no src declaration of its function, and the address "
								  "is defined at " +
								  done->elsewhere;
					++deleted;
				}
			}
		}
		catch (const Skip& skip)
		{
			why = skip.reason;
		}
		catch (const Error& error)
		{
			why = error.what();
		}
		if (!why.empty())
		{
			++kept;
			row.reason += "; its emitted block kept: " + why;
			if (row.outcome.empty())
				row.outcome = "kept";
		}
		else if (!done && row.requested)
		{
			// no emitted block: the caller asks for the address's plain body to be emitted again. An Error past
			// forwardBody's first edit (one that would leave the address half edited) ends the run.
			try
			{
				StaleBlocks::Done body = stale.forwardBody(row.ea);
				row.file = body.file;
				spans.push_back(Span{row.ea, body.file, "forwarder", body.id, std::nullopt});
				row.outcome = "forwarded-body";
				row.reason += "; its plain body made its forwarder, its registration removed";
				++bodies;
			}
			catch (const Skip& skip)
			{
				row.outcome = "refused";
				row.reason += "; its body kept: " + skip.reason;
				++refused;
			}
		}
		if (!row.outcome.empty())
			table << hexText(row.ea) << '\t' << row.outcome << '\t' << row.file << '\t' << row.reason << '\n';
	}
	for (const auto& [file, need] : needs)
	{
		if (need.includes.empty() && need.globals.empty())
			continue;
		// the block's lines, each with the rows that need it (its offset in the inserted text)
		std::string text = "\n" + kEmittedNeedsIf + "\n";
		std::vector<std::pair<int64_t, const std::vector<uint64_t>*>> owners;
		int64_t offset = 2;
		for (const auto& [line, needers] : need.includes)
		{
			text += line + "\n";
			owners.emplace_back(offset++, &needers);
		}
		if (!need.includes.empty() && !need.globals.empty())
		{
			text += "\n";
			++offset;
		}
		for (const auto& [line, needers] : need.globals)
		{
			text += line + "\n";
			owners.emplace_back(offset++, &needers);
		}
		size_t id = edits.insert(file, afterIncludes(edits.lines(file)), text + kEmittedEnd + "\n");
		for (const auto& [line, needers] : owners)
			for (uint64_t ea : *needers)
				spans.push_back(Span{ea, file, "needs", id, line});
	}
	for (const Deferred& d : deferred)
	{
		const std::vector<std::string>& target = edits.lines(d.file);
		size_t id = edits.insert(d.file, static_cast<int64_t>(target.size()) + 1, "\n" + d.text);
		spans.push_back(Span{d.ea, d.file, d.what, id, std::nullopt});
	}
	// the files this run edits, for merge-gate.py to restore between its rounds
	{
		std::ofstream files(dir / "merge-files.txt", std::ios::trunc);
		for (const std::string& path : edits.paths())
			files << path << '\n';
	}
	if (apply)
	{
		edits.apply();
		// where each row's text landed, for merge-gate.py to tell whose text a compile error is in
		std::ofstream out(dir / "merge-spans.tsv", std::ios::trunc);
		out << "address\tfile\tfirst\tlast\twhat\n";
		for (const Span& span : spans)
			if (std::optional<LineRange> range = edits.placed(span.id))
			{
				LineRange lines =
					span.offset ? LineRange{range->first + *span.offset, range->first + *span.offset} : *range;
				out << hexText(span.ea) << '\t' << span.file << '\t' << lines.first << '\t' << lines.last << '\t'
					<< span.what << '\n';
			}
	}
	std::fprintf(stderr,
				 "idakit merge: %zu emitted functions: %d merged, %d guarded (unresolved spots), %d skipped, "
				 "%d excluded; %d stale emitted blocks forwarded, %d deleted, %d kept; %d excluded plain bodies "
				 "forwarded, %d refused%s\n",
				 rows.size(), counts[0], counts[1], counts[2], counts[3], forwarded, deleted, kept, bodies, refused,
				 apply ? "" : " (dry run: src/ unchanged)");
	return 0;
}

}  // namespace idakit
