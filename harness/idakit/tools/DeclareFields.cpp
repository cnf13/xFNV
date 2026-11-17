// declare --pdb-fields: src's fields whose type is not the one the PDB declares for the member of their name (srcdb's
// pdb_typed: a field with neither a `field` nor a `field_recorded` item), each with the PDB's type spelled as src
// spells types and a verdict (docs/IDAKIT.md "idakit declare"): apply (the PDB's form fits the exe; --retype writes
// it), keep (the exe contradicts it: the `// PDB:` comment to write), spelling (the same type, srcdb's compare still
// to learn) or open (anything else), each with its reason. A class template's member is one row of the template (its
// instances' fields judged alike). Writes DIR/pdb-fields.tsv.
//
// declare --retype with such a table (its `field` column): each apply row's member declaration rewritten in the
// record's definition, each row's comment written above it; the layout guard refuses a type of another size or
// alignment. Changes go to DIR/applied.tsv (--revert puts them back), outcomes to DIR/retype.tsv, and the registered
// src functions whose bodies name an applied field to DIR/users.tsv (the emitted ones' addresses to DIR/reemit.list).

#include "DeclarePdb.hpp"

#include "idakit/Address.hpp"
#include "idakit/Database.hpp"
#include "idakit/SrcEdit.hpp"
#include "idakit/SrcModel.hpp"
#include "idakit/SrcTypes.hpp"

#include <Facts.hpp>

#include <algorithm>
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

// A field's declaration in its record's text: the declarator's span (its type and name, no specifiers, attributes,
// initialiser or bit-field width) and its line
struct FieldDeclaration
{
	size_t begin = 0, end = 0;
	int64_t line = 0;
};

// the type text as compared with src's spelling: no blanks, no elaborated `struct`/`class`/`enum`/`union`, no
// `FNVD_CDECL` (a function pointer's default convention)
std::string comparedText(const std::string& text)
{
	static const std::regex elaborated(R"(\b(struct|class|enum|union)\s+|\b(FNVD_CDECL|__cdecl)\b)");
	std::string out = std::regex_replace(text, elaborated, "");
	out.erase(std::remove_if(out.begin(), out.end(), [](char c) { return std::isspace(static_cast<unsigned char>(c)); }),
			  out.end());
	return out;
}

// The declaration of field `name` among the members of the record whose name is on line `from` and whose closing
// brace is on line `to`: the member statements at the body's top level (nested bodies, comments and preprocessor
// lines skipped), the one that declares `name` (followed by `;`, `[`, `=`, `{`, `)`, `:` or `,`); nullopt with why
std::optional<FieldDeclaration> fieldDeclaration(const Text& t, int64_t from, int64_t to, const std::string& name,
												 std::string& why)
{
	const std::string& s = t.text;
	size_t k = t.at(from), end = std::min(s.size(), t.at(to + 1));
	// the body's opening brace (a base clause has none)
	for (; k < end && s[k] != '{'; ++k)
		if (s.compare(k, 2, "//") == 0)
			k = s.find('\n', k);
	if (k >= end)
	{
		why = "no body between lines " + std::to_string(from) + " and " + std::to_string(to);
		return std::nullopt;
	}
	int depth = 0;
	size_t statement = k + 1;
	auto declares = [&](size_t b, size_t e) -> std::optional<FieldDeclaration> {
		// leading blanks, comments, preprocessor lines, attributes and specifiers
		for (bool more = true; more && b < e;)
		{
			more = false;
			while (b < e && std::isspace(static_cast<unsigned char>(s[b])))
				++b, more = true;
			if (s.compare(b, 2, "//") == 0 || s[b] == '#')
				b = std::min(e, s.find('\n', b)), more = true;
			else if (s.compare(b, 2, "/*") == 0)
				b = std::min(e, s.find("*/", b) + 2), more = true;
			else if (s.compare(b, 2, "[[") == 0)
				b = std::min(e, s.find("]]", b) + 2), more = true;
			else if (s.compare(b, 7, "mutable") == 0 && !identifierChar(s[b + 7]))
				b += 7, more = true;
		}
		std::string head = s.substr(b, std::min<size_t>(e - b, 16));
		static const std::regex notField(
			R"(^(using|typedef|friend|static|static_assert|template|virtual|explicit|inline|constexpr|operator|FNVD_)\b)");
		if (std::regex_search(head, notField))
			return std::nullopt;
		for (size_t at = s.find(name, b); at != std::string::npos && at + name.size() <= e;
			 at = s.find(name, at + 1))
		{
			size_t after = at + name.size();
			if ((at > b && (identifierChar(s[at - 1]) || s[at - 1] == ':' || s[at - 1] == '.' || s[at - 1] == '>' ||
							s[at - 1] == '~')) ||
				(after < e && identifierChar(s[after])))
				continue;
			size_t next = after;
			while (next < e && std::isspace(static_cast<unsigned char>(s[next])))
				++next;
			char c = next < e ? s[next] : ';';
			if (c == ':' && next + 1 < e && s[next + 1] == ':')
				continue;
			if (std::string(";[={):,").find(c) == std::string::npos)
				continue;
			// the declarator: up to a top-level `=`, `{`, `:` or the end; one declarator
			int d = 0;
			size_t stop = e;
			for (size_t m = b; m < e; ++m)
			{
				char x = s[m];
				if (x == '(' || x == '[' || x == '<')
					++d;
				else if (x == ')' || x == ']' || (x == '>' && !(m > b && s[m - 1] == '-')))
					--d;
				else if (d == 0 && m > after - 1 && (x == '=' || x == '{' || (x == ':' && s[m + 1] != ':' && s[m - 1] != ':')))
				{
					stop = m;
					break;
				}
				else if (d == 0 && x == ',')
				{
					why = "the member is one of several declarators in one declaration";
					return FieldDeclaration{};
				}
			}
			size_t last = s.find_last_not_of(" \t\n", stop - 1);
			return FieldDeclaration{b, last + 1, t.lineOf(b)};
		}
		return std::nullopt;
	};
	for (++k; k < end; ++k)
	{
		char c = s[k];
		if (s.compare(k, 2, "//") == 0)
		{
			k = s.find('\n', k);
			continue;
		}
		if (s.compare(k, 2, "/*") == 0)
		{
			k = s.find("*/", k) + 1;
			continue;
		}
		if (c == '"' || c == '\'')
		{
			for (++k; k < end && s[k] != c; ++k)
				if (s[k] == '\\')
					++k;
			continue;
		}
		if (c == '(' || c == '[' || c == '{')
			++depth;
		else if (c == ')' || c == ']')
			--depth;
		else if (c == '}')
		{
			if (--depth < 0)
				break;  // the record's closing brace
			if (depth == 0)
				statement = k + 1;
		}
		else if (c == ';' && depth == 0)
		{
			if (std::optional<FieldDeclaration> found = declares(statement, k))
			{
				if (found->end == 0)
					return std::nullopt;
				return found;
			}
			statement = k + 1;
		}
		else if (c == ':' && depth == 0 && s[k + 1] != ':' && s[k - 1] != ':')
		{
			std::string label = trim(s.substr(statement, k - statement));
			size_t nl = label.rfind('\n');
			label = trim(nl == std::string::npos ? label : label.substr(nl + 1));
			if (label == "public" || label == "protected" || label == "private")
				statement = k + 1;
		}
	}
	why = "no declaration of " + name + " at the record's top level (lines " + std::to_string(from) + "-" +
		  std::to_string(to) + ")";
	return std::nullopt;
}

// what the exe's code shows at a field's offset (Facts::thisAccesses of the methods of its record and of the classes
// that hold it at offset 0): the access widths, the signedness, float use
struct Evidence
{
	std::map<uint8_t, uint32_t> widths;  // width -> the first pc (reads and writes; no address)
	std::optional<uint32_t> signedAt, unsignedAt, realAt, integerAt;
	std::optional<uint32_t> orderedAt;  // an ordered condition (jl/jb...) or shift reads it: no bool's use
	bool any = false;
};

struct FieldRow
{
	std::string id, record, field, offset, src, pdb, verdict, reason, comment, key, file;
	int64_t line = 0;
	size_t instances = 1;
};

std::string hexOffset(int64_t bytes)
{
	char buffer[32];
	std::snprintf(buffer, sizeof buffer, "0x%llX", static_cast<unsigned long long>(bytes));
	return buffer;
}

// the last name of a qualified class name, its template arguments dropped (`NiTArray<NiNode *>` is NiTArray)
std::string shortName(const std::string& name)
{
	std::string out = name.substr(0, name.find('<'));
	size_t colon = out.rfind("::");
	return colon == std::string::npos ? out : out.substr(colon + 2);
}

class Fields : private PdbTypes
{
public:
	Fields(const SrcModel& model, SrcTypes& types, exefacts::Facts& facts) : PdbTypes(model, types), m_facts(facts)
	{
		for (const auto& [name, fields] : model.pdbFields())
			for (const SrcModel::PdbField& f : fields)
			{
				std::vector<std::string>& at = m_pdb[comparedName(name)][f.name];
				if (std::find(at.begin(), at.end(), f.type) == at.end())
					at.push_back(f.type);
			}
		// the thiscall methods with an exe address, by class, without those a secondary vtable holds (their this is
		// a base subobject's)
		std::set<std::string> adjusted;
		for (const auto& [name, record] : model.records())
			for (const SrcModel::Vtable& v : record.vtables)
				if (v.offset != 0)
					for (const SrcModel::Slot& slot : v.slots)
						adjusted.insert(slot.function);
		for (const auto& [mangled, ea] : m_address)
		{
			const SrcModel::Function* f = model.function(mangled);
			const SrcModel::Abi* abi = model.abi(mangled);
			if (f && !f->cls.empty() && !f->isStatic && abi && abi->cc == "thiscall" && !adjusted.count(mangled))
				m_methods[f->cls].insert(ea);
		}
		// each record's classes that hold it at offset 0: itself and those deriving from it there
		for (const auto& [name, record] : model.records())
			for (const std::string* r = &name; r;)
			{
				m_holders[*r].push_back(name);
				const SrcModel::Record* rec = model.record(*r);
				r = nullptr;
				if (rec)
					for (const SrcModel::Base& b : rec->bases)
						if (b.offset == 0 && !b.isVirtual)
						{
							r = &b.record;
							break;
						}
			}
	}

	std::vector<FieldRow> run();

private:
	struct Context
	{
		const SrcModel::Record* record = nullptr;
		const SrcModel::Field* field = nullptr;
		int64_t offset = 0;  // bytes
		Evidence evidence;
	};

	const std::vector<exefacts::ThisAccess>& accesses(uint64_t ea);
	Evidence evidence(const std::string& record, int64_t offset, int64_t size, bool scalar);
	std::optional<int64_t> room(const SrcModel::Record& record, int64_t offset) const;
	void judge(const Context& c, FieldRow& row, const std::string& pdbKey);
	void keep(FieldRow& row, const std::string& fact);
	std::optional<std::string> construction(const std::string& srcKey, const std::string& pdbKey) const;
	std::optional<std::string> weakens(const std::string& srcKey, const std::string& pdbKey, int depth = 0) const;
	bool byteCopied(const std::string& key) const;
	// the key as normal() compares it, with no const or volatile at any level
	std::string stripped(const std::string& key, int depth = 0) const;

	exefacts::Facts& m_facts;
	// comparedName(the PDB's record) -> member name -> its distinct types
	std::unordered_map<std::string, std::map<std::string, std::vector<std::string>>> m_pdb;
	std::unordered_map<std::string, std::set<uint64_t>> m_methods;         // class -> its methods' addresses
	std::unordered_map<std::string, std::vector<std::string>> m_holders;   // record -> the classes holding it at 0
	std::unordered_map<uint64_t, std::vector<exefacts::ThisAccess>> m_accesses;
	std::map<std::string, Text> m_texts;
};

const std::vector<exefacts::ThisAccess>& Fields::accesses(uint64_t ea)
{
	auto found = m_accesses.find(ea);
	if (found != m_accesses.end())
		return found->second;
	std::vector<exefacts::ThisAccess> out;
	try
	{
		out = m_facts.thisAccesses(uint32_t(ea));
	}
	catch (const std::exception&)
	{
	}
	m_facts.trim();
	return m_accesses.emplace(ea, std::move(out)).first->second;
}

// the accesses at exactly the field's offset: their widths; for a scalar of at most 4 bytes, the signedness its
// extensions and conditions show and its float use
Evidence Fields::evidence(const std::string& record, int64_t offset, int64_t size, bool scalar)
{
	Evidence e;
	auto holders = m_holders.find(record);
	if (holders == m_holders.end())
		return e;
	for (const std::string& cls : holders->second)
	{
		auto methods = m_methods.find(cls);
		if (methods == m_methods.end())
			continue;
		for (uint64_t ea : methods->second)
			for (const exefacts::ThisAccess& a : accesses(ea))
			{
				if (a.offset != offset || a.kind == exefacts::ArgumentAccess::Address)
					continue;
				e.any = true;
				e.widths.emplace(a.size, a.pc);
				if (!scalar)
					continue;
				if (a.real)
					e.realAt = e.realAt.value_or(a.pc);
				else if (a.size == size)
				{
					if (a.extension < 0 || a.sign < 0)
						e.signedAt = e.signedAt.value_or(a.pc);
					if (a.extension > 0 || a.sign > 0)
						e.unsignedAt = e.unsignedAt.value_or(a.pc);
					if (a.extension || a.sign)
						e.integerAt = e.integerAt.value_or(a.pc);
					if (a.sign)
						e.orderedAt = e.orderedAt.value_or(a.pc);
				}
			}
	}
	return e;
}

// the bytes from the offset to the next field (or the record's end), where the record's size is pinned to the exe's
std::optional<int64_t> Fields::room(const SrcModel::Record& record, int64_t offset) const
{
	if (!record.pinned)
		return std::nullopt;
	int64_t next = record.size;
	for (const SrcModel::Field& f : record.fields)
		if (f.offsetBits / 8 > offset)
			next = std::min(next, f.offsetBits / 8);
	return next - offset;
}

std::string Fields::stripped(const std::string& key, int depth) const
{
	if (depth > 64)
		return key;
	SrcTypes::Parts p = m_types.parts(key);
	if (p.kind == "typedef")
	{
		const SrcModel::Typedef* t = m_model.typedefNamed(p.name);
		return t ? stripped(t->target, depth + 1) : key;
	}
	if (p.kind == "const" || p.kind == "volatile")
		return stripped(p.of, depth + 1);
	if (p.kind == "ptr" || p.kind == "lref" || p.kind == "rref")
		return p.kind + "(" + stripped(p.of, depth + 1) + ")";
	if (p.kind == "array")
		return "array(" + stripped(p.of, depth + 1) + "," + std::to_string(p.count) + ")";
	if (std::optional<SrcTypes::Function> f = SrcTypes::function(key))
	{
		std::string out = "fn(" + f->cc + ";" + stripped(f->ret, depth + 1) + ";";
		for (const std::string& param : f->params)
			out += stripped(param, depth + 1) + ",";
		return out + (f->variadic ? "...)" : ")");
	}
	return normal(key, depth);
}

bool Fields::byteCopied(const std::string& key) const
{
	std::string k = resolved(key);
	SrcTypes::Parts p = m_types.parts(k);
	if (p.kind == "array")
		return byteCopied(p.of);
	if (p.kind == "lref" || p.kind == "rref")
		return false;
	if (p.kind == "record")
	{
		const SrcModel::Record* r = m_model.record(p.name);
		return !r || m_model.byteCopied(*r);
	}
	return true;
}

// a change that alters how C++ constructs, destroys or copies the record (a reference, or a member of another
// copy semantics: NiPointer<T> for T*): why, else nullopt
std::optional<std::string> Fields::construction(const std::string& srcKey, const std::string& pdbKey) const
{
	if (kindOf(pdbKey) == "ref")
		return "construction: the PDB's reference member (src's " + spelled(srcKey) + ")";
	bool was = byteCopied(srcKey), now = byteCopied(pdbKey);
	if (was != now)
		return std::string("construction: ") +
			   (now ? "src's " + spelled(srcKey) + " runs constructors or copies src's would not"
					: "the PDB's " + spelled(pdbKey) + " runs constructors, a destructor or a copy where src's copies bytes");
	return std::nullopt;
}

// the PDB's type would weaken src's: a void pointer for a typed one, a base for a derived pointee or template
// argument, at any level of pointers, arrays and function pointer parameters; why, else nullopt
std::optional<std::string> Fields::weakens(const std::string& srcKey, const std::string& pdbKey, int depth) const
{
	if (depth > 8)
		return std::nullopt;
	std::string s = resolved(srcKey), p = resolved(pdbKey);
	SrcTypes::Parts sp = m_types.parts(s), pp = m_types.parts(p);
	if ((sp.kind == "ptr" || sp.kind == "lref") && (pp.kind == "ptr" || pp.kind == "lref"))
	{
		std::string so = resolved(sp.of), po = resolved(pp.of);
		SrcTypes::Parts sop = m_types.parts(so), pop = m_types.parts(po);
		if (pop.kind == "builtin" && pop.name == "void" && !(sop.kind == "builtin" && sop.name == "void"))
			return "the PDB's void pointer would weaken src's " + spelled(srcKey);
		if (sop.kind == "record" && pop.kind == "record" && sop.name != pop.name && derives(sop.name, pop.name))
			return "the PDB's " + pop.name + " is a base of src's " + sop.name + ": it would weaken src's type";
		return weakens(so, po, depth + 1);
	}
	if (sp.kind == "array" && pp.kind == "array")
		return weakens(sp.of, pp.of, depth + 1);
	std::optional<SrcTypes::Function> sf = SrcTypes::function(s), pf = SrcTypes::function(p);
	if (sf && pf && sf->params.size() == pf->params.size())
	{
		if (std::optional<std::string> w = weakens(sf->ret, pf->ret, depth + 1))
			return w;
		for (size_t k = 0; k < sf->params.size(); ++k)
			if (std::optional<std::string> w = weakens(sf->params[k], pf->params[k], depth + 1))
				return w;
	}
	// one instance of a template for another, an argument's pointee a base of src's (NiPointer<NiAVObject> for
	// NiPointer<NiNode>)
	if (sp.kind == "record" && pp.kind == "record")
	{
		const SrcModel::Record* sr = m_model.record(sp.name);
		const SrcModel::Record* pr = m_model.record(pp.name);
		if (sr && pr && !sr->pattern.empty() && sr->pattern == pr->pattern)
		{
			auto argument = [](const std::string& name) {
				size_t lt = name.find('<');
				std::string a = lt == std::string::npos ? "" : name.substr(lt + 1, name.rfind('>') - lt - 1);
				a = trim(a);
				while (!a.empty() && (a.back() == '*' || a.back() == ' '))
					a.pop_back();
				return a.find(',') == std::string::npos ? a : std::string();
			};
			std::string sa = argument(sr->name), pa = argument(pr->name);
			if (!sa.empty() && !pa.empty() && sa != pa && m_model.record(sa) && m_model.record(pa) && derives(sa, pa))
				return "the PDB's " + pa + " is a base of src's " + sa + " (" + sr->name + "): it would weaken src's type";
		}
		if (sr && pr && sp.name != pp.name && derives(sp.name, pp.name))
			return "the PDB's " + pp.name + " is a base of src's " + sp.name + ": it would weaken src's type";
	}
	return std::nullopt;
}

// a keep row: the comment names the field as src declares it
void Fields::keep(FieldRow& row, const std::string& fact)
{
	row.verdict = "keep";
	row.reason = fact;
	std::string form = row.key.empty() ? row.pdb + " " + row.field : spelled(row.key, row.field);
	row.comment = "// PDB: " + form + "; the exe " + fact + ".";
}

void Fields::judge(const Context& c, FieldRow& row, const std::string& pdbKey)
{
	const std::string& srcKey = c.field->type;
	std::string missing;
	std::optional<std::string> target = toSrc(pdbKey, missing);
	row.pdb = target ? spelled(*target) : spelled(pdbKey);
	row.key = target.value_or("");
	auto open = [&](const std::string& why) {
		row.verdict = "open";
		row.reason = why;
	};
	auto apply = [&](const std::string& why) {
		row.verdict = "apply";
		row.reason = why;
	};
	if (c.field->bits)
		return open("a bit-field");
	if (srcKey.find("(unnamed") != std::string::npos || srcKey.find("(anonymous") != std::string::npos ||
		pdbKey.find("<unnamed") != std::string::npos)
		return open("an unnamed type");
	auto isUnion = [&](const std::string& key) {
		SrcTypes::Parts p = m_types.parts(resolved(key));
		const SrcModel::Record* r = p.kind == "record" ? m_model.record(p.name) : nullptr;
		return r && r->tag == "union";
	};
	if (isUnion(srcKey) || (target && isUnion(*target)))
		return open("a union");
	// an Xbox 360 type of the PDB's is the PC exe's other type
	std::smatch platform;
	if (std::regex_search(pdbKey, platform, platformPattern()))
	{
		row.key.clear();
		row.pdb = spelled(pdbKey);
		return keep(row, "is the PC build (src's " + row.src + ")");
	}
	if (!target)
		return open("the PDB's type is not in src: " + missing);
	// the field's own const or volatile is src's (the PDB's Xbox build may lack a volatile the PC code needs)
	{
		std::vector<std::string> quals;
		std::string bare = srcKey;
		for (SrcTypes::Parts p = m_types.parts(bare); p.kind == "const" || p.kind == "volatile"; p = m_types.parts(bare))
			quals.push_back(p.kind), bare = p.of;
		std::string inner = *target;
		for (SrcTypes::Parts p = m_types.parts(inner); p.kind == "const" || p.kind == "volatile"; p = m_types.parts(inner))
			inner = p.of;
		for (auto q = quals.rbegin(); q != quals.rend(); ++q)
			inner = *q + "(" + inner + ")";
		target = inner;
		row.key = inner;
	}
	if (normal(srcKey) == normal(*target) || normal(resolved(srcKey)) == normal(resolved(*target)))
	{
		row.verdict = "spelling";
		row.reason = normal(srcKey) == normal(pdbKey) || normal(resolved(srcKey)) == normal(resolved(pdbKey))
						 ? "the same type: srcdb's compare to learn the spelling"
						 : "the same type through ida_type_aliases.tsv's name for the PDB's";
		return;
	}
	if (stripped(srcKey) == stripped(*target))
	{
		row.verdict = "spelling";
		row.reason = "the same type but a pointee's const or volatile, which the exe cannot show";
		return;
	}
	std::optional<int64_t> ss = size(srcKey), ps = size(*target), sa = align(srcKey), pa = align(*target);
	if (!ss || !ps || !sa || !pa)
		return open("a size src does not know");
	if (std::optional<std::string> why = construction(srcKey, *target))
		return open(*why);
	const Evidence& e = c.evidence;
	auto bytes = [](int64_t n) { return n == 1 ? std::string("a byte") : n == 2 ? "a word" : n == 4 ? "a dword" : std::to_string(n) + " bytes"; };
	if (*ss != *ps)
	{
		// the exe's width of the field, or the room the exe's layout leaves it
		std::string sk0 = kindOf(srcKey);
		bool scalarSrc = sk0 == "int" || sk0 == "enum" || sk0 == "bool" || sk0 == "char" || sk0 == "wchar_t" ||
						 sk0 == "ptr" || sk0 == "float";
		for (const auto& [width, pc] : e.widths)
			if (scalarSrc && int64_t(width) == *ss && *ss <= 4)
				return keep(row, "accesses it as " + bytes(*ss) + " at " + hexText(pc));
		if (std::optional<int64_t> left = room(*c.record, c.offset); left && *ps > *left)
			return keep(row, "has " + std::to_string(*left) + " bytes there (" + c.record->name + " is " +
								 std::to_string(c.record->size) + " bytes, FNVD_LAYOUT; the next member at " +
								 hexOffset(c.offset + *left) + ")");
		if (!e.widths.empty() && e.widths.rbegin()->first == *ps && *ps < *ss)
			return open("the exe accesses " + bytes(*ps) + " of it (at " + hexText(e.widths.rbegin()->second) +
						"): src's field is wider than the PDB's, its padding to declare");
		return open("sizes differ: src " + std::to_string(*ss) + " bytes, the PDB " + std::to_string(*ps));
	}
	if (*sa != *pa)
		return open("alignment differs: src " + std::to_string(*sa) + ", the PDB " + std::to_string(*pa));
	if (std::optional<std::string> why = weakens(srcKey, *target))
		return open(*why);
	// an access wider than the field contradicts any scalar type
	if (!e.widths.empty() && int64_t(e.widths.rbegin()->first) > *ss)
		return open("the exe accesses " + bytes(e.widths.rbegin()->first) + " at its offset (" +
					hexText(e.widths.rbegin()->second) + ")");
	std::string sk = kindOf(srcKey), pk = kindOf(*target);
	auto pointerLike = [](const std::string& k) { return k == "ptr" || k == "ref"; };
	auto scalar = [](const std::string& k) {
		return k == "int" || k == "enum" || k == "bool" || k == "char" || k == "wchar_t";
	};
	if (pointerLike(sk) && pointerLike(pk))
	{
		std::string so = resolved(m_types.parts(resolved(srcKey)).of), po = resolved(m_types.parts(resolved(*target)).of);
		SrcTypes::Parts sp = m_types.parts(so), pp = m_types.parts(po);
		bool sPointer = sp.kind == "ptr" || sp.kind == "lref", pPointer = pp.kind == "ptr" || pp.kind == "lref";
		if (sPointer != pPointer && !(sp.kind == "builtin" && sp.name == "void"))
			return open("the pointees' levels of indirection differ: the exe's accesses decide");
		std::optional<SrcTypes::Function> sf = SrcTypes::function(so), pf = SrcTypes::function(po);
		if (sf && pf && (sf->cc != pf->cc || sf->params.size() != pf->params.size()))
			return open("the function pointers' conventions or parameter counts differ");
		if (sf.has_value() != pf.has_value() && !(sp.kind == "builtin" && sp.name == "void"))
			return open("a function pointer against a data pointer");
		// a function pointer's return and parameters: each the same, or a pointer for a pointer (the callee's code
		// decides an integer's width or signedness, which a field's accesses do not show)
		if (sf && pf)
			for (size_t k = 0; k <= sf->params.size(); ++k)
			{
				const std::string& a = k < sf->params.size() ? sf->params[k] : sf->ret;
				const std::string& b = k < pf->params.size() ? pf->params[k] : pf->ret;
				if (stripped(a) != stripped(b) && !(pointerLike(kindOf(a)) && pointerLike(kindOf(b))))
					return open("the function pointer's " +
								(k < sf->params.size() ? "parameter " + std::to_string(k) : std::string("return")) +
								": src's " + spelled(a) + ", the PDB's " + spelled(b));
			}
		return apply(sp.kind == "builtin" && sp.name == "void" ? "the PDB's pointee for src's void"
															   : "a pointer for a pointer");
	}
	if (pointerLike(sk) && pk == "record")
	{
		// a record holding one pointer: a smart pointer (hkRefPtr<T>, a NiPointer src copies as bytes), whose
		// construction, destruction and assignment the bodies would change to
		SrcTypes::Parts pp = m_types.parts(resolved(*target));
		const SrcModel::Record* r = m_model.record(pp.name);
		if (r && r->fields.size() == 1 && pointerLike(kindOf(r->fields[0].type)))
			return open("construction: the PDB's smart pointer " + row.pdb + " for src's " + row.src);
		return open("the PDB's record " + row.pdb + " for src's pointer");
	}
	if (pointerLike(sk))
		return open("the PDB's " + pk + " for src's pointer: a pointer never becomes an integer");
	if ((sk == "int" || sk == "enum") && pointerLike(pk))
		return apply("the PDB's pointer for src's integer of its width");
	if (scalar(sk) && scalar(pk))
	{
		if (sk == "enum" && pk != "enum")
			return open("the PDB's " + pk + " for src's enum " + row.src);
		if (pk == "enum")
			return apply("an enum of the field's width");
		if (pk == "bool")
		{
			// MSVC loads a bool with movzx: only an ordered compare or a shift is an integer's use
			if (e.orderedAt)
				return keep(row, "compares it as an integer at " + hexText(*e.orderedAt));
			return apply("a bool of the field's width");
		}
		if (sk == "bool")
			return apply("the PDB's " + pk + " for src's bool of its width");
		// two integers (char and wchar_t among them) of one width: their signedness
		const SrcModel::Type* st = m_model.type(m_types.bare(resolved(srcKey)));
		bool srcSigned = st ? st->isSigned : false;
		std::string pn = m_types.parts(resolved(*target)).name;
		bool pdbSigned = pn == "char" || pn == "signed char" || pn == "short" || pn == "int" || pn == "long" ||
						 pn == "long long" || pn == "int8_t" || pn == "int16_t" || pn == "int32_t" || pn == "int64_t";
		if (const SrcModel::Type* pt = m_model.type(m_types.bare(resolved(*target))))
			pdbSigned = pt->isSigned;
		if (srcSigned == pdbSigned)
		{
			// MSVC's char is a signed byte: src's int8_t (RULES' fixed width) for the PDB's char, or the reverse
			if (sk == "char" || pk == "char")
			{
				row.verdict = "spelling";
				row.reason = "char and int8_t: the same signed byte, RULES' fixed width";
				return;
			}
			return apply("the same width and signedness");
		}
		std::optional<uint32_t> against = pdbSigned ? e.unsignedAt : e.signedAt;
		std::optional<uint32_t> agrees = pdbSigned ? e.signedAt : e.unsignedAt;
		if (against && agrees)
			return open("the exe reads it both signed and unsigned (at " + hexText(*e.signedAt) + " and " +
						hexText(*e.unsignedAt) + ")");
		if (against)
			return keep(row, std::string(pdbSigned ? "reads it unsigned" : "reads it signed") + " at " + hexText(*against));
		return apply(agrees ? std::string("the exe reads it ") + (pdbSigned ? "signed" : "unsigned") + " at " +
								  hexText(*agrees)
							: std::string("no access contradicts its signedness"));
	}
	if (sk == "float" || pk == "float")
	{
		if (pk == "float" && e.realAt && !e.integerAt)
			return apply("the exe reads it as a float at " + hexText(*e.realAt));
		if (pk != "float" && e.realAt)
			return keep(row, "reads it as a float at " + hexText(*e.realAt));
		return open(sk == pk ? "float against double" : "a float against an integer: the exe's accesses do not show which");
	}
	if (sk == "record" && pk == "record")
		return apply("a record of the same size");
	if (pk == "record" && (scalar(sk) || sk == "array"))
		return apply("the PDB's record for src's " + sk + " of its size");
	if (sk == "array" && pk == "array")
	{
		SrcTypes::Parts sp = m_types.parts(resolved(srcKey)), pp = m_types.parts(resolved(*target));
		std::string se = kindOf(sp.of);
		if (sp.count == pp.count || (scalar(se) && size(sp.of).value_or(0) == 1))
			return apply(sp.count == pp.count ? "an array of the same bound" : "src's raw bytes for the PDB's array");
		return open("arrays of other bounds: " + std::to_string(sp.count) + " and " + std::to_string(pp.count));
	}
	if (sk == "array" && scalar(kindOf(m_types.parts(resolved(srcKey)).of)) &&
		size(m_types.parts(resolved(srcKey)).of).value_or(0) == 1)
		return apply("src's raw bytes for the PDB's " + pk);
	open("src's " + sk + " for the PDB's " + pk);
}

std::vector<FieldRow> Fields::run()
{
	// one row per record field, a class template's instances' rows joined into the template's
	struct Key
	{
		std::string file;
		int64_t line;
		std::string field;
		bool operator<(const Key& o) const { return std::tie(file, line, field) < std::tie(o.file, o.line, o.field); }
	};
	std::map<Key, std::vector<FieldRow>> groups;
	std::map<Key, std::string> patterns;
	for (const auto& [name, record] : m_model.records())
	{
		auto pdb = m_pdb.find(comparedName(name));
		if (pdb == m_pdb.end() || record.file.empty() || !record.line)
			continue;
		for (const SrcModel::Field& f : record.fields)
		{
			auto member = pdb->second.find(f.name);
			if (f.name.empty() || member == pdb->second.end() || m_model.pdbTyped("field", name, f.name) ||
				m_model.pdbTyped("field_recorded", name, f.name))
				continue;
			FieldRow row;
			row.record = name;
			row.field = f.name;
			row.offset = std::to_string(f.offsetBits / 8);
			row.src = spelled(f.type);
			row.file = record.file;
			Context c;
			c.record = &record;
			c.field = &f;
			c.offset = f.offsetBits / 8;
			if (member->second.size() > 1)
			{
				std::string types;
				for (const std::string& t : member->second)
					types += (types.empty() ? "" : ", ") + spelled(t);
				row.pdb = types;
				row.verdict = "open";
				row.reason = "the PDB's records of the name disagree: " + types;
			}
			else
			{
				std::string k = kindOf(f.type);
				std::optional<int64_t> s = size(f.type);
				c.evidence = evidence(name, c.offset, s.value_or(0),
									  k == "int" || k == "enum" || k == "bool" || k == "char" || k == "wchar_t" ||
										  k == "float" || k == "ptr");
				judge(c, row, member->second[0]);
			}
			Key key{record.file, record.line, f.name};
			if (!record.pattern.empty() && !SrcModel::explicitSpecialization(record))
				patterns[key] = record.pattern;
			groups[key].push_back(std::move(row));
		}
	}
	std::vector<FieldRow> rows;
	for (auto& [key, members] : groups)
	{
		FieldRow row = members[0];
		row.instances = members.size();
		// a class template's member (or a member of a class nested in one): one text for every instance
		auto pattern = patterns.find(key);
		if (pattern != patterns.end())
			row.record = pattern->second;
		std::set<std::string> offsets;
		for (const FieldRow& m : members)
			offsets.insert(m.offset);
		if (offsets.size() > 1)
			row.offset.clear();
		for (const FieldRow& m : members)
			if (m.verdict != row.verdict || m.key != row.key || m.comment != row.comment)
			{
				row.verdict = "open";
				row.reason = "the template's instances differ: " + m.record + " is " + m.verdict + " " +
							 (m.key.empty() ? "" : spelled(m.key)) + " (" + m.reason + "), " + members[0].record + " " +
							 members[0].verdict + " " + (members[0].key.empty() ? "" : spelled(members[0].key));
				row.comment.clear();
				break;
			}
		if (row.verdict != "open" && members.size() > 1)
			row.reason += " (all " + std::to_string(members.size()) + " instances)";
		row.id = row.record + "#" + row.field;
		// the declaration in the record's text: found, and spelled as src spells its type (a template's member
		// spelled with the template's parameters is no change a row can write)
		const SrcModel::Record* r = m_model.record(members[0].record);
		auto text = m_texts.find(r->file);
		if (text == m_texts.end())
			text = m_texts.emplace(r->file, Text(std::filesystem::exists(r->file) ? readFile(r->file) : "")).first;
		std::string why;
		std::optional<FieldDeclaration> d = fieldDeclaration(text->second, r->line, r->endLine, row.field, why);
		if (d)
		{
			row.line = d->line;
			std::string written = text->second.text.substr(d->begin, d->end - d->begin);
			const SrcModel::Field* f = r->field(row.field);
			if (row.verdict == "apply" && comparedText(written) != comparedText(spelled(f->type, row.field)))
			{
				row.verdict = "open";
				row.reason = (pattern != patterns.end() ? "the template's member is spelled `" : "its declaration is `") +
							 trim(written) + "`, not src's spelling of its type (" + spelled(f->type, row.field) + ")";
			}
		}
		else if (row.verdict == "apply" || row.verdict == "keep")
		{
			row.verdict = "open";
			row.reason = "its declaration is not found: " + why;
			row.comment.clear();
		}
		rows.push_back(std::move(row));
	}
	return rows;
}

// --retype's field rows (a table with a `field` column)
struct RetypeRow
{
	std::string id, record, field, verdict, comment, key;
};

}  // namespace

int declarePdbFields(const SrcModel& model, SrcTypes& types, const std::filesystem::path& out)
{
	exefacts::Program program(".");
	exefacts::Facts facts(program);
	std::vector<FieldRow> rows = Fields(model, types, facts).run();
	std::ofstream table(out / "pdb-fields.tsv", std::ios::trunc);
	table << "id\trecord\tfield\toffset\tsrc\tpdb\tverdict\treason\tcomment\tkey\tfile\tline\tinstances\n";
	std::map<std::string, size_t> verdicts, fields;
	for (const FieldRow& r : rows)
	{
		table << cell(r.id) << '\t' << cell(r.record) << '\t' << r.field << '\t' << r.offset << '\t' << cell(r.src)
			  << '\t' << cell(r.pdb) << '\t' << r.verdict << '\t' << cell(r.reason) << '\t' << cell(r.comment) << '\t'
			  << cell(r.key) << '\t' << r.file << '\t' << r.line << '\t' << r.instances << '\n';
		++verdicts[r.verdict];
		fields[r.verdict] += r.instances;
	}
	std::string summary;
	for (const auto& [verdict, count] : verdicts)
		summary += " " + std::to_string(count) + " " + verdict + " (" + std::to_string(fields[verdict]) + " fields)";
	table.close();
	std::fprintf(stderr, "idakit declare --pdb-fields: %zu rows:%s (%s)\n", rows.size(), summary.c_str(),
				 (out / "pdb-fields.tsv").c_str());
	return 0;
}

// --retype TABLE with field rows: each apply row's member declaration and each row's comment
int declareRetypeFields(const SrcModel& model, SrcTypes& types, const std::string& table,
						const std::filesystem::path& out, bool apply)
{
	std::ifstream in(table);
	if (!in)
		throw Error("declare --retype: cannot read " + table);
	std::string line;
	std::getline(in, line);
	std::vector<std::string> header = cells(line);
	auto column = [&](const std::string& name) -> size_t {
		auto found = std::find(header.begin(), header.end(), name);
		if (found == header.end())
			throw Error("declare --retype: " + table + " has no column " + name);
		return size_t(found - header.begin());
	};
	size_t id = column("id"), record = column("record"), field = column("field"), verdict = column("verdict"),
		   comment = column("comment"), key = column("key");
	std::vector<RetypeRow> rows;
	while (std::getline(in, line))
	{
		std::vector<std::string> c = cells(line);
		if (c.size() <= std::max({id, record, field, verdict, comment, key}))
			continue;
		RetypeRow r{c[id], c[record], c[field], c[verdict], c[comment], c[key]};
		if ((r.verdict == "apply" && !r.key.empty()) || !r.comment.empty())
			rows.push_back(r);
	}
	PdbTypes pdb(model, types);
	// a record by name, or a class template by its pattern's name: an instance of the pattern's own definition (no
	// explicit specialization) that has the field; they share the template's text
	std::map<std::string, std::vector<const SrcModel::Record*>> byPattern;
	for (const auto& [name, r] : model.records())
		if (!r.pattern.empty() && !SrcModel::explicitSpecialization(r))
			byPattern[r.pattern].push_back(&r);
	SrcChanges src;
	Includes includes;
	std::map<std::string, std::set<std::string>> inserted;  // file -> the include and declaration lines added to it
	std::map<std::string, std::string> outcome;
	struct Applied
	{
		const RetypeRow* row;
		const SrcModel::Record* record;
	};
	std::vector<Applied> applied;
	for (const RetypeRow& r : rows)
	{
		try
		{
			const SrcModel::Record* rec = model.record(r.record);
			if (!rec && byPattern.count(r.record))
				for (const SrcModel::Record* instance : byPattern[r.record])
					if (instance->field(r.field) && (!rec || instance->line < rec->line))
						rec = instance;
			if (!rec)
				throw Error("no record " + r.record);
			const SrcModel::Field* f = rec->field(r.field);
			if (!f)
				throw Error("no field " + r.field + " in " + rec->name);
			if (f->bits)
				throw Error("a bit-field");
			Text& t = src.text(rec->file);
			std::string why;
			std::optional<FieldDeclaration> d = fieldDeclaration(t, rec->line, rec->endLine, r.field, why);
			if (!d || d->end == 0)
				throw Error(why);
			std::vector<std::string> lines = t.lines();
			if (emittedBlock(lines, d->line))
				throw Error("the declaration is inside an emitted block");
			std::vector<Change> changes;
			if (r.verdict == "apply" && !r.key.empty())
			{
				// the layout guard: the new type's size and alignment are the old one's
				std::optional<int64_t> was = pdb.size(f->type), now = pdb.size(r.key);
				std::optional<int64_t> wasAlign = pdb.align(f->type), nowAlign = pdb.align(r.key);
				if (!was || !now || *was != *now || !wasAlign || !nowAlign || *wasAlign != *nowAlign)
					throw Error("layout guard: " + pdb.spelled(r.key) + " is " + (now ? std::to_string(*now) : "?") +
								" bytes aligned " + (nowAlign ? std::to_string(*nowAlign) : "?") + ", " +
								pdb.spelled(f->type) + " " + (was ? std::to_string(*was) : "?") + " aligned " +
								(wasAlign ? std::to_string(*wasAlign) : "?"));
				std::string written = t.text.substr(d->begin, d->end - d->begin);
				std::optional<std::string> old = types.spell(types.fixedWidthKey(f->type), r.field);
				std::optional<std::string> spelledNew = types.spell(types.fixedWidthKey(r.key), r.field);
				if (!old || !spelledNew)
					throw Error("src cannot spell " + (old ? r.key : f->type));
				if (comparedText(written) != comparedText(srcSpelling(*old)))
					throw Error("the declaration `" + trim(written) + "` is not src's spelling of its type (" +
								srcSpelling(*old) + ")");
				std::string now2 = srcSpelling(*spelledNew);
				if (now2 != written)
				{
					changes.push_back({d->begin, written.size(), now2, r.id});
					// what the new type needs declared in the record's file
					if (std::optional<Change> needs = declarationNeeds(model, types, includes, rec->file, t, r.key,
																		true, inserted, r.id))
						changes.push_back(*needs);
				}
			}
			if (!r.comment.empty())
			{
				int64_t first = d->line;
				std::string indent = lines[first - 1].substr(0, lines[first - 1].find_first_not_of(" \t"));
				bool recorded = false;
				for (int64_t l = commentTop(lines, first); l < first; ++l)
					recorded = recorded || trim(lines[l - 1]).starts_with("// PDB:");
				if (!recorded)
				{
					Change c{t.at(first), 0, commentBlock(r.comment, indent), r.id};
					c.comment = true;
					changes.push_back(c);
				}
			}
			for (Change& c : changes)
				src.add(rec->file, c);
			bool typed = std::any_of(changes.begin(), changes.end(), [](const Change& c) { return !c.comment; });
			outcome[r.id] = typed ? "applied" : changes.empty() ? "unchanged" : "commented";
			if (typed)
				applied.push_back({&r, rec});
		}
		catch (const std::exception& e)
		{
			outcome[r.id] = std::string("failed: ") + e.what();
		}
	}
	std::ofstream appliedOut(out / "applied.tsv", std::ios::trunc);
	appliedOut << "file\tline\told\tnew\trow\tfamily\n";
	std::map<std::string, std::vector<std::string>> perFile;
	src.write(apply, appliedOut, {}, perFile);
	appliedOut.close();
	std::ofstream outcomes(out / "retype.tsv", std::ios::trunc);
	outcomes << "id\toutcome\n";
	std::map<std::string, size_t> counts;
	for (const auto& [row, o] : outcome)
	{
		outcomes << row << '\t' << o << '\n';
		++counts[o.substr(0, o.find(':'))];
	}
	outcomes.close();
	// the users: each registered src function whose body names an applied field, the function a member of the
	// field's record or of a class deriving from it, or its body naming one of those classes
	std::map<std::string, uint64_t> registered;  // mangled -> its address
	for (const auto& [ea, refs] : model.refs())
		for (const SrcModel::Ref& r : refs)
			if ((r.kind == "body" || r.kind == "constructor" || r.kind == "destructor") && !r.function.empty())
				registered.emplace(r.function, ea);
	std::map<std::string, Text> files;
	std::ofstream users(out / "users.tsv", std::ios::trunc);
	users << "row\taddress\tfunction\tfile\tline\temitted\n";
	std::ofstream reemit(out / "reemit.list", std::ios::trunc);
	std::set<uint64_t> reemitted;
	size_t userCount = 0;
	std::set<std::string> userFunctions;
	for (const Applied& a : applied)
	{
		std::set<std::string> classes;  // the short names of the record and the classes deriving from it
		std::set<std::string> qualified;
		for (const auto& [name, r] : model.records())
			if (pdb.derives(name, a.record->name) || (!a.record->pattern.empty() && r.pattern == a.record->pattern))
				classes.insert(shortName(name)), qualified.insert(name);
		for (const auto& [mangled, ea] : registered)
		{
			const SrcModel::Definition* def = model.definition(mangled);
			const SrcModel::Function* fn = model.function(mangled);
			if (!def || def->file.empty() || def->instance)
				continue;
			auto file = files.find(def->file);
			if (file == files.end())
				file = files.emplace(def->file, Text(std::filesystem::exists(def->file) ? readFile(def->file) : ""))
						   .first;
			std::string_view s(file->second.text);
			size_t b = file->second.at(def->line), e = std::min(s.size(), file->second.at(def->endLine + 1));
			std::string_view body = b < e ? s.substr(b, e - b) : std::string_view();
			auto names = [&](const std::string& word) {
				for (size_t at = body.find(word); at != std::string_view::npos; at = body.find(word, at + 1))
					if ((at == 0 || !identifierChar(body[at - 1])) &&
						(at + word.size() >= body.size() || !identifierChar(body[at + word.size()])))
						return true;
				return false;
			};
			if (!names(a.row->field))
				continue;
			bool member = fn && qualified.count(fn->cls);
			if (!member && !std::any_of(classes.begin(), classes.end(), names))
				continue;
			users << a.row->id << '\t' << hexText(ea) << '\t' << (fn ? fn->name : mangled) << '\t' << def->file << '\t'
				  << def->line << '\t' << (def->emitted ? 1 : 0) << '\n';
			++userCount;
			userFunctions.insert(mangled);
			if (def->emitted && reemitted.insert(ea).second)
				reemit << hexText(ea) << '\n';
		}
	}
	users.close();
	reemit.close();
	std::string summary;
	for (const auto& [o, count] : counts)
		summary += " " + std::to_string(count) + " " + o;
	std::fprintf(stderr,
				 "idakit declare --retype: %zu field rows:%s; %zu files; %zu users (%zu functions), %zu to re-emit%s\n",
				 rows.size(), summary.c_str(), perFile.size(), userCount, userFunctions.size(), reemitted.size(),
				 apply ? "" : " (dry run: src unchanged)");
	for (const auto& [file, ids] : perFile)
	{
		std::string list;
		for (const std::string& r : ids)
			list += " " + r;
		std::fprintf(stderr, "  %s:%s\n", file.c_str(), list.c_str());
	}
	return 0;
}

}  // namespace idakit
