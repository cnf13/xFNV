// The PDB's types as src's (DeclarePdb.hpp).

#include "DeclarePdb.hpp"

#include "idakit/Database.hpp"
#include "idakit/SrcEdit.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>

namespace idakit
{

std::string comparedName(const std::string& name)
{
	static const std::regex stdScope(R"((^|[^A-Za-z0-9_:])std::)"), eastConst(R"(([A-Za-z_][A-Za-z0-9_:]*) const\b)");
	static const std::regex elaborated(R"(\b(enum|struct|class|union) +)");
	static const std::regex longLong(R"(\blong +long\b|\b__int64\b)"), longDouble(R"(\blong +double\b)");
	static const std::regex plainLong(R"(\blong\b)"), longLongMark("\x01"), longDoubleMark("\x02");
	std::string out = std::regex_replace(name, stdScope, "$1msvc9::"), before;
	while (before != out)
		before = out, out = std::regex_replace(out, eastConst, "const $1");
	out = std::regex_replace(out, elaborated, "");
	if (out.find("long") != std::string::npos || out.find("__int64") != std::string::npos)
	{
		out = std::regex_replace(out, longLong, "\x01");
		out = std::regex_replace(out, longDouble, "\x02");
		out = std::regex_replace(out, plainLong, "int");
		out = std::regex_replace(out, longLongMark, "long long");
		out = std::regex_replace(out, longDoubleMark, "long double");
	}
	out.erase(std::remove(out.begin(), out.end(), ' '), out.end());
	return out;
}

const std::regex& platformPattern()
{
	static const std::regex platform(R"((record|enum):((NiXenon|Xenon|XGraphics|hkPadSpu<|D3D[A-Z][a-z])[^,();]*))");
	return platform;
}

std::string cell(std::string text)
{
	std::replace(text.begin(), text.end(), '\t', ' ');
	std::replace(text.begin(), text.end(), '\n', ' ');
	return text;
}

std::vector<std::string> cells(const std::string& line)
{
	std::vector<std::string> out;
	std::istringstream in(line);
	for (std::string cell; std::getline(in, cell, '\t');)
		out.push_back(cell);
	if (!line.empty() && line.back() == '\t')
		out.emplace_back();
	return out;
}

std::string escaped(const std::string& text)
{
	std::string out;
	for (char c : text)
		out += c == '\\' ? "\\\\" : c == '\t' ? "\\t" : c == '\n' ? "\\n" : std::string(1, c);
	return out;
}

std::string unescaped(const std::string& text)
{
	std::string out;
	for (size_t k = 0; k < text.size(); ++k)
		if (text[k] == '\\' && k + 1 < text.size())
		{
			char c = text[++k];
			out += c == 't' ? '\t' : c == 'n' ? '\n' : c;
		}
		else
			out += text[k];
	return out;
}

std::string readFile(const std::string& path)
{
	std::ifstream in(path, std::ios::binary);
	if (!in)
		throw Error("declare --retype: cannot read " + path);
	std::ostringstream s;
	s << in.rdbuf();
	return s.str();
}

bool identifierChar(char c)
{
	return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

std::string trim(const std::string& s)
{
	size_t b = s.find_first_not_of(" \t\n"), e = s.find_last_not_of(" \t\n");
	return b == std::string::npos ? "" : s.substr(b, e - b + 1);
}

std::string commentBlock(const std::string& line, const std::string& indent)
{
	std::string comment = trim(line), text;
	size_t keepTo = comment.find("; the exe ");
	keepTo = keepTo == std::string::npos ? 0 : keepTo + 10;
	size_t limit = 120 - std::min<size_t>(100, indent.size() + 3 * std::count(indent.begin(), indent.end(), '\t'));
	while (comment.size() > limit)
	{
		size_t space = comment.rfind(' ', limit);
		if (space == std::string::npos || space < keepTo)
			space = comment.find(' ', std::max(keepTo, limit));
		if (space == std::string::npos)
			break;
		text += indent + comment.substr(0, space) + "\n";
		comment = "// " + comment.substr(space + 1);
		keepTo = 0;
	}
	return text + indent + comment + "\n";
}

Text::Text(std::string t) : text(std::move(t))
{
	starts.push_back(0);
	for (size_t k = 0; k < text.size(); ++k)
		if (text[k] == '\n')
			starts.push_back(k + 1);
}

int64_t Text::lineOf(size_t offset) const
{
	return std::upper_bound(starts.begin(), starts.end(), offset) - starts.begin();
}

std::string Text::line(int64_t number) const
{
	size_t begin = at(number), end = text.find('\n', begin);
	return text.substr(begin, (end == std::string::npos ? text.size() : end) - begin);
}

std::vector<std::string> Text::lines() const
{
	std::vector<std::string> out;
	for (size_t k = 1; k <= starts.size(); ++k)
		out.push_back(line(int64_t(k)));
	return out;
}

Text& SrcChanges::text(const std::string& file)
{
	auto found = m_texts.find(file);
	if (found == m_texts.end())
		found = m_texts.emplace(file, Text(readFile(file))).first;
	return found->second;
}

void SrcChanges::write(bool apply, std::ostream& applied, const std::map<std::string, std::string>& families,
					   std::map<std::string, std::vector<std::string>>& perFile)
{
	for (auto& [file, changes] : m_changes)
	{
		Text& t = text(file);
		std::sort(changes.begin(), changes.end(), [](const Change& a, const Change& b) {
			return a.offset != b.offset ? a.offset > b.offset : a.comment < b.comment;
		});
		for (size_t i = 1; i < changes.size(); ++i)
			if (changes[i].offset + changes[i].length > changes[i - 1].offset)
				throw Error("declare --retype: overlapping changes in " + file);
		std::string out = t.text;
		for (const Change& c : changes)
			out.replace(c.offset, c.length, c.text);
		// where each change landed: the original offset moved by the changes before it
		std::vector<Change> ascending(changes.rbegin(), changes.rend());
		int64_t shift = 0;
		Text after(out);
		std::set<std::string> rows;
		for (const Change& c : ascending)
		{
			size_t at = size_t(int64_t(c.offset) + shift);
			std::string old = t.text.substr(c.offset, c.length);
			auto family = families.find(c.row);
			applied << file << '\t' << after.lineOf(at) << '\t' << escaped(old) << '\t' << escaped(c.text) << '\t'
					<< c.row << '\t' << (family == families.end() ? "" : family->second) << '\n';
			shift += int64_t(c.text.size()) - int64_t(c.length);
			rows.insert(c.row);
		}
		perFile[file].assign(rows.begin(), rows.end());
		if (apply)
		{
			std::ofstream o(file, std::ios::binary | std::ios::trunc);
			o << out;
		}
	}
}

PdbTypes::PdbTypes(const SrcModel& model, SrcTypes& types) : m_model(model), m_types(types)
{
	for (const auto& [name, record] : model.records())
	{
		m_records.emplace(comparedName(name), name);
		if (!record.spelled.empty())
			m_records.emplace(comparedName(record.spelled), name);
	}
	for (const auto& [ea, refs] : model.refs())
		for (const SrcModel::Ref& r : refs)
		{
			if (r.function.empty())
				continue;
			bool declares = r.kind == "body" || r.kind == "constructor" || r.kind == "destructor" || r.kind == "address" ||
							r.kind == "fold";
			if (r.kind == "forwarder")
			{
				const std::vector<uint64_t>& own = model.bodyAddresses(r);
				declares = own.empty() || std::find(own.begin(), own.end(), ea) != own.end();
			}
			if (!declares)
				continue;
			m_address.emplace(r.function, ea);
			std::vector<std::string>& at = m_declaring[ea];
			if (std::find(at.begin(), at.end(), r.function) == at.end())
				at.push_back(r.function);
		}
}

std::optional<std::string> PdbTypes::toSrc(const std::string& key, std::string& missing) const
{
	SrcTypes::Parts p = m_types.parts(key);
	if (p.kind == "builtin")
		return m_types.fixedWidthKey(key);
	if (p.kind == "ptr" || p.kind == "lref" || p.kind == "rref" || p.kind == "const" || p.kind == "volatile")
	{
		std::optional<std::string> of = toSrc(p.of, missing);
		return of ? std::optional<std::string>(p.kind + "(" + *of + ")") : std::nullopt;
	}
	if (p.kind == "array")
	{
		std::optional<std::string> of = toSrc(p.of, missing);
		return of ? std::optional<std::string>("array(" + *of + "," + std::to_string(p.count) + ")") : std::nullopt;
	}
	if (p.kind == "record")
	{
		if (m_model.record(p.name))
			return key;
		if (auto found = m_records.find(comparedName(p.name)); found != m_records.end())
			return "record:" + found->second;
		if (auto alias = m_types.aliases().find(p.name); alias != m_types.aliases().end())
			if (const SrcModel::Record* r = m_types.recordSpelled(alias->second))
				return "record:" + r->name;
		// a PDB record src names by a typedef (D3DXMATRIX)
		if (m_model.typedefNamed(p.name))
			return "typedef:" + p.name;
		missing = p.name;
		return std::nullopt;
	}
	if (p.kind == "enum")
	{
		if (const SrcModel::Enum* e = m_types.enumOf(p.name))
			return "enum:" + e->name;
		if (m_model.typedefNamed(p.name))
			return "typedef:" + p.name;
		missing = p.name;
		return std::nullopt;
	}
	if (p.kind == "typedef" && m_model.typedefNamed(p.name))
		return key;
	if (std::optional<SrcTypes::Function> f = SrcTypes::function(key))
	{
		std::optional<std::string> ret = toSrc(f->ret, missing);
		if (!ret)
			return std::nullopt;
		f->ret = *ret;
		for (std::string& param : f->params)
		{
			std::optional<std::string> s = toSrc(param, missing);
			if (!s)
				return std::nullopt;
			param = *s;
		}
		return SrcTypes::functionKey(*f);
	}
	missing = key;
	return std::nullopt;
}

std::string PdbTypes::normal(const std::string& key, int depth) const
{
	if (depth > 64)
		return key;
	SrcTypes::Parts p = m_types.parts(key);
	if (p.kind == "typedef")
	{
		const SrcModel::Typedef* t = m_model.typedefNamed(p.name);
		return t ? normal(t->target, depth + 1) : key;
	}
	if (p.kind == "builtin")
	{
		static const std::set<std::string> integers = {"signed char", "unsigned char", "short", "unsigned short",
													   "int", "unsigned int", "long", "unsigned long", "long long",
													   "unsigned long long"};
		const SrcModel::Type* t = m_model.type(key);
		if (t && integers.count(p.name) && t->size > 0)
			return "int" + std::to_string(t->size) + (t->isSigned ? "s" : "u");
		return key;
	}
	if (p.kind == "ptr" || p.kind == "lref" || p.kind == "rref" || p.kind == "const" || p.kind == "volatile")
		return p.kind + "(" + normal(p.of, depth + 1) + ")";
	if (p.kind == "array")
		return "array(" + normal(p.of, depth + 1) + "," + std::to_string(p.count) + ")";
	if (std::optional<SrcTypes::Function> f = SrcTypes::function(key))
	{
		std::string out = "fn(" + f->cc + ";" + normal(f->ret, depth + 1) + ";";
		for (const std::string& param : f->params)
			out += normal(param, depth + 1) + ",";
		return out + (f->variadic ? "...)" : ")");
	}
	if (p.kind == "record" || p.kind == "enum")
		return p.kind + ":" + comparedName(p.name);
	return key;
}

std::string PdbTypes::spelled(const std::string& key, const std::string& name) const
{
	std::optional<std::string> s = m_types.spell(m_types.fixedWidthKey(key), name);
	return s ? srcSpelling(*s) : key;
}

std::string PdbTypes::resolved(const std::string& key) const
{
	std::string k = m_types.bare(key);
	for (SrcTypes::Parts p = m_types.parts(k); p.kind == "const" || p.kind == "volatile"; p = m_types.parts(k))
		k = m_types.bare(p.of);
	return k;
}

std::string PdbTypes::kindOf(const std::string& key) const
{
	std::string k = resolved(key);
	SrcTypes::Parts p = m_types.parts(k);
	if (p.kind == "builtin")
	{
		if (p.name == "bool" || p.name == "void" || p.name == "char" || p.name == "wchar_t")
			return p.name;
		if (p.name == "float" || p.name == "double" || p.name == "long double")
			return "float";
		return "int";
	}
	if (p.kind == "lref" || p.kind == "rref")
		return "ref";
	return p.kind;
}

std::optional<int64_t> PdbTypes::size(const std::string& key) const
{
	std::string k = kindOf(key);
	if (k == "ptr" || k == "ref")
		return 4;
	if (k == "array")
	{
		SrcTypes::Parts p = m_types.parts(resolved(key));
		std::optional<int64_t> element = size(p.of);
		return element && p.count >= 0 ? std::optional<int64_t>(*element * p.count) : std::nullopt;
	}
	if (std::optional<int64_t> s = m_model.sizeOf(resolved(key)))
		return s;
	// a key the model has no node for: its record's or enum's size
	SrcTypes::Parts p = m_types.parts(resolved(key));
	if (p.kind == "record")
		if (const SrcModel::Record* r = m_model.record(p.name))
			return r->size;
	if (p.kind == "enum")
		if (const SrcModel::Enum* e = m_model.enumeration(p.name))
			return m_model.sizeOf(e->underlying);
	return std::nullopt;
}

std::optional<int64_t> PdbTypes::align(const std::string& key) const
{
	std::string k = kindOf(key);
	if (k == "ptr" || k == "ref")
		return 4;
	SrcTypes::Parts p = m_types.parts(resolved(key));
	if (k == "array")
		return align(p.of);
	if (std::optional<int64_t> a = m_model.alignOf(resolved(key)))
		return a;
	if (p.kind == "record")
		if (const SrcModel::Record* r = m_model.record(p.name); r && r->align)
			return r->align;
	if (p.kind == "enum")
		if (const SrcModel::Enum* e = m_model.enumeration(p.name))
			return m_model.alignOf(e->underlying);
	return std::nullopt;
}

bool PdbTypes::derives(const std::string& record, const std::string& base) const
{
	if (record == base)
		return true;
	const SrcModel::Record* r = m_model.record(record);
	if (!r)
		return false;
	for (const SrcModel::Base& b : r->bases)
		if (derives(b.record, base))
			return true;
	return false;
}

std::set<std::string> Includes::closure(const std::string& file)
{
	std::set<std::string> seen;
	std::vector<std::string> work{file};
	while (!work.empty())
	{
		std::string f = work.back();
		work.pop_back();
		for (const std::string& next : direct(f))
			if (seen.insert(next).second)
				work.push_back(next);
	}
	return seen;
}

const std::vector<std::string>& Includes::direct(const std::string& file)
{
	auto found = m_direct.find(file);
	if (found != m_direct.end())
		return found->second;
	std::vector<std::string> out;
	static const std::regex include(R"re(^\s*#\s*include\s*"([^"]+)")re");
	std::ifstream in(file);
	for (std::string line; std::getline(in, line);)
	{
		std::smatch m;
		if (!std::regex_search(line, m, include))
			continue;
		for (const std::filesystem::path& p :
			 {std::filesystem::path("src") / m[1].str(), std::filesystem::path(file).parent_path() / m[1].str()})
			if (std::filesystem::exists(p))
			{
				out.push_back(p.lexically_normal().string());
				break;
			}
	}
	return m_direct.emplace(file, std::move(out)).first->second;
}

void typeNeeds(const SrcModel& model, const SrcTypes& types, const std::string& key, bool complete,
			   std::set<std::string>& headers, std::set<const SrcModel::Record*>& forwards)
{
	if (std::optional<SrcTypes::Function> f = SrcTypes::function(key))
	{
		typeNeeds(model, types, f->ret, false, headers, forwards);
		for (const std::string& p : f->params)
			typeNeeds(model, types, p, false, headers, forwards);
		return;
	}
	SrcTypes::Parts p = types.parts(key);
	if (p.kind == "ptr" || p.kind == "lref" || p.kind == "rref")
		return typeNeeds(model, types, p.of, false, headers, forwards);
	if (p.kind == "const" || p.kind == "volatile")
		return typeNeeds(model, types, p.of, complete, headers, forwards);
	if (p.kind == "array")
		return typeNeeds(model, types, p.of, true, headers, forwards);
	if (p.kind == "record" && !complete && p.name.find("::") == std::string::npos &&
		p.name.find('<') == std::string::npos)
		if (const SrcModel::Record* r = model.record(p.name); r && !r->file.empty())
		{
			forwards.insert(r);
			return;
		}
	if (p.kind == "record" || p.kind == "enum" || p.kind == "typedef")
		types.headers(key, headers);
}

std::optional<Change> declarationNeeds(const SrcModel& model, const SrcTypes& types, Includes& includes,
									   const std::string& file, const Text& t, const std::string& key, bool complete,
									   std::map<std::string, std::set<std::string>>& inserted, const std::string& row)
{
	std::set<std::string> headers;
	std::set<const SrcModel::Record*> forwards;
	typeNeeds(model, types, key, complete, headers, forwards);
	std::set<std::string> visible = includes.closure(file);
	visible.insert(file);
	std::string text;
	for (const std::string& h : headers)
	{
		std::string path = "src/" + h;
		if (visible.count(path) || !inserted[file].insert(includeLine(h)).second)
			continue;
		if (includes.closure(path).count(file))
			throw Error(h + " includes " + file + ": the new type's header would be a cycle");
		text += includeLine(h) + "\n";
	}
	for (const SrcModel::Record* fwd : forwards)
	{
		std::string line =
			(fwd->tag == "union" ? "union " : fwd->tag == "struct" ? "struct " : "class ") + fwd->name + ";";
		bool declared = visible.count(fwd->file) || t.text.find("class " + fwd->name + ";") != std::string::npos ||
						t.text.find("struct " + fwd->name + ";") != std::string::npos;
		if (declared || !inserted[file].insert(line).second)
			continue;
		text += line + "\n";
	}
	if (text.empty())
		return std::nullopt;
	// after the file's first run of #include lines, else after its #pragma once, else at its top
	std::vector<std::string> lines = t.lines();
	int64_t at = 1, first = 0;
	for (int64_t l = 1; l <= int64_t(lines.size()) && !first; ++l)
		if (trim(lines[l - 1]).starts_with("#include"))
			first = l;
	if (first)
		for (at = first; at <= int64_t(lines.size()) && trim(lines[at - 1]).starts_with("#include");)
			++at;
	else
		for (int64_t l = 1; l <= int64_t(lines.size()); ++l)
			if (trim(lines[l - 1]).starts_with("#pragma once"))
			{
				at = l + 1;
				break;
			}
	return Change{t.at(at), 0, text, row};
}

}  // namespace idakit
