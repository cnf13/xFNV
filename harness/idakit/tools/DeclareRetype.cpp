// declare --retype TABLE: a slot table's rows written into src (docs/IDAKIT.md "idakit declare"). The table is
// --pdb-signatures' (or any tool's with its columns, by name): id (`<mangled>#<slot>`, slot `ret` or a parameter's
// index from 0), verdict, comment and key (src's type key for the slot). An `apply` row's slot gets the key's type in
// the function's declaration and its definition (a plain body's head; a forwarder's head and its fnvd::original call:
// the result type, a parameter's `&` where it turns between pointer and reference), with the forward declaration or
// include a file needs to name the new type (declarationNeeds); a template instance's types are its template's and
// do not change. An emitted definition's body is never edited (DIR/reemit.list), its #else forwarder is, as any
// forwarder. A row's comment (`// PDB: ...` for keep, `// Inferred type: ...` or `// Guessed type: ...` from
// inference) goes above the declaration (an explicit specialization's), once. Names, default arguments, attributes
// (FNVD_NO_STACK_COOKIE) and the rest of the text stay. Each change is
// a line of DIR/applied.tsv (file, line, old text, new text, row id; text escaped \\ \t \n) that --revert puts back by
// row. Dry run unless --apply; DIR/retype.tsv has each row's outcome.

#include "DeclarePdb.hpp"

#include "idakit/Address.hpp"
#include "idakit/Database.hpp"
#include "idakit/SrcEdit.hpp"
#include "idakit/SrcModel.hpp"
#include "idakit/SrcTypes.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace idakit
{

namespace
{

// [begin, end) of a text
struct Span
{
	size_t begin = 0, end = 0;
};

// A declaration's or definition's pieces in a file's text: its first line's start, the return type (empty for none
// to change), each parameter's declarator (type and name: no leading attributes, no default argument) and name
// (empty: unnamed), the parameter list's closing parenthesis
struct Signature
{
	size_t first = 0;
	Span ret;
	std::vector<Span> params, names;
	size_t close = 0;
};

// the offset past the bracket matching the one at `open`, skipping comments; npos where it does not close
size_t matching(const std::string& text, size_t open)
{
	int depth = 0;
	for (size_t k = open; k < text.size(); ++k)
	{
		if (text.compare(k, 2, "//") == 0)
			k = std::min(text.find('\n', k), text.size() - 1);
		else if (text.compare(k, 2, "/*") == 0)
			k = std::min(text.find("*/", k) + 1, text.size() - 1);
		else if (text[k] == '(' || text[k] == '<' || text[k] == '[' || text[k] == '{')
			++depth;
		else if (text[k] == ')' || text[k] == '>' || text[k] == ']' || text[k] == '}')
		{
			// `->` and `>>` in a default argument's expression are no brackets of the list
			if (text[k] == '>' && k > 0 && text[k - 1] == '-')
				continue;
			if (--depth == 0)
				return k;
		}
	}
	return std::string::npos;
}

// The signature whose name (the function's last name component) appears between lines from and to (inclusive)
std::optional<Signature> signatureAt(const Text& t, const std::vector<std::string>& lines, int64_t from, int64_t to,
									 const std::string& name)
{
	std::string pattern;
	if (name.starts_with("operator"))
	{
		pattern = R"(\boperator\s*)";
		for (char c : name.substr(8))
			pattern += std::string(std::isalnum(static_cast<unsigned char>(c)) ? "" : "\\") + c;
	}
	else
		pattern = "\\b" + name;
	std::regex re(pattern + R"(\s*\()");
	size_t begin = t.at(from), end = t.at(to + 1);
	std::smatch m;
	std::string window = t.text.substr(begin, end - begin);
	if (!std::regex_search(window, m, re))
		return std::nullopt;
	Signature s;
	size_t nameAt = begin + size_t(m.position(0));
	size_t open = t.text.find('(', nameAt + name.size() - (name.starts_with("operator()") ? 1 : 0));
	if (name.starts_with("operator()"))
		open = t.text.find('(', t.text.find(')', nameAt) + 1);
	s.close = matching(t.text, open);
	if (s.close == std::string::npos)
		return std::nullopt;
	// the parameters: the list split at its top-level commas
	std::vector<Span> pieces;
	int depth = 0;
	size_t start = open + 1;
	for (size_t k = open + 1; k <= s.close; ++k)
	{
		char c = t.text[k];
		if (t.text.compare(k, 2, "//") == 0)
			k = t.text.find('\n', k);
		else if (t.text.compare(k, 2, "/*") == 0)
			k = t.text.find("*/", k) + 1;
		else if (c == '(' || c == '<' || c == '[' || c == '{')
			++depth;
		else if ((c == ')' || c == '>' || c == ']' || c == '}') && !(c == '>' && t.text[k - 1] == '-'))
			--depth;
		if ((c == ',' && depth == 0) || k == s.close)
		{
			pieces.push_back({start, k});
			start = k + 1;
		}
	}
	for (Span piece : pieces)
	{
		std::string text = t.text.substr(piece.begin, piece.end - piece.begin);
		if (trim(text).empty() || trim(text) == "void")
			continue;
		// leading blanks and attributes, then the declarator up to a top-level `=`
		size_t b = text.find_first_not_of(" \t\n");
		while (text.compare(b, 2, "[[") == 0)
			b = text.find_first_not_of(" \t\n", text.find("]]", b) + 2);
		size_t e = text.size();
		int d = 0;
		for (size_t k = b; k < text.size(); ++k)
		{
			char c = text[k];
			if (c == '(' || c == '<' || c == '[')
				++d;
			else if (c == ')' || c == '>' || c == ']')
				--d;
			else if (c == '=' && d == 0 && (k + 1 >= text.size() || text[k + 1] != '='))
			{
				e = k;
				break;
			}
		}
		e = text.find_last_not_of(" \t\n", e - 1) + 1;
		Span declarator{piece.begin + b, piece.begin + e};
		// the name: the last identifier, where something before it is the type (not cv or a lone keyword)
		Span named{declarator.end, declarator.end};
		size_t k = e;
		while (k > b && identifierChar(text[k - 1]))
			--k;
		std::string before = trim(text.substr(b, k - b));
		static const std::set<std::string> lone = {"", "const", "volatile", "unsigned", "signed", "struct",
												   "class", "enum", "union"};
		if (k < e && !lone.count(before) && !std::isdigit(static_cast<unsigned char>(text[k])) &&
			(before.back() == '*' || before.back() == '&' || before.back() == '>' || identifierChar(before.back())))
			named = {piece.begin + k, piece.begin + e};
		s.params.push_back(declarator);
		s.names.push_back(named);
	}
	// the return type: the head from the declaration's first line to the name, without `template <...>`, specifiers,
	// attributes (the seam's attribute macro FNVD_NO_STACK_COOKIE), a convention macro and
	// the qualifier
	s.first = t.at(declarationFirst(lines, t.lineOf(nameAt)));
	size_t head = s.first, tail = nameAt;
	auto skipBlank = [&]() {
		while (head < tail && std::isspace(static_cast<unsigned char>(t.text[head])))
			++head;
	};
	skipBlank();
	static const std::set<std::string> specifiers = {"virtual", "static", "inline", "explicit", "constexpr", "friend",
													 "extern", "FNVD_NOINLINE", "__forceinline", "FNVD_NO_STACK_COOKIE"};
	for (bool more = true; more && head < tail;)
	{
		more = false;
		if (t.text.compare(head, 8, "template") == 0)
		{
			size_t lt = t.text.find('<', head);
			size_t gt = lt < tail ? matching(t.text, lt) : std::string::npos;
			if (gt != std::string::npos && gt < tail)
				head = gt + 1, more = true;
		}
		else if (t.text.compare(head, 2, "[[") == 0)
			head = t.text.find("]]", head) + 2, more = true;
		else
		{
			size_t w = head;
			while (w < tail && identifierChar(t.text[w]))
				++w;
			if (specifiers.count(t.text.substr(head, w - head)))
				head = w, more = true;
		}
		skipBlank();
	}
	// from the back: blanks, the qualifier `X<...>::`, a convention macro
	auto blankBack = [&]() {
		while (tail > head && std::isspace(static_cast<unsigned char>(t.text[tail - 1])))
			--tail;
	};
	blankBack();
	while (tail >= head + 2 && t.text.compare(tail - 2, 2, "::") == 0)
	{
		tail -= 2;
		if (tail > head && t.text[tail - 1] == '>')
		{
			int d = 0;
			for (; tail > head; --tail)
			{
				char c = t.text[tail - 1];
				d += c == '>' ? 1 : c == '<' ? -1 : 0;
				if (d == 0 && c == '<')
				{
					--tail;
					break;
				}
			}
		}
		while (tail > head && identifierChar(t.text[tail - 1]))
			--tail;
		blankBack();
	}
	static const std::regex convention(R"((FNVD_[A-Z]+CALL|FNVD_CDECL|__cdecl|__stdcall|__thiscall|__fastcall)$)");
	std::smatch cm;
	std::string headText = t.text.substr(head, tail - head);
	if (std::regex_search(headText, cm, convention))
	{
		tail = head + size_t(cm.position(0));
		blankBack();
	}
	s.ret = {head, tail};
	return s;
}

struct Row
{
	std::string id, mangled, slot, verdict, comment, key, address, family;
};

class Retype
{
public:
	Retype(const SrcModel& model, SrcTypes& types) : m_model(model), m_types(types) {}

	// the outcome per row: applied, commented, reemit (the definition is emitted; the declaration is applied) or
	// `failed: why`
	std::map<std::string, std::string> outcome;
	std::vector<std::pair<std::string, std::string>> reemit;  // (address, row id)

	void function(const std::string& mangled, const std::vector<const Row*>& rows);
	// the changes per file, applied: the files written (apply) and applied.tsv's lines
	void write(bool apply, std::ostream& applied, std::map<std::string, std::vector<std::string>>& perFile)
	{
		m_src.write(apply, applied, m_families, perFile);
	}

private:
	// a declaration's or definition's name lines from..to; a definition's last line (its forwarder call's bound)
	struct Site
	{
		std::string file;
		int64_t from = 0, to = 0;
		bool declaration = false, definition = false;
		int64_t last = 0;
	};
	Text& text(const std::string& file) { return m_src.text(file); }
	std::string spelled(const std::string& key, const std::string& name) const;

	const SrcModel& m_model;
	SrcTypes& m_types;
	SrcChanges m_src;
	std::map<std::string, std::string> m_families;  // row id -> its virtual family's tag
	Includes m_includes;
	// the include and forward declaration lines the written functions add, per file (declarationNeeds)
	std::map<std::string, std::set<std::string>> m_inserted;
};

std::string Retype::spelled(const std::string& key, const std::string& name) const
{
	std::optional<std::string> s = m_types.spell(m_types.fixedWidthKey(key), name);
	if (!s)
		throw Error("src cannot spell " + key);
	return srcSpelling(*s);
}

void Retype::function(const std::string& mangled, const std::vector<const Row*>& rows)
{
	for (const Row* r : rows)
		m_families[r->id] = r->family;
	auto fail = [&](const std::string& why) {
		for (const Row* r : rows)
			outcome[r->id] = "failed: " + why;
	};
	const SrcModel::Function* f = m_model.function(mangled);
	std::optional<SrcTypes::Function> type = f ? SrcTypes::function(f->type) : std::nullopt;
	if (!type)
		return fail("not in the model");
	std::string name = f->name;
	for (int depth = 0, k = int(name.size()) - 1; k > 0; --k)
	{
		depth += name[k] == '>' ? 1 : name[k] == '<' ? -1 : 0;
		if (depth == 0 && name.compare(k - 1, 2, "::") == 0 && !name.substr(k + 1).starts_with("operator<"))
		{
			name = name.substr(k + 1);
			break;
		}
	}
	// the sites: the declaration, each explicit specialization's declaration, the definition; an emitted one's
	// forwarder under its block's #else (the hybrid's body is emitted again). A class template instance's member
	// (or a function template's specialization) is declared by the template for every instance, and an explicit
	// specialization must keep the template's types: no type of it changes (the rows fail), its comment goes above
	// its explicit specialization
	const SrcModel::Record* owner = f->cls.empty() ? nullptr : m_model.record(f->cls);
	auto specs = m_model.specializations().find(mangled);
	bool specialized = specs != m_model.specializations().end() && !specs->second.empty();
	bool shared = specialized || (owner && !owner->pattern.empty() && !SrcModel::explicitSpecialization(*owner));
	if (shared && std::any_of(rows.begin(), rows.end(), [](const Row* r) { return r->verdict == "apply"; }))
		return fail("a template's instance: the template's declaration types every instance's");
	if (shared && !specialized)
		return fail("a class template's member with no explicit specialization: its declaration is every "
					"instance's");
	std::vector<Site> sites;
	if (!shared)
		sites.push_back({f->file, f->line, f->line, true, false, 0});
	if (specialized)
	{
		bool declared = std::any_of(specs->second.begin(), specs->second.end(),
									[](const SrcModel::Specialization& s) { return !s.definition; });
		for (const SrcModel::Specialization& s : specs->second)
			sites.push_back({s.file, s.line, s.endLine, !s.definition || (!declared && sites.empty()), s.definition,
							 s.endLine});
	}
	const SrcModel::Definition* def = m_model.definition(mangled);
	bool emitted = def && def->emitted;
	if (def && !def->instance && !emitted)
		sites.push_back({def->file, def->line, def->line, false, true, def->endLine});
	if (emitted && !def->instance && std::filesystem::exists(def->file))
		if (std::optional<EmittedBlock> block = emittedBlock(text(def->file).lines(), def->line);
			block && block->forwarder)
			sites.push_back({def->file, block->forwarder->first, block->forwarder->first, false, true,
							 block->forwarder->last});
	// one site per name line: an inline definition is its declaration
	std::vector<Site> unique;
	for (const Site& s : sites)
	{
		auto same = std::find_if(unique.begin(), unique.end(), [&](const Site& u) {
			return u.file == s.file && s.from <= u.to && u.from <= s.to;
		});
		if (same == unique.end())
			unique.push_back(s);
		else
		{
			same->declaration = same->declaration || s.declaration;
			same->definition = same->definition || s.definition;
			same->last = std::max(same->last, s.last);
		}
	}
	std::vector<Change> changes;
	std::map<std::string, std::string> files;  // change index's file
	std::vector<std::string> changeFiles;
	// the include and forward declaration lines this function's rows add, kept only where all of it is written
	std::map<std::string, std::set<std::string>> inserted = m_inserted;
	try
	{
		std::set<std::string> written;
		for (const Site& site : unique)
		{
			if (site.file.empty() || !std::filesystem::exists(site.file))
				throw Error("no file " + site.file);
			Text& t = text(site.file);
			std::vector<std::string> lines = t.lines();
			int64_t to = site.to;
			if (site.definition)
				to = std::max(to, std::min<int64_t>(site.last, site.from + 8));
			std::optional<Signature> sig = signatureAt(t, lines, site.from, std::max(to, site.from + 3), name);
			if (!sig || sig->params.size() != type->params.size())
				throw Error("the signature at " + site.file + ":" + std::to_string(site.from) + " is not " + name +
							"'s with " + std::to_string(type->params.size()) + " parameters");
			// the forwarder's own call in a definition: fnvd::original::X<R>(ADDRESS, ...) of the function's address
			std::optional<Span> callType;
			std::vector<Span> callArgs;
			size_t callAt = std::string::npos;
			uint64_t ea = rows.empty() || rows[0]->address.empty() ? 0 : std::stoull(rows[0]->address, nullptr, 16);
			if (site.definition && ea)
			{
				size_t end = t.at(site.last + 1);
				for (size_t call = t.text.find("fnvd::original::", sig->close); call < end && callArgs.empty();
					 call = t.text.find("fnvd::original::", call + 1))
				{
					size_t lt = t.text.find('<', call), paren = t.text.find('(', call);
					size_t close = paren < end ? matching(t.text, paren) : std::string::npos;
					if (close == std::string::npos)
						break;
					std::vector<Span> args;
					int depth = 0;
					size_t start = paren + 1;
					for (size_t k = paren + 1; k <= close; ++k)
					{
						char c = t.text[k];
						if (c == '(' || c == '<' || c == '[')
							++depth;
						else if (c == ')' || c == '>' || c == ']')
							--depth;
						if ((c == ',' && depth == 0) || k == close)
						{
							size_t b = t.text.find_first_not_of(" \t\n", start);
							size_t e = t.text.find_last_not_of(" \t\n", k - 1) + 1;
							args.push_back({b, std::max(b, e)});
							start = k + 1;
						}
					}
					std::string first = args.empty() ? "" : t.text.substr(args[0].begin, args[0].end - args[0].begin);
					char* stop = nullptr;
					if (first.empty() || std::strtoull(first.c_str(), &stop, 0) != ea || *stop)
						continue;
					callArgs = args;
					callAt = call;
					if (lt < paren)
						callType = Span{lt + 1, matching(t.text, lt)};
				}
			}
			for (const Row* r : rows)
			{
				if (r->verdict != "apply" || r->key.empty())
					continue;
				// the new type where the site's file does not see it (a definition's file seeing the declaration's
				// header sees what that gets): its forward declaration, or its header's include
				if (site.declaration || !m_includes.closure(site.file).count(f->file))
					if (std::optional<Change> needs = declarationNeeds(m_model, m_types, m_includes, site.file, t,
																		r->key, false, inserted, r->id))
						changes.push_back(*needs), changeFiles.push_back(site.file);
				if (r->slot == "ret")
				{
					std::string old = t.text.substr(sig->ret.begin, sig->ret.end - sig->ret.begin);
					if (old.empty())
						throw Error("no return type at " + site.file + ":" + std::to_string(site.from));
					std::string now = spelled(r->key, "");
					if (now != old)
						changes.push_back({sig->ret.begin, old.size(), now, r->id}), changeFiles.push_back(site.file);
					if (callType)
					{
						// the call's result as fnvd::original passes it: a reference as its pointer, which the
						// statement dereferences (`return *fnvd::original::...`)
						SrcTypes::Parts p = m_types.parts(r->key);
						bool reference = p.kind == "lref" || p.kind == "rref";
						std::string abi = reference ? spelled("ptr(" + p.of + ")", "") : now;
						std::string was = t.text.substr(callType->begin, callType->end - callType->begin);
						if (was != abi)
							changes.push_back({callType->begin, was.size(), abi, r->id}), changeFiles.push_back(site.file);
						// the statement: `fnvd::original::...` for void, `return ...`, `return *...` for a reference
						size_t statement = t.text.rfind('\n', callAt) + 1;
						std::string lead = trim(t.text.substr(statement, callAt - statement));
						std::string have = lead == "return" ? "return " : lead;
						std::string want = abi == "void" ? "" : reference ? "return *" : "return ";
						if ((have.empty() || have == "return " || have == "return *") && have != want)
						{
							size_t from = lead.empty() ? callAt : statement + t.text.substr(statement).find(lead);
							std::string old = t.text.substr(from, callAt + 16 - from);
							changes.push_back({from, old.size(), want + "fnvd::original::", r->id});
							changeFiles.push_back(site.file);
						}
					}
					continue;
				}
				if (r->slot == "this")
					throw Error("a this row is not written");
				size_t k = std::stoul(r->slot);
				if (k >= sig->params.size())
					throw Error("no parameter " + r->slot);
				Span p = sig->params[k], n = sig->names[k];
				std::string paramName = t.text.substr(n.begin, n.end - n.begin);
				std::string old = t.text.substr(p.begin, p.end - p.begin);
				std::string now = spelled(r->key, paramName);
				if (now != old)
				{
					// a line the change takes past 120 columns breaks before the parameter
					size_t lineStart = t.text.rfind('\n', p.begin) + 1, lineEnd = t.text.find('\n', p.begin);
					std::string line = t.text.substr(lineStart, lineEnd - lineStart);
					size_t width = 0;
					for (char c : line)
						width += c == '\t' ? 4 : 1;
					size_t before = width;
					width += now.size() - old.size();
					size_t at = p.begin, length = old.size();
					std::string text = now;
					if (before <= 120 && width > 120 && at > lineStart && t.text[at - 1] == ' ' &&
						trim(t.text.substr(lineStart, at - lineStart)) != "")
					{
						std::string indent = line.substr(0, line.find_first_not_of(" \t"));
						text = "\n" + indent + (indent.find('\t') != std::string::npos || indent.empty() ? "\t" : "    ") + now;
						--at, ++length;
					}
					changes.push_back({at, length, text, r->id}), changeFiles.push_back(site.file);
				}
				// the forwarder passes a parameter turned between pointer and reference as the address it was
				if (!callArgs.empty() && !paramName.empty())
				{
					std::string oldKind = m_types.parts(m_types.bare(type->params[k])).kind;
					std::string newKind = m_types.parts(m_types.bare(r->key)).kind;
					// a usercall forwarder passes its register arguments cast to uint32_t, which a reference defeats
					if ((oldKind == "lref") != (newKind == "lref") &&
						t.text.compare(callAt, 25, "fnvd::original::usercall<") == 0)
						throw Error("a usercall forwarder passes " + paramName + " in a register");
					for (const Span& a : callArgs)
					{
						std::string arg = t.text.substr(a.begin, a.end - a.begin);
						if (oldKind == "ptr" && newKind == "lref" && arg == paramName)
							changes.push_back({a.begin, arg.size(), "&" + arg, r->id}), changeFiles.push_back(site.file);
						else if (oldKind == "lref" && newKind == "ptr" && arg == "&" + paramName)
							changes.push_back({a.begin, arg.size(), paramName, r->id}), changeFiles.push_back(site.file);
					}
				}
			}
			// the comments above the declaration (an explicit specialization's `template <>` line), each once
			if (site.declaration)
			{
				int64_t first = std::min(t.lineOf(sig->first), site.from);
				std::string indent = lines[first - 1].substr(0, lines[first - 1].find_first_not_of(" \t"));
				std::set<std::string> above;
				for (int64_t l = commentTop(lines, first); l < first; ++l)
					above.insert(trim(lines[l - 1]));
				for (const Row* r : rows)
					if (!r->comment.empty() &&
						!std::any_of(above.begin(), above.end(),
									 [&](const std::string& l) { return l.size() > 10 && trim(r->comment).starts_with(l); }) &&
						written.insert(r->comment).second)
					{
						Change c{t.at(first), 0, commentBlock(r->comment, indent), r->id};
						c.comment = true;
						changes.push_back(c), changeFiles.push_back(site.file);
					}
			}
		}
	}
	catch (const std::exception& e)
	{
		return fail(e.what());
	}
	m_inserted = std::move(inserted);
	for (size_t i = 0; i < changes.size(); ++i)
		m_src.add(changeFiles[i], changes[i]);
	for (const Row* r : rows)
	{
		bool changed = std::any_of(changes.begin(), changes.end(), [&](const Change& c) { return c.row == r->id; });
		if (r->verdict == "family")
			continue;  // its outcome says why its type is not written
		if (r->verdict == "apply" && emitted)
		{
			outcome[r->id] = "reemit";
			reemit.emplace_back(r->address, r->id);
		}
		else
			outcome[r->id] = changed ? (r->verdict == "apply" ? "applied" : "commented") : "unchanged";
	}
}

}  // namespace

// DeclareFields.cpp: a table of field rows (--pdb-fields')
int declareRetypeFields(const SrcModel& model, SrcTypes& types, const std::string& table,
						const std::filesystem::path& out, bool apply);

int declareRetype(const SrcModel& model, SrcTypes& types, const std::string& table, const std::filesystem::path& out,
				  bool apply)
{
	std::ifstream in(table);
	if (!in)
		throw Error("declare --retype: cannot read " + table);
	std::string line;
	std::getline(in, line);
	std::vector<std::string> header = cells(line);
	if (std::find(header.begin(), header.end(), "field") != header.end())
		return declareRetypeFields(model, types, table, out, apply);
	auto column = [&](const std::string& name) -> size_t {
		auto found = std::find(header.begin(), header.end(), name);
		if (found == header.end())
			throw Error("declare --retype: " + table + " has no column " + name);
		return size_t(found - header.begin());
	};
	size_t id = column("id"), verdict = column("verdict"), comment = column("comment"), key = column("key"),
		   address = column("address");
	// optional: a virtual family's tag (`<root>#<slot>/<rows>`, --pdb-signatures'), whose rows go in together or not
	std::optional<size_t> family;
	if (std::find(header.begin(), header.end(), "family") != header.end())
		family = column("family");
	std::vector<Row> rows;
	std::map<std::string, size_t> families;  // tag -> its apply rows in the table
	while (std::getline(in, line))
	{
		std::vector<std::string> c = cells(line);
		if (c.size() <= std::max({id, verdict, comment, key, address}))
			continue;
		Row r{c[id], "", "", c[verdict], c[comment], c[key], c[address], family && *family < c.size() ? c[*family] : ""};
		size_t hash = r.id.rfind('#');
		if (hash == std::string::npos || (r.verdict != "apply" && r.comment.empty()))
			continue;
		r.mangled = r.id.substr(0, hash);
		r.slot = r.id.substr(hash + 1);
		if (r.verdict == "apply" && !r.family.empty())
			++families[r.family];
		rows.push_back(r);
	}
	Retype retype(model, types);
	std::map<std::string, std::vector<const Row*>> byFunction;
	for (Row& r : rows)
	{
		size_t slash = r.family.rfind('/');
		if (r.verdict == "apply" && slash != std::string::npos &&
			families[r.family] != std::stoul(r.family.substr(slash + 1)))
		{
			retype.outcome[r.id] = "failed: its virtual family's change takes " + r.family.substr(slash + 1) +
								   " rows, the table has " + std::to_string(families[r.family]);
			if (r.comment.empty())
				continue;
			r.verdict = "family";
		}
		byFunction[r.mangled].push_back(&r);
	}
	for (const auto& [mangled, own] : byFunction)
		retype.function(mangled, own);
	std::ofstream applied(out / "applied.tsv", std::ios::trunc);
	applied << "file\tline\told\tnew\trow\tfamily\n";
	std::map<std::string, std::vector<std::string>> perFile;
	retype.write(apply, applied, perFile);
	std::ofstream outcomes(out / "retype.tsv", std::ios::trunc);
	outcomes << "id\toutcome\n";
	std::map<std::string, size_t> counts;
	for (const auto& [row, outcome] : retype.outcome)
	{
		outcomes << row << '\t' << outcome << '\n';
		++counts[outcome.substr(0, outcome.find(':'))];
	}
	std::ofstream reemit(out / "reemit.list", std::ios::trunc);
	std::set<std::string> addresses;
	for (const auto& [ea, row] : retype.reemit)
		if (addresses.insert(ea).second)
			reemit << ea << '\n';
	std::string summary;
	for (const auto& [outcome, count] : counts)
		summary += " " + std::to_string(count) + " " + outcome;
	// the tables are whole before anything is printed (a closed pipe ends the process at its first print)
	applied.close();
	outcomes.close();
	reemit.close();
	std::fprintf(stderr, "idakit declare --retype: %zu rows:%s; %zu files, %zu to re-emit%s\n", rows.size(),
				 summary.c_str(), perFile.size(), addresses.size(), apply ? "" : " (dry run: src unchanged)");
	for (const auto& [file, ids] : perFile)
	{
		std::string list;
		for (const std::string& r : ids)
			list += " " + r;
		std::fprintf(stderr, "  %s:%s\n", file.c_str(), list.c_str());
	}
	return 0;
}

// --revert: the named rows' changes in DIR/applied.tsv put back, with every row of a named row's virtual family (its
// family column: the family changes whole or not at all), each found at its line (else the nearest line that holds
// its new text), from the end of each file up; the rows left stay in applied.tsv
int declareRevert(const std::filesystem::path& out, const std::vector<std::string>& ids)
{
	std::set<std::string> wanted;
	for (const std::string& id : ids)
		if (id.starts_with("@"))
		{
			std::ifstream list(id.substr(1));
			for (std::string line; std::getline(list, line);)
				if (!line.empty())
					wanted.insert(line);
		}
		else
			wanted.insert(id);
	std::ifstream in(out / "applied.tsv");
	if (!in)
		throw Error("declare --revert: cannot read " + (out / "applied.tsv").string());
	std::string header, line;
	std::getline(in, header);
	struct Entry
	{
		std::string file, old, now, row, raw;
		int64_t line = 0;
	};
	std::vector<Entry> all, kept;
	std::map<std::string, std::vector<Entry>> byFile;
	std::set<std::string> families;
	while (std::getline(in, line))
	{
		std::vector<std::string> c = cells(line);
		if (c.size() < 5)
			continue;
		all.push_back(Entry{c[0], unescaped(c[2]), unescaped(c[3]), c[4], line, std::stoll(c[1])});
		if (wanted.count(c[4]) && c.size() > 5 && !c[5].empty())
			families.insert(c[5]);
	}
	for (const Entry& e : all)
	{
		std::vector<std::string> c = cells(e.raw);
		bool family = c.size() > 5 && families.count(c[5]);
		(wanted.count(e.row) || family ? byFile[e.file] : kept).push_back(e);
	}
	size_t reverted = 0, missing = 0;
	std::vector<std::string> gone;
	for (auto& [file, entries] : byFile)
	{
		Text t(readFile(file));
		// each entry's offset: its new text on its line, else the occurrence nearest to it
		std::vector<std::pair<size_t, const Entry*>> found;
		for (const Entry& e : entries)
		{
			size_t best = std::string::npos;
			int64_t distance = INT64_MAX;
			for (size_t at = t.text.find(e.now); at != std::string::npos; at = t.text.find(e.now, at + 1))
			{
				int64_t d = std::llabs(t.lineOf(at) - e.line);
				if (d < distance)
					best = at, distance = d;
			}
			if (best == std::string::npos)
			{
				++missing;
				gone.push_back(file + ":" + std::to_string(e.line) + ": " + e.row + "'s change is not there any more");
				continue;
			}
			found.emplace_back(best, &e);
		}
		std::sort(found.begin(), found.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
		for (const auto& [at, e] : found)
		{
			t.text.replace(at, e->now.size(), e->old);
			++reverted;
		}
		std::ofstream o(file, std::ios::binary | std::ios::trunc);
		o << t.text;
	}
	std::ofstream rewrite(out / "applied.tsv", std::ios::trunc);
	rewrite << header << '\n';
	for (const Entry& e : kept)
		rewrite << e.raw << '\n';
	rewrite.close();
	for (const std::string& g : gone)
		std::fprintf(stderr, "  %s\n", g.c_str());
	std::fprintf(stderr, "idakit declare --revert: %zu changes of %zu rows put back, %zu not found\n", reverted,
				 wanted.size(), missing);
	return missing ? 1 : 0;
}

}  // namespace idakit
