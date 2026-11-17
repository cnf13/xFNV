// idakit declare: a src declaration for each exe function src has none for (none that chooseRef takes), each a
// `// Not reconstructed yet.` forwarder to the exe's function (porting rules "Code not reconstructed yet"): its
// declaration in the class (or the free function's header) and its out-of-line definition in the .cpp the PDB names.
// The owner and name come from IDA's name (demangled), else from the prototype's `this`; the signature from src's own
// declaration without a definition, else the PDB's overload whose stack bytes fit the exe's `ret N`, else IDA's
// prototype; the convention always from the exe. A function whose text cannot be made exactly is skipped with the
// reason.
//
// Writes DIR/declare.tsv (address, outcome, kind, name, file, reason) and DIR/needs-class.tsv (class, address, name:
// the classes and virtual slots src lacks, for hdr-opus), and with --apply the src edits, all in one FileEdits::apply().
// The database is never saved. Run from the repository root (exefacts reads its env/exe; src paths are relative).

#include "DeclareWriter.hpp"
#include "IdaSupport.hpp"

#include "idakit/Address.hpp"
#include "idakit/AsmFacts.hpp"
#include "idakit/Command.hpp"
#include "idakit/Database.hpp"
#include "idakit/ExePrototype.hpp"
#include "idakit/ExeSlotTargets.hpp"
#include "idakit/ExeVtables.hpp"
#include "idakit/FunctionList.hpp"
#include "idakit/SrcEdit.hpp"
#include "idakit/SrcIda.hpp"
#include "idakit/SrcModel.hpp"
#include "idakit/SrcPlace.hpp"
#include "idakit/SrcTypes.hpp"

#include <Facts.hpp>
#include <demangle.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <regex>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace idakit
{

namespace
{

struct NeedsClass
{
	std::string cls;
	uint64_t ea;
	std::string name;
};

// What `declare --globals` writes for one exe data address: a global (FNVD_GLOBAL in header, after its last one, at
// least after its preamble), a static member of owner (FNVD_STATIC_MEMBER in its body), an exe vtable (FNVD_VFTABLE
// after owner's definition; a declaration src has in a .cpp, moveFrom's line moveLine as the model saw it, moves there),
// or an exe function's own address (FNVD_ORIGINAL_FUNCTION in header, placed as a global; one a .cpp declares at
// namespace scope, of the type a callback needs, moves the same way).
struct GlobalDecl
{
	uint64_t ea = 0;
	std::string kind;  // global, static_member, vftable, original_function
	std::string name;
	std::string marker;  // the name's marker comment, "" for a PDB name
	std::string key;	 // the type (src key; none for a vftable)
	const SrcModel::Record* owner = nullptr;
	std::string header;
	std::string moveFrom;  // a vftable or original function src declares in a .cpp: that file
	int64_t moveLine = 0;
	// a global the frozen gen/Prototypes.hpp declares (src's until now): its line there (deleted) and its name there
	int64_t frozenLine = 0;
	std::string frozenName;
};

// the suffix of the name `declare --globals` gives a function's FNVD_ORIGINAL_FUNCTION: the function's own name with
// it, which neither the PDB nor src's forwarders use (docs/IDAKIT.md "idakit declare")
constexpr const char* kOriginalFunctionSuffix = "_original";

// Whether a function whose pointer type is `function` is what a callback of pointer type `callback` holds, as the
// CRT's callbacks erase it (`void (__thiscall*)(void*)` for any item type's constructor or destructor): one
// convention and parameter count, each parameter the callback's own or, where the callback's is void*, an object
// pointer, and the callback's result the function's own or void (the result ignored).
bool erasesTo(SrcTypes& types, const std::string& function, const std::string& callback)
{
	auto pointee = [&](const std::string& key) -> std::optional<SrcTypes::Function> {
		SrcTypes::Parts p = types.parts(types.canonical(key));
		return p.kind == "ptr" ? SrcTypes::function(types.canonical(p.of)) : std::nullopt;
	};
	std::optional<SrcTypes::Function> f = pointee(function), c = pointee(callback);
	if (!f || !c || f->cc != c->cc || f->variadic != c->variadic || f->params.size() != c->params.size())
		return false;
	std::string ret = types.canonical(c->ret);
	if (ret != "builtin:void" && ret != types.canonical(f->ret))
		return false;
	for (size_t k = 0; k < f->params.size(); ++k)
	{
		std::string a = types.canonical(f->params[k]), b = types.canonical(c->params[k]);
		SrcTypes::Parts p = types.parts(a);
		bool objectPointer = p.kind == "ptr" && types.parts(types.canonical(p.of)).kind != "fn";
		if (a != b && !(b == "ptr(builtin:void)" && objectPointer))
			return false;
	}
	return true;
}

// IDA's name, read: the qualified name, its parts and what kind of function it names
struct Name
{
	std::string qualified, scope, last;
	std::string structor;  // ctor, dtor, deleting or ""
	bool unnamed = false;  // IDA's sub_XXXXXX
};

// a record name as part of an identifier: its words joined by `_` (`AStarSearch<CombatPlanNode *, X *>` ->
// `AStarSearch_CombatPlanNode_X`)
std::string identifierOf(const std::string& name)
{
	std::string out;
	for (size_t k = 0; k < name.size();)
	{
		if (!std::isalnum(static_cast<unsigned char>(name[k])) && name[k] != '_')
		{
			++k;
			continue;
		}
		size_t end = k;
		while (end < name.size() && (std::isalnum(static_cast<unsigned char>(name[end])) || name[end] == '_'))
			++end;
		std::string word = name.substr(k, end - k);
		if (word != "const" && word != "class" && word != "struct")
			out += (out.empty() ? "" : "_") + word;
		k = end;
	}
	return out;
}

// A C library function's name (the C standard's, its Annex K bounds-checked `_s` ones included, and the global names
// an underscore starts, the implementation's): the exe's function of the name is the VC9 CRT's or its headers' inline
// (`strpbrk(char*, const char*)`, `cos(float)`, `fopen_s`), declared in src/SDK/MSVC in namespace msvc9 (porting
// rules); a global declaration would clash with the C library's.
bool libraryName(const std::string& name)
{
	static const std::set<std::string> names = {
		"abs", "labs", "fabs", "acos", "asin", "atan", "atan2", "ceil", "cos", "cosh", "exp", "floor", "fmod",
		"frexp", "ldexp", "log", "log10", "modf", "pow", "sin", "sinh", "sqrt", "tan", "tanh", "round", "trunc",
		"fmin", "fmax", "hypot", "cbrt", "memchr", "memcmp", "memcpy", "memmove", "memset", "strcat", "strchr",
		"strcmp", "strcoll", "strcpy", "strcspn", "strerror", "strlen", "strncat", "strncmp", "strncpy", "strpbrk",
		"strrchr", "strspn", "strstr", "strtok", "strxfrm", "atof", "atoi", "atol", "strtod", "strtol", "strtoul",
		"rand", "srand", "calloc", "free", "malloc", "realloc", "abort", "atexit", "exit", "getenv", "system",
		"bsearch", "qsort", "div", "ldiv", "mblen", "mbtowc", "wctomb", "mbstowcs", "wcstombs", "printf", "sprintf",
		"fprintf", "sscanf", "scanf", "fopen", "fclose", "fread", "fwrite", "fseek", "ftell", "fgets", "fputs",
		"puts", "remove", "rename", "tmpfile", "vsprintf", "vprintf", "vfprintf", "snprintf", "vsnprintf", "fflush",
		"getc", "putc", "fgetc", "fputc", "isalnum", "isalpha", "iscntrl", "isdigit", "isgraph", "islower",
		"isprint", "ispunct", "isspace", "isupper", "isxdigit", "tolower", "toupper", "clock", "difftime", "mktime",
		"time", "asctime", "ctime", "gmtime", "localtime", "strftime", "wcslen", "wcscpy", "wcscmp", "wcsncpy",
		"wcscat", "wcschr", "wcsrchr", "wcsstr", "towlower", "towupper", "iswspace", "signal", "raise", "setjmp",
		"longjmp", "setlocale", "localeconv", "tmpfile_s", "tmpnam_s", "fopen_s", "freopen_s", "fprintf_s",
		"fscanf_s", "printf_s", "scanf_s", "snprintf_s", "sprintf_s", "sscanf_s", "vfprintf_s", "vfscanf_s",
		"vprintf_s", "vscanf_s", "vsnprintf_s", "vsprintf_s", "vsscanf_s", "gets_s", "getenv_s", "bsearch_s",
		"qsort_s", "wctomb_s", "mbstowcs_s", "wcstombs_s", "memcpy_s", "memmove_s", "strcpy_s", "strncpy_s",
		"strcat_s", "strncat_s", "strtok_s", "strerror_s", "strnlen_s", "asctime_s", "ctime_s", "gmtime_s",
		"localtime_s", "wcscpy_s", "wcsncpy_s", "wcscat_s", "wcsncat_s", "wcstok_s", "wcsnlen_s", "wmemcpy_s",
		"wmemmove_s", "swprintf_s", "vswprintf_s"};
	return name.starts_with('_') || names.count(name) != 0;
}

// IDA's name for an unnamed function, `sub_405A00`
std::string subName(uint64_t ea)
{
	return "sub_" + address(ea).substr(2);
}

bool headerFile(const std::string& path)
{
	return path.ends_with(".h") || path.ends_with(".hpp") || path.ends_with(".inl");
}

std::string withoutSrc(const std::string& path)
{
	return path.rfind("src/", 0) == 0 ? path.substr(4) : path;
}

std::string stem(const std::string& path)
{
	size_t dot = path.rfind('.');
	size_t slash = path.rfind('/');
	return dot == std::string::npos || (slash != std::string::npos && dot < slash) ? path : path.substr(0, dot);
}

// The line after a file's first run of #include lines, else after its `#pragma once`, else 1: where declarations a
// header needs before its classes go (ensureInclude's place).
int64_t afterIncludes(const std::vector<std::string>& lines)
{
	auto starts = [](const std::string& line, const char* word) {
		size_t at = line.find_first_not_of(" \t");
		return at != std::string::npos && line.compare(at, std::char_traits<char>::length(word), word) == 0;
	};
	int64_t at = 0, once = 0;
	for (int64_t k = 1; k <= static_cast<int64_t>(lines.size()); ++k)
	{
		if (starts(lines[k - 1], "#include"))
			at = k;
		else if (at)
			break;
		else if (!once && starts(lines[k - 1], "#pragma once"))
			once = k;
	}
	return (at ? at : once) + 1;
}

std::string trimmedText(const std::string& line)
{
	size_t a = line.find_first_not_of(" \t\r"), b = line.find_last_not_of(" \t\r");
	return a == std::string::npos ? std::string() : line.substr(a, b - a + 1);
}

// The line after a header's preamble, its leading run of preprocessor lines (#pragma, #include), forward declarations
// (`class X;`, `enum class E : T;`, `namespace N { class X; }`) and namespace-scope FNVD_GLOBALs, with the blank lines
// and comments between them; comments leading into the first other line stay with it. What goes there sees every
// include and forward declaration of the preamble, and no code comes between them. Line 1 for an empty file.
int64_t afterPreamble(const std::vector<std::string>& lines)
{
	static const std::regex forward(R"(^(?:template\s*<.*>\s*)?(?:class|struct|union|enum(?:\s+class|\s+struct)?))"
									R"(\s+[\w:]+(?:\s*:\s*[\w: ]+)?\s*;\s*(?://.*)?$)");
	static const std::regex namespaced(
		R"(^namespace\s+[\w:]+\s*\{\s*(?:(?:class|struct|union)\s+\w+\s*;\s*)+\}\s*;?$)");
	// a one-line FNVD_GLOBAL (one wrapped over lines ends the preamble above it)
	static const std::regex global(R"(^FNVD_GLOBAL\(.*\);\s*(?://.*)?$)");
	int64_t end = 0;
	bool comment = false;  // inside a /* */ comment
	for (int64_t k = 1; k <= static_cast<int64_t>(lines.size()); ++k)
	{
		std::string text = trimmedText(lines[k - 1]);
		if (comment)
		{
			comment = text.find("*/") == std::string::npos;
			continue;
		}
		if (text.empty() || text.starts_with("//"))
			continue;
		if (text.starts_with("/*"))
		{
			comment = text.find("*/", 2) == std::string::npos;
			continue;
		}
		if (lines[k - 1].starts_with("#pragma") || lines[k - 1].starts_with("#include") ||
			(lines[k - 1].starts_with("FNVD_GLOBAL(") && std::regex_match(text, global)) ||
			std::regex_match(text, forward) || std::regex_match(text, namespaced))
		{
			end = k;
			continue;
		}
		break;
	}
	return end + 1;
}

// The brace depth at the start of line `line` (comments, character and string literals left out): 0 at namespace
// scope of the file.
int braceDepth(const std::vector<std::string>& lines, int64_t line)
{
	int depth = 0;
	bool comment = false;
	for (int64_t k = 1; k < line && k <= static_cast<int64_t>(lines.size()); ++k)
	{
		const std::string& text = lines[k - 1];
		for (size_t c = 0; c < text.size(); ++c)
		{
			if (comment)
			{
				if (text.compare(c, 2, "*/") == 0)
					comment = false, ++c;
				continue;
			}
			if (text.compare(c, 2, "//") == 0)
				break;
			if (text.compare(c, 2, "/*") == 0)
			{
				comment = true;
				++c;
			}
			else if (text[c] == '"' || text[c] == '\'')
			{
				char quote = text[c];
				for (++c; c < text.size() && text[c] != quote; ++c)
					if (text[c] == '\\')
						++c;
			}
			else if (text[c] == '{')
				++depth;
			else if (text[c] == '}')
				--depth;
		}
	}
	return depth;
}

// The first line of the file that reads `line` after its leading blanks, 0 for none.
int64_t lineReading(const std::vector<std::string>& lines, const std::string& line)
{
	for (int64_t k = 1; k <= static_cast<int64_t>(lines.size()); ++k)
		if (trimmedText(lines[k - 1]) == line)
			return k;
	return 0;
}

// The reason declare gives an address whose src function is defined without a registration and is not tied to it.
constexpr const char* kUnregistered = "src defines it unregistered";
// The reason of an address whose src function is a class template instance's member made from the template's body:
// registered where it can be (Declarer::registered), else the skip's reason starts with it.
constexpr const char* kTemplateBody = "template body exists";

// The `//` comment lines src keeps with a definition (first: its name's line, last: its closing brace's): the run
// right above it, past at most two lines of its head (`template <>`, a return type on a line of its own), and the
// runs after its closing line, blank lines between them, up to the first run that code follows directly (the next
// definition's head comment) or the first code line. trailingEnd: the last line of the last such run (last for none),
// where the definition's own lines end.
struct DefinitionComments
{
	std::vector<std::string> lines;
	int64_t trailingEnd = 0;
};

DefinitionComments definitionComments(const std::vector<std::string>& lines, int64_t first, int64_t last)
{
	DefinitionComments out;
	int64_t size = static_cast<int64_t>(lines.size());
	auto comment = [&](int64_t k) { return trimmedText(lines[k - 1]).starts_with("//"); };
	auto empty = [&](int64_t k) { return trimmedText(lines[k - 1]).empty(); };
	int64_t k = std::min(first, size + 1) - 1;
	for (int head = 0; k >= 1 && head < 2 && !comment(k); ++head, --k)
	{
		std::string text = trimmedText(lines[k - 1]);
		if (text.empty() || text.starts_with('#') || text.ends_with(';') || text.ends_with('}') || text.ends_with('{'))
			break;
	}
	for (; k >= 1 && comment(k); --k)
		out.lines.push_back(lines[k - 1]);
	out.trailingEnd = last;
	for (int64_t at = last;;)
	{
		while (at < size && empty(at + 1))
			++at;
		if (at >= size || !comment(at + 1))
			break;
		int64_t end = at + 1;
		while (end < size && comment(end + 1))
			++end;
		if (end < size && !empty(end + 1))
			break;
		for (int64_t j = at + 1; j <= end; ++j)
			out.lines.push_back(lines[j - 1]);
		out.trailingEnd = at = end;
	}
	return out;
}

// The PDB's vtables (env/cache/pdb-types.txt, llvm-pdbutil's types dump), as names.py builds them: a
// class's primary table is its first base's with a table, then the virtuals it introduces at their vftable offsets
// (a gap where the offsets skip one), each slot named by the most derived class of the primary chain declaring a
// virtual of its name; its secondary tables are its other bases' subobjects in layout order (each base's own primary
// one included unless it is the class's primary base), each slot named by the class between it and that base that
// declares a virtual of the name, else by that base's table. Read on first use.
class PdbVtables
{
public:
	explicit PdbVtables(std::string path) : m_path(std::move(path)) {}
	// the qualified names (the PDB's spelling) of the slots of the class's table for its subobject-th vtable-bearing
	// subobject (0: its own primary table), "" for a gap; nullopt where the PDB has no such class or subobject
	std::optional<std::vector<std::string>> slots(const std::string& cls, size_t subobject)
	{
		load();
		std::vector<std::string> subs = subobjects(cls, 0);
		if (subobject >= subs.size())
			return std::nullopt;
		std::vector<std::string> out;
		for (const auto& [owner, name] : table(subs[subobject], 0))
		{
			std::string by = subobject == 0 || name.empty() ? owner : overrider(cls, subs[subobject], name);
			out.push_back(name.empty() ? "" : (by.empty() ? owner : by) + "::" + name);
		}
		return out;
	}

private:
	struct Method
	{
		std::string name;
		int64_t vtoff = -1;  // the slot's byte offset where it introduces one
		bool isVirtual = false;
	};
	struct Class
	{
		std::vector<std::string> bases;  // the non-virtual bases, in order
		std::vector<Method> methods;
	};
	using Table = std::vector<std::pair<std::string, std::string>>;  // (class, name) per slot; name "" a gap

	void load()
	{
		if (m_loaded)
			return;
		m_loaded = true;
		std::ifstream in(m_path);
		if (!in)
			throw Error("idakit: cannot read " + m_path);
		struct FieldList
		{
			std::vector<uint32_t> bases;
			std::vector<Method> methods;
			std::vector<std::pair<std::string, uint32_t>> lists;  // LF_METHOD: a name's overload list
		};
		std::unordered_map<uint32_t, std::string> names;  // a class's type index (its forward ref's too) -> name
		std::vector<std::pair<std::string, uint32_t>> definitions;  // class -> its field list, in dump order
		std::unordered_map<uint32_t, FieldList> fieldLists;
		std::unordered_map<uint32_t, std::vector<Method>> methodLists;
		auto index = [](const std::string& text, size_t at) {
			return static_cast<uint32_t>(std::stoul(text.substr(at), nullptr, 16));
		};
		auto quoted = [](const std::string& text) {
			size_t a = text.find('`'), b = text.rfind('`');
			return a == std::string::npos || b <= a ? std::string() : text.substr(a + 1, b - a - 1);
		};
		// `type = 0x.., vftable offset = N, attrs = ...`
		auto method = [](const std::string& text, Method& m) {
			size_t at = text.find("vftable offset = ");
			if (at == std::string::npos)
				return false;
			m.vtoff = std::stoll(text.substr(at + 17));
			size_t attrs = text.find("attrs = ", at);
			m.isVirtual = attrs != std::string::npos && text.find("virtual", attrs) != std::string::npos;
			return true;
		};
		std::string line, kind, className, pendingMethod;
		uint32_t current = 0, classFields = 0;
		bool forward = false, pendingBase = false;
		auto endClass = [&]() {
			if ((kind == "LF_CLASS" || kind == "LF_STRUCTURE") && !forward && classFields)
				definitions.emplace_back(className, classFields);
		};
		while (std::getline(in, line))
		{
			size_t start = line.find_first_not_of(' ');
			if (start == std::string::npos)
				continue;
			size_t bar = line.find(" | LF_", start);
			if (line.compare(start, 2, "0x") == 0 && bar != std::string::npos)
			{
				endClass();
				current = index(line, start + 2);
				size_t space = line.find(' ', bar + 3);
				kind = line.substr(bar + 3, space - bar - 3);
				className = kind == "LF_CLASS" || kind == "LF_STRUCTURE" ? quoted(line) : "";
				if (!className.empty())
					names[current] = className;
				forward = pendingBase = false;
				classFields = 0;
				pendingMethod.clear();
				continue;
			}
			std::string text = line.substr(start);
			if (kind == "LF_CLASS" || kind == "LF_STRUCTURE")
			{
				if (size_t at = text.find("field list: 0x"); at != std::string::npos)
					classFields = index(text, at + 14);
				if (text.find("forward ref") != std::string::npos)
					forward = true;
			}
			else if (kind == "LF_FIELDLIST")
			{
				FieldList& list = fieldLists[current];
				if (text == "- LF_BCLASS")
					pendingBase = true;
				else if (pendingBase && text.starts_with("type = 0x"))
				{
					list.bases.push_back(index(text, 9));
					pendingBase = false;
				}
				else if (text.starts_with("- LF_ONEMETHOD [name = "))
					pendingMethod = quoted(text);
				else if (!pendingMethod.empty() && text.starts_with("type = 0x"))
				{
					Method m{pendingMethod};
					if (method(text, m))
						list.methods.push_back(m);
					pendingMethod.clear();
				}
				else if (text.starts_with("- LF_METHOD [name = "))
				{
					if (size_t at = text.find("overload list = 0x"); at != std::string::npos)
						list.lists.emplace_back(quoted(text), index(text, at + 18));
				}
				else
					pendingBase = false, pendingMethod.clear();
			}
			else if (kind == "LF_METHODLIST" && text.starts_with("- Method [type = 0x"))
			{
				Method m;
				if (method(text, m))
					methodLists[current].push_back(m);
			}
		}
		endClass();
		for (const auto& [name, fields] : definitions)
		{
			auto list = fieldLists.find(fields);
			if (list == fieldLists.end() || m_classes.count(name))
				continue;
			Class& c = m_classes[name];
			for (uint32_t base : list->second.bases)
				if (auto found = names.find(base); found != names.end())
					c.bases.push_back(found->second);
			c.methods = list->second.methods;
			for (const auto& [methodName, overloads] : list->second.lists)
				if (auto found = methodLists.find(overloads); found != methodLists.end())
					for (Method m : found->second)
					{
						m.name = methodName;
						c.methods.push_back(m);
					}
		}
	}
	const Table& table(const std::string& cls, int depth)
	{
		if (auto found = m_tables.find(cls); found != m_tables.end())
			return found->second;
		Table slots;
		auto c = m_classes.find(cls);
		if (c != m_classes.end() && depth < 32)
		{
			for (const std::string& base : c->second.bases)
				if (const Table& t = table(base, depth + 1); !t.empty())
				{
					slots = t;
					m_primary[cls] = base;
					break;
				}
			std::vector<std::pair<int64_t, std::string>> introduced;
			std::set<std::string> declared;
			for (const Method& m : c->second.methods)
				if (m.isVirtual)
				{
					declared.insert(m.name);
					if (m.vtoff >= 0)
						introduced.emplace_back(m.vtoff / 4, m.name);
				}
			std::sort(introduced.begin(), introduced.end());
			for (const auto& [k, name] : introduced)
			{
				if (k < static_cast<int64_t>(slots.size()))
					continue;
				while (static_cast<int64_t>(slots.size()) < k)
					slots.emplace_back(cls, "");
				slots.emplace_back(cls, name);
			}
			for (auto& [owner, name] : slots)
				if (declared.count(name))
					owner = cls;
		}
		return m_tables[cls] = std::move(slots);
	}
	std::vector<std::string> subobjects(const std::string& cls, int depth)
	{
		std::vector<std::string> out;
		auto c = m_classes.find(cls);
		if (c == m_classes.end() || depth >= 32)
			return out;
		if (!table(cls, 0).empty())
			out.push_back(cls);
		auto primary = m_primary.find(cls);
		for (const std::string& base : c->second.bases)
		{
			std::vector<std::string> sub = subobjects(base, depth + 1);
			bool isPrimary = primary != m_primary.end() && primary->second == base;
			out.insert(out.end(), sub.begin() + (isPrimary && !sub.empty() ? 1 : 0), sub.end());
		}
		return out;
	}
	bool derives(const std::string& cls, const std::string& base, int depth)
	{
		if (cls == base)
			return true;
		auto c = m_classes.find(cls);
		return c != m_classes.end() && depth < 32 &&
			   std::any_of(c->second.bases.begin(), c->second.bases.end(),
						   [&](const std::string& b) { return derives(b, base, depth + 1); });
	}
	// cls or a class between it and base declaring a virtual of the name, breadth first; "" for none
	std::string overrider(const std::string& cls, const std::string& base, const std::string& name)
	{
		std::deque<std::string> todo{cls};
		std::set<std::string> seen;
		while (!todo.empty())
		{
			std::string c = todo.front();
			todo.pop_front();
			auto found = m_classes.find(c);
			if (!seen.insert(c).second || found == m_classes.end())
				continue;
			if (std::any_of(found->second.methods.begin(), found->second.methods.end(),
							[&](const Method& m) { return m.isVirtual && m.name == name; }))
				return c;
			for (const std::string& b : found->second.bases)
				if (derives(b, base, 0))
					todo.push_back(b);
		}
		return "";
	}

	std::string m_path;
	bool m_loaded = false;
	std::unordered_map<std::string, Class> m_classes;
	std::unordered_map<std::string, Table> m_tables;
	std::unordered_map<std::string, std::string> m_primary;  // a class's primary base (the first with a table)
};

class Declarer
{
public:
	Declarer(const SrcModel& model, SrcTypes& types, exefacts::Facts& facts, const ExeVtables& vtables,
			 const PdbSources& sources, SpecializationHomes& homes, const std::string& pdbTypes);

	// nullopt: src declares the address (it is not processed); "": to be declared; else why it is skipped
	std::optional<std::string> selection(uint64_t ea) const;
	// The address's declaration; throws Skip.
	Decl plan(uint64_t ea);
	// --folds: Y::g (qualified, as the PDB spells it), another source function the linker folded into the exe function
	// at ea, declared on Y as a forwarder to ea; chosen: g is the owner's name, which the PDB has no member of on Y
	// (the owner's signature); throws Skip
	Decl foldMember(uint64_t ea, const std::string& qualified, bool chosen = false);
	// --folds on an emit directory's calls (`call of O::f on R`): O::f a class template instance's member at ea, the
	// receiver of the src key R an object holding at its offset 0 (itself, a base, a by-value first field) another
	// instance of O's template, declared there as foldMember declares `Instance::f`; throws Skip
	Decl foldCall(uint64_t ea, const std::string& owner, const std::string& receiver);

	std::vector<NeedsClass> needs;
	// the kind of the function plan() looked at last (for a skipped row)
	const std::string& kind() const { return m_kind; }

private:
	// plan() but for IDA's noreturn
	Decl place(uint64_t ea);
	// the exe's facts plan() and foldMember() read (the bytes popped and read, m_pops, m_popsKnown, m_readBytes) and
	// IDA's prototype (in details; nullptr for none); throws Skip for a thunk, and for a usercall (m_usercall) whose
	// register prototype the exe contradicts (checkUsercall)
	const func_type_data_t* readExe(uint64_t ea, func_type_data_t& details);
	// IDA's __usercall/__userpurge prototype against the exe's code: its register arguments are exactly the registers
	// the code reads before writing them, its stack arguments the bytes the code reads and pops (the caller's cleanup
	// after a __usercall), its result the register its callers read; throws Skip naming the contradiction
	void checkUsercall(uint64_t ea, const func_type_data_t& fd);
	// rule 3 for a usercall: IDA's parameters in its order, each register argument in its register; a member's
	// `this` is IDA's first argument, in a register, a pointer to the class
	void userSignature(Decl& d, const func_type_data_t& fd);
	Name nameOf(uint64_t ea) const;
	// a qualified name (as nameOf cleans IDA's) read into its parts
	Name parsedName(const std::string& qualified) const;
	// the owners the `Same function as` comments at the address name (srcdb's fold refs' context)
	std::set<std::string> foldOwners(uint64_t ea) const;
	const SrcModel::Record* recordNamed(const std::string& name) const;
	// a template instance, or a record src models flat from a PDB template (`SettingCollectionMap` for the PDB's
	// SettingCollectionMap<Setting>): its members' definitions go in its own .cpp, not the TU the PDB names
	bool templated(const SrcModel::Record* r) const
	{
		return r && (!r->pattern.empty() || m_pdbTemplates.count(r->name));
	}
	const std::vector<const SrcModel::PdbFunction*>* pdbOverloads(const std::string& name) const;
	bool pdbClass(const std::string& name) const;
	std::string pdbSource(const std::string& name) const { return m_sources.source(name); }
	// An explicit specialization of a class template (`template <> class ctype<char> {`) as a class of its own: a
	// copy of the record without its pattern, as its members are declared and defined as any class's (no
	// `template <>`); any other record as it is
	const SrcModel::Record* asClass(const SrcModel::Record* r) const;
	// a class nested in a template instance: its members are specialized for the instance, as a template
	// instance's are (`template <> R Outer<A>::Inner::f(...)`)
	bool nestedInInstance(const SrcModel::Record& r) const
	{
		return r.pattern.empty() && r.name.find('<') != std::string::npos && !m_explicit.count(r.name);
	}
	// an explicit specialization's place: its home's header, line and .cpp (SpecializationHomes); throws Skip
	void home(Decl& d);

	Decl structor(Decl d, const SrcModel::Record& owner, const Name& n, const func_type_data_t* fd);
	Decl virtualSlot(Decl d, const Name& n, const SrcModel::Record* owner,
					 const std::vector<ExeVtables::Holder>& holders, const func_type_data_t* fd);
	// `this` of a member of the record's that src's vtable at the offset holds: `this`, or the base subobject at that
	// offset; nullopt where no non-virtual base starts there
	std::optional<std::string> subobjectSelf(const SrcModel::Record& record, int64_t offset) const;
	// the porting rules' `// Same function as owner (ea), registered there.` line right above the declaration of a
	// fold's other member copy, at its indentation; throws Skip where the declaration does not start its line
	LineInsert foldComment(const SrcModel::Function& copy, const SrcModel::Function& owner, uint64_t ea);
	// the same comment above each declaration src spells of the explicit specialization copy (its `template <>`
	// line), wrapped at 120 columns, where it is not there yet; throws Skip where src spells none in src/
	std::vector<LineInsert> specializationFoldComments(const SrcModel::Function& copy, const SrcModel::Function& owner,
													   uint64_t ea);
	// two slot functions of one class at a fold: why the PDB does not give src's name of the slot at index of the
	// record's src vtable at offset (porting rules "Folds"; the PDB's table of that subobject against src's, which is
	// the exe's: their longest common subsequence of qualified names holds src's slot); nullopt where it gives it
	std::optional<std::string> pdbSlotMismatch(const SrcModel::Record& record, int64_t offset, size_t index);
	// a chosen fold member's setter: the parameter the exe stores becomes the class's own src field at the offset it
	// stores to, not the owner's parameter; its index. Throws Skip where the class has no such field, or one IDA named
	std::optional<size_t> setterField(Decl& d, const SrcModel::Record& cls, const exefacts::Accessor& accessor);
	// the record or class's enum a type key names (through pointers, references, arrays and cv), as src spells it;
	// nullopt for a type of no class (a builtin, a pointer to one, a free enum)
	std::optional<std::string> classDependent(const std::string& key) const;
	// src registers a function of the qualified name by its name alone (an FNVD_REPLACES form without _OVERLOAD)
	bool registeredByName(const std::string& qualified) const { return m_registeredByName.count(qualified) != 0; }
	// the class has a nested type (record, enum, typedef) or an unscoped enumerator of the name
	bool typeMember(const std::string& cls, const std::string& name) const
	{
		return m_typeMembers.count(cls + "::" + name) != 0;
	}
	// a parameter of the name would hide the type src has of it
	bool typeName(const std::string& name) const
	{
		return m_model.record(name) || m_model.enumeration(name) || m_model.typedefNamed(name);
	}
	Decl member(Decl d, const SrcModel::Record& owner, const Name& n, const func_type_data_t* fd, bool idaOnly);
	Decl templateMember(Decl d, const SrcModel::Record& owner, const std::string& name, const Name& n,
						const func_type_data_t* fd);
	// An instance of a src class template srcdb has no record of (it records only the instances src uses), as the
	// PDB or IDA spells it: a record for it, the template's (file, lines) with the name spelled from src's names of
	// the arguments; nullptr with why `type not in src: <argument>` for an argument src lacks.
	const SrcModel::Record* instanceOf(const std::string& name, std::string& why) const;
	// a template argument as src spells it; nullopt with why for a type src lacks
	std::optional<std::string> argumentSpelling(std::string argument, std::string& why) const;
	// the template's declarations of a member, through an instance srcdb has when the owner is one it has not
	const std::vector<const SrcModel::Function*>& memberDeclarations(const SrcModel::Record& owner,
																	 const std::string& name) const;
	Decl unowned(Decl d, const Name& n, const func_type_data_t* fd);
	// the template declares the member with d's signature (its instance's types: src's record of a sibling instance
	// standing for the owner): d takes the declaration's types and const; false where none matches. defined: a
	// declaration the template defines counts too
	bool templateDeclares(Decl& d, const SrcModel::Record& owner, const std::string& name, bool defined = false) const;
	// src's declaration f, defined (or specialized) out of line; self: the call's object; slot: the exe's vtable
	// holds the address where src's vtable holds f (f defined: the tie's proof, addressed)
	Decl existing(Decl d, const SrcModel::Function& f, const std::string& self, bool slot = false);
	// why src's function f, which src defines, gets nothing for the address: the reason a skip gives
	std::string definedReason(const SrcModel::Function& f, uint64_t ea) const;
	// src's function f, defined and registered nowhere (a KEEP body), tied to the address by an FNVD_ADDRESS
	// assertion: where src's ABI agrees with the exe's, the member is accessible from namespace scope and the tie is
	// proven (tieProof); a class template instance's member src has the template's body of gets its explicit
	// specialization instead (instanceSpecialization; self: the call's object, a base subobject's cast for a
	// secondary vtable's entry); throws Skip
	Decl addressed(Decl d, const SrcModel::Function& f, bool slot, const std::string& self = "this");
	// Where src's declaration f (its ABI row) disagrees with the exe's code: pops, stack bytes read, ecx or edx read
	// under a convention that passes nothing there, an IDA usercall; "" where it agrees.
	std::string abiDisagreement(const SrcModel::Function& f) const;
	// The address as instance's member f, made from the template's body src has (a definition srcdb has as an
	// instance's): the explicit specialization of f (a C++ structor's: the template's Construct / Destruct) with its
	// `// Not reconstructed yet.` forwarder, at its home, src's declaration of f its signature; throws Skip with why
	// it cannot be
	Decl instanceSpecialization(Decl d, const SrcModel::Function& f, const SrcModel::Record& instance,
								const std::string& self);
	// The address as owner's member name, owner an instance srcdb has no record of: the explicit specialization of
	// the template's declaration of d's signature (templateDeclares, a defined one too); throws Skip
	Decl siblingSpecialization(Decl d, const SrcModel::Record& owner, const std::string& name,
							   const std::string& pdbName);
	// owner's member of the template's declarations, every one defined: of several, the one whose ABI fits the exe
	// (undefinedFit's choice among the defined), specialized (siblingSpecialization for an instance srcdb has no
	// record of)
	Decl instanceBody(Decl d, const SrcModel::Record& owner, const std::vector<const SrcModel::Function*>& declared,
					  const func_type_data_t* fd);
	// A fold of class template instances' members that src's refs name (chooseRef's names, porting rules "Folds"):
	// the owner (the instance the exe names at ea, else the alphabetically first) keeps or gets its explicit
	// specialization's forwarder, every other explicit specialization of them the `Same function as` comment; an
	// instance the template's own forwarder names gets nothing; throws Skip
	Decl instanceFold(Decl d, const RefChoice& choice);
	// what proves that src's definition of f is the exe's function at ea, "" for nothing (docs/IDAKIT.md "declare"):
	// slot (the exe's vtable), a comment at the definition naming the address, a `sub_<ADDR>` name
	std::string tieProof(const SrcModel::Function& f, const SrcModel::Definition& def, uint64_t ea, bool slot);
	// foldMember on a class template instance (porting rules "Folds", "Explicit specializations"): the instance's
	// member made from the same template declaration as the fold's owner at d.ea (src's declaration there, of
	// another instance of the template), as its explicit specialization at its home with the owner's `Same function
	// as` comment and a `// Not reconstructed yet.` forwarder; throws Skip
	Decl instanceFoldMember(Decl d, const SrcModel::Record& instance, const std::string& name);
	// why the exe's code at a fold of owner (of instance a) cannot be member's (of instance b, made from the same
	// template declaration): the instances' layouts differ in a size the code walks (a field's, a pointer field's
	// pointee's: the item stride), or a parameter or the return passes other bits or points to items of another
	// size; "" where they agree
	std::string instanceMismatch(const SrcModel::Record& a, const SrcModel::Record& b, const SrcModel::Function& owner,
								 const SrcModel::Function& member) const;
	// rule 1: the one declaration of the name without a definition whose ABI fits, of several the one whose
	// parameter types are IDA's prototype's (fd, may be null); nullptr for none. defined: of the declarations with a
	// definition instead (a template instance's overloads registered)
	const SrcModel::Function* undefinedFit(const std::string& qualified, const func_type_data_t* fd,
										   bool defined = false);
	// IDA's prototype's parameter types as keys, a thiscall's `this` aside; nullopt where one is no src key
	std::optional<std::vector<Param>> idbParams(const func_type_data_t* fd) const;
	// rule 2: the PDB's overload that fits the exe; false where the PDB has none of the name
	bool pdbSignature(Decl& d, const std::string& name, bool member, const func_type_data_t* fd);
	// rule 3, ahead of IDA's prototype: the function type of src's FNVD_ORIGINAL_FUNCTION constants at the address
	// (their one pointer-to-function type), where the exe's ABI agrees with it, the convention the exe's; false where
	// none applies
	bool constantSignature(Decl& d, const func_type_data_t* fd);
	// rule 3: the constant's type (constantSignature), else IDA's prototype (the exe's where the exe contradicts it),
	// `this` left out of a thiscall one
	void idbSignature(Decl& d, const func_type_data_t* fd);
	// the convention of a static or free function from the exe, and the fit of its parameters
	void freeConvention(Decl& d, const func_type_data_t* fd);
	// a declaration's stack bytes are what the exe pops (any, where the exe's code leaves them to an unknown
	// tail-jump target)
	bool popsFit(int64_t bytes) const { return !m_popsKnown || bytes == m_pops; }
	// a member name that clashes with nothing; throws Skip
	void checkMemberName(const Decl& d, const SrcModel::Record& owner);
	// no class deriving from owner would find the new member's name in two bases; throws Skip
	void checkDerivedLookups(const Decl& d, const SrcModel::Record& owner);
	void placeMember(Decl& d, const SrcModel::Record& owner, const std::string& pdbName);
	void placeFree(Decl& d, const std::string& pdbName);

	// the key in src's records (a PDB key's names as src spells them); nullopt where src lacks a type, why: which
	std::optional<std::string> srcKey(const std::string& key, std::string& why) const;
	std::optional<int64_t> size(const std::string& key) const;
	int64_t stackBytes(const Decl& d) const;
	std::string paramKeys(const std::vector<Param>& params, size_t from = 0) const;
	// paramKeys as IDA's prototypes have them: a reference as the pointer it is passed as
	std::string idbKeys(const std::vector<Param>& params, size_t from = 0) const;

	const SrcModel& m_model;
	SrcTypes& m_types;
	exefacts::Facts& m_facts;
	const ExeVtables& m_vtables;
	uint32_t m_pops = 0;
	bool m_usercall = false;  // IDA types the function __usercall/__userpurge, and the exe agrees (checkUsercall)
	AsmFacts m_asm;           // the callers' cleanup and result reads (checkUsercall)
	bool m_popsKnown = true;  // the exe proves m_pops (exefacts' pops)
	int64_t m_readBytes = 0;  // the stack argument bytes the code reads, up to the highest slot it reads
	bool m_readsEcx = false, m_readsEdx = false;  // the code reads ecx / edx before writing it (exefacts' abi)
	FileEdits m_text;  // src's files as read, for tieProof (never edited)
	std::string m_kind;
	mutable std::unordered_map<std::string, const SrcModel::Record*> m_owners;  // recordNamed's answers
	std::map<std::string, const SrcModel::Record*> m_patterns;  // each class template src has instances of -> one
	// the instances srcdb has no record of that this run spells (instanceOf), each with an instance it has
	mutable std::deque<SrcModel::Record> m_synthetic;
	mutable std::map<const SrcModel::Record*, const SrcModel::Record*> m_siblings;
	mutable std::map<std::string, const SrcModel::Record*> m_instances;  // by src spelling
	std::set<std::string> m_pdbTemplates;  // the PDB's class templates (its classes' names before `<`)
	std::unordered_map<std::string, std::vector<const SrcModel::PdbFunction*>> m_pdbByKey;
	std::set<std::string> m_pdbClasses;  // by classKey
	const PdbSources& m_sources;
	SpecializationHomes& m_homes;
	PdbVtables m_pdbVtables;
	std::unordered_map<std::string, std::set<uint64_t>> m_addressesOf;  // src function (mangled) -> its refs' addresses
	std::map<std::string, uint64_t> m_claimed;  // src declarations this run defines, -> the address
	std::map<std::string, uint64_t> m_newSignatures;  // owner::name(params) this run declares -> the address
	std::unordered_set<std::string> m_registeredByName;  // qualified names (registeredByName)
	std::unordered_set<std::string> m_typeMembers;       // Class::Name of the classes' nested types and enumerators
	// the namespaces src's names are in (every scope of a record, enum, typedef or function name that is no record)
	std::unordered_set<std::string> m_namespaces;
	mutable std::map<const SrcModel::Record*, const SrcModel::Record*> m_classes;  // asClass's answers
	mutable std::unordered_set<std::string> m_explicit;  // explicit class template specializations by name
	// the records by ExeVtables::classKey (nullptr for a key two records share)
	std::unordered_map<std::string, const SrcModel::Record*> m_recordsByKey;
};

Declarer::Declarer(const SrcModel& model, SrcTypes& types, exefacts::Facts& facts, const ExeVtables& vtables,
				   const PdbSources& sources, SpecializationHomes& homes, const std::string& pdbTypes)
	: m_model(model), m_types(types), m_facts(facts), m_vtables(vtables), m_sources(sources), m_homes(homes),
	  m_pdbVtables(pdbTypes)
{
	for (const auto& [name, record] : model.records())
	{
		if (!record.pattern.empty() && asClass(&record) == &record)
			m_patterns.emplace(record.pattern, &record);
		if (auto [at, added] = m_recordsByKey.emplace(ExeVtables::classKey(name), &record); !added)
			at->second = nullptr;
	}
	for (const auto& [name, overloads] : model.pdbFunctions())
		for (const SrcModel::PdbFunction& f : overloads)
		{
			m_pdbByKey[ExeVtables::classKey(name)].push_back(&f);
			if (!f.cls.empty())
				m_pdbClasses.insert(ExeVtables::classKey(f.cls));
			if (size_t open = f.cls.find('<'); open != std::string::npos)
				m_pdbTemplates.insert(f.cls.substr(0, open));
		}
	for (const auto& [ea, refs] : model.refs())
		for (const SrcModel::Ref& r : refs)
			if (!r.function.empty() && (r.kind == "body" || r.kind == "forwarder") && !placeholder(r))
			{
				m_addressesOf[r.function].insert(ea);
				if (const SrcModel::Function* f = model.function(r.function);
					f && r.kind == "body" && !r.form.ends_with("OVERLOAD"))
					m_registeredByName.insert(f->name);
			}
	for (const auto& [name, record] : model.records())
		m_typeMembers.insert(name);
	for (const auto& [name, e] : model.enums())
	{
		m_typeMembers.insert(name);
		if (!e.scoped && !scopeOf(name).empty())
			for (const SrcModel::Enumerator& v : e.enumerators)
				m_typeMembers.insert(scopeOf(name) + "::" + v.name);
	}
	for (const auto& [name, t] : model.typedefs())
		m_typeMembers.insert(name);
	auto scopes = [&](const std::string& name) {
		for (std::string s = scopeOf(name); !s.empty(); s = scopeOf(s))
			if (!model.record(s))
				m_namespaces.insert(s);
	};
	for (const auto& [name, record] : model.records())
		scopes(name);
	for (const auto& [name, e] : model.enums())
		scopes(name);
	for (const auto& [name, t] : model.typedefs())
		scopes(name);
	for (const auto& [mangled, f] : model.functions())
		if (f.cls.empty())
			scopes(f.name);
}

std::optional<std::string> Declarer::selection(uint64_t ea) const
{
	const std::vector<SrcModel::Ref>& refs = m_model.refsAt(ea);
	for (const SrcModel::Ref& r : refs)
		if (r.kind == "constructor" || r.kind == "destructor")
			return std::nullopt;
	RefChoice choice = chooseRef(m_model, ea);
	// a class template member's own forwarder for an instance src completes: the instance's explicit specialization is
	// to be declared (instanceFold)
	if (choice.ref)
		return choice.ref->generic && !choice.ref->function.empty() ? std::optional<std::string>("") : std::nullopt;
	// several src functions, one of them a template's instance: their fold (instanceFold); any other several, the
	// porter's
	if (choice.names.size() > 1)
	{
		bool instances = std::any_of(choice.named.begin(), choice.named.end(), [&](const SrcModel::Ref* r) {
			const SrcModel::Function* f = r->function.empty() ? nullptr : m_model.function(r->function);
			const SrcModel::Record* owner = f ? asClass(m_model.record(f->cls)) : nullptr;
			return f && ((owner && (!owner->pattern.empty() || nestedInInstance(*owner))) ||
						 (f->cls.empty() && m_model.specializations().count(f->mangled)));
		});
		return instances ? "" : "several src functions name it";
	}
	bool folded = false;
	for (const SrcModel::Ref& r : refs)
	{
		if (placeholder(r))
			continue;
		folded = folded || r.kind == "fold";
		if (r.kind == "address")
			return "src keeps an unregistered body for it (FNVD_ADDRESS)";
		// A forwarder ref inside a function src registers at another address is a call that body makes (a port's or
		// a marked body's raw `fnvd::original` call): it does not name the address (chooseRef), which is declared as
		// any other. Only when the calling function bears the address's own name is the address a second copy of a
		// function src already has (registered at the other address), which gets no second declaration.
		if (r.kind == "forwarder" && !r.target.empty())
			for (uint64_t other : m_model.bodyAddresses(r))
				if (other != ea && msvc9Names(r.target) == msvc9Names(nameOf(ea).qualified))
					return "forwarder's function registered at " + hexText(other);
	}
	// `Same function as` comments whose owner names no declaration at the address (chooseRef names none): src lacks
	// the owner, which is declared under the comments' name (place); comments naming several owners are the porter's
	if (folded)
		if (std::set<std::string> owners = foldOwners(ea); owners.size() != 1)
			return "fold comments name " + std::to_string(owners.size()) + " owners";
	return "";
}

std::set<std::string> Declarer::foldOwners(uint64_t ea) const
{
	std::set<std::string> owners;
	for (const SrcModel::Ref& r : m_model.refsAt(ea))
		if (r.kind == "fold" && !placeholder(r))
			owners.insert(r.context);
	return owners;
}

// The src record of a class as the PDB or IDA spells it: the instance or class of that name, else what SrcTypes takes
// IDA's struct of that name for (ida_type_aliases.tsv, a record src models flat from a PDB template), the one
// mapping emit and sync use; a std:: name as its msvc9:: one; else the one record of its spelling-insensitive key
// (the demangler's `NiTMap<char const *,__int64>` is src's `NiTMap<const char *, long long>`).
const SrcModel::Record* Declarer::recordNamed(const std::string& name) const
{
	if (const SrcModel::Record* r = m_model.record(name))
		return asClass(r);
	auto [cached, added] = m_owners.try_emplace(name, nullptr);
	if (added)
	{
		cached->second = m_types.recordOfClass(name);
		if (std::string msvc9 = msvc9Names(name); !cached->second && msvc9 != name)
		{
			const SrcModel::Record* r = m_model.record(msvc9);
			cached->second = r ? r : m_types.recordOfClass(msvc9);
		}
		if (!cached->second)
			if (auto byKey = m_recordsByKey.find(ExeVtables::classKey(name)); byKey != m_recordsByKey.end())
				cached->second = byKey->second;
	}
	return asClass(cached->second);
}

const SrcModel::Record* Declarer::asClass(const SrcModel::Record* r) const
{
	if (!r || r->pattern.empty() || r->line <= 0)
		return r;
	auto [cached, added] = m_classes.try_emplace(r, r);
	if (added && SrcModel::explicitSpecialization(*r))
	{
		SrcModel::Record copy = *r;
		copy.pattern.clear();
		cached->second = &m_synthetic.emplace_back(std::move(copy));
		m_explicit.insert(r->name);
	}
	return cached->second;
}

std::optional<std::string> Declarer::argumentSpelling(std::string a, std::string& why) const
{
	a = std::regex_replace(a, std::regex(R"(\b(?:class|struct|union|enum) )"), "");
	a = std::regex_replace(a, std::regex(R"(\s*__far\b)"), "");
	size_t first = a.find_first_not_of(' '), last = a.find_last_not_of(' ');
	if (first == std::string::npos)
		return std::nullopt;
	a = a.substr(first, last - first + 1);
	// the declarator's tail (`*`, `&`, ` const`) and a leading `const`
	std::string prefix, suffix;
	if (a.rfind("const ", 0) == 0)
		prefix = "const ", a.erase(0, 6);
	while (!a.empty())
	{
		if (a.back() == '*' || a.back() == '&')
		{
			suffix.insert(suffix.begin(), a.back());
			a.pop_back();
		}
		else if (a.back() == ' ')
			a.pop_back();
		else if (a.size() > 6 && a.compare(a.size() - 6, 6, " const") == 0)
		{
			// `X const` is const X; `X * const` a const pointer
			a.erase(a.size() - 6);
			size_t end = a.find_last_not_of(' ');
			if (end != std::string::npos && (a[end] == '*' || a[end] == '&'))
				suffix.insert(0, " const");
			else
				prefix = "const ";
		}
		else
			break;
	}
	if (!suffix.empty())
		suffix = " " + suffix;
	// srcdb's record names spell builtins as clang does (`NiTMap<unsigned int, TESFile *>`); IDA's and Windows'
	// typedef names for the integers (`NiTMap<uint,TESFile *>`, `BSMap<DWORD,bool>`) are those builtins (RULES'
	// fixed-width types, which src's instances name), Hex-Rays' `_DWORD` the unsigned one of its size
	static const std::map<std::string, std::string> builtins = {
		{"bool", "bool"}, {"char", "char"}, {"signed char", "signed char"}, {"unsigned char", "unsigned char"},
		{"short", "short"}, {"unsigned short", "unsigned short"}, {"int", "int"}, {"unsigned int", "unsigned int"},
		{"long", "long"}, {"unsigned long", "unsigned long"}, {"float", "float"}, {"double", "double"},
		{"void", "void"}, {"__int64", "long long"}, {"unsigned __int64", "unsigned long long"},
		{"wchar_t", "wchar_t"}, {"uchar", "unsigned char"}, {"ushort", "unsigned short"}, {"uint", "unsigned int"},
		{"BYTE", "unsigned char"}, {"WORD", "unsigned short"}, {"DWORD", "unsigned int"}, {"_BYTE", "unsigned char"},
		{"_WORD", "unsigned short"}, {"_DWORD", "unsigned int"}, {"_QWORD", "unsigned long long"},
		{"__int8", "signed char"}, {"__int16", "short"}, {"__int32", "int"}, {"unsigned __int8", "unsigned char"},
		{"unsigned __int16", "unsigned short"}, {"unsigned __int32", "unsigned int"}};
	std::string base;
	a = msvc9Names(a);
	if (a == "true" || a == "false" ||
		std::all_of(a.begin(), a.end(), [](char c) { return std::isdigit(static_cast<unsigned char>(c)) || c == '-'; }))
		base = a;
	else if (auto builtin = builtins.find(a); builtin != builtins.end())
		base = builtin->second;
	else if (const SrcModel::Record* r = recordNamed(a))
		base = r->name;
	else if (m_model.enumeration(a) || m_model.typedefNamed(a))
		base = a;
	else if (size_t open = a.find('<'); open != std::string::npos && a.back() == '>' &&
										m_patterns.count(a.substr(0, open)))
	{
		const SrcModel::Record* r = instanceOf(a, why);
		if (!r)
			return std::nullopt;
		base = r->name;
	}
	else
	{
		why = "type not in src: " + a;
		return std::nullopt;
	}
	return prefix + base + suffix;
}

const SrcModel::Record* Declarer::instanceOf(const std::string& name, std::string& why) const
{
	size_t open = name.find('<');
	auto pattern = m_patterns.find(name.substr(0, open));
	if (pattern == m_patterns.end() || name.back() != '>')
		return nullptr;
	// the arguments at the top level
	std::string inside = name.substr(open + 1, name.size() - open - 2), spelled;
	int depth = 0;
	size_t start = 0;
	for (size_t k = 0; k <= inside.size(); ++k)
	{
		char c = k < inside.size() ? inside[k] : ',';
		depth += c == '<' || c == '(' ? 1 : c == '>' || c == ')' ? -1 : 0;
		if (c != ',' || depth != 0)
			continue;
		std::optional<std::string> argument = argumentSpelling(inside.substr(start, k - start), why);
		if (!argument)
			return nullptr;
		spelled += (spelled.empty() ? "" : ", ") + *argument;
		start = k + 1;
	}
	std::string instance = pattern->first + "<" + spelled + ">";
	if (const SrcModel::Record* r = m_model.record(instance))
		return asClass(r);
	if (auto found = m_instances.find(instance); found != m_instances.end())
		return found->second;
	const SrcModel::Record& sibling = *pattern->second;
	// as many arguments as src's template takes (the PDB's and IDA's may be another template's: the PDB's
	// NiTPointerListBase<Allocator, T> is src's NiTPointerListBase<T>), or as src's spelling of the sibling leaves
	// its default arguments out
	auto arguments = [](const std::string& text) {
		size_t count = 1;
		int level = 0;
		for (size_t k = text.find('<') + 1; k < text.size(); ++k)
		{
			level += text[k] == '<' || text[k] == '(' ? 1 : text[k] == '>' || text[k] == ')' ? -1 : 0;
			count += text[k] == ',' && level == 0;
		}
		return count;
	};
	size_t count = arguments(instance);
	if (count != arguments(sibling.name) && (sibling.spelled.empty() || count != arguments(sibling.spelled)))
	{
		why = "template arguments: " + std::to_string(count) + ", src's " + pattern->first + " takes " +
			  std::to_string(arguments(sibling.name));
		return nullptr;
	}
	SrcModel::Record r;
	r.name = instance;
	r.file = sibling.file;
	r.tag = sibling.tag;
	r.pattern = sibling.pattern;
	r.line = sibling.line;
	r.endLine = sibling.endLine;
	const SrcModel::Record* added = &m_synthetic.emplace_back(std::move(r));
	m_siblings.emplace(added, &sibling);
	m_instances.emplace(instance, added);
	return added;
}

const std::vector<const SrcModel::Function*>& Declarer::memberDeclarations(const SrcModel::Record& owner,
																		   const std::string& name) const
{
	auto sibling = m_siblings.find(&owner);
	return m_model.functionsNamed((sibling == m_siblings.end() ? owner.name : sibling->second->name) + "::" + name);
}

bool Declarer::templateDeclares(Decl& d, const SrcModel::Record& owner, const std::string& name, bool defined) const
{
	auto sibling = m_siblings.find(&owner);
	const std::string& from = sibling == m_siblings.end() ? owner.name : sibling->second->name;
	auto instance = [&](std::string key) {
		for (size_t at = key.find(from); at != std::string::npos; at = key.find(from, at + owner.name.size()))
			key.replace(at, from.size(), owner.name);
		return key;
	};
	for (const SrcModel::Function* f : memberDeclarations(owner, name))
	{
		const SrcModel::Type* t = m_model.type(f->type);
		if (!t || (!defined && m_model.definition(f->mangled)) || f->isStatic != d.isStatic ||
			t->params.size() != d.params.size() || m_types.canonical(instance(t->ret)) != m_types.canonical(d.ret))
			continue;
		bool same = true;
		for (size_t k = 0; k < t->params.size() && same; ++k)
			same = m_types.canonical(instance(t->params[k])) == m_types.canonical(d.params[k].key);
		if (!same)
			continue;
		d.ret = instance(t->ret);
		for (size_t k = 0; k < t->params.size(); ++k)
			d.params[k].key = instance(t->params[k]);
		d.isConst = t->isConst;
		return true;
	}
	return false;
}

const std::vector<const SrcModel::PdbFunction*>* Declarer::pdbOverloads(const std::string& name) const
{
	auto found = m_pdbByKey.find(ExeVtables::classKey(name));
	return found == m_pdbByKey.end() ? nullptr : &found->second;
}

bool Declarer::pdbClass(const std::string& name) const
{
	return m_pdbClasses.count(ExeVtables::classKey(name)) != 0;
}

void Declarer::home(Decl& d)
{
	std::vector<std::string> keys{d.ret};
	for (const Param& p : d.params)
		keys.push_back(p.key);
	SpecializationHomes::Result found = m_homes.specializationHome(*d.owner, keys);
	if (const std::string* why = std::get_if<std::string>(&found))
		throw Skip{*why};
	const SpecializationHome& at = std::get<SpecializationHome>(found);
	d.header = at.header;
	d.after = at.after;
	d.source = at.source;
	d.homeIncludes = at.includes;
}

Name Declarer::nameOf(uint64_t ea) const
{
	Name n;
	std::string raw = get_name(ea).c_str();
	static const std::regex sub("_*sub_[0-9A-Fa-f]+");
	if (std::regex_match(raw, sub))
	{
		n.unnamed = true;
		return n;
	}
	std::string q = pdbSpelling(ea);
	// a name IDA holds as full demangled text (`public: void __cdecl X<A>::f(bool (__cdecl *)(A *))`): the qualified
	// name before its argument list, after the access, return type and convention
	if (size_t close = q.rfind(')'); close != std::string::npos && q.back() == ')')
	{
		int depth = 0;
		size_t open = std::string::npos;
		for (size_t k = close + 1; k-- > 0;)
			if (q[k] == ')')
				++depth;
			else if (q[k] == '(' && --depth == 0)
			{
				open = k;
				break;
			}
		if (open != std::string::npos)
		{
			std::string head = q.substr(0, open);
			int angles = 0;
			size_t start = 0;
			for (size_t k = 0; k < head.size(); ++k)
			{
				angles += head[k] == '<' ? 1 : head[k] == '>' ? -1 : 0;
				if (head[k] == ' ' && angles == 0 && head.compare(k + 1, 8, "operator") != 0 &&
					!(k >= 8 && head.compare(k - 8, 8, "operator") == 0))
					start = k + 1;
			}
			q = head.substr(start);
		}
	}
	// IDA's short demangling of a const member, and of a secondary vtable's entry (`X::f{for `B'}`)
	if (q.rfind("const ", 0) == 0)
		q.erase(0, 6);
	if (size_t brace = q.find("{for "); brace != std::string::npos)
		q.erase(brace);
	return parsedName(q);
}

Name Declarer::parsedName(const std::string& q) const
{
	Name n;
	n.qualified = q;
	n.last = lastComponent(q);
	n.scope = scopeOf(q);
	// an IDA or community placeholder the PDB does not have (`.text_X`, `j_X`, `FuncNNNN`,
	// `Unk_N`, `Class::9904C0`, ...) names nothing: the function is unnamed, `sub_X` with the chosen-name marker
	static const std::regex placeholder(
		R"(_?sub_[0-9A-F]+(_\d+)?|\.text_[0-9A-F]+|nullsub_\d+|j_.*|unknown_libname_\d+|loc_[0-9A-F]+|)"
		R"(Func[0-9A-F]{3,4}|Unk_?[0-9A-Fa-f]+|unk_?[0-9A-Fa-f]*|[A-Z]{1,2}[0-9A-F]{5,6}|[0-9A-F]{6}|au_re_.*|)"
		R"(def_[0-9A-F]+|.*_[0-9A-F]{6}(_\d+)?|return_\w+|\.sub_.*|_num_filtered_.*)");
	if (std::regex_match(n.last, placeholder) && !pdbOverloads(q))
		n.unnamed = true;
	if (!n.scope.empty())
	{
		// IDA's `_N` on a second address of a name: `X::X_0` is a constructor too
		std::string bare = withoutSuffix(n.last).value_or(n.last);
		std::string owner = lastComponent(n.scope);
		owner = owner.substr(0, owner.find('<'));
		if (n.last == "`scalar deleting destructor'" || n.last == "`vector deleting destructor'")
			n.structor = "deleting";
		else if (n.last.starts_with('~'))
			n.structor = "dtor";
		else if (constructorName(q) || bare == owner)
			n.structor = "ctor";
	}
	return n;
}

std::optional<std::string> Declarer::srcKey(const std::string& key, std::string& why) const
{
	SrcTypes::Parts p = m_types.parts(key);
	if (p.kind == "builtin")
		return key;
	if (p.kind == "record")
	{
		if (const SrcModel::Record* r = recordNamed(p.name))
			return "record:" + r->name;
		// an instance of a src template srcdb has no record of
		std::string inner, name = msvc9Names(p.name);
		if (size_t open = name.find('<'); open != std::string::npos && name.back() == '>' &&
										  m_patterns.count(name.substr(0, open)))
			if (const SrcModel::Record* r = instanceOf(name, inner))
				return "record:" + r->name;
		why = inner.empty() ? "type not in src: " + p.name : inner;
		return std::nullopt;
	}
	if (p.kind == "enum")
	{
		if (const SrcModel::Enum* e = m_types.enumOf(p.name))
			return "enum:" + e->name;
		why = "type not in src: enum " + p.name;
		return std::nullopt;
	}
	if (p.kind == "typedef")
	{
		if (m_model.typedefNamed(p.name))
			return key;
		why = "type not in src: typedef " + p.name;
		return std::nullopt;
	}
	if (p.kind == "ptr" || p.kind == "lref" || p.kind == "const" || p.kind == "volatile")
	{
		std::optional<std::string> of = srcKey(p.of, why);
		return of ? std::optional<std::string>(p.kind + "(" + *of + ")") : std::nullopt;
	}
	if (p.kind == "array")
	{
		std::optional<std::string> of = srcKey(p.of, why);
		return of ? std::optional<std::string>("array(" + *of + "," + std::to_string(p.count) + ")") : std::nullopt;
	}
	if (std::optional<SrcTypes::Function> f = SrcTypes::function(key))
	{
		std::optional<std::string> ret = srcKey(f->ret, why);
		if (!ret)
			return std::nullopt;
		f->ret = *ret;
		for (std::string& param : f->params)
		{
			std::optional<std::string> s = srcKey(param, why);
			if (!s)
				return std::nullopt;
			param = *s;
		}
		return SrcTypes::functionKey(*f);
	}
	why = "type src cannot spell: " + key;
	return std::nullopt;
}

std::optional<int64_t> Declarer::size(const std::string& key) const
{
	if (std::optional<int64_t> known = m_model.sizeOf(key))
		return known;
	SrcTypes::Parts p = m_types.parts(key);
	static const std::map<std::string, int64_t> builtins = {
		{"bool", 1},		 {"char", 1},		 {"signed char", 1},	{"unsigned char", 1}, {"short", 2},
		{"unsigned short", 2}, {"int", 4},	   {"unsigned int", 4}, {"long", 4},		  {"unsigned long", 4},
		{"float", 4},		   {"double", 8},	   {"long long", 8},	{"unsigned long long", 8}, {"wchar_t", 2}};
	if (p.kind == "builtin")
	{
		auto found = builtins.find(p.name);
		return found == builtins.end() ? std::nullopt : std::optional<int64_t>(found->second);
	}
	if (p.kind == "ptr" || p.kind == "lref" || p.kind == "rref")
		return 4;
	if (p.kind == "const" || p.kind == "volatile")
		return size(p.of);
	if (p.kind == "record")
	{
		const SrcModel::Record* r = m_model.record(p.name);
		return r ? std::optional<int64_t>(r->size) : std::nullopt;
	}
	if (p.kind == "enum")
	{
		const SrcModel::Enum* e = m_model.enumeration(p.name);
		return e ? size(e->underlying) : std::nullopt;
	}
	if (p.kind == "typedef")
	{
		const SrcModel::Typedef* t = m_model.typedefNamed(p.name);
		return t ? size(t->target) : std::nullopt;
	}
	return std::nullopt;
}

// The stack bytes the parameters take after the register ones (thiscall: `this` aside, a static one's first in ecx;
// fastcall: the first two of four bytes or fewer that are no float or record; usercall: those IDA puts in registers).
int64_t Declarer::stackBytes(const Decl& d) const
{
	int64_t bytes = 0;
	size_t registers = d.cc == "thiscall" && d.isStatic ? 1 : 0;
	for (size_t k = 0; k < d.params.size(); ++k)
	{
		std::optional<int64_t> s = size(d.params[k].key);
		if (!s)
			throw Skip{"size unknown: " + d.params[k].key};
		SrcTypes::Parts p = m_types.parts(m_types.canonical(d.params[k].key));
		bool fits = *s <= 4 && p.kind != "record" && !(p.kind == "builtin" && (p.name == "float" || p.name == "double"));
		if (k < registers || d.registers.count(d.params[k].name) ||
			(d.cc == "fastcall" && registers < 2 && fits && ++registers))
			continue;
		bytes += (*s + 3) & ~int64_t(3);
	}
	return bytes;
}

std::string Declarer::paramKeys(const std::vector<Param>& params, size_t from) const
{
	std::string out;
	for (size_t k = from; k < params.size(); ++k)
		out += m_types.canonical(params[k].key) + ",";
	return out;
}

std::string Declarer::idbKeys(const std::vector<Param>& params, size_t from) const
{
	std::string out;
	for (size_t k = from; k < params.size(); ++k)
	{
		std::string key = m_types.canonical(params[k].key);
		if (SrcTypes::Parts p = m_types.parts(key); p.kind == "lref")
			key = "ptr(" + p.of + ")";
		out += key + ",";
	}
	return out;
}

const func_type_data_t* Declarer::readExe(uint64_t ea, func_type_data_t& details)
{
	m_kind.clear();
	m_usercall = false;
	// thunks: compiler-made (an adjustor: `sub ecx, N; jmp`, a stdcall's `sub dword ptr [esp+4], N; jmp`), or a jump
	// to the real body
	if (ExeVtables::thunk(ea))
		throw Skip{"adjustor thunk"};
	insn_t insn;
	if (decode_insn(&insn, ea) > 0 && (insn.itype == NN_jmp || insn.itype == NN_jmpni || insn.itype == NN_jmpfi))
		throw Skip{"jump thunk"};
	try
	{
		std::optional<uint32_t> pops = m_facts.pops(static_cast<uint32_t>(ea));
		m_pops = pops.value_or(0);
		m_popsKnown = pops.has_value();
		const exefacts::Abi& abi = m_facts.abi(static_cast<uint32_t>(ea));
		m_readBytes = abi.args.empty() ? 0 : 4 * (int64_t(abi.args.back()) + 1);
		m_readsEcx = abi.ecx;
		m_readsEdx = abi.edx;
	}
	catch (const std::exception& error)
	{
		throw Skip{std::string("the bytes it pops: ") + error.what()};
	}
	m_facts.trim();
	tinfo_t idb;
	const func_type_data_t* fd = nullptr;
	if ((get_tinfo(&idb, ea) || guess_tinfo(&idb, ea) == GUESS_FUNC_OK) && idb.get_func_details(&details))
	{
		fd = &details;
		callcnv_t cc = fd->get_cc() & CM_CC_MASK;
		if (cc == CM_CC_SPECIALE)
			throw Skip{"usercall: variadic"};
		if (cc == CM_CC_SPECIAL || cc == CM_CC_SPECIALP)
		{
			checkUsercall(ea, *fd);
			m_usercall = true;
		}
	}
	return fd;
}

void Declarer::checkUsercall(uint64_t ea, const func_type_data_t& fd)
{
	uint32_t at = static_cast<uint32_t>(ea);
	// the registers the prototype passes, each an integer or pointer of four bytes or fewer in a whole register
	uint8_t passed = 0;
	int64_t offset = 0;
	for (const funcarg_t& a : fd)
	{
		size_t size = a.type.get_size();
		if (a.argloc.is_stkoff())
		{
			if (size == BADSIZE)
				throw Skip{"usercall: a stack argument of unknown size"};
			if (a.argloc.stkoff() != offset)
				throw Skip{"usercall: stack arguments not laid out in order from offset 0"};
			offset += (int64_t(size) + 3) & ~int64_t(3);
			continue;
		}
		std::string reg = userRegister(a.argloc);
		if (reg.empty())
			throw Skip{"usercall: an argument in " + locationText(a.argloc, size)};
		if (size > 4 || a.type.is_floating() || a.type.is_udt())
			throw Skip{"usercall: " + text(a.type) + " in " + reg};
		uint8_t bit = uint8_t(1u << userFamily(a.argloc));
		if (passed & bit)
			throw Skip{"usercall: two arguments in " + reg};
		passed |= bit;
	}
	// the registers the code reads before writing them; an argument IDA passes is checked against the registers the
	// function takes (exefacts' registerInputs, sync's dead-register pin: passed on to a callee that reads them)
	uint8_t read = 0, inputs = 0;
	try
	{
		read = m_facts.regArgs(at) & (exefacts::kEax | exefacts::kEbx | exefacts::kEsi | exefacts::kEdi);
		read |= ecxEdxReads(m_facts, static_cast<ea_t>(ea));
		inputs = m_facts.registerInputs(at);
	}
	catch (const std::exception& error)
	{
		throw Skip{std::string("usercall: the registers it reads: ") + error.what()};
	}
	auto names = [](uint8_t bits) {
		std::string out;
		for (int r = 0; r < 8; ++r)
			if ((bits & (1u << r)) && kUserRegisters[r])
				out += (out.empty() ? "" : ", ") + std::string(kUserRegisters[r]);
		return out;
	};
	if (uint8_t unread = passed & ~inputs)
		throw Skip{"usercall contradicted: IDA passes " + names(unread) + ", the code does not read it first"};
	if (uint8_t unpassed = read & ~passed)
		throw Skip{"usercall contradicted: the code reads " + names(unpassed) + ", IDA does not pass it"};
	// an x87 input: UserRegs passes none (0xC90B04 stores ST0 first)
	if (m_facts.x87Input(at))
		throw Skip{"usercall contradicted: the code uses ST0 before pushing it, IDA passes no x87 argument"};
	// the stack: a __userpurge pops its arguments, a __usercall none (its callers release them); either reads no
	// slot past them. A __usercall's bytes past the last slot the code reads are its callers' word alone: a caller
	// releases at least them after the call, none fewer (0xCB0AD0's IDA stack arguments: its callers push nothing)
	bool purges = (fd.get_cc() & CM_CC_MASK) == CM_CC_SPECIALP;
	if (!m_popsKnown)
		throw Skip{"usercall: pops what an unknown tail-jump target pops"};
	if (purges ? m_pops != offset : m_pops != 0)
		throw Skip{"usercall contradicted: the code pops " + std::to_string(m_pops) + " bytes, IDA's " +
				   (purges ? "__userpurge " : "__usercall ") + std::to_string(purges ? offset : 0)};
	if (m_readBytes > offset)
		throw Skip{"usercall contradicted: the code reads " + std::to_string(m_readBytes) +
				   " stack argument bytes, IDA passes " + std::to_string(offset)};
	if (!purges)
	{
		bool confirmed = m_readBytes == offset;
		for (int64_t released : m_asm.cleanup(static_cast<ea_t>(ea)))
		{
			if (released < offset)
				throw Skip{"usercall contradicted: a caller releases " + std::to_string(released) +
						   " bytes after the call, IDA passes " + std::to_string(offset)};
			confirmed = true;
		}
		if (!confirmed)
			throw Skip{"usercall unconfirmed: IDA passes " + std::to_string(offset) + " stack bytes, the code reads " +
					   std::to_string(m_readBytes) + " and no caller releases them"};
	}
	// the result: eax (al, ax), edx:eax or ST0 as its type says, and a direct caller that reads eax first after the
	// call reads it there. A caller whose first use is an x87 instruction (AsmFacts' kSt0) says nothing: an `fld` may
	// push a value of its own before eax is read (0xCE9DB0 after 0xCE9C40)
	const tinfo_t& ret = fd.rettype;
	size_t size = ret.get_size();
	std::optional<std::string> result = userResult(fd);
	if (!result)
		throw Skip{"usercall: returns " + text(ret) +
				   (ret.is_udt() || size == BADSIZE ? "" : " in " + locationText(fd.retloc, size))};
	std::string where = *result;
	for (int width : m_asm.resultReads(static_cast<ea_t>(ea)))
	{
		if (width <= 0)
			continue;
		if (width > 0 && (where.empty() || where == "st0"))
			throw Skip{"usercall contradicted: a caller reads eax after the call, IDA's result is " +
					   (where.empty() ? std::string("none") : where)};
		if (width > 0 && where == "eax" && size_t(width) > size)
			throw Skip{"usercall contradicted: a caller reads " + std::to_string(width) +
					   " bytes of eax, IDA's result has " + std::to_string(size)};
	}
}

void Declarer::userSignature(Decl& d, const func_type_data_t& fd)
{
	std::string why;
	std::optional<std::string> ret = m_types.keyOf(fd.rettype, why);
	if (!ret)
		throw Skip{"type not in src: " + why};
	d.ret = *ret;
	d.params.clear();
	d.selfReg.clear();
	d.registers.clear();
	size_t first = 0;
	// a member's `this`: IDA's first argument, in a register, a pointer to the class (IDA's `this` names one)
	if (d.owner && !fd.empty() && !userRegister(fd[0].argloc).empty() && fd[0].type.is_ptr())
	{
		tinfo_t pointee = fd[0].type.get_pointed_object();
		if (pointee.is_udt() && m_types.recordOf(pointee) == d.owner)
		{
			d.selfReg = userRegister(fd[0].argloc);
			first = 1;
		}
	}
	std::set<std::string> used;
	for (size_t k = first; k < fd.size(); ++k)
	{
		std::optional<std::string> key = m_types.keyOf(fd[k].type, why);
		if (!key)
			throw Skip{"type not in src: " + why};
		if (fd[k].type.is_udt() && fd[k].type.get_size() == BADSIZE)
			throw Skip{"type not in src: " + text(fd[k].type)};
		std::string name = fd[k].name.c_str();
		if (!identifier(name) || used.count(name) || typeName(name))
			name = "a" + std::to_string(d.params.size() + 1);
		used.insert(name);
		d.params.push_back({*key, name});
		if (std::string reg = userRegister(fd[k].argloc); !reg.empty())
			d.registers[name] = reg;
	}
	d.cc = (fd.get_cc() & CM_CC_MASK) == CM_CC_SPECIALP ? "userpurge" : "usercall";
	d.isStatic = d.owner && d.selfReg.empty();
	// the declaration takes the stack bytes IDA lays out (checkUsercall held them against the exe)
	int64_t bytes = stackBytes(d);
	int64_t laid = 0;
	for (const funcarg_t& a : fd)
		if (a.argloc.is_stkoff())
			laid += (int64_t(a.type.get_size()) + 3) & ~int64_t(3);
	if (bytes != laid)
		throw Skip{"usercall: the declaration's stack arguments take " + std::to_string(bytes) + " bytes, IDA's " +
				   std::to_string(laid)};
}

// IDA's noreturn (idaNoreturn) is a new declaration's (declared, specialized: its own first declaration); src's own
// declaration keeps its attribute as it is (srcDeclaration's), since a definition cannot add one.
Decl Declarer::plan(uint64_t ea)
{
	Decl d = place(ea);
	if (d.outcome == "declared" || d.outcome == "specialized")
		d.noreturn = d.noreturn || idaNoreturn(static_cast<ea_t>(ea));
	if (foldOwners(ea).size() == 1 && chooseRef(m_model, ea).names.empty())
		d.proof = (d.proof.empty() ? "" : d.proof + "; ") + "named by its Same function as comments";
	return d;
}

Decl Declarer::place(uint64_t ea)
{
	func_type_data_t details;
	const func_type_data_t* fd = readExe(ea, details);

	Name n = nameOf(ea);
	Decl d;
	d.ea = ea;
	// src's refs name class template instances' members there (selection): their fold, or one instance's
	if (RefChoice choice = chooseRef(m_model, ea); choice.names.size() > 1 || (choice.ref && choice.ref->generic))
		return instanceFold(d, choice);
	// an address only `Same function as` comments name (selection): the function is their owner, by the name they
	// spell (src's, which IDA's name of a folded template instance's member need not be)
	if (std::set<std::string> owners = foldOwners(ea); owners.size() == 1)
		n = parsedName(*owners.begin());
	// the VC9 standard library's classes (the PDB's and IDA's std::X) are src's msvc9::X (RULES)
	std::string scope = msvc9Names(n.scope);
	const SrcModel::Record* owner = scope.empty() ? nullptr : recordNamed(scope);
	if (!owner && !scope.empty())
	{
		// an instance srcdb has no record of (it records the instances src uses): the src template's, spelled from
		// the PDB's arguments
		if (size_t open = scope.find('<'); open != std::string::npos && m_patterns.count(scope.substr(0, open)))
		{
			if (scope.back() != '>')
				throw Skip{"class nested in a template instance src has no record of"};
			std::string why;
			owner = instanceOf(scope, why);
			if (!owner)
				throw Skip{why};
		}
		else if (pdbClass(n.scope))
		{
			needs.push_back({n.scope, ea, n.qualified});
			throw Skip{"class not in src"};
		}
	}
	const std::vector<ExeVtables::Holder>& holders = m_vtables.holders(static_cast<ea_t>(ea));
	// IDA's `~X` in a vtable slot that pops 4 bytes (the flags) is X's deleting destructor
	if (n.structor == "dtor" && m_pops == 4 && !holders.empty())
		n.structor = "deleting";
	// IDA's `X::X` whose code does not return `this` in eax on every path is no constructor MSVC compiled (0xAEA150
	// returns GetTickCount's value): an ordinary member, and the class's name is no name for one
	if (n.structor == "ctor" && !m_facts.returnsThis(static_cast<uint32_t>(ea)))
	{
		n.structor.clear();
		n.unnamed = true;
	}
	if (owner && !n.structor.empty())
		return structor(d, *owner, n, fd);
	if (!holders.empty() && n.structor.empty() && !(owner && m_siblings.count(owner)))
		return virtualSlot(d, n, owner, holders, fd);
	if (owner)
		return member(d, *owner, n, fd, false);
	return unowned(d, n, fd);
}

// A fold member is declared as src spells one without a body of its own (porting rules "Folds", "Code not
// reconstructed yet"): a `// Not reconstructed yet.` forwarder to ea, on its class, with the PDB's signature where the
// exe's ABI fits it, or (chosen: a name no PDB member gives) the owner's signature with the name's marker; srcdb's
// findFolds then counts it among the address's members. Nothing else is renamed or redeclared: a name src has
// already, on the class or one the declaration would hide or make ambiguous, is a conflict listed, not overwritten.
Decl Declarer::foldMember(uint64_t ea, const std::string& qualified, bool chosen)
{
	func_type_data_t details;
	const func_type_data_t* fd = readExe(ea, details);
	m_kind = "fold";
	if (m_usercall)
		throw Skip{"usercall"};
	// the emitter's own cases (IDAKIT.md "Related checker facts"); a setter is not: the value it stores is typed by
	// the member's parameter alone
	if (m_facts.emptyFunction(static_cast<uint32_t>(ea)))
		throw Skip{"empty function"};
	std::optional<exefacts::Accessor> accessor = m_facts.trivialAccessor(static_cast<uint32_t>(ea));
	if (accessor && !accessor->store)
		throw Skip{"trivial accessor"};
	Name n;
	n.qualified = qualified;
	n.last = lastComponent(qualified);
	n.scope = scopeOf(qualified);
	if (n.scope.empty())
		throw Skip{"a free function"};
	std::string bare = lastComponent(n.scope);
	bare = bare.substr(0, bare.find('<'));
	if (n.last == bare || n.last.starts_with('~') || n.last.starts_with('`') || n.last.starts_with("operator new") ||
		n.last.starts_with("operator delete"))
		throw Skip{"a constructor, destructor, operator new or delete"};
	const SrcModel::Record* owner = recordNamed(msvc9Names(n.scope));
	if (!owner)
	{
		if (pdbClass(n.scope))
			needs.push_back({n.scope, ea, n.qualified});
		throw Skip{"class not in src"};
	}
	if (nestedInInstance(*owner))
		throw Skip{"a member of a class nested in a class template instance"};
	if (!owner->pattern.empty())
	{
		Decl d;
		d.ea = ea;
		d.kind = m_kind;
		return instanceFoldMember(d, *owner, n.last);
	}
	if (owner->file.rfind("src/", 0) != 0)
		throw Skip{"class not in src/: " + owner->file};
	if (owner->endLine <= 0)
		throw Skip{"class has no definition in src"};
	// a virtual: the class's vtable (or a base's it shares) holds ea, and its other copies are `Same function as`
	// bodies (porting rules "Folds"), not forwarders
	std::set<std::string> chain;
	for (const std::string& c : m_vtables.chain(owner->name))
		chain.insert(ExeVtables::classKey(c));
	chain.insert(ExeVtables::classKey(owner->name));
	for (const ExeVtables::Holder& h : m_vtables.holders(static_cast<ea_t>(ea)))
		if (chain.count(ExeVtables::classKey(h.table->cls)))
			throw Skip{"a virtual: " + h.table->cls + "'s vtable holds it"};
	// the class has a member at the address already (a forwarder names it where its function src registers nowhere
	// else: chooseRef's rule)
	for (const SrcModel::Ref& r : m_model.refsAt(ea))
	{
		const SrcModel::Function* f = placeholder(r) ? nullptr : m_model.function(r.function);
		if (!f || f->cls != owner->name)
			continue;
		const std::vector<uint64_t>& registered = m_model.bodyAddresses(r);
		if (r.kind != "forwarder" || std::all_of(registered.begin(), registered.end(), [&](uint64_t a) {
				return a == ea;
			}))
			throw Skip{"the class has a member at the address: " + f->name};
	}

	Decl d;
	d.ea = ea;
	d.kind = m_kind;
	d.owner = owner;
	d.member = n.last;
	if (!identifier(d.member) && !d.member.starts_with("operator"))
		throw Skip{"a name C++ cannot take"};
	if (chosen)
	{
		// no name from the PDB: the owner's signature with this class as `this` (src's declaration at the address,
		// else the first of the fold's members by name), the owner's name with its marker
		RefChoice choice = chooseRef(m_model, ea);
		const SrcModel::Function* from =
			choice.ref && !placeholder(*choice.ref) ? m_model.function(choice.ref->function) : nullptr;
		if (!from)
			for (const std::string& name : choice.names)
				for (const SrcModel::Function* f : m_model.functionsNamed(name))
					if (!from && !f->cls.empty() && !f->isStatic)
						from = f;
		if (!from)
			throw Skip{"no src member at the address to take the signature from"};
		if (from->isStatic || from->cls.empty() || lastComponent(from->name) != d.member)
			throw Skip{"the owner " + from->name + " is no member of the name called on its object"};
		// a class template instance's member names its instance's arguments (NiPointer<T>::operator=, a map's
		// GetAt), which are no type of this class's
		if (const SrcModel::Record* r = asClass(m_model.record(from->cls));
			r && (!r->pattern.empty() || nestedInInstance(*r)))
			throw Skip{"the owner " + from->name + " is a class template instance's member (its signature is the "
					   "instance's)"};
		Decl o;
		srcDeclaration(o, m_model, *from, "this");
		if (o.cc != "thiscall")
			throw Skip{"the owner's convention is " + o.cc};
		const SrcModel::Abi* abi = m_model.abi(from->mangled);
		if (!popsFit(abi->stackBytes))
			throw Skip{"ABI: pops " + std::to_string(m_pops) + ", the owner's signature " +
					   std::to_string(abi->stackBytes) + " bytes"};
		d.ret = o.ret;
		d.params = o.params;
		d.cc = o.cc;
		d.isConst = o.isConst;
		d.indirect = o.indirect;
		d.exact = true;
		d.marker = "Not named in the PDB or IDA; name chosen here.";
		// a setter stores into this class's field, whatever the owner's parameter is; any other type of the owner's
		// that is its object's (a record, an enum of a class) is no evidence of this class's: no member
		std::optional<size_t> stored;
		if (accessor)
			stored = setterField(d, *owner, *accessor);
		std::vector<std::string> keys{d.ret};
		for (size_t k = 0; k < d.params.size(); ++k)
			if (k != stored)
				keys.push_back(d.params[k].key);
		for (const std::string& key : keys)
			if (std::optional<std::string> named = classDependent(key))
				throw Skip{"the owner " + from->name + "'s signature names " + *named +
						   ", which the exe's code does not show of " + owner->name};
	}
	else
	{
		if (!pdbOverloads(qualified))
			throw Skip{"not in the PDB (a PC-only member)"};
		// the PDB's overload whose stack bytes are the exe's `ret N` (thiscall for a member, the exe's convention for
		// a static one; a static one in ecx the code does not read); a type src lacks skips it with `type not in src`
		pdbSignature(d, qualified, true, fd);
	}
	if (d.cc != "thiscall")
	{
		d.isStatic = true;
		if (d.cc != "fastcall" && (ecxEdxReads(m_facts, static_cast<ea_t>(ea)) & exefacts::kEcx))
			throw Skip{"ABI: static in the PDB, the exe's code reads ecx"};
	}
	if (d.member.starts_with("operator") && d.member != "operator()" && (d.isStatic || d.params.size() > 1))
		throw Skip{d.isStatic ? "static operator" : "operator with " + std::to_string(d.params.size()) + " parameters"};
	std::string keys = paramKeys(d.params);
	if (d.member == "operator=" && d.params.size() == 1)
	{
		std::string p = m_types.canonical(d.params[0].key);
		if (p == "record:" + owner->name || p == "lref(record:" + owner->name + ")")
			throw Skip{"copy assignment"};
	}
	if (typeMember(owner->name, d.member))
		throw Skip{"conflict: the class has a type or enumerator of the name"};
	for (const SrcModel::Function* f : m_model.functionsNamed(owner->name + "::" + d.member))
	{
		const SrcModel::Type* t = m_model.type(f->type);
		std::vector<Param> ps;
		for (const std::string& k : t ? t->params : std::vector<std::string>{})
			ps.push_back({k, ""});
		auto at = m_addressesOf.find(f->mangled);
		std::string where = at == m_addressesOf.end() || at->second.empty() ? "without an address"
																			 : "at " + hexText(*at->second.begin());
		if (paramKeys(ps) == keys)
			throw Skip{"conflict: src declares " + f->name + " with these parameters " + where};
		if (ps.size() == d.params.size())
			throw Skip{"conflict: src declares " + f->name + " of as many parameters " + where +
					   " (src's calls could turn ambiguous)"};
	}
	if (registeredByName(owner->name + "::" + d.member))
		throw Skip{"conflict: src registers " + owner->name + "::" + d.member + " by its name alone"};
	if (m_newSignatures.count(owner->name + "::" + d.member + "(" + keys + ")"))
		throw Skip{"declared for another address in this run"};
	checkMemberName(d, *owner);
	checkDerivedLookups(d, *owner);
	m_newSignatures.emplace(owner->name + "::" + d.member + "(" + keys + ")", d.ea);
	d.self = d.isStatic ? "" : "this";
	d.outcome = "declared";
	placeMember(d, *owner, qualified);
	return d;
}

// The fold's owner is src's declaration at the address (chooseRef's: its registered body or its forwarder), else the
// one function the `Same function as` comments there name; the instance's member is the one made from the owner's
// template declaration (srcdb's function row: the template's file and line for every instance), declared as a
// copy: its explicit specialization at its home (SpecializationHomes) with the owner's comment above it and a
// `// Not reconstructed yet.` forwarder to the address. Where src has the template's inline body of the member and
// the exe's function is a transparent function's copy (bodyOutsideHeaders false), the member is no declaration's: a
// src call of it runs the template's body, which the checker compares with the exe's copy, and the fold-owner step
// (Unfold) types the call by it.
Decl Declarer::instanceFoldMember(Decl d, const SrcModel::Record& instance, const std::string& name)
{
	RefChoice choice = chooseRef(m_model, d.ea);
	const SrcModel::Function* owner =
		choice.ref && !placeholder(*choice.ref) ? m_model.function(choice.ref->function) : nullptr;
	if (!owner)
	{
		std::set<std::string> named = foldOwners(d.ea);
		if (named.size() > 1)
			throw Skip{"fold comments name " + std::to_string(named.size()) + " owners"};
		if (named.size() == 1 && m_model.functionsNamed(*named.begin()).size() == 1)
			owner = m_model.functionsNamed(*named.begin()).front();
	}
	if (!owner)
		throw Skip{"owner: src declares no function at the address"};
	const SrcModel::Record* of = owner->cls.empty() ? nullptr : asClass(m_model.record(owner->cls));
	if (!of || of->pattern != instance.pattern)
		throw Skip{"owner: " + owner->name + " is no member of an instance of " + instance.pattern};
	if (of->name == instance.name)
		throw Skip{"owner: the receiver's instance is the owner's own"};
	if (lastComponent(owner->name) != name)
		throw Skip{"owner: " + owner->name + " is no member of the name " + name};
	std::vector<const SrcModel::Function*> same;
	for (const SrcModel::Function* f : m_model.functionsNamed(instance.name + "::" + name))
		if (f->file == owner->file && f->line == owner->line && f->isStatic == owner->isStatic)
			same.push_back(f);
	if (same.size() != 1)
		throw Skip{"no member: src declares " + std::string(same.empty() ? "no " : "several ") + instance.name +
				   "::" + name + " from " + owner->name + "'s template declaration"};
	const SrcModel::Function& member = *same.front();
	// src specializes it already: a copy at the address (its comment is instanceFold's), or another exe address's
	// (the receiver's src type, or a second exe copy of the instance's member: the porter's)
	if (m_model.specializations().count(member.mangled))
	{
		auto found = m_addressesOf.find(member.mangled);
		bool here = found != m_addressesOf.end() && found->second.count(d.ea);
		throw Skip{"specialized: src specializes " + member.name +
				   (here ? " for the address" : " for another address (the receiver's type, or a second exe copy)")};
	}
	if (std::string why = instanceMismatch(*of, instance, *owner, member); !why.empty())
		throw Skip{"item size: " + why};
	const SrcModel::Definition* def = m_model.definition(member.mangled);
	if (def && def->emitted)
		throw Skip{"emitted: an emitted block (the emitter's)"};
	if (def && !def->instance)
		throw Skip{"defined: src defines " + member.name + " itself"};
	if (def && isHeader(def->file) && !bodyOutsideHeaders(m_model, d.ea))
		throw Skip{"transparent: the template's inline body, a transparent function's copy (the emitter calls it)"};
	if (std::string abi = abiDisagreement(member); !abi.empty())
		throw Skip{abi};
	if (auto [at, added] = m_claimed.emplace(member.mangled, d.ea); !added)
		throw Skip{"in this run: specialized for " + hexText(at->second)};
	srcDeclaration(d, m_model, member, member.isStatic ? "" : "this");
	d.owner = &instance;
	d.outcome = "specialized";
	d.fold = "Same function as " + owner->name + " (" + address(d.ea) + "), registered there.";
	d.proof = "fold copy of " + owner->name;
	home(d);
	return d;
}

std::string Declarer::instanceMismatch(const SrcModel::Record& a, const SrcModel::Record& b,
									   const SrcModel::Function& owner, const SrcModel::Function& member) const
{
	// the size of what a pointer or reference key points to, nullopt for any other key (or an unknown size)
	auto pointee = [&](const std::string& key) -> std::optional<int64_t> {
		SrcTypes::Parts p = m_types.parts(m_types.canonical(key));
		if (p.kind != "ptr" && p.kind != "lref" && p.kind != "rref")
			return std::nullopt;
		return m_model.sizeOf(p.of);
	};
	auto indirect = [&](const std::string& key) {
		std::string kind = m_types.parts(m_types.canonical(key)).kind;
		return kind == "ptr" || kind == "lref" || kind == "rref";
	};
	// two keys the code reads alike: the same type, or both of one size and, pointers, to items of one size
	auto alike = [&](const std::string& x, const std::string& y) {
		std::string cx = m_types.canonical(x), cy = m_types.canonical(y);
		if (cx == cy)
			return true;
		std::optional<int64_t> sx = m_model.sizeOf(cx), sy = m_model.sizeOf(cy);
		if (!sx || sx != sy || indirect(cx) != indirect(cy))
			return false;
		return !indirect(cx) || (pointee(cx) && pointee(cx) == pointee(cy));
	};
	if (a.size != b.size || a.fields.size() != b.fields.size())
		return a.name + " and " + b.name + " differ in layout";
	for (size_t k = 0; k < a.fields.size(); ++k)
		if (a.fields[k].offsetBits != b.fields[k].offsetBits || a.fields[k].bits != b.fields[k].bits ||
			!alike(a.fields[k].type, b.fields[k].type))
			return "field " + a.fields[k].name + " is " + a.fields[k].type + " in " + a.name + ", " +
				   b.fields[k].type + " in " + b.name + " (another item size)";
	const SrcModel::Type* x = m_model.type(owner.type);
	const SrcModel::Type* y = m_model.type(member.type);
	if (!x || !y || x->params.size() != y->params.size())
		return "no type of " + owner.name + " or " + member.name + " in the model";
	for (size_t k = 0; k < x->params.size(); ++k)
		if (!alike(x->params[k], y->params[k]))
			return "parameter " + std::to_string(k + 1) + " is " + x->params[k] + " in " + owner.name + ", " +
				   y->params[k] + " in " + member.name;
	if (!alike(x->ret, y->ret))
		return "the return is " + x->ret + " in " + owner.name + ", " + y->ret + " in " + member.name;
	return "";
}

Decl Declarer::foldCall(uint64_t ea, const std::string& owner, const std::string& receiver)
{
	m_kind = "fold";
	std::string scope = scopeOf(owner);
	const SrcModel::Record* of = scope.empty() ? nullptr : asClass(m_model.record(scope));
	if (!of || of->pattern.empty())
		throw Skip{"the owner " + owner + " is no class template instance's member"};
	SrcTypes::Parts pointer = m_types.parts(m_types.canonical(receiver));
	SrcTypes::Parts object = pointer.kind == "ptr" ? m_types.parts(m_types.canonical(pointer.of)) : SrcTypes::Parts{};
	if (object.kind == "const")
		object = m_types.parts(m_types.canonical(object.of));
	if (object.kind != "record")
		throw Skip{"receiver: no instance (" + receiver + "): the local's, field's or global's type"};
	// the nearest object at the receiver's address that is an instance of the owner's template: the record, a base
	// at offset 0 or a by-value first field, each's again
	std::string instance;
	std::vector<std::string> level{object.name};
	std::set<std::string> seen;
	for (int depth = 0; !level.empty() && instance.empty() && depth < 16; ++depth)
	{
		std::vector<std::string> next;
		for (const std::string& name : level)
		{
			const SrcModel::Record* r = seen.insert(name).second ? m_model.record(name) : nullptr;
			if (!r)
				continue;
			if (r->pattern == of->pattern && instance.empty())
				instance = r->name;
			for (const SrcModel::Base& base : r->bases)
				if (base.offset == 0 && !base.isVirtual)
					next.push_back(base.record);
			for (const SrcModel::Field& f : r->fields)
				if (SrcTypes::Parts p = m_types.parts(m_types.canonical(f.type));
					f.offsetBits == 0 && !f.bits && p.kind == "record")
					next.push_back(p.name);
		}
		level = std::move(next);
	}
	if (instance.empty())
		throw Skip{"receiver: no instance of " + of->pattern + " at " + object.name +
				   "'s address: the local's, field's or global's type"};
	return foldMember(ea, instance + "::" + lastComponent(owner));
}

// The field a setter's store lands in is the class's own src declaration at the offset (through its non-virtual bases
// and by-value members, the innermost there of the store's width), so the value it takes is of that field's type: a
// TESSound* field takes a TESSound*, whatever the owner's parameter is. A field IDA named (`unk<offset>`,
// `field_<offset>`, padding) is no evidence of the type.
std::optional<size_t> Declarer::setterField(Decl& d, const SrcModel::Record& cls, const exefacts::Accessor& accessor)
{
	if (accessor.base != -1)
		throw Skip{"a setter of a stack argument's object"};
	// the parameter whose stack slot the exe stores (each parameter rounded up to dwords)
	size_t index = 0;
	int64_t at = 0;
	for (; index < d.params.size() && at < accessor.value * 4; ++index)
	{
		std::optional<int64_t> size = m_model.sizeOf(d.params[index].key);
		if (!size)
			throw Skip{"a setter's parameter of no known size: " + d.params[index].key};
		at += (*size + 3) / 4 * 4;
	}
	if (index >= d.params.size() || at != accessor.value * 4)
		throw Skip{"the setter's stored argument is no parameter of the owner's signature"};
	const SrcModel::Record* r = &cls;
	int64_t offset = accessor.offset;
	const SrcModel::Field* field = nullptr;
	std::string holder;
	for (int depth = 0; r && depth < 32; ++depth)
	{
		const SrcModel::Record* inner = nullptr;
		int64_t innerOffset = 0;
		for (const SrcModel::Field& f : r->fields)
		{
			std::optional<int64_t> size = m_model.sizeOf(f.type);
			if (f.bits || !size || f.offsetBits % 8 || f.offsetBits / 8 > offset || offset >= f.offsetBits / 8 + *size)
				continue;
			// a record member (or an array of them: its first element only) holds the field; an array of scalars is
			// no field of the store's type
			std::string canonical = m_types.canonical(f.type);
			SrcTypes::Parts storage = m_types.parts(m_model.storage(f.type));
			const SrcModel::Record* record = storage.kind == "record" ? m_model.record(storage.name) : nullptr;
			if (record && offset - f.offsetBits / 8 < record->size)
			{
				inner = record;
				innerOffset = offset - f.offsetBits / 8;
			}
			else if (!record && m_types.parts(canonical).kind != "array" && f.offsetBits / 8 == offset &&
					 *size == accessor.width)
			{
				field = &f;
				holder = r->name;
			}
			break;
		}
		if (field || inner)
		{
			r = inner;
			offset = innerOffset;
			if (field)
				break;
			continue;
		}
		for (const SrcModel::Base& b : r->bases)
			if (const SrcModel::Record* base = m_model.record(b.record);
				base && !b.isVirtual && b.offset <= offset && offset < b.offset + base->size)
			{
				inner = base;
				innerOffset = offset - b.offset;
				break;
			}
		r = inner;
		offset = innerOffset;
	}
	if (!field)
		throw Skip{"the setter stores at " + hex(accessor.offset) + ", where " + cls.name + " has no field of " +
				   std::to_string(accessor.width) + " bytes"};
	static const std::regex idaNamed(R"((unk[0-9A-Fa-f_]*|field_[0-9A-Fa-f]+|pad\w*))");
	if (field->name.empty() || std::regex_match(field->name, idaNamed))
		throw Skip{"the setter stores into " + holder + "::" + (field->name.empty() ? "(unnamed)" : field->name) +
				   ", a field IDA named (no evidence of its type)"};
	d.params[index].key = field->type;
	d.proof = "the stored value is " + holder + "::" + field->name + "'s type";
	return index;
}

std::optional<std::string> Declarer::classDependent(const std::string& key) const
{
	std::string at = m_types.canonical(key);
	for (int depth = 0; depth < 16; ++depth)
	{
		SrcTypes::Parts p = m_types.parts(at);
		if (p.kind == "ptr" || p.kind == "lref" || p.kind == "rref" || p.kind == "array" || p.kind == "const")
		{
			at = p.of;
			continue;
		}
		if (p.kind == "record")
			return p.name;
		if (p.kind == "enum" && !scopeOf(p.name).empty() && m_model.record(scopeOf(p.name)))
			return p.name;
		return std::nullopt;
	}
	return std::nullopt;
}

std::optional<std::vector<Param>> Declarer::idbParams(const func_type_data_t* fd) const
{
	if (!fd)
		return std::nullopt;
	std::vector<Param> idb;
	std::string ignored;
	for (size_t k = (fd->get_cc() & CM_CC_MASK) == CM_CC_THISCALL ? 1 : 0; k < fd->size(); ++k)
	{
		std::optional<std::string> key = m_types.keyOf((*fd)[k].type, ignored);
		if (!key)
			return std::nullopt;
		idb.push_back({*key, ""});
	}
	return idb;
}

const SrcModel::Function* Declarer::undefinedFit(const std::string& qualified, const func_type_data_t* fd,
												 bool defined)
{
	std::vector<const SrcModel::Function*> fits;
	for (const SrcModel::Function* f : m_model.functionsNamed(qualified))
	{
		const SrcModel::Abi* abi = m_model.abi(f->mangled);
		if ((m_model.definition(f->mangled) != nullptr) == defined && abi && popsFit(abi->stackBytes))
			fits.push_back(f);
	}
	if (fits.size() > 1)
		if (std::optional<std::vector<Param>> idb = idbParams(fd))
		{
			// of several that fit the bytes, the overload whose parameter types are IDA's prototype's; a static
			// thiscall's first parameter (a structor's apMemory or apThis) is IDA's `this`
			std::vector<const SrcModel::Function*> matching;
			for (const SrcModel::Function* f : fits)
			{
				const SrcModel::Type* type = m_model.type(f->type);
				const SrcModel::Abi* abi = m_model.abi(f->mangled);
				if (!type)
					continue;
				std::vector<Param> params;
				for (const std::string& key : type->params)
					params.push_back({key, ""});
				if (idbKeys(params, f->isStatic && abi->cc == "thiscall" ? 1 : 0) == idbKeys(*idb))
					matching.push_back(f);
			}
			if (!matching.empty())
				fits = std::move(matching);
		}
	if (fits.size() > 1)
		throw Skip{"ambiguous overloads"};
	return fits.empty() ? nullptr : fits[0];
}

std::string Declarer::definedReason(const SrcModel::Function& f, uint64_t ea) const
{
	const SrcModel::Definition* def = m_model.definition(f.mangled);
	if (def && def->instance)
		return kTemplateBody;
	// an explicit specialization src registers at another address: the instance's member is that address's in src,
	// and a second definition of it would be a redefinition (a per-address forwarder is the porter's choice)
	if (m_model.specializations().count(f.mangled))
		for (uint64_t other : m_model.bodyAddresses(f.mangled))
			if (other != ea)
				return "src specializes the member for another address (" + hexText(other) + ")";
	return kUnregistered;
}

std::string Declarer::tieProof(const SrcModel::Function& f, const SrcModel::Definition& def, uint64_t ea, bool slot)
{
	if (slot)
		return "vtable slot";
	std::string last = lastComponent(f.name), sub = subName(ea);
	if (std::equal(last.begin(), last.end(), sub.begin(), sub.end(),
				   [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(b); }))
		return "sub_ name";
	// an address the comments write (`0x401510`, any case, leading zeros allowed): the function's own, else one of its
	// instructions (a porter's note on the exe's code at 0xABE4E9)
	static const std::regex written(R"(\b0x([0-9A-Fa-f]{1,8})\b)");
	std::string proof;
	for (const std::string& line : definitionComments(m_text.lines(def.file), def.line, def.endLine).lines)
		for (std::sregex_iterator at(line.begin(), line.end(), written), end; at != end; ++at)
		{
			uint64_t value = std::stoull((*at)[1].str(), nullptr, 16);
			if (value == ea)
				return "comment";
			if (func_t* fn = get_func(static_cast<ea_t>(value)); fn && fn->start_ea == ea)
				proof = "comment: an instruction of the function";
		}
	return proof;
}

std::string Declarer::abiDisagreement(const SrcModel::Function& f) const
{
	// the ABI: src's convention passes the registers the code reads, and pops what the exe pops
	const SrcModel::Abi* abi = m_model.abi(f.mangled);
	if (!abi)
		return "no ABI row for " + f.name;
	if (m_usercall)
		return "usercall: IDA's prototype passes registers src's convention does not";
	int64_t pops = abi->cc == "cdecl" ? 0 : abi->stackBytes;
	if (m_popsKnown && pops != m_pops)
		return "ABI: pops " + std::to_string(m_pops) + ", src's " + abi->cc + " signature " + std::to_string(pops);
	// a C-variadic function reads its `...` past the signature's bytes
	const SrcModel::Type* type = m_model.type(f.type);
	if (m_readBytes > abi->stackBytes && !(type && type->variadic))
		return "ABI: the code reads " + std::to_string(m_readBytes) + " stack bytes, src's signature has " +
			   std::to_string(abi->stackBytes);
	if (m_readsEcx && abi->cc != "thiscall" && abi->cc != "fastcall")
		return "ABI: the code reads ecx, src's " + abi->cc + " passes nothing there";
	if (m_readsEdx && abi->cc != "fastcall")
		return "ABI: the code reads edx, src's " + abi->cc + " passes nothing there";
	return "";
}

Decl Declarer::instanceSpecialization(Decl d, const SrcModel::Function& f, const SrcModel::Record& instance,
									   const std::string& self)
{
	std::string why = std::string(kTemplateBody) + ": ";
	// a C++ constructor or destructor is the porting rules' Construct / Destruct of the template, specialized
	const SrcModel::Function* target = &f;
	if (!f.structor.empty())
	{
		std::string name = f.structor == "ctor" ? "Construct" : "Destruct";
		std::vector<const SrcModel::Function*> declared;
		for (const SrcModel::Function* s : m_model.functionsNamed(f.cls + "::" + name))
			if (s->isStatic && abiDisagreement(*s).empty())
				declared.push_back(s);
		if (declared.size() != 1)
			throw Skip{why + "a C++ " + (f.structor == "ctor" ? "constructor" : "destructor") + ": the template has " +
					   (declared.empty() ? "no " : "more than one ") + name + " of its ABI (porting rules " +
					   "\"Constructors and destructors\")"};
		target = declared[0];
	}
	const SrcModel::Definition* def = m_model.definition(target->mangled);
	if (def && def->emitted)
		throw Skip{why + "an emitted block (the emitter's)"};
	if (def && !def->instance)
		throw Skip{why + "not the template's body: " + target->name};
	if (!m_model.bodyAddresses(target->mangled).empty())
		throw Skip{why + "src registers it at " + hexText(m_model.bodyAddresses(target->mangled).front())};
	// an explicit specialization declares the template's member: src's declaration is the exe's ABI or nothing
	if (std::string abi = abiDisagreement(*target); !abi.empty())
		throw Skip{why + abi};
	if (auto [at, added] = m_claimed.emplace(target->mangled, d.ea); !added)
		throw Skip{why + "specialized for " + hexText(at->second) + " in this run"};
	srcDeclaration(d, m_model, *target, self);
	d.owner = &instance;
	d.outcome = "specialized";
	home(d);
	return d;
}

Decl Declarer::siblingSpecialization(Decl d, const SrcModel::Record& owner, const std::string& name,
									  const std::string& pdbName)
{
	if (!templateDeclares(d, owner, name, true))
		throw Skip{"signature differs from the template's declaration"};
	std::string keys = owner.name + "::" + name + "(" + paramKeys(d.params) + ")";
	if (auto [at, added] = m_claimed.emplace(keys, d.ea); !added)
		throw Skip{"specialized for " + hexText(at->second) + " in this run"};
	d.owner = &owner;
	d.member = name;
	d.self = d.isStatic ? "" : "this";
	d.outcome = "specialized";
	d.pdbName = pdbName;
	home(d);
	return d;
}

Decl Declarer::instanceFold(Decl d, const RefChoice& choice)
{
	d.kind = m_kind = "fold";
	std::string why = "several src functions name it: ";
	if (m_usercall)
		throw Skip{why + "usercall"};
	struct Member
	{
		const SrcModel::Function* f;
		const SrcModel::Record* r;  // a class template instance's member's class
		bool specialized;           // src spells its explicit specialization (else the template's forwarder names it)
		bool plain;                 // no template's instance: a function of its own that only forwards there
	};
	std::vector<Member> members;
	std::set<std::string> seen;
	for (const SrcModel::Ref* ref : choice.named)
	{
		const SrcModel::Function* f = ref->function.empty() ? nullptr : m_model.function(ref->function);
		if (!f)
			throw Skip{why + ref->target + " (a template's forwarder src makes no instance from)"};
		if (ref->kind != "forwarder")
			throw Skip{why + f->name + " is no forwarder"};
		if (!seen.insert(f->mangled).second)
			continue;
		const SrcModel::Record* r = asClass(m_model.record(f->cls));
		bool specialized = m_model.specializations().count(f->mangled) != 0;
		if (r && (!r->pattern.empty() || nestedInInstance(*r)))
			members.push_back({f, r, specialized, false});
		else if (f->cls.empty() && specialized)
			members.push_back({f, nullptr, true, false});
		else
		{
			// another src function is a copy only where it is a forwarder to the address and nothing else (a
			// definition of a few lines whose one call of an exe address is that one)
			const SrcModel::Definition* def = m_model.definition(f->mangled);
			auto at = m_addressesOf.find(f->mangled);
			if (!def || def->emitted || def->endLine - def->line > 5 || at == m_addressesOf.end() ||
				at->second != std::set<uint64_t>{d.ea})
				throw Skip{why + f->name + " is no template's instance and does more than forward there"};
			members.push_back({f, nullptr, false, true});
		}
	}
	std::sort(members.begin(), members.end(), [](const Member& a, const Member& b) {
		return std::tie(a.plain, a.f->name, a.f->mangled) < std::tie(b.plain, b.f->name, b.f->mangled);
	});
	if (members.front().plain)
		throw Skip{why + "no template's instance among them"};
	// the owner: the instance the exe names there, else the alphabetically first template's instance
	Name n = nameOf(d.ea);
	const SrcModel::Record* named = n.scope.empty() ? nullptr : recordNamed(msvc9Names(n.scope));
	const Member* owner = &members.front();
	for (const Member& m : members)
		if (!m.plain && ((named && m.r && m.r->name == named->name) ||
						 (!m.r && msvc9Names(m.f->name) == msvc9Names(n.qualified))))
		{
			owner = &m;
			break;
		}
	Decl o;
	if (owner->specialized)
	{
		// src's explicit specialization forwards there already: only the copies' comments are written
		o = d;
		o.outcome = "folded";
		o.mangled = owner->f->mangled;
		o.owner = owner->r;
		o.member = lastComponent(owner->f->name);
		o.pdbName = owner->f->name;
	}
	else
		o = instanceSpecialization(d, *owner->f, *owner->r, "this");
	std::string copies;
	size_t generic = 0;
	for (const Member& m : members)
	{
		if (&m == owner)
			continue;
		if (m.plain)
			o.foldCopies.push_back(foldComment(*m.f, *owner->f, d.ea));
		else if (!m.specialized)
		{
			++generic;
			continue;
		}
		else
			for (LineInsert& comment : specializationFoldComments(*m.f, *owner->f, d.ea))
				o.foldCopies.push_back(std::move(comment));
		copies += (copies.empty() ? "" : ", ") + m.f->name;
	}
	if (o.outcome == "folded" && o.foldCopies.empty())
		throw Skip{why + "the copies are commented already"};
	o.proof = (o.proof.empty() ? "" : o.proof + "; ") + "fold owner" +
			  (copies.empty() ? "" : "; Same function as comments: " + copies) +
			  (generic ? "; the template's forwarder stays for " + std::to_string(generic) + " other instances" : "");
	return o;
}

Decl Declarer::addressed(Decl d, const SrcModel::Function& f, bool slot, const std::string& self)
{
	std::string reason = definedReason(f, d.ea);
	if (reason == kTemplateBody)
	{
		const SrcModel::Record* instance = asClass(m_model.record(f.cls));
		if (!instance)
			throw Skip{reason + ": no record of " + f.cls};
		return instanceSpecialization(d, f, *instance, self);
	}
	if (reason != kUnregistered)
		throw Skip{reason};
	const SrcModel::Definition& def = *m_model.definition(f.mangled);
	std::string why = std::string(kUnregistered) + ": ";
	// gen/Prototypes.hpp is frozen (porting rules): its entries move to their homes, nothing is added to it
	if (def.file == "gen/Prototypes.hpp")
		throw Skip{why + "a gen/Prototypes.hpp forwarder"};
	if (def.emitted)
		throw Skip{why + "an emitted block (the emitter's)"};
	for (const SrcModel::Ref& r : m_model.refsAt(d.ea))
		if (r.kind == "forwarder" && r.function == f.mangled)
			throw Skip{why + "its body forwards to the address"};
	if (!m_model.bodyAddresses(f.mangled).empty())
		throw Skip{why + "src registers it at " + hexText(m_model.bodyAddresses(f.mangled).front())};
	if (std::string abi = abiDisagreement(f); !abi.empty())
		throw Skip{why + abi};
	const SrcModel::Abi* abi = m_model.abi(f.mangled);
	// FNVD_ADDRESS names the function from namespace scope; a member it cannot reach is the porting rules' (made
	// protected and registered through the member macros), never declare's
	if (!f.cls.empty() && f.access != "public")
		throw Skip{why + (f.access.empty() ? std::string("member of unknown access") : f.access + " member") +
				   ", not accessible from namespace scope"};
	std::string proof = tieProof(f, def, d.ea, slot);
	if (proof.empty())
		throw Skip{why + "the tie is the name alone (no vtable slot, no comment at the definition naming the address)"};
	if (auto [at, added] = m_claimed.emplace(f.mangled, d.ea); !added)
		throw Skip{why + "tied to " + hexText(at->second) + " in this run"};
	d.outcome = "addressed";
	d.proof = proof;
	d.mangled = f.mangled;
	d.owner = asClass(m_model.record(f.cls));
	d.member = lastComponent(f.name);
	d.scope = d.owner ? "" : scopeOf(f.name);
	d.cc = abi->cc;
	d.exact = true;
	d.header = "";
	d.source = def.file;
	d.pdbName = f.name;
	return d;
}

Decl Declarer::existing(Decl d, const SrcModel::Function& f, const std::string& self, bool slot)
{
	if (m_model.definition(f.mangled))
		return addressed(d, f, slot, self.empty() ? "this" : self);
	const SrcModel::Abi* abi = m_model.abi(f.mangled);
	const SrcModel::Type* type = m_model.type(f.type);
	if (!abi || !type)
		throw Skip{"no ABI row for " + f.name};
	if (!popsFit(abi->stackBytes))
		throw Skip{"ABI: pops " + std::to_string(m_pops) + ", signature " + std::to_string(abi->stackBytes) + " bytes"};
	if (type->variadic)
		throw Skip{"variadic"};
	// a record result: the exe's hidden result pointer, to a local the definition returns (in registers: none)
	d.indirect = abi->ret == "indirect";
	if (!d.indirect && m_types.parts(m_types.canonical(type->ret)).kind == "record")
		throw Skip{"a record returned in registers"};
	if (!conventionCode(abi->cc))
		throw Skip{"src convention " + abi->cc};
	const SrcModel::Record* r = asClass(m_model.record(f.cls));
	if (auto [at, added] = m_claimed.emplace(f.mangled, d.ea); !added)
		throw Skip{f.name + " is defined for " + hexText(at->second) + " in this run"};
	srcDeclaration(d, m_model, f, self);
	d.owner = r;
	d.outcome = r && (!r->pattern.empty() || nestedInInstance(*r)) ? "specialized" : "defined";
	// a specialization is declared and defined at its home; src's own declaration stays as it is otherwise
	if (d.outcome == "specialized")
	{
		home(d);
		return d;
	}
	d.header = "";
	bool templated = this->templated(r);
	// a template's (or a record src models flat from one): the class's own .cpp, never a TU that instantiated it
	// (the PDB's source, and classSource where a definition went there)
	d.source = templated ? "" : pdbSource(f.name);
	if (d.source.empty() && !f.cls.empty() && !templated)
		d.source = m_model.classSource(f.cls);
	if (d.source.empty())
	{
		const std::string& home = r ? r->file : f.file;
		d.source = headerFile(home) ? stem(home) + ".cpp" : home;
	}
	return d;
}

Decl Declarer::structor(Decl d, const SrcModel::Record& owner, const Name& n, const func_type_data_t* fd)
{
	d.kind = m_kind = "structor";
	// Construct/Destruct take `this` as the thiscall's ecx
	if (m_usercall)
		throw Skip{"usercall: a constructor or destructor"};
	std::string name = n.structor == "ctor" ? "Construct" : n.structor == "dtor" ? "Destruct" : "DeletingDestruct";
	std::string qualified = owner.name + "::" + name;
	bool nested = nestedInInstance(owner);
	// a template instance (or a class nested in one): the template's own structor of the name, specialized for it;
	// where the template has none, one is declared in the template (its injected class name for the instance's) and
	// specialized below
	if ((!owner.pattern.empty() || nested) && !m_siblings.count(&owner) && !m_model.functionsNamed(qualified).empty())
	{
		Decl t = templateMember(d, owner, name, n, fd);
		t.kind = m_kind = "structor";
		return t;
	}
	m_kind = "structor";
	if (const SrcModel::Function* f = undefinedFit(qualified, fd))
		return existing(d, *f, "");
	// a class nested in a template instance: a structor of its own would be the template's, for every instance
	if (nested)
		throw Skip{"template lacks member: " + qualified};
	std::string self = "ptr(record:" + owner.name + ")";
	d.owner = &owner;
	d.member = name;
	d.isStatic = true;
	d.cc = "thiscall";
	if (n.structor == "ctor")
	{
		d.ret = self;
		d.params = {{"ptr(builtin:void)", "apMemory"}};
		Decl signature = d;
		signature.params.clear();
		signature.isStatic = false;
		if (!pdbSignature(signature, n.qualified, true, fd))
			idbSignature(signature, fd);
		if (signature.cc != "thiscall")
			throw Skip{"constructor not thiscall"};
		for (Param& p : signature.params)
			d.params.push_back(p);
	}
	else if (n.structor == "dtor")
	{
		d.params = {{self, "apThis"}};
		if (!popsFit(0))
			throw Skip{"ABI: pops " + std::to_string(m_pops) + ", signature 0 bytes"};
	}
	else
	{
		d.ret = self;
		d.params = {{self, "apThis"}, {"typedef:uint32_t", "auiFlags"}};
		if (!popsFit(4))
			throw Skip{"ABI: pops " + std::to_string(m_pops) + ", signature 4 bytes"};
	}
	if (int64_t bytes = stackBytes(d); !popsFit(bytes))
		throw Skip{"ABI: pops " + std::to_string(m_pops) + ", signature " + std::to_string(bytes) + " bytes"};
	// a second structor of the same parameters takes its address in its name, as does one that would overload a
	// structor src registers by name alone (its FNVD_REPLACES_CONSTRUCT could no longer pick it) or one src has of
	// as many parameters (src's calls could turn ambiguous)
	std::string keys = paramKeys(d.params);
	auto clashes = [&](const std::string& candidate) {
		if (registeredByName(owner.name + "::" + candidate))
			return true;
		for (const SrcModel::Function* f : m_model.functionsNamed(owner.name + "::" + candidate))
			if (const SrcModel::Type* t = m_model.type(f->type))
			{
				std::vector<Param> ps;
				for (const std::string& k : t->params)
					ps.push_back({k, ""});
				if (paramKeys(ps) == keys || ps.size() == d.params.size())
					return true;
			}
		return m_newSignatures.count(owner.name + "::" + candidate + "(" + keys + ")") != 0;
	};
	if (clashes(name))
	{
		d.member = name + "_" + hex8(d.ea);
		d.marker = "Not named in the PDB or IDA; name chosen here.";
	}
	// a structor name is the class's own: a base's of the same name is not hidden by it in any use src makes
	if (owner.field(d.member))
		throw Skip{"name taken"};
	m_newSignatures.emplace(owner.name + "::" + d.member + "(" + keys + ")", d.ea);
	if (!owner.pattern.empty())
	{
		const std::vector<const SrcModel::Function*>& declared = memberDeclarations(owner, name);
		if (!declared.empty() && std::all_of(declared.begin(), declared.end(), [&](const SrcModel::Function* f) {
				return m_model.definition(f->mangled) != nullptr;
			}))
			return instanceBody(d, owner, declared, fd);
		// an explicit specialization declares the template's member: its signature must be one the template has
		if (!declared.empty() && !templateDeclares(d, owner, name))
			throw Skip{"signature differs from the template's declaration"};
		if (declared.empty())
		{
			// the declaration the template gets must hold for every instance: no type naming this one's arguments
			// (a record or enum one of them spells, at any depth: `const msvc9::basic_string<...>&` of
			// pair<string, string>), which only the template's parameters could spell; src's model has no names of
			// those, so such a structor is skipped
			size_t open = owner.name.find('<');
			std::string arguments = owner.name.substr(open + 1, owner.name.rfind('>') - open - 1);
			auto named = [&](const std::string& type) {
				auto inner = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == ':'; };
				for (size_t at = arguments.find(type); at != std::string::npos; at = arguments.find(type, at + 1))
				{
					size_t end = at + type.size();
					if ((at == 0 || !inner(arguments[at - 1])) &&
						(end == arguments.size() || (!inner(arguments[end]) && arguments[end] != '<')))
						return true;
				}
				return false;
			};
			std::function<bool(const std::string&)> namesArgument = [&](const std::string& key) {
				if (std::optional<SrcTypes::Function> f = SrcTypes::function(key))
					return namesArgument(f->ret) ||
						   std::any_of(f->params.begin(), f->params.end(), [&](const std::string& p) {
							   return namesArgument(p);
						   });
				SrcTypes::Parts p = m_types.parts(key);
				if (p.kind == "enum")
					return named(p.name);
				if (p.kind == "record")
				{
					// the record, or a record or enum among its own template arguments (BSTFreeListElem<T>'s T)
					if (named(p.name))
						return true;
					static const std::regex word(R"([A-Za-z_][A-Za-z_0-9:]*)");
					for (auto it = std::sregex_iterator(p.name.begin(), p.name.end(), word); it != std::sregex_iterator();
						 ++it)
						if ((m_model.record(it->str()) || m_model.enumeration(it->str())) && named(it->str()))
							return true;
					return false;
				}
				return !p.of.empty() && namesArgument(p.of);
			};
			for (size_t k = 1; k < d.params.size(); ++k)
				if (namesArgument(m_types.canonical(d.params[k].key)))
					throw Skip{"structor parameters name the template instance's arguments"};
			d.templateDeclaration = lastComponent(owner.pattern);
		}
		d.outcome = "specialized";
		d.pdbName = n.qualified;
		home(d);
		return d;
	}
	d.outcome = "declared";
	placeMember(d, owner, n.qualified);
	return d;
}

Decl Declarer::virtualSlot(Decl d, const Name& n, const SrcModel::Record* owner,
						   const std::vector<ExeVtables::Holder>& holders, const func_type_data_t* fd)
{
	d.kind = m_kind = "virtual";
	if (m_usercall)
		throw Skip{"usercall: in a vtable"};
	struct Hit
	{
		const SrcModel::Record* record;
		int64_t offset;
		const SrcModel::Slot* slot;
		size_t index;  // the slot's
	};
	std::vector<Hit> hits;
	std::set<std::string> functions;
	std::string firstClass;
	for (const ExeVtables::Holder& h : holders)
	{
		const SrcModel::Record* r = recordNamed(h.table->cls);
		// an instance srcdb has no record of: the slots of its template's, from an instance srcdb has (their kinds
		// only; the functions are that instance's)
		const SrcModel::Record* layout = r;
		if (std::string name = msvc9Names(h.table->cls), why; !r && name.back() == '>')
			if (size_t open = name.find('<'); open != std::string::npos && m_patterns.count(name.substr(0, open)))
				if ((r = instanceOf(name, why)))
					layout = m_siblings.count(r) ? m_siblings.at(r) : nullptr;
		if (firstClass.empty())
			firstClass = r ? r->name : h.table->cls;
		if (!r || !layout)
			continue;
		for (const SrcModel::Vtable& vt : layout->vtables)
			if (vt.offset == h.table->offset && h.slot >= 0 && static_cast<size_t>(h.slot) < vt.slots.size())
			{
				hits.push_back({r, vt.offset, &vt.slots[h.slot], static_cast<size_t>(h.slot)});
				if (layout == r)
					functions.insert(vt.slots[h.slot].function);
			}
	}
	// a fold: the exe's vtables hold the address where several classes' src slots name their own functions, none
	// registered (chooseRef took none). The owner the porting rules' "Folds" names gets the forwarder: the base-most
	// class among them (one that derives from none of the others), of several such the alphabetically first; every
	// other member's declaration gets the rule's `// Same function as` comment
	std::function<bool(const std::string&, const std::string&)> derives = [&](const std::string& record,
																			  const std::string& base) {
		const SrcModel::Record* r = m_model.record(record);
		return r && std::any_of(r->bases.begin(), r->bases.end(), [&](const SrcModel::Base& b) {
				   return b.record == base || derives(b.record, base);
			   });
	};
	auto baseMost = [&](const std::set<std::string>& classes) {
		for (const std::string& cls : classes)
			if (std::none_of(classes.begin(), classes.end(),
							 [&](const std::string& other) { return other != cls && derives(cls, other); }))
				return cls;
		return *classes.begin();
	};
	// several classes' deleting destructors (no src declarations: srcdb's slots name the `??_E`/`??_G` functions):
	// the owner's DeletingDestruct; the others have no declaration to comment
	std::set<std::string> deletingClasses;
	for (const Hit& h : hits)
		if (h.slot->kind == "deleting" && h.offset == 0)
			deletingClasses.insert(h.record->name);
	if (functions.size() > 1 && deletingClasses.size() > 1 && hits.size() == holders.size() &&
		std::all_of(hits.begin(), hits.end(),
					[](const Hit& h) { return h.slot->kind == "deleting" && h.offset == 0; }))
	{
		// a template instance's: the owner's DeletingDestruct is its explicit specialization (structor), the other
		// instances' the template's
		const SrcModel::Record* cls = m_model.record(baseMost(deletingClasses));
		if (!cls)
			throw Skip{"fold: no record of " + baseMost(deletingClasses)};
		Name deleting = n;
		deleting.structor = "deleting";
		deleting.unnamed = false;
		deleting.scope = cls->name;
		deleting.last = "`scalar deleting destructor'";
		deleting.qualified = deleting.scope + "::" + deleting.last;
		Decl declared = structor(d, *cls, deleting, fd);
		declared.proof = "fold owner of the deleting destructors of " + std::to_string(deletingClasses.size()) +
						 " classes";
		return declared;
	}
	const SrcModel::Function* f = nullptr;
	std::vector<const SrcModel::Function*> copies;
	// a class template instance's copy: the comment goes on src's explicit specialization of it; one made from the
	// template's body (none) gets nothing, a comment on the template's member marking every instance
	auto instance = [&](const SrcModel::Function& member) {
		const SrcModel::Record* r = asClass(m_model.record(member.cls));
		return r && (!r->pattern.empty() || nestedInInstance(*r));
	};
	if (functions.size() > 1)
	{
		std::map<std::string, std::vector<const SrcModel::Function*>> byClass;
		for (const std::string& mangled : functions)
		{
			const SrcModel::Function* member = m_model.function(mangled);
			if (!member || member->cls.empty())
				throw Skip{"fold: a slot's function is no member src declares"};
			byClass[member->cls].push_back(member);
		}
		// two slot functions of one class: each is the PDB's virtual at its slot (src's vtables are the exe's), else
		// which name is which slot's is not known
		for (auto& [cls, members] : byClass)
		{
			if (members.size() < 2)
				continue;
			for (const SrcModel::Function* member : members)
				for (const Hit& h : hits)
					if (h.slot->function == member->mangled)
						if (std::optional<std::string> why = pdbSlotMismatch(*h.record, h.offset, h.index))
							throw Skip{"fold: two slot functions of " + cls + ", " + *why};
			std::sort(members.begin(), members.end(),
					  [](const SrcModel::Function* a, const SrcModel::Function* b) { return a->name < b->name; });
		}
		std::set<std::string> classes;
		for (const auto& [cls, members] : byClass)
			classes.insert(cls);
		// of one class's several, the alphabetically first
		f = byClass.at(baseMost(classes)).front();
		for (const auto& [cls, members] : byClass)
			for (const SrcModel::Function* member : members)
				if (member != f)
					copies.push_back(member);
	}
	else if (!functions.empty())
		f = m_model.function(*functions.begin());
	const Hit* hit = nullptr;
	for (const Hit& h : hits)
		if (f && h.record->name == f->cls)
			hit = &h;
	// the owner's declaration with the other members' fold comments (porting rules "Folds"), read before anything is
	// claimed for the owner (a copy whose declaration cannot take the comment skips the address)
	std::vector<LineInsert> comments;
	std::string names;
	size_t generic = 0;  // copies made from their template's body
	for (const SrcModel::Function* copy : copies)
	{
		if (!instance(*copy))
			comments.push_back(foldComment(*copy, *f, d.ea));
		else if (m_model.specializations().count(copy->mangled))
			for (LineInsert& comment : specializationFoldComments(*copy, *f, d.ea))
				comments.push_back(std::move(comment));
		else
		{
			++generic;
			continue;
		}
		names += (names.empty() ? "" : ", ") + copy->name;
	}
	auto withCopies = [&](Decl declared) {
		if (copies.empty())
			return declared;
		declared.foldCopies = comments;
		declared.proof = (declared.proof.empty() ? "" : declared.proof + "; ") + "fold owner" +
						 (names.empty() ? "" : "; Same function as comments: " + names) +
						 (generic ? "; the template's member stays for " + std::to_string(generic) + " other instances"
								  : "");
		return declared;
	};
	// a slot src holds as one class's virtual destructor (in every table holding the address: none folds it with
	// another class's): the exe's function is that class's deleting destructor (srcdb's slot names the `??_E`/`??_G`
	// function, no declaration), whatever IDA calls it (`Destroy`, `Func0000`); a secondary table's is an adjustor
	// thunk to the primary one's
	if (!hits.empty() && hits.size() == holders.size() && std::all_of(hits.begin(), hits.end(), [&](const Hit& h) {
			return h.slot->kind == "deleting" && h.offset == 0 && h.record == hits.front().record;
		}))
	{
		const SrcModel::Record& cls = *hits.front().record;
		Name deleting = n;
		deleting.structor = "deleting";
		deleting.unnamed = false;
		deleting.scope = cls.name;
		deleting.last = "`scalar deleting destructor'";
		deleting.qualified = deleting.scope + "::" + deleting.last;
		return structor(d, cls, deleting, fd);
	}
	// a member src's slot inherits from a base the exe has no vtable of (no instance of its own: the Havok agents'
	// hkpSymmetricAgentLinearCast<T>), or a fold's owner that a derived class's slot holds where the owner's own class
	// has no exe vtable holding the address: the exe's function is that base's member
	bool inherited = std::any_of(hits.begin(), hits.end(), [&](const Hit& h) {
		return f && h.slot->function == f->mangled && h.slot->kind == "method";
	});
	if (f && !hit && !hits.empty() &&
		((hits.front().slot->kind == "method" && m_vtables.of(f->cls).empty()) || (!copies.empty() && inherited)))
		if (const SrcModel::Record* base = m_model.record(f->cls))
			for (const SrcModel::Vtable& vt : base->vtables)
				for (const SrcModel::Slot& slot : vt.slots)
					if (slot.function == f->mangled)
					{
						std::optional<std::string> self = subobjectSelf(*base, vt.offset);
						if (!self)
							throw Skip{"no base of " + base->name + " at " + hex(vt.offset)};
						return withCopies(existing(d, *f, *self, true));
					}
	if (!f || !hit || hit->slot->kind != "method")
	{
		if (!copies.empty())
			throw Skip{"fold: the owner's slot is no method src's vtable holds"};
		std::string name = n.unnamed ? subName(d.ea) : n.qualified;
		needs.push_back({owner ? owner->name : firstClass, d.ea, name});
		throw Skip{"virtual not in src's vtable"};
	}
	// a secondary table's entry takes the base subobject at its offset as `this`
	std::optional<std::string> self = subobjectSelf(*hit->record, hit->offset);
	if (!self)
		throw Skip{"no base of " + hit->record->name + " at " + hex(hit->offset)};
	return withCopies(existing(d, *f, *self, true));
}

LineInsert Declarer::foldComment(const SrcModel::Function& copy, const SrcModel::Function& owner, uint64_t ea)
{
	// the copy's own declaration, in src/ and outside the frozen gen/Prototypes.hpp, written on lines of its own
	if (copy.file.rfind("src/", 0) != 0)
		throw Skip{"fold: " + copy.name + " is declared outside src/: " + copy.file};
	const std::vector<std::string>& lines = m_text.lines(copy.file);
	if (copy.line < 1 || copy.line > static_cast<int64_t>(lines.size()) ||
		lines[copy.line - 1].find(lastComponent(copy.name)) == std::string::npos)
		throw Skip{"fold: no declaration of " + copy.name + " at " + copy.file + ":" + std::to_string(copy.line)};
	std::string above = copy.line > 1 ? trimmedText(lines[copy.line - 2]) : "";
	bool starts = above.empty() || above.starts_with("//") || above.starts_with("#") || above.ends_with(";") ||
				  above.ends_with("{") || above.ends_with("}") || above.ends_with(":");
	if (!starts)
		throw Skip{"fold: " + copy.name + "'s declaration does not start its line (" + copy.file + ":" +
				   std::to_string(copy.line) + ")"};
	const std::string& text = lines[copy.line - 1];
	std::string indent = text.substr(0, text.find_first_not_of(" \t"));
	return {copy.file, copy.line,
			indent + "// Same function as " + owner.name + " (" + address(ea) + "), registered there."};
}

// The text as `//` comment lines at the indentation, wrapped at the spaces to 120 columns (a tab counting 4).
std::string wrappedComment(const std::string& text, const std::string& indent)
{
	size_t room =
		120 - 3 - std::count(indent.begin(), indent.end(), '\t') * 4 - std::count(indent.begin(), indent.end(), ' ');
	std::string comment, line;
	std::istringstream words(text);
	for (std::string word; words >> word;)
	{
		if (!line.empty() && line.size() + 1 + word.size() > room)
		{
			comment += (comment.empty() ? "" : "\n") + indent + "// " + line;
			line.clear();
		}
		line += (line.empty() ? "" : " ") + word;
	}
	return comment + (comment.empty() ? "" : "\n") + indent + "// " + line;
}

std::vector<LineInsert> Declarer::specializationFoldComments(const SrcModel::Function& copy,
															 const SrcModel::Function& owner, uint64_t ea)
{
	std::string text = "Same function as " + owner.name + " (" + address(ea) + "), registered there.";
	std::vector<LineInsert> out;
	auto found = m_model.specializations().find(copy.mangled);
	if (found != m_model.specializations().end())
		for (const SrcModel::Specialization& s : found->second)
		{
			if (s.file.rfind("src/", 0) != 0)
				continue;
			const std::vector<std::string>& lines = m_text.lines(s.file);
			if (s.line < 1 || s.line > static_cast<int64_t>(lines.size()) ||
				!trimmedText(lines[s.line - 1]).starts_with("template"))
				throw Skip{"fold: no `template <>` line of " + copy.name + " at " + s.file + ":" +
						   std::to_string(s.line)};
			// the comment the copy has already, in the `//` run right above it (srcdb's foldRef reads that run)
			std::string above;
			for (int64_t k = s.line - 1; k >= 1 && trimmedText(lines[k - 1]).starts_with("//"); --k)
				above = trimmedText(lines[k - 1]).substr(2) + " " + above;
			if (above.find("Same function as") != std::string::npos)
				continue;
			std::string indent = lines[s.line - 1].substr(0, lines[s.line - 1].find_first_not_of(" \t"));
			std::string comment = wrappedComment(text, indent);
			out.push_back({s.file, s.line, comment});
		}
	if (out.empty() && (found == m_model.specializations().end() ||
						std::none_of(found->second.begin(), found->second.end(),
									 [](const SrcModel::Specialization& s) { return s.file.rfind("src/", 0) == 0; })))
		throw Skip{"fold: src spells no explicit specialization of " + copy.name + " in src/"};
	return out;
}

// src's vtables are the exe's, slot for slot; the PDB's (the Xbox build's) lack the PC's own slots or add some of their
// own, so the two are matched by name: the longest sequence of src's slots whose qualified names the PDB's table of
// the same subobject (the record's vtables by offset against the PDB's subobjects in layout order) holds in the same
// order. A slot on it is the PDB's virtual of src's name there; a deleting destructor is no anchor.
std::optional<std::string> Declarer::pdbSlotMismatch(const SrcModel::Record& record, int64_t offset, size_t index)
{
	std::vector<const SrcModel::Vtable*> tables;
	for (const SrcModel::Vtable& vt : record.vtables)
		tables.push_back(&vt);
	std::sort(tables.begin(), tables.end(),
			  [](const SrcModel::Vtable* a, const SrcModel::Vtable* b) { return a->offset < b->offset; });
	size_t subobject = 0;
	while (subobject < tables.size() && tables[subobject]->offset != offset)
		++subobject;
	if (subobject == tables.size() || index >= tables[subobject]->slots.size())
		return "no src slot " + std::to_string(index) + " of " + record.name + " at " + hex(offset);
	const std::vector<SrcModel::Slot>& own = tables[subobject]->slots;
	std::string where = "slot " + std::to_string(index) + " of " + record.name + "'s vtable" +
						(offset ? " at " + hex(offset) : std::string()) + " (src: " + own[index].name + ")";
	std::optional<std::vector<std::string>> pdb = m_pdbVtables.slots(record.name, subobject);
	if (!pdb)
		return "the PDB has no vtable of " + record.name + (offset ? " at " + hex(offset) : "") + " to name " + where;
	std::vector<std::string> theirs;
	for (const std::string& name : *pdb)
		theirs.push_back(name.empty() ? "" : msvc9Names(name));
	// lengths[i][j]: the longest common subsequence of src's slots from i and the PDB's from j
	size_t n = own.size(), m = theirs.size();
	std::vector<std::vector<uint16_t>> lengths(n + 1, std::vector<uint16_t>(m + 1, 0));
	auto same = [&](size_t i, size_t j) {
		return own[i].kind == "method" && !theirs[j].empty() && own[i].name == theirs[j];
	};
	for (size_t i = n; i-- > 0;)
		for (size_t j = m; j-- > 0;)
			lengths[i][j] = same(i, j) ? lengths[i + 1][j + 1] + 1 : std::max(lengths[i + 1][j], lengths[i][j + 1]);
	for (size_t i = 0, j = 0; i < n && j < m;)
		if (same(i, j) && lengths[i][j] == lengths[i + 1][j + 1] + 1)
		{
			if (i == index)
				return std::nullopt;
			++i, ++j;
		}
		else if (lengths[i + 1][j] >= lengths[i][j + 1])
			++i;
		else
			++j;
	return "the PDB's vtable of " + record.name + " does not hold " + where + " in src's order";
}

std::optional<std::string> Declarer::subobjectSelf(const SrcModel::Record& record, int64_t offset) const
{
	if (offset == 0)
		return "this";
	const SrcModel::Record* r = &record;
	while (r && offset != 0)
	{
		const SrcModel::Base* inner = nullptr;
		for (const SrcModel::Base& b : r->bases)
			if (!b.isVirtual && b.offset <= offset && (!inner || b.offset > inner->offset))
				inner = &b;
		if (!inner)
			break;
		if (inner->offset == offset)
			return "static_cast<" + inner->record + "*>(this)";
		offset -= inner->offset;
		r = m_model.record(inner->record);
	}
	return std::nullopt;
}

Decl Declarer::instanceBody(Decl d, const SrcModel::Record& owner,
							const std::vector<const SrcModel::Function*>& declared, const func_type_data_t* fd)
{
	const SrcModel::Function* f = declared.size() == 1 ? declared[0] : undefinedFit(declared[0]->name, fd, true);
	if (!f)
		throw Skip{std::string(kTemplateBody) + ": ABI: pops " + std::to_string(m_pops) +
				   ", no defined overload fits"};
	if (m_siblings.count(&owner))
		return siblingSpecialization(d, owner, d.member, d.pdbName);
	return addressed(d, *f, false);
}

Decl Declarer::templateMember(Decl d, const SrcModel::Record& owner, const std::string& name, const Name& n,
							  const func_type_data_t* fd)
{
	d.kind = m_kind = "template";
	// the template's declarations are of the standard conventions; a usercall instance's is the porter's
	if (m_usercall)
		throw Skip{"usercall: a template instance's member"};
	const std::vector<const SrcModel::Function*>& declared = memberDeclarations(owner, name);
	if (declared.empty())
		throw Skip{"template lacks member: " + owner.name + "::" + name};
	if (m_siblings.count(&owner))
	{
		// an instance srcdb has no record of (a sibling's declarations name no address of this one): the template
		// declares the member, the instance's signature is the PDB's (else IDA's) as the exe fits it, specialized
		d.owner = &owner;
		d.member = name;
		if (!pdbSignature(d, n.qualified, true, fd))
			idbSignature(d, fd);
		d.isStatic = d.cc != "thiscall";
		return siblingSpecialization(d, owner, name, n.qualified);
	}
	if (std::all_of(declared.begin(), declared.end(),
					[&](const SrcModel::Function* f) { return m_model.definition(f->mangled) != nullptr; }))
		return instanceBody(d, owner, declared, fd);
	// of the template's declarations, an undefined one that fits, else a defined one (the template's body)
	const SrcModel::Function* f = undefinedFit(owner.name + "::" + name, fd);
	if (!f)
		f = undefinedFit(owner.name + "::" + name, fd, true);
	if (!f)
		throw Skip{"ABI: pops " + std::to_string(m_pops) + ", no overload fits"};
	return existing(d, *f, "this");
}

bool Declarer::pdbSignature(Decl& d, const std::string& name, bool member, const func_type_data_t* fd)
{
	const std::vector<const SrcModel::PdbFunction*>* overloads = pdbOverloads(name);
	if (!overloads)
		return false;
	std::vector<Decl> fits;
	std::string why;
	for (const SrcModel::PdbFunction* o : *overloads)
	{
		const SrcModel::Type* type = m_model.type(o->type);
		if (!type || type->kind != "fn")
			continue;
		if (type->variadic)
		{
			why = "variadic";
			continue;
		}
		Decl c = d;
		c.params.clear();
		std::string missing;
		std::optional<std::string> ret = srcKey(type->ret, missing);
		for (size_t k = 0; ret && k < type->params.size(); ++k)
		{
			std::optional<std::string> key = srcKey(type->params[k], missing);
			if (!key)
			{
				ret.reset();
				break;
			}
			c.params.push_back({*key, k < o->params.size() ? o->params[k] : ""});
		}
		if (!ret)
		{
			why = missing;
			continue;
		}
		c.ret = *ret;
		// a record result: the exe's hidden result pointer, first after `this`
		SrcTypes::Parts result = m_types.parts(m_types.canonical(c.ret));
		if (result.kind == "record")
		{
			c.ret = "ptr(" + c.ret + ")";
			c.params.insert(c.params.begin(), Param{c.ret, "apResult"});
		}
		else if (result.kind == "rref" || m_types.parts(m_types.canonical(c.ret)).kind == "array")
		{
			why = "returns " + type->ret;
			continue;
		}
		try
		{
			if (member && !o->isStatic)
			{
				c.cc = "thiscall";
				c.isStatic = false;
				if (int64_t bytes = stackBytes(c); !popsFit(bytes))
				{
					why = "ABI: pops " + std::to_string(m_pops) + ", signature " + std::to_string(bytes) + " bytes";
					continue;
				}
			}
			else
			{
				c.isStatic = member;
				freeConvention(c, fd);
			}
		}
		catch (const Skip& skip)
		{
			why = skip.reason;
			continue;
		}
		fits.push_back(std::move(c));
	}
	auto alike = [&](const Decl& a, const Decl& b) {
		return paramKeys(a.params) == paramKeys(b.params) && m_types.canonical(a.ret) == m_types.canonical(b.ret);
	};
	if (fits.size() > 1)
		if (std::optional<std::vector<Param>> idb = idbParams(fd))
		{
			// of several that fit the bytes, the overload whose parameter types are IDA's prototype's
			std::vector<Decl> matching;
			for (const Decl& c : fits)
				if (idbKeys(c.params) == idbKeys(*idb))
					matching.push_back(c);
			if (!matching.empty())
				fits = std::move(matching);
		}
	// overloads alike to the exe (the same parameter types) are one
	for (size_t k = 1; k < fits.size(); ++k)
		if (!alike(fits[k], fits[0]))
			throw Skip{"ambiguous overloads"};
	if (fits.empty())
		throw Skip{why.empty() ? "no PDB overload of a function type" : why};
	std::vector<Param> params = fits[0].params;
	// parameter names: the PDB's, else a<k>; each an identifier used once
	std::set<std::string> used;
	for (size_t k = 0; k < params.size(); ++k)
	{
		std::string& name = params[k].name;
		if (!identifier(name) || name == "this" || used.count(name) || typeName(name))
			name = "a" + std::to_string(k + 1);
		used.insert(name);
	}
	std::string marker = d.marker, member_ = d.member;
	d = fits[0];
	d.params = params;
	d.marker = marker;
	d.member = member_;
	return true;
}

void Declarer::freeConvention(Decl& d, const func_type_data_t* fd)
{
	// the convention is the exe's alone: a function whose pops are an unknown tail-jump target's may be either
	if (!m_popsKnown)
		throw Skip{"ABI: pops what an unknown tail-jump target pops"};
	if (fd && (fd->get_cc() & CM_CC_MASK) == CM_CC_FASTCALL)
		d.cc = "fastcall";
	else
		d.cc = m_pops == 0 ? "cdecl" : "stdcall";
	if (d.cc == "cdecl")
	{
		// the caller pops: the signature's arguments cover every stack slot the code reads (IDA's argument count is no
		// fact of the exe: a script command handler that reads none of its eight arguments has 0)
		if (int64_t bytes = stackBytes(d); m_readBytes > bytes)
			throw Skip{"ABI: cdecl, the code reads " + std::to_string(m_readBytes) + " argument bytes, the signature " +
					   std::to_string(bytes)};
		return;
	}
	if (int64_t bytes = stackBytes(d); bytes != m_pops)
		throw Skip{"ABI: pops " + std::to_string(m_pops) + ", signature " + std::to_string(bytes) + " bytes"};
}

bool Declarer::constantSignature(Decl& d, const func_type_data_t* fd)
{
	// the constants' one function type: every typed constant at the address agrees, `void*` ones say nothing
	std::optional<SrcTypes::Function> type;
	for (const SrcModel::Ref& r : m_model.refsAt(d.ea))
	{
		if (r.kind != "original_function" || placeholder(r))
			continue;
		SrcTypes::Parts pointer = m_types.parts(m_types.bare(r.type));
		std::optional<SrcTypes::Function> f = pointer.kind == "ptr" ? SrcTypes::function(pointer.of) : std::nullopt;
		if (!f)
			continue;
		if (type && SrcTypes::functionKey(*type) != SrcTypes::functionKey(*f))
			return false;
		type = f;
		d.proof = "signature: src constant " + r.target;
	}
	auto none = [&] {
		d.proof.clear();
		return false;
	};
	if (!type || type->variadic)
		return none();
	// a type the constant's TU alone declares cannot be named where the function is declared
	auto local = [](const std::string& key) { return key.find(" @") != std::string::npos; };
	if (local(type->ret) || std::any_of(type->params.begin(), type->params.end(), local))
		return none();
	// a record result: src's pointer types spell the exe's hidden result pointer themselves
	if (m_types.parts(m_types.canonical(type->ret)).kind == "record")
		return none();
	bool thiscall = type->cc == "thiscall";
	if (thiscall && type->params.empty())
		return none();
	// the convention is the exe's: a thiscall constant where the code reads ecx or IDA's prototype is a thiscall, any
	// other where the code reads no ecx (a fastcall's own) and no edx but a fastcall's
	bool idaThiscall = fd && (fd->get_cc() & CM_CC_MASK) == CM_CC_THISCALL;
	bool idaFastcall = fd && (fd->get_cc() & CM_CC_MASK) == CM_CC_FASTCALL;
	if (thiscall ? !(m_readsEcx || idaThiscall) || m_readsEdx : idaThiscall || (!idaFastcall && m_readsEcx))
		return none();
	if ((type->cc == "fastcall") != idaFastcall)
		return none();
	Decl c = d;
	c.ret = type->ret;
	c.params.clear();
	// parameter names: IDA's where its prototype has as many, else a<k>; each an identifier used once
	size_t from = thiscall ? 1 : 0;
	size_t idaFrom = idaThiscall ? 1 : 0;
	bool idaNames = fd && fd->size() >= idaFrom && fd->size() - idaFrom == type->params.size() - from;
	std::set<std::string> used;
	for (size_t k = from; k < type->params.size(); ++k)
	{
		std::string name = idaNames ? (*fd)[idaFrom + k - from].name.c_str() : "";
		if (!identifier(name) || name == "this" || used.count(name) || typeName(name))
			name = "a" + std::to_string(c.params.size() + 1);
		used.insert(name);
		c.params.push_back({type->params[k], name});
	}
	try
	{
		if (thiscall)
		{
			c.cc = "thiscall";
			int64_t bytes = stackBytes(c);
			if (!popsFit(bytes) || m_readBytes > bytes)
				return none();
		}
		else
			freeConvention(c, fd);
	}
	catch (const Skip&)
	{
		return none();
	}
	d = std::move(c);
	return true;
}

void Declarer::idbSignature(Decl& d, const func_type_data_t* fd)
{
	if (!m_usercall && constantSignature(d, fd))
		return;
	if (!fd)
		throw Skip{"no IDA prototype"};
	if (m_usercall)
		return userSignature(d, *fd);
	func_type_data_t details = *fd;
	tinfo_t type;
	if (!type.create_func(details))
		throw Skip{"no IDA prototype"};
	details = *fd;
	if (contradicted(m_facts, static_cast<ea_t>(d.ea), type))
	{
		std::optional<tinfo_t> exe = exePrototype(m_facts, static_cast<ea_t>(d.ea), type);
		if (!exe || !exe->get_func_details(&details))
			throw Skip{"IDA's prototype contradicts the exe"};
	}
	callcnv_t cc = details.get_cc() & CM_CC_MASK;
	bool thiscall = cc == CM_CC_THISCALL;
	if (cc == CM_CC_ELLIPSIS)
		throw Skip{"variadic"};
	std::string why;
	if (details.rettype.is_udt())
		throw Skip{"returns a record by value (IDA's prototype)"};
	std::optional<std::string> ret = m_types.keyOf(details.rettype, why);
	if (!ret)
		throw Skip{"type not in src: " + why};
	// IDA's va_list is the exe's char*; src's (an array type on x64) cannot be returned
	if (*ret == "typedef:va_list")
		ret = "ptr(builtin:char)";
	d.ret = *ret;
	d.params.clear();
	std::set<std::string> used;
	for (size_t k = thiscall ? 1 : 0; k < details.size(); ++k)
	{
		std::optional<std::string> key = m_types.keyOf(details[k].type, why);
		if (!key)
			throw Skip{"type not in src: " + why};
		if (details[k].type.is_udt() && details[k].type.get_size() == BADSIZE)
			throw Skip{"type not in src: " + text(details[k].type)};
		std::string name = details[k].name.c_str();
		if (!identifier(name) || used.count(name) || typeName(name))
			name = "a" + std::to_string(d.params.size() + 1);
		used.insert(name);
		d.params.push_back({*key, name});
	}
	if (thiscall)
	{
		d.cc = "thiscall";
		if (int64_t bytes = stackBytes(d); !popsFit(bytes))
			throw Skip{"ABI: pops " + std::to_string(m_pops) + ", signature " + std::to_string(bytes) + " bytes"};
	}
	else
		freeConvention(d, &details);
}

void Declarer::checkMemberName(const Decl& d, const SrcModel::Record& owner)
{
	if (owner.field(d.member))
		throw Skip{"name taken"};
	std::string keys = paramKeys(d.params);
	if (m_model.functionsNamed(owner.name + "::" + d.member).empty())
	{
		// a new name would hide a base's functions of the name, or override a virtual of the same parameters
		std::vector<std::string> bases{owner.name};
		for (size_t k = 0; k < bases.size(); ++k)
		{
			const SrcModel::Record* r = m_model.record(bases[k]);
			for (const SrcModel::Base& b : r ? r->bases : std::vector<SrcModel::Base>{})
			{
				bases.push_back(b.record);
				for (const SrcModel::Function* f : m_model.functionsNamed(b.record + "::" + d.member))
				{
					const SrcModel::Type* t = m_model.type(f->type);
					std::vector<Param> ps;
					for (const std::string& key : t ? t->params : std::vector<std::string>{})
						ps.push_back({key, ""});
					throw Skip{std::string(paramKeys(ps) == keys && f->isVirtual ? "would override " : "would hide ") +
							   f->name};
				}
			}
		}
	}
}

// A new member name on owner must keep every lookup of it in a derived class unambiguous: a class deriving from owner
// by one base and, by another, from a class with a member of the name, and none of its own (TESSoundFile's
// GetModelLength beside TESModel's in the classes deriving from both), would find it in two bases
void Declarer::checkDerivedLookups(const Decl& d, const SrcModel::Record& owner)
{
	std::map<std::string, bool> reachesOwner, declaresName;
	std::function<bool(const std::string&)> reaches = [&](const std::string& record) -> bool {
		if (record == owner.name)
			return true;
		auto [at, added] = reachesOwner.emplace(record, false);
		if (!added)
			return at->second;
		const SrcModel::Record* r = m_model.record(record);
		bool found = r && std::any_of(r->bases.begin(), r->bases.end(),
									  [&](const SrcModel::Base& b) { return reaches(b.record); });
		return reachesOwner[record] = found;
	};
	std::function<bool(const std::string&)> declares = [&](const std::string& record) -> bool {
		auto [at, added] = declaresName.emplace(record, false);
		if (!added)
			return at->second;
		const SrcModel::Record* r = m_model.record(record);
		bool found = !m_model.functionsNamed(record + "::" + d.member).empty() ||
					 (r && std::any_of(r->bases.begin(), r->bases.end(),
									   [&](const SrcModel::Base& b) { return declares(b.record); }));
		return declaresName[record] = found;
	};
	for (const auto& [name, r] : m_model.records())
	{
		if (name == owner.name || !reaches(name) || !m_model.functionsNamed(name + "::" + d.member).empty())
			continue;
		for (const SrcModel::Base& b : r.bases)
			if (!reaches(b.record) && declares(b.record))
				throw Skip{"conflict: " + name + " derives from " + owner.name + " and " + b.record + ", which has " +
						   d.member + " (its lookups would turn ambiguous)"};
	}
}

Decl Declarer::member(Decl d, const SrcModel::Record& owner, const Name& n, const func_type_data_t* fd, bool idaOnly)
{
	d.kind = m_kind = n.unnamed ? "unnamed" : "member";
	// IDA's name of a constructor or destructor whose class only the prototype's `this` gave
	if (!n.unnamed && n.structor.empty())
	{
		std::string bare = withoutSuffix(n.last).value_or(n.last);
		std::string cls = lastComponent(owner.name);
		cls = cls.substr(0, cls.find('<'));
		if (bare == cls || bare == "~" + cls)
		{
			Name s = n;
			s.structor = bare == cls ? "ctor" : "dtor";
			return structor(d, owner, s, fd);
		}
	}
	// a template instance's member, or a member of a class nested in one: the template's declaration, specialized
	if (!owner.pattern.empty() || nestedInInstance(owner))
		return templateMember(d, owner, n.unnamed ? subName(d.ea) : n.last, n, fd);
	if (owner.file.rfind("src/", 0) != 0)
		throw Skip{"class not in src/: " + owner.file};
	if (owner.endLine <= 0)
		throw Skip{"class has no definition in src"};
	std::string name = n.unnamed ? subName(d.ea) : n.last;
	if (name.starts_with("operator"))
	{
		std::string op = name.substr(8);
		static const std::set<std::string> operators = {"==", "!=", "<", ">", "<=", ">=", "[]", "()", "+", "-", "*",
														 "/", "%", "+=", "-=", "*=", "/=", "!", "=", "&", "|", "^",
														 "<<", ">>", "&=", "|=", "^=", "->", "++", "--", "~", "&&",
														 "||", "%=", "<<=", ">>="};
		if (!operators.count(op))
			throw Skip{"operator " + op};
	}
	else if (!identifier(name))
	{
		// a name C++ cannot take (IDA's `X::`helper'`): the address's
		name = subName(d.ea);
		d.kind = m_kind = "unnamed";
	}
	// the PDB's name: IDA's, or IDA's without the `_N` it gives a second address of a name
	std::string pdbName = owner.name + "::" + name;
	bool known = idaOnly ? false : pdbOverloads(n.qualified) || !m_model.functionsNamed(pdbName).empty();
	std::string base = name;
	if (!idaOnly && !known)
		if (std::optional<std::string> stripped = withoutSuffix(name))
			if (pdbOverloads(owner.name + "::" + *stripped) || pdbOverloads(scopeOf(n.qualified) + "::" + *stripped))
				base = *stripped, known = true;
	std::string pdbQualified = idaOnly ? "" : (base == name ? n.qualified : scopeOf(n.qualified) + "::" + base);
	// a usercall: no src declaration or PDB signature spells its registers
	if (!idaOnly && !m_usercall)
		if (const SrcModel::Function* f = undefinedFit(owner.name + "::" + base, fd))
			return existing(d, *f, "this");
	d.owner = &owner;
	d.member = base;
	if (n.unnamed || d.kind == "unnamed")
		d.marker = "Not named in the PDB or IDA; name chosen here.";
	else if (!pdbOverloads(pdbQualified.empty() ? pdbName : pdbQualified))
		d.marker = "IDA name; not in the PDB.";
	if (idaOnly || m_usercall || !pdbSignature(d, pdbQualified, true, fd))
		idbSignature(d, fd);
	if (d.cc != "thiscall" && d.selfReg.empty())
		d.isStatic = true;
	// a member operator is non-static and takes at most one parameter (operator() any): a hidden result pointer or a
	// static convention makes the exe's function no operator C++ can declare
	if (d.member.starts_with("operator") && d.member != "operator()" && (d.isStatic || d.params.size() > 1))
		throw Skip{d.isStatic ? "static operator" : "operator with " + std::to_string(d.params.size()) + " parameters"};
	if (d.member == "operator=" && d.params.size() == 1)
	{
		std::string p = m_types.canonical(d.params[0].key);
		if (p == "record:" + owner.name || p == "lref(record:" + owner.name + ")")
			throw Skip{"copy assignment"};
	}
	// a name src (or this run) has with these parameters: the exe's second copy takes IDA's name, else its address;
	// so does one that names a type of the class, would overload a function src registers by name alone (its
	// FNVD_REPLACES could no longer pick it), or one src has of as many parameters (src's calls could turn ambiguous)
	std::string keys = paramKeys(d.params);
	const SrcModel::Function* unregistered = nullptr;  // src's definition of the name and parameters, registered nowhere
	auto clash = [&](const std::string& candidate) -> std::optional<std::string> {
		if (typeMember(owner.name, candidate))
			return "";
		if (m_newSignatures.count(owner.name + "::" + candidate + "(" + keys + ")"))
			return "declared for another address in this run";
		// src's calls of an overload of as many parameters may turn ambiguous (`f(0, 0)`)
		bool ambiguous = false;
		for (const SrcModel::Function* f : m_model.functionsNamed(owner.name + "::" + candidate))
			if (const SrcModel::Type* t = m_model.type(f->type))
			{
				std::vector<Param> ps;
				for (const std::string& k : t->params)
					ps.push_back({k, ""});
				if (paramKeys(ps) != keys)
				{
					ambiguous = ambiguous || ps.size() == d.params.size();
					continue;
				}
				if (!m_model.definition(f->mangled))
					return std::string("ABI: src's declaration does not fit");
				// defined and registered nowhere: the address's KEEP body where the tie is proven (addressed)
				if (m_model.bodyAddresses(f->mangled).empty())
				{
					unregistered = f;
					return std::string(kUnregistered);
				}
				return "";
			}
		if (ambiguous || registeredByName(owner.name + "::" + candidate))
			return "";
		return std::nullopt;
	};
	if (std::optional<std::string> taken = clash(d.member))
	{
		if (*taken == kUnregistered)
			return addressed(d, *unregistered, false);
		if (taken->starts_with("ABI"))
			throw Skip{*taken};
		std::string ida = identifier(n.last) ? n.last : "";
		if (!ida.empty() && ida != d.member && !clash(ida))
		{
			d.member = ida;
			d.marker = "IDA name; not in the PDB.";
		}
		else
		{
			d.member = d.member.starts_with("operator") ? subName(d.ea) : d.member + "_" + hex8(d.ea);
			d.marker = "Not named in the PDB or IDA; name chosen here.";
			if (clash(d.member))
				throw Skip{"name taken"};
		}
	}
	checkMemberName(d, owner);
	m_newSignatures.emplace(owner.name + "::" + d.member + "(" + keys + ")", d.ea);
	d.self = d.isStatic ? "" : "this";
	d.outcome = "declared";
	placeMember(d, owner, pdbQualified.empty() ? pdbName : pdbQualified);
	return d;
}

Decl Declarer::unowned(Decl d, const Name& n, const func_type_data_t* fd)
{
	// rule 3: a thiscall prototype whose `this` is a src record's; a usercall's whose first argument, in a register,
	// IDA names `this` and is a src record's (else a free function)
	bool userThis = m_usercall && fd && !fd->empty() && (*fd)[0].name == "this" &&
					!userRegister((*fd)[0].argloc).empty();
	bool thiscall = fd && (fd->get_cc() & CM_CC_MASK) == CM_CC_THISCALL;
	const SrcModel::Record* r = nullptr;
	if ((thiscall || userThis) && !fd->empty() && (*fd)[0].type.is_ptr())
	{
		tinfo_t pointee = (*fd)[0].type.get_pointed_object();
		if (pointee.is_udt())
			r = m_types.recordOf(pointee);
	}
	if (thiscall && !r)
		throw Skip{"thiscall, and IDA's this is no src record (" +
				   (fd->empty() ? std::string("none") : text((*fd)[0].type)) + ")"};
	if (r)
	{
		if (!n.structor.empty())
			return structor(d, *r, n, fd);
		Name own = n;
		own.qualified = r->name + "::" + n.last;
		if (!n.unnamed && !identifier(n.last))
			own.unnamed = true;
		return member(d, *r, own, fd, true);
	}
	d.kind = m_kind = n.unnamed ? "unnamed" : "free";
	std::string pdbName = n.qualified;
	if (!n.unnamed && !m_usercall)
		if (const SrcModel::Function* f = undefinedFit(n.qualified, fd))
			return existing(d, *f, "");
	const std::vector<const SrcModel::PdbFunction*>* overloads = n.unnamed ? nullptr : pdbOverloads(n.qualified);
	bool pdbFree = overloads && std::all_of(overloads->begin(), overloads->end(),
											[](const SrcModel::PdbFunction* f) { return f->cls.empty(); });
	// a CRT function (IDA's library flag): its declaration is src/SDK/MSVC's, in namespace msvc9 under its VC9 name
	// (porting rules), and a global one would clash with the C library's
	if (func_t* f = get_func(static_cast<ea_t>(d.ea)); f && (f->flags & FUNC_LIB))
		throw Skip{"CRT function: src/SDK/MSVC's"};
	if (!n.unnamed && ((n.scope.empty() && libraryName(n.last)) || n.scope == "std" || n.scope.starts_with("std::")))
		throw Skip{"CRT function: src/SDK/MSVC's"};
	// a namespace C++ can name (not IDA's `anonymous namespace')
	bool namedScope = true;
	for (std::string s = n.scope; namedScope && !s.empty(); s = scopeOf(s))
		namedScope = identifier(lastComponent(s));
	if (pdbFree && namedScope && identifier(n.last))
	{
		d.member = n.last;
		d.scope = n.scope;
		if (m_usercall)
			idbSignature(d, fd);
		else
			pdbSignature(d, n.qualified, false, fd);
	}
	else
	{
		// a scope src lacks (no record, and plan has passed over a PDB class src lacks) is an owner question: a
		// template parameter's name (IDA's `TAlloc::Allocate` of a Gamebryo allocator interface), a class src does
		// not have, IDA's guess (`bhkUtilFunctions::GetTriangleCountOfShape`) or IDA's `anonymous namespace'. Its
		// bare name would be a free function of the global namespace, where it can overload one src has
		if (!n.scope.empty() && (!namedScope || !m_namespaces.count(n.scope)))
			throw Skip{"scope not in src: " + n.scope +
					   " (an owner question: a template parameter's name, a class src lacks, or IDA's guess)"};
		// IDA's descriptive name stays, in its namespace, when src has the namespace; a PDB class src lacks is the
		// header's job. A name that is a class's (`RevertGlobalData::HUDMainMenu`) would hide the class in the file
		// that declares it
		if (!n.unnamed && identifier(n.last) && (n.scope.empty() || !pdbClass(n.scope)) && !recordNamed(n.last) &&
			!pdbClass(n.last))
		{
			d.member = n.last;
			d.scope = n.scope;
			d.marker = "IDA name; not in the PDB.";
		}
		else
		{
			d.kind = m_kind = "unnamed";
			d.member = subName(d.ea);
			d.marker = "Not named in the PDB or IDA; name chosen here.";
		}
		pdbName = "";
		idbSignature(d, fd);
		if (d.cc == "thiscall")
			throw Skip{"thiscall, and IDA's this is no src record"};
	}
	std::string qualified = (d.scope.empty() ? "" : d.scope + "::") + d.member;
	std::string keys = paramKeys(d.params);
	// src's declaration of these parameters is the function's; one of other parameters makes the new declaration an
	// overload, which can change what src's unqualified calls of the name and its FNVD_REPLACES registrations name
	// (0xA63AE0's `Allocate(int32_t)` beside BSCore's `Allocate(uint32_t)`): renamed in the database to a unique name
	// or given its true owner first
	const SrcModel::Function* other = nullptr;
	for (const SrcModel::Function* f : m_model.functionsNamed(qualified))
		if (const SrcModel::Type* t = m_model.type(f->type))
		{
			std::vector<Param> ps;
			for (const std::string& k : t->params)
				ps.push_back({k, ""});
			if (paramKeys(ps) == keys)
			{
				if (!m_model.definition(f->mangled))
					throw Skip{"ABI: src's declaration does not fit"};
				return addressed(d, *f, false);
			}
			if (!other)
				other = f;
		}
	if (other)
		throw Skip{"overload: src declares " + qualified + " of other parameters (" + other->type + ") at " +
				   other->file + ":" + std::to_string(other->line)};
	if (!m_newSignatures.emplace(qualified + "(" + keys + ")", d.ea).second)
		throw Skip{"declared for another address in this run"};
	// a second signature of the name in this run is an overload of the first
	for (auto it = m_newSignatures.lower_bound(qualified + "(");
		 it != m_newSignatures.end() && it->first.starts_with(qualified + "("); ++it)
		if (it->second != d.ea)
		{
			uint64_t at = it->second;
			m_newSignatures.erase(qualified + "(" + keys + ")");
			throw Skip{"overload: this run declares " + qualified + " of other parameters for " + hexText(at)};
		}
	d.isStatic = false;
	d.self = "";
	d.outcome = "declared";
	placeFree(d, pdbName);
	return d;
}

void Declarer::placeMember(Decl& d, const SrcModel::Record& owner, const std::string& pdbName)
{
	d.header = owner.file;
	d.pdbName = pdbName;
	if (!headerFile(owner.file))
	{
		d.source = owner.file;
		return;
	}
	d.source = templated(&owner) ? "" : pdbSource(pdbName);
	if (d.source.empty() && !templated(&owner))
		d.source = m_model.classSource(owner.name);
	if (d.source.empty())
		d.source = stem(owner.file) + ".cpp";
}

void Declarer::placeFree(Decl& d, const std::string& pdbName)
{
	d.pdbName = pdbName;
	std::string source = pdbName.empty() ? "" : pdbSource(pdbName);
	if (source.empty())
	{
		// the src bodies nearest below and above the address: their file when they share one, else the lower
		// one's folder's Unplaced.h/.cpp
		const auto& refs = m_model.refs();
		auto bodyFile = [](const std::vector<SrcModel::Ref>& list) -> std::string {
			for (const SrcModel::Ref& r : list)
				if (r.kind == "body" && r.file.rfind("src/", 0) == 0 && r.file.ends_with(".cpp"))
					return r.file;
			return "";
		};
		std::string below, above;
		auto at = refs.lower_bound(d.ea);
		for (auto it = at; it != refs.begin() && below.empty();)
			below = bodyFile((--it)->second);
		for (auto it = at; it != refs.end() && above.empty(); ++it)
			above = bodyFile(it->second);
		if (below.empty() && above.empty())
			throw Skip{"no src body near it to place it by"};
		if (below == above)
			source = below;
		else
		{
			std::string near = below.empty() ? above : below;
			source = near.substr(0, near.rfind('/') + 1) + "Unplaced.cpp";
		}
	}
	d.source = source;
	d.header = stem(source) + ".h";
}

// --- text ------------------------------------------------------------------------------------------------------------

class Writer
{
public:
	Writer(const SrcModel& model, SrcTypes& types, FileEdits& edits, IncludeGraph& includes)
		: m_model(model), m_types(types), m_forwarders(types), m_edits(edits), m_includes(includes)
	{
		for (const auto& [name, record] : model.records())
			if (!record.pattern.empty() && record.file.rfind("src/", 0) == 0)
				m_templateFiles.emplace(record.pattern, record.file);
	}

	// the declaration and definition texts, checked whole before anything is added; throws Skip
	void add(const Decl& d);
	// the class blocks, in address order per class, and each header's forward declarations
	void flush();
	// An explicit specialization's declaration at its home (d.header after line d.after) with its comment lines (""
	// for none), and what the header needs for it; throws Skip before any edit
	void declareSpecialization(const Decl& d, const std::string& comment);
	// a function's out-of-line definition head (`R X::f(params) const`)
	std::string definitionHead(const Decl& d) const { return cppSignature(d, true); }
	// --globals: one exe global's, static member's or vtable's declaration (GlobalDecl), with what its header needs;
	// the text checked whole before any edit; throws Skip. Returns the declaration's line.
	std::string declareGlobal(const GlobalDecl& g);
	// A declaration rewritten where it is (declare --hk-symbols): every type it names one src has, what d.header
	// needs for it before line `before`, and what d.source needs for its forwarder; throws Skip before any edit
	void inPlace(const Decl& d, int64_t before);

private:
	// --globals' part of flush(): each header's forward declarations at the end of its preamble, its globals' blocks,
	// the marker comments of vtable runs that moved whole
	void flushGlobals();
	void typeNames(const std::string& key, std::set<std::string>& names) const;
	// the model's lines of the record are its file's (its name on its first, a closing brace on its last); throws Skip
	// for a model older than the file, whose lines would put an edit elsewhere
	void checkLines(const SrcModel::Record& r);
	// a run of FNVD_VFTABLE lines a marker comment heads in a .cpp: its line count and how many of them --globals moves
	struct Run
	{
		int size = 0, moved = 0;
	};
	std::map<std::pair<std::string, int64_t>, Run> m_runs;  // (file, marker line) -> run
	// a run of a .cpp's FNVD_ORIGINAL_FUNCTION lines some of which move (file, first line -> run): its last line, how
	// many constants it has, and each moved one's line and declaration (in m_globalBlocks' (header, at), its index)
	struct ConstantRun
	{
		int64_t last = 0;
		int size = 0;
		std::pair<std::string, int64_t> block;
		std::vector<std::pair<int64_t, size_t>> moved;
	};
	std::map<std::pair<std::string, int64_t>, ConstantRun> m_constantRuns;
	std::map<std::string, std::vector<std::string>> m_globalForwards;  // header -> forward declarations --globals adds
	std::map<std::pair<std::string, int64_t>, std::vector<std::string>> m_globalBlocks;  // (header, line) -> globals
	// what a header needs for a declaration: includes to add, forward declarations (name, line) to add
	struct HeaderNeeds
	{
		std::vector<std::string> includes;
		std::vector<std::pair<std::string, std::string>> forwards;
	};

	void typeNeeds(const std::string& key, std::set<std::string>& records, std::set<std::string>& headers) const;
	// every name in a type key (a template instance's arguments included) is a type srcdb has (a record, enum or
	// typedef: a name src only forward declares is none), and none is a nested record header defines on or after
	// line; throws Skip naming the first that is not
	void checkDeclared(const std::string& key, const std::string& header = "", int64_t line = 0) const;
	bool reaches(const std::string& file, const std::string& target) { return m_includes.reaches(file, target); }
	// src spells a declaration of d's specialization in its home header
	bool specializationDeclared(const Decl& d) const;
	void insertSpecialization(const Decl& d, const std::string& comment);
	// a type key as a specialization's declaration at its home spells it: through its typedefs' targets where the
	// home does not see a typedef (one a header of the specialization's users declares)
	std::string homeKey(const Decl& d, const std::string& key) const;
	// before: the line the declaration goes before (its forward declarations must come earlier); throws Skip where
	// an include would close an include cycle
	HeaderNeeds headerNeeds(const Decl& d, const std::string& header, int64_t before);
	void addHeaderNeeds(const std::string& header, const HeaderNeeds& needs);
	void sourceNeeds(const Decl& d);
	std::string cppSignature(const Decl& d, bool definition) const;
	// an addressed KEEP body's FNVD_ADDRESS assertion; throws Skip before any edit
	void addressMarker(const Decl& d);
	// a class spelled at a place without the namespaces open there, through a `using` alias (its text in alias)
	// where its template arguments hold a comma
	std::string classAt(std::string cls, const std::string& scope, const std::vector<std::string>& lines,
						std::string& alias) const;
	// the function pointer type naming an overloaded f (cls: its class as spelled there); "" for one not overloaded
	std::string overloadPointer(const Decl& d, const SrcModel::Function& f, const std::string& cls) const;

	// the template declarations this run gave `[[noreturn]]`: (file, line)
	std::set<std::pair<std::string, int64_t>> m_noreturnMarked;
	const SrcModel& m_model;
	SrcTypes& m_types;
	Forwarders m_forwarders;
	FileEdits& m_edits;
	std::map<std::pair<std::string, std::string>, std::vector<std::string>> m_blocks;  // (header, class) -> lines
	std::map<std::string, const SrcModel::Record*> m_owners;  // class (or "template X") -> record
	std::set<std::pair<std::string, std::string>> m_templateDeclared;  // (header, declaration) a template got
	std::set<std::string> m_created;
	std::map<std::string, std::string> m_templateFiles;  // a class template's name -> its header
	IncludeGraph& m_includes;
	std::set<std::pair<std::string, std::string>> m_forwarded;   // (header, record) forward declared by this run
	std::map<std::string, std::vector<std::string>> m_forwards;  // header -> the forward declarations it gets
};


std::string Writer::cppSignature(const Decl& d, bool definition) const
{
	return m_forwarders.signature(d, definition, [&](const std::string& key) { return homeKey(d, key); });
}

// The declarations a type needs where it is only declared: a plain record a forward declaration (records), a nested
// one, an enum or a typedef its header (headers); a template instance its template's header, and each name in its
// arguments the same (a forward declaration where it can be, so a header does not include its own includers).
void Writer::typeNeeds(const std::string& key, std::set<std::string>& records, std::set<std::string>& headers) const
{
	if (std::optional<SrcTypes::Function> f = SrcTypes::function(key))
	{
		typeNeeds(f->ret, records, headers);
		for (const std::string& param : f->params)
			typeNeeds(param, records, headers);
		return;
	}
	SrcTypes::Parts p = m_types.parts(key);
	if (p.kind == "record")
	{
		for (size_t k = 0; k < p.name.size();)
		{
			auto word = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == ':'; };
			if (!word(p.name[k]) || p.name[k] == ':')
			{
				++k;
				continue;
			}
			size_t end = k;
			while (end < p.name.size() && word(p.name[end]))
				++end;
			std::string name = p.name.substr(k, end - k);
			k = end;
			if (end < p.name.size() && p.name[end] == '<')
			{
				if (auto found = m_templateFiles.find(name); found != m_templateFiles.end())
					headers.insert(withoutSrc(found->second));
			}
			else if (const SrcModel::Record* r = m_model.record(name))
			{
				if (name.find("::") == std::string::npos && r->pattern.empty())
					records.insert(name);
				else
					m_types.headers("record:" + name, headers);
			}
			else if (m_model.enumeration(name))
				m_types.headers("enum:" + name, headers);
			else if (m_model.typedefNamed(name))
				m_types.headers("typedef:" + name, headers);
		}
	}
	else if (p.kind == "typedef")
	{
		// a typedef of the compiler's headers (`__m128`, xmmintrin.h): their header, as an angle include
		const SrcModel::Typedef* t = m_model.typedefNamed(p.name);
		size_t include = t ? t->file.rfind("/include/") : std::string::npos;
		if (t && !t->file.starts_with("src/") && include != std::string::npos)
			headers.insert("<" + t->file.substr(include + 9) + ">");
		else
			m_types.headers(key, headers);
	}
	else if (p.kind == "enum")
		m_types.headers(key, headers);
	else if (!p.of.empty())
		typeNeeds(p.of, records, headers);
}

void Writer::checkDeclared(const std::string& key, const std::string& header, int64_t line) const
{
	if (std::optional<SrcTypes::Function> f = SrcTypes::function(key))
	{
		checkDeclared(f->ret, header, line);
		for (const std::string& param : f->params)
			checkDeclared(param, header, line);
		return;
	}
	SrcTypes::Parts p = m_types.parts(key);
	if (p.kind != "record")
	{
		if (!p.of.empty())
			checkDeclared(p.of, header, line);
		return;
	}
	auto word = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == ':'; };
	for (size_t k = 0; k < p.name.size();)
	{
		if (!word(p.name[k]) || p.name[k] == ':')
		{
			++k;
			continue;
		}
		size_t end = k;
		while (end < p.name.size() && word(p.name[end]))
			++end;
		std::string name = p.name.substr(k, end - k);
		bool member = k > 0 && p.name[k - 1] == ':';  // a template instance's member (`X<A>::Type`)
		k = end;
		const SrcModel::Record* r = m_model.record(name);
		// a nested record of the header that follows the declaration cannot be named there
		if (r && !header.empty() && r->file == header && r->line >= line && name.find("::") != std::string::npos)
			throw Skip{"nested type declared after the declaration: " + name};
		if (member || (end < p.name.size() && p.name[end] == '<') ||
			std::isdigit(static_cast<unsigned char>(name[0])) || keywords().count(name) || name == "wchar_t" || r ||
			m_model.enumeration(name) || m_model.typedefNamed(name))
			continue;
		throw Skip{"type src has no definition of: " + name};
	}
}

std::string Writer::homeKey(const Decl& d, const std::string& key) const
{
	if (d.outcome != "specialized" || d.header.empty())
		return key;
	for (size_t at = key.find("typedef:"); at != std::string::npos; at = key.find("typedef:", at + 1))
	{
		size_t end = at + 8;
		for (int depth = 0; end < key.size(); ++end)
		{
			if (depth == 0 && (key[end] == ',' || key[end] == ')' || key[end] == ';'))
				break;
			depth += key[end] == '<' ? 1 : key[end] == '>' ? -1 : 0;
		}
		const SrcModel::Typedef* t = m_model.typedefNamed(key.substr(at + 8, end - at - 8));
		if (t && t->file.rfind("src/", 0) == 0 && t->file != d.header && !m_includes.reaches(d.header, t->file))
			return m_forwarders.fixedKey(m_types.canonical(key));
	}
	return key;
}

bool Writer::specializationDeclared(const Decl& d) const
{
	auto found = m_model.specializations().find(d.mangled);
	if (found == m_model.specializations().end())
		return false;
	return std::any_of(found->second.begin(), found->second.end(), [&](const SrcModel::Specialization& s) {
		return s.file == d.header && !s.definition;
	});
}

// The namespaces open at a line of a file's lines, outermost first, joined by `::`.
std::string scopeAt(const std::vector<std::string>& lines, int64_t line)
{
	std::string out;
	for (const auto& [name, at] : namespacesAt(lines, line))
		out += (out.empty() ? "" : "::") + name;
	return out;
}

void Writer::insertSpecialization(const Decl& d, const std::string& comment)
{
	// a free function template's specialization in its own namespace takes its unqualified name
	Decl at = d;
	if (!at.owner && !at.scope.empty() && scopeAt(m_edits.lines(d.header), d.after + 1) == at.scope)
		at.scope.clear();
	m_edits.insert(d.header, d.after + 1,
				   "\n" + (comment.empty() ? "" : comment + "\n") + m_forwarders.templateHeads(d) +
					   wrapped(cppSignature(at, false) + ";", "", "\t"));
}

void Writer::declareSpecialization(const Decl& d, const std::string& comment)
{
	checkDeclared(d.ret);
	for (const Param& p : d.params)
		checkDeclared(p.key);
	if (d.owner)
		checkDeclared("record:" + d.owner->name);
	HeaderNeeds needs = headerNeeds(d, d.header, d.after + 1);
	cppSignature(d, false);
	addHeaderNeeds(d.header, needs);
	insertSpecialization(d, comment);
}

Writer::HeaderNeeds Writer::headerNeeds(const Decl& d, const std::string& header, int64_t before)
{
	std::set<std::string> records, headers;
	typeNeeds(homeKey(d, d.ret), records, headers);
	for (const Param& p : d.params)
		typeNeeds(homeKey(d, p.key), records, headers);
	if (d.outcome == "specialized" && d.owner)
		typeNeeds("record:" + d.owner->name, records, headers);
	// what the specialization's home needs defined there (SpecializationHome)
	for (const std::string& file : d.homeIncludes)
		headers.insert(withoutSrc(file));
	HeaderNeeds needs;
	std::string own = withoutSrc(header);
	for (const std::string& h : headers)
	{
		if (h == own || m_edits.has(header, includeLine(h)))
			continue;
		// a compiler header (`<xmmintrin.h>`) includes no src header
		if (!h.starts_with('<') && reaches("src/" + h, header))
			throw Skip{"would include " + h + ", which includes " + own};
		needs.includes.push_back(h);
	}
	const std::vector<std::string>& lines = m_edits.lines(header);
	for (const std::string& name : records)
	{
		const SrcModel::Record* r = m_model.record(name);
		// a record of the same header defined ahead of the declaration needs nothing; one defined after it does
		if (!r || (r->file == header && r->line < before) || (d.owner && d.owner->name == name))
			continue;
		if (m_edits.has(header, "#include \"" + withoutSrc(r->file) + "\""))
			continue;
		// a forward declaration ahead of where the declaration goes, in the file or added by this run
		std::string tag = r->tag == "struct" ? "struct" : r->tag == "union" ? "union" : "class";
		bool declared = m_forwarded.count({header, name}) != 0;
		for (int64_t k = 1; k < before && k <= static_cast<int64_t>(lines.size()) && !declared; ++k)
		{
			const std::string& line = lines[k - 1];
			size_t text = line.find_first_not_of(" \t");
			for (const char* t : {"class ", "struct ", "union "})
				declared = declared || (text != std::string::npos && line.compare(text, std::string::npos,
																				   std::string(t) + name + ";") == 0);
		}
		if (!declared)
			needs.forwards.push_back({name, tag + " " + name + ";"});
	}
	return needs;
}

void Writer::addHeaderNeeds(const std::string& header, const HeaderNeeds& needs)
{
	for (const std::string& h : needs.includes)
	{
		ensureInclude(m_edits, header, h);
		if (!h.starts_with('<'))
			m_includes.add(header, "src/" + h);
	}
	// the forward declarations go in at flush(), one block after the includes this run adds
	for (const auto& [name, line] : needs.forwards)
		if (m_forwarded.insert({header, name}).second)
			m_forwards[header].push_back(line);
}

void Writer::sourceNeeds(const Decl& d)
{
	std::set<std::string> headers;
	m_types.headers(d.ret, headers);
	for (const Param& p : d.params)
		m_types.headers(p.key, headers);
	if (d.owner)
		m_types.headers("record:" + d.owner->name, headers);
	if (!d.header.empty() && headerFile(d.header))
		headers.insert(withoutSrc(d.header));
	for (const std::string& h : headers)
		if ("src/" + h != d.source)
			ensureInclude(m_edits, d.source, h);
}

// A class as src spells it (fixed64's), without the namespaces open at a place (scope), and through a `using` alias
// where its template arguments hold a comma (it would split a macro's arguments): the alias's text in alias ("" for
// none), wrapped at 120 columns.
std::string Writer::classAt(std::string cls, const std::string& scope, const std::vector<std::string>& lines,
							std::string& alias) const
{
	alias.clear();
	if (!scope.empty() && cls.starts_with(scope + "::"))
		cls = cls.substr(scope.size() + 2);
	if (cls.find(',') == std::string::npos)
		return cls;
	// the alias's name: the class's words without their namespaces and the standard library's default arguments
	// (`vector_pair_basic_string_char_basic_string_char`)
	std::string bare;
	for (size_t k = 0; k < cls.size();)
	{
		if (cls[k] == ',')
		{
			size_t end = k + 1;
			for (int depth = 0; end < cls.size(); ++end)
				if (cls[end] == '<')
					++depth;
				else if ((cls[end] == '>' && depth-- == 0) || (cls[end] == ',' && depth == 0))
					break;
			static const std::regex qualifiers(R"(^\s*(\w+::)*)");
			std::string argument = std::regex_replace(cls.substr(k + 1, end - k - 1), qualifiers, "");
			if (argument.starts_with("allocator<") || argument.starts_with("char_traits<"))
			{
				k = end;
				continue;
			}
		}
		bare += cls[k++];
	}
	std::string aliasName = identifierOf(std::regex_replace(bare, std::regex(R"(\b\w+::)"), ""));
	// at most 120 columns: broken after a template argument's comma where it is longer, continuations one unit in
	int64_t size = static_cast<int64_t>(lines.size());
	std::string unit = indentUnit(lines, 1, size), text = "using " + aliasName + " = " + cls + ";", line;
	size_t unitWidth = unit == "\t" ? 4 : unit.size();
	for (size_t k = 0; k < text.size();)
	{
		size_t comma = text.find(", ", k);
		std::string piece = text.substr(k, comma == std::string::npos ? std::string::npos : comma + 2 - k);
		size_t width = alias.empty() ? line.size() : unitWidth + line.size();
		if (!line.empty() && width + piece.size() > 120)
		{
			alias += (alias.empty() ? "" : "\n" + unit) + line.substr(0, line.size() - 1);
			line.clear();
		}
		line += piece;
		k += piece.size();
	}
	alias += (alias.empty() ? "" : "\n" + unit) + line;
	return aliasName;
}

// The function pointer type a macro names f by where it is overloaded: src declares the name more than once, or a
// base class declares it with another type (a using-declaration may bring that into the class's overload set, which
// srcdb does not record; one of the same type the class's own hides). A member's is a member pointer on cls (the
// class as spelled there). "" where f is not overloaded.
std::string Writer::overloadPointer(const Decl& d, const SrcModel::Function& f, const std::string& cls) const
{
	bool overloaded = m_model.functionsNamed(f.name).size() > 1;
	std::vector<std::string> bases;
	if (const SrcModel::Record* r = m_model.record(f.cls))
		for (const SrcModel::Base& b : r->bases)
			bases.push_back(b.record);
	for (size_t k = 0; k < bases.size() && !overloaded; ++k)
	{
		for (const SrcModel::Function* other : m_model.functionsNamed(bases[k] + "::" + lastComponent(f.name)))
			overloaded = overloaded || other->type != f.type;
		if (const SrcModel::Record* r = m_model.record(bases[k]))
			for (const SrcModel::Base& b : r->bases)
				bases.push_back(b.record);
	}
	if (!overloaded)
		return "";
	std::optional<SrcTypes::Function> fn = SrcTypes::function(f.type);
	if (!fn)
		throw Skip{"no function type of " + f.name};
	std::string pointer;
	if (!f.cls.empty() && !f.isStatic)
	{
		// a member pointer, as src's FNVD_REPLACES_OVERLOAD spell them: a thiscall member names no convention (a
		// C-variadic member's is cdecl, C++'s default for it)
		if (fn->cc != (fn->variadic ? "cdecl" : "thiscall"))
			throw Skip{"an overloaded member of convention " + fn->cc};
		std::string ret = m_forwarders.spelled(d, fn->ret);
		if (ret.find_first_of("([") != std::string::npos)
			throw Skip{"a member pointer's result src cannot spell in front: " + ret};
		std::string params;
		for (const std::string& p : fn->params)
			params += (params.empty() ? "" : ", ") + m_forwarders.spelled(d, p);
		if (fn->variadic)
			params += params.empty() ? "..." : ", ...";
		pointer = ret + " (" + cls + "::*)(" + params + ")" + (fn->isConst ? " const" : "");
	}
	else
		pointer = m_forwarders.spelled(d, "ptr(" + f.type + ")");
	return fixed64(pointer);
}

// An addressed KEEP body's assertion (Reconstruction.hpp's FNVD_ADDRESS): after its definition at namespace scope and
// the comment lines right after it (definitionComments), or, for a definition inside its class, after the outermost
// class it is in and the FNVD_LAYOUT / FNVD_ADDRESS lines right after that; the function spelled without the
// namespaces open there, a class with template arguments through a `using` alias (classAt), and its pointer type
// after it where src declares the name more than once (overloadPointer).
void Writer::addressMarker(const Decl& d)
{
	const SrcModel::Function* f = m_model.function(d.mangled);
	const SrcModel::Definition* def = m_model.definition(d.mangled);
	if (!f || !def)
		throw Skip{"no src definition of " + d.mangled};
	const std::vector<std::string>& lines = m_edits.lines(def->file);
	int64_t size = static_cast<int64_t>(lines.size());
	std::string word = d.member.starts_with("operator") ? "operator" : d.member;
	if (def->line < 1 || def->endLine < def->line || def->endLine > size ||
		lines[def->line - 1].find(word) == std::string::npos || lines[def->endLine - 1].find('}') == std::string::npos)
		throw Skip{"the model's lines of " + f->name + " are not its file's (a model older than src)"};
	const SrcModel::Record* outer = nullptr;
	for (std::string cls = f->cls; !cls.empty(); cls = scopeOf(cls))
		if (const SrcModel::Record* r = m_model.record(cls);
			r && r->file == def->file && r->line <= def->line && def->line <= r->endLine)
			outer = r;
	int64_t after = definitionComments(lines, def->line, def->endLine).trailingEnd;
	if (outer)
	{
		after = outer->endLine;
		while (after < size && (lines[after].starts_with("FNVD_LAYOUT(") || lines[after].starts_with("FNVD_ADDRESS(")))
			++after;
	}
	if (braceDepth(lines, after + 1) != static_cast<int>(namespacesAt(lines, after + 1).size()))
		throw Skip{"the definition is not at namespace scope"};
	std::string scope = scopeAt(lines, after + 1);
	// an instance's class as srcdb spells it without its default arguments (`msvc9::vector<X>`)
	const SrcModel::Record* owner = m_model.record(f->cls);
	std::string name = fixed64(owner && !owner->spelled.empty() ? owner->spelled + "::" + lastComponent(f->name)
																	: f->name);
	if (!scope.empty() && name.starts_with(scope + "::"))
		name = name.substr(scope.size() + 2);
	std::string cls = scopeOf(name), last = lastComponent(name), alias;
	if (last.find(',') != std::string::npos)
		throw Skip{"template arguments in the function's name: " + last};
	if (!cls.empty())
		name = classAt(cls, "", lines, alias) + "::" + last;
	std::string pointer = overloadPointer(d, *f, scopeOf(name));
	std::string text = "FNVD_ADDRESS(" + address(d.ea) + ", " + name + (pointer.empty() ? "" : ", " + pointer) + ");";
	if (m_edits.has(def->file, text))
		throw Skip{"src has the assertion already"};
	// an alias src (or this run) has already, matched by its first line
	std::string block = alias.empty() || m_edits.has(def->file, alias.substr(0, alias.find('\n'))) ? "" : alias + "\n";
	m_edits.insert(def->file, after + 1, block + wrapped(text, "", indentUnit(lines, 1, size)) + "\n");
}

void Writer::add(const Decl& d)
{
	if (d.outcome == "addressed" || d.outcome == "folded")
	{
		// an addressed KEEP body's assertion; a fold whose owner src specializes already: its copies' comments only
		if (d.outcome == "addressed")
			addressMarker(d);
		for (const LineInsert& copy : d.foldCopies)
			m_edits.insert(copy.file, copy.line, copy.text);
		return;
	}
	// every type the texts name is one src has, and a class's new member names none of its header's nested types
	// defined after the class
	bool inClass = d.outcome == "declared" && d.owner;
	std::string header = inClass ? d.header : "";
	int64_t line = inClass ? d.owner->endLine : 0;
	checkDeclared(d.ret, header, line);
	for (const Param& p : d.params)
		checkDeclared(p.key, header, line);
	if (d.owner)
		checkDeclared("record:" + d.owner->name);
	// a new declaration's record parameter by value is copied by the forwarder: only a byte copy is the exe's (a
	// copy constructor and destructor src would run are none of its)
	for (const Param& p : d.params)
		if (SrcTypes::Parts pp = m_types.parts(m_types.canonical(p.key)); !d.exact && pp.kind == "record")
			if (const SrcModel::Record* r = m_model.record(pp.name); !r || r->vfptr || !r->trivialCopy)
				throw Skip{"record with a non-trivial copy passed by value: " + pp.name};
	// the texts first: a type src cannot spell skips the function before any edit (the definition is written last)
	std::string declaration = cppSignature(d, false);
	cppSignature(d, true);
	m_forwarders.statements(d);
	// a specialization that never returns: C++ wants `[[noreturn]]` on the function's first declaration, which is
	// the template's declaration of the member (string::_Xran's): it goes there where that lacks it
	std::string noreturnFile;
	int64_t noreturnLine = 0;
	if (d.outcome == "specialized" && d.noreturn && d.templateDeclaration.empty())
	{
		const SrcModel::Function* f = m_model.function(d.mangled);
		if (!f || f->file.rfind("src/", 0) != 0)
			throw Skip{"noreturn: no template declaration of the member in src"};
		if (!f->noreturn)
		{
			const std::vector<std::string>& lines = m_edits.lines(f->file);
			if (f->line < 1 || f->line > static_cast<int64_t>(lines.size()))
				throw Skip{"the model's lines of " + f->name + " are not its file's (a model older than src)"};
			noreturnLine = declarationFirst(lines, f->line);
			while (noreturnLine < f->line && trimmedText(lines[noreturnLine - 1]).starts_with("template"))
				++noreturnLine;
			if (lines[noreturnLine - 1].find("[[noreturn]]") == std::string::npos)
				noreturnFile = f->file;
		}
	}
	// a marker that is a comment's further lines (declare --hk-symbols' signature line after no name marker) goes
	// on the next line
	std::string comment = "// Not reconstructed yet." +
						  (d.marker.empty() || d.marker[0] == '\n' ? d.marker : " " + d.marker);
	// a fold's copy: the porting rules' comment, which srcdb's foldRef reads in the run above the declaration
	if (!d.fold.empty())
		comment += "\n" + wrappedComment(d.fold, "");
	// a new member goes in on the lines before its class's closing brace (a structor a template gets: its template's)
	auto checkClosing = [&](const std::string& header) {
		const std::vector<std::string>& lines = m_edits.lines(header);
		int64_t end = d.owner->endLine;
		size_t text = end >= 1 && end <= static_cast<int64_t>(lines.size()) ? lines[end - 1].find_first_not_of(" \t")
																			 : std::string::npos;
		if (text == std::string::npos || lines[end - 1][text] != '}')
			throw Skip{"the class's closing brace does not start its line"};
	};
	if (d.outcome == "declared" && d.owner)
		checkClosing(d.header);
	if (!d.templateDeclaration.empty())
		checkClosing(d.owner->file);
	// what the header needs, before any edit (an include cycle skips the function); a specialization is declared at
	// its home (SpecializationHomes), unless src declares it there already
	HeaderNeeds needs;
	bool declareSpecialization = d.outcome == "specialized" && !specializationDeclared(d);
	if (d.outcome == "declared")
		needs = headerNeeds(d, d.header, d.owner ? d.owner->line : int64_t(m_edits.lines(d.header).size()) + 1);
	else if (declareSpecialization)
		needs = headerNeeds(d, d.header, d.after + 1);

	// a structor the template gets for an instance's specialization: once per template and parameter types (the
	// names aside), in the injected class name
	if (!d.templateDeclaration.empty())
	{
		auto generic = [&](Decl g) {
			g.outcome = "declared";
			std::string text = cppSignature(g, false) + ";";
			std::string owner = fixed64(d.owner->name);
			for (size_t at = text.find(owner); at != std::string::npos; at = text.find(owner, at))
			{
				text.replace(at, owner.size(), d.templateDeclaration);
				at += d.templateDeclaration.size();
			}
			return text;
		};
		std::string text = generic(d);
		Decl unnamed = d;
		for (Param& p : unnamed.params)
			p.name.clear();
		std::string key = "template " + d.owner->pattern;
		if (m_templateDeclared.insert({d.owner->file, generic(unnamed)}).second)
		{
			m_blocks[{d.owner->file, key}].push_back("// Not reconstructed yet.\n" + text);
			m_owners[key] = d.owner;
		}
	}

	if (d.outcome == "declared" && d.owner)
	{
		std::string& block = m_blocks[{d.header, d.owner->name}].emplace_back();
		block = comment + "\n" + declaration + ";";
		m_owners[d.owner->name] = d.owner;
		addHeaderNeeds(d.header, needs);
	}
	else if (d.outcome == "declared")
	{
		const std::string& header = d.header;
		if (!std::filesystem::exists(header) && m_created.insert(header).second)
			m_edits.insert(header, 1, "#pragma once\n\n#include \"Reconstruction.hpp\"");
		// a header this run creates has no lines to put a block of forward declarations after (flush() would add it
		// after this run's inserts, below the declarations): each goes right above the first declaration needing it
		std::string forwards;
		if (m_created.count(header))
		{
			for (const auto& [name, line] : needs.forwards)
				if (m_forwarded.insert({header, name}).second)
					forwards += line + "\n";
			if (!forwards.empty())
				forwards += "\n";
			needs.forwards.clear();
		}
		addHeaderNeeds(header, needs);
		std::string text = "\n" + forwards + comment + "\n" + wrapped(declaration + ";", "", "\t");
		if (!d.scope.empty())
			text = "\n" + forwards + "namespace " + d.scope + " {\n" + comment + "\n" +
				   wrapped(declaration + ";", "", "\t") + "\n}  // namespace " + d.scope;
		m_edits.insert(header, static_cast<int64_t>(m_edits.lines(header).size()) + 1, text);
	}
	else if (declareSpecialization)
	{
		addHeaderNeeds(d.header, needs);
		insertSpecialization(d, comment);
	}
	if (!noreturnFile.empty() && m_noreturnMarked.insert({noreturnFile, noreturnLine}).second)
	{
		std::string line = m_edits.lines(noreturnFile)[noreturnLine - 1];
		size_t text = line.find_first_not_of(" \t");
		m_edits.replace(noreturnFile, {noreturnLine, noreturnLine}, line.insert(text, "[[noreturn]] "));
	}

	if (!std::filesystem::exists(d.source) && m_created.insert(d.source).second && headerFile(d.header))
		ensureInclude(m_edits, d.source, withoutSrc(d.header));
	sourceNeeds(d);
	std::string unit = std::filesystem::exists(d.source)
						   ? indentUnit(m_edits.lines(d.source), 1, static_cast<int64_t>(m_edits.lines(d.source).size()))
						   : "\t";
	std::string body =
		"\n" + (d.fold.empty() ? std::string() : wrappedComment(d.fold, "") + "\n") +
		m_forwarders.definition(d, d.outcome == "specialized", unit, true,
								[&](const std::string& key) { return homeKey(d, key); });
	for (const LineInsert& copy : d.foldCopies)
		m_edits.insert(copy.file, copy.line, copy.text);
	m_edits.insert(d.source, static_cast<int64_t>(m_edits.lines(d.source).size()) + 1, body);
}

void Writer::inPlace(const Decl& d, int64_t before)
{
	checkDeclared(d.ret, d.header, before);
	for (const Param& p : d.params)
		checkDeclared(p.key, d.header, before);
	if (d.owner)
		checkDeclared("record:" + d.owner->name);
	HeaderNeeds needs = headerNeeds(d, d.header, before);
	cppSignature(d, false);
	cppSignature(d, true);
	m_forwarders.statements(d);
	addHeaderNeeds(d.header, needs);
	if (!d.source.empty())
		sourceNeeds(d);
}

void Writer::flush()
{
	for (const auto& [where, declarations] : m_blocks)
	{
		const SrcModel::Record* owner = m_owners[where.second];
		const std::vector<std::string>& lines = m_edits.lines(where.first);
		// the members' indentation: the class body's (a nested class's is deeper than its closing brace's)
		const std::string& closing = lines[owner->endLine - 1];
		std::string outer = closing.substr(0, closing.find_first_not_of(" \t"));
		std::string inner;
		for (int64_t k = owner->line + 1; k < owner->endLine && inner.empty(); ++k)
		{
			const std::string& line = lines[k - 1];
			size_t text = line.find_first_not_of(" \t");
			if (text != std::string::npos && text > outer.size() && line[text] != '#')
				inner = line.substr(0, text);
		}
		std::string unit = indentUnit(lines, owner->line + 1, owner->endLine - 1);
		if (inner.empty())
			inner = outer + unit;
		std::string text = outer + "public:";
		for (const std::string& declaration : declarations)
			text += "\n" + wrapped(declaration, inner, unit);
		m_edits.insert(where.first, owner->endLine, text);
	}
	m_blocks.clear();
	for (const auto& [header, lines] : m_forwards)
	{
		std::string text;
		for (const std::string& line : lines)
			text += (text.empty() ? "" : "\n") + line;
		m_edits.insert(header, afterIncludes(m_edits.lines(header)), text);
	}
	m_forwards.clear();
	flushGlobals();
}

// --- --globals ---------------------------------------------------------------------------------------------------------

// Every record, enum and typedef name a type key names (a template instance's arguments, a nested class's outer
// classes included).
void Writer::typeNames(const std::string& key, std::set<std::string>& names) const
{
	if (std::optional<SrcTypes::Function> f = SrcTypes::function(key))
	{
		typeNames(f->ret, names);
		for (const std::string& param : f->params)
			typeNames(param, names);
		return;
	}
	SrcTypes::Parts p = m_types.parts(key);
	if (p.kind == "enum" || p.kind == "typedef")
		names.insert(p.name);
	if (p.kind != "record")
	{
		if (!p.of.empty())
			typeNames(p.of, names);
		return;
	}
	auto word = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == ':'; };
	for (size_t k = 0; k < p.name.size();)
	{
		if (!word(p.name[k]) || p.name[k] == ':')
		{
			++k;
			continue;
		}
		size_t end = k;
		while (end < p.name.size() && word(p.name[end]))
			++end;
		std::string name = p.name.substr(k, end - k);
		k = end;
		if (m_model.record(name) || m_model.enumeration(name) || m_model.typedefNamed(name))
			names.insert(name);
	}
}

void Writer::checkLines(const SrcModel::Record& r)
{
	const std::vector<std::string>& lines = m_edits.lines(r.file);
	if (r.line < 1 || r.endLine < r.line || r.endLine > static_cast<int64_t>(lines.size()))
		throw Skip{"no definition of " + r.name + " in " + r.file};
	std::string name = r.name.substr(0, r.name.find('<'));
	name = lastComponent(name);
	if (lines[r.line - 1].find(name) == std::string::npos || !trimmedText(lines[r.endLine - 1]).starts_with("}"))
		throw Skip{"the model's lines of " + r.name + " are not " + r.file + "'s: the model is older than src"};
}

std::string Writer::declareGlobal(const GlobalDecl& g)
{
	static const std::set<std::string> markers = {"// IDA name; not in the PDB.",
												  "// Not named in the PDB or IDA; name chosen here.",
												  "// Not named in the PDB or IDA; names chosen here."};
	std::string comment = g.marker.empty() ? "" : g.marker + "\n";
	// a declaration of the name the file's own text has already in the scope it goes in, which the model does not
	// know (a model older than src)
	auto declaredHere = [&](const std::string& file) {
		return Skip{file + " declares " + g.name + " already: the model is older than src"};
	};
	if (g.kind == "vftable")
	{
		// a nested class's vtable goes at namespace scope: after its outermost enclosing class in the same file
		const SrcModel::Record* owner = g.owner;
		for (size_t colon = g.owner->name.find("::"); colon != std::string::npos;
			 colon = g.owner->name.find("::", colon + 2))
			if (const SrcModel::Record* outer = m_model.record(g.owner->name.substr(0, colon));
				outer && outer->file == g.owner->file && outer->endLine >= g.owner->endLine)
			{
				owner = outer;
				break;
			}
		checkLines(*owner);
		for (const std::string& line : m_edits.lines(owner->file))
			if (line.starts_with("FNVD_VFTABLE(" + g.name + ","))
				throw declaredHere(owner->file);
		const std::vector<std::string>& lines = m_edits.lines(owner->file);
		std::string text = "FNVD_VFTABLE(" + g.name + ", " + address(g.ea) + ");";
		if (!g.moveFrom.empty())
		{
			// exactly the declaration's line moves, at the .cpp's namespace scope (`FNVD_VFTABLE(name, addr);` on a
			// line of its own): the model's line when it reads so, else the file's one such line
			const std::vector<std::string>& from = m_edits.lines(g.moveFrom);
			auto declares = [&](int64_t k) {
				const std::string& line = from[k - 1];
				return line.starts_with("FNVD_VFTABLE(" + g.name + ",") && trimmedText(line).ends_with(");") &&
					   braceDepth(from, k) == 0;
			};
			int64_t moved = 0;
			if (g.moveLine >= 1 && g.moveLine <= static_cast<int64_t>(from.size()) && declares(g.moveLine))
				moved = g.moveLine;
			else
				for (int64_t k = 1; k <= static_cast<int64_t>(from.size()); ++k)
					if (declares(k))
					{
						if (moved)
							throw Skip{g.moveFrom + " declares " + g.name + " on several lines"};
						moved = k;
					}
			if (!moved)
				throw Skip{"no line of " + g.moveFrom + " is " + g.name + "'s own `FNVD_VFTABLE(...);` to move"};
			// its marker comment moves with it: the marker above the run of FNVD_VFTABLE lines it heads goes from
			// the .cpp once every line of the run has moved (flush)
			int64_t first = moved;
			while (first > 1 && from[first - 2].starts_with("FNVD_VFTABLE("))
				--first;
			if (first > 1 && markers.count(trimmedText(from[first - 2])))
			{
				comment = trimmedText(from[first - 2]) + "\n";
				int64_t last = moved;
				while (last < static_cast<int64_t>(from.size()) && from[last].starts_with("FNVD_VFTABLE("))
					++last;
				m_runs[{g.moveFrom, first - 1}].size = static_cast<int>(last - first + 1);
				++m_runs[{g.moveFrom, first - 1}].moved;
			}
			// the .cpp's own uses see the header's
			if (!reaches(g.moveFrom, owner->file))
				ensureInclude(m_edits, g.moveFrom, withoutSrc(owner->file));
			m_edits.replace(g.moveFrom, {moved, moved}, "");
		}
		// after the class's closing brace and the vtables declared after it, their marker comments included; an
		// instance's after the last vtable of its template's instances at namespace scope below the template, where
		// the instances' vtables are
		const int64_t size = static_cast<int64_t>(lines.size());
		auto vtableLine = [&](int64_t k) {
			return lines[k - 1].starts_with("FNVD_VFTABLE(") ||
				   (markers.count(trimmedText(lines[k - 1])) && k < size && lines[k].starts_with("FNVD_VFTABLE("));
		};
		int64_t after = owner->endLine;
		while (after < size && vtableLine(after + 1))
			++after;
		if (!owner->pattern.empty())
		{
			std::string instances = "FNVD_VFTABLE(" + identifierOf(owner->pattern) + "_";
			for (int64_t k = after + 1; k <= size; ++k)
				if (lines[k - 1].starts_with(instances) && braceDepth(lines, k) == 0)
					after = k;
		}
		m_edits.insert(owner->file, after + 1, comment + text);
		return text;
	}
	// an original function a .cpp declares at namespace scope moves: exactly its `FNVD_ORIGINAL_FUNCTION(...);` line
	// (the model's line when it reads so, else the file's one such line), in its run of such lines (and their
	// markers)
	int64_t moved = 0;
	struct
	{
		int64_t first = 0, second = 0;
		int size = 0;
	} movedRun;
	if (!g.moveFrom.empty())
	{
		const std::vector<std::string>& from = m_edits.lines(g.moveFrom);
		const int64_t count = static_cast<int64_t>(from.size());
		auto constant = [&](int64_t k) {
			return from[k - 1].starts_with("FNVD_ORIGINAL_FUNCTION(") && trimmedText(from[k - 1]).ends_with(");") &&
				   braceDepth(from, k) == 0;
		};
		auto declares = [&](int64_t k) {
			return constant(k) && from[k - 1].find(", " + g.name + ", 0x") != std::string::npos;
		};
		if (g.moveLine >= 1 && g.moveLine <= count && declares(g.moveLine))
			moved = g.moveLine;
		else
			for (int64_t k = 1; k <= count; ++k)
				if (declares(k))
				{
					if (moved)
						throw Skip{g.moveFrom + " declares " + g.name + " on several lines"};
					moved = k;
				}
		if (!moved)
			throw Skip{"no line of " + g.moveFrom + " is " + g.name + "'s own `FNVD_ORIGINAL_FUNCTION(...);` to move"};
		auto inRun = [&](int64_t k) {
			return k >= 1 && k <= count &&
				   (constant(k) || (markers.count(trimmedText(from[k - 1])) && k < count && constant(k + 1)));
		};
		movedRun.first = movedRun.second = moved;
		while (inRun(movedRun.first - 1))
			--movedRun.first;
		while (inRun(movedRun.second + 1))
			++movedRun.second;
		for (int64_t k = movedRun.first; k <= movedRun.second; ++k)
			movedRun.size += constant(k);
		if (moved > 1 && markers.count(trimmedText(from[moved - 2])))
			comment = trimmedText(from[moved - 2]) + "\n";
	}
	std::optional<std::string> type = m_types.spell(m_forwarders.fixedKey(g.key));
	if (!type)
		throw Skip{"type src cannot spell: " + g.key};
	std::string spelledType = fixed64(*type);
	std::string alias;
	if (spelledType.find_first_of(",()[") != std::string::npos)
	{
		alias = "using " + g.name + "_t = " + spelledType + ";\n";
		spelledType = g.name + "_t";
	}
	Decl d;
	d.ea = g.ea;
	d.ret = m_forwarders.fixedKey(g.key);
	bool member = g.kind == "static_member";
	std::string header = member ? g.owner->file : g.header;
	if (member)
		checkLines(*g.owner);
	checkDeclared(d.ret, member ? header : "", member ? g.owner->endLine : 0);
	const std::vector<std::string>& lines = m_edits.lines(header);
	int64_t size = static_cast<int64_t>(lines.size());
	bool created = !std::filesystem::exists(header);
	// the core type past cv, arrays and pointers: an array of unknown bound has no standalone storage (fnvd::Immortal
	// of it is no complete object)
	for (SrcTypes::Parts p = m_types.parts(m_types.canonical(d.ret)); !p.of.empty();
		 p = m_types.parts(m_types.canonical(p.of)))
	{
		if (p.kind == "array" && p.count < 0)
			throw Skip{"an array of unknown bound: its standalone storage needs the bound"};
		if (p.kind != "const" && p.kind != "volatile" && p.kind != "array" && p.kind != "ptr")
			break;
	}
	// an object by value: FNVD_GLOBAL's standalone form (fnvd::Immortal) holds it, so its class is complete (an
	// array's element too: storage() reads only the model's own keys)
	SrcTypes::Parts bare = m_types.parts(m_types.canonical(m_model.storage(d.ret)));
	while ((bare.kind == "const" || bare.kind == "volatile" || bare.kind == "array") && !bare.of.empty())
		bare = m_types.parts(m_types.canonical(bare.of));
	if (member && bare.kind == "record" && bare.name == g.owner->name)
		throw Skip{"a static member of its own class by value"};
	// what of the header must come above the declaration: a record it holds by value and a nested record it names
	// (no forward declaration names either) the header defines, an enum or typedef it declares; one nested in a class
	// of the header, for a declaration at namespace scope, that outermost class
	auto below = [&](const std::string& name, int64_t line) {
		if (!member)
			for (size_t colon = name.find("::"); colon != std::string::npos; colon = name.find("::", colon + 2))
				if (const SrcModel::Record* outer = m_model.record(name.substr(0, colon));
					outer && outer->file == header && outer->line <= line && outer->endLine >= line)
					return outer->endLine;
		return line;
	};
	int64_t definedAbove = 0;
	std::set<std::string> names;
	typeNames(d.ret, names);
	for (const std::string& name : names)
	{
		if (member && name == g.owner->name)
			continue;
		if (const SrcModel::Record* r = m_model.record(name))
		{
			if (r->file == header && r->endLine > 0 &&
				((bare.kind == "record" && bare.name == name) || name.find("::") != std::string::npos))
				definedAbove = std::max(definedAbove, below(name, r->endLine));
		}
		else if (const SrcModel::Enum* e = m_model.enumeration(name); e && e->file == header)
			definedAbove = std::max(definedAbove, below(name, e->line));
		else if (const SrcModel::Typedef* t = m_model.typedefNamed(name); t && t->file == header)
			throw Skip{"its type " + name + " is a typedef of " + header + " (the model has no line for it)"};
	}
	// where it goes. A static member: after the class's last static member (below a nested class it needs, and not
	// in a nested class), else the class body's last line under public:. Any other: after the header's last
	// namespace-scope FNVD_GLOBAL, at least after the header's includes and forward declarations (its preamble), and
	// below a record it needs defined
	int64_t at = 0, preamble = created ? 1 : afterPreamble(lines);
	if (member)
	{
		if (definedAbove > g.owner->endLine)
			throw Skip{"its type is defined below " + g.owner->name + " in " + header};
		const auto& records = m_model.records();
		std::vector<std::pair<int64_t, int64_t>> nested;
		for (auto r = records.lower_bound(g.owner->name + "::");
			 r != records.end() && r->first.starts_with(g.owner->name + "::"); ++r)
			if (r->second.file == header && r->second.line > g.owner->line && r->second.endLine < g.owner->endLine)
				nested.push_back({r->second.line, r->second.endLine});
		auto inNested = [&](int64_t k) {
			return std::any_of(nested.begin(), nested.end(),
							   [&](const auto& n) { return n.first <= k && k <= n.second; });
		};
		for (int64_t k = g.owner->line + 1; k < g.owner->endLine; ++k)
			if (std::string text = trimmedText(lines[k - 1]);
				text.starts_with("FNVD_STATIC_MEMBER(") && text.ends_with(");") && !inNested(k))
			{
				if (text.find(", " + g.name + ", 0x") != std::string::npos)
					throw declaredHere(header);
				at = k + 1;
			}
		if (at && at <= definedAbove)
			at = 0;
	}
	else
	{
		for (int64_t k = 1; k <= size; ++k)
			if ((lines[k - 1].starts_with("FNVD_GLOBAL(") || lines[k - 1].starts_with("FNVD_ORIGINAL_FUNCTION(")) &&
				trimmedText(lines[k - 1]).ends_with(");") && braceDepth(lines, k) == 0)
			{
				if (lines[k - 1].find(", " + g.name + ", 0x") != std::string::npos)
					throw declaredHere(header);
				at = k + 1;
			}
		at = std::max({at, preamble, definedAbove + 1});
		if (braceDepth(lines, at) != 0)
			throw Skip{"its place in " + header + " (line " + std::to_string(at) + ") is not at namespace scope"};
	}
	// what the header needs above it: includes join the header's include run, forward declarations its preamble
	int64_t before = member ? g.owner->line : at;
	HeaderNeeds needs = headerNeeds(d, header, before);
	if (bare.kind == "record")
	{
		std::set<std::string> headers;
		m_types.headers("record:" + bare.name, headers);
		// its implicit structors, which the standalone form instantiates, destroy each NiPointer member it holds, its
		// bases' and its by-value members' included: NiDecRefCount needs each one's class complete
		std::set<std::string> seen;
		std::function<void(const std::string&)> walk = [&](const std::string& name) {
			const SrcModel::Record* r = m_model.record(name);
			if (!r || !seen.insert(name).second)
				return;
			for (const SrcModel::Base& base : r->bases)
				walk(base.record);
			for (const SrcModel::Field& field : r->fields)
			{
				SrcTypes::Parts p = m_types.parts(m_types.canonical(field.type));
				while (p.kind == "array" && !p.of.empty())
					p = m_types.parts(m_types.canonical(p.of));
				if (p.kind != "record")
					continue;
				if (!p.name.starts_with("NiPointer<"))
					walk(p.name);
				else if (std::string pointee = trimmedText(p.name.substr(10, p.name.rfind('>') - 10));
						 m_model.record(pointee))
					m_types.headers("record:" + pointee, headers);
			}
		};
		walk(bare.name);
		for (const std::string& h : headers)
		{
			if ("src/" + h == header || m_edits.has(header, includeLine(h)))
				continue;
			if (!h.starts_with('<') && reaches("src/" + h, header))
				throw Skip{"would include " + h + ", which includes " + withoutSrc(header)};
			needs.includes.push_back(h);
		}
		std::erase_if(needs.forwards, [&](const auto& f) { return f.first == bare.name; });
	}
	// an include the header has already must be above it too (headerNeeds takes any line of the header's)
	{
		std::set<std::string> records, headers;
		typeNeeds(d.ret, records, headers);
		if (bare.kind == "record")
			m_types.headers("record:" + bare.name, headers);
		for (const std::string& name : records)
			if (const SrcModel::Record* r = m_model.record(name); r && r->file != header)
				headers.insert(withoutSrc(r->file));
		for (const std::string& h : headers)
			if (int64_t k = lineReading(lines, includeLine(h)); k >= before)
				throw Skip{header + " includes " + h + " below the declaration's place (line " + std::to_string(k) +
						   ")"};
	}
	if (!needs.includes.empty() && !created && afterIncludes(lines) > before)
		throw Skip{"the includes it needs would land below its place in " + header};
	if (created && m_created.insert(header).second)
		m_edits.insert(header, 1, "#pragma once\n\n#include \"Reconstruction.hpp\"");
	for (const auto& [name, line] : needs.forwards)
		if (m_forwarded.insert({header, name}).second)
			m_globalForwards[header].push_back(line);
	needs.forwards.clear();
	addHeaderNeeds(header, needs);
	std::string macro = member								 ? "FNVD_STATIC_MEMBER("
						: g.kind == "original_function" ? "FNVD_ORIGINAL_FUNCTION("
														: "FNVD_GLOBAL(";
	std::string text = macro + spelledType + ", " + g.name + ", " + address(g.ea) + ");";
	if (member && !at)
	{
		m_blocks[{header, g.owner->name}].push_back(comment + alias + text);
		m_owners[g.owner->name] = g.owner;
	}
	else if (member)
	{
		const std::string& line = lines[at - 2];
		std::string indent = line.substr(0, line.find_first_not_of(" \t"));
		std::string block;
		std::istringstream parts(comment + alias + text);
		for (std::string part; std::getline(parts, part);)
			block += (block.empty() ? "" : "\n") + indent + part;
		m_edits.insert(header, at, block);
	}
	else
		m_globalBlocks[{header, at}].push_back(comment + alias + text);
	if (moved)
	{
		// the .cpp's line goes, with the marker right above it; the .cpp's uses see the header's
		const std::vector<std::string>& from = m_edits.lines(g.moveFrom);
		bool marked = moved > 1 && markers.count(trimmedText(from[moved - 2]));
		m_edits.replace(g.moveFrom, {marked ? moved - 1 : moved, moved}, "");
		if (!reaches(g.moveFrom, header))
			ensureInclude(m_edits, g.moveFrom, withoutSrc(header));
		// the comment heading the run of constants it is in goes with them once the whole run has moved (flush)
		ConstantRun& run = m_constantRuns[{g.moveFrom, movedRun.first}];
		run.last = movedRun.second;
		run.size = movedRun.size;
		run.block = {header, at};
		run.moved.push_back({moved, m_globalBlocks[{header, at}].size() - 1});
	}
	return text;
}

void Writer::flushGlobals()
{
	// a header's forward declarations end its preamble; its globals' blocks follow, each set off by blank lines
	auto blank = [](const std::vector<std::string>& lines, int64_t k) {
		return k < 1 || k > static_cast<int64_t>(lines.size()) || trimmedText(lines[k - 1]).empty();
	};
	auto preambleOf = [&](const std::string& header) {
		return std::filesystem::exists(header) ? afterPreamble(m_edits.lines(header)) : int64_t{1};
	};
	for (const auto& [header, forwards] : m_globalForwards)
	{
		const std::vector<std::string>& lines = m_edits.lines(header);
		int64_t at = preambleOf(header);
		std::string text;
		for (const std::string& line : forwards)
			text += (text.empty() ? "" : "\n") + line;
		// set off from an include above it (a created file's #include "Reconstruction.hpp" too)
		if (!std::filesystem::exists(header) || (at > 1 && trimmedText(lines[at - 2]).starts_with("#")))
			text = "\n" + text;
		if (!m_globalBlocks.count({header, at}) && !blank(lines, at))
			text += "\n";
		m_edits.insert(header, at, text);
	}
	// a run of a .cpp's constants that has moved whole keeps its order and takes the comment heading it (its `//` lines
	// right above it) along, above its first declaration; a blank line it leaves doubled goes
	for (auto& [where, run] : m_constantRuns)
	{
		const auto& [file, first] = where;
		if (static_cast<int>(run.moved.size()) != run.size)
			continue;
		const std::vector<std::string>& from = m_edits.lines(file);
		const int64_t count = static_cast<int64_t>(from.size());
		std::vector<std::string>& block = m_globalBlocks[run.block];
		std::vector<size_t> slots;
		for (const auto& [line, index] : run.moved)
			slots.push_back(index);
		std::ranges::sort(slots);
		std::ranges::sort(run.moved);
		std::vector<std::string> ordered;
		for (const auto& [line, index] : run.moved)
			ordered.push_back(block[index]);
		for (size_t k = 0; k < slots.size(); ++k)
			block[slots[k]] = ordered[k];
		int64_t top = first;
		while (top > 1 && trimmedText(from[top - 2]).starts_with("//"))
			--top;
		if (top < first)
		{
			std::string comment;
			for (int64_t k = top; k < first; ++k)
				comment += trimmedText(from[k - 1]) + "\n";
			m_edits.replace(file, {top, first - 1}, "");
			block[slots[0]] = comment + block[slots[0]];
		}
		auto blank = [&](int64_t k) { return k < 1 || k > count || trimmedText(from[k - 1]).empty(); };
		if (blank(top - 1) && run.last < count && blank(run.last + 1))
			m_edits.replace(file, {run.last + 1, run.last + 1}, "");
	}
	m_constantRuns.clear();
	for (const auto& [where, declarations] : m_globalBlocks)
	{
		const auto& [header, at] = where;
		const std::vector<std::string>& lines = m_edits.lines(header);
		std::string text;
		for (const std::string& declaration : declarations)
			text += (text.empty() ? "" : "\n") + declaration;
		// a block right after the forward declarations this run adds, or in a created file, is set off from them; one
		// after a namespace-scope FNVD_GLOBAL continues its run; any other is set off from a line of text above it
		bool afterForwards = m_globalForwards.count(header) && at == preambleOf(header);
		bool continues = !afterForwards && at > 1 && lines[at - 2].starts_with("FNVD_GLOBAL(");
		if (afterForwards || !std::filesystem::exists(header) || (!continues && !blank(lines, at - 1)))
			text = "\n" + text;
		if (!blank(lines, at))
			text += "\n";
		m_edits.insert(header, at, text);
	}
	// a marker comment heading a run of moved vtables goes once the whole run has moved
	for (const auto& [where, run] : m_runs)
		if (run.moved == run.size)
			m_edits.replace(where.first, {where.second, where.second}, "");
	m_globalForwards.clear();
	m_globalBlocks.clear();
	m_runs.clear();
}

// --- --place-specializations -----------------------------------------------------------------------------------------

// A src function's declaration as a Decl: src's types exactly, its parameter names, the convention src lowers it to;
// nullopt where the model lacks its type or class.
std::optional<Decl> modelDecl(const SrcModel& model, const SrcModel::Function& f, const std::string& outcome)
{
	const SrcModel::Type* type = model.type(f.type);
	if (!type || type->kind != "fn")
		return std::nullopt;
	Decl d;
	d.outcome = outcome;
	d.exact = true;
	d.mangled = f.mangled;
	d.owner = f.cls.empty() ? nullptr : model.record(f.cls);
	if (!f.cls.empty() && !d.owner)
		return std::nullopt;
	d.member = lastComponent(f.name);
	d.scope = d.owner ? "" : scopeOf(f.name);
	d.ret = type->ret;
	d.isConst = type->isConst;
	d.isStatic = f.isStatic;
	const SrcModel::Abi* abi = model.abi(f.mangled);
	d.cc = abi ? abi->cc : type->cc;
	// src's own names, which the moved body uses (reserved ones like `__that` included)
	for (size_t k = 0; k < type->params.size(); ++k)
		d.params.push_back(
			{type->params[k], k < f.params.size() && !f.params[k].empty() ? f.params[k] : "a" + std::to_string(k + 1)});
	return d;
}

std::string text(const std::vector<std::string>& lines, LineRange range)
{
	std::string out;
	for (int64_t number = range.first; number <= range.last; ++number)
		out += lines[number - 1] + "\n";
	return out;
}

bool blank(const std::string& line)
{
	return line.find_first_not_of(" \t") == std::string::npos;
}

// A range of lines to delete, with the blank line after it when one is before it too (the text stood on its own).
LineRange tidied(const std::vector<std::string>& lines, LineRange range)
{
	int64_t size = static_cast<int64_t>(lines.size());
	if (range.last < size && blank(lines[range.last]) && (range.first == 1 || blank(lines[range.first - 2])))
		++range.last;
	return range;
}

// The code of a definition's body for comparing two copies of one function (their heads name the same function,
// however spelled): from its first `{`, without comments and blanks.
std::string code(const std::string& definition)
{
	static const std::regex comment(R"(//[^\n]*)"), space(R"(\s+)");
	std::string text = std::regex_replace(definition, comment, "");
	size_t brace = text.find('{');
	return std::regex_replace(brace == std::string::npos ? text : text.substr(brace), space, "");
}

// The #if nesting at a line of a file (a header's include guard aside).
int conditionalDepth(const std::vector<std::string>& lines, int64_t line)
{
	static const std::regex directive(R"(^\s*#\s*(if|ifdef|ifndef|endif)\b\s*(\S*))");
	int depth = 0;
	bool guard = false;
	std::smatch m;
	for (int64_t number = 1; number < line && number <= static_cast<int64_t>(lines.size()); ++number)
	{
		if (!std::regex_search(lines[number - 1], m, directive))
			continue;
		std::string word = m[1].str();
		if (word == "endif")
			--depth;
		else
		{
			// the first directive `#ifndef X` followed by `#define X` guards the whole file
			if (depth == 0 && !guard && word == "ifndef" && number < static_cast<int64_t>(lines.size()) &&
				lines[number].find("#define " + m[2].str()) != std::string::npos)
			{
				guard = true;
				continue;
			}
			++depth;
		}
	}
	return depth;
}

// Puts every explicit specialization in its one place (porting.md "Bodies", SpecializationHomes) and every forwarder
// or emitted guarded block a header defines into a .cpp; writes DIR/place.tsv (mangled name, action, from, to).
class Placer
{
public:
	Placer(const SrcModel& model, FileEdits& edits, Writer& writer, SpecializationHomes& homes, IncludeGraph& includes,
		   const PdbSources& sources, std::ostream& table)
		: m_model(model), m_edits(edits), m_writer(writer), m_homes(homes), m_includes(includes), m_sources(sources),
		  m_table(table)
	{
		for (const auto& [ea, refs] : model.refs())
			for (const SrcModel::Ref& r : refs)
				if (!r.function.empty())
					m_refs[r.function].push_back(&r);
	}

	// every function srcdb has explicit specialization rows of
	void specializations();
	// every non-template function a header defines as a forwarder or an emitted guarded block
	void headerDefinitions();
	// a header left without an emitted body loses the block of what they needed
	void needsBlocks();

	std::map<std::string, int> counts;

private:
	// a definition's text and the lines it takes
	struct Moved
	{
		std::string file;
		LineRange range;   // its lines (its comment lines and an emitted block's guard included)
		std::string text;  // in a header: the definition as it goes to a .cpp (its comment lines, the code without
						   // `inline`); in a .cpp: its lines without `inline`
		std::string code;  // what two copies compare by: an emitted block's forwarder
		bool emitted = false;  // a header's emitted block, which its forwarder leaves
		bool guarded = false;  // an emitted block, in a header or a .cpp
	};

	void note(const std::string& name, const std::string& action, const std::string& from, const std::string& to)
	{
		m_table << name << '\t' << action << '\t' << from << '\t' << to << '\n';
		++counts[action.starts_with("skipped:") ? "skipped" : action];
	}
	// a definition row's text; the reason where it cannot move
	std::variant<Moved, std::string> definition(const std::string& file, int64_t first, int64_t last);
	// the line of the FNVD_REPLACES registration a .cpp has of an emitted header body; 0 for none
	std::vector<std::pair<std::string, int64_t>> registrations(const std::string& mangled);
	void remove(const std::string& file, LineRange range);
	void append(const std::string& source, const std::string& header, const std::string& text);
	// the namespaces open at a line, as `namespace a {` lines around a moved text
	std::string inNamespaces(const std::string& file, int64_t line, const std::string& text);
	// the text a header definition's lines become when its declaration takes its place: its comment lines, then its
	// head made a declaration
	std::string declarationInPlace(const Moved& m, const std::string& name);
	// a file that declared or defined a specialization which now goes elsewhere includes its home header, so each
	// translation unit that saw it before its uses sees it still; not where the home header includes the file
	void keepVisible(const std::string& file, const std::string& homeHeader);

	const SrcModel& m_model;
	FileEdits& m_edits;
	Writer& m_writer;
	SpecializationHomes& m_homes;
	IncludeGraph& m_includes;
	const PdbSources& m_sources;
	std::ostream& m_table;
	std::unordered_map<std::string, std::vector<const SrcModel::Ref*>> m_refs;  // by function (mangled)
	std::map<std::string, int> m_unguarded;  // header -> emitted blocks taken out of it
};

std::variant<Placer::Moved, std::string> Placer::definition(const std::string& file, int64_t first, int64_t last)
{
	const std::vector<std::string>& lines = m_edits.lines(file);
	if (first < 1 || last > static_cast<int64_t>(lines.size()) || first > last)
		return std::string("lines out of range");
	Moved m;
	m.file = file;
	std::optional<EmittedBlock> block = emittedBlock(lines, first);
	if (block && !block->forwarder)
		return std::string("emitted block without a forwarder");
	if (!headerFile(file))
	{
		// it stays where it is, `inline` taken off (an emitted block's two definitions alike)
		m.range = block ? block->whole : LineRange{first, last};
		m.text = withoutInline(text(lines, m.range));
		m.code = block ? text(lines, *block->forwarder) : m.text;
		m.guarded = block.has_value();
		return m;
	}
	if (block)
	{
		int64_t top = commentTop(lines, block->whole.first);
		m.range = {top, block->whole.last};
		m.code = text(lines, *block->forwarder);
		m.text = (top < block->whole.first ? text(lines, {top, block->whole.first - 1}) : "") + withoutInline(m.code);
		m.emitted = true;
		m.guarded = true;
	}
	else
	{
		if (conditionalDepth(lines, first) > 0)
			return std::string("inside a preprocessor conditional");
		int64_t top = commentTop(lines, first);
		m.range = {top, last};
		m.text = withoutInline(text(lines, m.range));
		m.code = m.text;
	}
	m.text = inNamespaces(file, m.range.first, m.text);
	return m;
}

std::string Placer::declarationInPlace(const Moved& m, const std::string& name)
{
	const std::vector<std::string>& lines = m_edits.lines(m.file);
	std::string out;
	for (int64_t k = m.range.first; k <= m.range.last; ++k)
	{
		size_t start = lines[k - 1].find_first_not_of(" \t");
		if (start == std::string::npos || lines[k - 1].compare(start, 2, "//") != 0)
			break;
		out += lines[k - 1] + "\n";
	}
	std::optional<InlineDefinition> split = inlineDefinition(m.code, name);
	std::string declaration = split ? split->declaration : "";
	// a definition's code with its comment lines in it keeps them in its declaration
	if (declaration.starts_with(out))
		out.clear();
	while (!declaration.empty() && declaration.back() == '\n')
		declaration.pop_back();
	return out + declaration;
}

void Placer::keepVisible(const std::string& file, const std::string& homeHeader)
{
	if (!headerFile(homeHeader) || file == homeHeader || m_includes.reaches(file, homeHeader) ||
		m_includes.reaches(homeHeader, file))
		return;
	ensureInclude(m_edits, file, withoutSrc(includedHeader(homeHeader)));
	m_includes.add(file, homeHeader);
}

std::string Placer::inNamespaces(const std::string& file, int64_t line, const std::string& body)
{
	std::string scope = scopeAt(m_edits.lines(file), line);
	if (scope.empty())
		return body;
	return "namespace " + scope + "\n{\n" + body + "}  // namespace " + scope + "\n";
}

std::vector<std::pair<std::string, int64_t>> Placer::registrations(const std::string& mangled)
{
	std::vector<std::pair<std::string, int64_t>> out;
	auto found = m_refs.find(mangled);
	if (found == m_refs.end())
		return out;
	for (const SrcModel::Ref* r : found->second)
		if ((r->kind == "body" || r->kind == "constructor" || r->kind == "destructor") && r->file.ends_with(".cpp"))
			out.emplace_back(r->file, r->line);
	return out;
}

void Placer::remove(const std::string& file, LineRange range)
{
	m_edits.replace(file, tidied(m_edits.lines(file), range), "");
}

void Placer::append(const std::string& source, const std::string& header, const std::string& body)
{
	ensureInclude(m_edits, source, withoutSrc(includedHeader(header)));
	const std::vector<std::string>& lines = m_edits.lines(source);
	std::string text = body;
	while (!text.empty() && text.back() == '\n')
		text.pop_back();
	m_edits.insert(source, static_cast<int64_t>(lines.size()) + 1, "\n" + text);
}

void Placer::specializations()
{
	for (const auto& [mangled, rows] : m_model.specializations())
	{
		const SrcModel::Function* f = m_model.function(mangled);
		if (!f)
		{
			note(mangled, "skipped:not in the model", rows.front().file, "");
			continue;
		}
		SpecializationHomes::Result found = m_homes.specializationHome(mangled);
		if (const std::string* why = std::get_if<std::string>(&found))
		{
			note(mangled, "skipped:" + *why, rows.front().file, "");
			continue;
		}
		const SpecializationHome home = std::get<SpecializationHome>(found);
		bool homeIsSource = !headerFile(home.header);
		// the definitions: one stays, in a .cpp (the first one without `inline`), else the first header's moves to
		// the home's .cpp; every other copy goes (each the same code as the one kept)
		std::vector<const SrcModel::Specialization*> sources, headers, declarations;
		for (const SrcModel::Specialization& s : rows)
			(s.definition ? (headerFile(s.file) ? headers : sources) : declarations).push_back(&s);
		std::vector<std::pair<const SrcModel::Specialization*, Moved>> texts;
		std::string why;
		for (const auto* list : {&sources, &headers})
			for (const SrcModel::Specialization* s : *list)
			{
				std::variant<Moved, std::string> m = definition(s->file, s->line, s->endLine);
				if (const std::string* reason = std::get_if<std::string>(&m))
					why = *reason + " at " + s->file + ":" + std::to_string(s->line);
				else
					texts.emplace_back(s, std::get<Moved>(m));
			}
		if (!why.empty())
		{
			note(mangled, "skipped:" + why, rows.front().file, "");
			continue;
		}
		size_t kept = 0;
		for (size_t k = 0; k < texts.size(); ++k)
			if (!headerFile(texts[k].first->file) && !texts[k].first->isInline)
			{
				kept = k;
				break;
			}
		for (size_t k = 0; k < texts.size() && why.empty(); ++k)
			if (code(texts[k].second.code) != code(texts[kept].second.code))
				why = "definitions differ: " + texts[k].first->file + ":" + std::to_string(texts[k].first->line);
		// an emitted header block's registration line in a .cpp goes with it
		std::vector<std::pair<std::string, int64_t>> registered;
		for (const auto& [s, m] : texts)
			if (m.emitted)
				for (const auto& [file, line] : registrations(mangled))
				{
					const std::vector<std::string>& lines = m_edits.lines(file);
					std::string_view l = line >= 1 && line <= static_cast<int64_t>(lines.size())
											 ? std::string_view(lines[line - 1])
											 : std::string_view();
					size_t start = l.find_first_not_of(" \t");
					if (start == std::string_view::npos || !l.substr(start).starts_with("FNVD_REPLACES") ||
						!l.ends_with(";") || emittedBlock(lines, line))
						why = "registration not one plain line at " + file + ":" + std::to_string(line);
					else
						registered.emplace_back(file, line);
				}
		if (!why.empty())
		{
			note(mangled, "skipped:" + why, rows.front().file, "");
			continue;
		}
		// the declaration at home: src's own there, else a new one (with the comment of one that goes)
		bool declaredAtHome = false;
		std::string comment;
		for (const SrcModel::Specialization* s : declarations)
			if (s->file == home.header)
				declaredAtHome = true;
			else if (comment.empty())
			{
				const std::vector<std::string>& lines = m_edits.lines(s->file);
				int64_t top = commentTop(lines, s->line);
				if (top < s->line)
					comment = text(lines, {top, s->line - 1});
			}
		if (homeIsSource && !texts.empty() && texts[kept].first->file == home.header)
			declaredAtHome = true;
		if (!comment.empty() && comment.back() == '\n')
			comment.pop_back();
		const std::string& body = texts.empty() ? std::string() : texts[kept].second.text;
		if (comment.empty() && body.find("fnvd::original::") != std::string::npos &&
			body.find("Not reconstructed yet.") != std::string::npos)
			comment = "// Not reconstructed yet.";
		// a header definition at the home whose declaration takes its place, when the Writer cannot declare it after
		// the template (a type there only forward declared): the definition compiled there, so its declaration does
		std::optional<size_t> inPlace;
		if (!declaredAtHome && !homeIsSource)
		{
			std::optional<Decl> d = modelDecl(m_model, *f, "specialized");
			if (!d)
			{
				note(mangled, "skipped:no type of it in the model", rows.front().file, "");
				continue;
			}
			d->header = home.header;
			d->after = home.after;
			d->homeIncludes = home.includes;
			try
			{
				m_writer.declareSpecialization(*d, comment);
				note(mangled, "declared", "", home.header + ":" + std::to_string(home.after));
			}
			catch (const Skip& s)
			{
				for (size_t k = 0; k < texts.size() && !inPlace; ++k)
					if (texts[k].first->file == home.header &&
						inlineDefinition(texts[k].second.code, lastComponent(f->name)))
						inPlace = k;
				if (!inPlace)
				{
					note(mangled, "skipped:" + s.reason, rows.front().file, home.header);
					continue;
				}
				note(mangled, "declared", "", home.header + ":" + std::to_string(texts[*inPlace].first->line));
			}
		}
		for (const SrcModel::Specialization* s : declarations)
			if (s->file != home.header)
			{
				const std::vector<std::string>& lines = m_edits.lines(s->file);
				remove(s->file, {commentTop(lines, s->line), s->endLine});
				keepVisible(s->file, home.header);
				note(mangled, "removed-declaration", s->file + ":" + std::to_string(s->line), home.header);
			}
		for (size_t k = 0; k < texts.size(); ++k)
		{
			const auto& [s, m] = texts[k];
			std::string from = s->file + ":" + std::to_string(s->line);
			if (k != kept)
			{
				if (inPlace == k)
					m_edits.replace(m.file, m.range, declarationInPlace(m, lastComponent(f->name)));
				else
					remove(m.file, m.range);
				if (headerFile(m.file))
					keepVisible(m.file, home.header);
				note(mangled, "removed-definition", from, texts[kept].first->file);
			}
			else if (headerFile(s->file))
			{
				if (inPlace == k)
					m_edits.replace(m.file, m.range, declarationInPlace(m, lastComponent(f->name)));
				else
					remove(m.file, m.range);
				keepVisible(m.file, home.header);
				append(home.source, s->file, m.text);
				if (headerFile(home.header))
					ensureInclude(m_edits, home.source, withoutSrc(includedHeader(home.header)));
				note(mangled, m.emitted ? "unguarded" : "moved-definition", from, home.source);
			}
			else if (s->isInline)
			{
				m_edits.replace(m.file, m.range, m.text);
				note(mangled, "uninlined", from, from);
			}
			if (m.emitted)
				++m_unguarded[m.file];
		}
		for (const auto& [file, line] : registered)
		{
			m_edits.replace(file, {line, line}, "");
			note(mangled, "removed-registration", file + ":" + std::to_string(line), "");
		}
	}
}

void Placer::headerDefinitions()
{
	// in file and line order, so each .cpp gets its definitions in the order the header had them
	std::vector<std::pair<std::string, const SrcModel::Definition*>> order;
	for (const auto& [mangled, def] : m_model.definitions())
		if (!def.instance && headerFile(def.file) && def.file.rfind("src/", 0) == 0 &&
			!m_model.specializations().count(mangled))
			order.emplace_back(mangled, &def);
	std::sort(order.begin(), order.end(), [](const auto& a, const auto& b) {
		return std::tie(a.second->file, a.second->line) < std::tie(b.second->file, b.second->line);
	});
	for (const auto& [mangled, pointer] : order)
	{
		const SrcModel::Definition& def = *pointer;
		const SrcModel::Function* f = m_model.function(mangled);
		if (!f)
			continue;
		const std::vector<std::string>& lines = m_edits.lines(def.file);
		if (def.line < 1 || def.endLine > static_cast<int64_t>(lines.size()))
			continue;
		bool inClass = f->file == def.file && f->line == def.line;
		int64_t first = declarationFirst(lines, def.line);
		std::optional<EmittedBlock> block = emittedBlock(lines, def.line);
		if (!block)
		{
			// a `// Not reconstructed yet.` forwarder: its own fnvd::original call, the words above it or in it
			auto refs = m_refs.find(mangled);
			bool forwards = refs != m_refs.end() &&
							std::any_of(refs->second.begin(), refs->second.end(), [&](const SrcModel::Ref* r) {
								return r->kind == "forwarder" && r->file == def.file && r->line >= first &&
									   r->line <= def.endLine;
							});
			std::string words = text(lines, {commentTop(lines, first), def.endLine});
			if (!inClass && f->file == def.file && f->line >= 1 && f->line <= static_cast<int64_t>(lines.size()))
			{
				int64_t declared = declarationFirst(lines, f->line);
				words += text(lines, {commentTop(lines, declared), declared});
			}
			if (!forwards || words.find("Not reconstructed yet.") == std::string::npos)
				continue;
		}
		std::string from = def.file + ":" + std::to_string(def.line);
		// a C++ structor out of its class (an explicit specialization srcdb has no rows of): its declaration takes its
		// place, the definition goes to a .cpp; one in its class stays
		if (!f->structor.empty() && (inClass || block))
		{
			note(mangled, "skipped:a C++ structor", from, "");
			continue;
		}
		if (!f->structor.empty())
		{
			std::variant<Moved, std::string> m = definition(def.file, first, def.endLine);
			const Moved* moved = std::get_if<Moved>(&m);
			std::string declaration = moved ? declarationInPlace(*moved, lastComponent(f->name)) : "";
			if (declaration.find(';') == std::string::npos)
			{
				note(mangled, "skipped:a C++ structor", from, "");
				continue;
			}
			std::string target = definitionSource(m_model, m_sources, *f, def.file);
			m_edits.replace(def.file, moved->range, declaration);
			append(target, def.file, moved->text);
			note(mangled, "moved-definition", from, target);
			continue;
		}
		if (f->cls.empty() && f->isStatic)
		{
			note(mangled, "skipped:internal linkage", from, "");
			continue;
		}
		std::string target = definitionSource(m_model, m_sources, *f, def.file);
		if (!inClass || block)
		{
			std::variant<Moved, std::string> m = definition(def.file, first, def.endLine);
			if (const std::string* why = std::get_if<std::string>(&m))
			{
				note(mangled, "skipped:" + *why, from, "");
				continue;
			}
			const Moved& moved = std::get<Moved>(m);
			std::vector<std::pair<std::string, int64_t>> registered = moved.emitted ? registrations(mangled)
																					 : decltype(registered){};
			remove(def.file, moved.range);
			append(target, def.file, moved.text);
			for (const auto& [file, line] : registered)
			{
				m_edits.replace(file, {line, line}, "");
				note(mangled, "removed-registration", file + ":" + std::to_string(line), "");
			}
			if (moved.emitted)
				++m_unguarded[def.file];
			note(mangled, moved.emitted ? "unguarded" : "moved-definition", from, target);
			continue;
		}
		// in the class: the declaration stays there, the definition goes out of line
		if (conditionalDepth(lines, first) > 0)
		{
			note(mangled, "skipped:inside a preprocessor conditional", from, "");
			continue;
		}
		std::optional<InlineDefinition> inlined =
			inlineDefinition(text(lines, {first, def.endLine}), lastComponent(f->name));
		std::optional<Decl> d = modelDecl(m_model, *f, "declared");
		if (!inlined || !d)
		{
			note(mangled, inlined ? "skipped:no type of it in the model" : "skipped:no inline body", from, "");
			continue;
		}
		std::string head;
		try
		{
			head = m_writer.definitionHead(*d);
		}
		catch (const Skip& s)
		{
			note(mangled, "skipped:" + s.reason, from, "");
			continue;
		}
		std::string declaration = inlined->declaration;
		while (!declaration.empty() && declaration.back() == '\n')
			declaration.pop_back();
		m_edits.replace(def.file, {first, def.endLine}, declaration);
		std::string body = wrapped(head, "", "\t") + "\n{\n\t// Not reconstructed yet.\n";
		for (const std::string& statement : inlined->statements)
			if (statement != "// Not reconstructed yet.")
				body += "\t" + statement + "\n";
		append(target, def.file, body + "}\n");
		note(mangled, "moved-definition", from, target);
	}
}

void Placer::needsBlocks()
{
	for (const auto& [header, removed] : m_unguarded)
	{
		const std::vector<std::string>& lines = m_edits.lines(header);
		int bodies = static_cast<int>(std::count(lines.begin(), lines.end(), kEmittedBodyIf));
		if (removed < bodies)
			continue;
		for (int64_t k = 1; k <= static_cast<int64_t>(lines.size()); ++k)
		{
			if (lines[k - 1] != kEmittedNeedsIf)
				continue;
			int64_t end = k;
			while (end <= static_cast<int64_t>(lines.size()) && lines[end - 1] != kEmittedEnd)
				++end;
			if (end > static_cast<int64_t>(lines.size()))
				break;
			LineRange range{k, end};
			if (k > 1 && blank(lines[k - 2]))
				range.first = k - 1;
			m_edits.replace(header, range, "");
			note("-", "removed-needs-block", header + ":" + std::to_string(k), "");
			k = end;
		}
	}
}

}  // namespace

struct DeclarationWriter::Impl
{
	Impl(const SrcModel& model, SrcTypes& types, FileEdits& edits) : writer(model, types, edits, includes) {}
	IncludeGraph includes;
	Writer writer;
};

DeclarationWriter::DeclarationWriter(const SrcModel& model, SrcTypes& types, FileEdits& edits)
	: m_impl(std::make_unique<Impl>(model, types, edits))
{
}

DeclarationWriter::~DeclarationWriter() = default;

void DeclarationWriter::add(const Decl& d)
{
	m_impl->writer.add(d);
}

void DeclarationWriter::inPlace(const Decl& d, int64_t before)
{
	m_impl->writer.inPlace(d, before);
}

void DeclarationWriter::flush()
{
	m_impl->writer.flush();
}

// --place-specializations: no database, no addresses; src's explicit specializations and header forwarders placed
int placeSpecializations(const SrcModel& model, SrcTypes& types, const PdbSources& sources,
						 const std::filesystem::path& out, bool apply)
{
	FileEdits edits;
	IncludeGraph includes;
	SpecializationHomes homes(model, includes);
	Writer writer(model, types, edits, includes);
	std::ofstream table(out / "place.tsv", std::ios::trunc);
	table << "mangled\taction\tfrom\tto\n";
	Placer placer(model, edits, writer, homes, includes, sources, table);
	placer.specializations();
	placer.headerDefinitions();
	placer.needsBlocks();
	writer.flush();
	if (apply)
		edits.apply();
	std::string summary;
	for (const auto& [action, count] : placer.counts)
		summary += " " + std::to_string(count) + " " + action;
	std::fprintf(stderr, "idakit declare --place-specializations:%s%s\n", summary.empty() ? " nothing" : summary.c_str(),
				 apply ? "" : " (dry run: src unchanged)");
	return 0;
}

namespace
{

// The PDB's data symbols (env/cache/pdb-globals.txt, llvm-pdbutil's globals dump): each S_GDATA32 / S_LDATA32 name's
// type as the dump spells it (`BGSCameraPath*`, `NiPointer<NiNode>`; "" for an array) and its type index, the first of
// a name; by last name component. The arrays' records come from the types dump (env/cache/pdb-types.txt): LF_ARRAY's
// byte size and element type.
struct PdbGlobals
{
	PdbGlobals(const std::string& path, const std::string& typesPath)
	{
		std::ifstream in(path);
		if (!in)
			throw Error("idakit: cannot read " + path);
		static const std::regex symbol(R"(\| S_[GL]DATA32 \[size = \d+\] `([^`]+)`)");
		static const std::regex type(R"(type = 0x([0-9A-F]+) \((.*)\), addr = )");
		std::string line, name;
		while (std::getline(in, line))
		{
			std::smatch m;
			if (std::regex_search(line, m, symbol))
				name = m[1];
			else if (!name.empty() && std::regex_search(line, m, type))
			{
				if (types.emplace(name, m[2]).second)
				{
					indices.emplace(name, static_cast<uint32_t>(std::stoul(m[1], nullptr, 16)));
					byLast[lastComponent(name)].push_back(name);
				}
				name.clear();
			}
			else
				name.clear();
		}
		std::ifstream typesIn(typesPath);
		if (!typesIn)
			throw Error("idakit: cannot read " + typesPath);
		static const std::regex element(R"(size: (\d+), index type: .*, element type: 0x([0-9A-F]+) \((.*)\)$)");
		uint32_t array = 0;
		while (std::getline(typesIn, line))
		{
			size_t bar = line.find(" | LF_ARRAY [");
			if (bar != std::string::npos)
			{
				size_t hex = line.find("0x");
				array = hex < bar ? static_cast<uint32_t>(std::stoul(line.substr(hex + 2, bar - hex - 2), nullptr, 16))
								  : 0;
				continue;
			}
			std::smatch m;
			if (array && std::regex_search(line, m, element))
				arrays.emplace(array,
							   Array{std::stoll(m[1]), static_cast<uint32_t>(std::stoul(m[2], nullptr, 16)), m[3]});
			array = 0;
		}
	}
	struct Array
	{
		int64_t bytes;
		uint32_t element;
		std::string spelled;  // the element's spelling, "" for an array
	};
	std::map<std::string, std::string> types;
	std::map<std::string, uint32_t> indices;
	std::map<std::string, std::vector<std::string>> byLast;
	std::map<uint32_t, Array> arrays;
};

// A type as the PDB or a check file spells it (a builtin, typedef, record or enum src has, its leading const, its
// pointers) as a src key; nullopt for anything else (a function pointer, an array, a name src lacks).
std::optional<std::string> spellingKey(const SrcModel& model, std::string text)
{
	auto trim = [](std::string s) {
		size_t a = s.find_first_not_of(" \t"), b = s.find_last_not_of(" \t");
		return a == std::string::npos ? std::string() : s.substr(a, b - a + 1);
	};
	text = trim(text);
	if (text.empty() || text.find("...") != std::string::npos || text.find_first_of("()[") != std::string::npos)
		return std::nullopt;
	int pointers = 0;
	while (!text.empty() && text.back() == '*')
	{
		++pointers;
		text = trim(text.substr(0, text.size() - 1));
	}
	bool isConst = text.starts_with("const ");
	if (isConst)
		text = trim(text.substr(6));
	static const std::set<std::string> builtins = {
		"void", "bool", "char", "signed char", "unsigned char", "short", "unsigned short", "int", "unsigned int", "long",
		"unsigned long", "float", "double", "__int64", "unsigned __int64", "long long", "unsigned long long"};
	std::string key;
	if (builtins.count(text))
		key = "builtin:" + (text == "__int64" ? "long long" : text == "unsigned __int64" ? "unsigned long long" : text);
	else if (model.typedefNamed(text))
		key = "typedef:" + text;
	else if (model.record(text))
		key = "record:" + text;
	else if (model.enumeration(text))
		key = "enum:" + text;
	else
		return std::nullopt;
	if (isConst)
		key = "const(" + key + ")";
	for (int k = 0; k < pointers; ++k)
		key = "ptr(" + key + ")";
	return key;
}

// The PDB's array type at the index as a src key (`array(ptr(builtin:char),40)`): its element's key, its count its
// bytes over the element's src size (-1 for no bytes: an unknown bound); nullopt for an element src lacks or a size
// the element does not divide.
std::optional<std::string> pdbArrayKey(const SrcModel& model, SrcTypes& types, const PdbGlobals& pdb, uint32_t index)
{
	auto array = pdb.arrays.find(index);
	if (array == pdb.arrays.end())
		return std::nullopt;
	std::optional<std::string> element = array->second.spelled.empty()
											  ? pdbArrayKey(model, types, pdb, array->second.element)
											  : spellingKey(model, array->second.spelled);
	if (!element)
		return std::nullopt;
	if (array->second.bytes == 0)
		return "array(" + *element + ",-1)";
	std::optional<int64_t> size = element->starts_with("ptr(") ? 4 : model.sizeOf(types.canonical(*element));
	if (!size || *size <= 0 || array->second.bytes % *size != 0)
		return std::nullopt;
	return "array(" + *element + "," + std::to_string(array->second.bytes / *size) + ")";
}

// the base of the record at the offset with a vtable there: its direct base at the offset, else the one inside the
// direct base holding it; "" for none
std::string vtableBase(const SrcModel& model, const SrcModel::Record& record, int64_t offset)
{
	for (const SrcModel::Base& base : record.bases)
		if (base.offset == offset)
			return base.record;
	for (const SrcModel::Base& base : record.bases)
	{
		const SrcModel::Record* inner = model.record(base.record);
		std::optional<int64_t> size = model.sizeOf("record:" + base.record);
		if (inner && size && offset > base.offset && offset < base.offset + *size)
			return vtableBase(model, *inner, offset - base.offset);
	}
	return "";
}

bool dummyType(const tinfo_t& type)
{
	tinfo_t t = type;
	while (t.is_array())
		t = t.get_array_element();
	qstring name;
	if (!t.get_type_name(&name))
		return false;
	static const std::set<std::string> dummies = {"_BYTE", "_WORD", "_DWORD", "_QWORD", "_OWORD", "_UNKNOWN"};
	return dummies.count(name.c_str()) != 0;
}

// the frozen generated declarations (IDAKIT: nothing adds to it; its globals move to their homes)
const std::string kPrototypes = "gen/Prototypes.hpp";

// The lines of a frozen gen/Prototypes.hpp global's entry: its FNVD_GLOBAL line (at line, as the model has it), the
// comment and alias lines right above it, and the blank line that separates it from the entry before; nullopt when
// line is no FNVD_GLOBAL of the name (the model is older than the file).
std::optional<LineRange> frozenEntry(const std::vector<std::string>& lines, int64_t line, const std::string& name)
{
	if (line < 1 || line > static_cast<int64_t>(lines.size()) ||
		!lines[line - 1].starts_with("FNVD_GLOBAL(") || lines[line - 1].find(", " + name + ", ") == std::string::npos)
		return std::nullopt;
	int64_t first = line;
	while (first > 1 && (lines[first - 2].starts_with("//") || lines[first - 2].starts_with("using ")))
		--first;
	if (first > 1 && trimmedText(lines[first - 2]).empty())
		--first;
	return LineRange{first, line};
}

// A renamed name's new name and the header now declaring it ("" for one whose declaration stays where it is: nothing
// to include). within: the files declaring the old name, whose uses are those in the file and in the files reaching
// it; empty for every file's.
struct Renamed
{
	std::string name, header;
	std::vector<std::string> within;
};

// Each whole-word use of a renamed name (old -> new) in src's files and gen/ (gen/Prototypes.hpp's own entries go
// with their moves, not renamed; a name renamed within files only where the file sees one of them; a comment's words
// are no uses): the files' lines rewritten, and each file includes the headers now declaring the names it uses
// (unless it reaches them; a header that reaches the file cannot be included there). Returns the files changed, each
// with what it could not include ("" for nothing).
std::vector<std::pair<std::string, std::string>> renameUses(FileEdits& edits, IncludeGraph& includes,
															 const std::map<std::string, Renamed>& renames)
{
	std::vector<std::pair<std::string, std::string>> changed;
	if (renames.empty())
		return changed;
	auto word = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; };
	std::set<std::string> headers;  // the headers declaring the names a file's lines use
	std::map<std::string, const Renamed*> visible;  // the renames the file sees
	auto renamed = [&](const std::string& line) {
		std::string out;
		for (size_t k = 0; k < line.size();)
		{
			if (line.compare(k, 2, "//") == 0)
				return out + line.substr(k);
			if (line.compare(k, 2, "/*") == 0)
			{
				size_t close = line.find("*/", k + 2);
				size_t end = close == std::string::npos ? line.size() : close + 2;
				out += line.substr(k, end - k);
				k = end;
				continue;
			}
			if (!word(line[k]) || (k > 0 && word(line[k - 1])))
			{
				out += line[k++];
				continue;
			}
			size_t end = k;
			while (end < line.size() && word(line[end]))
				++end;
			std::string name = line.substr(k, end - k);
			if (auto found = visible.find(name); found != visible.end())
			{
				out += found->second->name;
				if (!found->second->header.empty())
					headers.insert(found->second->header);
			}
			else
				out += name;
			k = end;
		}
		return out;
	};
	std::vector<std::string> files;
	for (const char* root : {"src", "gen"})
		for (const auto& entry : std::filesystem::recursive_directory_iterator(root))
			if (entry.is_regular_file() && entry.path() != kPrototypes)
				if (std::string ext = entry.path().extension().string();
					ext == ".h" || ext == ".hpp" || ext == ".inl" || ext == ".cpp")
					files.push_back(entry.path().string());
	std::sort(files.begin(), files.end());
	for (const std::string& path : files)
	{
		std::ifstream in(path);
		std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
		visible.clear();
		for (const auto& [old, rename] : renames)
			if (text.find(old) != std::string::npos &&
				(rename.within.empty() ||
				 std::any_of(rename.within.begin(), rename.within.end(), [&](const std::string& file) {
					 return file == path || includes.reaches(path, file);
				 })))
				visible.emplace(old, &rename);
		if (visible.empty())
			continue;
		const std::vector<std::string>& lines = edits.lines(path);
		headers.clear();
		for (int64_t k = 1; k <= static_cast<int64_t>(lines.size()); ++k)
			if (std::string line = renamed(lines[k - 1]); line != lines[k - 1])
				edits.replace(path, {k, k}, line);
		if (headers.empty())
			continue;
		// the file's own unguarded include (ensureInclude's test): a header it reaches only through an emitted
		// hybrid-only block's include does not serve the standalone build
		std::string missing;
		for (const std::string& header : headers)
			if (header != path)
			{
				if (includes.reaches(header, path))
					missing += (missing.empty() ? "" : ", ") + header;
				else if (ensureInclude(edits, path, withoutSrc(header)))
					includes.add(path, header);
			}
		changed.emplace_back(path, missing);
	}
	return changed;
}

}  // namespace

// --globals: a src declaration for each exe global, static member and vtable an emit directory's bodies read without
// one (spots.tsv's `no src global at ADDR`, `vtable address`), and for each function whose address they take without
// a src FNVD_ORIGINAL_FUNCTION at it (`address of function ADDR`), each address once (docs/IDAKIT.md "idakit
// declare"). --vftables (dir unused): an FNVD_VFTABLE constant for each exe vtable src has none at, by the same rules.
int declareGlobals(const SrcModel& model, SrcTypes& types, const std::filesystem::path& dir,
				   const std::filesystem::path& out, const std::string& pdbPath, const std::string& pdbTypesPath,
				   const std::vector<uint64_t>& only, bool vftables, bool apply)
{
	ExeVtables exeVtables;
	std::map<ea_t, const ExeVtables::Table*> tables;
	for (const ExeVtables::Table& t : exeVtables.tables())
		tables.emplace(t.address, &t);
	// the addresses and the rows reading each; the functions' addresses among them, with the src types they are passed
	// as (a callback's: `address of function ADDR (name) for TYPE`)
	std::map<uint64_t, std::set<uint64_t>> readers;
	std::set<uint64_t> functions;
	std::map<uint64_t, std::set<std::string>> callbacks;
	if (vftables)
	{
		for (const auto& [ea, table] : tables)
			if (only.empty() || std::find(only.begin(), only.end(), ea) != only.end())
				readers[ea];
	}
	else
	{
		std::ifstream in(dir / "spots.tsv");
		if (!in)
			throw Error("idakit: cannot read " + (dir / "spots.tsv").string());
		// a string compared by its address too: the emitter prints its src global's address, `&aX`
		static const std::regex reason(R"(^(?:(?:no src global at|vtable address) 0x([0-9a-fA-F]+))"
									   R"(|string literal at 0x([0-9a-fA-F]+) .*compared by its address$)"
									   R"(|address of function 0x([0-9a-fA-F]+)(?: \((?:[^:]|::)*?\))?(?: for (.+))?$))");
		std::string line;
		while (std::getline(in, line))
		{
			std::vector<std::string> cells;
			std::istringstream row(line);
			for (std::string cell; std::getline(row, cell, '\t');)
				cells.push_back(cell);
			std::smatch m;
			if (cells.size() >= 3 && std::regex_search(cells[2], m, reason))
			{
				uint64_t ea = std::stoull(m[1].matched ? m[1].str() : m[2].matched ? m[2].str() : m[3].str(), nullptr,
										  16);
				if (only.empty() || std::find(only.begin(), only.end(), ea) != only.end())
				{
					readers[ea].insert(std::stoull(cells[0], nullptr, 16));
					if (m[3].matched)
						functions.insert(ea);
					if (m[4].matched)
						callbacks[ea].insert(m[4]);
				}
			}
		}
	}
	// what each reading body is: the file it goes into (what it replaces: `// replaces the forwarder at FILE:LINE`, the
	// emitted body's), and the type its file-local stand-in for each address reads (`static FNVD_GLOBAL(T,
	// g_XXXXXXXX, addr)`)
	std::map<uint64_t, std::string> joins;
	std::map<uint64_t, std::set<std::string>> reads;
	static const std::regex replaces(R"(^// replaces the (?:forwarder|emitted body) at (.*):\d+$)");
	for (const auto& [ea, rows] : readers)
		for (uint64_t row : rows)
		{
			char file[16];
			std::snprintf(file, sizeof file, "%08llX.cpp", static_cast<unsigned long long>(row));
			std::ifstream in(dir / "src" / file);
			std::string line, previous;
			while (std::getline(in, line))
			{
				if (std::smatch m; std::regex_match(line, m, replaces))
					joins[row] = m[1];
				if (std::optional<EmittedGlobal> g = emittedGlobal(line, previous))
				{
					char standIn[16];
					std::snprintf(standIn, sizeof standIn, "g_%08llX", static_cast<unsigned long long>(ea));
					if (g->name == standIn)
						reads[ea].insert(g->type);
				}
				previous = line;
			}
		}
	PdbGlobals pdb(pdbPath, pdbTypesPath);
	// src's names: every global, static member, vtable constant and original function src declares, by name
	std::multimap<std::string, const SrcModel::Ref*> declared;
	for (const auto& [ea, refs] : model.refs())
		for (const SrcModel::Ref& ref : refs)
			if (ref.kind == "global" || ref.kind == "static_member" || ref.kind == "vftable" ||
				ref.kind == "original_function")
				declared.emplace(ref.target, &ref);
	// the records by their spelling-insensitive key, for the exe's vtables' classes
	std::map<std::string, const SrcModel::Record*> byKey;
	for (const auto& [name, record] : model.records())
		if (record.file.starts_with("src/") && record.endLine > 0)
		{
			byKey.emplace(ExeVtables::classKey(name), &record);
			if (!record.spelled.empty())
				byKey.emplace(ExeVtables::classKey(record.spelled), &record);
		}

	FileEdits edits;
	IncludeGraph includes;
	Writer writer(model, types, edits, includes);
	// a src declaration outside the emitted hybrid-only blocks (FileEdits::has's unguarded line): a declaration only
	// such a block holds is the emitter's, written again on its re-emit (a line the model places outside the file:
	// held unguarded)
	auto unguarded = [&](const SrcModel::Ref& ref) {
		const std::vector<std::string>& lines = edits.lines(ref.file);
		return ref.line < 1 || ref.line > static_cast<int64_t>(lines.size()) ||
			   edits.has(ref.file, lines[ref.line - 1], true);
	};
	std::ofstream table(out / (vftables ? "declare-vftables.tsv" : "declare-globals.tsv"), std::ios::trunc);
	table << "address\taction\tfile\treason\n";
	std::map<std::string, int> counts;
	std::set<std::string> taken;  // the names this run declares
	std::map<std::string, Renamed> renames;  // a moved Prototypes global's name there -> its new one
	// the header for what the reading bodies use: the one of the .cpp most of them join, else its folder's Unplaced.h
	auto readersHeader = [&](const std::set<uint64_t>& rows) {
		std::map<std::string, int> votes;
		for (uint64_t row : rows)
			if (auto j = joins.find(row); j != joins.end())
				++votes[j->second];
		if (votes.empty())
			throw Skip{"no reading body's .cpp"};
		std::string cpp = std::max_element(votes.begin(), votes.end(), [](const auto& a, const auto& b) {
							  return a.second < b.second;
						  })->first;
		std::string header = headerFile(cpp) ? cpp : stem(cpp) + ".h";
		if (!std::filesystem::exists(header))
			header = std::filesystem::path(cpp).parent_path().string() + "/Unplaced.h";
		return header;
	};
	for (const auto& [ea, rows] : readers)
	{
		// --vftables: a vtable src has a constant at (outside the emitted blocks), in a header or a .cpp, is src's
		if (vftables && std::ranges::any_of(model.refsAt(ea), [&](const SrcModel::Ref& ref) {
				return ref.kind == "vftable" && unguarded(ref);
			}))
		{
			++counts["present"];
			continue;
		}
		GlobalDecl g;
		g.ea = ea;
		std::string file, reason;
		try
		{
			// what src declares at the address already (a declaration inside an emitted hybrid-only block is the
			// emitter's, written again on its re-emit: no src declaration). A function's address is no data: a global
			// src or the frozen gen/Prototypes.hpp declares there names the bytes, `&g` its address, and stays
			for (const SrcModel::Ref& ref : model.refsAt(ea))
			{
				if ((ref.kind != "global" && ref.kind != "static_member" && ref.kind != "vftable") ||
					functions.count(ea))
					continue;
				if (!unguarded(ref))
					continue;
				// the frozen gen/Prototypes.hpp's global moves to its home: its line there goes (Writer)
				if (placeholder(ref))
				{
					g.frozenLine = ref.line;
					g.frozenName = ref.target;
					continue;
				}
				char standIn[16];
				std::snprintf(standIn, sizeof standIn, "g_%08llX", static_cast<unsigned long long>(ea));
				if (ref.target == standIn)
					continue;  // an emitted body's stand-in, which its re-emit replaces
				if (headerFile(ref.file))
					throw Skip{"src declares " + ref.target + " in " + ref.file};
				if (ref.kind != "vftable")
					throw Skip{"src declares " + ref.target + " in " + ref.file + " (a src-header item)"};
				g.moveFrom = ref.file;
				g.moveLine = ref.line;
				g.name = ref.target;  // its uses keep their name
			}
			qstring idaName;
			get_name(&idaName, ea);
			std::string name = idaName.c_str();
			// a number Hex-Rays read as an address, or a function's bytes: no data of the exe's (a function whose
			// address a body takes aside)
			if (segment_t* segment = getseg(ea); !functions.count(ea) && (!segment || segment->type == SEG_CODE))
				throw Skip{"no data at the address (code or no segment)"};
			if (functions.count(ea))
			{
				// a function's own address: FNVD_ORIGINAL_FUNCTION, unless src declares one at it; one passed as a
				// callback (the type the emit's body passes it as): of that type, unless src declares one of it in a
				// header. A .cpp's at namespace scope moves to its header, its name kept; a file-local one (`static`,
				// an anonymous namespace) is src's to make a header's.
				std::string callback;
				if (auto found = callbacks.find(ea); found != callbacks.end())
				{
					if (found->second.size() > 1)
						throw Skip{"the bodies pass the function as several types"};
					callback = *found->second.begin();
				}
				const SrcModel::Ref* moved = nullptr;
				for (const SrcModel::Ref& ref : model.refsAt(ea))
				{
					if (ref.kind != "original_function" || placeholder(ref) || !unguarded(ref))
						continue;
					if (callback.empty())
						throw Skip{"src declares " + ref.target + " in " + ref.file};
					if (types.canonical(ref.type) != types.canonical(callback))
						continue;
					if (headerFile(ref.file))
						throw Skip{"src declares " + ref.target + " in " + ref.file};
					if (ref.local)
						throw Skip{ref.file + " declares " + ref.target + " file-local: a body of another file cannot "
											  "name it"};
					if (moved)
						throw Skip{"src declares " + moved->target + " and " + ref.target + " of the type in .cpp files"};
					moved = &ref;
				}
				g.kind = "original_function";
				g.marker = "// Not named in the PDB or IDA; name chosen here.";
				// the src functions at the address (a C++ structor has no pointer to name it), else IDA's: each one's
				// pointer type; a member's takes its object's pointer first, as the exe passes it
				std::vector<std::pair<const SrcModel::Function*, std::string>> fs;
				for (const SrcModel::Ref& ref : model.refsAt(ea))
					if ((ref.kind == "body" || ref.kind == "forwarder" || ref.kind == "address") && !placeholder(ref))
						if (const SrcModel::Function* candidate = model.function(ref.function);
							candidate && candidate->structor.empty() &&
							std::ranges::none_of(fs, [&](const auto& f) { return f.first->name == candidate->name; }))
						{
							std::optional<SrcTypes::Function> type = SrcTypes::function(candidate->type);
							if (!type)
								throw Skip{"src's type of " + candidate->name + " is no function type"};
							if (!candidate->cls.empty() && !candidate->isStatic)
							{
								std::string object = "record:" + candidate->cls;
								type->params.insert(type->params.begin(),
													"ptr(" + (type->isConst ? "const(" + object + ")" : object) + ")");
								type->isConst = false;
							}
							fs.push_back({candidate, "ptr(" + SrcTypes::functionKey(*type) + ")"});
						}
				if (fs.size() > 1 && callback.empty())
					throw Skip{"src declares " + fs[0].first->name + " and " + fs[1].first->name + " at the address"};
				// IDA's name without its parameters (a mangled one demangled)
				auto idaName = [&] {
					qstring shortName;
					std::string text = get_short_name(&shortName, ea) > 0 ? shortName.c_str() : name;
					return identifierOf(text.substr(0, text.find('('))) + kOriginalFunctionSuffix;
				};
				if (fs.empty())
				{
					tinfo_t ida;
					std::string why;
					if (!get_tinfo(&ida, ea) || !ida.is_func())
						throw Skip{"neither src nor IDA has a type of the function"};
					std::optional<std::string> key = types.keyOf(ida, why);
					if (!key)
						throw Skip{"IDA's type of the function is no src type: " + why};
					fs.push_back({nullptr, "ptr(" + *key + ")"});
				}
				// a callback's type is the function's own as the CRT's callbacks erase it (a fold's: one of the
				// functions src names there), else src's two types of it disagree (a declaration's to fix)
				if (!callback.empty() &&
					std::ranges::none_of(fs, [&](const auto& f) { return erasesTo(types, f.second, callback); }))
					throw Skip{"src passes it as " + callback + ", " +
							   (fs[0].first ? "src's " + fs[0].first->name : "IDA's type") + " is " + fs[0].second};
				g.key = callback.empty() ? fs[0].second : callback;
				std::string base = fs.size() == 1 && fs[0].first ? identifierOf(fs[0].first->name) : "";
				g.name = moved		   ? moved->target
						 : !base.empty() ? base + kOriginalFunctionSuffix
										 : idaName();
				if (moved)
				{
					g.moveFrom = moved->file;
					g.moveLine = moved->line;
					g.marker.clear();
				}
				// a moved constant goes to its .cpp's own header, which the .cpp's uses see
				g.header = moved && std::filesystem::exists(stem(moved->file) + ".h") ? stem(moved->file) + ".h"
																					 : readersHeader(rows);
				// a new constant's name src gives the address under another type already: a callback typedef's
				// constant takes the typedef's name before the suffix (`X_Destruct_EhVectorCallback_original`)
				auto named = [&] {
					return std::ranges::find_if(model.refsAt(ea), [&](const SrcModel::Ref& ref) {
						return ref.kind == "original_function" && ref.target == g.name && unguarded(ref);
					});
				};
				if (auto other = named(); !moved && other != model.refsAt(ea).end())
				{
					std::string typedefName = callback.starts_with("typedef:") ? callback.substr(8) : "";
					if (typedefName.empty())
						throw Skip{"src declares " + g.name + " at the address as " + other->type + " in " +
								   other->file};
					std::string stemName =
						g.name.substr(0, g.name.size() - std::string_view(kOriginalFunctionSuffix).size());
					g.name = stemName + "_" + identifierOf(lastComponent(typedefName)) + kOriginalFunctionSuffix;
					if (named() != model.refsAt(ea).end())
						throw Skip{"src declares " + g.name + " at the address in " + other->file};
				}
			}
			else if (auto t = tables.find(ea); t != tables.end())
			{
				// a vtable: its class and subobject from the exe's RTTI (IDA's repeats `_N` aside)
				g.kind = "vftable";
				auto record = byKey.find(ExeVtables::classKey(t->second->cls));
				if (record == byKey.end())
					throw Skip{"no src class " + t->second->cls + " for the vtable"};
				g.owner = record->second;
				std::string base = t->second->offset == 0 ? "" : vtableBase(model, *g.owner, t->second->offset);
				if (t->second->offset != 0 && base.empty())
					throw Skip{"no src base of " + g.owner->name + " at " + address(t->second->offset)};
				if (g.moveFrom.empty())
					g.name = identifierOf(g.owner->name) + "_vftable" + (base.empty() ? "" : "_" + identifierOf(base));
				if (g.moveFrom.empty() && (!g.owner->pattern.empty() || g.owner->name.find("::") != std::string::npos ||
										   (!base.empty() && !identifier(base))))
					g.marker = "// Not named in the PDB or IDA; name chosen here.";
			}
			else if (name.starts_with("??_7"))
				throw Skip{"no exe vtable at the address of " + name};
			else if (name.starts_with("??_R0?A"))
			{
				// a compiler-generated RTTI type descriptor (IDA's ??_R0?AV<Class>@@@8): bytes in BSCore/RTTI.h,
				// named RTTI_<Class> as the rows there are
				qstring demangled;
				std::string text = demangle_name(&demangled, name.c_str(), MNG_NODEFINIT) > 0 ? demangled.c_str() : "";
				size_t end = text.find(" `RTTI Type Descriptor'");
				if (end == std::string::npos || end == 0)
					throw Skip{"the RTTI type descriptor " + name + " names no class"};
				g.kind = "global";
				g.name = "RTTI_" + identifierOf(text.substr(0, end));
				g.marker = "// Not named in the PDB or IDA; name chosen here.";
				g.key = "const(typedef:uint8_t)";
				g.header = "src/BSCore/RTTI.h";
			}
			else
			{
				// the PDB's name: IDA's itself, or a Setting's (`bDecals:Decals`) unique one of its last component
				std::string pdbName;
				if (name.starts_with('?'))
				{
					// the main name alone: a data symbol's short form leads with its type (`NiColorA X::fAmplitude`)
					qstring demangled;
					if (demangle_name(&demangled, name.c_str(), MNG_NODEFINIT) > 0)
						name = demangled.c_str();
				}
				if (name.starts_with('.'))
					name = name.substr(1);
				if (pdb.types.count(name))
					pdbName = name;
				else if (size_t colon = name.find(':');
						 colon != std::string::npos && name.find("::") == std::string::npos)
				{
					auto found = pdb.byLast.find(name.substr(0, colon));
					if (found == pdb.byLast.end() || found->second.size() != 1)
						throw Skip{"IDA's name " + name + " is no identifier, and the PDB has no one global of it"};
					pdbName = found->second[0];
				}
				std::string qualified = pdbName.empty() ? name : pdbName;
				if (pdbName.empty())
					g.marker = "// IDA name; not in the PDB.";
				std::string scope = qualified.rfind("::") == std::string::npos
										? ""
										: qualified.substr(0, qualified.rfind("::"));
				g.name = lastComponent(qualified);
				if (!identifier(g.name) || libraryName(g.name))
					throw Skip{"name " + qualified + " is no identifier src can declare"};
				if (!scope.empty())
				{
					const SrcModel::Record* owner = model.record(scope);
					if (!owner || !owner->file.starts_with("src/") || owner->endLine == 0)
						throw Skip{"no src class " + scope + " for " + qualified};
					if (!owner->pattern.empty())
						throw Skip{qualified + " is a static member of a class template instance"};
					g.kind = "static_member";
					g.owner = owner;
					// a member named as a namespace-scope type (`HDRParams HDRParams`) hides it in its class
					if (model.record(g.name) || model.enumeration(g.name) || model.typedefNamed(g.name))
						throw Skip{"src names a type " + g.name + ", which the member would hide in " + scope};
				}
				else
					g.kind = "global";
				// the type: a C string's first char (src's form of a string, `FNVD_GLOBAL(const char, aX, ...)`), the
				// PDB's, else IDA's own, else the one every use reads
				std::optional<std::string> key;
				std::string from = "the PDB";
				if (is_strlit(get_flags(ea)))
				{
					key = "const(builtin:char)";
					from = "the string";
				}
				else if (!pdbName.empty())
				{
					const std::string& spelled = pdb.types.at(pdbName);
					if (!spelled.empty())
						key = spellingKey(model, spelled);
					else if (pdb.arrays.count(pdb.indices.at(pdbName)))
					{
						// an array (the dump spells none): IDA's reading of it as its first element is no type of it
						key = pdbArrayKey(model, types, pdb, pdb.indices.at(pdbName));
						if (!key)
							throw Skip{"the PDB's type of " + pdbName + " is an array src cannot spell"};
					}
				}
				tinfo_t ida;
				std::string why;
				if (!key && get_tinfo(&ida, ea) && !dummyType(ida))
				{
					key = types.keyOf(ida, why);
					from = "IDA";
				}
				if (!key)
				{
					const std::set<std::string>& used = reads[ea];
					if (used.size() > 1)
						throw Skip{"uses read different types"};
					if (used.empty() || used.begin()->starts_with("gen::"))
						throw Skip{"no type: neither the PDB nor IDA has one, and the uses read bytes"};
					key = spellingKey(model, *used.begin());
					if (!key)
						throw Skip{"the uses' type " + *used.begin() + " is no src type"};
					from = "the uses";
				}
				// a container or smart pointer of the folds' class-neutral element (`NiPointer<NiRefObject>`,
				// `BSSimpleList<void *>`, `BSSimpleArray<void *, 1024>`; IDAKIT "Sync", folds) names no element type
				// of the global's own
				static const std::regex neutral(R"(^record:(?:NiPointer<NiRefObject>|[\w:]+<void \*(?:, *\d+)?>)$)");
				if (from != "the PDB" && std::regex_match(*key, neutral))
					throw Skip{"no type: " + from + " has only the folds' class-neutral " + key->substr(7)};
				g.key = *key;
				// an object the address does not align (a folded function's dummy `this` Hex-Rays reads as one)
				std::optional<int64_t> size = model.sizeOf(types.canonical(g.key));
				if (size && *size > 1 && ea % std::min<int64_t>(*size, 4) != 0)
					throw Skip{"the address does not align " + g.key};
				// the header: the one of the .cpp most of the reading bodies join, else its folder's Unplaced.h
				if (g.kind == "global")
					g.header = readersHeader(rows);
			}
			std::string qualifiedName = (g.kind == "static_member" ? g.owner->name + "::" : "") + g.name;
			// a name src gives a type or function in the same scope (IDA's `bhkSphereShape` for a class's name
			// string) would hide it
			if (model.record(qualifiedName) || model.enumeration(qualifiedName) || model.typedefNamed(qualifiedName) ||
				!model.functionsNamed(qualifiedName).empty())
				throw Skip{"src names a type or function " + qualifiedName};
			for (auto [other, end] = declared.equal_range(qualifiedName); other != end; ++other)
				if (other->second->address != ea && unguarded(*other->second))
					throw Skip{"src declares " + qualifiedName + " at " + address(other->second->address) + " in " +
							   other->second->file};
			if (g.kind != "static_member")
				for (auto [other, end] = declared.equal_range(g.name); other != end; ++other)
					if (other->second->local && unguarded(*other->second))
						throw Skip{"a .cpp declares " + g.name + " file-local (" + other->second->file + ")"};
			if (!taken.insert(qualifiedName).second)
				throw Skip{"this run declares " + qualifiedName + " at another address"};
			file = g.kind == "global" || g.kind == "original_function" ? g.header : g.owner->file;
			// a frozen gen/Prototypes.hpp global: its entry there goes, and src's uses of its name there take the new
			// one
			std::optional<LineRange> frozen;
			if (!g.frozenName.empty() &&
				!(frozen = frozenEntry(edits.lines(kPrototypes), g.frozenLine, g.frozenName)))
				throw Skip{"the model's line of " + g.frozenName + " is not " + kPrototypes +
						   "'s: the model is older than src"};
			reason = writer.declareGlobal(g);
			if (frozen)
			{
				edits.replace(kPrototypes, *frozen, "");
				reason += " (moved from " + kPrototypes +
						  (g.frozenName == qualifiedName ? "" : ", where it is " + g.frozenName) + ")";
				if (g.frozenName != qualifiedName)
					renames[g.frozenName] = Renamed{qualifiedName, file, {}};
			}
			++counts["declared"];
			table << hexText(ea) << "\tdeclared\t" << file << '\t' << reason << '\n';
		}
		catch (const Skip& s)
		{
			++counts["skipped"];
			table << hexText(ea) << "\tskipped\t" << file << '\t' << s.reason << '\n';
		}
	}
	writer.flush();
	// the uses of the moved Prototypes globals' old names
	for (const auto& [path, missing] : renameUses(edits, includes, renames))
		table << "\trenamed\t" << path << '\t'
			  << (missing.empty() ? "uses of gen/Prototypes.hpp names moved under another name"
								  : "renamed, but cannot include " + missing + ", which reaches the file")
			  << '\n';
	if (apply)
		edits.apply();
	if (vftables)
		std::fprintf(stderr, "idakit declare --vftables: %zu exe vtables: %d src's, %d declared, %d skipped%s\n",
					 readers.size(), counts["present"], counts["declared"], counts["skipped"],
					 apply ? "" : " (dry run: src unchanged)");
	else
		std::fprintf(stderr, "idakit declare --globals: %zu addresses: %d declared, %d skipped%s\n", readers.size(),
					 counts["declared"], counts["skipped"], apply ? "" : " (dry run: src unchanged)");
	return 0;
}

// --layouts: a sizeof pin, `FNVD_LAYOUT(sizeof(A) == 0xN);`, for each class an emit directory's allocations build
// without one (spots.tsv's `literal allocation size N: no size check for A`, Allocations). The exe's evidence is the
// sizes its allocations of one A take, not IDA's struct size (sync writes src's layouts into the database): where every
// site's N agrees and equals srcdb's hybrid sizeof, the pin goes after A's definition (its outermost class's, for a
// nested class), or after the last FNVD_LAYOUT line naming A that follows it. Sites that disagree with each other or
// with srcdb are `size differs` (DIR/declare-layouts.tsv); a template instance is skipped: its pin needs a home that
// sees its arguments and a typedef (a comma would split FNVD_LAYOUT's argument).
int declareLayouts(const SrcModel& model, const std::filesystem::path& dir, const std::filesystem::path& out,
				   bool apply)
{
	struct Sites
	{
		std::set<int64_t> sizes;
		std::set<std::string> addresses;
	};
	std::map<std::string, Sites> classes;
	{
		std::ifstream in(dir / "spots.tsv");
		if (!in)
			throw Error("idakit: cannot read " + (dir / "spots.tsv").string());
		static const std::regex reason(R"(^literal allocation size (\d+): no size check for (.+)$)");
		std::string line;
		while (std::getline(in, line))
		{
			std::vector<std::string> cells;
			std::istringstream row(line);
			for (std::string cell; std::getline(row, cell, '\t');)
				cells.push_back(cell);
			std::smatch m;
			if (cells.size() >= 3 && std::regex_match(cells[2], m, reason))
			{
				Sites& sites = classes[m[2]];
				sites.sizes.insert(std::stoll(m[1]));
				sites.addresses.insert(cells[0]);
			}
		}
	}
	FileEdits edits;
	std::ofstream table(out / "declare-layouts.tsv", std::ios::trunc);
	table << "class\taction\tfile\tsizes\tsrc size\treason\taddresses\n";
	std::map<std::string, int> counts;
	for (const auto& [name, sites] : classes)
	{
		std::string sizes, file, reason;
		for (int64_t size : sites.sizes)
			sizes += (sizes.empty() ? "" : ",") + address(static_cast<uint64_t>(size));
		std::string action = "skipped";
		const SrcModel::Record* record = model.record(name);
		// the class whose definition the pin follows: the outermost record of a nested class's scope
		const SrcModel::Record* outer = record;
		for (size_t colons = name.find("::"); record && colons != std::string::npos; colons = name.find("::", colons + 2))
			if (const SrcModel::Record* scope = model.record(name.substr(0, colons)))
			{
				outer = scope;
				break;
			}
		if (!record)
			reason = "no src record";
		else if (record->pinned)
			reason = "src pins its sizeof";
		else if (!record->pattern.empty() || name.find('<') != std::string::npos)
			reason = "template instance";
		else if (sites.sizes.size() > 1 || *sites.sizes.begin() != record->size)
			action = "size differs";
		else
		{
			file = outer->file;
			const std::vector<std::string>& lines = edits.lines(file);
			int64_t size = static_cast<int64_t>(lines.size());
			if (outer->endLine < 1 || outer->endLine > size || lines[outer->endLine - 1].find('}') == std::string::npos)
				reason = "the model's lines are not the file's";
			else
			{
				// after the definition, or after the last FNVD_LAYOUT statement about A that follows it; A is
				// spelled as src writes it there, without the namespaces open at its definition
				static const std::regex layout(R"(^\s*FNVD_LAYOUT\()");
				std::string scope = scopeAt(lines, outer->endLine);
				std::string spelled = !scope.empty() && name.starts_with(scope + "::") ? name.substr(scope.size() + 2)
																						: name;
				auto names = [&](const std::string& statement) {
					for (const std::string& n : {name, spelled})
						if (statement.find("sizeof(" + n + ")") != std::string::npos ||
							statement.find("offsetof(" + n + ",") != std::string::npos)
							return true;
					return false;
				};
				int64_t after = outer->endLine;
				for (int64_t k = outer->endLine + 1; k <= size; ++k)
				{
					if (!std::regex_search(lines[k - 1], layout))
						continue;
					int64_t end = k;
					std::string statement = lines[k - 1];
					while (statement.find(");") == std::string::npos && end < size)
						statement += lines[end++];
					if (names(statement) && scopeAt(lines, end) == scope)
						after = end;
					k = end;
				}
				std::string indent = lines[after - 1].substr(0, lines[after - 1].find_first_not_of(" \t"));
				if (after == outer->endLine)
					indent = lines[outer->line - 1].substr(0, lines[outer->line - 1].find_first_not_of(" \t"));
				edits.insert(file, after + 1,
							 indent + "FNVD_LAYOUT(sizeof(" + spelled + ") == " +
								 address(static_cast<uint64_t>(record->size)) + ");");
				action = "declared";
			}
		}
		std::string listed;
		for (const std::string& a : sites.addresses)
			listed += (listed.empty() ? "" : ",") + a;
		++counts[action];
		table << name << '\t' << action << '\t' << file << '\t' << sizes << '\t'
			  << (record ? address(static_cast<uint64_t>(record->size)) : "") << '\t' << reason << '\t' << listed
			  << '\n';
	}
	if (apply)
		edits.apply();
	std::fprintf(stderr, "idakit declare --layouts: %zu classes: %d declared, %d size differs, %d skipped%s\n",
				 classes.size(), counts["declared"], counts["size differs"], counts["skipped"],
				 apply ? "" : " (dry run: src unchanged)");
	return 0;
}

// DeclareCallees.cpp
std::vector<uint64_t> calleeAddresses(const std::filesystem::path& dir, const std::vector<uint64_t>& only,
									  std::map<uint64_t, std::string>& passed);
std::vector<std::tuple<uint64_t, std::string, std::string>> instanceCalls(const std::filesystem::path& dir);
// DeclareSignatures.cpp, DeclareRetype.cpp
int declarePdbSignatures(const SrcModel& model, SrcTypes& types, const std::filesystem::path& out);
// DeclareFields.cpp
int declarePdbFields(const SrcModel& model, SrcTypes& types, const std::filesystem::path& out);
int declareRetype(const SrcModel& model, SrcTypes& types, const std::string& table, const std::filesystem::path& out,
				  bool apply);
int declareRevert(const std::filesystem::path& out, const std::vector<std::string>& ids);
// DeclareHavok.cpp
int declareHavok(const SrcModel& model, SrcTypes& types, const PdbSources& sources, const std::string& symbols,
				 const std::vector<uint64_t>& only, const std::filesystem::path& out, bool apply);

namespace
{

// --unregister ADDR... | @FILE: the registrations DIR/declare.tsv's `registered` rows name for those addresses taken
// out of their files again (a checkbody DIFF's row): the FNVD_REPLACES statement at the address, the `using` alias
// right above it where nothing else in the file names it, and a blank line it leaves at the file's end; outcomes in
// DIR/unregister.tsv. Dry run unless apply.
int declareUnregister(const std::filesystem::path& out, const std::vector<std::string>& items, bool apply)
{
	std::vector<uint64_t> wanted = parseAddresses(items);
	std::set<uint64_t> addresses(wanted.begin(), wanted.end());
	std::ifstream in(out / "declare.tsv");
	if (!in)
		throw Error("declare --unregister: cannot read " + (out / "declare.tsv").string());
	std::map<uint64_t, std::string> files;  // a registered row's address -> its file
	for (std::string line; std::getline(in, line);)
	{
		std::vector<std::string> c;
		for (size_t from = 0;;)
		{
			size_t tab = line.find('\t', from);
			c.push_back(line.substr(from, tab == std::string::npos ? std::string::npos : tab - from));
			if (tab == std::string::npos)
				break;
			from = tab + 1;
		}
		if (c.size() > 4 && c[1] == "registered" && c[0].starts_with("0x"))
			if (uint64_t ea = std::stoull(c[0], nullptr, 16); addresses.count(ea))
				files[ea] = c[4];
	}
	FileEdits edits;
	std::ofstream table(out / "unregister.tsv", std::ios::trunc);
	table << "address\toutcome\tfile\tlines\n";
	std::map<std::string, int> outcomes;
	for (uint64_t ea : addresses)
	{
		auto found = files.find(ea);
		std::string outcome = "not registered by this declare run", file, span;
		if (found != files.end())
		{
			file = found->second;
			const std::vector<std::string>& lines = edits.lines(file);
			int64_t size = static_cast<int64_t>(lines.size());
			std::regex at("^\\s*FNVD_REPLACES\\w*\\(\\s*0x0*" + hexText(ea).substr(2) + "\\s*,", std::regex::icase);
			int64_t first = 0;
			for (int64_t k = 1; k <= size && !first; ++k)
				if (std::regex_search(lines[k - 1], at))
					first = k;
			outcome = "no registration of the address in the file";
			if (first)
			{
				int64_t last = first;
				while (last < size && !trimmedText(lines[last - 1]).ends_with(';'))
					++last;
				// the `using` alias statement right above it (its continuation lines included), where nothing else in
				// the file names the alias
				static const std::regex alias(R"(^using (\w+) = )");
				int64_t top = first, k = first - 1;
				auto ends = [&](int64_t j) {
					std::string text = trimmedText(lines[j - 1]);
					return text.empty() || text.ends_with(';') || text.starts_with("//") || text.starts_with('#');
				};
				while (k > 1 && !ends(k - 1))
					--k;
				std::smatch m;
				if (k >= 1 && k < first && trimmedText(lines[first - 2]).ends_with(';') &&
					std::regex_search(lines[k - 1], m, alias))
				{
					std::regex word("\\b" + m[1].str() + "\\b");
					bool used = false;
					for (int64_t j = 1; j <= size && !used; ++j)
						used = (j < k || j > last) && std::regex_search(lines[j - 1], word);
					if (!used)
						top = k;
				}
				// a blank line the registration leaves at the file's end
				if (last == size && top > 1 && trimmedText(lines[top - 2]).empty())
					--top;
				edits.replace(file, {top, last}, "");
				outcome = "removed";
				span = std::to_string(top) + "-" + std::to_string(last);
			}
		}
		++outcomes[outcome];
		table << hexText(ea) << '\t' << outcome << '\t' << file << '\t' << span << '\n';
	}
	if (apply)
		edits.apply();
	std::string summary;
	for (const auto& [outcome, count] : outcomes)
		summary += "; " + std::to_string(count) + " " + outcome;
	std::fprintf(stderr, "idakit declare --unregister: %zu addresses%s%s\n", addresses.size(), summary.c_str(),
				 apply ? "" : " (dry run: src unchanged)");
	return 0;
}

}  // namespace

int runDeclare(Args& args)
{
	std::filesystem::path out = args.required("out");
	// --revert ROW...: the named rows of DIR/applied.tsv put back (no model)
	if (args.flag("revert"))
		return declareRevert(out, args.positional());
	// --unregister ADDR...: the named registered rows of DIR/declare.tsv taken out of src again (no model)
	if (args.flag("unregister"))
		return declareUnregister(out, args.positional(), args.flag("apply"));
	std::string modelPath = args.required("model");
	bool apply = args.flag("apply");
	if (args.flag("layouts"))
	{
		std::filesystem::path emitted = args.required("emitted");
		std::filesystem::create_directories(out);
		return declareLayouts(SrcModel(modelPath), emitted, out, apply);
	}
	std::string aliases = args.required("aliases");
	if (args.flag("pdb-signatures") || args.flag("pdb-fields") || args.option("retype") || args.option("hk-symbols"))
	{
		std::filesystem::create_directories(out);
		SrcModel model(modelPath);
		SrcTypes types(model, aliases);
		if (args.flag("pdb-fields"))
			return declarePdbFields(model, types, out);
		if (std::optional<std::string> symbols = args.option("hk-symbols"))
			return declareHavok(model, types, PdbSources(args.required("sources")), *symbols,
								parseAddresses(args.positional()), out, apply);
		return args.flag("pdb-signatures") ? declarePdbSignatures(model, types, out)
										   : declareRetype(model, types, *args.option("retype"), out, apply);
	}
	PdbSources sources(args.required("sources"));
	std::filesystem::create_directories(out);
	SrcModel model(modelPath);
	SrcTypes types(model, aliases);
	if (args.flag("place-specializations"))
		return placeSpecializations(model, types, sources, out, apply);

	std::string idb = args.required("idb");
	std::vector<uint64_t> addresses = parseAddresses(args.positional());
	exefacts::Program program(".");
	exefacts::Facts facts(program);
	Database::Options options;
	options.hexrays = false;
	Database database(idb, options);
	if (args.flag("globals") || args.flag("vftables"))
	{
		bool vftables = args.flag("vftables");
		return declareGlobals(model, types, vftables ? std::string() : args.required("emitted"), out,
							  args.option("pdb-globals").value_or("env/cache/pdb-globals.txt"),
							  args.option("pdb-types").value_or("env/cache/pdb-types.txt"), addresses, vftables, apply);
	}
	ExeVtables vtables;
	ExeSlotTargets slotTargets(vtables, facts);
	IncludeGraph includes;
	SpecializationHomes homes(model, includes);
	// --pdb-types: the PDB's vtables, which name two slot functions of one class at a fold
	Declarer declarer(model, types, facts, vtables, sources, homes,
					  args.option("pdb-types").value_or("env/cache/pdb-types.txt"));

	// --folds FILE: lines `ADDRESS<TAB>Y::g[<TAB>chosen]`, each a member the linker folded into the exe function at
	// ADDRESS that src lacks (harness/tools/fold_members.py's folds.list), declared on Y (chosen: g is the owner's
	// name, declared with the owner's signature); --fold-calls: the emit directory's calls of a class template
	// instance's member on another instance of its template (spots.tsv's `call of O::f on R`), each declared on the
	// receiver's instance; outcomes in declare-folds.tsv
	std::optional<std::string> folds = args.option("folds");
	bool foldCalls = args.flag("fold-calls");
	if (folds || foldCalls)
	{
		struct Row
		{
			uint64_t ea;
			std::string qualified, receiver;
			bool chosen = false;
		};
		std::vector<Row> list;
		if (foldCalls)
			for (const auto& [ea, owner, receiver] : instanceCalls(args.required("emitted")))
				list.push_back({ea, owner, receiver});
		else
		{
			std::ifstream in(*folds);
			if (!in)
				throw Error("declare --folds: cannot read " + *folds);
			for (std::string line; std::getline(in, line);)
			{
				size_t tab = line.find('\t');
				if (line.empty() || tab == std::string::npos)
					continue;
				Row row{std::stoull(line.substr(0, tab), nullptr, 16), line.substr(tab + 1), ""};
				if (size_t more = row.qualified.find('\t'); more != std::string::npos)
				{
					row.chosen = row.qualified.substr(more + 1) == "chosen";
					row.qualified.resize(more);
				}
				list.push_back(row);
			}
		}
		FileEdits edits;
		Writer writer(model, types, edits, includes);
		std::ofstream table(out / "declare-folds.tsv", std::ios::trunc);
		table << "address\tcandidate\toutcome\tkind\tname\tfile\treason\n";
		std::map<std::string, int> outcomes;
		std::map<std::string, int> reasons;
		size_t rows = list.size();
		for (const Row& row : list)
		{
			Decl d;
			std::string reason;
			try
			{
				d = foldCalls ? declarer.foldCall(row.ea, row.qualified, row.receiver)
							  : declarer.foldMember(row.ea, row.qualified, row.chosen);
				writer.add(d);
			}
			catch (const Skip& s)
			{
				reason = s.reason;
				d.outcome = "skipped";
				d.source.clear();
				++reasons[reason.substr(0, reason.find(':'))];
			}
			++outcomes[d.outcome];
			std::string name = d.outcome == "declared" || d.outcome == "specialized" ? d.owner->name + "::" + d.member
																					 : "";
			table << hexText(row.ea) << '\t' << row.qualified << (row.receiver.empty() ? "" : " on " + row.receiver)
				  << '\t' << d.outcome << "\tfold\t" << name << '\t' << d.source << '\t' << reason << '\n';
		}
		writer.flush();
		std::ofstream needs(out / "needs-class.tsv", std::ios::trunc);
		needs << "class\taddress\tname\n";
		for (const NeedsClass& n : declarer.needs)
			needs << n.cls << '\t' << hexText(n.ea) << '\t' << n.name << '\n';
		if (apply)
			edits.apply();
		std::string summary;
		for (const auto& [outcome, count] : outcomes)
			summary += " " + std::to_string(count) + " " + outcome;
		std::fprintf(stderr, "idakit declare --folds: %zu candidates:%s%s\n", rows, summary.c_str(),
					 apply ? "" : " (dry run: src unchanged)");
		for (const auto& [reason, count] : reasons)
			std::fprintf(stderr, "  %6d skipped: %s\n", count, reason.c_str());
		return 0;
	}

	// --callees: the functions the emit directory's bodies call without a src declaration, else (no addresses) every
	// game function
	bool callees = args.flag("callees");
	std::map<uint64_t, std::string> passed;
	if (callees)
		addresses = calleeAddresses(args.required("emitted"), addresses, passed);
	else if (addresses.empty())
		for (size_t k = 0, count = get_func_qty(); k < count; ++k)
			if (func_t* f = getn_func(k); f && f->start_ea < kGameEnd)
				addresses.push_back(f->start_ea);
	std::sort(addresses.begin(), addresses.end());

	FileEdits edits;
	Writer writer(model, types, edits, includes);
	std::ofstream table(out / (callees ? "declare-callees.tsv" : "declare.tsv"), std::ios::trunc);
	table << "address\toutcome\tkind\tname\tfile\treason\n";
	std::map<std::string, int> outcomes;
	for (const auto& [ea, reason] : passed)
	{
		++outcomes["skipped"];
		table << hexText(ea) << "\tskipped\t\t" << functionName(ea) << "\t\t" << reason << '\n';
	}
	size_t selected = 0;
	// An FNVD_ORIGINAL_FUNCTION constant at a declared function's address whose name the declaration takes, in the
	// constant's scope or one inside it (where the new member would hide it), takes the suffix `_original` (declare's
	// own constants' form) with the chosen-name marker, and src's uses of it in the files that see it the new name.
	auto unanonymous = [](std::string name) {
		for (size_t at = name.find("(anonymous namespace)::"); at != std::string::npos;
			 at = name.find("(anonymous namespace)::", at))
			name.erase(at, 23);
		return name;
	};
	std::set<std::string> globalNames;  // src's globals, static members and constants, by qualified name
	for (const auto& [address, refs] : model.refs())
		for (const SrcModel::Ref& r : refs)
			if (r.kind == "global" || r.kind == "static_member" || r.kind == "original_function" ||
				r.kind == "vftable")
				globalNames.insert(unanonymous(r.target));
	std::map<std::string, Renamed> constantRenames;  // a clashing constant's name -> its new one
	std::set<std::pair<std::string, int64_t>> markedConstants;  // (file, line) of the constants renamed
	auto clashes = [&](const Decl& d, const std::string& qualified) {
		std::vector<std::pair<const SrcModel::Ref*, std::string>> out;
		std::string scope = scopeOf(qualified);
		for (const SrcModel::Ref& r : model.refsAt(d.ea))
		{
			if (r.kind != "original_function" || placeholder(r))
				continue;
			std::string target = unanonymous(r.target), within = scopeOf(target);
			if (lastComponent(target) != lastComponent(qualified) ||
				!(within.empty() || within == scope || scope.starts_with(within + "::")))
				continue;
			std::string renamed = lastComponent(target) + kOriginalFunctionSuffix;
			std::string full = within.empty() ? renamed : within + "::" + renamed;
			if (globalNames.count(full) || !model.functionsNamed(full).empty() || model.record(full))
				throw Skip{"constant " + r.target + " clashes, and " + full + " is taken"};
			out.emplace_back(&r, renamed);
		}
		return out;
	};
	for (uint64_t ea : addresses)
	{
		std::optional<std::string> skip = declarer.selection(ea);
		// a callee src declares already (a C++ structor src registers, called through its class's Construct or
		// Destruct) is the Calls pass's
		if (!skip && callees)
		{
			++outcomes["skipped"];
			table << hexText(ea) << "\tskipped\t\t" << functionName(ea) << "\t\tsrc declares it already\n";
		}
		if (!skip)
			continue;
		++selected;
		Decl d;
		d.ea = ea;
		d.kind = "";
		std::string name = functionName(ea);
		if (skip->empty())
		{
			try
			{
				d = declarer.plan(ea);
				std::vector<std::pair<const SrcModel::Ref*, std::string>> clashing;
				if (d.outcome == "declared" || d.outcome == "specialized")
					clashing = clashes(d, d.owner ? d.owner->name + "::" + d.member
												  : (d.scope.empty() ? "" : d.scope + "::") + d.member);
				writer.add(d);
				for (const auto& [ref, renamed] : clashing)
				{
					Renamed& rename = constantRenames[lastComponent(unanonymous(ref->target))];
					rename.name = renamed;
					if (std::find(rename.within.begin(), rename.within.end(), ref->file) == rename.within.end())
						rename.within.push_back(ref->file);
					d.proof += (d.proof.empty() ? "" : "; ") + std::string("constant ") + ref->target + " renamed " +
							   renamed;
					if (!markedConstants.insert({ref->file, ref->line}).second)
						continue;
					// the chosen-name marker above the constant, in place of an IDA-name marker heading the comment
					// line right above it (the rest of that line kept on its own comment line)
					const std::vector<std::string>& lines = edits.lines(ref->file);
					const std::string& line = lines.at(ref->line - 1);
					std::string indent = line.substr(0, line.find_first_not_of(" \t"));
					std::string marker = indent + "// Not named in the PDB or IDA; name chosen here.";
					std::string above = ref->line > 1 ? trimmedText(lines[ref->line - 2]) : "";
					const std::string idaMarker = "// IDA name; not in the PDB.";
					if (above.starts_with(idaMarker))
					{
						std::string rest = trimmedText(above.substr(idaMarker.size()));
						edits.replace(ref->file, {ref->line - 1, ref->line - 1},
									  marker + (rest.empty() ? "" : "\n" + indent + "// " + rest));
					}
					else if (above != trimmedText(marker))
						edits.insert(ref->file, ref->line, marker);
				}
			}
			catch (const Skip& s)
			{
				skip = s.reason;
				d.kind = declarer.kind();
			}
		}
		if (!skip->empty())
		{
			d.outcome = "skipped";
			d.source.clear();
		}
		else
			name = d.owner ? d.owner->name + "::" + d.member : (d.scope.empty() ? "" : d.scope + "::") + d.member;
		++outcomes[d.outcome];
		// an addressed row's reason column holds what ties src's body to the address
		table << hexText(ea) << '\t' << d.outcome << '\t' << d.kind << '\t' << name << '\t' << d.source << '\t'
			  << (skip->empty() ? d.proof : *skip) << '\n';
	}
	writer.flush();
	// the clashing constants' uses (edits.lines are src as read: the new declarations' text is not renamed)
	renameUses(edits, includes, constantRenames);
	std::ofstream needs(out / "needs-class.tsv", std::ios::trunc);
	needs << "class\taddress\tname\n";
	for (const NeedsClass& n : declarer.needs)
		needs << n.cls << '\t' << hexText(n.ea) << '\t' << n.name << '\n';
	if (apply)
		edits.apply();
	std::string summary;
	for (const auto& [outcome, count] : outcomes)
		summary += " " + std::to_string(count) + " " + outcome;
	std::fprintf(stderr, "idakit declare: %zu addresses, %zu without a src declaration:%s%s\n", addresses.size(),
				 selected, summary.c_str(), apply ? "" : " (dry run: src unchanged)");
	return 0;
}

}  // namespace idakit
