// The types phases: the hand-made IDA types ida_type_aliases.tsv names made typedefs of src's, then src's enums,
// records and vtables written over IDA's. A record's IDA struct is exactly src's layout: what src does not describe
// is left undefined bytes.

#include "idakit/Sync.hpp"

#include "IdaSupport.hpp"

#include <algorithm>
#include <cstdio>
#include <set>

namespace idakit
{

namespace
{

// srcdb's name of an enum declared in a .cpp (`Name @src/x.cpp`, visible in that TU only): no IDA name spells it (a
// record's is IdaStructNames::unnamedLocal's)
bool tuLocal(const std::string& name, const std::string& file)
{
	return name.ends_with(" @" + file);
}

// IDA's name of a src vtable: `X_vtbl` for the primary one, `X_%04X_vtbl` for the one at offset N
std::string vtableName(const std::string& ida, int64_t offset)
{
	if (offset == 0)
		return ida + "_vtbl";
	char suffix[32];
	std::snprintf(suffix, sizeof suffix, "_%04llX_vtbl", static_cast<unsigned long long>(offset));
	return ida + suffix;
}

funcarg_t argument(const char* name, const tinfo_t& type)
{
	funcarg_t a;
	a.name = name;
	a.type = type;
	return a;
}

}  // namespace

// A rename does not carry IDA's references: globals and prototypes typed with the old name keep printing it after
// the rename and a reopen, so every alias takes the typedef form, written in place (the same name and ordinal, so
// what names the type either way resolves to src's).
//
// IDA's enum is first written over as an enum without enumerators, then the typedef over that: IDA's in-place replace
// of an enum by a non-enum leaves the enum's enumerator names in the type library's name index, and a later in-place
// replace of any enum used by a type named like one of them fails with internal error 3053 (IDA's FormTypeID, whose
// enumerator ActorValueInfo names the struct holding ActorValue::Section, and MediaSet the one holding
// MediaSet::MEDIASETTYPE). An enum replaced by an enum drops the old enumerator names cleanly.
void Sync::aliases()
{
	til_t* til = get_idati();
	for (const auto& [ida, src] : m_types.aliases())
	{
		tinfo_t current;
		if (!current.get_named_type(til, ida.c_str()))
		{
			log("alias-absent", ida, src);
			count("aliases: absent");
			continue;
		}
		const SrcModel::Record* record = m_types.recordSpelled(src);
		const SrcModel::Enum* e = record ? nullptr : m_model.enumeration(src);
		if (!record && !e)
		{
			log("alias-not-src", ida, src);
			count("aliases: not src");
			continue;
		}
		std::string spelling = record ? m_structNames.pushName(record->name) : e->name;
		if (spelling == ida)
			continue;
		// src's type exists under its spelling before the typedef names it
		if (!(record ? recordType(record->name) : enumType(e->name)))
		{
			log("alias-failed", ida, "no IDA type " + spelling);
			continue;
		}
		// IDA's own enum, not the typedef an earlier run wrote (is_enum resolves it; a named type's decltype is always
		// the reference to it, so is_decl_enum never holds)
		if (enum_type_data_t members;
			!current.is_typedef() && current.get_enum_details(&members) && !members.empty())
		{
			members.clear();
			members.group_sizes.clear();
			tinfo_t cleared;
			cleared.create_enum(members);
			if (tinfo_code_t code = cleared.set_named_type(til, ida.c_str(), NTF_REPLACE); code != TERR_OK)
			{
				log("alias-failed", ida, "clearing its enumerators: " + std::string(tinfo_errstr(code)));
				continue;
			}
		}
		tinfo_t alias;
		alias.create_typedef(til, spelling.c_str());
		if (tinfo_code_t code = alias.set_named_type(til, ida.c_str(), NTF_REPLACE); code != TERR_OK)
		{
			log("alias-failed", ida, spelling + ": " + tinfo_errstr(code));
			continue;
		}
		log("alias-typedef", ida, spelling);
		count("aliases: typedef");
	}
}

void Sync::enums()
{
	std::map<std::string, const SrcModel::Enum*> sorted;
	for (const auto& [name, e] : m_model.enums())
		sorted.emplace(name, &e);
	for (const auto& [name, e] : sorted)
	{
		if (tuLocal(name, e->file) || unnamedRecord(name))
		{
			log(tuLocal(name, e->file) ? "enum-local" : "enum-unnamed", name);
			continue;
		}
		if (tinfo_t current; current.get_named_type(get_idati(), name.c_str()) && !current.is_enum())
		{
			log("enum-name-taken", name, text(current));
			continue;
		}
		if (writeEnum(*e))
			count("enums written");
	}
}

bool Sync::idaNamed(const SrcModel::Record& record)
{
	return !m_structNames.unnamedLocal(record) && !unnamedRecord(m_structNames.pushName(record.name));
}

void Sync::records()
{
	// pass 1: a struct for every record (a shell of src's size where IDA has none), so that every layout's types
	// convert whatever the order; IDA's size of each struct it had, for the resize log
	std::map<std::string, int64_t> before;
	for (const auto& [name, record] : m_model.records())
	{
		if (m_structNames.unnamedLocal(record))
		{
			log("record-local", name);
			continue;
		}
		if (!idaNamed(record))
			continue;
		const std::string& ida = m_structNames.pushName(name);
		if (auto [owner, added] = m_pushed.emplace(ida, name); !added)
		{
			log("push-name-shared", name, ida + " is " + owner->second + "'s");
			continue;
		}
		if (tinfo_t t; t.get_named_type(get_idati(), ida.c_str()) && t.is_udt())
			before.emplace(ida, static_cast<int64_t>(t.get_size()));
		recordType(name);
	}
	// pass 2: the layouts, each record's bases and by-value member records first
	std::set<std::string> done;
	for (const auto& [name, record] : m_model.records())
		writeAfterParts(record, done, before);
}

void Sync::writeAfterParts(const SrcModel::Record& record, std::set<std::string>& done,
						   const std::map<std::string, int64_t>& before)
{
	if (!done.insert(record.name).second)
		return;
	for (const SrcModel::Base& b : record.bases)
		if (const SrcModel::Record* r = m_model.record(b.record))
			writeAfterParts(*r, done, before);
	for (const SrcModel::Field& f : record.fields)
		if (const SrcModel::Type* t = m_model.type(m_model.storage(f.type)); t && t->kind == "record")
			if (const SrcModel::Record* r = m_model.record(t->name))
				writeAfterParts(*r, done, before);
	// a record without an IDA name (an anonymous member's) is written inline in its holder
	const std::string& ida = m_structNames.pushName(record.name);
	auto owner = m_pushed.find(ida);
	if (!idaNamed(record) || owner == m_pushed.end() || owner->second != record.name)
		return;
	std::optional<tinfo_t> t = layout(record);
	if (!t)
		return;
	if (tinfo_code_t code = t->set_named_type(get_idati(), ida.c_str(), NTF_REPLACE); code != TERR_OK)
	{
		log("struct-create-failed", record.name, ida + ": " + tinfo_errstr(code));
		return;
	}
	count("records written");
	if (auto old = before.find(ida); old != before.end() && old->second != record.size)
	{
		log("struct-resized", record.name, ida + ": IDA " + hex(old->second) + ", src " + hex(record.size));
		count("structs resized");
	}
}

std::optional<tinfo_t> Sync::layout(const SrcModel::Record& record)
{
	udt_type_data_t udt;
	bool cppObject = false;
	if (record.vfptr)
	{
		std::string table = vtableName(m_structNames.pushName(record.name), 0);
		tinfo_t vtbl;
		if (!vtbl.get_named_type(get_idati(), table.c_str()) &&
			vtbl.create_forward_decl(get_idati(), BTF_STRUCT, table.c_str(), NTF_REPLACE) != TERR_OK)
			log("member-unmapped", record.name + "+0", "__vftable: no type " + table);
		else
			udt.add_member("__vftable", pointerTo(vtbl), 0).set_vftable();
	}
	for (const SrcModel::Base& b : record.bases)
	{
		std::string where = record.name + "+" + hex(b.offset);
		const SrcModel::Record* base = m_model.record(b.record);
		// an empty base takes no bytes (MSVC places a member at its offset): IDA cannot hold it
		if (base && !b.isVirtual && m_model.isEmpty(*base))
		{
			count("empty bases left out");
			continue;
		}
		std::optional<tinfo_t> t = base && !b.isVirtual ? recordType(b.record) : std::nullopt;
		if (!t)
		{
			log("member-unmapped", where, (b.isVirtual ? "virtual base " : "base ") + b.record);
			count("members unmapped");
			continue;
		}
		udt.add_member("", *t, static_cast<uint64>(b.offset) * 8).set_baseclass();
		cppObject = true;
	}
	addFields(record, 0, record.name, udt);
	bool isUnion = record.tag == "union";
	udt.is_union = isUnion;
	// a struct's members in offset order at src's offsets; a union's are all at 0 (IDA rejects a fixed union)
	if (!isUnion)
	{
		std::stable_sort(udt.begin(), udt.end(), [](const udm_t& a, const udm_t& b) { return a.offset < b.offset; });
		udt.set_fixed(true);
	}
	udt.total_size = udt.unpadded_size = static_cast<size_t>(record.size);
	if (cppObject)
		udt.taudt_bits |= TAUDT_CPPOBJ;
	size_t members = udt.size();
	tinfo_t t;
	if (!t.create_udt(udt, isUnion ? BTF_UNION : BTF_STRUCT) || static_cast<int64_t>(t.get_size()) != record.size)
	{
		log("struct-create-failed", record.name,
			std::to_string(members) + " members, size " + std::to_string(record.size) +
				(t.empty() ? std::string() : ", IDA's " + std::to_string(t.get_size())));
		return std::nullopt;
	}
	return t;
}

void Sync::addFields(const SrcModel::Record& record, int64_t offsetBits, const std::string& holder,
					 udt_type_data_t& udt)
{
	for (const SrcModel::Field& f : record.fields)
	{
		int64_t at = offsetBits + f.offsetBits;
		std::string where = holder + "+" + hex(at / 8);
		auto unmapped = [&](const std::string& why) {
			log("member-unmapped", where, (f.name.empty() ? "anonymous member" : f.name) + ": " + why);
			count("members unmapped");
		};
		if (f.bits)
		{
			// an unnamed bit-field is padding: C++ has no member there
			if (f.name.empty() || *f.bits == 0)
				continue;
			std::optional<tinfo_t> t = typeOf(f.type);
			std::optional<int64_t> bytes = m_model.sizeOf(f.type);
			tinfo_t bits;
			if (!t || !bytes ||
				!bits.create_bitfield(static_cast<uchar>(*bytes), static_cast<uchar>(*f.bits), !t->is_signed()))
			{
				unmapped("bit-field of " + f.type);
				continue;
			}
			udm_t& m = udt.add_member(f.name.c_str(), bits, static_cast<uint64>(at));
			m.size = static_cast<uint64>(*f.bits);
			continue;
		}
		// a member whose record has no IDA name (an anonymous member's, or a named member's unnamed type inside one)
		// is that record's layout; C++ makes an anonymous struct's members the holder's, which a struct holds at
		// their offsets (a union holds every member at 0: it keeps the struct as one `___u<idx>` member)
		const SrcModel::Type* type = m_model.type(f.type);
		const SrcModel::Record* inner = type && type->kind == "record" ? m_model.record(type->name) : nullptr;
		if (inner && !unnamedRecord(m_structNames.pushName(inner->name)))
			inner = nullptr;
		if (inner && f.name.empty() && inner->tag != "union" && !udt.is_union)
		{
			addFields(*inner, at, holder, udt);
			continue;
		}
		std::optional<tinfo_t> t = inner ? layout(*inner) : typeOf(f.type);
		if (!t)
		{
			unmapped("type " + f.type);
			continue;
		}
		std::optional<int64_t> size = inner ? std::optional(inner->size) : m_model.sizeOf(f.type);
		if (!size || static_cast<int64_t>(t->get_size()) != *size)
		{
			unmapped("IDA's " + text(*t) + " has " + hex(t->get_size()) + " bytes, src " +
					 (size ? hex(*size) : std::string("none")));
			continue;
		}
		std::string name = f.name.empty() ? "___u" + std::to_string(udt.size()) : f.name;
		udt.add_member(name.c_str(), *t, static_cast<uint64>(at));
	}
}

void Sync::vtables()
{
	for (const auto& [name, record] : m_model.records())
	{
		const std::string& ida = m_structNames.pushName(name);
		auto owner = m_pushed.find(ida);
		if (record.vtables.empty() || owner == m_pushed.end() || owner->second != name)
			continue;
		for (const SrcModel::Vtable& v : record.vtables)
			writeVtable(record, v, vtableName(ida, v.offset));
	}
}

void Sync::writeVtable(const SrcModel::Record& record, const SrcModel::Vtable& table, const std::string& ida)
{
	std::string key = record.name + "+" + hex(table.offset);
	// the object a slot's this points to: the class, or for a secondary table its base subobject at that offset
	std::string self = table.offset == 0 ? record.name : baseAt(m_model, record, table.offset);
	std::optional<tinfo_t> selfType = self.empty() ? std::nullopt : recordType(self);
	std::optional<tinfo_t> flags = primitive("unsigned int");
	if (!selfType || !flags)
	{
		log("vtable-unmapped", key, "no class at the offset");
		return;
	}
	tinfo_t thisType = pointerTo(*selfType);
	udt_type_data_t udt;
	std::map<std::string, int> used;
	for (size_t k = 0; k < table.slots.size(); ++k)
	{
		const SrcModel::Slot& slot = table.slots[k];
		std::string name = slot.kind == "deleting" ? "__vecDelDtor" : lastComponent(slot.name);
		if (name.empty())
			name = "slot" + std::to_string(k);
		if (++used[name] > 1)
			name += "_" + std::to_string(k);
		std::optional<tinfo_t> type;
		if (slot.kind == "deleting")
		{
			// MSVC's deleting destructor: the delete flags on the stack, the object returned
			func_type_data_t fd;
			fd.set_cc(CM_CC_THISCALL);
			fd.rettype = pointerTo(voidType());
			fd.push_back(argument("this", thisType));
			fd.push_back(argument("auiFlags", *flags));
			if (tinfo_t f; f.create_func(fd))
				type = f;
		}
		else if (const SrcModel::Function* function = m_model.function(slot.function))
		{
			type = prototype(*function);
			func_type_data_t fd;
			if (type && table.offset != 0 && type->get_func_details(&fd) && !fd.empty())
			{
				fd[0].type = thisType;
				if (tinfo_t f; f.create_func(fd))
					type = f;
				else
					type.reset();
			}
		}
		if (!type)
		{
			log("vtable-slot-untyped", key + "[" + std::to_string(k) + "]", slot.name);
			type = voidType();
		}
		udt.add_member(name.c_str(), pointerTo(*type), 32 * static_cast<uint64>(k));
	}
	udt.set_vftable(true);
	udt.set_fixed(true);
	udt.total_size = udt.unpadded_size = 4 * table.slots.size();
	tinfo_t t;
	tinfo_code_t code = TERR_OK;
	if (!t.create_udt(udt, BTF_STRUCT) || (code = t.set_named_type(get_idati(), ida.c_str(), NTF_REPLACE)) != TERR_OK)
	{
		log("vtable-create-failed", key, ida + ": " + tinfo_errstr(code));
		return;
	}
	count("vtables written");
}

}  // namespace idakit
