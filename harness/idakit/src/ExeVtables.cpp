#include "idakit/ExeVtables.hpp"

#include "idakit/Asm.hpp"
#include "idakit/Database.hpp"

#include <algorithm>
#include <cctype>
#include <climits>
#include <regex>
#include <string_view>

namespace idakit
{

namespace
{

bool identifierChar(char c)
{
	return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

// text with every whole-word `from` (not preceded by an identifier character) replaced by `to`
std::string replaceWord(const std::string& text, const std::string& from, const std::string& to)
{
	std::string out;
	size_t at = 0;
	while (true)
	{
		size_t found = text.find(from, at);
		if (found == std::string::npos)
			break;
		bool starts = found == 0 || !identifierChar(text[found - 1]);
		bool ends = found + from.size() >= text.size() || !identifierChar(text[found + from.size()]) ||
					!identifierChar(from.back());
		out.append(text, at, found - at);
		out += starts && ends ? to : from;
		at = found + from.size();
	}
	out.append(text, at, std::string::npos);
	return out;
}

// the RTTI complete object locator's subobject offset, when the dword before the table is one
std::optional<int64_t> locatorOffset(ea_t table)
{
	ea_t locator = get_dword(table - 4);
	if (!is_mapped(locator) || !is_mapped(locator + 16) || get_dword(locator) != 0)
		return std::nullopt;
	if (!is_mapped(get_dword(locator + 12)))
		return std::nullopt;
	return static_cast<int64_t>(get_dword(locator + 4));
}

// a vtable name's demangling `X::`vftable'` (or `...{for `Y'}`) without `const `; nullopt for any other name
std::optional<std::string> demangleVftable(const std::string& mangled)
{
	qstring demangled;
	if (demangle_name(&demangled, mangled.c_str(), 0) <= 0)
		return std::nullopt;
	std::string spelled = demangled.c_str();
	if (spelled.rfind("const ", 0) == 0)
		spelled.erase(0, 6);
	if (spelled.rfind("::`vftable'") == std::string::npos)
		return std::nullopt;
	return spelled;
}

// the class an RTTI type descriptor names (`.?AVX@@` at +8), spelled as the demangler spells a vtable's class
std::optional<std::string> descriptorClass(ea_t descriptor)
{
	std::string name;
	for (ea_t at = descriptor + 8; name.size() < 4096; ++at)
	{
		uchar c = get_byte(at);
		if (c == 0)
			break;
		name += static_cast<char>(c);
	}
	if (name.size() < 5 || name.rfind(".?A", 0) != 0)
		return std::nullopt;
	std::optional<std::string> spelled = demangleVftable("??_7" + name.substr(4) + "6B@");
	if (!spelled)
		return std::nullopt;
	return spelled->substr(0, spelled->rfind("::`vftable'"));
}

// the primary base in the class hierarchy a complete object locator names (+16): the first base class descriptor
// after the class's own whose member displacement (+8) is 0 and that is no virtual base (+12, pdisp, is -1)
std::optional<std::string> rttiParent(ea_t locator)
{
	ea_t hierarchy = get_dword(locator + 16);
	if (!is_mapped(hierarchy) || !is_mapped(hierarchy + 12))
		return std::nullopt;
	uint32 count = get_dword(hierarchy + 8);
	ea_t array = get_dword(hierarchy + 12);
	for (uint32 k = 1; k < count && k < 1024; ++k)
	{
		ea_t base = get_dword(array + 4 * k);
		if (get_dword(base + 12) == 0xFFFFFFFF && get_dword(base + 8) == 0)
			return descriptorClass(get_dword(base));
	}
	return std::nullopt;
}

// every non-virtual base (pdisp -1) in the class hierarchy a complete object locator names, the class's own entry
// left out, with its member displacement in the class
std::vector<std::pair<std::string, int64_t>> rttiBases(ea_t locator)
{
	std::vector<std::pair<std::string, int64_t>> out;
	ea_t hierarchy = get_dword(locator + 16);
	if (!is_mapped(hierarchy) || !is_mapped(hierarchy + 12))
		return out;
	uint32 count = get_dword(hierarchy + 8);
	ea_t array = get_dword(hierarchy + 12);
	for (uint32 k = 1; k < count && k < 1024; ++k)
	{
		ea_t base = get_dword(array + 4 * k);
		if (get_dword(base + 12) != 0xFFFFFFFF)
			continue;
		if (std::optional<std::string> cls = descriptorClass(get_dword(base)))
			out.emplace_back(*cls, static_cast<int32_t>(get_dword(base + 8)));
	}
	return out;
}

// some instruction stores the address into memory as an immediate (a constructor setting a vfptr); a call through a
// slot (`call [X::vftable+N]`) does not count
bool storedAsVfptr(ea_t address)
{
	xrefblk_t xb;
	for (bool ok = xb.first_to(address, XREF_DATA); ok; ok = xb.next_to())
	{
		if (xb.type != dr_O || !is_code(get_flags(xb.from)))
			continue;
		insn_t insn;
		if (decode_insn(&insn, xb.from) <= 0 || insn.itype != NN_mov)
			continue;
		optype_t target = insn.ops[0].type;
		if ((target == o_mem || target == o_phrase || target == o_displ) && insn.ops[1].type == o_imm &&
			static_cast<ea_t>(insn.ops[1].value & 0xFFFFFFFFu) == address)
			return true;
	}
	return false;
}

}  // namespace

std::string ExeVtables::classKey(const std::string& name)
{
	std::string text = name;
	for (const char* anonymous : {"`anonymous namespace'", "(anonymous namespace)"})
		for (size_t at = text.find(anonymous); at != std::string::npos; at = text.find(anonymous))
			text.replace(at, std::string(anonymous).size(), "(anon)");
	for (const char* keyword : {"class ", "struct ", "union ", "enum "})
		text = replaceWord(text, keyword, "");
	text = replaceWord(text, "__int64", "long long");
	// the demangler's `char const *` is clang's `const char *`
	if (text.find(" const") != std::string::npos)
	{
		static const std::regex postfix(
			R"(\b((?:unsigned |signed )?(?:long long|char|short|int|long)|[A-Za-z_][A-Za-z_0-9:]*) const\b)");
		text = std::regex_replace(text, postfix, "const $1");
	}
	std::string out;
	for (char c : text)
		if (!std::isspace(static_cast<unsigned char>(c)))
			out += c;
	// a function type's: the demangler's default `__cdecl` and empty `(void)` are clang's `(*)()`
	out = replaceWord(out, "__cdecl", "");
	for (size_t at = out.find("(void)"); at != std::string::npos; at = out.find("(void)", at))
		out.replace(at, 6, "()");
	return replaceWord(out, "std::", "msvc9::");
}

ExeVtables::ExeVtables()
{
	segment_t* text = get_segm_by_name(".text");
	if (!text)
		throw Error("idakit: the database has no .text segment");
	for (size_t k = 0; k < get_nlist_size(); ++k)
	{
		const char* name = get_nlist_name(k);
		if (!name || std::string_view(name).substr(0, 4) != "??_7")
			continue;
		ea_t address = get_nlist_ea(k);
		// IDA names every table of a class `??_7X@@6B@` (the map's name) and the repeats `..._0`, `..._1`: the
		// suffix goes before demangling, the locator says which subobject the table is for
		std::string mangled = name;
		bool repeat = false;
		size_t underscore = mangled.rfind('_');
		if (underscore != std::string::npos && underscore > 0 && mangled[underscore - 1] == '@' &&
			underscore + 1 < mangled.size() &&
			std::all_of(mangled.begin() + underscore + 1, mangled.end(), [](char c) { return std::isdigit(c); }))
		{
			mangled.erase(underscore);
			repeat = true;
		}
		std::optional<std::string> demangled = demangleVftable(mangled);
		if (!demangled)
			continue;
		const std::string& spelled = *demangled;
		size_t marker = spelled.rfind("::`vftable'");
		bool secondary = repeat || spelled.find("{for", marker) != std::string::npos;
		std::optional<int64_t> offset = locatorOffset(address);
		if (!offset)
		{
			// no RTTI: only the primary table has a known offset
			if (secondary)
				continue;
			offset = 0;
		}
		Table table;
		table.cls = spelled.substr(0, marker);
		table.offset = *offset;
		table.address = address;
		for (ea_t slot = address; slot < address + 4096; slot += 4)
		{
			// the map's labels are user names in the IDB, its strings' auto names (`aText`); IDA's dummy names
			// (off_, unk_) are no label
			flags64_t flags = get_flags(slot);
			if (slot != address && (has_user_name(flags) || has_auto_name(flags) || storedAsVfptr(slot)))
				break;
			ea_t value = get_dword(slot);
			if (value < text->start_ea || value >= text->end_ea)
				break;
			table.entries.push_back(value);
		}
		auto key = std::make_pair(classKey(table.cls), table.offset);
		if (m_byKey.count(key))
			continue;
		m_byKey.emplace(key, m_tables.size());
		m_tables.push_back(std::move(table));
	}
	// after the scan: holders point into m_tables
	for (const Table& table : m_tables)
	{
		for (size_t slot = 0; slot < table.entries.size(); ++slot)
			m_holders[table.entries[slot]].push_back({&table, static_cast<int>(slot)});
		if (table.offset == 0 && locatorOffset(table.address))
		{
			ea_t locator = get_dword(table.address - 4);
			if (std::optional<std::string> base = rttiParent(locator))
				m_parents.emplace(classKey(table.cls), *base);
			for (const auto& [base, displacement] : rttiBases(locator))
				m_derived[classKey(base)].emplace_back(table.cls, displacement);
		}
	}
}

const ExeVtables::Table* ExeVtables::find(const std::string& cls, int64_t offset) const
{
	auto found = m_byKey.find({classKey(cls), offset});
	return found == m_byKey.end() ? nullptr : &m_tables[found->second];
}

std::vector<const ExeVtables::Table*> ExeVtables::of(const std::string& cls) const
{
	std::vector<const Table*> out;
	std::string key = classKey(cls);
	for (auto found = m_byKey.lower_bound({key, INT64_MIN}); found != m_byKey.end() && found->first.first == key;
		 ++found)
		out.push_back(&m_tables[found->second]);
	return out;
}

const std::vector<ExeVtables::Holder>& ExeVtables::holders(ea_t entry) const
{
	static const std::vector<Holder> none;
	auto found = m_holders.find(entry);
	return found == m_holders.end() ? none : found->second;
}

const std::string* ExeVtables::parent(const std::string& cls) const
{
	auto found = m_parents.find(classKey(cls));
	return found == m_parents.end() ? nullptr : &found->second;
}

std::vector<std::string> ExeVtables::chain(const std::string& cls) const
{
	std::vector<std::string> out{cls};
	for (const std::string* base = parent(cls); base; base = parent(*base))
	{
		auto same = [&](const std::string& c) { return classKey(c) == classKey(*base); };
		if (std::any_of(out.begin(), out.end(), same))
			break;
		out.push_back(*base);
	}
	return out;
}

std::vector<ea_t> ExeVtables::slotFunctions(const std::string& cls, int64_t offset) const
{
	// the class's own table, and each derived class's table for the subobject the class is there
	std::vector<std::pair<std::string, int64_t>> tables{{cls, 0}};
	if (auto derived = m_derived.find(classKey(cls)); derived != m_derived.end())
		tables.insert(tables.end(), derived->second.begin(), derived->second.end());
	std::vector<ea_t> out;
	for (const auto& [c, subobject] : tables)
	{
		const Table* table = find(c, subobject);
		if (table == nullptr || offset < 0 || size_t(offset / 4) >= table->entries.size())
			continue;
		ea_t entry = table->entries[offset / 4];
		ea_t target = thunk(entry).value_or(std::pair<int64_t, ea_t>{0, entry}).second;
		if (std::find(out.begin(), out.end(), target) == out.end())
			out.push_back(target);
	}
	return out;
}

std::optional<std::pair<int64_t, ea_t>> ExeVtables::thunk(ea_t entry)
{
	insn_t first, second;
	if (decode_insn(&first, entry) <= 0 || first.itype != NN_sub || first.ops[1].type != o_imm)
		return std::nullopt;
	// the this the thunk adjusts: ecx (register 1 in IDA's x86 numbering: R_ax, R_cx, ...), or a stdcall's first
	// stack argument, `dword ptr [esp+4]` (83/81 6C 24 04: ModRM [disp8 + SIB], SIB base esp, no index)
	bool ecx = first.ops[0].type == o_reg && first.ops[0].reg == 1;
	bool stack = (get_byte(entry) == 0x83 || get_byte(entry) == 0x81) && get_byte(entry + 1) == 0x6C &&
				 get_byte(entry + 2) == 0x24 && get_byte(entry + 3) == 0x04;
	if (!ecx && !stack)
		return std::nullopt;
	if (decode_insn(&second, entry + first.size) <= 0 || second.itype != NN_jmp || second.ops[0].type != o_near)
		return std::nullopt;
	return std::make_pair(-signed32(first.ops[1].value), second.ops[0].addr);
}

}  // namespace idakit
