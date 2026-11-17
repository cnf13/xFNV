// The checks a table of function slots passes before declare --retype writes it (DeclarePdb.hpp's SlotRowChecks):
// virtual families changed whole, no overload made another's, no width narrowed that the exe keeps.

#include "DeclarePdb.hpp"

#include "idakit/Address.hpp"

#include <Facts.hpp>

#include <functional>
#include <set>

namespace idakit
{

// the virtual families: a method slot of a record's vtable joined with the same slot of each base's vtable at the
// base's offset
SlotRowChecks::SlotRowChecks(const SrcModel& model, SrcTypes& types, exefacts::Facts& facts)
	: PdbTypes(model, types), m_facts(facts)
{
	std::function<std::string(const std::string&)> root = [&](const std::string& m) {
		auto found = m_family.find(m);
		if (found == m_family.end() || found->second == m)
			return m;
		std::string r = root(found->second);
		m_family[m] = r;
		return r;
	};
	auto join = [&](const std::string& a, const std::string& b) {
		m_family.emplace(a, a);
		m_family.emplace(b, b);
		std::string ra = root(a), rb = root(b);
		if (ra != rb)
			m_family[std::max(ra, rb)] = std::min(ra, rb);
	};
	for (const auto& [name, record] : m_model.records())
		for (const SrcModel::Base& base : record.bases)
		{
			const SrcModel::Record* b = m_model.record(base.record);
			if (!b || base.isVirtual)
				continue;
			for (const SrcModel::Vtable& own : record.vtables)
				for (const SrcModel::Vtable& theirs : b->vtables)
					if (theirs.offset + base.offset == own.offset)
						for (size_t k = 0; k < theirs.slots.size() && k < own.slots.size(); ++k)
							if (own.slots[k].kind == "method" && theirs.slots[k].kind == "method" &&
								!own.slots[k].function.empty() && !theirs.slots[k].function.empty())
								join(own.slots[k].function, theirs.slots[k].function);
		}
	std::vector<std::string> keys;
	for (const auto& [m, parent] : m_family)
		keys.push_back(m);
	for (const std::string& m : keys)
		m_members[root(m)].push_back(m);
	for (const std::string& m : keys)
		m_family[m] = root(m);
	// a usercall's locations: its own forwarder's or registration's (a ref at the function's own address)
	for (const auto& [ea, refs] : m_model.refs())
		for (const SrcModel::Ref& r : refs)
			if (auto own = m_address.find(r.function);
				!r.locations.empty() && own != m_address.end() && own->second == ea)
				m_locations[r.function] = r.locations;
}

void SlotRowChecks::judgeFamilies(std::vector<SlotRow>& rows, const Takes& takes) const
{
	std::map<std::pair<std::string, std::string>, std::vector<size_t>> bySlot;  // (family, slot) -> rows
	for (size_t i = 0; i < rows.size(); ++i)
		if (auto found = m_family.find(rows[i].mangled); found != m_family.end() && rows[i].slot != "this")
			bySlot[{found->second, rows[i].slot}].push_back(i);
	// a member's own type of the slot; nullopt where src's type is not known
	auto ownType = [&](const std::string& member, const std::string& slot) -> std::optional<std::string> {
		const SrcModel::Function* f = m_model.function(member);
		std::optional<SrcTypes::Function> type = f ? SrcTypes::function(f->type) : std::nullopt;
		if (!type || (slot != "ret" && std::stoul(slot) >= type->params.size()))
			return std::nullopt;
		return slot == "ret" ? type->ret : type->params[std::stoul(slot)];
	};
	std::vector<SlotRow> added;
	for (const auto& [family, indices] : bySlot)
	{
		std::string target, why;
		for (size_t i : indices)
			if (rows[i].verdict == "apply" && target.empty())
				target = rows[i].key;
		if (target.empty())
			continue;
		for (size_t i : indices)
		{
			const SlotRow& r = rows[i];
			if (r.verdict == "spelling")
				continue;
			if (r.verdict == "apply")
			{
				if (normal(target) != normal(r.key))
					why = why.empty() ? "the overrides disagree (" + spelled(target) + ", " + r.proposed + ")" : why;
				continue;
			}
			// a row that keeps its function's type agrees where that type is the family's
			std::optional<std::string> own = r.verdict == "keep" ? ownType(r.mangled, r.slot) : std::nullopt;
			if (!own || normal(*own) != normal(target))
				why = why.empty() ? r.function + "'s row is " + r.verdict : why;
		}
		std::set<std::string> covered;
		for (size_t i : indices)
			covered.insert(rows[i].mangled);
		std::vector<SlotRow> missing;
		for (const std::string& member : m_members.at(family.first))
		{
			if (covered.count(member))
				continue;
			const SrcModel::Function* f = m_model.function(member);
			std::optional<std::string> own = ownType(member, family.second);
			if (!f || !own)
			{
				why = why.empty() ? "a member's type is not known: " + member : why;
				continue;
			}
			if (normal(*own) == normal(target))
				continue;
			if (std::string refusal = takes(*f, family.second, *own); !refusal.empty())
			{
				why = why.empty() ? refusal : why;
				continue;
			}
			auto at = m_address.find(member);
			std::string unshown;
			if (narrows(*own, target))
				if (std::string fact =
						narrowing(*f, at == m_address.end() ? 0 : at->second, family.second, target, unshown);
					!fact.empty() || !unshown.empty())
				{
					why = why.empty() ? f->name + " keeps its " + spelled(*own) + ": " +
											(fact.empty() ? unshown : "the exe " + fact)
									  : why;
					continue;
				}
			SlotRow row;
			row.mangled = member;
			row.function = f->name;
			row.slot = family.second;
			row.src = spelled(*own);
			row.proposed = spelled(target);
			row.key = target;
			row.verdict = "apply";
			row.reason = "the family's type: " + rows[indices[0]].function + "'s override";
			if (at != m_address.end())
				row.address = hexText(at->second);
			missing.push_back(row);
		}
		if (!why.empty())
		{
			for (size_t i : indices)
				if (rows[i].verdict == "apply")
				{
					rows[i].verdict = "open";
					rows[i].reason = "virtual family: " + why;
				}
			continue;
		}
		// each row of the change carries the family and its row count: --retype writes all of them or none
		size_t count = missing.size();
		for (size_t i : indices)
			count += rows[i].verdict == "apply";
		std::string tag = family.first + "#" + family.second + "/" + std::to_string(count);
		for (size_t i : indices)
			if (rows[i].verdict == "apply")
				rows[i].family = tag;
		for (SlotRow& row : missing)
			row.family = tag;
		added.insert(added.end(), missing.begin(), missing.end());
	}
	rows.insert(rows.end(), added.begin(), added.end());
}

void SlotRowChecks::judgeOverloads(std::vector<SlotRow>& rows) const
{
	std::map<std::string, std::vector<size_t>> byFunction;
	for (size_t i = 0; i < rows.size(); ++i)
		if (rows[i].verdict == "apply" && rows[i].slot != "ret" && rows[i].slot != "this")
			byFunction[rows[i].mangled].push_back(i);
	for (const auto& [mangled, indices] : byFunction)
	{
		const SrcModel::Function* f = m_model.function(mangled);
		std::optional<SrcTypes::Function> type = f ? SrcTypes::function(f->type) : std::nullopt;
		if (!type)
			continue;
		for (size_t i : indices)
			type->params[std::stoul(rows[i].slot)] = rows[i].key;
		std::string mine;
		for (const std::string& p : type->params)
			mine += normal(resolved(p)) + ",";
		for (const SrcModel::Function* other : m_model.functionsNamed(f->name))
		{
			std::optional<SrcTypes::Function> o = other->mangled != mangled ? SrcTypes::function(other->type)
																			 : std::nullopt;
			if (!o || o->isConst != type->isConst)
				continue;
			std::string theirs;
			for (const std::string& p : o->params)
				theirs += normal(resolved(p)) + ",";
			if (theirs == mine)
				for (size_t i : indices)
				{
					rows[i].verdict = "open";
					rows[i].reason = "it would make the overload the same as " + other->mangled;
				}
		}
	}
}

bool SlotRowChecks::narrows(const std::string& srcKey, const std::string& key) const
{
	auto integer = [](const std::string& k) { return k == "int" || k == "bool" || k == "char" || k == "wchar_t"; };
	std::string sk = kindOf(srcKey), k = kindOf(key);
	std::optional<int64_t> ss = size(srcKey), ks = size(key);
	return integer(sk) && integer(k) && ss && ks && (*ks < *ss || (*ks == *ss && k == "bool" && sk != "bool"));
}

std::optional<int64_t> SlotRowChecks::stackOffset(const SrcModel::Function& function, size_t k) const
{
	std::optional<SrcTypes::Function> type = SrcTypes::function(function.type);
	const std::map<int64_t, std::string>* user = userLocations(function.mangled);
	// a usercall's parameter in a register, and its stack ones (no location: on the stack) after a `this` there
	auto stacked = [&](int64_t i) {
		auto at = user->find(i);
		return at == user->end() || at->second == "stack";
	};
	if (!type || k >= type->params.size() ||
		(!user && type->cc != "cdecl" && type->cc != "stdcall" && type->cc != "thiscall") || (user && !stacked(k)))
		return std::nullopt;
	int64_t offset = 0;
	if (const SrcModel::Abi* abi = m_model.abi(function.mangled); abi && abi->ret == "indirect")
		offset = 4;
	if (user && !function.cls.empty() && !function.isStatic && stacked(-1))
		offset += 4;
	for (size_t i = 0; i < k; ++i)
	{
		if (user && !stacked(int64_t(i)))
			continue;
		std::optional<int64_t> s = size(type->params[i]);
		if (!s)
			return std::nullopt;
		offset += (*s + 3) / 4 * 4;
	}
	return offset;
}

const std::map<int64_t, std::string>* SlotRowChecks::userLocations(const std::string& mangled) const
{
	auto found = m_locations.find(mangled);
	return found == m_locations.end() ? nullptr : &found->second;
}

std::string SlotRowChecks::narrowing(const SrcModel::Function& function, uint64_t ea, const std::string& slot,
									 const std::string& key, std::string& unshown) const
{
	std::optional<int64_t> ks = size(key);
	if (!ea || !ks)
	{
		unshown = !ea ? "no exe address" : "a size src does not know";
		return "";
	}
	try
	{
		if (slot == "ret")
		{
			std::optional<exefacts::ReturnWidth> width = m_facts.returnWidth(uint32_t(ea));
			if (!width || width->width == 0)
			{
				unshown = "the exe's return value is not traced";
				return "";
			}
			if (width->width > *ks)
				return "writes all " + std::to_string(width->width) + " bytes of eax on every return path";
			// a bool getter that returns a raw byte is the byte (declarations.md "Return widths")
			if (kindOf(key) == "bool" && width->raw)
				return "returns al as set at " + hexText(width->raw) + ", which may be other than 0 or 1";
			return "";
		}
		std::optional<int64_t> offset = stackOffset(function, std::stoul(slot));
		if (!offset)
		{
			std::optional<SrcTypes::Function> type = SrcTypes::function(function.type);
			const std::map<int64_t, std::string>* user = userLocations(function.mangled);
			auto at = user ? user->find(int64_t(std::stoul(slot))) : std::map<int64_t, std::string>::const_iterator();
			if (user && at != user->end())
				unshown = "a usercall's parameter in " + at->second;
			else
				unshown = "a register convention (" + (type ? type->cc : std::string("unknown")) + ")";
			return "";
		}
		// the widest read of the parameter's bytes
		std::pair<int64_t, uint32_t> widest{0, 0};
		for (const exefacts::ArgumentAccess& a : m_facts.argumentAccesses(uint32_t(ea)))
			if (a.offset < *offset + 4 && a.offset + a.size > *offset)
			{
				if (a.kind == exefacts::ArgumentAccess::Address)
				{
					unshown = "its address is taken at " + hexText(a.pc);
					return "";
				}
				int64_t bytes = std::max<int64_t>(a.size, a.offset + a.size - *offset);
				if (a.kind == exefacts::ArgumentAccess::Read && bytes > widest.first)
					widest = {bytes, a.pc};
			}
		if (widest.first > *ks)
			return "reads it as " + std::string(widest.first == 2 ? "a word" : "a dword") + " at " +
				   hexText(widest.second);
		// the dword passed on whole to a callee, or a caller's own argument passed in whole (declarations.md
		// "Parameter widths"): src's narrower type would convert it on the way
		for (const exefacts::PassedArgument& p : m_facts.passedArguments(uint32_t(ea)))
			if (p.offset == *offset)
				return "passes it on whole to " + (p.target ? hexText(p.target) : std::string("an indirect call")) +
					   " at " + hexText(p.pc);
		for (uint32_t caller : m_facts.program().callers(uint32_t(ea)))
		{
			std::vector<exefacts::PassedArgument> passed;
			try
			{
				passed = m_facts.passedArguments(caller);
			}
			catch (const std::exception&)
			{
				continue;
			}
			for (const exefacts::PassedArgument& p : passed)
				if (p.target == ea && int64_t(p.index) * 4 == *offset)
					return "takes a caller's own argument whole (" + hexText(caller) + " passes it on at " +
						   hexText(p.pc) + ")";
		}
	}
	catch (const std::exception&)
	{
		unshown = "the exe's code does not decode";
	}
	return "";
}

void SlotRowChecks::judgeWidths(std::vector<SlotRow>& rows) const
{
	for (SlotRow& row : rows)
	{
		const SrcModel::Function* f = row.verdict == "apply" ? m_model.function(row.mangled) : nullptr;
		std::optional<SrcTypes::Function> type = f ? SrcTypes::function(f->type) : std::nullopt;
		if (!type || row.slot == "this" || (row.slot != "ret" && std::stoul(row.slot) >= type->params.size()))
			continue;
		const std::string& own = row.slot == "ret" ? type->ret : type->params[std::stoul(row.slot)];
		if (!narrows(own, row.key))
			continue;
		auto at = m_address.find(row.mangled);
		std::string unshown;
		std::string fact = narrowing(*f, at == m_address.end() ? 0 : at->second, row.slot, row.key, unshown);
		if (fact.empty() && unshown.empty())
			continue;
		row.verdict = "open";
		row.reason = "it narrows src's " + row.src + ": " + (fact.empty() ? unshown : "the exe " + fact);
	}
}

}  // namespace idakit
