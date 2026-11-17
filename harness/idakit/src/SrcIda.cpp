#include "idakit/SrcIda.hpp"

#include "IdaSupport.hpp"

#include <algorithm>
#include <cctype>

namespace idakit
{

namespace
{

bool identifierChar(char c)
{
	return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

}  // namespace

std::vector<std::string> idaSpellings(const std::string& name)
{
	std::string t;
	// class-keys dropped
	for (size_t k = 0; k < name.size();)
	{
		bool word = k == 0 || !identifierChar(name[k - 1]);
		bool dropped = false;
		for (const char* keyword : {"class ", "struct ", "union "})
		{
			size_t length = std::char_traits<char>::length(keyword);
			if (word && name.compare(k, length, keyword) == 0)
			{
				k += length;
				dropped = true;
				break;
			}
		}
		if (!dropped)
			t += name[k++];
	}
	// one blank before each run of * and &, none inside it; none around commas or before '>'
	std::string spaced;
	for (size_t k = 0; k < t.size(); ++k)
	{
		char c = t[k];
		if (c == '*' || c == '&')
		{
			while (!spaced.empty() && spaced.back() == ' ')
				spaced.pop_back();
			if (spaced.empty() || (spaced.back() != '*' && spaced.back() != '&'))
				spaced += ' ';
			spaced += c;
		}
		else if (c == ',')
		{
			while (!spaced.empty() && spaced.back() == ' ')
				spaced.pop_back();
			spaced += ',';
			while (k + 1 < t.size() && t[k + 1] == ' ')
				++k;
		}
		else if (c == '>')
		{
			while (!spaced.empty() && spaced.back() == ' ')
				spaced.pop_back();
			if (!spaced.empty() && spaced.back() == '>')
				spaced += ' ';
			spaced += '>';
		}
		else if (c == ' ' && !spaced.empty() && spaced.back() == ' ')
			continue;
		else
			spaced += c;
	}
	while (!spaced.empty() && spaced.back() == ' ')
		spaced.pop_back();
	while (!spaced.empty() && spaced.front() == ' ')
		spaced.erase(0, 1);
	std::vector<std::string> out{spaced};
	auto add = [&](const std::string& s) {
		if (std::find(out.begin(), out.end(), s) == out.end())
			out.push_back(s);
	};
	if (spaced.find('<') != std::string::npos)
	{
		std::string noEnum;
		for (size_t k = 0; k < spaced.size();)
			if ((k == 0 || !identifierChar(spaced[k - 1])) && spaced.compare(k, 5, "enum ") == 0)
				k += 5;
			else
				noEnum += spaced[k++];
		add(noEnum);
		std::string longs = spaced;
		for (size_t at = longs.find("unsigned int"); at != std::string::npos;
			 at = longs.find("unsigned int", at + 13))
			longs.replace(at, 12, "unsigned long");
		add(longs);
		// `enum ` before each capitalised identifier followed by ' ', '>' or ','
		std::string enums;
		for (size_t k = 0; k < spaced.size();)
		{
			bool start = (k == 0 || !identifierChar(spaced[k - 1])) && std::isupper(static_cast<unsigned char>(spaced[k]));
			if (!start)
			{
				enums += spaced[k++];
				continue;
			}
			size_t end = k;
			while (end < spaced.size() && identifierChar(spaced[end]))
				++end;
			bool before = end < spaced.size() && (spaced[end] == ' ' || spaced[end] == '>' || spaced[end] == ',');
			bool tagged = enums.size() >= 5 && enums.compare(enums.size() - 5, 5, "enum ") == 0;
			if (before && !tagged)
				enums += "enum ";
			enums.append(spaced, k, end - k);
			k = end;
		}
		add(enums);
	}
	return out;
}

std::vector<std::string> msvc9Variants(const std::string& spelling)
{
	if (spelling.find("msvc9::") == std::string::npos)
		return {};
	std::string standard = spelling, bare = spelling;
	for (size_t at = standard.find("msvc9::"); at != std::string::npos; at = standard.find("msvc9::", at))
		standard.replace(at, 7, "std::");
	for (size_t at = bare.find("msvc9::"); at != std::string::npos; at = bare.find("msvc9::", at))
		bare.erase(at, 7);
	return {standard, bare};
}

std::string msvc9Names(std::string name)
{
	for (size_t at = name.find("std::"); at != std::string::npos; at = name.find("std::", at + 1))
		if (at == 0 || !(std::isalnum(static_cast<unsigned char>(name[at - 1])) || name[at - 1] == '_'))
			name.replace(at, 5, "msvc9::");
	return name;
}

bool unnamedRecord(const std::string& name)
{
	// clang's `(anonymous struct at ...)`, `(anonymous union at ...)`; `(anonymous namespace)::X` is X's scope
	for (size_t at = name.find("(anonymous "); at != std::string::npos; at = name.find("(anonymous ", at + 1))
		if (name.compare(at, 21, "(anonymous namespace)") != 0)
			return true;
	return name.find("(unnamed ") != std::string::npos;
}

bool placeholder(const SrcModel::Ref& ref)
{
	return ref.file == "gen/Prototypes.hpp";
}

RefChoice chooseRef(const SrcModel& model, uint64_t address)
{
	RefChoice choice;
	// an FNVD_ORIGINAL_FUNCTION constant is the exe function's address as a value (a callback src stores or passes),
	// no declaration of the function: an address src names only so reads as undeclared
	std::vector<const SrcModel::Ref*> bodies, forwarders;
	// the copies src marks as folds of the address (porting rules "Folds"): their forwarders call the owner's code
	std::set<std::string> copies;
	for (const SrcModel::Ref& r : model.refsAt(address))
		if (r.kind == "fold" && !r.function.empty())
			copies.insert(r.function);
	for (const SrcModel::Ref& r : model.refsAt(address))
	{
		if (r.kind == "constructor" || r.kind == "destructor")
		{
			if (!choice.structor)
				choice.structor = &r;
			continue;
		}
		if (placeholder(r))
			continue;
		if (r.kind == "body")
			bodies.push_back(&r);
		else if (r.kind == "forwarder" && !r.target.empty() && !(!r.function.empty() && copies.count(r.function)))
		{
			// a forwarder names the address only when the src function around it is not its own body elsewhere
			// (registered or FNVD_ADDRESS'd there: a call from that body)
			const std::vector<uint64_t>& registered = model.bodyAddresses(r);
			if (std::all_of(registered.begin(), registered.end(), [&](uint64_t a) { return a == address; }))
				forwarders.push_back(&r);
		}
	}
	// a class template member's own forwarder names the address for the instances src completes from it only where
	// no body and no explicit specialization names it: the exe's instance there is that specialization, the
	// template's body the code of the instances src has not specialized
	if (!bodies.empty() || std::any_of(forwarders.begin(), forwarders.end(), [&](const SrcModel::Ref* r) {
			return !r->generic && model.specializations().count(r->function);
		}))
		std::erase_if(forwarders, [](const SrcModel::Ref* r) { return r->generic; });
	// at a C++ constructor's or destructor's address, the forwarders naming it are its class's Construct / Destruct
	// (porting rules); any other function's raw call of the structor (a port's `fnvd::original` call) names nothing
	if (choice.structor)
	{
		auto classOf = [](const std::string& qualified) {
			size_t scope = qualified.rfind("::");
			std::string cls = scope == std::string::npos ? "" : qualified.substr(0, scope);
			return cls.substr(0, cls.find('<'));
		};
		std::string cls = classOf(choice.structor->target);
		std::erase_if(forwarders, [&](const SrcModel::Ref* r) {
			size_t scope = r->target.rfind("::");
			std::string member = scope == std::string::npos ? r->target : r->target.substr(scope + 2);
			return cls.empty() || classOf(r->target) != cls ||
				   (!member.starts_with("Construct") && !member.starts_with("Destruct"));
		});
	}
	for (const auto* list : {&bodies, &forwarders})
		for (const SrcModel::Ref* r : *list)
		{
			choice.names.insert(r->target);
			choice.named.push_back(r);
		}
	if (!bodies.empty())
		choice.ref = bodies[0];
	else if (choice.names.size() > 1)
		choice.ref = nullptr;
	else if (!forwarders.empty())
		choice.ref = forwarders[0];
	return choice;
}

std::optional<int64_t> secondaryVtable(const SrcModel::Record& record, const std::string& mangled)
{
	std::optional<int64_t> secondary;
	for (const SrcModel::Vtable& vtable : record.vtables)
		for (const SrcModel::Slot& slot : vtable.slots)
			if (slot.function == mangled)
			{
				if (vtable.offset == 0)
					return std::nullopt;
				secondary = vtable.offset;
			}
	return secondary;
}

std::string baseAt(const SrcModel& model, const SrcModel::Record& record, int64_t offset)
{
	for (const SrcModel::Base& base : record.bases)
		if (!base.isVirtual && base.offset == offset)
			return base.record;
	for (const SrcModel::Base& base : record.bases)
		if (const SrcModel::Record* inner = model.record(base.record);
			inner && !base.isVirtual && base.offset < offset && offset < base.offset + inner->size)
			if (std::string found = baseAt(model, *inner, offset - base.offset); !found.empty())
				return found;
	return "";
}

std::optional<int64_t> entryOffset(const SrcModel& model, const SrcModel::Function& member)
{
	const SrcModel::Record* owner = model.record(member.cls);
	if (!owner)
		return std::nullopt;
	return member.isVirtual ? secondaryVtable(*owner, member.mangled).value_or(0) : 0;
}

std::string entryClass(const SrcModel& model, const SrcModel::Function& member)
{
	const SrcModel::Record* owner = model.record(member.cls);
	if (std::optional<int64_t> offset = entryOffset(model, member); offset && *offset != 0)
		return baseAt(model, *owner, *offset);
	return member.cls;
}

std::string structorName(const std::string& name)
{
	std::string last = lastComponent(name);
	for (const char* structor : {"DeletingDestruct", "Destruct", "Construct"})
		if (last == structor || last.starts_with(std::string(structor) + "_"))
			return structor;
	return "";
}

std::string idaFunctionName(const std::string& qualified)
{
	std::string name = qualified;
	auto replaceAll = [&](const std::string& from, const std::string& to) {
		for (size_t at = name.find(from); at != std::string::npos; at = name.find(from, at + to.size()))
			name.replace(at, from.size(), to);
	};
	replaceAll("msvc9::", "std::");
	replaceAll("(anonymous namespace)::", "");
	std::string structor = structorName(name), scope = scopeOf(name);
	if (structor == "DeletingDestruct")
		return "";
	if (!scope.empty() && structor == "Construct")
		return scope + "::" + lastComponent(scope);
	if (!scope.empty() && structor == "Destruct")
		return scope + "::~" + lastComponent(scope);
	return name;
}

std::string tuSpelling(std::string name)
{
	static const std::string anonymous = "(anonymous namespace)::";
	for (size_t at = name.find(anonymous); at != std::string::npos; at = name.find(anonymous, at))
		name.erase(at, anonymous.size());
	for (size_t at = name.find(" @src/"); at != std::string::npos; at = name.find(" @src/", at))
	{
		size_t end = name.find(".cpp", at);
		name.erase(at, end == std::string::npos ? std::string::npos : end + 4 - at);
	}
	return name;
}

IdaStructNames::IdaStructNames(const SrcModel& model) : m_model(model)
{
	// a named member's unnamed type takes MSVC's name for it, `Parent::<unnamed_type_member>` (IDA's)
	for (const auto& [name, record] : model.records())
		for (const SrcModel::Field& f : record.fields)
		{
			const SrcModel::Type* type = f.name.empty() ? nullptr : model.type(model.storage(f.type));
			if (type && type->kind == "record" && unnamedRecord(type->name))
				m_unnamedNames.emplace(type->name, (record.spelled.empty() ? name : record.spelled) +
														"::<unnamed_type_" + f.name + ">");
		}
	// a TU-local record's name without the file and the anonymous namespace, where no other record has it
	std::unordered_map<std::string, int> uses;
	std::vector<std::pair<std::string, std::string>> locals;
	for (const auto& [name, record] : model.records())
	{
		std::string suffix = " @" + record.file;
		if (!name.ends_with(suffix))
		{
			++uses[name];
			continue;
		}
		std::string plain = name.substr(0, name.size() - suffix.size());
		static const std::string anonymous = "(anonymous namespace)::";
		for (size_t at = plain.find(anonymous); at != std::string::npos; at = plain.find(anonymous, at))
			plain.erase(at, anonymous.size());
		++uses[plain];
		locals.emplace_back(name, plain);
	}
	for (const auto& [name, plain] : locals)
		if (uses[plain] == 1)
			m_localNames.emplace(name, plain);
}

bool IdaStructNames::unnamedLocal(const SrcModel::Record& record) const
{
	return record.name.ends_with(" @" + record.file) && !m_localNames.contains(record.name);
}

std::vector<IdaStructNames::Candidate> IdaStructNames::candidates(const std::string& record) const
{
	const SrcModel::Record* r = m_model.record(record);
	std::vector<Candidate> out;
	auto add = [&](const std::string& s, bool own) {
		for (const Candidate& c : out)
			if (c.spelling == s)
				return;
		out.push_back({s, own});
	};
	auto unnamed = m_unnamedNames.find(record);
	if (unnamed != m_unnamedNames.end())
		for (const std::string& s : idaSpellings(unnamed->second))
			add(s, false);
	if (r && !r->spelled.empty())
		for (const std::string& s : idaSpellings(r->spelled))
			add(s, false);
	auto local = m_localNames.find(record);
	for (const std::string& s : idaSpellings(local == m_localNames.end() ? record : local->second))
		add(s, true);
	for (size_t k = 0, n = out.size(); k < n; ++k)
		for (const std::string& s : msvc9Variants(out[k].spelling))
			add(s, false);
	return out;
}

const std::string& IdaStructNames::of(const std::string& record, std::vector<std::string>* rejected)
{
	auto cached = m_names.find(record);
	if (cached != m_names.end())
		return cached->second;
	const SrcModel::Record* r = m_model.record(record);
	std::vector<Candidate> all = candidates(record);
	std::string chosen = all[0].spelling;
	for (const Candidate& c : all)
	{
		tinfo_t t;
		if (!t.get_named_type(get_idati(), c.spelling.c_str()) || !t.is_udt())
			continue;
		bool sameSize = !r || static_cast<int64_t>(t.get_size()) == r->size;
		if (sameSize || (c.own && c.spelling.find('<') == std::string::npos))
		{
			chosen = c.spelling;
			break;
		}
		if (!c.own && rejected)
			rejected->push_back(c.spelling + ": IDA " + hex(t.get_size()) + ", src " + hex(r->size) + ": not merged");
	}
	return m_names.emplace(record, chosen).first->second;
}

const std::string& IdaStructNames::pushName(const std::string& record)
{
	auto cached = m_pushNames.find(record);
	if (cached != m_pushNames.end())
		return cached->second;
	if (m_creationNames.empty())
		for (const auto& [name, r] : m_model.records())
			m_creationNames.emplace(candidates(name)[0].spelling, name);
	std::vector<Candidate> all = candidates(record);
	std::string chosen = all[0].spelling;
	// the exe's spellings first: a name with msvc9:: is none of the exe's (IDA's std:: classes, the CRT's), only a
	// struct created under src's spelling
	std::vector<Candidate> order = all;
	std::stable_partition(order.begin(), order.end(),
						  [](const Candidate& c) { return c.spelling.find("msvc9::") == std::string::npos; });
	for (const Candidate& c : order)
	{
		// another record's creation name is that record's struct, whether or not a sync created it yet
		if (auto other = m_creationNames.find(c.spelling); other != m_creationNames.end() && other->second != record)
			continue;
		if (tinfo_t t; t.get_named_type(get_idati(), c.spelling.c_str()) && t.is_udt())
		{
			chosen = c.spelling;
			break;
		}
	}
	return m_pushNames.emplace(record, chosen).first->second;
}

const SrcModel::Record* IdaStructNames::recordOf(const std::string& idaName)
{
	if (!m_reverseFilled)
	{
		m_reverseFilled = true;
		for (const auto& [name, record] : m_model.records())
		{
			const std::string& ida = of(name);
			tinfo_t t;
			// only a struct the database has: a record no IDA struct is taken for names nothing of IDA's
			if (t.get_named_type(get_idati(), ida.c_str()) && t.is_udt())
				m_records.emplace(ida, &record);
		}
	}
	auto found = m_records.find(idaName);
	return found == m_records.end() ? nullptr : found->second;
}

}  // namespace idakit
