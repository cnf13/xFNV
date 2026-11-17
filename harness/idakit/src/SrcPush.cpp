// What Sync and Unfold share: the src declaration behind an address, the folds, the log and the state dump
// (writeState): what the database holds after the phases, read from the database itself, so that two runs are equal
// exactly where their state.tsv files are.

#include "idakit/SrcPush.hpp"

#include "IdaSupport.hpp"

#include "idakit/Database.hpp"
#include "idakit/ExePrototype.hpp"

#include <Facts.hpp>

#include <algorithm>
#include <fstream>
#include <utility>

namespace idakit
{

namespace
{

// a cell without the separators of the TSV
std::string cell(std::string text)
{
	std::replace_if(text.begin(), text.end(), [](char c) { return c == '\t' || c == '\n' || c == '\r'; }, ' ');
	return text;
}

std::string row(std::initializer_list<std::string> cells)
{
	std::string out;
	for (const std::string& c : cells)
		out += (out.empty() ? "" : "\t") + cell(c);
	return out;
}

std::string typeText(const tinfo_t& type)
{
	return type.empty() ? "-" : text(type);
}

std::string addressType(ea_t address)
{
	tinfo_t type;
	return get_tinfo(&type, address) ? text(type) : "-";
}

void writeRows(const std::filesystem::path& path, const std::vector<std::string>& rows)
{
	std::ofstream out(path);
	for (const std::string& r : rows)
		out << r << '\n';
	if (!out)
		throw Error("idakit: cannot write " + path.string());
}

// the type the members of a fold share where they disagree: a pointer, reference, array or function as `void *`, an
// enum as the integer of its width; anything else (an integer, a float, a record by value) as it is
tinfo_t neutral(const tinfo_t& type)
{
	if (type.is_ptr() || type.is_array() || type.is_func())
		return pointerTo(voidType());
	if (type.is_enum())
	{
		size_t size = type.get_size();
		return tinfo_t(size == 1 ? BT_INT8 : size == 2 ? BT_INT16 : BT_INT32);
	}
	return type;
}

}  // namespace

SrcPush::SrcPush(std::string command, const SrcModel& model, const ExeVtables& exe, exefacts::Facts& facts,
				 SrcTypes& types)
	: m_command(std::move(command)), m_model(model), m_exe(exe), m_facts(facts), m_types(types), m_structNames(model)
{
}

void SrcPush::log(const std::string& category, const std::string& key, const std::string& detail)
{
	m_log.push_back({category, key, detail});
}

const SrcModel::Function* SrcPush::declared(ea_t address) const
{
	const SrcModel::Ref* ref = chooseRef(m_model, address).ref;
	if (ref && (ref->kind == "body" || ref->kind == "forwarder") && !ref->function.empty())
		if (const SrcModel::Function* f = m_model.function(ref->function))
			return f;
	for (const SrcModel::Ref& r : m_model.refsAt(address))
		if (r.kind == "address" && !r.function.empty())
			if (const SrcModel::Function* f = m_model.function(r.function))
				return f;
	return nullptr;
}

std::string SrcPush::callName(const SrcModel::Function& function)
{
	std::string name = idaFunctionName(function.name);
	return name.empty() ? function.name : name;
}

std::vector<const SrcModel::Function*> SrcPush::calledAs(ea_t address, const std::string& name, const tinfo_t& type)
{
	if (m_byCallName.empty())
	{
		for (const auto& [mangled, f] : m_model.functions())
			m_byCallName[callName(f)].push_back(&f);
		for (auto& [key, functions] : m_byCallName)
			std::sort(functions.begin(), functions.end(),
					  [](const SrcModel::Function* a, const SrcModel::Function* b) { return a->mangled < b->mangled; });
	}
	std::vector<const SrcModel::Function*> out;
	auto named = m_byCallName.find(name);
	if (named == m_byCallName.end())
		return out;
	for (const SrcModel::Function* f : named->second)
		if (std::optional<tinfo_t> own = prototype(*f, address); own && sameType(*own, type))
			out.push_back(f);
	return out;
}

// declared's own test: chooseRef's forwarder is the declaration where its function is known, a usercall
// registration (FNVD_REPLACES_USERCALL/_USERPURGE) the body that replaced it
bool SrcPush::usercallDeclared(ea_t address) const
{
	const SrcModel::Ref* ref = chooseRef(m_model, address).ref;
	bool user = (ref && ref->kind == "forwarder" && ref->form == "usercall") ||
				(ref && ref->kind == "body" && (ref->form == "USERCALL" || ref->form == "USERPURGE"));
	return user && !ref->function.empty() && m_model.function(ref->function);
}

std::vector<ea_t> SrcPush::definedAt(const SrcModel::Function& function)
{
	const std::vector<uint64_t>& registered = m_model.bodyAddresses(function.mangled);
	if (!registered.empty())
		return {registered.begin(), registered.end()};
	if (m_forwarders.empty())
	{
		for (const auto& [address, refs] : m_model.refs())
			for (const SrcModel::Ref& ref : refs)
				if (ref.kind == "forwarder")
					m_forwarders[ref.file].emplace_back(ref.line, address);
		for (auto& [file, lines] : m_forwarders)
			std::sort(lines.begin(), lines.end());
	}
	std::vector<ea_t> out;
	const SrcModel::Definition* definition = m_model.definition(function.mangled);
	auto file = definition ? m_forwarders.find(definition->file) : m_forwarders.end();
	if (file == m_forwarders.end())
		return out;
	auto from = std::lower_bound(file->second.begin(), file->second.end(), std::pair{definition->line, ea_t(0)});
	for (auto at = from; at != file->second.end() && at->first <= definition->endLine; ++at)
		if (std::find(out.begin(), out.end(), at->second) == out.end())
			out.push_back(at->second);
	return out;
}

std::optional<tinfo_t> SrcPush::usercallPrototype(ea_t address, std::string& why)
{
	why.clear();
	auto refuse = [&](std::string reason) -> std::optional<tinfo_t> {
		why = std::move(reason);
		return std::nullopt;
	};
	const SrcModel::Ref* ref = chooseRef(m_model, address).ref;
	const SrcModel::Function* function = ref ? m_model.function(ref->function) : nullptr;
	if (!function)
		return refuse("no src function");
	tinfo_t stored;
	func_type_data_t held;
	if (!get_tinfo(&stored, address) || !stored.get_func_details(&held))
		return refuse("the database has no prototype");
	callcnv_t cc = held.get_cc() & CM_CC_MASK;
	if (cc != CM_CC_SPECIAL && cc != CM_CC_SPECIALP)
		return refuse("the database's type is no usercall: " + text(stored));
	if (ref->locations.empty())
		return refuse("the model records no locations for " + function->name);
	// src's declaration as written: its parameters by the indexes the locations name
	std::optional<tinfo_t> declared = prototype(*function);
	func_type_data_t fd;
	if (!declared || !declared->get_func_details(&fd))
		return refuse("src's prototype is unmappable");
	if (fd.is_vararg_cc())
		return refuse("src's prototype is C-variadic");

	// src's form: each parameter at its location, the stack ones in order
	size_t first = !function->cls.empty() && !function->isStatic ? 1 : 0;
	std::vector<std::string> where(fd.size());
	for (size_t k = 0; k < fd.size(); ++k)
	{
		funcarg_t& a = fd[k];
		if (a.flags & FAI_RETPTR)
			return refuse("a record returned by value: the model records no location for its hidden result pointer");
		int64_t index = static_cast<int64_t>(k) - static_cast<int64_t>(first);
		std::string which = index < 0 ? std::string("this") : "parameter " + std::to_string(index + 1);
		auto found = ref->locations.find(index);
		if (found == ref->locations.end())
			return refuse("the model records no location for " + which);
		where[k] = found->second;
		size_t size = a.type.get_size();
		if (where[k] == "stack")
		{
			// usercall lays the stack arguments out again, in order
			a.argloc.set_stkoff(0);
			continue;
		}
		if (size == BADSIZE || size > 4 || a.type.is_floating() || a.type.is_udt())
			return refuse(which + ": src's " + text(a.type) + " fits no register");
		// a byte in the register's low byte where it has one (al, cl, dl, bl: IDA's 16 to 19)
		int reg = str2reg(where[k].c_str());
		a.argloc.set_reg1(size == 1 && reg < 4 ? reg + 16 : reg);
	}

	// matched with the database's by location: its registers, its stack arguments by offset. A count or a location
	// they disagree on is decided by the exe (below); a size or the result's place is refused
	std::map<std::string, const funcarg_t*> registers;
	std::vector<const funcarg_t*> stack;
	std::string disagreement;
	for (const funcarg_t& a : held)
		if (a.argloc.is_stkoff())
			stack.push_back(&a);
		else if (std::string reg = userRegister(a.argloc); !reg.empty())
			registers[reg] = &a;
		else if (disagreement.empty())
			disagreement = "the database passes an argument in " + locationText(a.argloc, a.type.get_size());
	std::sort(stack.begin(), stack.end(),
			  [](const funcarg_t* a, const funcarg_t* b) { return a->argloc.stkoff() < b->argloc.stkoff(); });
	if (disagreement.empty() && fd.size() != held.size())
		disagreement = "the database's type has " + std::to_string(held.size()) + " arguments, src's " +
					   std::to_string(fd.size());
	size_t nextStack = 0;
	for (size_t k = 0; k < fd.size() && disagreement.empty(); ++k)
	{
		int64_t index = static_cast<int64_t>(k) - static_cast<int64_t>(first);
		std::string which = index < 0 ? std::string("this") : "parameter " + std::to_string(index + 1);
		if (where[k] != "stack")
		{
			if (!registers.contains(where[k]))
				disagreement = which + ": src's in " + where[k] + ", the database passes nothing there";
			continue;
		}
		if (nextStack == stack.size())
		{
			disagreement = which + ": src's on the stack, the database has " + std::to_string(stack.size()) +
						   " stack arguments";
			continue;
		}
		const funcarg_t& own = *stack[nextStack++];
		if (stackSize(fd[k].type) != stackSize(own.type))
			return refuse(which + ": src's " + std::to_string(stackSize(fd[k].type)) + " stack bytes, the database's " +
						  std::to_string(stackSize(own.type)));
	}
	// the result where its type's standard return puts it (the seam's adapter's place), the database's place
	if (!held.rettype.is_void() && !fd.rettype.is_void())
	{
		auto place = [](const func_type_data_t& f) {
			std::optional<std::string> user = userResult(f);
			return user ? *user : locationText(f.retloc, f.rettype.get_size());
		};
		if (place(held) != place(fd))
			return refuse("the database's result in " + place(held) + ", src's in " + place(fd));
	}
	if (disagreement.empty())
	{
		std::optional<tinfo_t> type = usercall(std::move(fd), cc == CM_CC_SPECIALP);
		if (!type)
			return refuse("IDA rejects src's usercall type");
		return type;
	}

	// the exe decides (user, 2026-11-11): src's form where the code's facts (registers read before written, the
	// registers it takes, stack slots read, pops) fit it and contradict the database's; a `__userpurge` where the
	// code pops, else the database's convention where the pops are unknown
	bool purge = cc == CM_CC_SPECIALP;
	try
	{
		if (std::optional<uint32_t> pops = m_facts.pops(static_cast<uint32_t>(address)))
			purge = *pops != 0;
	}
	catch (const exefacts::Raised&)
	{
		// the decode raises: usercallMisfit refuses both forms
	}
	std::optional<tinfo_t> type = usercall(std::move(fd), purge);
	if (!type)
		return refuse(disagreement + "; IDA rejects src's usercall type");
	std::string srcMisfit = usercallMisfit(m_facts, address, *type);
	std::string heldMisfit = usercallMisfit(m_facts, address, stored);
	if (srcMisfit.empty() && heldMisfit.empty())
		return refuse(disagreement + "; the exe's code fits both");
	if (!srcMisfit.empty() && heldMisfit.empty())
		return refuse(disagreement + "; the exe's code fits only the database's (src's: " + srcMisfit + ")");
	if (!srcMisfit.empty())
		return refuse(disagreement + "; the exe's code fits neither (src's: " + srcMisfit + "; the database's: " +
					  heldMisfit + ")");
	why = disagreement + "; the exe's code fits src's, not the database's: " + heldMisfit;
	return type;
}

std::optional<tinfo_t> SrcPush::srcPrototype(ea_t address)
{
	if (auto fold = m_folds.find(address); fold != m_folds.end())
		return fold->second.type;
	if (usercallDeclared(address))
	{
		std::string why;
		if (std::optional<tinfo_t> type = usercallPrototype(address, why))
			return type;
		tinfo_t type;
		return get_tinfo(&type, address) ? std::optional<tinfo_t>(type) : std::nullopt;
	}
	if (const SrcModel::Function* f = declared(address))
		return prototype(*f, address);
	return std::nullopt;
}

void SrcPush::findFolds()
{
	m_folds.clear();
	for (const auto& [address, refs] : m_model.refs())
	{
		if (address >= kGameEnd)
			break;
		// A C++ structor src registers and the Construct / Destruct forwarder chooseRef takes for it are one function,
		// the forwarder's; a forwarder names the address only as chooseRef counts it (its function registers nowhere
		// else; at a structor, its class's Construct / Destruct).
		RefChoice choice = chooseRef(m_model, address);
		const SrcModel::Function* owner = declared(address);
		std::map<std::string, const SrcModel::Function*> members;
		bool copy = false;
		for (const SrcModel::Ref& r : refs)
		{
			if (placeholder(r))
				continue;
			copy |= r.kind == "fold";
			const SrcModel::Function* member = nullptr;
			if (r.kind == "constructor" || r.kind == "destructor")
				member = owner && choice.ref && choice.ref->kind == "forwarder" ? owner : m_model.function(r.function);
			else if (r.kind == "body" || r.kind == "address" || r.kind == "fold")
				member = m_model.function(r.function);
			else if (r.kind == "forwarder" && choice.names.count(r.target))
			{
				const std::vector<uint64_t>& registered = m_model.bodyAddresses(r);
				if (std::all_of(registered.begin(), registered.end(), [&](uint64_t a) { return a == address; }))
					member = m_model.function(r.function);
			}
			if (member)
				members.emplace(member->mangled, member);
		}
		if (!copy && members.size() < 2)
			continue;
		if (!functionStart(address))
		{
			log("fold-not-a-function", hex(address), std::to_string(members.size()) + " members");
			continue;
		}
		Fold& fold = m_folds[address];
		for (const auto& [mangled, member] : members)
			fold.members.push_back(member);
	}

	for (auto& [address, fold] : m_folds)
	{
		// each member's prototype; one the converter cannot build agrees on nothing. The neutral prototype starts
		// from the declared function's (the registered body's, one of the members), else the first member's the exe's
		// code does not contradict (a free wrapper of a thiscall, folded with the members, is no start), else the
		// first member's.
		const SrcModel::Function* owner = declared(address);
		std::vector<func_type_data_t> types;
		std::optional<size_t> declaredAt, agreeing;
		bool unseen = false;
		for (const SrcModel::Function* member : fold.members)
		{
			std::optional<tinfo_t> t = prototype(*member, address);
			func_type_data_t fd;
			if (!t || !t->get_func_details(&fd))
			{
				unseen = true;
				continue;
			}
			if (member == owner)
				declaredAt = types.size();
			if (!agreeing && !contradicted(m_facts, address, *t))
				agreeing = types.size();
			types.push_back(std::move(fd));
		}
		if (types.empty())
			continue;
		func_type_data_t fd = types[declaredAt ? *declaredAt : agreeing ? *agreeing : 0];
		// a type stays where every member has it: an untyped member, or one of another arity, agrees on nothing
		bool comparable = !unseen && std::all_of(types.begin(), types.end(), [&](const func_type_data_t& m) {
			return m.size() == fd.size();
		});
		auto agreed = [&](const tinfo_t& type, auto&& of) {
			return comparable && std::all_of(types.begin(), types.end(),
											 [&](const func_type_data_t& m) { return sameType(of(m), type); });
		};
		bool keepReturn = agreed(fd.rettype, [](const func_type_data_t& m) { return m.rettype; });
		// a hidden result pointer goes with the return type
		for (size_t k = 0; k < fd.size(); ++k)
			if (fd[k].flags & FAI_RETPTR)
				keepReturn &= agreed(fd[k].type, [k](const func_type_data_t& m) { return m[k].type; });
		if (!keepReturn)
			fd.rettype = neutral(fd.rettype);
		bool thiscall = (fd.get_cc() & CM_CC_MASK) == CM_CC_THISCALL;
		for (size_t k = 0; k < fd.size(); ++k)
		{
			funcarg_t& argument = fd[k];
			bool retptr = argument.flags & FAI_RETPTR;
			// `this` is class-neutral: every member's class shares the code
			bool self = k == 0 && !retptr && (thiscall || argument.name == "this");
			bool keep = !self && (retptr ? keepReturn
										 : agreed(argument.type, [k](const func_type_data_t& m) { return m[k].type; }));
			if (keep)
				continue;
			argument.type = neutral(argument.type);
			// a hidden result pointer is a plain `void *` argument then, the one the function returns
			if (retptr)
				argument.flags &= ~(FAI_HIDDEN | FAI_RETPTR | FAI_STRUCT);
		}
		tinfo_t type;
		if (!type.create_func(fd))
			continue;
		// every member contradicted by the exe's code (free wrappers of a thiscall, a member dropping a parameter it
		// passes as a literal): the neutral prototype is this push's own, so the exe's convention and stack bytes win.
		// A free wrapper's first pointer parameter is the object it passes in ecx, where the code reads ecx (not a hidden
		// result pointer, which comes first: such a fold is left to exePrototype as it is).
		if (!agreeing && contradicted(m_facts, address, type))
		{
			tinfo_t over = type;
			func_type_data_t built;
			if (type.get_func_details(&built) && (built.get_cc() & CM_CC_MASK) != CM_CC_THISCALL && !built.empty() &&
				built[0].type.is_ptr() && !(built[0].flags & FAI_RETPTR) &&
				unpassedRegisters(m_facts, address, type).find("ecx") != std::string::npos)
			{
				func_type_data_t member;
				member.rettype = built.rettype;
				member.set_cc(CM_CC_THISCALL);
				for (const funcarg_t& a : built)
				{
					funcarg_t copy;
					copy.type = a.type;
					copy.name = a.name;
					member.push_back(copy);
				}
				if (tinfo_t t; t.create_func(member))
					over = t;
			}
			std::optional<tinfo_t> exe = exePrototype(m_facts, address, over);
			log("fold-exe-prototype", hex(address),
				text(type) + " -> " + (exe ? text(*exe) : std::string("none: kept")));
			if (exe)
				type = *exe;
		}
		fold.type = type;
	}
}

void SrcPush::writeLogs(const std::filesystem::path& dir) const
{
	std::filesystem::create_directories(dir);
	std::vector<std::string> rows{"category\tkey\tdetail"};
	for (const std::vector<std::string>& r : m_log)
		rows.push_back(row({r[0], r[1], r[2]}));
	writeRows(dir / (m_command + "-log.tsv"), rows);
}

void SrcPush::functionState(ea_t function, unsigned kinds, std::vector<std::string>& rows)
{
	std::string key = hex(function);
	func_t* f = get_func(function);
	udcall_map_t calls;
	if ((kinds & kUdcalls) && restore_user_defined_calls(&calls, function))
		for (const auto& [call, udc] : calls)
			rows.push_back(row({"udcall", key, hex(call), udc.name.c_str(), typeText(udc.tif)}));
	lvar_uservec_t lvars;
	if ((kinds & kLvarSettings) && restore_user_lvar_settings(&lvars, function) && !lvars.empty())
	{
		for (const lvar_saved_info_t& v : lvars.lvvec)
			rows.push_back(row({"lvar", key, v.ll.location.dstr(), hex(v.ll.defea), std::to_string(v.flags),
								std::to_string(v.size), v.name.c_str(), typeText(v.type), v.cmt.c_str()}));
		// dstr's text is a shared buffer: each is copied before the next call
		std::string maps;
		for (const auto& [from, to] : lvars.lmaps)
		{
			maps += (maps.empty() ? "" : "; ") + std::string(from.dstr());
			maps += std::string(" -> ") + to.dstr();
		}
		rows.push_back(row({"lvar", key, "maps " + maps, "stkoff_delta " + std::to_string(lvars.stkoff_delta),
							"ulv_flags " + std::to_string(lvars.ulv_flags)}));
	}
	for (ea_t item : itemHeads(f))
	{
		if (kinds & kOperandTypes)
			for (int n = 0; n < UA_MAXOP; ++n)
				if (tinfo_t type; get_op_tinfo(&type, item, n))
					rows.push_back(row({"optype", hex(item), std::to_string(n), typeText(type)}));
		// a point sits at the end of the instruction whose sp change it records
		ea_t end = get_item_end(item);
		if ((kinds & (kUserPoints | kAllPoints)) && is_usersp(end))
			rows.push_back(row({"stkpnt", hex(end), std::to_string(get_sp_delta(f, end))}));
		else if (sval_t delta = (kinds & kAllPoints) ? get_sp_delta(f, end) : 0)
			rows.push_back(row({"stkpnt", hex(end), std::to_string(delta), "auto"}));
	}
}

// One row per named local type (its definition), game function (type, name, `outlined` where IDA marks it outlined
// code), address src declares data at (type, name, the size of IDA's item there), and each game function's
// functionState rows but IDA's own stack points.
void SrcPush::writeState(const std::filesystem::path& dir) const
{
	std::filesystem::create_directories(dir);
	std::vector<std::string> rows;
	til_t* til = get_idati();
	for (uint32 ordinal = 1, limit = get_ordinal_limit(til); ordinal < limit; ++ordinal)
	{
		const char* name = get_numbered_type_name(til, ordinal);
		tinfo_t type;
		if (!name || !*name || !type.get_numbered_type(til, ordinal))
			continue;
		qstring definition;
		type.print(&definition, name, PRTYPE_DEF | PRTYPE_TYPE | PRTYPE_1LINE | PRTYPE_SEMI);
		size_t size = type.get_size();
		rows.push_back(row({"type", name, definition.c_str(), size == BADSIZE ? "-" : std::to_string(size)}));
	}
	for (ea_t function : gameFunctions())
	{
		func_t* f = get_func(function);
		rows.push_back(row({"function", hex(function), addressType(function), get_name(function).c_str(),
							(f->flags & FUNC_OUTLINE) ? "outlined" : ""}));
		functionState(function, kUdcalls | kLvarSettings | kOperandTypes | kUserPoints, rows);
	}
	for (const auto& [address, refs] : m_model.refs())
		if (std::any_of(refs.begin(), refs.end(),
						[](const SrcModel::Ref& r) { return r.kind == "global" || r.kind == "static_member"; }))
			rows.push_back(row({"data", hex(address), addressType(address), get_name(address).c_str(),
								std::to_string(get_item_size(address))}));
	std::sort(rows.begin(), rows.end());
	writeRows(dir / "state.tsv", rows);
}

}  // namespace idakit
