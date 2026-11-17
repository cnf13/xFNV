#include "idakit/ExePrototype.hpp"

#include "IdaSupport.hpp"

#include "idakit/AsmFacts.hpp"

#include <Facts.hpp>

#include <algorithm>
#include <set>
#include <vector>

namespace idakit
{

namespace
{

// the stack bytes an argument takes; nullopt for an unsized type (it fits no popped byte count)
std::optional<uint64_t> slotBytes(const tinfo_t& type)
{
	size_t size = type.get_size();
	if (size == BADSIZE)
		return std::nullopt;
	return (size + 3) & ~uint64_t(3);
}

funcarg_t argument(const tinfo_t& type, const qstring& name)
{
	funcarg_t arg;
	arg.type = type;
	arg.name = name;
	return arg;
}

}  // namespace

uint8_t ecxEdxReads(exefacts::Facts& facts, ea_t function)
{
	uint32_t at = static_cast<uint32_t>(function);
	const exefacts::Abi& abi = facts.abi(at);
	uint8_t read = (abi.ecx ? exefacts::kEcx : 0) | (abi.edx ? exefacts::kEdx : 0);
	return read & facts.registerInputs(at);
}

bool contradicted(exefacts::Facts& facts, ea_t function, const tinfo_t& type)
{
	func_type_data_t fd;
	if (!type.get_func_details(&fd))
		return false;
	callcnv_t cc = fd.get_cc() & CM_CC_MASK;
	if (cc != CM_CC_CDECL && cc != CM_CC_STDCALL && cc != CM_CC_THISCALL && cc != CM_CC_FASTCALL)
		return false;
	const exefacts::Abi* abi;
	uint8_t reads = 0;
	try
	{
		abi = &facts.abi(static_cast<uint32_t>(function));
		reads = ecxEdxReads(facts, function);
	}
	catch (const exefacts::Raised&)
	{
		return false;
	}
	std::optional<uint32_t> pops;
	try
	{
		pops = facts.pops(static_cast<uint32_t>(function));
	}
	catch (const exefacts::Raised&)
	{
		// a tail target exefacts cannot decode leaves the pops unknown
	}
	if ((reads & exefacts::kEcx) && cc != CM_CC_THISCALL && cc != CM_CC_FASTCALL)
		return true;
	if ((reads & exefacts::kEdx) && cc != CM_CC_FASTCALL)
		return true;
	// pops exefacts cannot prove (no tail-jump target of the code is known) contradict nothing
	if (!pops)
		return false;
	// A `ret N` fixes the argument count itself: reads past it are the stack walk's drift (0xB99770: `ret 8`, "reads"
	// slots 3 and 5), not the prototype's
	if (*pops == 0 && !abi->args.empty() && uint64_t(abi->args.back()) >= fd.stkargs / 4)
		return true;
	return *pops != uint32_t(type.calc_purged_bytes());
}

std::optional<tinfo_t> exePrototype(exefacts::Facts& facts, ea_t function, const tinfo_t& type)
{
	func_type_data_t old;
	if (!type.get_func_details(&old))
		return std::nullopt;
	const exefacts::Abi* abi;
	std::optional<uint32_t> known;
	uint8_t reads = 0;
	try
	{
		abi = &facts.abi(static_cast<uint32_t>(function));
		known = facts.pops(static_cast<uint32_t>(function));
		reads = ecxEdxReads(facts, function);
	}
	catch (const exefacts::Raised&)
	{
		return std::nullopt;
	}
	// no convention without the bytes the code pops
	if (!known)
		return std::nullopt;
	const uint32_t pops = *known;
	if (reads & exefacts::kEdx)
		return std::nullopt;
	std::vector<const funcarg_t*> registers, stack;
	for (const funcarg_t& a : old)
		(a.argloc.is_stkoff() ? stack : registers).push_back(&a);
	if ((old.get_cc() & CM_CC_MASK) == CM_CC_FASTCALL && registers.size() > 1)
		return std::nullopt;
	callcnv_t cc = CM_CC_THISCALL;
	if (!(reads & exefacts::kEcx))
	{
		cc = pops ? CM_CC_STDCALL : CM_CC_CDECL;
		// an ecx argument the type had becomes the first stack one (dropped when it does not fit)
		stack.insert(stack.begin(), registers.begin(), registers.end());
		registers.clear();
	}
	uint64_t want = pops;
	if (cc == CM_CC_CDECL)
	{
		uint64_t declared = 0;
		for (const funcarg_t* a : stack)
			declared += slotBytes(a->type).value_or(0);
		uint64_t read = abi->args.empty() ? 0 : 4 * (uint64_t(abi->args.back()) + 1);
		want = std::max(declared, read);
	}
	func_type_data_t fd;
	fd.rettype = old.rettype;
	fd.set_cc(cc);
	if (cc == CM_CC_THISCALL)
		fd.push_back(registers.empty() ? argument(pointerTo(voidType()), "this")
									   : argument(registers[0]->type, registers[0]->name));
	uint64_t used = 0;
	for (const funcarg_t* a : stack)
	{
		std::optional<uint64_t> size = slotBytes(a->type);
		if (!size || used + *size > want)
		{
			if (used != want)
				return std::nullopt;
			break;
		}
		fd.push_back(argument(a->type, a->name));
		used += *size;
	}
	for (int k = 0; used < want; ++k, used += 4)
		fd.push_back(argument(tinfo_t(BT_INT32), qstring("x") + std::to_string(k).c_str()));
	tinfo_t out;
	if (!out.create_func(fd))
		return std::nullopt;
	return out;
}

std::optional<tinfo_t> exeReturn(exefacts::Facts& facts, AsmFacts& asmFacts, ea_t function, const tinfo_t& type,
								 std::string& why)
{
	func_type_data_t fd;
	if (!type.get_func_details(&fd) || !fd.rettype.is_void())
		return std::nullopt;
	try
	{
		if (!facts.leavesEax(static_cast<uint32_t>(function)))
			return std::nullopt;
	}
	catch (const exefacts::Raised&)
	{
		return std::nullopt;
	}
	std::set<int> widths;
	for (int width : asmFacts.resultReads(function))
		if (width != 0)
			widths.insert(width);
	if (widths.empty())
		return std::nullopt;
	int width = *widths.begin();
	if (widths.size() != 1 || (width != 1 && width != 2 && width != 4))
	{
		why = "callers read the result at more than one width or in ST0";
		return std::nullopt;
	}
	type_t base = width == 1 ? BT_INT8 | BTMT_USIGNED : width == 2 ? BT_INT16 : BT_INT32;
	fd.rettype = tinfo_t(base);
	tinfo_t out;
	if (!out.create_func(fd))
		return std::nullopt;
	return out;
}

std::optional<bool> returnHoldsWrites(exefacts::Facts& facts, AsmFacts& asmFacts, ea_t function, const tinfo_t& type)
{
	func_type_data_t fd;
	if (!type.get_func_details(&fd))
		return std::nullopt;
	const tinfo_t& ret = fd.rettype;
	if (ret.is_void() || ret.is_ptr() || ret.is_floating() || !(ret.is_integral() || ret.is_enum() || ret.is_bool()))
		return std::nullopt;
	std::optional<exefacts::ReturnWidth> exe = facts.returnWidth(static_cast<uint32_t>(function));
	size_t size = ret.get_size();
	int read = 0;
	for (int width : asmFacts.resultReads(function))
		if (width == 1 || width == 2 || width == 4)
			read = std::max(read, width);
	if (!exe || size == BADSIZE || read == 0)
		return std::nullopt;
	return exe->width <= size || size_t(read) <= size;
}

std::optional<tinfo_t> widerReturn(exefacts::Facts& facts, AsmFacts& asmFacts, ea_t function, const tinfo_t& type)
{
	if (returnHoldsWrites(facts, asmFacts, function, type) != false)
		return std::nullopt;
	func_type_data_t fd;
	type.get_func_details(&fd);
	std::optional<exefacts::ReturnWidth> exe = facts.returnWidth(static_cast<uint32_t>(function));
	bool isSigned = exe->extension < 0 || (exe->extension == 0 && fd.rettype.is_signed());
	type_t base = exe->width == 4 ? BT_INT32 : BT_INT16;
	fd.rettype = tinfo_t(type_t(base | (isSigned ? BTMT_SIGNED : BTMT_USIGNED)));
	fd.retloc = argloc_t();
	tinfo_t out;
	if (!out.create_func(fd))
		return std::nullopt;
	return out;
}

std::vector<std::pair<int, std::string>> inputRegisters(exefacts::Facts& facts, ea_t function)
{
	static constexpr std::pair<exefacts::Family, const char*> kInputs[] = {
		{exefacts::kEax, "eax"}, {exefacts::kEbx, "ebx"}, {exefacts::kEdi, "edi"}, {exefacts::kEsi, "esi"}};
	exefacts::Families inputs = facts.regArgs(static_cast<uint32_t>(function));
	std::vector<std::pair<int, std::string>> registers;
	for (const auto& [family, name] : kInputs)
		if (inputs & family)
			registers.emplace_back(str2reg(name), name);
	return registers;
}

std::optional<tinfo_t> usercall(func_type_data_t fd, bool pops)
{
	uint64_t offset = 0;
	for (funcarg_t& a : fd)
		if (!a.argloc.is_reg())
		{
			std::optional<uint64_t> size = slotBytes(a.type);
			if (!size)
				return std::nullopt;
			a.argloc.set_stkoff(static_cast<sval_t>(offset));
			offset += *size;
		}
	fd.set_cc(pops ? CM_CC_SPECIALP : CM_CC_SPECIAL);
	tinfo_t type;
	if (!type.create_func(fd))
		return std::nullopt;
	return type;
}

std::string unpassedRegisters(exefacts::Facts& facts, ea_t function, const tinfo_t& type)
{
	std::vector<std::pair<int, std::string>> read = inputRegisters(facts, function);
	try
	{
		uint8_t reads = ecxEdxReads(facts, function);
		if (reads & exefacts::kEcx)
			read.emplace_back(str2reg("ecx"), "ecx");
		if (reads & exefacts::kEdx)
			read.emplace_back(str2reg("edx"), "edx");
	}
	catch (const exefacts::Raised&)
	{
	}
	// a register by its family: a low byte (al, cl, dl, bl: IDA's 16 to 19) passes its dword's register
	auto family = [](int reg) { return reg >= 16 && reg < 20 ? reg - 16 : reg; };
	func_type_data_t fd;
	std::set<int> passed;
	if (type.get_func_details(&fd))
		for (const funcarg_t& a : fd)
			if (a.argloc.is_reg1() || a.argloc.is_reg2())
			{
				passed.insert(family(a.argloc.reg1()));
				if (a.argloc.is_reg2())
					passed.insert(family(a.argloc.reg2()));
			}
	std::string names;
	for (const auto& [reg, name] : read)
		if (!passed.contains(reg))
			names += (names.empty() ? "" : ", ") + name;
	return names;
}

std::string usercallMisfit(exefacts::Facts& facts, ea_t function, const tinfo_t& type)
{
	func_type_data_t fd;
	if (!type.get_func_details(&fd))
		return "the type has no details";
	uint32_t at = static_cast<uint32_t>(function);
	try
	{
		if (std::string unpassed = unpassedRegisters(facts, function, type); !unpassed.empty())
			return "the code reads " + unpassed + " before writing it, the type does not pass it";
		exefacts::Families takes = facts.registerInputs(at);
		uint64_t stack = 0;
		for (const funcarg_t& a : fd)
		{
			if (a.argloc.is_stkoff())
			{
				stack = std::max(stack, uint64_t(a.argloc.stkoff()) + slotBytes(a.type).value_or(4));
				continue;
			}
			int reg = a.argloc.is_reg1() ? a.argloc.reg1() : -1;
			reg = reg >= 16 && reg < 20 ? reg - 16 : reg;
			if (reg < 0 || reg >= 8 || !(takes & (1u << reg)))
				return "the type passes " + locationText(a.argloc, a.type.get_size()) + ", which the code never takes";
		}
		const std::vector<int32_t>& args = facts.abi(at).args;
		uint64_t read = args.empty() ? 0 : 4 * (uint64_t(*std::max_element(args.begin(), args.end())) + 1);
		if (stack < read)
			return "the type passes " + std::to_string(stack) + " stack bytes, the code reads " + std::to_string(read);
		bool purge = (fd.get_cc() & CM_CC_MASK) == CM_CC_SPECIALP;
		if (std::optional<uint32_t> pops = facts.pops(at); pops && *pops != (purge ? stack : 0))
			return "the code pops " + std::to_string(*pops) + ", the type " + std::to_string(purge ? stack : 0);
	}
	catch (const exefacts::Raised& error)
	{
		return std::string("no exe facts: ") + error.what();
	}
	return "";
}

}  // namespace idakit
