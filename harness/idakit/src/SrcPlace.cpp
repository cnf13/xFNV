#include "idakit/SrcPlace.hpp"

#include "idakit/Database.hpp"
#include "idakit/ExeVtables.hpp"
#include "IdaSupport.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <regex>
#include <set>

namespace idakit
{

namespace
{

using TypeName = SpecializationHomes::TypeName;

bool headerFile(const std::string& path)
{
	return path.ends_with(".h") || path.ends_with(".hpp") || path.ends_with(".inl");
}

std::string stem(const std::string& path)
{
	size_t dot = path.rfind('.');
	size_t slash = path.rfind('/');
	return dot == std::string::npos || (slash != std::string::npos && dot < slash) ? path : path.substr(0, dot);
}

// The text's name without its cv, pointer and reference parts and a leading class-key.
std::string bareName(std::string text)
{
	static const std::regex decoration(R"(\b(?:const|volatile|class|struct|union|enum)\b|[*&])");
	text = std::regex_replace(text, decoration, "");
	size_t first = text.find_first_not_of(' '), last = text.find_last_not_of(' ');
	return first == std::string::npos ? "" : text.substr(first, last - first + 1);
}

// The components of a qualified name, split at its top-level `::`.
std::vector<std::string> components(const std::string& name)
{
	std::vector<std::string> out;
	int depth = 0;
	size_t start = 0;
	for (size_t k = 0; k < name.size(); ++k)
	{
		char c = name[k];
		depth += c == '<' || c == '(' ? 1 : c == '>' || c == ')' ? -1 : 0;
		if (depth == 0 && c == ':' && k + 1 < name.size() && name[k + 1] == ':')
		{
			out.push_back(name.substr(start, k - start));
			start = k + 2;
			++k;
		}
	}
	out.push_back(name.substr(start));
	return out;
}

// The top-level template arguments of every `<...>` in a name (`A<X, B<Y>>::C<Z>`: X, B<Y>, Z) and theirs in turn,
// each bare; complete: the name is held by value, so are its arguments that are no pointer or reference.
void templateArguments(const std::string& name, bool complete, std::vector<TypeName>& out)
{
	for (size_t k = 0; k < name.size(); ++k)
	{
		// a template's `<`, not an operator's
		bool word = k > 0 && (std::isalnum(static_cast<unsigned char>(name[k - 1])) || name[k - 1] == '_');
		if (name[k] != '<' || !word || (k >= 8 && name.compare(k - 8, 8, "operator") == 0))
			continue;
		int depth = 0;
		size_t start = k + 1;
		for (size_t j = k; j < name.size(); ++j)
		{
			char c = name[j];
			depth += c == '<' || c == '(' ? 1 : c == '>' || c == ')' ? -1 : 0;
			if ((c == ',' && depth == 1) || depth == 0)
			{
				std::string text = name.substr(start, j - start);
				std::string argument = bareName(text);
				bool held = complete && text.find_first_of("*&(") == std::string::npos;
				if (!argument.empty())
				{
					out.push_back({argument, held});
					templateArguments(argument, held, out);
				}
				start = j + 1;
			}
			if (depth == 0)
			{
				k = j;
				break;
			}
		}
	}
}

// The names after `record:`, `enum:` and `typedef:` in a type key, with their template arguments: a declaration
// needs none of them complete.
void keyNames(const std::string& key, std::vector<TypeName>& out)
{
	for (const char* kind : {"record:", "enum:", "typedef:"})
		for (size_t at = key.find(kind); at != std::string::npos; at = key.find(kind, at + 1))
		{
			size_t start = at + std::char_traits<char>::length(kind), end = start;
			for (int depth = 0; end < key.size(); ++end)
			{
				char c = key[end];
				if (depth == 0 && (c == ',' || c == ')' || c == ';'))
					break;
				depth += c == '<' ? 1 : c == '>' ? -1 : 0;
			}
			std::string name = key.substr(start, end - start);
			out.push_back({name, false});
			templateArguments(name, false, out);
		}
}

}  // namespace

bool IncludeGraph::reaches(const std::string& file, const std::string& target)
{
	std::set<std::string> seen{file};
	std::vector<std::string> pending{file};
	while (!pending.empty())
	{
		std::string at = pending.back();
		pending.pop_back();
		if (at == target)
			return true;
		auto cached = m_includes.find(at);
		if (cached == m_includes.end())
		{
			std::vector<std::string> list;
			std::ifstream in(at);
			static const std::regex include(R"(^\s*#\s*include\s*"([^"]+)\")");
			std::smatch m;
			for (std::string line; std::getline(in, line);)
				if (std::regex_search(line, m, include))
				{
					std::string name = m[1].str();
					std::string sibling = at.substr(0, at.rfind('/') + 1) + name;
					list.push_back(std::filesystem::exists("src/" + name) ? "src/" + name : sibling);
				}
			cached = m_includes.emplace(at, std::move(list)).first;
		}
		std::vector<std::string> next = cached->second;
		if (auto added = m_added.find(at); added != m_added.end())
			next.insert(next.end(), added->second.begin(), added->second.end());
		for (const std::string& n : next)
			if (seen.insert(n).second)
				pending.push_back(n);
	}
	return false;
}

PdbSources::PdbSources(const std::string& path)
{
	std::ifstream table(path);
	if (!table)
		throw Error("idakit: cannot read the PDB function sources " + path);
	for (std::string line; std::getline(table, line);)
		if (size_t tab = line.find('\t'); tab != std::string::npos)
			m_sources.emplace(ExeVtables::classKey(line.substr(0, tab)), line.substr(tab + 1));
}

std::string PdbSources::source(const std::string& name) const
{
	auto found = m_sources.find(ExeVtables::classKey(name));
	if (found == m_sources.end() || !found->second.ends_with(".cpp"))
		return "";
	std::string path = "src/" + found->second;
	return std::filesystem::exists(path) ? path : "";
}

std::string definitionSource(const SrcModel& model, const PdbSources& sources, const SrcModel::Function& f,
							 const std::string& header)
{
	if (std::string source = sources.source(f.name); !source.empty())
		return source;
	if (std::string source = f.cls.empty() ? "" : model.classSource(f.cls); !source.empty())
		return source;
	return headerFile(header) ? stem(header) + ".cpp" : header;
}

std::string includedHeader(const std::string& file)
{
	if (!file.ends_with(".inl"))
		return file;
	std::string header = stem(file) + ".h";
	return std::filesystem::exists(header) ? header : file;
}

std::vector<std::pair<std::string, int64_t>> namespacesAt(const std::vector<std::string>& lines, int64_t line)
{
	static const std::regex opener(R"(^\s*namespace\s+([A-Za-z_][A-Za-z_0-9:]*)\s*(\{)?)");
	struct Open
	{
		std::string name;
		int64_t line;
		int depth;  // the brace depth inside it
	};
	std::vector<Open> open;
	int depth = 0;
	std::optional<std::pair<std::string, int64_t>> pending;  // a `namespace a` whose `{` is still to come
	for (int64_t number = 1; number < line && number <= static_cast<int64_t>(lines.size()); ++number)
	{
		std::string text = lines[number - 1].substr(0, lines[number - 1].find("//"));
		std::smatch m;
		size_t from = 0;
		if (std::regex_search(text, m, opener))
		{
			pending = {m[1].str(), number};
			from = m.position(1) + m.length(1);
		}
		for (size_t k = from; k < text.size(); ++k)
		{
			if (text[k] == '{')
			{
				++depth;
				if (pending)
				{
					open.push_back({pending->first, pending->second, depth});
					pending.reset();
				}
			}
			else if (text[k] == '}')
			{
				if (!open.empty() && open.back().depth == depth)
					open.pop_back();
				--depth;
			}
			else if (text[k] == ';')
				pending.reset();
		}
	}
	std::vector<std::pair<std::string, int64_t>> out;
	for (const Open& o : open)
		out.emplace_back(o.name, o.line);
	return out;
}

SpecializationHomes::SpecializationHomes(const SrcModel& model, IncludeGraph& includes)
	: m_model(model), m_includes(includes)
{
	for (const auto& [name, record] : model.records())
		if (!record.pattern.empty() && record.file.rfind("src/", 0) == 0 && name.find('<') != std::string::npos)
			m_patterns.emplace(record.pattern, Place{record.file, record.endLine});
}

const std::vector<std::string>& SpecializationHomes::lines(const std::string& file)
{
	auto [cached, added] = m_lines.try_emplace(file);
	if (added)
	{
		std::ifstream in(file);
		for (std::string line; std::getline(in, line);)
			cached->second.push_back(line);
	}
	return cached->second;
}

int64_t SpecializationHomes::blockEnd(const std::string& file, int64_t line)
{
	const std::vector<std::string>& text = lines(file);
	int depth = 0;
	for (int64_t number = std::max<int64_t>(line, 1); number <= static_cast<int64_t>(text.size()); ++number)
	{
		const std::string& l = text[number - 1];
		size_t comment = l.find("//");
		for (size_t k = 0; k < std::min(l.size(), comment); ++k)
		{
			if (l[k] == '{')
				++depth;
			else if (l[k] == '}' && --depth == 0)
				return number;
			else if (l[k] == ';' && depth == 0)
				return number;
		}
	}
	return line;
}

const SrcModel::Record* SpecializationHomes::outermost(const std::string& name) const
{
	std::string prefix;
	for (const std::string& c : components(name))
	{
		prefix += (prefix.empty() ? "" : "::") + c;
		if (const SrcModel::Record* r = m_model.record(prefix))
			return r;
	}
	return nullptr;
}

std::string SpecializationHomes::namespaceOf(const std::string& name) const
{
	std::string prefix;
	for (const std::string& c : components(name))
	{
		std::string next = (prefix.empty() ? "" : prefix + "::") + c;
		if (c.find('<') != std::string::npos || m_model.record(next) || m_model.enumeration(next))
			break;
		prefix = next;
	}
	return prefix;
}

void SpecializationHomes::classify(const TypeName& type, const std::string& ownerName, std::vector<Place>& pinned,
								   std::vector<Place>& required, int depth)
{
	const std::string& name = type.name;
	if (name.empty() || name == ownerName || depth > 8)
		return;
	if (const SrcModel::Record* r = m_model.record(name))
	{
		const SrcModel::Record* outer = outermost(name);
		// a type local to a .cpp: that file is the only place that can name it
		if (!headerFile(r->file))
			pinned.push_back({r->file, r->endLine});
		else if (outer && outer != r)
			pinned.push_back({outer->file, outer->endLine});
		// a plain class is forward declared, unless the instance holds it
		else if (!r->pattern.empty() || name.find("::") != std::string::npos || type.complete)
			required.push_back({r->file, r->endLine});
		return;
	}
	if (const SrcModel::Enum* e = m_model.enumeration(name))
	{
		if (const SrcModel::Record* outer = outermost(scopeOf(name)))
			pinned.push_back({outer->file, outer->endLine});
		else if (!headerFile(e->file))
			pinned.push_back({e->file, blockEnd(e->file, e->line)});
		else
			required.push_back({e->file, blockEnd(e->file, e->line)});
		return;
	}
	if (const SrcModel::Typedef* t = m_model.typedefNamed(name))
	{
		// spelled through its target where the home does not see it
		if (t->file.rfind("src/", 0) == 0)
		{
			std::vector<TypeName> targets;
			keyNames(t->target, targets);
			for (TypeName& n : targets)
			{
				n.complete = n.complete || type.complete;
				classify(n, ownerName, pinned, required, depth + 1);
			}
		}
		return;
	}
	if (size_t open = name.find('<'); open != std::string::npos)
		if (auto found = m_patterns.find(name.substr(0, open)); found != m_patterns.end())
			required.push_back(found->second);
}

SpecializationHomes::Result SpecializationHomes::home(const Place& at, const std::string& scope,
													  const std::string& ownerName, const std::vector<TypeName>& names)
{
	if (at.file.rfind("src/", 0) != 0)
		return std::string("template not in src/: " + at.file);
	std::vector<Place> pinned, required;
	for (const TypeName& name : names)
		classify(name, ownerName, pinned, required, 0);
	auto visible = [](const Place& p) { return p.file.rfind("src/", 0) == 0; };
	// the template's header is the home unless a type it cannot see is pinned elsewhere: one nested in a class, or
	// one whose header includes the template's; a type it defines later moves the home past it
	Place chosen = at;
	std::vector<Place> away;
	for (const Place& p : pinned)
		if (p.file == at.file)
			chosen.after = std::max(chosen.after, p.after);
		else if (!m_includes.reaches(at.file, p.file))
			away.push_back(p);
	for (const Place& r : required)
		if (!visible(r))
			continue;
		else if (r.file == at.file)
			chosen.after = std::max(chosen.after, r.after);
		else if (!m_includes.reaches(at.file, r.file) && m_includes.reaches(r.file, at.file))
			away.push_back(r);
	std::vector<Place> sources = required;
	if (!away.empty())
	{
		std::set<std::string> files;
		for (const Place& p : away)
			files.insert(p.file);
		std::string header;
		for (const std::string& candidate : files)
			if (std::all_of(files.begin(), files.end(), [&](const std::string& other) {
					return other == candidate || m_includes.reaches(candidate, other);
				}))
				header = candidate;
		if (header.empty())
			return std::string("nested arguments in unrelated headers");
		chosen = {header, 0};
		for (const Place& p : away)
			if (p.file == header)
				chosen.after = std::max(chosen.after, p.after);
		sources.push_back(at);
	}
	if (chosen.after <= 0)
		return std::string("no end line for the template in " + chosen.file);
	// a home inside a namespace that does not enclose the template's is past that namespace
	std::vector<std::string> wanted = scope.empty() ? std::vector<std::string>{} : components(scope);
	std::vector<std::pair<std::string, int64_t>> open = namespacesAt(lines(chosen.file), chosen.after + 1);
	std::vector<std::string> enclosing;
	for (const auto& [name, line] : open)
	{
		for (const std::string& c : components(name))
			enclosing.push_back(c);
		if (enclosing.size() > wanted.size() || !std::equal(enclosing.begin(), enclosing.end(), wanted.begin()))
		{
			chosen.after = blockEnd(chosen.file, line);
			break;
		}
	}
	SpecializationHome out{chosen.file, chosen.after,
						   headerFile(chosen.file) ? stem(chosen.file) + ".cpp" : chosen.file, {}};
	// the headers of what it needs defined there (and of the template, for a home elsewhere)
	for (const Place& r : sources)
		if (visible(r) && r.file != chosen.file && headerFile(r.file) && !m_includes.reaches(chosen.file, r.file))
		{
			if (m_includes.reaches(r.file, chosen.file))
				return std::string("would include " + r.file + ", which includes " + chosen.file);
			if (std::find(out.includes.begin(), out.includes.end(), r.file) == out.includes.end())
				out.includes.push_back(r.file);
		}
	return out;
}

bool SpecializationHomes::holds(const SrcModel::Record& record, const std::string& name, int depth) const
{
	if (depth > 8)
		return false;
	auto inside = [&](const std::string& held) {
		if (held == name)
			return true;
		const SrcModel::Record* r = m_model.record(held);
		return r && r != &record && holds(*r, name, depth + 1);
	};
	for (const SrcModel::Base& b : record.bases)
		if (inside(b.record))
			return true;
	for (const SrcModel::Field& f : record.fields)
	{
		std::string key = f.type;
		for (int k = 0; k < 8; ++k)
		{
			if (key.starts_with("const(") || key.starts_with("volatile("))
				key = key.substr(key.find('(') + 1, key.size() - key.find('(') - 2);
			else if (key.starts_with("array("))
				key = key.substr(6, key.rfind(',') - 6);
			else if (key.starts_with("typedef:"))
			{
				const SrcModel::Typedef* t = m_model.typedefNamed(key.substr(8));
				if (!t)
					break;
				key = t->target;
			}
			else
				break;
		}
		if ((key.starts_with("record:") && inside(key.substr(7))) || (key.starts_with("enum:") && key.substr(5) == name))
			return true;
	}
	return false;
}

void SpecializationHomes::heldArguments(const SrcModel::Record& owner, std::vector<TypeName>& names) const
{
	if (owner.fields.empty() && owner.bases.empty())
		return;
	for (TypeName& n : names)
		if (n.complete && !holds(owner, n.name, 0))
			n.complete = false;
}

SpecializationHomes::Result SpecializationHomes::specializationHome(const SrcModel::Record& owner,
																	const std::vector<std::string>& keys)
{
	const SrcModel::Record* outer = outermost(owner.name);
	Place at = outer ? Place{outer->file, outer->endLine} : Place{owner.file, owner.endLine};
	std::vector<TypeName> names;
	templateArguments(owner.name, true, names);
	heldArguments(owner, names);
	for (const std::string& key : keys)
		keyNames(key, names);
	return home(at, namespaceOf(owner.name), owner.name, names);
}

SpecializationHomes::Result SpecializationHomes::specializationHome(const std::string& mangled)
{
	const SrcModel::Function* f = m_model.function(mangled);
	if (!f)
		return std::string("not in the model");
	std::vector<TypeName> names;
	keyNames(f->type, names);
	// a function template's arguments (a member function template's too)
	templateArguments(lastComponent(f->name), false, names);
	if (!f->cls.empty())
	{
		const SrcModel::Record* owner = m_model.record(f->cls);
		if (!owner)
			return std::string("class not in the model: " + f->cls);
		const SrcModel::Record* outer = outermost(owner->name);
		Place at = outer ? Place{outer->file, outer->endLine} : Place{owner->file, owner->endLine};
		templateArguments(owner->name, true, names);
		heldArguments(*owner, names);
		return home(at, namespaceOf(owner->name), owner->name, names);
	}
	// a function template's: after the template's declaration
	return home(Place{f->file, blockEnd(f->file, f->line)}, namespaceOf(scopeOf(f->name)), "", names);
}

}  // namespace idakit
