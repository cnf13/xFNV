// declare --pdb-signatures: the return, this and parameter slots of src's PDB-named functions whose type is not the
// one the PDB declares (srcdb's pdb_typed: a slot with neither a `param` nor a `recorded` item), each with the PDB's
// type spelled as src spells types and a verdict (docs/IDAKIT.md "idakit declare"): apply (the PDB's form fits the
// exe; --retype writes it), keep (the exe contradicts it: the `// PDB:` comment to write), spelling (the same type,
// srcdb's compare still to learn) or open (anything else), each with its reason. Writes DIR/pdb-signatures.tsv, the
// table --retype reads (Retype.cpp has its columns).

#include "DeclarePdb.hpp"

#include "idakit/Address.hpp"
#include "idakit/Database.hpp"
#include "idakit/SrcEdit.hpp"
#include "idakit/SrcModel.hpp"
#include "idakit/SrcTypes.hpp"

#include <Facts.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <numeric>
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

using Row = SlotRow;

class Signatures : private SlotRowChecks
{
public:
	Signatures(const SrcModel& model, SrcTypes& types, exefacts::Facts& facts) : SlotRowChecks(model, types, facts)
	{
		for (const auto& [name, overloads] : model.pdbFunctions())
			for (const SrcModel::PdbFunction& pdb : overloads)
				m_pdb[comparedName(name)].push_back(&pdb);
	}

	std::vector<Row> run();

private:
	struct Context
	{
		const SrcModel::Function* function = nullptr;
		SrcTypes::Function type;
		std::optional<SrcTypes::Function> pdb;  // the paired PDB function's
		const SrcModel::PdbFunction* pdbFunction = nullptr;
		size_t shift = 0;  // src's hidden result pointer: src's parameter k + shift is the PDB's k
		uint64_t ea = 0;
		std::string fold;  // another src function at the address
	};

	void judge(const Context& c, Row& row, const std::string& srcKey, const std::string& pdbKey);
	void judgeCount(const Context& c, std::vector<Row>& rows, const std::vector<std::string>& slots);
	std::string paramName(const Context& c, size_t k) const;
	void keep(const Context& c, Row& row, const std::string& fact);

	std::unordered_map<std::string, std::vector<const SrcModel::PdbFunction*>> m_pdb;  // by comparedName
};

std::string Signatures::paramName(const Context& c, size_t k) const
{
	return k < c.function->params.size() ? c.function->params[k] : "";
}

// a keep row: the comment names the slot as srcdb's markers read it (Model.cpp), the parameter by src's name
void Signatures::keep(const Context& c, Row& row, const std::string& fact)
{
	row.verdict = "keep";
	row.reason = fact;
	std::string form;
	if (row.slot == "ret")
		form = "returns " + row.proposed;
	else
	{
		std::string name = paramName(c, std::stoul(row.slot));
		if (name.empty())
		{
			row.verdict = "open";
			row.reason = fact + "; the parameter has no name a `// PDB:` comment can give";
			return;
		}
		form = row.key.empty() ? row.proposed + " " + name : spelled(row.key, name);
	}
	row.comment = "// PDB: " + form + "; the exe " + fact + ".";
}

void Signatures::judge(const Context& c, Row& row, const std::string& srcKey, const std::string& pdbKey)
{
	std::string missing;
	std::optional<std::string> target = toSrc(pdbKey, missing);
	row.proposed = target ? spelled(*target) : spelled(pdbKey);
	row.key = target.value_or("");
	auto open = [&](const std::string& why) {
		row.verdict = "open";
		row.reason = why;
	};
	// an Xbox 360 type of the PDB's (src's alias for it included: ida_type_aliases.tsv's NiXenonRenderer is
	// NiDX9Renderer) is the PC exe's other type
	std::smatch platform;
	if (std::regex_search(pdbKey, platform, platformPattern()))
	{
		row.key.clear();
		row.proposed = spelled(pdbKey);
		keep(c, row, "is the PC build (src's " + row.src + ")");
		return;
	}
	if (!target)
		return open("the PDB's type is not in src: " + missing);
	if (normal(srcKey) == normal(*target) || normal(resolved(srcKey)) == normal(resolved(*target)))
	{
		row.verdict = "spelling";
		row.reason = normal(srcKey) == normal(pdbKey) || normal(resolved(srcKey)) == normal(resolved(pdbKey))
						 ? "the same type: srcdb's compare to learn the spelling"
						 : "the same type through ida_type_aliases.tsv's name for the PDB's";
		return;
	}
	if (!c.fold.empty())
		return open("fold: " + c.fold + " is the same exe function");
	std::string sk = kindOf(srcKey), pk = kindOf(*target);
	std::optional<int64_t> ss = size(srcKey), ps = size(*target);
	bool ret = row.slot == "ret";
	// the PDB's narrower integer or bool where src has the width the exe keeps (declarations.md "Return widths",
	// "Parameter widths"): keep with the exe's fact, open where the exe's code does not show it
	auto narrowed = [&]() {
		std::string unshown;
		std::string fact = narrowing(*c.function, c.ea, row.slot, *target, unshown);
		if (!fact.empty())
			return keep(c, row, fact);
		if (!unshown.empty())
			return open("the exe's code does not show the width it needs: " + unshown);
		std::string what = *ps == 1 ? "a byte" : "a word";
		row.verdict = "apply";
		if (ret)
			row.reason = pk == "bool" ? "every return path leaves 0 or 1 in al" : "every return path writes " + what;
		else
			row.reason = "the exe reads no more than " + what + " of it and passes it on whole nowhere";
	};
	auto returnWidth = [&]() -> std::optional<exefacts::ReturnWidth> {
		try
		{
			return c.ea ? m_facts.returnWidth(uint32_t(c.ea)) : std::nullopt;
		}
		catch (const std::exception&)
		{
			return std::nullopt;
		}
	};
	auto pointerLike = [](const std::string& k) { return k == "ptr" || k == "ref"; };
	if (pointerLike(sk) && pointerLike(pk))
	{
		std::string so = resolved(m_types.parts(resolved(srcKey)).of), po = resolved(m_types.parts(resolved(*target)).of);
		SrcTypes::Parts sp = m_types.parts(so), pp = m_types.parts(po);
		if (pp.kind == "builtin" && pp.name == "void" && !(sp.kind == "builtin" && sp.name == "void"))
			return open("the PDB's void pointer would weaken src's " + row.src);
		bool sPointer = sp.kind == "ptr" || sp.kind == "lref", pPointer = pp.kind == "ptr" || pp.kind == "lref";
		if (sPointer != pPointer && !(sp.kind == "builtin" && sp.name == "void"))
			return open("the pointees' levels of indirection differ: the exe's accesses decide");
		if (sp.kind == "record" && pp.kind == "record" && sp.name != pp.name && derives(sp.name, pp.name))
			return open("the PDB's " + pp.name + " is a base of src's " + sp.name + ": it would weaken src's type");
		row.verdict = "apply";
		row.reason = sk == pk ? "a pointer for a pointer" : "the PDB's " + std::string(pk == "ref" ? "reference" : "pointer");
		return;
	}
	if (pointerLike(sk) && (pk == "int" || pk == "enum" || pk == "bool"))
		return open("the PDB's integer for src's pointer: a pointer never becomes an integer");
	if (pk == "void" || sk == "void")
	{
		if (!ret)
			return open("a void parameter");
		std::optional<exefacts::ReturnWidth> width = returnWidth();
		if (pk == "void")
		{
			if (width && width->width > 0)
				return keep(c, row, "writes eax on every return path (" + std::to_string(width->width) + " bytes)");
			return open("the PDB returns nothing and the exe's eax at the return is not shown");
		}
		if (pk == "float" || pk == "record")
			return open("the PDB returns a value in ST0 or memory where src returns nothing");
		if (width && ps && width->width >= *ps)
		{
			row.verdict = "apply";
			row.reason = "every return path writes " + std::to_string(width->width) + " bytes of eax";
			return;
		}
		return open("src returns nothing and the exe's eax at the return is not shown");
	}
	if (!ss || !ps)
		return open("a size src does not know");
	auto scalar = [](const std::string& k) { return k == "int" || k == "enum" || k == "bool" || k == "char" || k == "wchar_t"; };
	if (scalar(sk) && scalar(pk))
	{
		if (pk == "enum")
		{
			if (*ps != *ss)
				return open("src's enum " + row.proposed + " is " + std::to_string(*ps) + " bytes, the slot " +
							std::to_string(*ss));
			row.verdict = "apply";
			row.reason = "an enum of the slot's width";
			return;
		}
		if (sk == "enum")
			return open("the PDB's integer for src's enum " + row.src);
		if (narrows(srcKey, *target))
			return narrowed();
		if (*ps == *ss)
		{
			if (sk == "int" && pk == "int")
				return open("signedness differs: the exe's compares decide");
			row.verdict = "apply";
			row.reason = "the same width";
			return;
		}
		return open("the PDB's type is wider than the slot (" + std::to_string(*ps) + " bytes, src " +
					std::to_string(*ss) + ")");
	}
	if ((sk == "int" || sk == "enum") && pointerLike(pk) && *ss == 4)
	{
		row.verdict = "apply";
		row.reason = "the PDB's pointer for src's integer of its width";
		return;
	}
	if (*ss != *ps)
		return open("sizes differ: src " + std::to_string(*ss) + " bytes, the PDB " + std::to_string(*ps));
	if (sk == "float" || pk == "float")
		return open(sk == pk ? "float against double: x87 precision is the asm's" : "a float against an integer");
	if (sk == "record" && pk == "record")
	{
		row.verdict = "apply";
		row.reason = "a record of the same size";
		return;
	}
	open("src's " + sk + " for the PDB's " + pk);
}

// A function the PDB has only with another parameter count (no pairing): keep where the exe's stack bytes are src's
// and not the PDB's, else open
void Signatures::judgeCount(const Context& c, std::vector<Row>& rows, const std::vector<std::string>& slots)
{
	std::vector<const SrcModel::PdbFunction*> pdbs;
	if (auto found = m_pdb.find(comparedName(c.function->name)); found != m_pdb.end())
		pdbs = found->second;
	std::string counts, pdbForm;
	bool oneCount = !pdbs.empty();
	for (const SrcModel::PdbFunction* pdb : pdbs)
		if (std::optional<SrcTypes::Function> f = SrcTypes::function(pdb->type))
		{
			if (counts.find(" " + std::to_string(f->params.size()) + ",") == std::string::npos)
				counts += " " + std::to_string(f->params.size()) + ",";
			oneCount = oneCount && f->params.size() == SrcTypes::function(pdbs[0]->type)->params.size();
		}
	std::string reason, fact;
	bool sameCount = false;
	for (const SrcModel::PdbFunction* pdb : pdbs)
		if (SrcTypes::function(pdb->type)->params.size() == c.type.params.size())
			sameCount = true;
	if (pdbs.empty())
		reason = "no PDB function of its name has a type src can read";
	else if (sameCount)
		reason = "its overloads cannot be paired with the PDB's" + (c.fold.empty() ? "" : " (fold: " + c.fold + ")");
	else
		reason = "parameter count: src " + std::to_string(c.type.params.size()) + ", the PDB" +
				 counts.substr(0, counts.size() - 1);
	// the exe's stack bytes against src's and the PDB's, for the one PDB signature of another count
	if (!pdbs.empty() && !sameCount && oneCount && pdbs.size() == 1 && c.ea && c.fold.empty())
	{
		std::optional<SrcTypes::Function> f = SrcTypes::function(pdbs[0]->type);
		int64_t bytes = 0;
		bool known = true;
		for (const std::string& p : f->params)
		{
			std::string missing;
			std::optional<std::string> s = toSrc(p, missing);
			std::optional<int64_t> n = s ? size(*s) : std::nullopt;
			known = known && n;
			bytes += n ? (*n + 3) / 4 * 4 : 0;
		}
		std::string missing;
		std::optional<std::string> r = toSrc(f->ret, missing);
		if (r && kindOf(*r) == "record")
			bytes += 4;  // MSVC's hidden result pointer, where the PDB returns a record
		const SrcModel::Abi* abi = m_model.abi(c.function->mangled);
		std::optional<uint32_t> pops;
		try
		{
			pops = m_facts.pops(uint32_t(c.ea));
		}
		catch (const std::exception&)
		{
		}
		if (known && abi && pops && (abi->cc == "thiscall" || abi->cc == "stdcall") && *pops == abi->stackBytes &&
			bytes != abi->stackBytes)
			fact = "pops " + std::to_string(*pops) + " bytes of arguments (ret " + std::to_string(*pops) + ")";
		// a cdecl's callee may leave arguments unread: only a read past the PDB's arguments contradicts them
		else if (known && abi && pops && abi->cc == "cdecl" && *pops == 0 && bytes < abi->stackBytes)
		{
			std::optional<uint32_t> past;
			bool taken = false;
			try
			{
				for (const exefacts::ArgumentAccess& a : m_facts.argumentAccesses(uint32_t(c.ea)))
				{
					taken = taken || a.kind == exefacts::ArgumentAccess::Address;
					if (a.kind == exefacts::ArgumentAccess::Read && a.offset + a.size > bytes && !past)
						past = a.pc;
				}
			}
			catch (const std::exception&)
			{
				taken = true;
			}
			if (past && !taken)
				fact = "reads its arguments past the PDB's " + std::to_string(bytes) + " bytes (at " + hexText(*past) +
					   ")";
		}
		if (!fact.empty())
		{
			std::vector<std::string> params;
			for (size_t k = 0; k < f->params.size(); ++k)
			{
				std::string name = k < pdbs[0]->params.size() ? pdbs[0]->params[k] : "";
				std::string s = toSrc(f->params[k], missing).value_or(f->params[k]);
				params.push_back(spelled(s, name));
			}
			std::string ret = spelled(toSrc(f->ret, missing).value_or(f->ret));
			pdbForm = ret + " " + c.function->name.substr(c.function->name.rfind("::") == std::string::npos
															  ? 0
															  : c.function->name.rfind("::") + 2) +
					  "(" + std::accumulate(params.begin(), params.end(), std::string(),
											[](const std::string& a, const std::string& b) {
												return a.empty() ? b : a + ", " + b;
											}) +
					  ")";
		}
	}
	for (const std::string& slot : slots)
	{
		Row& row = rows.emplace_back();
		row.slot = slot;
		row.src = slot == "ret" ? spelled(c.type.ret) : slot == "this" ? "this" : spelled(c.type.params[std::stoul(slot)]);
		if (!fact.empty())
		{
			row.verdict = "keep";
			row.reason = reason + "; the exe " + fact;
			row.proposed = pdbForm;
			row.comment = "// PDB: " + pdbForm + "; the exe " + fact + ".";
		}
		else
		{
			row.verdict = "open";
			row.reason = reason;
		}
	}
}

std::vector<Row> Signatures::run()
{
	std::vector<Row> rows;
	size_t pairs = 0;
	std::vector<const SrcModel::Function*> functions;
	for (const auto& [mangled, f] : m_model.functions())
		functions.push_back(&f);
	std::sort(functions.begin(), functions.end(),
			  [](const SrcModel::Function* a, const SrcModel::Function* b) { return a->mangled < b->mangled; });
	// finish-line.py's number 4: a PDB-named function with a ref at an address (its own or a call from its body)
	std::set<std::string> addressed;
	for (const auto& [ea, refs] : m_model.refs())
		for (const SrcModel::Ref& r : refs)
			if (!r.function.empty())
				addressed.insert(r.function);
	for (const SrcModel::Function* f : functions)
	{
		if (f->provenance != SrcModel::Provenance::Pdb || !addressed.count(f->mangled))
			continue;
		std::optional<SrcTypes::Function> type = SrcTypes::function(f->type);
		if (!type)
			continue;
		bool member = !f->cls.empty() && !f->isStatic;
		// a usercall's parameters are in src's register order, not the PDB's: srcdb's pairing by position says
		// nothing of them
		const std::map<int64_t, std::string>* user = userLocations(f->mangled);
		std::vector<std::string> slots;
		for (std::string slot : {std::string("ret"), std::string("this")})
			if ((slot != "this" || member) && !m_model.pdbTyped("param", f->mangled, slot) &&
				!m_model.pdbTyped("recorded", f->mangled, slot))
				slots.push_back(slot);
		for (size_t k = 0; k < type->params.size(); ++k)
			if ((user || !m_model.pdbTyped("param", f->mangled, std::to_string(k))) &&
				!m_model.pdbTyped("recorded", f->mangled, std::to_string(k)))
				slots.push_back(std::to_string(k));
		if (slots.empty())
			continue;
		Context c;
		c.function = f;
		c.type = *type;
		if (auto at = m_address.find(f->mangled); at != m_address.end())
		{
			c.ea = at->second;
			for (const std::string& other : m_declaring[c.ea])
				if (other != f->mangled)
					c.fold = c.fold.empty() ? m_model.function(other) ? m_model.function(other)->name : other : c.fold;
		}
		const std::string* pair = m_model.pdbTyped("pair", f->mangled, "");
		if (!pair && (pair = m_model.pdbTyped("pair", f->mangled, "hidden")))
			c.shift = 1;
		pairs += pair != nullptr;
		size_t first = rows.size();
		if (!pair || !(c.pdb = SrcTypes::function(*pair)))
			judgeCount(c, rows, slots);
		else
		{
			const SrcModel::PdbFunction* pdb = nullptr;
			if (auto found = m_pdb.find(comparedName(f->name)); found != m_pdb.end())
				for (const SrcModel::PdbFunction* p : found->second)
					if (p->type == *pair)
						pdb = p;
			for (const std::string& slot : slots)
			{
				Row& row = rows.emplace_back();
				row.slot = slot;
				if (slot == "this")
				{
					row.src = spelled(type->isConst ? "ptr(const(record:" + f->cls + "))" : "ptr(record:" + f->cls + ")");
					row.proposed = pdb ? pdb->isStatic || pdb->cls.empty() ? "static" : pdb->cls : "";
					row.verdict = "open";
					row.reason = pdb && (pdb->isStatic || pdb->cls.empty())
									 ? "the PDB's function has no this"
									 : "this: src's " + f->cls + ", the PDB's " + row.proposed + " (an owner question)";
					continue;
				}
				size_t k = slot == "ret" ? 0 : std::stoul(slot);
				std::string srcKey = slot == "ret" ? type->ret : type->params[k];
				row.src = spelled(srcKey);
				if (c.shift && (slot == "ret" || k == 0))
				{
					std::string missing;
					std::optional<std::string> r = toSrc(c.pdb->ret, missing);
					row.proposed = spelled(r.value_or(c.pdb->ret));
					row.key = r.value_or("");
					row.slot = "ret";
					keep(c, row, "returns it through the hidden result pointer " +
									 (paramName(c, 0).empty() ? std::string("src passes first") : paramName(c, 0)));
					row.slot = slot;
					continue;
				}
				if (user && slot != "ret")
				{
					// a usercall's parameter is the PDB's of its name, at the location src gives it
					std::string name = paramName(c, k);
					auto at = user->find(int64_t(k));
					std::string where = name + "@" + (at == user->end() ? std::string("stack") : at->second);
					std::vector<size_t> named;
					for (size_t i = 0; pdb && i < pdb->params.size() && i < c.pdb->params.size(); ++i)
						if (!name.empty() && pdb->params[i] == name)
							named.push_back(i);
					if (named.size() != 1)
					{
						row.verdict = "open";
						row.reason = "a usercall, its parameters in register order: " +
									 std::string(named.empty() ? "no PDB parameter is named " : "PDB parameters named ") +
									 where + (named.empty() ? "" : " twice");
						continue;
					}
					judge(c, row, srcKey, c.pdb->params[named[0]]);
					// the same type as the PDB's of its name: nothing to write
					if (row.verdict == "spelling")
						rows.pop_back();
					continue;
				}
				judge(c, row, srcKey, slot == "ret" ? c.pdb->ret : c.pdb->params[k - c.shift]);
			}
		}
		for (size_t i = first; i < rows.size(); ++i)
		{
			rows[i].mangled = f->mangled;
			rows[i].function = f->name;
			rows[i].address = c.ea ? hexText(c.ea) : "";
		}
	}
	if (!pairs)
		throw Error("declare --pdb-signatures: the model has no pdb_typed pair rows (index it with this srcdb)");
	// a virtual's slot changes in its whole family: every member src has gets the type but one whose type the PDB
	// confirmed (pdb_typed); src's own type, where the PDB has none, gives way
	judgeFamilies(rows, [&](const SrcModel::Function& member, const std::string& slot, const std::string& own) {
		return m_model.pdbTyped("param", member.mangled, slot)
				   ? "the PDB's overrides disagree (" + member.name + " has " + spelled(own) + ")"
				   : std::string();
	});
	judgeOverloads(rows);
	for (Row& row : rows)
		row.id = row.mangled + "#" + row.slot;
	return rows;
}

}  // namespace

int declarePdbSignatures(const SrcModel& model, SrcTypes& types, const std::filesystem::path& out)
{
	exefacts::Program program(".");
	exefacts::Facts facts(program);
	std::vector<Row> rows = Signatures(model, types, facts).run();
	std::ofstream table(out / "pdb-signatures.tsv", std::ios::trunc);
	table << "id\taddress\tfunction\tslot\tsrc\tpdb\tverdict\treason\tcomment\tkey\tfamily\n";
	std::map<std::string, size_t> verdicts;
	std::set<std::string> functions;
	for (const Row& r : rows)
	{
		table << cell(r.id) << '\t' << r.address << '\t' << cell(r.function) << '\t' << r.slot << '\t' << cell(r.src)
			  << '\t' << cell(r.proposed) << '\t' << r.verdict << '\t' << cell(r.reason) << '\t' << cell(r.comment)
			  << '\t' << cell(r.key) << '\t' << cell(r.family) << '\n';
		++verdicts[r.verdict];
		functions.insert(r.mangled);
	}
	std::string summary;
	for (const auto& [verdict, count] : verdicts)
		summary += " " + std::to_string(count) + " " + verdict;
	table.close();
	std::fprintf(stderr, "idakit declare --pdb-signatures: %zu slots of %zu functions:%s (%s)\n", rows.size(),
				 functions.size(), summary.c_str(), (out / "pdb-signatures.tsv").c_str());
	return 0;
}

}  // namespace idakit
