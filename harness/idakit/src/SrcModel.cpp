#include "idakit/SrcModel.hpp"

#include "idakit/Database.hpp"

#include <algorithm>
#include <fstream>
#include <regex>
#include <sqlite3.h>

namespace idakit
{

namespace
{

class Query
{
public:
	Query(sqlite3* db, const char* sql)
	{
		if (sqlite3_prepare_v2(db, sql, -1, &m_statement, nullptr) != SQLITE_OK)
			throw Error(std::string("idakit: src model: ") + sqlite3_errmsg(db) + " in " + sql);
	}
	~Query() { sqlite3_finalize(m_statement); }
	Query(const Query&) = delete;
	Query& operator=(const Query&) = delete;

	bool step() { return sqlite3_step(m_statement) == SQLITE_ROW; }
	std::string text(int column) const
	{
		const unsigned char* value = sqlite3_column_text(m_statement, column);
		return value ? reinterpret_cast<const char*>(value) : "";
	}
	int64_t integer(int column) const { return sqlite3_column_int64(m_statement, column); }
	bool flag(int column) const { return integer(column) != 0; }
	bool null(int column) const { return sqlite3_column_type(m_statement, column) == SQLITE_NULL; }

private:
	sqlite3_stmt* m_statement = nullptr;
};

class Connection
{
public:
	explicit Connection(const std::string& path)
	{
		// immutable: the model is a finished file (srcdb renames it into place), read without locks
		std::string uri = "file:" + path + "?immutable=1";
		if (sqlite3_open_v2(uri.c_str(), &m_db, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI, nullptr) != SQLITE_OK)
		{
			std::string message = m_db ? sqlite3_errmsg(m_db) : "out of memory";
			sqlite3_close_v2(m_db);
			throw Error("idakit: cannot open the src model " + path + ": " + message);
		}
	}
	~Connection() { sqlite3_close_v2(m_db); }
	sqlite3* get() const { return m_db; }

private:
	sqlite3* m_db = nullptr;
};

}  // namespace

SrcModel::SrcModel(const std::string& path) : m_path(path)
{
	Connection connection(path);
	sqlite3* db = connection.get();
	{
		Query version(db, "SELECT value FROM meta WHERE name = 'version'");
		if (!version.step() || version.text(0) != std::to_string(kFormatVersion))
			throw Error("idakit: " + path + " is not a src model of format " + std::to_string(kFormatVersion) +
						" (rebuild it: srcdb index)");
	}
	{
		Query q(db, "SELECT key, kind, name, of, count, size, signed, float, cc, ret, variadic, const, class "
					"FROM type");
		while (q.step())
		{
			Type t;
			t.key = q.text(0);
			t.kind = q.text(1);
			t.name = q.text(2);
			t.of = q.text(3);
			t.count = q.integer(4);
			t.size = q.integer(5);
			t.isSigned = q.flag(6);
			t.isFloat = q.flag(7);
			t.cc = q.text(8);
			t.ret = q.text(9);
			t.variadic = q.flag(10);
			t.isConst = q.flag(11);
			t.cls = q.text(12);
			std::string key = t.key;
			m_types.emplace(std::move(key), std::move(t));
		}
		Query params(db, "SELECT type, param FROM type_param ORDER BY type, idx");
		while (params.step())
			if (auto found = m_types.find(params.text(0)); found != m_types.end())
				found->second.params.push_back(params.text(1));
	}
	{
		Query q(db, "SELECT name, file, tag, size, vfptr, template, spelled, trivial_copy, line, end_line, align, "
					"abstract FROM record");
		while (q.step())
		{
			Record r;
			r.name = q.text(0);
			r.file = q.text(1);
			r.tag = q.text(2);
			r.size = q.integer(3);
			r.vfptr = q.flag(4);
			r.pattern = q.text(5);
			r.spelled = q.text(6);
			r.trivialCopy = q.flag(7);
			r.line = q.integer(8);
			r.endLine = q.integer(9);
			r.align = q.integer(10);
			r.abstract = q.flag(11);
			std::string name = r.name;
			m_records.emplace(std::move(name), std::move(r));
		}
		Query fields(db, "SELECT record, name, type, offset, bits FROM field ORDER BY record, idx");
		while (fields.step())
			if (auto found = m_records.find(fields.text(0)); found != m_records.end())
			{
				Field f{fields.text(1), fields.text(2), fields.integer(3), std::nullopt};
				if (!fields.null(4))
					f.bits = fields.integer(4);
				found->second.fields.push_back(std::move(f));
			}
		Query bases(db, "SELECT record, type, offset, virtual FROM base ORDER BY record, idx");
		while (bases.step())
			if (auto found = m_records.find(bases.text(0)); found != m_records.end())
			{
				// a base named through a typedef (`class X : public msvc9::string`) is the record the typedef
				// names, resolved once the typedefs are read
				found->second.bases.push_back({bases.text(1), bases.integer(2), bases.flag(3)});
			}
		Query vtables(db, "SELECT record, offset FROM vtable ORDER BY record, idx");
		while (vtables.step())
			if (auto found = m_records.find(vtables.text(0)); found != m_records.end())
				found->second.vtables.push_back({vtables.integer(1), {}});
		Query slots(db, "SELECT record, vtable, kind, function, name, pure FROM slot ORDER BY record, vtable, idx");
		while (slots.step())
			if (auto found = m_records.find(slots.text(0)); found != m_records.end())
			{
				size_t table = static_cast<size_t>(slots.integer(1));
				if (table < found->second.vtables.size())
					found->second.vtables[table].slots.push_back(
						{slots.text(2), slots.text(3), slots.text(4), slots.flag(5)});
			}
		Query pins(db, "SELECT DISTINCT record FROM pin");
		while (pins.step())
			if (auto found = m_records.find(pins.text(0)); found != m_records.end())
				found->second.pinned = true;
		Query usings(db, "SELECT record, function FROM using_member ORDER BY record, idx");
		while (usings.step())
			if (auto found = m_records.find(usings.text(0)); found != m_records.end())
				found->second.usingMembers.push_back(usings.text(1));
	}
	{
		Query q(db, "SELECT name, underlying, file, scoped, line FROM enum");
		while (q.step())
			m_enums.emplace(q.text(0), Enum{q.text(0), q.text(1), q.text(2), q.integer(4), q.flag(3), {}});
		Query enumerators(db, "SELECT enum, name, value FROM enumerator ORDER BY enum, idx");
		while (enumerators.step())
			if (auto found = m_enums.find(enumerators.text(0)); found != m_enums.end())
				found->second.enumerators.push_back({enumerators.text(1), enumerators.integer(2)});
		Query typedefs(db, "SELECT name, file, target FROM typedef");
		while (typedefs.step())
			m_typedefs.emplace(typedefs.text(0), Typedef{typedefs.text(0), typedefs.text(1), typedefs.text(2)});
		for (auto& [name, record] : m_records)
			for (Base& b : record.bases)
			{
				const Type* t = type(storage(b.record));
				if (t && t->kind == "record")
					b.record = t->name;
			}
	}
	{
		// per class, its .cpp files by member definitions held, most first: the first row of each class wins
		Query q(db, "SELECT f.class, d.file, count(*) AS n FROM definition d JOIN function f ON f.mangled = d.mangled "
					"WHERE f.class != '' AND d.file LIKE '%.cpp' GROUP BY f.class, d.file ORDER BY f.class, n DESC");
		while (q.step())
			m_classSources.emplace(q.text(0), q.text(1));
		Query tus(db, "SELECT path FROM tu");
		while (tus.step())
			m_tus.insert(tus.text(0));
		Query definitions(db, "SELECT mangled, file, line, end_line, instance, emitted FROM definition");
		while (definitions.step())
			m_definitions.emplace(definitions.text(0),
								  Definition{definitions.text(1), definitions.integer(2), definitions.integer(3),
											 definitions.flag(4), definitions.flag(5)});
		Query specializations(db, "SELECT mangled, file, line, end_line, definition, inline FROM specialization "
								  "ORDER BY mangled, file, line");
		while (specializations.step())
			m_specializations[specializations.text(0)].push_back(
				Specialization{specializations.text(1), specializations.integer(2), specializations.integer(3),
							   specializations.flag(4), specializations.flag(5)});
	}
	{
		Query q(db, "SELECT mangled, name, type, class, static, virtual, file, structor, empty, line, provenance, "
					"access, noreturn FROM function");
		while (q.step())
		{
			Function f;
			f.mangled = q.text(0);
			f.name = q.text(1);
			f.type = q.text(2);
			f.cls = q.text(3);
			f.isStatic = q.flag(4);
			f.isVirtual = q.flag(5);
			f.file = q.text(6);
			f.structor = q.text(7);
			f.empty = q.flag(8);
			f.line = q.integer(9);
			std::string provenance = q.text(10);
			f.provenance = provenance == "pdb"		? Provenance::Pdb
						   : provenance == "inferred" ? Provenance::Inferred
						   : provenance == "library"  ? Provenance::Library
						   : provenance == "guess"	  ? Provenance::Guess
													  : Provenance::Src;
			f.access = q.text(11);
			f.noreturn = q.flag(12);
			std::string key = f.mangled;
			m_functions.emplace(std::move(key), std::move(f));
		}
		for (const auto& [mangled, f] : m_functions)
		{
			m_functionsByName[f.name].push_back(&f);
			if (!f.structor.empty())
				m_structors[f.cls].push_back(&f);
			// a conversion function: `C::operator T` (its name after the class's, `operator ` and a type)
			else if (!f.cls.empty() && f.name.size() > f.cls.size() + 2 &&
					 f.name.compare(0, f.cls.size(), f.cls) == 0 &&
					 f.name.compare(f.cls.size() + 2, 9, "operator ") == 0 &&
					 f.name.compare(f.cls.size() + 11, 3, "new") != 0 &&
					 f.name.compare(f.cls.size() + 11, 6, "delete") != 0)
				m_conversions[f.cls].push_back(&f);
		}
		Query params(db, "SELECT function, name FROM param ORDER BY function, idx");
		while (params.step())
			if (auto found = m_functions.find(params.text(0)); found != m_functions.end())
				found->second.params.push_back(params.text(1));
		Query slots(db, "SELECT function, slot, inferred FROM function_slot "
						"ORDER BY function, slot = 'ret' DESC, CAST(slot AS INTEGER)");
		while (slots.step())
			if (auto found = m_functions.find(slots.text(0)); found != m_functions.end())
			{
				found->second.slots.push_back(slots.text(1));
				if (slots.flag(2))
					found->second.inferredSlots.insert(slots.text(1));
			}
		Query defaults(db, "SELECT function, required FROM default_args");
		while (defaults.step())
			if (auto found = m_functions.find(defaults.text(0)); found != m_functions.end())
				found->second.required = defaults.integer(1);
		// a declaration without parameter names takes its definition's
		Query defined(db, "SELECT definition, idx, name FROM definition_param WHERE name != ''");
		while (defined.step())
			if (auto found = m_functions.find(defined.text(0)); found != m_functions.end())
			{
				size_t index = static_cast<size_t>(defined.integer(1));
				std::vector<std::string>& names = found->second.params;
				if (index < names.size() && names[index].empty())
					names[index] = defined.text(2);
			}
		Query abis(db, "SELECT function, cc, ret, stack_bytes FROM abi");
		while (abis.step())
			m_abis.emplace(abis.text(0), Abi{abis.text(1), abis.text(2), abis.integer(3)});
		Query accessors(db, "SELECT function, kind, field, param FROM accessor");
		while (accessors.step())
			if (auto found = m_functions.find(accessors.text(0)); found != m_functions.end())
				m_accessors[found->second.cls].push_back(
					Accessor{&found->second, accessors.text(1), accessors.text(2),
							 accessors.null(3) ? -1 : accessors.integer(3)});
		Query steps(db, "SELECT function, steps FROM steps");
		while (steps.step())
			m_steps.emplace(steps.text(0), steps.text(1));
	}
	// a forwarder's arguments, by its ref's location and address
	std::map<std::tuple<std::string, int64_t, int64_t, uint64_t>, std::vector<ForwardedArgument>> arguments;
	{
		Query q(db, "SELECT file, line, col, address, idx, name, literal, value FROM ref_param");
		while (q.step())
		{
			auto& list = arguments[{q.text(0), q.integer(1), q.integer(2), static_cast<uint64_t>(q.integer(3))}];
			size_t idx = static_cast<size_t>(q.integer(4));
			if (list.size() <= idx)
				list.resize(idx + 1);
			list[idx] = {q.text(5), q.flag(6), q.null(7) ? std::nullopt : std::optional<int64_t>(q.integer(7))};
		}
	}
	// a usercall's parameter locations, by its ref's location and address
	std::map<std::tuple<std::string, int64_t, int64_t, uint64_t>, std::map<int64_t, std::string>> locations;
	for (Query q(db, "SELECT file, line, col, address, idx, location FROM ref_location"); q.step();)
		locations[{q.text(0), q.integer(1), q.integer(2), static_cast<uint64_t>(q.integer(3))}][q.integer(4)] =
			q.text(5);
	// the instances' members made from each class template member (srcdb's pattern key); one src spells an explicit
	// specialization of is its own code, none of the member's
	std::unordered_map<std::string, std::vector<const Function*>> instances;
	{
		Query q(db, "SELECT pattern, function FROM function_pattern ORDER BY pattern, function");
		while (q.step())
			if (const Function* f = function(q.text(1)); f && !m_specializations.contains(f->mangled))
				instances[q.text(0)].push_back(f);
	}
	{
		Query q(db, "SELECT file, line, col, kind, form, address, target, function, type, local, context FROM ref "
					"ORDER BY address, file, line, col");
		while (q.step())
		{
			Ref r;
			r.file = q.text(0);
			r.line = q.integer(1);
			r.col = q.integer(2);
			r.kind = q.text(3);
			r.form = q.text(4);
			r.address = static_cast<uint64_t>(q.integer(5));
			r.target = q.text(6);
			r.function = q.text(7);
			r.type = q.text(8);
			r.local = q.flag(9);
			if (r.kind == "fold")
				r.context = q.text(10);
			if (auto found = arguments.find({r.file, r.line, r.col, r.address}); found != arguments.end())
				r.arguments = std::move(found->second);
			if (auto found = locations.find({r.file, r.line, r.col, r.address}); found != locations.end())
				r.locations = std::move(found->second);
			// a function's own addresses: its registrations (a C++ structor's included) and a kept body's FNVD_ADDRESS
			if (r.kind == "body" || r.kind == "constructor" || r.kind == "destructor" || r.kind == "address")
			{
				m_bodyAddresses[r.function].push_back(r.address);
				m_bodyAddressesByName[r.target].push_back(r.address);
			}
			std::vector<Ref>& at = m_refs[r.address];
			// a forwarder in a class template's member (srcdb's `pattern:` key, a pack template's aside, which has a
			// function row) forwards for each instance's member made from it: one ref per instance src completes (none:
			// no function), as if each instance spelled it
			if (r.function.starts_with("pattern:") && !function(r.function))
			{
				auto made = instances.find(r.function);
				r.function.clear();
				r.generic = true;
				if (made != instances.end())
				{
					for (const Function* f : made->second)
					{
						Ref instance = r;
						instance.target = f->name;
						instance.function = f->mangled;
						at.push_back(std::move(instance));
					}
					continue;
				}
			}
			at.push_back(std::move(r));
		}
	}
	{
		std::unordered_map<int64_t, std::vector<std::string>> names;
		Query params(db, "SELECT function, idx, name FROM pdb_param ORDER BY function, idx");
		while (params.step())
			names[params.integer(0)].push_back(params.text(2));
		Query q(db, "SELECT id, name, class, type, static, code_size FROM pdb_function ORDER BY id");
		while (q.step())
		{
			PdbFunction f{q.text(1), q.text(2), q.text(3), q.flag(4), q.integer(5), {}};
			if (auto found = names.find(q.integer(0)); found != names.end())
				f.params = std::move(found->second);
			m_pdbFunctions[f.name].push_back(std::move(f));
		}
	}
	for (Query q(db, "SELECT record, name, type FROM pdb_field ORDER BY record, name, type"); q.step();)
		m_pdbFields[q.text(0)].push_back(PdbField{q.text(1), q.text(2)});
	for (Query q(db, "SELECT kind, key, item, type FROM pdb_typed"); q.step();)
		m_pdbTyped.emplace(pdbTypedKey(q.text(0), q.text(1), q.text(2)), q.text(3));
}

std::string SrcModel::pdbTypedKey(const std::string& kind, const std::string& key, const std::string& item)
{
	return kind + '\n' + key + '\n' + item;
}

const std::string* SrcModel::pdbTyped(const std::string& kind, const std::string& key, const std::string& item) const
{
	return find(m_pdbTyped, pdbTypedKey(kind, key, item));
}

std::vector<SrcModel::Use> SrcModel::uses(const std::string& mangled) const
{
	Connection connection(m_path);
	sqlite3_stmt* statement = nullptr;
	const char* sql = "SELECT file, line, col FROM function_use WHERE function = ? ORDER BY file, line, col";
	if (sqlite3_prepare_v2(connection.get(), sql, -1, &statement, nullptr) != SQLITE_OK)
		throw Error(std::string("idakit: src model: ") + sqlite3_errmsg(connection.get()) + " in " + sql);
	sqlite3_bind_text(statement, 1, mangled.c_str(), -1, SQLITE_TRANSIENT);
	std::vector<Use> out;
	while (sqlite3_step(statement) == SQLITE_ROW)
		out.push_back({reinterpret_cast<const char*>(sqlite3_column_text(statement, 0)),
					   sqlite3_column_int64(statement, 1), sqlite3_column_int64(statement, 2)});
	sqlite3_finalize(statement);
	return out;
}

const std::vector<SrcModel::Ref>& SrcModel::refsAt(uint64_t address) const
{
	static const std::vector<Ref> none;
	auto found = m_refs.find(address);
	return found == m_refs.end() ? none : found->second;
}

bool SrcModel::registers(uint64_t address) const
{
	const std::vector<Ref>& refs = refsAt(address);
	return std::any_of(refs.begin(), refs.end(), [](const Ref& r) {
		return r.kind == "body" || r.kind == "constructor" || r.kind == "destructor";
	});
}

bool SrcModel::handPorted(uint64_t address) const
{
	const std::vector<Ref>& refs = refsAt(address);
	return std::any_of(refs.begin(), refs.end(), [&](const Ref& r) {
		if (r.kind != "body" && r.kind != "constructor" && r.kind != "destructor")
			return false;
		const Definition* d = definition(r.function);
		return d && !d->emitted;
	});
}

const std::vector<uint64_t>& SrcModel::bodyAddresses(const Ref& forwarder) const
{
	static const std::vector<uint64_t> none;
	const auto& map = forwarder.function.empty() ? m_bodyAddressesByName : m_bodyAddresses;
	auto found = map.find(forwarder.function.empty() ? forwarder.target : forwarder.function);
	return found == map.end() ? none : found->second;
}

const std::vector<uint64_t>& SrcModel::bodyAddresses(const std::string& mangled) const
{
	static const std::vector<uint64_t> none;
	auto found = m_bodyAddresses.find(mangled);
	return found == m_bodyAddresses.end() ? none : found->second;
}

const std::vector<const SrcModel::Function*>& SrcModel::structorsOf(const std::string& cls) const
{
	static const std::vector<const Function*> none;
	auto found = m_structors.find(cls);
	return found == m_structors.end() ? none : found->second;
}

const std::vector<const SrcModel::Function*>& SrcModel::conversionsOf(const std::string& cls) const
{
	static const std::vector<const Function*> none;
	auto found = m_conversions.find(cls);
	return found == m_conversions.end() ? none : found->second;
}

const std::vector<SrcModel::Accessor>& SrcModel::accessorsOf(const std::string& cls) const
{
	static const std::vector<Accessor> none;
	auto found = m_accessors.find(cls);
	return found == m_accessors.end() ? none : found->second;
}

const std::vector<const SrcModel::Function*>& SrcModel::functionsNamed(const std::string& name) const
{
	static const std::vector<const Function*> none;
	auto found = m_functionsByName.find(name);
	return found == m_functionsByName.end() ? none : found->second;
}

bool SrcModel::explicitSpecialization(const Record& record)
{
	if (record.pattern.empty() || record.line <= 0)
		return false;
	std::ifstream in(record.file);
	std::string line, previous;
	for (int64_t k = 1; k <= record.line && std::getline(in, line); ++k)
		if (k < record.line)
			previous = line;
	static const std::regex specialization(R"(template\s*<\s*>)");
	return std::regex_search(line, specialization) || std::regex_search(previous, specialization);
}

int SrcModel::templateHeads(const std::string& mangled, const std::string& cls) const
{
	int heads = mangled.rfind("??$", 0) == 0 ? 1 : 0;
	int depth = 0;
	for (size_t k = 0; k <= cls.size(); ++k)
	{
		if (k == cls.size() || (depth == 0 && cls.compare(k, 2, "::") == 0))
			if (const Record* r = record(cls.substr(0, k)); r && !r->pattern.empty() && !explicitSpecialization(*r))
				++heads;
		if (k < cls.size())
			depth += cls[k] == '<' ? 1 : cls[k] == '>' ? -1 : 0;
	}
	return heads;
}

std::string SrcModel::classSource(const std::string& cls) const
{
	if (auto found = m_classSources.find(cls); found != m_classSources.end())
		return found->second;
	const Record* r = record(cls);
	if (!r)
		return "";
	std::string sibling = r->file.substr(0, r->file.rfind('.')) + ".cpp";
	return m_tus.count(sibling) ? sibling : "";
}

std::optional<int64_t> SrcModel::sizeOf(const std::string& key) const
{
	const Type* t = type(key);
	if (!t)
		return std::nullopt;
	if (t->kind == "builtin")
		return t->size;
	if (t->kind == "ptr" || t->kind == "lref" || t->kind == "rref" || t->kind == "memptr")
		return 4;
	if (t->kind == "const" || t->kind == "volatile")
		return sizeOf(t->of);
	if (t->kind == "array" || t->kind == "vector")
	{
		std::optional<int64_t> element = sizeOf(t->of);
		if (!element || t->count < 0)
			return std::nullopt;
		return *element * t->count;
	}
	if (t->kind == "record")
	{
		const Record* r = record(t->name);
		return r ? std::optional<int64_t>(r->size) : std::nullopt;
	}
	if (t->kind == "enum")
	{
		const Enum* e = enumeration(t->name);
		return e ? sizeOf(e->underlying) : std::nullopt;
	}
	if (t->kind == "typedef")
	{
		const Typedef* alias = typedefNamed(t->name);
		return alias ? sizeOf(alias->target) : std::nullopt;
	}
	return std::nullopt;
}

std::optional<int64_t> SrcModel::alignOf(const std::string& key) const
{
	const Type* t = type(key);
	if (!t)
		return std::nullopt;
	if (t->kind == "builtin" || t->kind == "ptr" || t->kind == "lref" || t->kind == "rref" || t->kind == "memptr" ||
		t->kind == "enum")
		return sizeOf(key);
	// a compiler vector aligns to its size (`__m128`: 16)
	if (t->kind == "vector")
		return sizeOf(key);
	if (t->kind == "const" || t->kind == "volatile" || t->kind == "array")
		return alignOf(t->of);
	if (t->kind == "record")
	{
		const Record* r = record(t->name);
		return r && r->align ? std::optional<int64_t>(r->align) : std::nullopt;
	}
	if (t->kind == "typedef")
	{
		const Typedef* alias = typedefNamed(t->name);
		return alias ? alignOf(alias->target) : std::nullopt;
	}
	return std::nullopt;
}

std::string SrcModel::storage(const std::string& key) const
{
	const Type* t = type(key);
	if (!t)
		return key;
	if (t->kind == "const" || t->kind == "volatile" || t->kind == "array")
		return storage(t->of);
	if (t->kind == "typedef")
	{
		const Typedef* alias = typedefNamed(t->name);
		return alias ? storage(alias->target) : key;
	}
	return key;
}

bool SrcModel::isEmpty(const Record& record) const
{
	if (record.vfptr || !record.vtables.empty())
		return false;
	for (const Field& f : record.fields)
		if (!f.bits || *f.bits != 0)
			return false;
	for (const Base& b : record.bases)
	{
		const Record* base = this->record(b.record);
		if (b.isVirtual || !base || !isEmpty(*base))
			return false;
	}
	return true;
}

bool SrcModel::byteCopied(const Record& record) const
{
	if (!record.trivialCopy)
		return false;
	// a user-declared destructor, or a constructor taking one reference (a copy constructor, or one C++ may take for
	// it), runs code where the exe copies the bytes
	for (const Function* structor : structorsOf(record.name))
	{
		if (structor->structor == "dtor")
			return false;
		const Type* t = type(structor->type);
		if (t && t->params.size() == 1)
			if (const Type* param = type(t->params[0]); param && (param->kind == "lref" || param->kind == "rref"))
				return false;
	}
	for (const Base& b : record.bases)
	{
		const Record* base = this->record(b.record);
		if (b.isVirtual || !base || !byteCopied(*base))
			return false;
	}
	for (const Field& f : record.fields)
	{
		const Type* t = type(storage(f.type));
		if (t && t->kind == "record")
			if (const Record* member = this->record(t->name); !member || !byteCopied(*member))
				return false;
	}
	return true;
}

}  // namespace idakit
