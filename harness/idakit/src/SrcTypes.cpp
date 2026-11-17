#include "idakit/SrcTypes.hpp"

#include "idakit/Database.hpp"
#include "idakit/ExeVtables.hpp"
#include "idakit/SrcEdit.hpp"

#include "IdaSupport.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <regex>

namespace idakit
{

namespace
{

// Hex-Rays' names for data of a known size and unknown type
bool untypedName(const std::string& name)
{
	static const std::set<std::string> names = {"_BYTE", "_WORD",  "_DWORD", "_QWORD", "_OWORD", "_TBYTE",
												 "_BOOL1", "_BOOL2", "_BOOL4", "_BOOL8", "_UNKNOWN"};
	return names.count(name) != 0;
}

// an integer at its size; Hex-Rays' untyped data of an integer's size (`_BYTE`, `_WORD`, `_DWORD`, `_QWORD`) is the
// unsigned integer of that size
std::optional<std::string> integer(const tinfo_t& t)
{
	size_t size = t.get_size();
	type_t full = t.get_realtype();
	bool untyped = full == BT_UNK_BYTE || full == BT_UNK_WORD || full == BT_UNK_DWORD || full == BT_UNK_QWORD;
	bool isSigned = !t.is_unsigned() && !untyped;
	// IDA's is_char() takes `signed char` for `char` too: only BTMT_CHAR is plain char
	if (size == 1 && get_full_type(full) == (BT_INT8 | BTMT_CHAR))
		return "builtin:char";
	switch (size)
	{
	case 1:
		return isSigned ? "builtin:signed char" : "builtin:unsigned char";
	case 2:
		return isSigned ? "builtin:short" : "builtin:unsigned short";
	case 4:
		return isSigned ? "builtin:int" : "builtin:unsigned int";
	case 8:
		return isSigned ? "builtin:long long" : "builtin:unsigned long long";
	default:
		return std::nullopt;
	}
}

std::string finalName(const tinfo_t& t)
{
	qstring name;
	t.get_final_type_name(&name);
	return name.c_str();
}

bool unspellable(const std::string& name)
{
	return name.empty() || unnamedRecord(name);
}

// hdrgen.py's tidy: `BSSimpleList<TESForm *>` as `BSSimpleList<TESForm*>`; class keys and doubled blanks dropped
std::string tidy(std::string name)
{
	name = std::regex_replace(name, std::regex("\\b(?:class|struct|union|enum) "), "");
	name = std::regex_replace(name, std::regex("\\s+\\*"), "*");
	name = std::regex_replace(name, std::regex("\\s+&"), "&");
	name = std::regex_replace(name, std::regex(",\\s*"), ", ");
	return std::regex_replace(std::regex_replace(name, std::regex("\\s+"), " "), std::regex("^ | $"), "");
}

// the template arguments at the top level of the text
std::vector<std::string> splitTop(const std::string& text)
{
	std::vector<std::string> out;
	int depth = 0;
	size_t start = 0;
	auto trimmed = [](const std::string& s) {
		size_t first = s.find_first_not_of(' '), last = s.find_last_not_of(' ');
		return first == std::string::npos ? std::string() : s.substr(first, last - first + 1);
	};
	for (size_t k = 0; k < text.size(); ++k)
	{
		char c = text[k];
		depth += c == '<' || c == '(' || c == '[';
		depth -= c == '>' || c == ')' || c == ']';
		if (c == ',' && depth == 0)
		{
			out.push_back(trimmed(text.substr(start, k - start)));
			start = k + 1;
		}
	}
	if (std::string tail = trimmed(text.substr(start)); !tail.empty())
		out.push_back(tail);
	return out;
}

// a template argument as the member types of IDA's struct for an instance name it: pointers dropped, `NiPointer<X>`
// as `XPtr`
std::string argumentKey(const std::string& argument)
{
	auto unpointed = [](std::string a) {
		while (!a.empty() && (a.back() == '*' || a.back() == ' '))
			a.pop_back();
		return a;
	};
	std::string a = unpointed(tidy(argument));
	std::smatch m;
	if (std::regex_match(a, m, std::regex("NiPointer<(.+)>")))
		return unpointed(tidy(m[1].str())) + "Ptr";
	return a;
}

// IDA's words for a type in its flattened struct names, each at its cost (the least kept)
using Words = std::map<std::string, int>;

void addWord(Words& words, const std::string& word, int cost)
{
	auto [at, added] = words.emplace(word, cost);
	if (!added)
		at->second = std::min(at->second, cost);
}

// a leading run of the arguments' words, joined by `_`, at their summed cost
Words runWords(const std::vector<Words>& arguments, size_t count)
{
	Words out{{"", 0}};
	for (size_t k = 0; k < count; ++k)
	{
		Words next;
		for (const auto& [prefix, prefixCost] : out)
			for (const auto& [word, cost] : arguments[k])
				addWord(next, prefix + (k ? "_" : "") + word, prefixCost + cost);
		out = std::move(next);
	}
	return out;
}

// the least cost at which the text from at is the words of arguments [k, end), joined by `_` or by nothing (`IntInt`)
std::optional<int> runCost(const std::string& text, size_t at, const std::vector<Words>& arguments, size_t k,
						   size_t end)
{
	std::optional<int> least;
	for (const auto& [word, cost] : arguments[k])
	{
		if (text.compare(at, word.size(), word) != 0)
			continue;
		size_t next = at + word.size();
		std::optional<int> rest;
		if (k + 1 == end)
			rest = next == text.size() ? std::optional<int>(0) : std::nullopt;
		else
		{
			rest = runCost(text, next, arguments, k + 1, end);
			if (std::optional<int> joined = next < text.size() && text[next] == '_'
												? runCost(text, next + 1, arguments, k + 1, end)
												: std::nullopt;
				joined && (!rest || *joined < *rest))
				rest = joined;
		}
		if (rest && (!least || cost + *rest < *least))
			least = cost + *rest;
	}
	return least;
}

// IDA's words for a src template argument, each at its cost (the pointer levels, consts and template arguments the
// word leaves out): `int` as Int, SInt32 or int; `unsigned int` as UInt32, UInt or uint; `T *` as TPtr or TP (0), and
// as T (1); `const T *` as TConstPtr (0) or TPtr (1); `NiPointer<T>` as TPtr; `A::B` as A_B or A__B; `X<A, B>` as X_A
// or X_A_B (X's own words; B left out costs 1), or X alone (1).
Words argumentWords(const std::string& argument)
{
	static const std::map<std::string, std::vector<std::string>> builtins = {
		{"int", {"Int", "SInt32", "int"}},
		{"unsigned int", {"UInt32", "UInt", "uint"}},
		{"short", {"SInt16", "Short"}},
		{"unsigned short", {"UInt16", "UShort", "WORD"}},
		{"char", {"Char", "char"}},
		{"signed char", {"SInt8"}},
		{"unsigned char", {"UInt8", "UChar", "BYTE"}},
		{"long", {"Long", "long"}},
		{"unsigned long", {"ULong", "ul"}},
		{"bool", {"Bool", "bool"}},
		{"float", {"Float", "float"}},
		{"double", {"Double", "double"}},
		{"void", {"void"}},
	};
	Words out;
	std::string a = tidy(argument);
	if (!a.empty() && a.back() == '*')
	{
		std::string pointee = tidy(a.substr(0, a.size() - 1));
		bool constPointee = pointee.compare(0, 6, "const ") == 0;
		if (constPointee)
			pointee.erase(0, 6);
		for (const auto& [word, cost] : argumentWords(pointee))
		{
			// a const pointee's words without Const leave it out
			int dropped = cost + (constPointee ? 1 : 0);
			addWord(out, word + "Ptr", dropped);
			addWord(out, word + "P", dropped);
			addWord(out, word, dropped + 1);
			if (constPointee)
				addWord(out, word + "ConstPtr", cost);
		}
		return out;
	}
	if (a.compare(0, 6, "const ") == 0)
	{
		for (const auto& [word, cost] : argumentWords(a.substr(6)))
			addWord(out, word, cost + 1);
		return out;
	}
	if (a.compare(0, 10, "NiPointer<") == 0 && a.back() == '>')
	{
		for (const auto& [word, cost] : argumentWords(a.substr(10, a.size() - 11)))
			addWord(out, word + "Ptr", cost);
	}
	else if (auto builtin = builtins.find(a); builtin != builtins.end())
	{
		for (const std::string& word : builtin->second)
			addWord(out, word, 0);
	}
	else if (size_t open = a.find('<'); open != std::string::npos && a.back() == '>')
	{
		std::vector<Words> arguments;
		for (const std::string& inner : splitTop(a.substr(open + 1, a.size() - open - 2)))
			arguments.push_back(argumentWords(inner));
		// IDA's names spell leading runs of a nested instance's arguments (BSSimpleArray_BSSimpleArray_NiPoint3); the
		// longer runs only while their words stay few
		std::vector<Words> runs;
		for (size_t count = 1, product = 1; count <= arguments.size(); ++count)
		{
			product *= arguments[count - 1].size();
			if (product > 256)
				break;
			runs.push_back(runWords(arguments, count));
		}
		for (const auto& [name, nameCost] : argumentWords(a.substr(0, open)))
		{
			addWord(out, name, nameCost + 1);
			for (size_t k = 0; k < runs.size(); ++k)
				for (const auto& [run, cost] : runs[k])
					addWord(out, name + "_" + run, nameCost + cost + static_cast<int>(arguments.size() - k - 1));
		}
	}
	else
	{
		for (const char* separator : {"_", "__"})
		{
			std::string word = a;
			for (size_t at = word.find("::"); at != std::string::npos; at = word.find("::", at))
				word.replace(at, 2, separator);
			addWord(out, word, 0);
		}
	}
	return out;
}

// `::` as `_`
std::string underscored(std::string text)
{
	for (size_t at = text.find("::"); at != std::string::npos; at = text.find("::", at))
		text.replace(at, 2, "_");
	return text;
}

std::string regexEscaped(const std::string& text)
{
	return std::regex_replace(text, std::regex(R"([.^$|()\[\]{}*+?\\])"), R"(\$&)");
}

// blanks dropped: IDA's struct names write `X<A*>` as well as the demangler's `X<A *>`
std::string blankless(std::string text)
{
	text.erase(std::remove(text.begin(), text.end(), ' '), text.end());
	return text;
}

// the C++ type with each name of a src typedef of a builtin (`uint32_t`) as the builtin (`unsigned int`)
std::string builtinsSpelled(const SrcModel& model, const std::string& text)
{
	std::string out;
	for (size_t k = 0; k < text.size();)
	{
		size_t end = k;
		while (end < text.size() && (std::isalnum(static_cast<unsigned char>(text[end])) || text[end] == '_'))
			++end;
		if (end == k)
		{
			out += text[k++];
			continue;
		}
		std::string word = text.substr(k, end - k);
		const SrcModel::Typedef* t = k && text[k - 1] == ':' ? nullptr : model.typedefNamed(word);
		out += t && t->target.compare(0, 8, "builtin:") == 0 ? t->target.substr(8) : word;
		k = end;
	}
	return out;
}

}  // namespace

SrcTypes::SrcTypes(const SrcModel& model, const std::string& aliases) : m_model(model), m_structNames(model)
{
	std::ifstream table(aliases);
	if (!table)
		throw Error("idakit: cannot read the IDA type aliases " + aliases);
	std::string line;
	std::getline(table, line);  // the header
	while (std::getline(table, line))
	{
		size_t tab = line.find('\t');
		size_t end = tab == std::string::npos ? tab : line.find('\t', tab + 1);
		if (tab != std::string::npos)
			m_aliases.emplace(line.substr(0, tab), line.substr(tab + 1, end - tab - 1));
	}
	for (const auto& [name, record] : model.records())
	{
		for (const std::string& s : idaSpellings(name))
			m_spellings.emplace(blankless(s), name);
		if (!record.spelled.empty())
			for (const std::string& s : idaSpellings(record.spelled))
				m_spellings.emplace(blankless(s), name);
	}
	// the records src declares and defines nowhere (TU-local ones, `Name @file`, have no IDA name); of two names of one
	// spelling, clang's (`, ` between template arguments) over the PDB's types' (none)
	auto blanks = [](const std::string& s) { return std::count(s.begin(), s.end(), ' '); };
	for (const auto& [key, type] : model.types())
		if (type.kind == "record" && !model.record(type.name) && type.name.find(" @") == std::string::npos)
			for (const std::string& s : idaSpellings(type.name))
				if (auto [at, added] = m_incomplete.emplace(blankless(s), type.name);
					!added && blanks(type.name) > blanks(at->second))
					at->second = type.name;
}

const SrcModel::Record* SrcTypes::recordOf(const tinfo_t& udt)
{
	std::string why;
	return record(udt, why);
}

const SrcModel::Record* SrcTypes::record(const tinfo_t& udt, std::string& why)
{
	return record(finalName(udt), &udt, why);
}

const SrcModel::Record* SrcTypes::recordOfClass(const std::string& cppName)
{
	auto lookup = [&](const std::string& name) -> const SrcModel::Record* {
		std::string why;
		tinfo_t t;
		if (t.get_named_type(get_idati(), name.c_str()) && t.is_udt())
			return record(t, why);
		return record(name, nullptr, why);
	};
	std::vector<std::string> spellings = idaSpellings(cppName);
	for (size_t k = 0, n = spellings.size(); k < n; ++k)
		for (const std::string& s : msvc9Variants(spellings[k]))
			spellings.push_back(s);
	for (const std::string& s : spellings)
		if (const SrcModel::Record* r = lookup(s))
			return r;
	// IDA's flattened names of an instance (`SettingCollectionMap_Setting`, then the template's bare name), least
	// cost first: a record src models flat, or the instance itself, never another instance of the template
	Words words = argumentWords(cppName);
	std::vector<std::pair<int, std::string>> ranked;
	for (const auto& [word, cost] : words)
		ranked.push_back({cost, word});
	std::sort(ranked.begin(), ranked.end());
	for (const auto& [cost, word] : ranked)
		if (const SrcModel::Record* r = lookup(word))
			if (r->pattern.empty() || ExeVtables::classKey(r->name) == ExeVtables::classKey(cppName))
				return r;
	return nullptr;
}

const SrcModel::Enum* SrcTypes::enumOf(const std::string& name) const
{
	if (const SrcModel::Enum* e = m_model.enumeration(name))
		return e;
	auto alias = m_aliases.find(name);
	return alias == m_aliases.end() ? nullptr : m_model.enumeration(alias->second);
}

const SrcModel::Record* SrcTypes::record(const std::string& name, const tinfo_t* udt, std::string& why)
{
	if (const SrcModel::Record* r = m_structNames.recordOf(name))
		return r;
	// the src type IDA's name stands for, and by what
	std::string target, by;
	if (auto alias = m_aliases.find(name); alias != m_aliases.end())
		target = alias->second, by = "ida_type_aliases.tsv";
	else if (auto spelled = m_spellings.find(blankless(name)); spelled != m_spellings.end())
		target = spelled->second, by = "its C++ name";
	else if (std::string instance = udt ? flattened(name, *udt) : ""; !instance.empty())
		target = instance, by = "its flattened name";
	else if (m_model.typedefNamed(name))
		target = name, by = "the src typedef of that name";
	else
	{
		why = "no src record for IDA's " + name;
		return nullptr;
	}
	const SrcModel::Record* r = recordSpelled(target);
	if (!r)
		why = "IDA's " + name + " is " + target + " (by " + by + "), which src does not declare";
	else if (udt && r->size != static_cast<int64_t>(udt->get_size()))
	{
		why = "IDA's " + name + " is " + r->name + " (by " + by + "), but has " + std::to_string(udt->get_size()) +
			  " bytes, src " + std::to_string(r->size);
		r = nullptr;
	}
	return r;
}

const SrcModel::Record* SrcTypes::recordSpelled(const std::string& spelling) const
{
	// the record as named or spelled (or with its typedefs of builtins as the builtins: `ListBoxItem<uint32_t>` is
	// src's ListBoxItem<unsigned int>), or a typedef of it
	const SrcModel::Record* r = m_model.record(spelling);
	for (const std::string& s : {spelling, builtinsSpelled(m_model, spelling)})
		for (const std::string& ida : idaSpellings(s))
			if (auto found = m_spellings.find(blankless(ida)); found != m_spellings.end() && !r)
				r = m_model.record(found->second);
	if (const SrcModel::Typedef* t = r ? nullptr : m_model.typedefNamed(spelling))
		if (const SrcModel::Type* held = m_model.type(m_model.storage(t->target)); held && held->kind == "record")
			r = m_model.record(held->name);
	return r;
}

std::string SrcTypes::flattened(const std::string& name, const tinfo_t& udt)
{
	if (auto cached = m_flattened.find(name); cached != m_flattened.end())
		return cached->second;
	std::string templateName, suffix;
	if (name.size() > 6 && name.compare(name.size() - 6, 6, "_tList") == 0)
		templateName = "BSSimpleList", suffix = name.substr(0, name.size() - 6);
	else if (size_t underscore = name.find('_'); underscore != std::string::npos)
		templateName = name.substr(0, underscore), suffix = name.substr(underscore + 1);
	else
		templateName = name;
	// IDA's std:: is src's msvc9::
	if (templateName.compare(0, 5, "std::") == 0)
		templateName = "msvc9::" + templateName.substr(5);
	std::string wanted = underscored(suffix);
	// a template named without arguments: the instance whose arguments the struct's member types name
	std::string body;
	udt_type_data_t members;
	if (suffix.empty() && udt.get_udt_details(&members))
		for (const udm_t& m : members)
			body += text(m.type) + " " + m.name.c_str() + ";";
	// each instance the name fits: partial where it spells other than all of the instance's arguments up to its
	// defaulted ones (r.spelled), at the cost of its words
	struct Fit
	{
		const SrcModel::Record* record;
		bool partial;
		int cost;
	};
	std::vector<Fit> fits;
	for (const auto& [record, r] : m_model.records())
	{
		if (r.pattern != templateName || record.size() < templateName.size() + 2 || record.back() != '>')
			continue;
		size_t open = templateName.size();
		std::vector<std::string> arguments = splitTop(record.substr(open + 1, record.size() - open - 2));
		size_t spelled = arguments.size();
		if (const std::string& s = r.spelled; s.size() > open + 1 && s.back() == '>')
			spelled = splitTop(s.substr(open + 1, s.size() - open - 2)).size();
		std::optional<Fit> best;
		if (!suffix.empty())
		{
			// IDA's name spells a run of the arguments: the leading ones (`NiTArray_NiRefObjectPtr` for the PDB's
			// NiTArray<NiPointer<NiRefObject>, NiTNewInterface<...>>), or others (`NiTMapBase_IntInt` for
			// NiTMapBase<DFALL<NiTMapItem<int, int>>, int, int>)
			std::vector<Words> words;
			for (const std::string& a : arguments)
				words.push_back(argumentWords(a));
			for (size_t first = 0; first < arguments.size(); ++first)
				for (size_t end = first + 1; end <= arguments.size(); ++end)
					if (std::optional<int> cost = runCost(wanted, 0, words, first, end))
					{
						Fit fit{&r, first != 0 || end != spelled, *cost};
						if (!best || std::make_pair(fit.partial, fit.cost) < std::make_pair(best->partial, best->cost))
							best = fit;
					}
		}
		else if (!body.empty() && std::all_of(arguments.begin(), arguments.end(), [&](const std::string& a) {
					 return std::regex_search(body, std::regex("\\b" + regexEscaped(argumentKey(a)) + "\\b"));
				 }))
			best = Fit{&r, false, 0};
		if (best)
			fits.push_back(*best);
	}
	// the best fits at the struct's size (else at any: record() then reports the size)
	int64_t size = static_cast<int64_t>(udt.get_size());
	bool sized = std::any_of(fits.begin(), fits.end(), [&](const Fit& f) { return f.record->size == size; });
	if (sized)
		fits.erase(std::remove_if(fits.begin(), fits.end(), [&](const Fit& f) { return f.record->size != size; }),
				   fits.end());
	auto rank = [](const Fit& f) { return std::make_pair(f.partial, f.cost); };
	if (!fits.empty())
	{
		auto least = rank(*std::min_element(fits.begin(), fits.end(), [&](const Fit& a, const Fit& b) {
			return rank(a) < rank(b);
		}));
		fits.erase(std::remove_if(fits.begin(), fits.end(), [&](const Fit& f) { return rank(f) != least; }),
				   fits.end());
	}
	return m_flattened[name] = fits.size() == 1 ? fits.front().record->name : "";
}

std::optional<std::string> SrcTypes::keyOf(const tinfo_t& type, std::string& why)
{
	if (type.is_typedef())
	{
		qstring name;
		type.get_type_name(&name);
		std::string typedefName = name.c_str();
		if (untypedName(typedefName))
		{
			why = "IDA's untyped " + typedefName;
			return std::nullopt;
		}
		const SrcModel::Typedef* alias = m_model.typedefNamed(typedefName);
		std::optional<int64_t> size = alias ? m_model.sizeOf(alias->target) : std::nullopt;
		if (size && *size == static_cast<int64_t>(type.get_size()))
			return "typedef:" + typedefName;
		// IDA's wchar_t (a typedef of unsigned __int16) is src's char16_t (sync writes it for one)
		if (typedefName == "wchar_t" && type.get_size() == 2)
			return "builtin:char16_t";
		// a typedef src does not declare is the type it names
	}
	if (type.is_void())
		return "builtin:void";
	if (type.is_bool() && type.get_size() == 1)
		return "builtin:bool";
	if (type.is_floating())
	{
		if (type.get_size() == 4)
			return "builtin:float";
		if (type.get_size() == 8)
			return "builtin:double";
		why = "IDA's " + text(type);
		return std::nullopt;
	}
	if (type.is_enum())
	{
		std::string name = finalName(type);
		if (const SrcModel::Enum* e = enumOf(name);
			e && (e->name == name || m_model.sizeOf("enum:" + e->name) == static_cast<int64_t>(type.get_size())))
			return "enum:" + e->name;
		why = "no src enum " + name;
		return std::nullopt;
	}
	if (type.is_udt())
	{
		if (const SrcModel::Record* r = record(type, why))
			return "record:" + r->name;
		// IDA's union for a vector type of the compiler (`__m128`, `__m64`) is the compiler's typedef of the name
		// (xmmintrin.h's `typedef float __m128 __attribute__((__vector_size__(16)))`), at its size
		std::string name = finalName(type);
		if (const SrcModel::Typedef* t = m_model.typedefNamed(name))
			if (const SrcModel::Type* held = m_model.type(m_model.storage(t->target));
				held && held->kind == "vector" && m_model.sizeOf(t->target) == static_cast<int64_t>(type.get_size()))
				return "typedef:" + name;
		return std::nullopt;
	}
	if (type.is_ptr())
	{
		tinfo_t pointee = type.get_pointed_object();
		if (pointee.is_func())
		{
			// a pointer to a function: its convention (vararg: a variadic cdecl), result and parameters as keys
			func_type_data_t fd;
			if (!pointee.get_func_details(&fd))
			{
				why = "function pointer " + text(type);
				return std::nullopt;
			}
			Function f;
			f.cc = conventionName(fd.get_cc());
			if (f.cc == "vararg")
				f.cc = "cdecl", f.variadic = true;
			if (!conventionCode(f.cc))
			{
				why = "function pointer " + text(type);
				return std::nullopt;
			}
			std::optional<std::string> ret = keyOf(fd.rettype, why);
			if (!ret)
				return std::nullopt;
			f.ret = *ret;
			for (const funcarg_t& a : fd)
			{
				std::optional<std::string> param = keyOf(a.type, why);
				if (!param)
					return std::nullopt;
				f.params.push_back(*param);
			}
			return "ptr(" + functionKey(f) + ")";
		}
		std::optional<std::string> inner = keyOf(pointee, why);
		if (!inner)
		{
			// a pointer to a record src declares and defines nowhere: IDA's struct or forward declaration of its name
			// (sync's, for src's pointers to it) is that incomplete type, which src can hold only behind a pointer
			auto incomplete = pointee.is_udt() || pointee.is_forward_struct() || pointee.is_forward_union()
								  ? m_incomplete.find(blankless(finalName(pointee)))
								  : m_incomplete.end();
			if (incomplete == m_incomplete.end())
				return std::nullopt;
			inner = "record:" + incomplete->second;
		}
		return "ptr(" + (pointee.is_const() ? "const(" + *inner + ")" : *inner) + ")";
	}
	if (type.is_array())
	{
		std::optional<std::string> element = keyOf(type.get_array_element(), why);
		if (!element)
			return std::nullopt;
		int count = type.get_array_nelems();
		return "array(" + *element + "," + std::to_string(count > 0 ? count : -1) + ")";
	}
	if (type.is_integral())
		if (std::optional<std::string> key = integer(type))
			return key;
	why = "IDA's " + text(type);
	return std::nullopt;
}

SrcTypes::Parts SrcTypes::parts(const std::string& key) const
{
	Parts p;
	if (const SrcModel::Type* t = m_model.type(key))
	{
		p.kind = t->kind;
		p.of = t->of;
		p.name = t->name;
		p.count = t->count;
		if (p.kind == "builtin")
			p.name = key.substr(8);
		return p;
	}
	size_t colon = key.find(':'), paren = key.find('(');
	if (colon != std::string::npos && (paren == std::string::npos || colon < paren))
	{
		p.kind = key.substr(0, colon);
		p.name = key.substr(colon + 1);
		return p;
	}
	if (paren == std::string::npos || key.back() != ')')
		return p;
	p.kind = key.substr(0, paren);
	p.of = key.substr(paren + 1, key.size() - paren - 2);
	if (p.kind == "array")
	{
		int depth = 0;
		size_t comma = std::string::npos;
		for (size_t k = 0; k < p.of.size(); ++k)
		{
			char c = p.of[k];
			depth += c == '(' || c == '<';
			depth -= c == ')' || c == '>';
			if (c == ',' && depth == 0)
				comma = k;
		}
		if (comma != std::string::npos)
		{
			p.count = std::stoll(p.of.substr(comma + 1));
			p.of.resize(comma);
		}
	}
	return p;
}

std::optional<SrcTypes::Function> SrcTypes::function(const std::string& key)
{
	if (!key.starts_with("fn(") || key.back() != ')')
		return std::nullopt;
	// the fields at the top level: cc;ret;params;quals, and the params' commas
	std::vector<std::string> fields(1);
	std::vector<std::string> params;
	int depth = 0;
	for (size_t k = 3; k + 1 < key.size(); ++k)
	{
		char c = key[k];
		depth += c == '(' || c == '<';
		depth -= c == ')' || c == '>';
		if (depth == 0 && c == ';')
			fields.emplace_back();
		else if (depth == 0 && c == ',' && fields.size() == 3)
			params.push_back(fields.back()), fields.back().clear();
		else
			fields.back() += c;
	}
	if (fields.size() != 4)
		return std::nullopt;
	Function f;
	f.cc = fields[0];
	f.ret = fields[1];
	if (!fields[2].empty())
		params.push_back(fields[2]);
	if (!params.empty() && params.back() == "...")
		f.variadic = true, params.pop_back();
	f.params = std::move(params);
	f.isConst = fields[3] == "const";
	return f;
}

std::string SrcTypes::functionKey(const Function& f)
{
	std::string key = "fn(" + f.cc + ";" + f.ret + ";";
	for (size_t k = 0; k < f.params.size(); ++k)
		key += (k ? "," : "") + f.params[k];
	if (f.variadic)
		key += f.params.empty() ? "..." : ",...";
	return key + ";" + (f.isConst ? "const" : "") + ")";
}

std::optional<std::string> SrcTypes::spell(const std::string& key, const std::string& name) const
{
	return declarator(key, name);
}

std::string SrcTypes::fixedWidthKey(const std::string& key) const
{
	// SrcEdit's text form of the fixed widths (RULES: long as int32_t too)
	std::string (*fixedText)(std::string) = &idakit::fixedWidth;
	Parts p = parts(key);
	if (p.kind == "builtin")
	{
		std::string name = p.name == "long long" ? "__int64" : p.name == "unsigned long long" ? "unsigned __int64" : p.name;
		std::string fixed = fixedText(name);
		return fixed == p.name ? key : "typedef:" + fixed;
	}
	if (p.kind == "ptr" || p.kind == "lref" || p.kind == "rref" || p.kind == "const" || p.kind == "volatile")
		return p.kind + "(" + fixedWidthKey(p.of) + ")";
	if (p.kind == "array")
		return "array(" + fixedWidthKey(p.of) + "," + std::to_string(p.count) + ")";
	if (std::optional<Function> f = function(key))
	{
		f->ret = fixedWidthKey(f->ret);
		for (std::string& param : f->params)
			param = fixedWidthKey(param);
		return functionKey(*f);
	}
	return key;
}

// clang's names of the builtin integers and their fixed-width typedefs as src spells them (porting rules: every scalar
// fixed width), a longer name before any it starts with; char and long have none on every target and stay
static const std::pair<std::string_view, const char*> kFixedWidths[] = {
	{"unsigned long long", "uint64_t"}, {"unsigned short", "uint16_t"}, {"unsigned char", "uint8_t"},
	{"unsigned int", "uint32_t"},		{"signed char", "int8_t"},		{"long long", "int64_t"},
	{"short", "int16_t"},				{"int", "int32_t"}};

// a builtin's fixed-width spelling; nullptr for one with none
static const char* fixedWidthName(std::string_view name)
{
	for (const auto& [clang, fixed] : kFixedWidths)
		if (name == clang)
			return fixed;
	return nullptr;
}

// a record's, enum's or typedef's name with the builtin integers among its template arguments spelled fixed width
// (`BSTask<int64_t>` for clang's `BSTask<long long>`): the same type
static std::string fixedWidthArguments(const std::string& name)
{
	auto word = [&](size_t k) { return k < name.size() && (std::isalnum(uint8_t(name[k])) || name[k] == '_'); };
	std::string out;
	int depth = 0;
	for (size_t k = 0; k < name.size();)
	{
		if (name[k] == '<')
			++depth;
		else if (name[k] == '>')
			--depth;
		const std::pair<std::string_view, const char*>* found = nullptr;
		if (depth > 0 && !(k > 0 && word(k - 1)))
			for (const auto& width : kFixedWidths)
				if (name.compare(k, width.first.size(), width.first) == 0 && !word(k + width.first.size()))
				{
					found = &width;
					break;
				}
		if (found)
		{
			out += found->second;
			k += found->first.size();
		}
		else
			out += name[k++];
	}
	return out;
}

// inner: the declarator so far (`p`, `* p`, `(*p)[4]`)
std::optional<std::string> SrcTypes::declarator(const std::string& key, const std::string& inner) const
{
	Parts p = parts(key);
	auto join = [&](const std::string& base) {
		if (inner.empty())
			return base;
		return base + (inner[0] == '*' || inner[0] == '&' ? "" : " ") + inner;
	};
	if (p.kind == "builtin")
	{
		const char* fixed = fixedWidthName(p.name);
		return join(fixed ? fixed : p.name);
	}
	if (p.kind == "record" || p.kind == "enum" || p.kind == "typedef")
	{
		if (unspellable(p.name))
			return std::nullopt;
		// a TU-local record's or enum's name (srcdb's `Name @file.cpp`) as its TU spells it
		std::string name = tuSpelling(p.name);
		return join(name.find('<') == std::string::npos ? name : fixedWidthArguments(name));
	}
	if (p.kind == "ptr" || p.kind == "lref" || p.kind == "rref")
	{
		std::string sigil = p.kind == "ptr" ? "*" : p.kind == "lref" ? "&" : "&&";
		std::string next = inner.empty() ? sigil
						 : inner[0] == '*' || inner[0] == '&' || parts(p.of).kind == "array" ? sigil + inner
																							  : sigil + " " + inner;
		return declarator(p.of, next);
	}
	if (p.kind == "const" || p.kind == "volatile")
	{
		Parts of = parts(p.of);
		if (of.kind == "ptr")
			return declarator(p.of, inner.empty() ? p.kind : p.kind + " " + inner);
		std::optional<std::string> rest = declarator(p.of, inner);
		return rest ? std::optional<std::string>(p.kind + " " + *rest) : std::nullopt;
	}
	// a pointer or reference to the array keeps its sigil inside: `T (*p)[N]`, `T (&)[N]`
	if (p.kind == "array")
	{
		std::string held = !inner.empty() && (inner[0] == '*' || inner[0] == '&') ? "(" + inner + ")" : inner;
		return declarator(p.of, held + "[" + (p.count >= 0 ? std::to_string(p.count) : "") + "]");
	}
	// a function, under a pointer (inner: `*`, `* p`): its convention as the seam's macro inside the parentheses,
	// `int (FNVD_THISCALL* p)(uint32_t)`
	if (std::optional<Function> f = function(key); f && !inner.empty())
	{
		static const std::map<std::string, std::string> macros = {{"cdecl", "FNVD_CDECL"},
																  {"stdcall", "FNVD_STDCALL"},
																  {"thiscall", "FNVD_THISCALL"},
																  {"fastcall", "FNVD_FASTCALL"}};
		auto macro = macros.find(f->cc);
		if (macro == macros.end())
			return std::nullopt;
		std::string params;
		for (const std::string& param : f->params)
		{
			std::optional<std::string> s = declarator(param, "");
			if (!s)
				return std::nullopt;
			params += (params.empty() ? "" : ", ") + *s;
		}
		if (f->variadic)
			params += params.empty() ? "..." : ", ...";
		return declarator(f->ret, "(" + macro->second + inner + ")(" + params + ")" + (f->isConst ? " const" : ""));
	}
	return std::nullopt;
}

std::string SrcTypes::canonical(const std::string& key) const
{
	if (std::optional<Function> f = function(key))
	{
		f->ret = canonical(f->ret);
		for (std::string& param : f->params)
			param = canonical(param);
		return functionKey(*f);
	}
	Parts p = parts(key);
	if (p.kind == "typedef")
	{
		const SrcModel::Typedef* alias = m_model.typedefNamed(p.name);
		return alias ? canonical(alias->target) : key;
	}
	if (p.kind == "const" || p.kind == "volatile")
		return canonical(p.of);
	if (p.kind == "ptr" || p.kind == "lref" || p.kind == "rref")
		return p.kind + "(" + canonical(p.of) + ")";
	if (p.kind == "array")
		return "array(" + canonical(p.of) + "," + std::to_string(p.count) + ")";
	if (key == "builtin:long")
		return "builtin:int";
	if (key == "builtin:unsigned long")
		return "builtin:unsigned int";
	return key;
}

void SrcTypes::headers(const std::string& key, std::set<std::string>& out) const
{
	if (std::optional<Function> f = function(key))
	{
		headers(f->ret, out);
		for (const std::string& param : f->params)
			headers(param, out);
		return;
	}
	Parts p = parts(key);
	std::string file;
	if (p.kind == "record")
	{
		if (const SrcModel::Record* r = m_model.record(p.name))
			file = r->file;
		// an instance's arguments: each name in them that is a src record, enum or typedef
		size_t open = p.name.find('<');
		for (size_t k = open; k != std::string::npos && k < p.name.size();)
		{
			size_t start = p.name.find_first_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_", k);
			if (start == std::string::npos)
				break;
			size_t end = p.name.find_first_not_of(
				"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_0123456789:", start);
			std::string name = p.name.substr(start, end - start);
			for (const char* kind : {"record:", "enum:", "typedef:"})
				if (m_model.type(kind + name))
					headers(kind + name, out);
			// a template instance among them (`NiPointer<NiRefObject>`): its own header (and its arguments')
			if (end != std::string::npos && p.name[end] == '<')
			{
				size_t close = end;
				for (int depth = 0; close < p.name.size(); ++close)
					if ((depth += p.name[close] == '<' ? 1 : p.name[close] == '>' ? -1 : 0) == 0)
						break;
				if (std::string instance = p.name.substr(start, close + 1 - start); m_model.record(instance))
					headers("record:" + instance, out);
			}
			k = end;
		}
	}
	else if (p.kind == "enum")
	{
		if (const SrcModel::Enum* e = m_model.enumeration(p.name))
			file = e->file;
	}
	else if (p.kind == "typedef")
	{
		if (const SrcModel::Typedef* t = m_model.typedefNamed(p.name))
			file = t->file;
	}
	else if (!p.of.empty())
		headers(p.of, out);
	if (file.rfind("src/", 0) == 0)
		out.insert(file.substr(4));
}

bool SrcTypes::derivesAtZero(const std::string& record, const std::string& base) const
{
	if (record == base)
		return true;
	const SrcModel::Record* r = m_model.record(record);
	if (!r)
		return false;
	for (const SrcModel::Base& b : r->bases)
		if (b.offset == 0 && !b.isVirtual && derivesAtZero(b.record, base))
			return true;
	return false;
}

std::string SrcTypes::bare(const std::string& key) const
{
	std::string out = key;
	for (Parts p = parts(out); p.kind == "typedef"; p = parts(out))
	{
		const SrcModel::Typedef* alias = m_model.typedefNamed(p.name);
		if (!alias)
			break;
		out = alias->target;
	}
	return out;
}

bool SrcTypes::constPointee(const std::string& key) const
{
	// the pointer's own const (`const T* const p`) aside
	Parts p = parts(bare(key));
	if (p.kind == "const")
		p = parts(bare(p.of));
	return (p.kind == "ptr" || p.kind == "lref") && parts(bare(p.of)).kind == "const";
}

const SrcModel::Record* SrcTypes::templateRecord(const std::string& key, const std::string& pattern,
												 bool pointer) const
{
	Parts p = parts(canonical(key));
	if (pointer)
		p = p.kind == "ptr" ? parts(p.of) : Parts{};
	const SrcModel::Record* r = p.kind == "record" ? m_model.record(p.name) : nullptr;
	return r && r->pattern == pattern ? r : nullptr;
}

const char* conventionName(cm_t cc)
{
	switch (cc & CM_CC_MASK)
	{
	case CM_CC_CDECL: return "cdecl";
	case CM_CC_STDCALL: return "stdcall";
	case CM_CC_FASTCALL: return "fastcall";
	case CM_CC_THISCALL: return "thiscall";
	case CM_CC_ELLIPSIS: return "vararg";
	case CM_CC_SPECIAL: return "usercall";
	case CM_CC_SPECIALP: return "userpurge";
	case CM_CC_SPECIALE: return "usercall_vararg";
	case CM_CC_PASCAL: return "pascal";
	case CM_CC_VOIDARG: return "voidarg";
	default: return "unknown";
	}
}

std::optional<callcnv_t> conventionCode(const std::string& name)
{
	for (callcnv_t code : {CM_CC_CDECL, CM_CC_STDCALL, CM_CC_THISCALL, CM_CC_FASTCALL})
		if (name == conventionName(code))
			return code;
	return std::nullopt;
}

}  // namespace idakit
