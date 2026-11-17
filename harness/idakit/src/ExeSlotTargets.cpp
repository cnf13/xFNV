#include "idakit/ExeSlotTargets.hpp"

#include <algorithm>
#include <regex>

namespace idakit
{

namespace
{

// the outermost struct (a base or an embedded member) starting at byte `at` of a struct: the struct itself at 0
std::optional<tinfo_t> objectAt(tinfo_t type, int64_t at)
{
	while (at != 0)
	{
		udm_t member;
		member.offset = uint64(at) * 8;
		if (at < 0 || type.find_udm(&member, STRMEM_OFFSET) < 0 || !member.type.is_udt())
			return std::nullopt;
		at -= int64_t(member.offset / 8);
		type = member.type;
	}
	if (!type.is_udt())
		return std::nullopt;
	return type;
}

// The pointer a dword load at byte `at` of an object reads: a struct's pointer member starting there (bases and
// embedded members descended into), or an element of an array of pointers (the object is itself a pointer)
std::optional<tinfo_t> pointerAt(tinfo_t type, int64_t at)
{
	while (type.is_udt())
	{
		udm_t member;
		member.offset = uint64(at) * 8;
		if (at < 0 || type.find_udm(&member, STRMEM_OFFSET) < 0)
			return std::nullopt;
		at -= int64_t(member.offset / 8);
		type = member.type;
	}
	if (!type.is_ptr() || at % 4 != 0)
		return std::nullopt;
	return type;
}

// the pointer a function's database type passes at the root (-1: ecx; k: stack argument slot k)
std::optional<tinfo_t> argumentPointer(ea_t function, int32_t root)
{
	tinfo_t type;
	func_type_data_t fd;
	if (!get_tinfo(&type, function) || !type.get_func_details(&fd))
		return std::nullopt;
	for (const funcarg_t& a : fd)
	{
		// register 1 is ecx in IDA's x86 numbering (R_ax, R_cx, ...)
		bool here = root < 0 ? a.argloc.is_reg1() && a.argloc.reg1() == 1
							 : a.argloc.is_stkoff() && a.argloc.stkoff() == 4 * int64_t(root);
		if (here)
			return a.type.is_ptr() ? std::optional<tinfo_t>(a.type) : std::nullopt;
	}
	return std::nullopt;
}

// IDA's struct of a vtable's class: the demangler's spelling, else without its class-keys (IDA's spelling of a
// template instance, `hkpSymmetricAgent<hkpTransformAgent>`)
std::optional<tinfo_t> classStruct(const std::string& cls)
{
	static const std::regex keyword(R"(\b(?:class|struct|union|enum) )");
	for (const std::string& name : {cls, std::regex_replace(cls, keyword, "")})
		if (tinfo_t type; type.get_named_type(get_idati(), name.c_str()) && type.is_udt())
			return type;
	return std::nullopt;
}

}  // namespace

ExeSlotTargets::ExeSlotTargets(const ExeVtables& exe, exefacts::Facts& facts) : m_exe(exe), m_facts(facts)
{
	m_facts.setSlotTargets(this);
}

ExeSlotTargets::~ExeSlotTargets()
{
	m_facts.setSlotTargets(nullptr);
}

std::optional<std::string> ExeSlotTargets::receiverClass(tinfo_t pointer, int64_t at,
														 const exefacts::VtableJump& jump) const
{
	for (int32_t load : jump.loads)
	{
		std::optional<tinfo_t> loaded = pointerAt(pointer.get_pointed_object(), at + load);
		if (!loaded)
			return std::nullopt;
		pointer = *loaded;
		at = 0;
	}
	std::optional<tinfo_t> receiver = objectAt(pointer.get_pointed_object(), at + jump.receiver);
	qstring name;
	if (!receiver || !receiver->get_type_name(&name) || name.empty())
		return std::nullopt;
	return std::string(name.c_str());
}

std::optional<std::vector<uint32_t>> ExeSlotTargets::targets(uint32_t function, const exefacts::VtableJump& jump)
{
	// the root objects: (a pointer to the struct, the subobject offset the root points to)
	std::vector<std::pair<tinfo_t, int64_t>> roots;
	if (jump.root < 0)
	{
		std::vector<const ExeVtables::Table*> tables;
		for (const ExeVtables::Holder& holder : m_exe.holders(function))
			tables.push_back(holder.table);
		for (const ExeVtables::Table* table : tables)
		{
			// a class deriving from another holder's class: that class's slot covers its own
			std::vector<std::string> chain = m_exe.chain(table->cls);
			bool derived = std::any_of(tables.begin(), tables.end(), [&](const ExeVtables::Table* other) {
				return std::any_of(chain.begin() + 1, chain.end(), [&](const std::string& c) {
					return ExeVtables::classKey(c) == ExeVtables::classKey(other->cls);
				});
			});
			if (derived)
				continue;
			std::optional<tinfo_t> type = classStruct(table->cls);
			if (!type)
				return std::nullopt;
			tinfo_t pointer;
			pointer.create_ptr(*type);
			roots.emplace_back(pointer, table->offset);
		}
	}
	if (roots.empty())
	{
		std::optional<tinfo_t> pointer = argumentPointer(function, jump.root);
		if (!pointer)
			return std::nullopt;
		roots.emplace_back(*pointer, 0);
	}
	// every class the receiver may have: each root's
	std::vector<uint32_t> out;
	for (const auto& [pointer, at] : roots)
	{
		std::optional<std::string> cls = receiverClass(pointer, at, jump);
		if (!cls)
			return std::nullopt;
		for (ea_t target : m_exe.slotFunctions(*cls, jump.slot))
			if (std::find(out.begin(), out.end(), uint32_t(target)) == out.end())
				out.push_back(static_cast<uint32_t>(target));
	}
	if (out.empty())
		return std::nullopt;
	return out;
}

}  // namespace idakit
