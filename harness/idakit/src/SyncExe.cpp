// The exe phase's pins, what the exe's code says (exefacts) about the functions src does not declare, and IDA's
// outlined-code marks cleared.

#include "IdaSupport.hpp"

#include "idakit/ExePrototype.hpp"
#include "idakit/Sync.hpp"

#include <Facts.hpp>
#include <demangle.hpp>

#include <algorithm>
#include <cstring>
#include <set>

namespace idakit
{

namespace
{

// the function's type and its details; nullopt without a function type
std::optional<std::pair<tinfo_t, func_type_data_t>> functionType(ea_t address)
{
	tinfo_t type;
	func_type_data_t fd;
	if (!get_tinfo(&type, address) || !type.get_func_details(&fd))
		return std::nullopt;
	return std::pair{type, fd};
}

// the exefacts family of a whole register, its low word or low byte (IDA's registers 0-7, al ... bl 16-19); 0 for
// another location (esp, ebp, ah ..., a pair, an offset into one, the stack)
exefacts::Families registerFamily(const argloc_t& where)
{
	static constexpr exefacts::Families kFamilies[8] = {exefacts::kEax, exefacts::kEcx, exefacts::kEdx, exefacts::kEbx,
														0,				0,				exefacts::kEsi, exefacts::kEdi};
	if (!where.is_reg1() || where.regoff() != 0)
		return 0;
	int reg = where.reg1() >= 16 && where.reg1() < 20 ? where.reg1() - 16 : where.reg1();
	return reg >= 0 && reg < 8 ? kFamilies[reg] : 0;
}

// IDA's name names a non-static class member: a mangled one whose demangling has an access specifier and no
// `static` (or does not demangle), an unmangled one with a scope (a namespace's function too: it is not told apart)
bool memberName(ea_t address)
{
	std::string name = get_name(address).c_str();
	if (!name.starts_with('?'))
		return !scopeOf(name).empty();
	qstring demangled;
	if (demangle_name(&demangled, name.c_str(), 0) <= 0)
		return true;
	std::string text = demangled.c_str();
	for (const char* access : {"public: ", "protected: ", "private: "})
		if (text.starts_with(access))
			return text.compare(std::strlen(access), 7, "static ") != 0;
	return false;
}

// fd's arguments in the standard convention that keeps their registers (kept, in argument order): none left,
// stdcall where the function pops, else cdecl; ecx a thiscall; ecx and edx a fastcall. nullopt where that
// convention puts a register argument or the return elsewhere or pops other than pops
std::optional<tinfo_t> standardConvention(func_type_data_t fd, const std::vector<exefacts::Families>& kept,
										  uint32_t pops)
{
	callcnv_t cc = CM_CC_INVALID;
	if (kept.empty())
		cc = pops ? CM_CC_STDCALL : CM_CC_CDECL;
	else if (kept == std::vector<exefacts::Families>{exefacts::kEcx})
		cc = CM_CC_THISCALL;
	else if (kept == std::vector<exefacts::Families>{exefacts::kEcx, exefacts::kEdx})
		cc = CM_CC_FASTCALL;
	else
		return std::nullopt;
	const func_type_data_t wanted = fd;
	for (funcarg_t& a : fd)
		a.argloc = argloc_t();
	fd.retloc = argloc_t();
	fd.set_cc(cc);
	tinfo_t type;
	func_type_data_t got;
	if (!type.create_func(fd) || !type.get_func_details(&got) || got.size() != wanted.size())
		return std::nullopt;
	for (size_t k = 0; k < wanted.size(); ++k)
		if (wanted[k].argloc.is_reg() && got[k].argloc != wanted[k].argloc)
			return std::nullopt;
	if (!wanted.rettype.is_void() && got.retloc != wanted.retloc)
		return std::nullopt;
	if (type.calc_purged_bytes() != int(pops))
		return std::nullopt;
	return type;
}

}  // namespace

void Sync::pins()
{
	for (ea_t address : gameFunctions())
		if (!srcDeclares(address))
		{
			pinVariadic(address);
			pinUsercall(address);
			pinReturn(address);
			pinReturnWidth(address);
			pinDeadRegisters(address);
		}
}

void Sync::outlined()
{
	for (ea_t address : gameFunctions())
		if (func_t* f = get_func(address); f && (f->flags & FUNC_OUTLINE))
		{
			f->flags &= ~FUNC_OUTLINE;
			if (!update_func(f))
			{
				count("outlined marks failed");
				log("outlined-failed", hex(address), "update_func failed");
				continue;
			}
			count("outlined marks cleared");
			log("outlined-cleared", hex(address), "");
		}
}

// A C-variadic function (varying caller cleanups and a va_start) becomes `R __cdecl(fixed..., ...)`, the fixed part
// keeping its types (Interface::PrintLine 0x703C00 was `void(const char*)` and dropped the callers' format arguments).
void Sync::pinVariadic(ea_t address)
{
	std::optional<int32_t> fixed;
	try
	{
		fixed = m_facts.variadicFixedArgs(static_cast<uint32_t>(address));
	}
	catch (const exefacts::Raised&)
	{
	}
	if (!fixed)
		return;
	auto known = functionType(address);
	if (known && known->second.is_vararg_cc())
	{
		count("variadic already");
		return;
	}
	func_type_data_t fd;
	fd.rettype = known ? known->second.rettype : tinfo_t(BT_INT32);
	for (int32_t k = 0; k < *fixed; ++k)
	{
		funcarg_t arg;
		bool have = known && size_t(k) < known->second.size();
		arg.type = have ? known->second[k].type : tinfo_t(BT_INT32);
		arg.name = have ? known->second[k].name : qstring(("a" + std::to_string(k + 1)).c_str());
		fd.push_back(arg);
	}
	fd.set_cc(CM_CC_ELLIPSIS);
	tinfo_t type;
	if (!type.create_func(fd) || !writePrototype(address, type))
	{
		count("variadic unforceable");
		return;
	}
	count("variadic pinned");
	log("variadic-pinned", hex(address), text(type) + " (was: " + (known ? text(known->first) : "untyped") + ")");
}

// A function reading registers outside every standard convention (eax, ebx, esi, edi) before writing them gets them
// as register arguments of a `__usercall` (`__userpurge` when it pops its stack arguments), its arguments and return
// location kept; its callers' decompiles then pass the values (0xD00370, `void __fastcall()`, reads esi). Not for a
// function that overwrites a callee-saved input without saving it (a C++ body cannot reproduce that), nor a
// C-variadic one (the type would drop its `...`).
void Sync::pinUsercall(ea_t address)
{
	uint32_t at = static_cast<uint32_t>(address);
	std::vector<std::pair<int, std::string>> registers = inputRegisters(m_facts, address);
	if (registers.empty())
		return;
	if (m_facts.clobbers(at) & m_facts.regArgs(at) & (exefacts::kEbx | exefacts::kEsi | exefacts::kEdi))
	{
		count("usercall clobbers a callee-saved input");
		return;
	}
	auto known = functionType(address);
	if (known && known->first.is_vararg_cc())
	{
		count("usercall skipped: variadic");
		return;
	}
	bool readsStack = false;
	std::optional<uint32_t> pops;
	try
	{
		readsStack = !m_facts.abi(at).args.empty();
		pops = m_facts.pops(at);
	}
	catch (const exefacts::Raised&)
	{
	}
	if (!known && (readsStack || (pops && *pops)))
	{
		count("usercall untyped with stack args");
		return;
	}
	// the type's arguments where they are; each input register it does not pass yet an `int` argument there
	func_type_data_t fd;
	if (known)
		fd = known->second;
	else
		fd.rettype = voidType();
	std::set<int> located;
	for (const funcarg_t& a : fd)
		if (a.argloc.is_reg1())
			located.insert(a.argloc.reg1());
	callcnv_t cc = fd.get_cc() & CM_CC_MASK;
	bool added = false;
	for (const auto& [reg, name] : registers)
		if (!located.contains(reg))
		{
			funcarg_t arg;
			arg.type = tinfo_t(BT_INT32);
			arg.name = ("r_" + name).c_str();
			arg.argloc.set_reg1(reg);
			fd.push_back(arg);
			added = true;
		}
	if (!added && known && (cc == CM_CC_SPECIAL || cc == CM_CC_SPECIALP))
	{
		count("usercall already");
		return;
	}
	// a function whose pops the exe does not prove (no tail-jump target is known) keeps its type's; an untyped one is
	// left
	if (!pops && !known)
	{
		count("usercall skipped: pops unknown");
		return;
	}
	bool purges = pops ? *pops != 0 : known->first.calc_purged_bytes() != 0;
	std::optional<tinfo_t> type = usercall(std::move(fd), purges);
	if (!type || !writePrototype(address, *type))
	{
		count("usercall unforceable");
		return;
	}
	count("usercall pinned");
	log("usercall-pinned", hex(address), text(*type) + " (was: " + (known ? text(known->first) : "untyped") + ")");
}

// A void function whose code sets eax and whose direct callers read eax after the call at one width returns that
// width (BSAudioManager::ProcessMessage 0xADF080: ProcessMessages does `movzx eax, al; test`). A type that already
// returns what the rule gives is counted.
void Sync::pinReturn(ea_t address)
{
	auto known = functionType(address);
	if (!known)
		return;
	std::string why;
	if (!known->second.rettype.is_void())
	{
		func_type_data_t fd = known->second;
		fd.rettype = voidType();
		fd.retloc = argloc_t();
		if (tinfo_t voided; voided.create_func(fd))
			if (std::optional<tinfo_t> type = exeReturn(m_facts, m_asm, address, voided, why);
				type && sameType(*type, known->first))
				count("return already");
		return;
	}
	std::optional<tinfo_t> type = exeReturn(m_facts, m_asm, address, known->first, why);
	if (!why.empty())
		count("return widths disagree or st");
	if (!type)
		return;
	if (!writePrototype(address, *type))
	{
		count("return unforceable");
		return;
	}
	count("return pinned");
	log("return-pinned", hex(address), text(*type) + " (was: " + text(known->first) + ")");
}

// An integer return narrower than the bytes of eax every return path writes is that width, where a caller reads more
// of eax than the type holds (ExePrototype's widerReturn; TESForm::GetFormType 0x401170 ends `movzx eax, byte
// [eax+4]`: callers `cmp eax, N` read all four bytes, which a one-byte type leaves undefined in their decompile). A
// type the rule judges already right (returnHoldsWrites) is counted.
void Sync::pinReturnWidth(ea_t address)
{
	auto known = functionType(address);
	if (!known)
		return;
	std::optional<tinfo_t> type = widerReturn(m_facts, m_asm, address, known->first);
	if (!type)
	{
		if (returnHoldsWrites(m_facts, m_asm, address, known->first) == true)
			count("return width already");
		return;
	}
	if (!writePrototype(address, *type))
	{
		count("return width unforceable");
		return;
	}
	count("return width widened");
	log("return-width-widened", hex(address), text(*type) + " (was: " + text(known->first) + ")");
}

// A register argument of IDA's prototype (a thiscall's or fastcall's ecx, a fastcall's edx, any register of a
// __usercall or __userpurge) that the code never reads before writing it, on every path, is no argument
// (exefacts' registerInputs: a `push ecx` that only reserves a slot is no read, a register passed on to a callee
// that reads it is one): with it in the database every caller's decompile passes it a value (0x55A1E0's callers an
// undefined local, 0xD51270's its own `this`). It is dropped and the convention follows from what is left and
// the exe's pops: no register argument left, stdcall where the function pops (`ret N`), else cdecl; ecx alone
// first a thiscall, ecx and edx first a fastcall, where that convention keeps every remaining register and the
// return location and pops what the exe pops; else a __userpurge (pops) or __usercall (IDA's choice of the two
// where the exe's pops are unknown: 0xE2F910 leaves through `jmp eax`). An unread ecx stays where
// the function may be a member that ignores `this`: its address is in a vtable, or IDA's name (the PDB's names
// reach the database as IDA's) names a non-static class member. A type whose registers are all read is counted
// as already matching.
void Sync::pinDeadRegisters(ea_t address)
{
	auto known = functionType(address);
	if (!known)
		return;
	callcnv_t cc = known->second.get_cc() & CM_CC_MASK;
	if (cc != CM_CC_THISCALL && cc != CM_CC_FASTCALL && cc != CM_CC_SPECIAL && cc != CM_CC_SPECIALP)
		return;
	uint32_t at = static_cast<uint32_t>(address);
	exefacts::Families inputs = m_facts.registerInputs(at);
	// the register arguments the code does not read, by index (a location registerFamily cannot name is read)
	std::vector<size_t> dead;
	bool registers = false;
	for (size_t k = 0; k < known->second.size(); ++k)
		if (known->second[k].argloc.is_reg())
		{
			registers = true;
			exefacts::Families family = registerFamily(known->second[k].argloc);
			if (family && !(inputs & family))
				dead.push_back(k);
		}
	if (!registers)
		return;
	if (dead.empty())
	{
		count("dead registers already");
		return;
	}
	auto isEcx = [&](size_t k) { return registerFamily(known->second[k].argloc) == exefacts::kEcx; };
	if (std::any_of(dead.begin(), dead.end(), isEcx))
	{
		std::string member = !m_exe.holders(address).empty() ? "a vtable slot"
						   : memberName(address)			  ? "a member's name"
															  : "";
		if (!member.empty())
		{
			std::erase_if(dead, isEcx);
			count("dead ecx kept: " + member);
			log("dead-ecx-kept", hex(address), member + ": " + text(known->first));
			if (dead.empty())
				return;
		}
	}
	func_type_data_t fd;
	fd.rettype = known->second.rettype;
	fd.retloc = known->second.retloc;
	std::vector<exefacts::Families> kept;
	std::string dropped;
	for (size_t k = 0; k < known->second.size(); ++k)
	{
		const funcarg_t& a = known->second[k];
		if (std::find(dead.begin(), dead.end(), k) != dead.end())
		{
			qstring name;
			get_reg_name(&name, a.argloc.reg1(), a.type.get_size());
			dropped += (dropped.empty() ? "" : ", ") + std::string(name.c_str());
			continue;
		}
		fd.push_back(a);
		if (a.argloc.is_reg())
			kept.push_back(registerFamily(a.argloc));
	}
	std::optional<uint32_t> pops;
	try
	{
		pops = m_facts.pops(at);
	}
	catch (const exefacts::Raised&)
	{
	}
	// a usercall keeping some register keeps IDA's pops where the exe's are unknown (it leaves through `jmp eax`)
	bool user = cc == CM_CC_SPECIAL || cc == CM_CC_SPECIALP;
	if (!pops && !(user && !kept.empty()))
	{
		count("dead registers: pops unknown");
		log("dead-registers-pops-unknown", hex(address), dropped + ": " + text(known->first));
		return;
	}
	int purged = pops ? int(*pops) : known->first.calc_purged_bytes();
	std::optional<tinfo_t> type = pops ? standardConvention(fd, kept, *pops) : std::nullopt;
	if (!type && !kept.empty())
		type = usercall(fd, purged != 0);
	if (!type || type->calc_purged_bytes() != purged)
	{
		count("dead registers: stack arguments not the exe's pops");
		log("dead-registers-pops", hex(address),
			dropped + ": pops " + std::to_string(purged) + ", " + text(known->first));
		return;
	}
	if (!writePrototype(address, *type))
	{
		count("dead registers unforceable");
		log("dead-registers-unforceable", hex(address), dropped + ": " + text(*type));
		return;
	}
	count("dead registers dropped");
	log("dead-registers-dropped", hex(address), dropped + ": " + text(*type) + " (was: " + text(known->first) + ")");
}

}  // namespace idakit
