// The exe's purge of an indirect call through a vtable slot: the bytes every implementation of the slot pops. The
// realigned frames' esp walk (idakit frames), the variadic calls' stack-slot model and the vtable call types (Unfold)
// read it.

#include "idakit/SrcPush.hpp"

#include "IdaSupport.hpp"

#include "idakit/Asm.hpp"

#include <Facts.hpp>

#include <set>

namespace idakit
{

namespace
{

// The operand an indirect call takes its target from: the call's own memory operand, or the `mov reg, [base+disp]`
// loading its register in the straight-line code before it
std::optional<std::pair<ea_t, int>> callSource(func_t* function, ea_t call)
{
	Insn insn(call);
	if (insn.opType(0) == o_displ || insn.opType(0) == o_phrase)
		return std::pair{call, 0};
	if (insn.opType(0) != o_reg)
		return std::nullopt;
	std::string reg = insn.opText(0);
	ea_t ea = call;
	for (int k = 0; k < 8; ++k)
	{
		ea = prevHead(ea, function->start_ea);
		if (ea == BADADDR || ea < function->start_ea)
			return std::nullopt;
		Insn before(ea);
		if (before.mnem().starts_with('j') || before.mnem() == "call" || before.mnem() == "ret" ||
			before.mnem() == "retn")
			return std::nullopt;
		if (before.opType(0) == o_reg && before.opText(0) == reg)
		{
			if (before.mnem() == "mov" && (before.opType(1) == o_displ || before.opType(1) == o_phrase))
				return std::pair{ea, 1};
			return std::nullopt;
		}
	}
	return std::nullopt;
}

}  // namespace

std::optional<int64_t> SrcPush::slotPops(const std::string& cls, int64_t offset)
{
	auto [cached, added] = m_slotPops.try_emplace({cls, offset});
	if (!added)
		return cached->second;
	// an implementation whose pops the exe does not prove says nothing
	std::set<int64_t> found;
	for (ea_t target : m_exe.slotFunctions(cls, offset))
	{
		try
		{
			if (std::optional<uint32_t> pops = m_facts.pops(static_cast<uint32_t>(target)))
				found.insert(*pops);
		}
		catch (const exefacts::Raised&)
		{
		}
	}
	if (found.size() == 1)
		cached->second = *found.begin();
	return cached->second;
}

std::optional<int64_t> SrcPush::indirectPops(func_t* function, ea_t call)
{
	auto source = callSource(function, call);
	if (!source)
		return std::nullopt;
	tid_t path[MAXSTRUCPATH];
	adiff_t delta;
	qstring name;
	if (get_stroff_path(path, &delta, source->first, source->second) <= 0 || !get_tid_name(&name, path[0]) ||
		!std::string_view(name.c_str()).ends_with("_vtbl"))
		return std::nullopt;
	std::string cls(name.c_str(), name.length() - 5);
	Insn insn(source->first);
	int64_t offset = insn.opType(source->second) == o_displ ? insn.opValue(source->second) & 0xFFFFFFFF : 0;
	if (std::optional<int64_t> pops = slotPops(cls, offset))
		return pops;
	tinfo_t vtbl;
	udm_t member;
	member.offset = offset * 8;
	if (!vtbl.get_type_by_tid(path[0]) || vtbl.find_udm(&member, STRMEM_OFFSET) < 0 ||
		member.offset != uint64(offset * 8))
		return std::nullopt;
	return stackBytes(member.type);
}

}  // namespace idakit
