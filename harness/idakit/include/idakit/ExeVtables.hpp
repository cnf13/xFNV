// The exe's vtables as the open database names them: every `??_7X@@6B@...` name (the map names each of a class's
// tables `??_7X@@6B@`, IDA the repeats `..._0`, `..._1`), the subobject's offset from the complete object locator in
// the dword before the table (a class without RTTI, the shader library's: its one table, offset 0), and each class's
// primary base from its RTTI class hierarchy.
//
// A table ends at the first dword outside .text (the next table's locator), at the next label (a user name, or a
// string's auto name: data that happens to look like a .text address), or at a later slot code stores as a vfptr
// (`mov [reg], offset slot`: a table without RTTI or name starts there). Those are vtable-check.py's bounds, read from
// the IDB instead of the PE and map.
#pragma once

#include "idakit/Ida.hpp"

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace idakit
{

class ExeVtables
{
public:
	struct Table
	{
		std::string cls;  // as the demangler spells it (`NiTMap<unsigned int,class NiNode *>`)
		int64_t offset = 0;
		ea_t address = BADADDR;
		std::vector<ea_t> entries;
	};

	struct Holder
	{
		const Table* table;
		int slot;
	};

	// Reads every table of the open database.
	ExeVtables();
	ExeVtables(const ExeVtables&) = delete;  // holders point into m_tables
	ExeVtables& operator=(const ExeVtables&) = delete;

	// The class's table for the subobject at the offset (0: the primary one), by spelling-insensitive class name.
	const Table* find(const std::string& cls, int64_t offset) const;
	// Every table of the class, the primary one first.
	std::vector<const Table*> of(const std::string& cls) const;
	const std::vector<Table>& tables() const { return m_tables; }
	// Every table slot the entry fills.
	const std::vector<Holder>& holders(ea_t entry) const;
	// The class's primary base: the first entry after the class in its RTTI base class array that sits at offset 0
	// (a vtable of its own or not; MSVC may put a polymorphic base ahead of a non-polymorphic first one). nullptr
	// without one, or for a class without RTTI.
	const std::string* parent(const std::string& cls) const;
	// The class and its primary bases, nearest first.
	std::vector<std::string> chain(const std::string& cls) const;
	// The functions the slot at the byte offset holds in the class's primary table and, for each class deriving from
	// it (a non-virtual base in that class's RTTI hierarchy), in its table for the subobject the class is (an adjustor
	// thunk as its target), each once: the functions a call through that slot on an object of the class can reach.
	std::vector<ea_t> slotFunctions(const std::string& cls, int64_t offset) const;

	// (this adjustment, target) of an adjustor thunk, the compiler's `sub ecx, N` or a stdcall's (COM) `sub dword ptr
	// [esp+4], N`, then `jmp target`. `add ecx, N; jmp target` is a body: it tail-calls a member of a member (N > 0)
	// or, an override in a secondary vtable, of the full object (N < 0: 0xD647F0, 0xCD2460).
	static std::optional<std::pair<int64_t, ea_t>> thunk(ea_t entry);
	// A class name's spelling-insensitive key: no class-keys or blanks, `std::` as src's `msvc9::`, `__int64` as
	// `long long`, a builtin's or name's postfix `const` before it, both spellings of the anonymous namespace alike,
	// a function type's `__cdecl` and `(void)` as clang's `()`. Matches a demangled name to clang's.
	static std::string classKey(const std::string& name);

private:
	std::vector<Table> m_tables;
	std::map<std::pair<std::string, int64_t>, size_t> m_byKey;
	std::map<ea_t, std::vector<Holder>> m_holders;
	std::map<std::string, std::string> m_parents;  // by classKey
	// by a base's classKey: each class deriving from it, and the base's displacement there
	std::map<std::string, std::vector<std::pair<std::string, int64_t>>> m_derived;
};

}  // namespace idakit
