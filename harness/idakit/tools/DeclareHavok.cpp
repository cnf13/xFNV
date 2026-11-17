// declare --hk-symbols TSV: src's declarations of the exe functions tied to a Havok library symbol made the symbol's
// (docs/IDAKIT.md "idakit declare"). TSV is env/scratch/HkSyms/apply/symbols.tsv, which `havok-syms.py apply` writes
// (address, lib, object, mangled, demangled, evidence, mark, sse: the SSE build's type of each math slot from the SDK's
// header, names: the library's parameter names); an address with exactly one row and no mark is judged. Per address,
// src's declaration against the symbol's:
// - the name: the symbol's, where src's is IDA's `sub_` placeholder (provenance guess); a constructor, destructor or
//   deleting destructor is the porting rules' Construct, Destruct or DeletingDestruct (a second copy's
//   `Destruct_<ADDR>`, as declare names it);
// - the owner: the symbol's class, where src has it and src's uses do not name src's class (a move), else src's;
// - the types: the symbol's (a math slot the SSE header's), each in src's spelling and defined in src, a class result
//   through src's explicit hidden result pointer, the convention and stack bytes the exe's (exefacts: pops, ecx, the
//   argument slots read), no narrowing the exe keeps (SlotRowChecks::narrowing), a virtual's family changed whole.
// Verdicts in DIR/hk-declare.tsv: same (nothing but the evidence comment changes), apply (types, name, owner),
// disagree (src's declaration is the game PDB's, src's own, inferred or an earlier library one, and differs: not
// changed), pdb (the game PDB's and the same), open with the reason. --apply writes the apply and same rows of ADDR...
// (all of them without): the declaration where it is (declare's Writer::inPlace for the header's needs) or, for a
// move, through declare's Writer on its new owner, its forwarder (an emitted block replaced by the plain forwarder, its
// address in reemit.list), every use srcdb's function_use records (the identifier at its spelling) and each fold
// comment naming it; callers inside emitted blocks go to reemit.list. A row whose old name src spells anywhere else
// (a dependent call in a template, text srcdb did not see) is open: nothing is written for it. DIR/hk-applied.tsv has
// each outcome, DIR/renames.tsv the database's new names (harness/tools/ida_rename.py).

#include "DeclarePdb.hpp"
#include "DeclareWriter.hpp"
#include "IdaSupport.hpp"

#include "idakit/Address.hpp"
#include "idakit/Database.hpp"
#include "idakit/SrcEdit.hpp"
#include "idakit/SrcIda.hpp"
#include "idakit/SrcModel.hpp"
#include "idakit/SrcPlace.hpp"
#include "idakit/SrcTypes.hpp"

#include <Facts.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <regex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace idakit
{

namespace
{

// One symbols.tsv row's symbol, read from its demangled text
struct Symbol
{
	std::string lib, object, mangled, demangled, evidence;
	std::string access;  // public, protected, private; "" for a free function
	bool isStatic = false, isVirtual = false, isConst = false, variadic = false;
	bool c = false;  // an extern "C" name: no types
	std::string cc;  // thiscall, cdecl, stdcall, fastcall
	std::string ret, scope, name;
	std::vector<std::string> params;  // as the demangler spells them
	std::string structor;			  // ctor, dtor, deleting or ""
	// the SSE build's type of a math slot (-1: the result), a model key in the PDB's names, from the SDK's header
	// declaration (symbols.tsv's `sse`): the libraries are the FPU build
	std::map<int, std::string> sse;
	std::vector<std::string> cvNames;  // the parameters' names in the library's CodeView (symbols.tsv's `names`)
};

std::string trimmed(const std::string& s)
{
	size_t b = s.find_first_not_of(" \t");
	if (b == std::string::npos)
		return "";
	return s.substr(b, s.find_last_not_of(" \t") + 1 - b);
}

// the text's lines, without their newlines
std::vector<std::string> lineList(const std::string& text)
{
	std::vector<std::string> out;
	for (size_t from = 0; from < text.size();)
	{
		size_t end = text.find('\n', from);
		out.push_back(text.substr(from, end == std::string::npos ? std::string::npos : end - from));
		from = end == std::string::npos ? text.size() : end + 1;
	}
	return out;
}

// the top-level pieces of a list (no `<>`, `()` or `[]` open), split at sep
std::vector<std::string> splitTop(const std::string& text, char sep)
{
	std::vector<std::string> out;
	int depth = 0;
	size_t start = 0;
	for (size_t k = 0; k <= text.size(); ++k)
	{
		char c = k < text.size() ? text[k] : sep;
		if (c == '<' || c == '(' || c == '[')
			++depth;
		else if (c == '>' || c == ')' || c == ']')
			--depth;
		else if (c == sep && depth == 0)
		{
			out.push_back(trimmed(text.substr(start, k - start)));
			start = k + 1;
		}
	}
	return out;
}

// The symbol of a demangled text: `[access: ][static |virtual ]RET CC SCOPE::NAME(PARAMS)[ const]`; why says what
// is not read
std::optional<Symbol> parseSymbol(Symbol s, std::string& why)
{
	std::string text = s.demangled;
	if (text.find('(') == std::string::npos)
	{
		// an extern "C" name: `_name` (cdecl) or `_name@N` (stdcall)
		std::string name = s.mangled;
		if (name.starts_with("_"))
			name = name.substr(1);
		if (size_t at = name.find('@'); at != std::string::npos)
			name = name.substr(0, at);
		if (!identifier(name))
		{
			why = "a C name C++ cannot spell: " + s.mangled;
			return std::nullopt;
		}
		s.c = true;
		s.name = name;
		return s;
	}
	for (const char* access : {"public: ", "protected: ", "private: "})
		if (text.starts_with(access))
		{
			s.access = std::string(access).substr(0, std::string(access).size() - 2);
			text = text.substr(std::string(access).size());
		}
	if (text.starts_with("static "))
		s.isStatic = true, text = text.substr(7);
	if (text.starts_with("virtual "))
		s.isVirtual = true, text = text.substr(8);
	size_t at = std::string::npos, length = 0;
	int depth = 0;
	for (size_t k = 0; k < text.size() && at == std::string::npos; ++k)
	{
		char c = text[k];
		if (c == '<' || c == '(')
			++depth;
		else if (c == '>' || c == ')')
			--depth;
		else if (depth == 0 && (k == 0 || text[k - 1] == ' '))
			for (const char* cc : {"__thiscall ", "__cdecl ", "__stdcall ", "__fastcall "})
				if (text.compare(k, std::string(cc).size(), cc) == 0)
				{
					at = k, length = std::string(cc).size();
					s.cc = std::string(cc).substr(2, std::string(cc).size() - 3);
				}
	}
	if (at == std::string::npos)
	{
		why = "no calling convention in the symbol";
		return std::nullopt;
	}
	s.ret = trimmed(text.substr(0, at));
	std::string rest = text.substr(at + length);
	if (std::regex_search(rest, std::regex(R"(\boperator\b)")))
	{
		why = "an operator";
		return std::nullopt;
	}
	size_t open = std::string::npos;
	depth = 0;
	for (size_t k = 0; k < rest.size(); ++k)
	{
		if (rest[k] == '<')
			++depth;
		else if (rest[k] == '>')
			--depth;
		else if (rest[k] == '(' && depth == 0)
		{
			open = k;
			break;
		}
	}
	if (open == std::string::npos)
	{
		why = "no parameter list";
		return std::nullopt;
	}
	std::string qualified = rest.substr(0, open);
	if (!qualified.empty() && qualified.back() == '>')
	{
		why = "a function template's instance";
		return std::nullopt;
	}
	if (qualified.find("`anonymous namespace'") != std::string::npos)
	{
		why = "a file-local function (anonymous namespace)";
		return std::nullopt;
	}
	depth = 0;
	size_t close = std::string::npos;
	for (size_t k = open; k < rest.size(); ++k)
	{
		depth += rest[k] == '(' ? 1 : rest[k] == ')' ? -1 : 0;
		if (depth == 0)
		{
			close = k;
			break;
		}
	}
	if (close == std::string::npos)
	{
		why = "an unclosed parameter list";
		return std::nullopt;
	}
	std::string params = rest.substr(open + 1, close - open - 1);
	std::string tail = trimmed(rest.substr(close + 1));
	s.isConst = tail == "const" || tail.starts_with("const ");
	if (params != "void" && !params.empty())
		s.params = splitTop(params, ',');
	if (!s.params.empty() && s.params.back() == "...")
		s.variadic = true, s.params.pop_back();
	s.name = lastComponent(qualified);
	s.scope = scopeOf(qualified);
	if (s.name.starts_with("`"))
	{
		if (s.name == "`scalar deleting destructor'" || s.name == "`vector deleting destructor'" ||
			s.name == "`scalar deleting dtor'" || s.name == "`vector deleting dtor'")
			s.structor = "deleting";
		else
		{
			why = "a compiler function: " + s.name;
			return std::nullopt;
		}
	}
	else if (s.name.starts_with("~"))
		s.structor = "dtor";
	else if (!s.scope.empty() && !s.access.empty() && s.ret.empty() && s.name == lastComponent(s.scope))
		s.structor = "ctor";
	if (s.structor.empty() && s.ret.empty())
	{
		why = "no result type";
		return std::nullopt;
	}
	return s;
}

// A demangled type as a model key in the PDB's names (PdbTypes::toSrc maps it to src's): `class X const &` is
// `lref(const(record:X))`; nullopt with why for a form not read (a member pointer)
std::optional<std::string> typeKey(std::string text, std::string& why)
{
	text = trimmed(text);
	static const std::regex functionPointer(
		R"(^(.*)\((__cdecl|__stdcall|__thiscall|__fastcall) ?\* ?(const)?\)\((.*)\)$)");
	static const std::regex arrayReference(R"(^(.*) \((&|\*|\* ?const)\)\[(\d+)\]$)");
	std::smatch m;
	if (std::regex_match(text, m, functionPointer))
	{
		SrcTypes::Function f;
		f.cc = m[2].str().substr(2);
		std::optional<std::string> ret = typeKey(m[1].str(), why);
		if (!ret)
			return std::nullopt;
		f.ret = *ret;
		std::string list = m[4].str();
		if (list != "void" && !list.empty())
			for (const std::string& p : splitTop(list, ','))
			{
				if (p == "...")
				{
					f.variadic = true;
					continue;
				}
				std::optional<std::string> k = typeKey(p, why);
				if (!k)
					return std::nullopt;
				f.params.push_back(*k);
			}
		std::string key = "ptr(" + SrcTypes::functionKey(f) + ")";
		return m[3].matched ? "const(" + key + ")" : key;
	}
	if (std::regex_match(text, m, arrayReference))
	{
		std::optional<std::string> element = typeKey(m[1].str(), why);
		if (!element)
			return std::nullopt;
		std::string kind = m[2].str() == "&" ? "lref" : "ptr";
		std::string key = kind + "(array(" + *element + "," + m[3].str() + "))";
		return m[2].str().find("const") != std::string::npos ? "const(" + key + ")" : key;
	}
	if (text.find("::*") != std::string::npos)
	{
		why = "a member pointer: " + text;
		return std::nullopt;
	}
	// the modifiers after the base type, read from the right, applied from the left
	std::vector<std::string> modifiers;
	for (bool more = true; more;)
	{
		more = false;
		text = trimmed(text);
		for (const char* m : {"&&", "&", "*", "const", "volatile", "__ptr64", "__restrict"})
		{
			std::string suffix = m;
			if (text.size() <= suffix.size() || !text.ends_with(suffix))
				continue;
			// a cv word or pointer qualifier only as a word of its own (after a blank, `*` or `&`)
			char before = text[text.size() - suffix.size() - 1];
			if (identifierChar(suffix[0]) && before != ' ' && before != '*' && before != '&')
				continue;
			if (suffix != "__ptr64" && suffix != "__restrict")
				modifiers.push_back(suffix);
			text = text.substr(0, text.size() - suffix.size());
			more = true;
			break;
		}
	}
	text = trimmed(text);
	std::string key;
	static const std::map<std::string, std::string> builtins = {
		{"void", "void"},
		{"bool", "bool"},
		{"char", "char"},
		{"signed char", "signed char"},
		{"unsigned char", "unsigned char"},
		{"short", "short"},
		{"unsigned short", "unsigned short"},
		{"int", "int"},
		{"unsigned int", "unsigned int"},
		{"__int64", "long long"},
		{"unsigned __int64", "unsigned long long"},
		{"float", "float"},
		{"double", "double"},
		{"long double", "long double"},
		{"wchar_t", "wchar_t"},
	};
	// Havok's long and unsigned long are its pointer-sized integers (hkBaseTypes.h: hkLong, hkUlong), src's typedefs
	// of them (RULES: what holds an address is a pointer-sized integer)
	if (text == "long" || text == "unsigned long")
		key = text == "long" ? "typedef:hkLong" : "typedef:hkUlong";
	else if (auto b = builtins.find(text); b != builtins.end())
		key = "builtin:" + b->second;
	else if (text.starts_with("class ") || text.starts_with("struct ") || text.starts_with("union "))
		key = "record:" + text.substr(text.find(' ') + 1);
	else if (text.starts_with("enum "))
		key = "enum:" + text.substr(5);
	else
	{
		why = "a type the symbol spells without its kind: " + text;
		return std::nullopt;
	}
	for (auto k = modifiers.rbegin(); k != modifiers.rend(); ++k)
	{
		if (*k == "const")
			key = "const(" + key + ")";
		else if (*k == "volatile")
			key = "volatile(" + key + ")";
		else if (*k == "*")
			key = "ptr(" + key + ")";
		else if (*k == "&")
			key = "lref(" + key + ")";
		else if (*k == "&&")
			key = "rref(" + key + ")";
	}
	return key;
}

// The parameter names the Havok SDK's headers give a function: its declarations in the headers named after its
// class (the outermost) and after its object file, each of the symbol's parameter count; nullopt where none is, or
// where two disagree
class SdkNames
{
public:
	explicit SdkNames(const std::string& root)
	{
		if (!std::filesystem::exists(root))
			return;
		for (const auto& entry : std::filesystem::recursive_directory_iterator(root))
			if (entry.is_regular_file() && (entry.path().extension() == ".h" || entry.path().extension() == ".inl"))
				m_files[entry.path().stem().string()].push_back(entry.path().string());
	}

	std::optional<std::vector<std::string>> names(const Symbol& s)
	{
		std::set<std::string> stems;
		if (!s.scope.empty())
			stems.insert(s.scope.substr(0, s.scope.find("::")).substr(0, s.scope.find('<')));
		if (!s.object.empty())
			stems.insert(s.object.substr(0, s.object.find('.')));
		std::optional<std::vector<std::string>> found;
		std::regex call("\\b" + s.name + "\\s*\\(");
		for (const std::string& stem : stems)
		{
			auto files = m_files.find(stem);
			if (files == m_files.end())
				continue;
			for (const std::string& path : files->second)
			{
				const std::string& text = this->text(path);
				for (auto it = std::sregex_iterator(text.begin(), text.end(), call); it != std::sregex_iterator(); ++it)
				{
					size_t open = size_t(it->position(0) + it->length(0) - 1);
					// a call through an object is no declaration
					size_t before = size_t(it->position(0));
					if (before > 0 && (text[before - 1] == '.' || text[before - 1] == '>'))
						continue;
					int depth = 0;
					size_t close = std::string::npos;
					for (size_t k = open; k < text.size(); ++k)
					{
						depth += text[k] == '(' ? 1 : text[k] == ')' ? -1 : 0;
						if (depth == 0)
						{
							close = k;
							break;
						}
					}
					if (close == std::string::npos)
						continue;
					std::string list = text.substr(open + 1, close - open - 1);
					std::vector<std::string> params;
					if (trimmed(list) != "void" && !trimmed(list).empty())
						params = splitTop(list, ',');
					if (params.size() != s.params.size())
						continue;
					std::vector<std::string> names;
					for (std::string p : params)
					{
						p = p.substr(0, p.find('='));
						p = trimmed(p.substr(0, p.find('[')));
						size_t e = p.size();
						size_t b = e;
						while (b > 0 && (std::isalnum(static_cast<unsigned char>(p[b - 1])) || p[b - 1] == '_'))
							--b;
						std::string name = p.substr(b, e - b);
						std::string type = trimmed(p.substr(0, b));
						bool upper = std::none_of(name.begin(), name.end(),
												  [](char c) { return std::islower(static_cast<unsigned char>(c)); });
						if (type.empty() || type == "const" || upper || !identifier(name))
							name.clear();
						names.push_back(name);
					}
					if (found && *found != names)
						return std::nullopt;
					found = names;
				}
			}
		}
		return found;
	}

private:
	const std::string& text(const std::string& path)
	{
		auto found = m_texts.find(path);
		if (found == m_texts.end())
		{
			std::string t = readFile(path);
			// comments out, so a commented-out declaration says nothing
			t = std::regex_replace(t, std::regex(R"(//[^\n]*)"), "");
			found = m_texts.emplace(path, t).first;
		}
		return found->second;
	}

	std::unordered_map<std::string, std::vector<std::string>> m_files;  // stem -> paths
	std::unordered_map<std::string, std::string> m_texts;
};

// One address's row of hk-declare.tsv, and what --apply writes for it
struct Row
{
	uint64_t ea = 0;
	Symbol symbol;
	const SrcModel::Function* function = nullptr;
	std::string src, verdict, changes, reason, needs, declaration;
	Decl decl;			  // the new declaration (outcome `declared`)
	bool move = false;	  // to another class (or out of one)
	bool comment = true;  // the signature line goes above it (not for a C name)
	std::string oldName;  // src's last name component
	std::string newQualified;
	const SrcModel::Function* merge = nullptr;	// src's bodiless declaration of it, which gets the forwarder
	std::string hidden;							// a derived class's member of the new name
	std::string provenance;						// src's declaration's (srcdb's provenance)
	std::string detail;							// what differs, slot by slot
	std::string result;	 // the symbol's class result, which the exe returns through the hidden result pointer
	bool handBody = false;	// a registered body src keeps: only its name changes
	bool nameOnly = false;	// src's declaration kept but its name (a C name, a member src has as a free function)
	bool qualify = false;	// a static member src has as a free function: its uses spelled with its class

	void open(const std::string& why)
	{
		if (verdict == "open")
			return;
		verdict = "open";
		reason = why;
	}
};

bool headerFile(const std::string& path)
{
	return path.ends_with(".h") || path.ends_with(".hpp") || path.ends_with(".inl");
}

std::string stemOf(const std::string& path)
{
	size_t dot = path.rfind('.');
	return dot == std::string::npos ? path : path.substr(0, dot);
}

// A comment line above src's declaration without the provenance markers the new declaration's comment replaces: a
// marker phrase goes and the rest of its line stays (a comment going on to the next lines stays whole); a line of
// the Havok SDK's evidence or an `// Inferred type` or `// Guessed type` clause goes whole. Empty: nothing of the
// line stays.
std::string withoutMarkers(const std::string& line)
{
	std::string t = trimmed(line);
	if (!t.starts_with("//"))
		return t;
	if (t.find("Havok SDK ") != std::string::npos || t.find("Inferred type") != std::string::npos ||
		t.find("Guessed type") != std::string::npos)
		return "";
	std::string text = trimmed(t.substr(2));
	for (const char* phrase : {"Not reconstructed yet.", "IDA name; not in the PDB.",
							   "Not named in the PDB or IDA; name chosen here.", "Not named in the PDB or IDA."})
		if (size_t at = text.find(phrase); at != std::string::npos)
			text = trimmed(text.erase(at, std::string(phrase).size()));
	return text.empty() ? "" : "// " + text;
}

// The comment lines [from, to) above a declaration that stay, each withoutMarkers(): an `// Inferred type` or
// `// Guessed type` clause with its further lines (up to the next marker line, srcdb's reading) goes, unless types (src's
// types stay: the clause still holds), when it stays as it is
std::vector<std::string> keptComments(const std::vector<std::string>& lines, int64_t from, int64_t to, bool types)
{
	std::vector<std::string> out;
	bool clause = false;
	for (int64_t k = from; k < to; ++k)
	{
		std::string t = trimmed(lines[k - 1]);
		std::string text = t.starts_with("//") ? trimmed(t.substr(2)) : "";
		bool marker = text.starts_with("Inferred type") || text.starts_with("Guessed type") ||
					  text.starts_with("PDB:") || text.starts_with("Havok SDK ") ||
					  text.find("not in the PDB.") != std::string::npos ||
					  text.find("Not named in the PDB or IDA") != std::string::npos ||
					  text.find("Not reconstructed yet.") != std::string::npos;
		if (marker)
			clause = text.starts_with("Inferred type") || text.starts_with("Guessed type");
		if (clause)
		{
			if (types)
				out.push_back(t);
			continue;
		}
		if (std::string rest = withoutMarkers(lines[k - 1]); !rest.empty())
			out.push_back(rest);
	}
	return out;
}

// The first place word stands in line as a whole identifier; npos where it does not
size_t wordAt(const std::string& line, const std::string& word)
{
	for (size_t at = line.find(word); at != std::string::npos; at = line.find(word, at + 1))
		if ((at == 0 || !identifierChar(line[at - 1])) &&
			(at + word.size() == line.size() || !identifierChar(line[at + word.size()])))
			return at;
	return std::string::npos;
}

// The Havok pointer-sized typedefs a key spells, in order: normal() takes hkUlong and hkLong for uint32_t and
// int32_t, but a slot spelled otherwise changes (RULES: a pointer-sized integer)
std::string hkIntegers(const std::string& key)
{
	static const std::regex word(R"(typedef:(hkUlong|hkLong)\b)");
	std::string out;
	for (auto it = std::sregex_iterator(key.begin(), key.end(), word); it != std::sregex_iterator(); ++it)
		out += (*it)[1].str() + ",";
	return out;
}

// A line's width at tab width 4
size_t columns(const std::string& line)
{
	size_t n = 0;
	for (char c : line)
		n = c == '\t' ? n / 4 * 4 + 4 : n + 1;
	return n;
}

// A whole-line `//` comment re-wrapped at 120 columns (tab width 4): its words filled greedily, each new line with
// the first's indent and `// `, a list item's continuation indented under its text
std::string rewrapped(const std::string& line)
{
	size_t text = line.find_first_not_of(" \t");
	std::string indent = line.substr(0, text);
	std::string body = trimmed(line.substr(text + 2));
	std::string first = indent + "// ", next = first + (body.starts_with("- ") ? "  " : "");
	std::vector<std::string> words;
	for (size_t k = 0; k < body.size();)
	{
		size_t e = body.find(' ', k);
		if (e == std::string::npos)
			e = body.size();
		if (e > k)
			words.push_back(body.substr(k, e - k));
		k = e + 1;
	}
	std::string out, current = first;
	bool empty = true;
	for (const std::string& w : words)
	{
		if (!empty && columns(current + " " + w) > 120)
		{
			out += current + "\n";
			current = next, empty = true;
		}
		current += (empty ? "" : " ") + w;
		empty = false;
	}
	return out + current;
}

class Havok : private SlotRowChecks
{
public:
	Havok(const SrcModel& model, SrcTypes& types, exefacts::Facts& facts, const PdbSources& sources)
		: SlotRowChecks(model, types, facts), m_forwarders(types), m_sdk("resources/havok/source/Source"),
		  m_sources(sources)
	{
	}

	std::vector<Row> rows;

	void read(const std::string& symbols, const std::vector<uint64_t>& only);
	void judgeAll();
	// the apply and same rows written into edits; reemit and renames collected
	void write(FileEdits& edits, std::map<uint64_t, std::string>& outcomes, std::set<uint64_t>& reemit);

private:
	void judge(Row& r);
	void judgeC(Row& r);
	// src's declaration kept but its name, the symbol's (reason: why src's types stay)
	void judgeName(Row& r, const std::string& reason);
	std::optional<std::string> key(const std::string& text, std::string& why) const;
	std::string contradiction(const Row& r) const;
	void families();
	void clashes();
	void uses();
	const SrcModel::Record* recordOf(const std::string& name) const;
	// the first slot whose new type a call converts silently from or to src's (an arithmetic slot of another kind
	// or width, an arithmetic result of another type); "" for none
	std::string silentConversion(const Row& r) const;
	std::string describe(const SrcModel::Function& f) const;
	// the lines above the declaration: `// Not reconstructed yet.` where its definition is a forwarder, the name's
	// marker, the signature line
	std::string comment(const Row& r, bool forwarder) const;
	// `// Havok SDK signature (<lib>, tied by <evidence>)`, with its SSE math types and its hidden result pointer
	std::string signatureLine(const Row& r) const;
	// the name's marker: the Havok SDK's where the game PDB has no function of pdbName, else none
	std::string nameMarker(const Symbol& s, const std::string& pdbName) const;
	// a parameter the comment kept above the declaration names (`aN`, `argN`, src's own name) that the new
	// declaration no longer has; "" for none
	std::string staleParameter(const Row& r, const std::vector<std::string>& lines) const;
	// where a moved function is declared and its forwarder defined (SrcPlace's rule)
	void place(const Row& r, Decl& d) const;
	// the declaration's lines: its first (the head) and last (its `;`), its comment run's top
	struct Lines
	{
		int64_t top = 0, first = 0, last = 0;
	};
	std::optional<Lines> declarationLines(const std::vector<std::string>& lines, const SrcModel::Function& f) const;
	// the definition's lines to replace: a forwarder's, or an emitted block's whole; why where neither
	std::optional<LineRange> definitionRange(const Row& r, FileEdits& edits, bool& emitted, std::string& why) const;
	// whether a src line is inside an emitted block
	bool inEmitted(const std::string& file, int64_t line) const;

	Forwarders m_forwarders;
	SdkNames m_sdk;
	const PdbSources& m_sources;
	mutable std::optional<std::map<std::string, std::vector<std::string>>> m_headers;  // src's headers by stem
	// per row (by address) the uses of its old name to rewrite: (file, line) -> (col, old, new)
	std::map<uint64_t, std::vector<SrcModel::Use>> m_uses;
	std::map<uint64_t, std::vector<std::pair<std::string, int64_t>>> m_folds;  // fold comment lines
	std::map<uint64_t, std::set<uint64_t>> m_emittedCallers;  // the emitted bodies naming it
	mutable std::map<std::string, std::vector<std::string>> m_fileLines;  // inEmitted's src files, by path
};

const SrcModel::Record* Havok::recordOf(const std::string& name) const
{
	if (const SrcModel::Record* r = m_model.record(name))
		return r;
	if (auto found = m_records.find(comparedName(name)); found != m_records.end())
		return m_model.record(found->second);
	return nullptr;
}

bool Havok::inEmitted(const std::string& file, int64_t line) const
{
	auto found = m_fileLines.find(file);
	if (found == m_fileLines.end())
		found = m_fileLines.emplace(file, Text(readFile(file)).lines()).first;
	return emittedBlock(found->second, line).has_value();
}

std::optional<std::string> Havok::key(const std::string& text, std::string& why) const
{
	std::optional<std::string> k = typeKey(text, why);
	if (!k)
		return std::nullopt;
	std::string missing;
	std::optional<std::string> s = toSrc(*k, missing);
	if (!s)
		why = "type not in src: " + std::regex_replace(missing, std::regex(R"(\b(class|struct|union|enum) )"), "");
	return s;
}

std::string Havok::describe(const SrcModel::Function& f) const
{
	std::optional<SrcTypes::Function> t = SrcTypes::function(f.type);
	if (!t)
		return f.name;
	std::string params;
	for (const std::string& p : t->params)
		params += (params.empty() ? "" : ", ") + spelled(p);
	return std::string(f.isStatic ? "static " : f.isVirtual ? "virtual " : "") + spelled(t->ret) + " " + f.name + "(" +
		   params + ")" + (t->isConst ? " const" : "") + " [" + t->cc + "]";
}

void Havok::read(const std::string& symbols, const std::vector<uint64_t>& only)
{
	std::ifstream in(symbols);
	if (!in)
		throw Error("declare --hk-symbols: cannot read " + symbols);
	std::string line;
	std::getline(in, line);
	std::vector<std::string> header = cells(line);
	auto column = [&](const std::string& name) {
		auto found = std::find(header.begin(), header.end(), name);
		if (found == header.end())
			throw Error("declare --hk-symbols: " + symbols + " has no column " + name);
		return size_t(found - header.begin());
	};
	size_t address = column("address"), lib = column("lib"), object = column("object"), mangled = column("mangled"),
		   demangled = column("demangled"), evidence = column("evidence"), mark = column("mark"), sse = column("sse"),
		   names = column("names");
	std::map<uint64_t, std::vector<std::vector<std::string>>> byAddress;
	while (std::getline(in, line))
	{
		std::vector<std::string> c = cells(line);
		if (c.size() <= std::max({address, lib, object, mangled, demangled, evidence, mark, sse, names}))
			continue;
		byAddress[std::stoull(c[address], nullptr, 16)].push_back(c);
	}
	std::set<uint64_t> wanted(only.begin(), only.end());
	for (const auto& [ea, list] : byAddress)
	{
		if (list.size() != 1 || !list[0][mark].empty() || (!wanted.empty() && !wanted.count(ea)))
			continue;
		const std::vector<std::string>& c = list[0];
		Row& r = rows.emplace_back();
		r.ea = ea;
		r.symbol.lib = c[lib];
		r.symbol.object = c[object].substr(0, c[object].find(' '));
		r.symbol.mangled = c[mangled];
		r.symbol.demangled = c[demangled];
		r.symbol.evidence = c[evidence];
		// `k=KEY` per math slot (-1: the result), `;` between
		for (const std::string& slot : splitTop(c[sse], ';'))
			if (size_t at = slot.find('='); at != std::string::npos)
				r.symbol.sse[std::stoi(slot.substr(0, at))] = slot.substr(at + 1);
		if (!c[names].empty())
			r.symbol.cvNames = splitTop(c[names], '|');
	}
	for (uint64_t ea : wanted)
		if (!byAddress.count(ea))
			std::fprintf(stderr, "idakit declare --hk-symbols: %s is not in %s\n", hexText(ea).c_str(),
						 symbols.c_str());
}

// The exe's facts against the new declaration's ABI: its stack bytes are what the exe pops (thiscall, stdcall; a
// cdecl none), ecx and edx are read only where the convention passes them, and no argument slot is read past them
std::string Havok::contradiction(const Row& r) const
{
	const Decl& d = r.decl;
	uint32_t ea = uint32_t(r.ea);
	int64_t bytes = d.indirect ? 4 : 0;
	size_t registers = d.cc == "thiscall" && d.isStatic ? 1 : 0;
	size_t k = 0;
	for (const Param& p : d.params)
	{
		std::optional<int64_t> n = size(p.key);
		if (!n)
			return "the size of " + spelled(p.key) + " is not known";
		bool inRegister = k < registers;
		// fastcall: the first two of four bytes or fewer that are no float or record go in ecx and edx
		if (d.cc == "fastcall" && registers < 2 && *n <= 4 && kindOf(p.key) != "float" && kindOf(p.key) != "record")
			inRegister = true, ++registers;
		if (!inRegister)
			bytes += (*n + 3) / 4 * 4;
		++k;
	}
	std::optional<uint32_t> pops;
	const exefacts::Abi* abi = nullptr;
	try
	{
		pops = m_facts.pops(ea);
		abi = &m_facts.abi(ea);
	}
	catch (const std::exception& e)
	{
		return std::string("exefacts: ") + e.what();
	}
	if (pops && (d.cc == "thiscall" || d.cc == "stdcall" || d.cc == "fastcall") && int64_t(*pops) != bytes)
		return "pops " + std::to_string(*pops) + " bytes, the symbol's arguments are " + std::to_string(bytes);
	if (pops && d.cc == "cdecl" && *pops != 0)
		return "pops " + std::to_string(*pops) + " bytes, a cdecl none";
	// a register no convention passes (eax, ebx, esi, edi read before written): the exe's code takes a usercall's
	try
	{
		exefacts::Families inputs = m_facts.regArgs(ea);
		for (auto [family, name] : {std::pair{exefacts::kEax, "eax"}, {exefacts::kEbx, "ebx"}, {exefacts::kEsi, "esi"},
									{exefacts::kEdi, "edi"}})
			if (inputs & family)
				return std::string("reads ") + name + ", which no convention passes (a usercall)";
	}
	catch (const std::exception& e)
	{
		return std::string("exefacts: ") + e.what();
	}
	bool ecx = d.cc == "thiscall" || d.cc == "fastcall";
	if (abi && abi->ecx && !ecx)
		return "reads ecx, which a " + d.cc + " does not pass";
	if (abi && abi->edx && d.cc != "fastcall")
		return "reads edx, which a " + d.cc + " does not pass";
	if (abi && !d.cc.empty())
		for (int32_t slot : abi->args)
			if (int64_t(slot) * 4 + 4 > bytes && !r.symbol.variadic)
				return "reads its argument bytes past the symbol's " + std::to_string(bytes) + " (slot " +
					   std::to_string(slot) + ")";
	return "";
}

void Havok::judgeC(Row& r)
{
	if (!r.function->cls.empty())
		return r.open("a C function src declares on a class (" + r.function->cls + ")");
	judgeName(r, "a C name: src's types stay");
}

void Havok::judgeName(Row& r, const std::string& reason)
{
	const SrcModel::Function& f = *r.function;
	try
	{
		srcDeclaration(r.decl, m_model, f, f.cls.empty() || f.isStatic ? "" : "this");
	}
	catch (const Skip& s)
	{
		return r.open(s.reason);
	}
	r.decl.ea = r.ea;
	r.decl.outcome = "declared";
	r.decl.member = r.symbol.name;
	r.comment = false;
	r.nameOnly = true;
	r.newQualified = (scopeOf(f.name).empty() ? "" : scopeOf(f.name) + "::") + r.symbol.name;
	// the name src's declaration takes where it is: a guess's marker unless the game PDB has a function of it
	r.decl.marker = nameMarker(r.symbol, r.newQualified);
	// written as a new declaration's text, so that its convention is spelled as src's free functions spell it
	// (FNVD_CDECL); the header's declaration keeps its own text but the name (write())
	r.decl.exact = false;
	static const std::regex placeholderName(R"((_?sub_)?[0-9A-Fa-f]{6,8})");
	if (r.oldName == r.symbol.name)
	{
		if (!r.symbol.c)
			return r.open(reason);
		r.verdict = "same";
		r.changes = "evidence";
		r.reason = reason;
		return;
	}
	if (f.provenance != SrcModel::Provenance::Guess && !r.symbol.c)
	{
		r.verdict = "disagree";
		r.reason = r.provenance + " declaration differs";
		r.changes = "name";
		r.detail = "name " + r.oldName + " -> " + r.symbol.name + "; " + reason;
		return;
	}
	if (f.provenance != SrcModel::Provenance::Guess)
		return r.open("src's own name (not IDA's or chosen): " + r.oldName);
	if (!std::regex_match(r.oldName, placeholderName))
		return r.open("src's IDA or chosen name differs (no placeholder): " + r.oldName + " -> " + r.symbol.name);
	// a member's name as a free function: no free src function of that name in any namespace (a C library's of src's
	// msvc9 among them), so that no call finds another overload
	if (!r.symbol.c && f.cls.empty())
		for (const auto& [mangled, other] : m_model.functions())
			if (other.cls.empty() && lastComponent(other.name) == r.symbol.name)
				return r.open("name taken: src has " + other.name);
	// nor a function of the game PDB's (the CRT's printf, a game function) by that bare name
	if (!r.symbol.c && f.cls.empty() && m_model.pdbFunctions().count(r.symbol.name))
		return r.open("name taken: the game PDB has a function " + r.symbol.name);
	r.verdict = "apply";
	r.changes = "name";
	r.reason = reason;
	r.detail = "name " + r.oldName + " -> " + r.symbol.name;
}

void Havok::judge(Row& r)
{
	std::string why;
	std::optional<Symbol> parsed = parseSymbol(r.symbol, why);
	if (!parsed)
		return r.open("symbol: " + why);
	r.symbol = *parsed;
	const Symbol& s = r.symbol;
	auto declaring = m_declaring.find(r.ea);
	if (declaring == m_declaring.end() || declaring->second.empty())
		return r.open("src declares nothing at it");
	if (declaring->second.size() > 1)
		return r.open("fold: src names " + std::to_string(declaring->second.size()) + " functions at it");
	r.function = m_model.function(declaring->second[0]);
	if (!r.function)
		return r.open("src's declaration is not in the model");
	const SrcModel::Function& f = *r.function;
	r.src = describe(f);
	r.oldName = lastComponent(f.name);
	r.provenance = f.provenance == SrcModel::Provenance::Pdb			? "pdb"
				   : f.provenance == SrcModel::Provenance::Src		? "src"
				   : f.provenance == SrcModel::Provenance::Inferred ? "inferred"
				   : f.provenance == SrcModel::Provenance::Library	? "library"
																	: "guess";
	if (s.c)
	{
		if (f.provenance == SrcModel::Provenance::Pdb)
		{
			r.verdict = "pdb";
			r.reason = "the game PDB's declaration";
			return;
		}
		return judgeC(r);
	}
	const SrcModel::Record* srcOwner = f.cls.empty() ? nullptr : m_model.record(f.cls);
	bool member = !s.access.empty();
	const SrcModel::Record* owner = nullptr;
	Decl& d = r.decl;
	d.ea = r.ea;
	d.outcome = "declared";
	d.noreturn = f.noreturn;
	if (member)
	{
		owner = recordOf(s.scope);
		auto usable = [](const SrcModel::Record* x) {
			return x && x->pattern.empty() && x->name.find('<') == std::string::npos &&
				   x->file.rfind("src/", 0) == 0 && x->line > 0 && x->endLine > 0;
		};
		if (srcOwner && !usable(srcOwner))
			return r.open("src's class is a class template instance or has no definition in src: " + srcOwner->name);
		// a use inside an emitted block is the emitter's (re-emitted, its block forwarded where the change breaks it):
		// it names no place
		std::vector<SrcModel::Use> uses = m_model.uses(f.mangled);
		bool used = std::any_of(uses.begin(), uses.end(),
								[&](const SrcModel::Use& u) { return !inEmitted(u.file, u.line); }) ||
					std::any_of(m_model.refsAt(r.ea).begin(), m_model.refsAt(r.ea).end(),
								[](const SrcModel::Ref& x) { return x.kind == "fold"; });
		const SrcModel::Definition* body = m_model.definition(f.mangled);
		bool kept = body && !body->emitted && (m_model.registers(r.ea) || m_model.handPorted(r.ea));
		// the symbol's types are written where its `this` fits: on its own class, where src has it (a move where src
		// has the function elsewhere and nothing names it there; a static member src has as a global free function,
		// with uses, moves too, each use spelled qualified, `X::f(...)`, the same call)
		if (usable(owner) && owner != srcOwner && f.cls.empty() && s.structor.empty() && used && s.isStatic &&
			scopeOf(f.name).empty() && r.oldName != s.name && !kept)
			r.qualify = true;
		else if (!usable(owner) || (owner != srcOwner && (used || kept)))
		{
			// elsewhere src's declaration keeps its place and types under the SDK's name
			if (!s.structor.empty())
				return r.open("needs class: " + s.scope + " (a " + s.structor + ")");
			// src's own declaration of the symbol's name (the game PDB's, without a body) is that function: a merge
			// question, not a second name
			if (const std::vector<const SrcModel::Function*> named = m_model.functionsNamed(s.scope + "::" + s.name);
				!named.empty())
				return r.open("same name: src declares " + named[0]->name);
			std::string place = !owner			 ? "needs class " + s.scope
								: !usable(owner) ? "a class template instance's member"
								: kept			 ? std::string("a registered body")
												 : "a move to " + owner->name + " with uses";
			return judgeName(r, place + ": keeps its place under the new name");
		}
	}
	else if (!s.scope.empty())
	{
		// a namespace's function goes in that namespace (a move where src has it elsewhere); a scope src has as a
		// class is an owner question
		if (recordOf(s.scope))
			return r.open("namespace is a class in src: " + s.scope);
		if (!identifier(lastComponent(s.scope)))
			return r.open("namespace is no identifier: " + s.scope);
		d.scope = s.scope;
	}
	if (s.isVirtual && s.structor.empty() && !f.isVirtual)
		return r.open("the symbol is virtual, src's declaration is not");
	if (f.isVirtual && !(s.isVirtual && s.structor.empty()))
		return r.open("src's declaration is virtual, the symbol is not");
	r.move = owner != srcOwner || (!owner && scopeOf(f.name) != d.scope);
	if (r.move && f.isVirtual)
		return r.open("a virtual on another class (" + (owner ? owner->name : std::string("none")) + ")");
	d.owner = owner;
	d.self = owner && !s.isStatic ? "this" : "";
	d.isStatic = owner && s.isStatic;
	d.isConst = s.isConst;
	d.cc = s.cc;
	d.member = s.name;
	d.marker = "Havok SDK name; not in the PDB.";
	// the types; a math slot the SSE build's (the SDK header's declaration), not the FPU libraries'
	auto slotKey = [&](int slot, const std::string& text) -> std::optional<std::string> {
		auto sse = s.sse.find(slot);
		if (sse == s.sse.end())
			return key(text, why);
		std::string missing;
		std::optional<std::string> k = toSrc(sse->second, missing);
		if (!k)
			why = "type not in src: " + std::regex_replace(missing, std::regex(R"(\b(class|struct|union|enum) )"), "");
		return k;
	};
	std::vector<std::string> keys;
	for (size_t k = 0; k < s.params.size(); ++k)
	{
		std::optional<std::string> key = slotKey(int(k), s.params[k]);
		if (!key)
			return r.open(why);
		keys.push_back(*key);
	}
	std::optional<std::string> ret =
		s.structor.empty() ? slotKey(-1, s.ret) : std::optional<std::string>("builtin:void");
	if (!ret)
		return r.open(why);
	d.ret = *ret;
	// a record returned by value: MSVC's hidden result pointer for every member function, and for a free one but a
	// trivially copied record of 1, 2, 4 or 8 bytes
	if (kindOf(d.ret) == "record")
	{
		const SrcModel::Record* rr = m_model.record(m_types.parts(resolved(d.ret)).name);
		std::optional<int64_t> n = size(d.ret);
		d.indirect = member || !rr || !rr->trivialCopy || !n || (*n != 1 && *n != 2 && *n != 4 && *n != 8);
	}
	// the names: the SDK headers', else the library's CodeView, else src's where they are no IDA names, else a<k>
	std::optional<std::vector<std::string>> sdk = m_sdk.names(s);
	std::optional<SrcTypes::Function> srcType = SrcTypes::function(f.type);
	static const std::regex idaParameter(R"((a|arg|x|(e?[abcd]x|e?[sd]i|e?bp|[abcd][lh]|xmm|st))\d+[a-z]?)");
	std::set<std::string> taken;
	for (size_t k = 0; k < keys.size(); ++k)
	{
		std::string name = sdk && k < sdk->size() ? (*sdk)[k] : "";
		if (name.empty() && k < s.cvNames.size() && !s.cvNames[k].starts_with("_"))
			name = s.cvNames[k];
		if (name.empty() && srcType && srcType->params.size() == keys.size() && k < f.params.size() &&
			identifier(f.params[k]) && !std::regex_match(f.params[k], idaParameter))
			name = f.params[k];
		if (name.empty() || !identifier(name) || taken.count(name) || name == "this")
			name = "a" + std::to_string(k + 1);
		taken.insert(name);
		d.params.push_back({keys[k], name});
	}
	// a class result: src passes the exe's hidden result pointer explicitly, first after `this`, and returns it
	// (declarations.md "ABI")
	if (d.indirect && s.structor.empty())
	{
		r.result = spelled(d.ret);
		d.ret = "ptr(" + d.ret + ")";
		if (taken.count("apResult"))
			return r.open("a parameter of the SDK's is named apResult");
		d.params.insert(d.params.begin(), {d.ret, "apResult"});
		d.indirect = false;
	}
	// a constructor, destructor or deleting destructor in the porting rules' form
	if (!s.structor.empty())
	{
		if (!owner || r.needs.size())
			return r.open("needs class: " + s.scope + " (a " + s.structor + ")");
		std::string self = "ptr(record:" + owner->name + ")";
		d.isStatic = true;
		d.self.clear();
		d.isConst = false;
		d.indirect = false;
		d.cc = "thiscall";
		if (s.structor == "ctor")
		{
			d.member = "Construct";
			d.ret = self;
			d.params.insert(d.params.begin(), {"ptr(builtin:void)", "apMemory"});
		}
		else if (s.structor == "dtor")
		{
			d.member = "Destruct";
			d.params = {{self, "apThis"}};
		}
		else
		{
			d.member = "DeletingDestruct";
			d.ret = self;
			d.params = {{self, "apThis"}, {"typedef:uint32_t", "auiFlags"}};
		}
		// a second structor of a class's name and parameters (or as many) takes its address in its name (declare's
		// rule)
		for (const SrcModel::Function* other : m_model.functionsNamed(owner->name + "::" + d.member))
		{
			std::optional<SrcTypes::Function> t = SrcTypes::function(other->type);
			bool clash = !t || t->params.size() == d.params.size();
			if (other->mangled != f.mangled && clash)
			{
				d.member += "_" + hex8(r.ea);
				d.marker = "Not named in the PDB or IDA; name chosen here.";
				break;
			}
		}
	}
	r.newQualified = owner ? owner->name + "::" + d.member : (d.scope.empty() ? "" : d.scope + "::") + d.member;
	// a structor's PDB name is the symbol's (X::X), not its porting form's
	if (d.marker.starts_with("Havok SDK name"))
		d.marker = nameMarker(s, s.structor.empty() ? r.newQualified : s.scope + "::" + s.name);
	// the exe's ABI
	if (std::string fact = contradiction(r); !fact.empty())
		return r.open("the exe contradicts: " + fact);
	// a narrower integer or bool than src's where the exe keeps src's width
	if (srcType && s.structor.empty())
	{
		auto narrowed = [&](const std::string& slot, const std::string& srcKey, const std::string& newKey) {
			if (!narrows(srcKey, newKey))
				return std::string();
			std::string unshown;
			return narrowing(f, r.ea, slot, newKey, unshown);
		};
		if (std::string fact = narrowed("ret", srcType->ret, d.ret); !fact.empty())
			return r.open("the exe keeps src's width: " + fact);
		if (srcType->params.size() == d.params.size())
			for (size_t k = 0; k < d.params.size(); ++k)
				if (std::string fact = narrowed(std::to_string(k), srcType->params[k], d.params[k].key); !fact.empty())
					return r.open("the exe keeps src's width: parameter " + std::to_string(k) + " " + fact);
	}
	// what changes
	std::vector<std::string> changes, detail;
	// hkUlong and hkLong are uint32_t and int32_t to normal(); spelled otherwise, a slot changes (RULES' pointer-sized
	// integers)
	auto sameSlot = [&](const std::string& a, const std::string& b) {
		return normal(a) == normal(b) && hkIntegers(a) == hkIntegers(b);
	};
	const SrcModel::Abi* abi = m_model.abi(f.mangled);
	if (!srcType)
		detail.push_back("src's type unknown");
	else
	{
		if (!sameSlot(srcType->ret, d.ret))
			detail.push_back("ret " + spelled(srcType->ret) + " -> " + spelled(d.ret));
		if (srcType->params.size() != d.params.size())
			detail.push_back("count " + std::to_string(srcType->params.size()) + " -> " +
							 std::to_string(d.params.size()));
		else
			for (size_t k = 0; k < d.params.size(); ++k)
				if (!sameSlot(srcType->params[k], d.params[k].key))
					detail.push_back("p" + std::to_string(k) + " " + spelled(srcType->params[k]) + " -> " +
									 spelled(d.params[k].key));
		if (srcType->isConst != d.isConst)
			detail.push_back(std::string("const ") + (d.isConst ? "added" : "removed"));
	}
	if (f.isStatic != d.isStatic)
		detail.push_back(std::string("static ") + (d.isStatic ? "added" : "removed"));
	if (!abi || abi->cc != d.cc)
		detail.push_back("cc " + (abi ? abi->cc : std::string("?")) + " -> " + d.cc);
	if (!detail.empty())
		changes.push_back("types");
	if (r.oldName != d.member)
		changes.push_back("name"), detail.push_back("name " + r.oldName + " -> " + d.member);
	if (r.move)
		changes.push_back("owner"), detail.push_back("owner " + f.cls + " -> " + (owner ? owner->name : d.scope));
	for (const std::string& x : detail)
		r.detail += (r.detail.empty() ? "" : "; ") + x;
	bool renamed = r.oldName != d.member && s.structor.empty();
	bool types = !detail.empty() && changes[0] == "types";
	// IDA's address placeholder: `sub_ADDR`, `_sub_ADDR`, or the bare address
	static const std::regex placeholderName(R"((_?sub_)?[0-9A-Fa-f]{6,8})");
	bool placeholder = std::regex_match(r.oldName, placeholderName);
	if (changes.empty())
	{
		// the game PDB's declaration agrees: nothing to write
		r.verdict = f.provenance == SrcModel::Provenance::Pdb ? "pdb" : "same";
		r.changes = "evidence";
	}
	// a declaration of hand evidence (the game PDB's, src's own) or of other evidence (an `// Inferred type` clause,
	// an earlier Havok SDK signature) is not changed: only a placeholder name of a src or inferred declaration whose
	// types agree takes the SDK's name
	else if (f.provenance != SrcModel::Provenance::Guess &&
			 !(!types && !r.move && placeholder &&
			   (f.provenance == SrcModel::Provenance::Src || f.provenance == SrcModel::Provenance::Inferred)))
	{
		r.verdict = "disagree";
		r.reason = r.provenance + " declaration differs";
		for (const std::string& c : changes)
			r.changes += (r.changes.empty() ? "" : ",") + c;
		return;
	}
	// an IDA or chosen name other than a placeholder stays a question of names, not settled here
	else if (renamed && !placeholder)
		return r.open("src's IDA or chosen name differs (no placeholder): " + r.oldName + " -> " + d.member);
	else
	{
		r.verdict = "apply";
		for (const std::string& c : changes)
			r.changes += (r.changes.empty() ? "" : ",") + c;
	}
	r.reason = r.needs.empty() ? "" : "needs " + r.needs + ": written on " + f.cls;
}

// A virtual's family changes whole: every member src has ends with this row's name and types (its own row's, or as
// src has it)
void Havok::families()
{
	std::unordered_map<std::string, Row*> byMangled;
	for (Row& r : rows)
		if (r.function && (r.verdict == "apply" || r.verdict == "same"))
			byMangled[r.function->mangled] = &r;
	auto signatureOf = [&](const std::string& mangled) -> std::string {
		if (auto found = byMangled.find(mangled); found != byMangled.end() && found->second->verdict != "open")
		{
			const Decl& d = found->second->decl;
			std::string out = d.member + "(";
			for (const Param& p : d.params)
				out += normal(p.key) + hkIntegers(p.key) + ",";
			return out + ")" + (d.isConst ? "const" : "") + normal(d.ret) + hkIntegers(d.ret);
		}
		const SrcModel::Function* f = m_model.function(mangled);
		std::optional<SrcTypes::Function> t = f ? SrcTypes::function(f->type) : std::nullopt;
		if (!t)
			return "?";
		std::string out = lastComponent(f->name) + "(";
		for (const std::string& p : t->params)
			out += normal(p) + hkIntegers(p) + ",";
		return out + ")" + (t->isConst ? "const" : "") + normal(t->ret) + hkIntegers(t->ret);
	};
	for (bool changed = true; changed;)
	{
		changed = false;
		for (Row& r : rows)
		{
			if (!r.function || !r.function->isVirtual || r.verdict == "open" || r.verdict == "pdb" ||
				r.verdict == "disagree")
				continue;
			std::string own = signatureOf(r.function->mangled);
			for (const std::string& member : familyOf(r.function->mangled))
			{
				if (member == r.function->mangled)
					continue;
				std::string other = signatureOf(member);
				// a covariant result: the members' results may differ where both are pointers to records
				if (other.substr(0, other.find(')')) != own.substr(0, own.find(')')))
				{
					const SrcModel::Function* m = m_model.function(member);
					r.open("virtual family: " + (m ? m->name : member) + " differs");
					changed = true;
					break;
				}
			}
		}
	}
}

// The new name is free where it goes: no other src function of the qualified name and the same parameters (or, for a
// free function, any: an overload would change what src's calls and registrations resolve to, declare's rule); a
// member's overload only where every call tells the two apart (another parameter count, or at one position pointers or
// references to records neither of which derives from the other) and src registers neither by its name alone; no
// member of a base or derived class it would hide or be hidden by (a structor's porting name aside), no field or
// type of it
void Havok::clashes()
{
	std::set<std::string> registeredByName;  // mangled: a body FNVD_REPLACES names without an overload's pointer type
	for (const auto& [ea, refs] : m_model.refs())
		for (const SrcModel::Ref& ref : refs)
			if (ref.kind == "body" && ref.form.find("OVERLOAD") == std::string::npos && !ref.function.empty())
				registeredByName.insert(ref.function);
	auto pointee = [&](const std::string& key) -> std::string {
		std::string k = kindOf(key);
		if (k != "ptr" && k != "ref")
			return "";
		SrcTypes::Parts p = m_types.parts(resolved(m_types.parts(resolved(key)).of));
		return p.kind == "record" ? p.name : "";
	};
	// every call tells the two parameter lists apart
	auto distinct = [&](const std::vector<std::string>& a, const std::vector<std::string>& b) {
		if (a.size() != b.size())
			return true;
		for (size_t k = 0; k < a.size(); ++k)
		{
			std::string x = pointee(a[k]), y = pointee(b[k]);
			if (!x.empty() && !y.empty() && x != y && !derives(x, y) && !derives(y, x))
				return true;
		}
		return false;
	};
	auto keysOf = [](const Decl& d) {
		std::vector<std::string> out;
		for (const Param& p : d.params)
			out.push_back(p.key);
		return out;
	};
	auto sameKeys = [&](const std::vector<std::string>& a, const std::vector<std::string>& b) {
		if (a.size() != b.size())
			return false;
		for (size_t k = 0; k < a.size(); ++k)
			if (normal(a[k]) != normal(b[k]))
				return false;
		return true;
	};
	std::map<std::string, std::vector<Row*>> byName;
	for (Row& r : rows)
		if (r.verdict == "apply" || r.verdict == "same")
			byName[r.newQualified].push_back(&r);
	for (auto& [name, list] : byName)
		for (Row* row : list)
		{
			Row& r = *row;
			bool structor = !r.symbol.structor.empty();
			for (const Row* other : list)
				if (other != row && (!r.decl.owner || !distinct(keysOf(r.decl), keysOf(other->decl))))
				{
					r.open("overload of rows: " + std::to_string(list.size()) + " take " + name);
					break;
				}
			if (r.verdict == "open")
				continue;
			// the name and owner as src has them: no new name, only the same signature as another's is new
			bool renamed = r.newQualified != r.function->name;
			for (const SrcModel::Function* other : m_model.functionsNamed(name))
			{
				if (other->mangled == r.function->mangled)
					continue;
				std::optional<SrcTypes::Function> t = SrcTypes::function(other->type);
				if (!t || sameKeys(t->params, keysOf(r.decl)))
				{
					// src declares the function already, without a body or an address (the game PDB's name of it):
					// that declaration gets the forwarder, this one goes
					const SrcModel::Abi* abi = m_model.abi(other->mangled);
					bool bare = t && !m_model.definition(other->mangled) && !m_address.count(other->mangled) &&
								m_model.refsAt(r.ea).size() <= 1 && abi && abi->cc == r.decl.cc &&
								normal(t->ret) == normal(r.decl.ret) && t->isConst == r.decl.isConst &&
								other->isStatic == r.decl.isStatic && !other->isVirtual && !r.function->isVirtual &&
								other->structor.empty();
					if (bare && r.merge == nullptr)
					{
						r.merge = other;
						r.verdict = "apply";
						r.changes = r.move ? "merge,owner" : "merge";
						r.reason = "src declares it as " + other->name + " (" +
								   (other->provenance == SrcModel::Provenance::Pdb ? "the game PDB's" : "src's") +
								   ", no body)";
						continue;
					}
					r.open("same signature: src has " + describe(*other));
					break;
				}
				if (!renamed)
					continue;
				if (!r.decl.owner || !distinct(t->params, keysOf(r.decl)) || other->required >= 0)
				{
					r.open("overload: src has " + describe(*other));
					break;
				}
				if (registeredByName.count(other->mangled))
				{
					r.open("overload src registers by its name: " + other->name);
					break;
				}
			}
			if (r.verdict == "open" || !r.decl.owner || r.merge || !renamed)
				continue;
			const SrcModel::Record& owner = *r.decl.owner;
			if (owner.field(r.decl.member))
			{
				r.open("name taken: a field " + owner.name + "::" + r.decl.member);
				continue;
			}
			if (m_model.record(name))
			{
				r.open("name taken: a type " + name);
				continue;
			}
			if (structor)
				continue;
			// a base's member of the name the new one would hide; a derived class's hides the new one, which changes
			// a call only where a rewritten use's receiver is of that class (uses() opens such a row when it has
			// uses), or where the new one is virtual
			bool isVirtual = r.function->isVirtual || r.symbol.isVirtual;
			std::vector<std::string> family = isVirtual ? familyOf(r.function->mangled) : std::vector<std::string>();
			for (const auto& [recordName, record] : m_model.records())
			{
				bool base = derives(owner.name, recordName);
				if (recordName == owner.name || !(base || derives(recordName, owner.name)))
					continue;
				for (const SrcModel::Function* other : m_model.functionsNamed(recordName + "::" + r.decl.member))
					if (std::find(family.begin(), family.end(), other->mangled) == family.end())
					{
						if (!base && !isVirtual)
						{
							r.hidden = other->name;
							continue;
						}
						r.open("name taken: " + other->name + " in a " + (base ? "base" : "derived class"));
						break;
					}
				if (r.verdict == "open")
					break;
			}
		}
}

// Each row's old name, where src spells it: its declaration and definition, srcdb's recorded uses (rewritten), fold
// comments naming it and `//` comments mentioning it (rewritten), emitted bodies (re-emitted). An old name only this
// row's function has is looked for in every src file: a spelling none of those explains (a dependent call in a
// template, a line srcdb did not see) makes the row open. A move with recorded uses is open (their receivers or
// qualifiers name the old class or scope).
void Havok::uses()
{
	std::map<std::string, std::vector<Row*>> byOld;
	for (Row& r : rows)
		if ((r.verdict == "apply" || r.verdict == "same") && r.oldName != r.decl.member)
			byOld[r.oldName].push_back(&r);
	std::unordered_map<std::string, size_t> named;  // last name component -> src functions of it
	for (const auto& [mangled, f] : m_model.functions())
		if (byOld.count(lastComponent(f.name)))
			++named[lastComponent(f.name)];
	// every spelling of an old name only one function has, in src: (file, line, col, in a `//` comment)
	struct Spelling
	{
		std::string file;
		int64_t line = 0, col = 0;
		bool comment = false;
	};
	std::map<std::string, std::vector<Spelling>> spelled;
	for (const auto& entry : std::filesystem::recursive_directory_iterator("src"))
	{
		std::string path = entry.path().string();
		if (!entry.is_regular_file() || !(headerFile(path) || path.ends_with(".cpp")))
			continue;
		Text t(readFile(path));
		for (size_t k = 0; k < t.text.size();)
		{
			if (!identifierChar(t.text[k]) || (k > 0 && identifierChar(t.text[k - 1])))
			{
				++k;
				continue;
			}
			size_t e = k;
			while (e < t.text.size() && identifierChar(t.text[e]))
				++e;
			std::string word = t.text.substr(k, e - k);
			if (auto found = byOld.find(word); found != byOld.end() && found->second.size() == 1 && named[word] == 1)
			{
				int64_t line = t.lineOf(k);
				size_t start = t.at(line);
				size_t slashes = t.text.find("//", start);
				spelled[word].push_back({path, line, int64_t(k - start + 1), slashes < k});
			}
			k = e;
		}
	}
	std::map<std::string, std::vector<std::string>> fileLines;
	auto linesOf = [&](const std::string& file) -> const std::vector<std::string>& {
		auto found = fileLines.find(file);
		if (found == fileLines.end())
			found = fileLines.emplace(file, Text(readFile(file)).lines()).first;
		return found->second;
	};
	// the emitted bodies whose block holds a line
	auto emittedAt = [&](const std::string& file, int64_t line, uint64_t ea) {
		const std::vector<std::string>& lines = linesOf(file);
		std::optional<EmittedBlock> block = emittedBlock(lines, line);
		if (!block)
			return false;
		for (const auto& [mangled, def] : m_model.definitions())
			if (def.emitted && def.file == file && def.line >= block->whole.first && def.line <= block->whole.last)
				if (auto at = m_address.find(mangled); at != m_address.end())
					m_emittedCallers[ea].insert(at->second);
		return true;
	};
	for (Row& r : rows)
	{
		if (r.verdict != "apply" && r.verdict != "same")
			continue;
		const SrcModel::Function& f = *r.function;
		const std::string& old = r.oldName;
		std::vector<SrcModel::Use> uses = m_model.uses(f.mangled);
		// a use of a function whose types change keeps its text: a call whose argument or result the new types
		// convert silently (an arithmetic slot of another kind or width; a result of another arithmetic type, whose
		// comparisons may change) is open, any other mismatch fails to compile and is fixed or dropped by hand; a
		// merge's uses follow the old declaration's call form
		bool retyped = r.verdict == "apply" && (r.changes.find("types") != std::string::npos || r.merge);
		std::string silent = retyped && !r.merge ? silentConversion(r) : "";
		size_t code = 0;
		if (old == r.decl.member)
		{
			// the name stays: only a move or a retype has its uses to fix; a caller inside an emitted block is
			// re-emitted
			for (const SrcModel::Use& u : uses)
				if (!emittedAt(u.file, u.line, r.ea))
					++code;
			if (r.move && code)
				r.open("a move with uses: " + std::to_string(code) +
					   " (their receivers or qualifiers name the old class)");
			if (r.merge && code)
				r.open("a merge with uses: " + std::to_string(code));
			if (!silent.empty() && code)
				r.open("a retype with uses the new types convert silently: " + silent);
			continue;
		}
		std::set<std::pair<std::string, int64_t>> explained;
		// the declaration's and definition's own lines
		if (std::optional<Lines> l = declarationLines(linesOf(f.file), f))
			for (int64_t k = l->top; k <= l->last; ++k)
				explained.insert({f.file, k});
		if (const SrcModel::Definition* def = m_model.definition(f.mangled))
		{
			const std::vector<std::string>& lines = linesOf(def->file);
			std::optional<EmittedBlock> block = def->emitted ? emittedBlock(lines, def->line) : std::nullopt;
			int64_t from = block ? block->whole.first : declarationFirst(lines, def->line);
			int64_t to = block ? block->whole.last : def->endLine;
			for (int64_t k = from; k <= to; ++k)
				explained.insert({def->file, k});
		}
		for (const SrcModel::Ref& ref : m_model.refsAt(r.ea))
			if (ref.kind == "fold")
			{
				explained.insert({ref.file, ref.line});
				m_folds[r.ea].push_back({ref.file, ref.line});
			}
		for (const SrcModel::Use& u : uses)
		{
			explained.insert({u.file, u.line});
			// a caller inside an emitted block is the emitter's: re-emitted, not edited
			if (emittedAt(u.file, u.line, r.ea))
				continue;
			const std::vector<std::string>& lines = linesOf(u.file);
			if (u.line < 1 || u.line > int64_t(lines.size()) ||
				lines[u.line - 1].compare(size_t(u.col - 1), old.size(), old) != 0)
			{
				r.open("use not spelled as the model has it (a model older than src): " + u.file + ":" +
					   std::to_string(u.line));
				break;
			}
			m_uses[r.ea].push_back(u);
			++code;
		}
		if (r.verdict == "open")
			continue;
		for (const Spelling& s : spelled[old])
		{
			if (explained.count({s.file, s.line}) || emittedAt(s.file, s.line, r.ea))
				continue;
			if (s.comment)
			{
				m_uses[r.ea].push_back({s.file, s.line, s.col});
				continue;
			}
			r.open("unrecorded use: " + old + " at " + s.file + ":" + std::to_string(s.line));
			break;
		}
		if (r.merge && code)
			r.open("a merge with uses: " + std::to_string(code));
		if (!silent.empty() && code)
			r.open("a retype with uses the new types convert silently: " + silent);
		// a member src calls on its object becomes the porting form's static Construct, Destruct or DeletingDestruct:
		// its calls take the object as an argument
		if (!r.symbol.structor.empty() && r.function->isStatic != r.decl.isStatic && code)
			r.open("a structor's porting form with uses: " + std::to_string(code));
		if (r.verdict != "open" && !r.hidden.empty() && code)
			r.open("name hidden in a derived class with uses: " + r.hidden);
		if (r.verdict != "open" && r.move && !r.qualify && (code || !m_folds[r.ea].empty()))
			r.open("a move with uses: " + std::to_string(code + m_folds[r.ea].size()) +
				   " (their receivers or qualifiers name the old class)");
	}
}

std::string Havok::silentConversion(const Row& r) const
{
	std::optional<SrcTypes::Function> t = SrcTypes::function(r.function->type);
	if (!t)
		return "";
	auto arithmetic = [&](const std::string& key) {
		std::string k = kindOf(key);
		return k == "bool" || k == "char" || k == "wchar_t" || k == "float" || k == "int" || k == "enum";
	};
	// an integer and an enum of one width pass the same bits
	auto integral = [&](const std::string& key) { return kindOf(key) == "int" || kindOf(key) == "enum"; };
	if (arithmetic(t->ret) && arithmetic(r.decl.ret) && normal(t->ret) != normal(r.decl.ret))
		return "result " + spelled(t->ret) + " -> " + spelled(r.decl.ret);
	if (t->params.size() != r.decl.params.size())
		return "";
	for (size_t k = 0; k < t->params.size(); ++k)
	{
		const std::string &a = t->params[k], &b = r.decl.params[k].key;
		bool same = kindOf(a) == kindOf(b) || (integral(a) && integral(b));
		if (arithmetic(a) && arithmetic(b) && (!same || size(a) != size(b)))
			return "p" + std::to_string(k) + " " + spelled(a) + " -> " + spelled(b);
	}
	return "";
}

void Havok::judgeAll()
{
	for (Row& r : rows)
	{
		try
		{
			judge(r);
		}
		catch (const std::exception& e)
		{
			r.open(std::string("error: ") + e.what());
		}
		if (r.verdict.empty())
			r.verdict = "open";
	}
	families();
	clashes();
	uses();
	// a comment kept above a declaration rewritten where it is that names a parameter the new one no longer has no
	// longer fits it: fixed by hand
	FileEdits probe;
	for (Row& r : rows)
		if ((r.verdict == "apply" || r.verdict == "same") && !r.move && !r.merge)
			if (std::string p = staleParameter(r, probe.lines(r.function->file)); !p.empty())
				r.open("a kept comment names a parameter the declaration no longer has: " + p);
	// what write() needs of src's text: the declaration's own lines (a merge's may be its definition), a definition
	// that is a forwarder or an emitted block (a hand-written body changes by hand)
	for (Row& r : rows)
	{
		if (r.verdict != "apply")
			continue;
		const SrcModel::Function& f = *r.function;
		const SrcModel::Definition* def = m_model.definition(f.mangled);
		bool byDefinition = def && def->file == f.file && def->line == f.line;
		if (byDefinition && !r.merge)
			r.open("src declares it only by its definition");
		else if (!byDefinition && !declarationLines(probe.lines(f.file), f))
			r.open("src's declaration is not on the model's lines (or an inline definition)");
		// a merge into src's declaration that returns a class by value: the exe's hidden result pointer is written
		// explicitly in src (declarations.md "ABI"), which src's declaration does not do (judge() writes it for the
		// symbol's own declaration)
		if (r.merge)
			if (std::optional<SrcTypes::Function> t = SrcTypes::function(r.merge->type);
				t && m_types.parts(resolved(t->ret)).kind == "record")
				r.open("a merge into a declaration with a class result by value");
		bool emitted = false;
		std::string why;
		definitionRange(r, probe, emitted, why);
		// a registered body keeps its text but for its name: a rename alone rewrites the name in its head
		if (why == "hand-written body" && r.changes == "name")
			r.handBody = true;
		else if (!why.empty())
			r.open("src's definition: " + why);
	}
	for (Row& r : rows)
		if (r.verdict == "apply" || r.verdict == "same" || r.verdict == "disagree")
		{
			try
			{
				r.declaration = m_forwarders.signature(r.decl, false);
			}
			catch (const Skip& s)
			{
				if (r.verdict != "disagree")
					r.open(s.reason);
			}
		}
}

std::optional<Havok::Lines> Havok::declarationLines(const std::vector<std::string>& lines,
													const SrcModel::Function& f) const
{
	if (f.line < 1 || f.line > int64_t(lines.size()) || lines[f.line - 1].find(lastComponent(f.name)) == std::string::npos)
		return std::nullopt;
	Lines l;
	l.first = declarationFirst(lines, f.line);
	int depth = 0;
	for (int64_t k = f.line; k <= int64_t(lines.size()) && !l.last; ++k)
		for (char c : lines[k - 1])
		{
			depth += c == '(' ? 1 : c == ')' ? -1 : 0;
			if (depth == 0 && (c == ';' || c == '{'))
			{
				if (c == '{')
					return std::nullopt;  // an inline definition
				l.last = k;
				break;
			}
		}
	if (!l.last)
		return std::nullopt;
	l.top = commentTop(lines, l.first);
	return l;
}

std::optional<LineRange> Havok::definitionRange(const Row& r, FileEdits& edits, bool& emitted, std::string& why) const
{
	emitted = false;
	const SrcModel::Definition* def = m_model.definition(r.function->mangled);
	if (!def)
		return std::nullopt;
	if (def->instance)
	{
		why = "a template's body";
		return std::nullopt;
	}
	const std::vector<std::string>& lines = edits.lines(def->file);
	if (def->emitted)
	{
		std::optional<EmittedBlock> block = emittedBlock(lines, def->line);
		if (!block)
		{
			why = "an emitted definition without its block";
			return std::nullopt;
		}
		emitted = true;
		return block->whole;
	}
	if (m_model.registers(r.ea) || m_model.handPorted(r.ea))
	{
		why = "hand-written body";
		return std::nullopt;
	}
	std::optional<LineRange> range = definitionLines(lines, *def, r.ea);
	bool forwarder = false;
	for (int64_t k = range ? range->first : 1; range && k <= range->last; ++k)
		forwarder = forwarder || lines[k - 1].find("Not reconstructed yet.") != std::string::npos;
	if (!range || !forwarder)
	{
		why = "hand-written body";
		return std::nullopt;
	}
	return range;
}

std::string Havok::comment(const Row& r, bool forwarder) const
{
	std::string first = forwarder ? "Not reconstructed yet." : "";
	if (!r.decl.marker.empty())
		first += (first.empty() ? "" : " ") + r.decl.marker;
	std::string text = first.empty() ? "" : "// " + first;
	if (r.comment)
		text += (text.empty() ? "" : "\n") + signatureLine(r);
	return text;
}

std::string Havok::signatureLine(const Row& r) const
{
	// short lines: declare's Writer wraps a longer marker line without carrying its `//`; a further line names the
	// SSE math types and the hidden result pointer, also as `Havok SDK` evidence (withoutMarkers drops both)
	std::string text = "// Havok SDK signature (" + r.symbol.lib + ", tied by " + r.symbol.evidence + ").";
	std::vector<std::string> notes;
	if (!r.symbol.sse.empty())
		notes.push_back("math types the SSE headers'");
	if (!r.result.empty())
		notes.push_back("result through the hidden result pointer apResult");
	for (size_t k = 0; k < notes.size(); ++k)
		text += (k == 0 ? "\n// Havok SDK: " : "; ") + notes[k] + (k + 1 == notes.size() ? "." : "");
	return text;
}

std::string Havok::nameMarker(const Symbol&, const std::string& pdbName) const
{
	return m_model.pdbFunctions().count(pdbName) ? "" : "Havok SDK name; not in the PDB.";
}

std::string Havok::staleParameter(const Row& r, const std::vector<std::string>& lines) const
{
	std::optional<Lines> l = declarationLines(lines, *r.function);
	if (!l)
		return "";
	std::set<std::string> now, old;
	for (const Param& p : r.decl.params)
		now.insert(p.name);
	for (const std::string& p : r.function->params)
		if (identifier(p) && p != "this")
			old.insert(p);
	static const std::regex idaName(R"((a|arg)\d+)");
	for (const std::string& text : keptComments(lines, l->top, l->first, r.nameOnly))
	{
		for (size_t at = 0; at < text.size();)
		{
			if (!identifierChar(text[at]) || (at > 0 && identifierChar(text[at - 1])))
			{
				++at;
				continue;
			}
			size_t e = at;
			while (e < text.size() && identifierChar(text[e]))
				++e;
			// a word after a `.` is a member or a file's extension (`hkpReportContactMgr.cpp`), no parameter
			std::string word = text.substr(at, e - at);
			if (!now.count(word) && (at == 0 || text[at - 1] != '.') &&
				(std::regex_match(word, idaName) || old.count(word)))
				return word;
			at = e;
		}
	}
	return "";
}

// SrcPlace's rule (definitionSource), with the symbol's name as the PDB's: a member on its class, its forwarder in
// the PDB's source file where src has it, else the class's .cpp, else the header's same-stem .cpp; a namespace's
// function in its namespace's header (the PDB source's same-stem header, else the one src header named after the
// namespace, else where it is), its forwarder in the PDB's source, else that header's same-stem .cpp (where it stays:
// its definition's file)
void Havok::place(const Row& r, Decl& d) const
{
	const SrcModel::Function& f = *r.function;
	const Symbol& s = r.symbol;
	std::string source = m_sources.source(s.scope.empty() ? s.name : s.scope + "::" + s.name);
	if (d.owner)
	{
		d.header = d.owner->file;
		if (!headerFile(d.owner->file))
		{
			d.source = d.owner->file;
			return;
		}
		d.source = !source.empty() ? source : m_model.classSource(d.owner->name);
		if (d.source.empty())
			d.source = stemOf(d.owner->file) + ".cpp";
		return;
	}
	d.header = f.file;
	std::string header = source.empty() ? "" : stemOf(source) + ".h";
	if (header.empty() || !std::filesystem::exists(header))
	{
		header.clear();
		if (!m_headers)
		{
			m_headers.emplace();
			for (const auto& entry : std::filesystem::recursive_directory_iterator("src"))
				if (entry.is_regular_file() && entry.path().extension() == ".h")
					(*m_headers)[entry.path().stem().string()].push_back(entry.path().string());
		}
		if (auto found = m_headers->find(lastComponent(d.scope)); found != m_headers->end() && found->second.size() == 1)
			header = found->second[0];
	}
	if (!header.empty())
		d.header = header;
	const SrcModel::Definition* definition = m_model.definition(f.mangled);
	if (!source.empty())
		d.source = source;
	else if (d.header == f.file && definition)
		d.source = definition->file;
	else
		d.source = headerFile(d.header) ? stemOf(d.header) + ".cpp" : d.header;
}

void Havok::write(FileEdits& edits, std::map<uint64_t, std::string>& outcomes, std::set<uint64_t>& reemit)
{
	DeclarationWriter writer(m_model, m_types, edits);
	// the lines each accepted row's edits take, per file: no two rows touch one line
	std::map<std::string, std::set<int64_t>> claimed, claimedUses;
	struct Edit
	{
		std::string file;
		LineRange range;  // last = first - 1: an insert before first
		std::string text;
	};
	std::map<std::pair<std::string, int64_t>, std::vector<std::pair<int64_t, std::pair<std::string, std::string>>>>
		useLines;  // (file, line) -> (col, (old, new))
	// a .cpp on disk the index did not compile is newer than the model (one an earlier apply created and a restore of
	// src left): a forwarder appended to it would be its second definition
	auto newerThanModel = [&](const std::string& source) {
		return source.ends_with(".cpp") && std::filesystem::exists(source) && !m_model.isTu(source);
	};
	for (Row& r : rows)
	{
		if (r.verdict != "apply" && r.verdict != "same")
			continue;
		std::vector<Edit> own;
		std::string why;
		const SrcModel::Function& f = *r.function;
		const std::vector<std::string>& declLines = edits.lines(f.file);
		const SrcModel::Definition* definition = m_model.definition(f.mangled);
		// a function src declares only by its definition (a .cpp's): a merge removes that alone
		bool byDefinition = definition && definition->file == f.file && definition->line == f.line;
		std::optional<Lines> l = byDefinition ? std::nullopt : declarationLines(declLines, f);
		if (!l && !(byDefinition && r.merge))
		{
			outcomes[r.ea] = "failed: the declaration's lines are not the model's (or an inline definition)";
			continue;
		}
		bool emitted = false;
		std::optional<LineRange> def = definitionRange(r, edits, emitted, why);
		// a registered body stays as it is for a row that changes nothing but the evidence comment, or only the name
		// (its head's name rewritten as a use)
		bool keepBody = why == "hand-written body" && (r.verdict == "same" || r.handBody);
		if (!def && !why.empty() && !keepBody)
		{
			outcomes[r.ea] = "failed: " + why;
			continue;
		}
		if (keepBody && r.handBody)
		{
			const std::vector<std::string>& defLines = edits.lines(definition->file);
			size_t at = definition->line >= 1 && definition->line <= int64_t(defLines.size())
							? wordAt(defLines[definition->line - 1], r.oldName)
							: std::string::npos;
			if (at == std::string::npos)
			{
				outcomes[r.ea] = "failed: the body's head does not spell the name on the model's line";
				continue;
			}
			m_uses[r.ea].push_back({definition->file, definition->line, int64_t(at + 1)});
		}
		Decl d = r.decl;
		// the comment lines above the declaration that are no marker stay (an `// Inferred type` clause too where
		// src's types stay)
		std::string kept;
		for (const std::string& rest : l ? keptComments(declLines, l->top, l->first, r.nameOnly)
										 : std::vector<std::string>())
			kept += rest + "\n";
		if (r.merge)
		{
			// src's bodiless declaration of the function gets the forwarder; this declaration and its definition go
			if (!kept.empty())
			{
				outcomes[r.ea] = "failed: a merge with comments above the declaration that goes";
				continue;
			}
			Decl m;
			try
			{
				srcDeclaration(m, m_model, *r.merge, r.merge->cls.empty() || r.merge->isStatic ? "" : "this");
			}
			catch (const Skip& s)
			{
				outcomes[r.ea] = "failed: " + s.reason;
				continue;
			}
			m.ea = r.ea;
			m.outcome = "defined";
			m.noreturn = d.noreturn;
			m.header = r.merge->file;
			// the forwarder goes where SrcPlace puts a definition of src's declaration: the PDB's source, else its
			// class's .cpp, else its header's same-stem .cpp (the .cpp it is declared in)
			m.source = definitionSource(m_model, m_sources, *r.merge, r.merge->file);
			if (newerThanModel(m.source))
			{
				outcomes[r.ea] = "failed: the forwarder's .cpp is not in the model (a file newer than the model)";
				continue;
			}
			// a slot the symbol spells hkUlong or hkLong where src's declaration spells another integer of its size:
			// that declaration takes the symbol's spelling of it, the rest as src has it
			bool respell = hkIntegers(m.ret) != hkIntegers(r.decl.ret);
			for (size_t k = 0; k < m.params.size() && k < r.decl.params.size(); ++k)
				if (hkIntegers(m.params[k].key) != hkIntegers(r.decl.params[k].key))
					m.params[k].key = r.decl.params[k].key, respell = true;
			if (respell)
			{
				const std::vector<std::string>& mergeLines = edits.lines(r.merge->file);
				std::optional<Lines> ml = declarationLines(mergeLines, *r.merge);
				if (!ml)
				{
					outcomes[r.ea] = "failed: the merged declaration's lines are not the model's";
					continue;
				}
				if (hkIntegers(m.ret) != hkIntegers(r.decl.ret))
					m.ret = r.decl.ret;
				m.exact = false;
				std::string text;
				for (int64_t k = ml->first; k <= ml->last; ++k)
					text += mergeLines[k - 1] + "\n";
				std::string head;
				static const std::regex attribute(R"(\b(FNVD_NO_STACK_COOKIE)\b)");
				for (auto it = std::sregex_iterator(text.begin(), text.end(), attribute); it != std::sregex_iterator();
					 ++it)
					head += (*it)[1].str() + " ";
				const std::string& first = mergeLines[ml->first - 1];
				std::string mergeIndent = first.substr(0, first.find_first_not_of(" \t"));
				Decl declared = m;
				declared.outcome = "declared";
				try
				{
					own.push_back({r.merge->file, {ml->first, ml->last},
								   wrapped(head + m_forwarders.signature(declared, false) + ";", mergeIndent,
										   indentUnit(mergeLines, ml->first, ml->last))});
				}
				catch (const Skip& s)
				{
					outcomes[r.ea] = "failed: " + s.reason;
					continue;
				}
			}
			d = m;
			if (l)
				own.push_back({f.file, {l->top, l->last}, ""});
			if (def)
				own.push_back({definition->file, *def, ""});
		}
		else if (r.move)
		{
			if (!kept.empty())
			{
				outcomes[r.ea] = "failed: a move with comments above the declaration";
				continue;
			}
			// declare's Writer writes `// Not reconstructed yet. <marker>` above it: the signature line goes after
			if (r.comment)
				d.marker += "\n" + signatureLine(r);
			place(r, d);
			if (d.source.empty())
			{
				outcomes[r.ea] = "failed: no .cpp for the forwarder";
				continue;
			}
			if (newerThanModel(d.source))
			{
				outcomes[r.ea] = "failed: the forwarder's .cpp is not in the model (a file newer than the model)";
				continue;
			}
			own.push_back({f.file, {l->top, l->last}, ""});
			if (def)
				own.push_back({definition->file, *def, ""});
		}
		else
		{
			d.header = f.file;
			d.source = definition ? definition->file : "";
			// the head's own words that stay: virtual, the seam's attribute macros; the tail's override, final, = 0
			std::string head, tail;
			std::string text;
			for (int64_t k = l->first; k <= l->last; ++k)
				text += declLines[k - 1] + "\n";
			static const std::regex attribute(R"(\b(FNVD_NO_STACK_COOKIE)\b)");
			for (auto it = std::sregex_iterator(text.begin(), text.end(), attribute); it != std::sregex_iterator(); ++it)
				head += (*it)[1].str() + " ";
			if (std::regex_search(text, std::regex(R"(^\s*virtual\b)")))
				head = "virtual " + head;
			size_t close = text.rfind(')');
			std::string after = close == std::string::npos ? "" : text.substr(close);
			for (const char* word : {"override", "final"})
				if (std::regex_search(after, std::regex(std::string("\\b") + word + "\\b")))
					tail += std::string(" ") + word;
			if (std::regex_search(after, std::regex(R"(=\s*0\s*;)")))
				tail += " = 0";
			std::string indent = declLines[l->first - 1].substr(0, declLines[l->first - 1].find_first_not_of(" \t"));
			std::string unit = indentUnit(declLines, l->first, l->last);
			std::string declaration;
			if (r.nameOnly)
			{
				// a C name, or a member src has as a free function: src's declaration stays as it is written, but
				// the name (its convention macro, its parameters' names and defaults)
				bool wide = false, comments = false;
				std::string joined;
				for (int64_t k = l->first; k <= l->last; ++k)
				{
					std::string line = declLines[k - 1];
					if (k == f.line)
						if (size_t at = wordAt(line, r.oldName); at != std::string::npos)
							line.replace(at, r.oldName.size(), d.member);
					declaration += (declaration.empty() ? "" : "\n") + line;
					wide = wide || columns(line) > 120;
					comments = comments || line.find("//") != std::string::npos || line.find("/*") != std::string::npos;
					joined += (joined.empty() ? "" : " ") + trimmed(line);
				}
				// grown past 120 columns: its text, on one line, wrapped as a new declaration is
				if (wide && !comments)
					declaration = wrapped(joined, indent, unit);
			}
			else
				try
				{
					declaration = wrapped(head + m_forwarders.signature(d, false) + tail + ";", indent, unit);
				}
				catch (const Skip& s)
				{
					outcomes[r.ea] = "failed: " + s.reason;
					continue;
				}
			std::string commentText;
			for (const std::string& line : lineList(comment(r, !keepBody && definition)))
				commentText += indent + line + "\n";
			std::string keptText;
			for (const std::string& line : lineList(kept))
				if (!line.empty())
					keptText += indent + line + "\n";
			// src's types staying, their `// Inferred type` clause stays right above the declaration, after the markers
			own.push_back({f.file, {l->top, l->last},
						   (r.nameOnly ? commentText + keptText : keptText + commentText) + declaration});
			if (def)
			{
				const std::vector<std::string>& defLines = edits.lines(definition->file);
				std::string defUnit = indentUnit(defLines, def->first, def->last);
				try
				{
					own.push_back({definition->file, *def, m_forwarders.definition(d, false, defUnit.empty() ? "\t" : defUnit)});
				}
				catch (const Skip& s)
				{
					outcomes[r.ea] = "failed: " + s.reason;
					continue;
				}
			}
		}
		// the uses and fold comments
		for (const SrcModel::Use& u : m_uses[r.ea])
			own.push_back({u.file, {u.line, u.line}, ""});
		for (const auto& [file, line] : m_folds[r.ea])
			own.push_back({file, {line, line}, ""});
		// no line another row's edits take; a use's line shared only with other rows' uses
		auto isUse = [&](const std::string& file, int64_t k) {
			return std::any_of(m_uses[r.ea].begin(), m_uses[r.ea].end(),
							   [&](const SrcModel::Use& u) { return u.file == file && u.line == k; });
		};
		bool overlaps = false;
		for (const Edit& e : own)
			for (int64_t k = e.range.first; k <= e.range.last && !overlaps; ++k)
				overlaps = claimed[e.file].count(k) != 0 || (!isUse(e.file, k) && claimedUses[e.file].count(k) != 0);
		if (overlaps)
		{
			outcomes[r.ea] = "failed: another row's edit takes one of its lines";
			continue;
		}
		try
		{
			if (r.move || r.merge)
				writer.add(d);
			else
				writer.inPlace(d, l->first);
		}
		catch (const Skip& s)
		{
			outcomes[r.ea] = "failed: " + s.reason;
			continue;
		}
		size_t uses = 0, folds = 0;
		for (const Edit& e : own)
		{
			bool use = isUse(e.file, e.range.first);
			bool fold = std::any_of(m_folds[r.ea].begin(), m_folds[r.ea].end(), [&](const auto& p) {
				return p.first == e.file && p.second == e.range.first;
			});
			if (use)
			{
				claimedUses[e.file].insert(e.range.first);
				continue;
			}
			// a removed block's blank line after it goes with it, so no two blank lines are left
			LineRange range = e.range;
			const std::vector<std::string>& lines = edits.lines(e.file);
			if (e.text.empty() && range.last < int64_t(lines.size()) && trimmed(lines[range.last]).empty() &&
				(range.first == 1 || trimmed(lines[range.first - 2]).empty()) && !claimed[e.file].count(range.last + 1))
				++range.last;
			for (int64_t k = range.first; k <= range.last; ++k)
				claimed[e.file].insert(k);
			if (fold)
				continue;
			edits.replace(e.file, range, e.text);
		}
		// a use of a static member src had as a free function spells its class, whose header its .cpp includes
		for (const SrcModel::Use& u : m_uses[r.ea])
		{
			useLines[{u.file, u.line}].push_back({u.col, {r.oldName, r.qualify ? r.newQualified : d.member}}), ++uses;
			if (r.qualify && u.file.ends_with(".cpp") && d.header.rfind("src/", 0) == 0)
				ensureInclude(edits, u.file, d.header.substr(4));
		}
		std::string oldQualified = f.name, newQualified = r.newQualified;
		for (const auto& [file, line] : m_folds[r.ea])
		{
			std::string text = edits.lines(file)[line - 1];
			size_t at = text.find(oldQualified);
			if (at != std::string::npos)
				text.replace(at, oldQualified.size(), newQualified);
			edits.replace(file, {line, line}, text);
			++folds;
		}
		if (emitted)
			reemit.insert(r.ea);
		for (uint64_t caller : m_emittedCallers[r.ea])
			reemit.insert(caller);
		outcomes[r.ea] = std::string(r.verdict == "same" ? "commented" : "applied") +
						 (uses ? "; " + std::to_string(uses) + " uses" : "") +
						 (folds ? "; " + std::to_string(folds) + " fold comments" : "") + (emitted ? "; re-emit" : "");
	}
	// each use line once, its names replaced from the right; a whole-line comment that grows past 120 columns
	// re-wrapped
	for (auto& [where, list] : useLines)
	{
		const std::string& old = edits.lines(where.first)[where.second - 1];
		std::string text = old;
		std::sort(list.begin(), list.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
		for (const auto& [col, names] : list)
			text.replace(size_t(col - 1), names.first.size(), names.second);
		if (trimmed(text).starts_with("//") && columns(text) > 120 && columns(old) <= 120)
			text = rewrapped(text);
		edits.replace(where.first, {where.second, where.second}, text);
	}
	writer.flush();
}

}  // namespace

int declareHavok(const SrcModel& model, SrcTypes& types, const PdbSources& sources, const std::string& symbols,
				 const std::vector<uint64_t>& only, const std::filesystem::path& out, bool apply)
{
	exefacts::Program program(".");
	exefacts::Facts facts(program);
	Havok havok(model, types, facts, sources);
	havok.read(symbols, only);
	havok.judgeAll();
	std::map<uint64_t, std::string> outcomes;
	std::set<uint64_t> reemit;
	FileEdits edits;
	size_t files = 0;
	if (apply)
	{
		havok.write(edits, outcomes, reemit);
		files = edits.paths().size();
		edits.apply();
	}
	std::ofstream table(out / "hk-declare.tsv", std::ios::trunc);
	table << "address\tsrc\tsymbol\tverdict\tchanges\treason\tneeds\tevidence\tlib\tdeclaration\tprovenance\tsrc_name\t"
			 "new_name\tdetail\tmangled\n";
	std::map<std::string, size_t> verdicts, reasons;
	for (const Row& r : havok.rows)
	{
		table << hexText(r.ea) << '\t' << cell(r.src) << '\t' << cell(r.symbol.demangled) << '\t' << r.verdict << '\t'
			  << r.changes << '\t' << cell(r.reason) << '\t' << cell(r.needs) << '\t' << r.symbol.evidence << '\t'
			  << r.symbol.lib << '\t' << cell(r.declaration) << '\t' << r.provenance << '\t'
			  << cell(r.function ? r.function->name : "") << '\t' << cell(r.newQualified) << '\t' << cell(r.detail)
			  << '\t' << r.symbol.mangled << '\n';
		++verdicts[r.verdict];
		if (r.verdict == "open")
			++reasons[r.reason.substr(0, r.reason.find(':'))];
		else if (r.verdict == "apply")
			++reasons["apply: " + r.changes + (r.needs.empty() ? "" : " (needs class)")];
		else if (r.verdict == "disagree")
			++reasons["disagree: " + r.reason];
	}
	table.close();
	if (apply)
	{
		std::ofstream applied(out / "hk-applied.tsv", std::ios::trunc);
		applied << "address\toutcome\n";
		std::ofstream renames(out / "renames.tsv", std::ios::trunc);
		std::map<std::string, size_t> counts;
		for (const Row& r : havok.rows)
		{
			auto found = outcomes.find(r.ea);
			if (found == outcomes.end())
				continue;
			applied << hexText(r.ea) << '\t' << cell(found->second) << '\n';
			++counts[found->second.substr(0, found->second.find_first_of(":;"))];
			if (!found->second.starts_with("failed") && r.function && r.newQualified != r.function->name)
				renames << hexText(r.ea) << '\t' << r.newQualified << "\tHavok SDK " << r.symbol.lib << " "
						<< r.symbol.mangled << " (" << r.symbol.evidence << ")\n";
		}
		std::ofstream list(out / "reemit.list", std::ios::trunc);
		for (uint64_t ea : reemit)
			list << hexText(ea) << '\n';
		std::string summary;
		for (const auto& [outcome, count] : counts)
			summary += " " + std::to_string(count) + " " + outcome;
		std::fprintf(stderr, "idakit declare --hk-symbols --apply:%s; %zu files, %zu to re-emit\n", summary.c_str(),
					 files, reemit.size());
	}
	std::string summary;
	for (const auto& [verdict, count] : verdicts)
		summary += " " + std::to_string(count) + " " + verdict;
	std::fprintf(stderr, "idakit declare --hk-symbols: %zu addresses:%s (%s)\n", havok.rows.size(), summary.c_str(),
				 (out / "hk-declare.tsv").c_str());
	std::vector<std::pair<size_t, std::string>> sorted;
	for (const auto& [reason, count] : reasons)
		sorted.push_back({count, reason});
	std::sort(sorted.rbegin(), sorted.rend());
	for (const auto& [count, reason] : sorted)
		std::fprintf(stderr, "  %6zu %s\n", count, reason.c_str());
	return 0;
}

}  // namespace idakit
