// The steps that fix what Hex-Rays reads wrongly from a function, each a fact about the exe written into the function's
// stores, in rounds of a decompile followed by the steps until a decompile leaves nothing to change (rebuild): the
// local steps (merged arrays, frame locals, passed objects, constructed locals, value locals), then the call-site
// steps (vtable calls, indirect calls, fold owners; the variadic calls before the rounds). Every emit and decompile
// session runs them, never saved; idakit locals (locals) saves the lvar settings the local steps give:
//
//   merged arrays       an array local Hex-Rays merged from neighbouring slots, only indexed by constants and read
//                       through a part helper, narrowed to its element type, so the other slots get locals of their own
//   frame locals        a local the function passes as `&local` to a `T *` parameter, where the code takes the address
//                       of another frame object inside sizeof(T), gets IDA's frame variable's type at its offset:
//                       Hex-Rays would type it T and merge the frame objects behind it into it. Only where the callee's
//                       type is no evidence (its body dereferences nothing, or src does not declare it) and the callee
//                       is no constructor (constructed locals own its object); the callee keeps its type
//   passed objects      a local passed as `&local` (or an array) that Hex-Rays typed from its first access while the
//                       exe's object spans more, so that Hex-Rays merges the object's members into it: the largest
//                       struct parameter it is passed to, where the callee's type is evidence (as for frame locals, or
//                       the call is indirect) and the frame's locals up to its end hold no struct local and no local
//                       whose address the code takes; for a local passed to no struct parameter and holding no
//                       pointer, an array over the assigned-only, pointer-free locals behind it, as far as the exe
//                       states the object's extent (the call's constant extent, or locals stored only on the way to
//                       the call), and over the never-written ones the callee fills, within the call's constant
//                       extent. A constructor's object is constructed locals'
//   constructed locals  a stack local whose address a constructor call takes (src registers it as X's constructor, or
//                       the PDB names it X::X; never a fold), or into which the code stores an exe vtable of X (an
//                       inlined constructor's vptr store, at the subobject's offset for a secondary table), is an X:
//                       the lvar gets X's type where X's IDA struct is at src's size, Hex-Rays takes it and it fits in
//                       the frame. Where Hex-Rays merged other objects into the slot (a constructor or vtable of an
//                       unrelated class on it, or a use some path reaches before any constructor call or vtable
//                       store, X's destructor calls and stores into its members aside), the lvar is split
//                       (LVINF_SPLIT) at each constructor call or primary vtable store that starts an object, typed
//                       there, and the split undone where Hex-Rays merges them again (a split the database held, the
//                       debug build's, is left as it is); a base's constructor or vtable on the slot is the same
//                       object (the most-derived class wins). A call building its record result in a slot Hex-Rays
//                       has as another type types a local nothing else names or holds (no split)
//   value locals        a local Hex-Rays typed from one access while every occurrence takes it as another type (a
//                       store the stored value's, a read what takes it, its address the object's): typed as the one
//                       asked-for type that meets every occurrence without a reinterpreting cast
//   reused arguments    a stack argument's slot the exe stores a value of another type in once the argument is dead
//                       (a read's count, a string pushed by reference, a call's hidden result): the lvar is split at
//                       the first such store and typed as the value (the piece at its own start where Hex-Rays starts
//                       it elsewhere), so that Hex-Rays reads the other value as a variable of its own
//   vtable calls        a virtual call's operand type: the vtbl member's type where every exe implementation of the
//                       slot pops the bytes that type passes (else Hex-Rays guesses the arguments from the pushes)
//   indirect calls      any other indirect call's operand type, where Hex-Rays passes more stack bytes than the exe
//                       pushes there (the stack-slot model): a type passing the exe's bytes
//   fold owners         a call of a fold typed as its owner's src function (UnfoldFoldOwners.cpp)

#include "idakit/Unfold.hpp"

#include "IdaSupport.hpp"

#include "idakit/Asm.hpp"

#include <Facts.hpp>

#include <algorithm>
#include <functional>
#include <regex>

namespace idakit
{

namespace
{

// the rounds of steps of a function's rebuild, at most
constexpr int kRounds = 4;

// every expression of the ctree, in visiting order
std::vector<const cexpr_t*> expressions(cfunc_t& cfunc)
{
	struct Visitor : ctree_visitor_t
	{
		std::vector<const cexpr_t*> all;

		Visitor() : ctree_visitor_t(CV_FAST) {}

		int idaapi visit_expr(cexpr_t* e) override
		{
			all.push_back(e);
			return 0;
		}
	} visitor;
	visitor.apply_to(&cfunc.body, nullptr);
	return visitor.all;
}

// A body that dereferences nothing (a folded empty function: any class fits its parameters): at most 32 bytes, no
// call, no memory operand but frame slots
bool dereferencesNothing(ea_t address)
{
	func_t* function = get_func(address);
	if (function == nullptr || function->end_ea - function->start_ea > 32)
		return false;
	static const std::regex frameSlot(R"([^\[]*\[e[bs]p\b.*)");
	for (ea_t ea : codeItems(function))
	{
		Insn insn(ea);
		if (insn.mnem() == "call")
			return false;
		for (int n : {0, 1})
		{
			int kind = insn.opType(n);
			if (kind == o_mem || ((kind == o_displ || kind == o_phrase) && !std::regex_match(insn.opText(n), frameSlot)))
				return false;
		}
	}
	return true;
}

// the class and offset of the vtable slot a call target reads (`this->vfptr->f`), else nullopt
std::optional<std::pair<std::string, int64_t>> vtableSlot(const cexpr_t* e)
{
	int64_t offset = 0;
	while (e->op == cot_memref || e->op == cot_memptr)
	{
		qstring name;
		if (e->op == cot_memptr && e->x->type.is_ptr() && e->x->type.get_pointed_object().get_type_name(&name) &&
			std::string_view(name.c_str()).ends_with("_vtbl"))
			return std::pair{std::string(name.c_str(), name.length() - 5), offset + e->m};
		offset += e->m;
		e = e->x;
	}
	return std::nullopt;
}

// the call target is a member of a function type src declares: a field read through its records, not through an
// array element Hex-Rays makes of a record too small for the offset (`__vftable[3].f` for a slot past the vtbl's end)
bool srcMember(const cexpr_t* e)
{
	if (e->op != cot_memref && e->op != cot_memptr)
		return false;
	for (; e->op == cot_memref || e->op == cot_memptr; e = e->x)
		if (e->x->op == cot_idx)
			return false;
	return true;
}

// Some path from the function's entry reaches the instruction at target without passing a call in avoid.
bool reachesAvoiding(func_t* function, const FlowChart& chart, const std::set<ea_t>& avoid, ea_t target)
{
	const std::vector<FlowChart::Block>& blocks = chart.blocks();
	std::vector<bool> seen(blocks.size());
	std::vector<size_t> pending;
	for (size_t k = 0; k < blocks.size(); ++k)
		if (blocks[k].start == function->start_ea)
			pending.push_back(k);
	while (!pending.empty())
	{
		size_t k = pending.back();
		pending.pop_back();
		if (seen[k])
			continue;
		seen[k] = true;
		const FlowChart::Block& block = blocks[k];
		auto passed = avoid.lower_bound(block.start);
		if (target >= block.start && target < block.end && (passed == avoid.end() || *passed >= target))
			return true;
		if (passed != avoid.end() && *passed < block.end)
			continue;
		for (int next : block.succs)
			pending.push_back(static_cast<size_t>(next));
	}
	return false;
}

// Some path from the instruction at from (past it) reaches the instruction at target without passing a call in avoid.
bool reachesFrom(const FlowChart& chart, ea_t from, ea_t target, const std::set<ea_t>& avoid = {})
{
	const std::vector<FlowChart::Block>& blocks = chart.blocks();
	std::vector<bool> seen(blocks.size());
	std::vector<size_t> pending;
	for (size_t k = 0; k < blocks.size(); ++k)
		if (from >= blocks[k].start && from < blocks[k].end)
		{
			auto passed = avoid.upper_bound(from);
			if (target > from && target < blocks[k].end && (passed == avoid.end() || *passed >= target))
				return true;
			if (passed != avoid.end() && *passed < blocks[k].end)
				continue;
			for (int next : blocks[k].succs)
				pending.push_back(static_cast<size_t>(next));
		}
	while (!pending.empty())
	{
		size_t k = pending.back();
		pending.pop_back();
		if (seen[k])
			continue;
		seen[k] = true;
		auto passed = avoid.lower_bound(blocks[k].start);
		if (target >= blocks[k].start && target < blocks[k].end && (passed == avoid.end() || *passed >= target))
			return true;
		if (passed != avoid.end() && *passed < blocks[k].end)
			continue;
		for (int next : blocks[k].succs)
			pending.push_back(static_cast<size_t>(next));
	}
	return false;
}

// The struct holds a member of the part's type at the offset (a member's member, a base, too).
bool holdsAt(tinfo_t type, uint64 offset, const tinfo_t& part)
{
	while (type.is_udt())
	{
		udm_t member;
		member.offset = offset * 8;
		if (type.find_udm(&member, STRMEM_OFFSET) < 0)
			return false;
		offset -= member.offset / 8;
		if (offset == 0 && sameType(member.type, part))
			return true;
		type = member.type;
	}
	return false;
}

std::optional<std::pair<int, int64_t>> lvalueSlot(const cexpr_t* e);

// The lvar an address expression points into and the byte offset there: &lvalue, an array lvar decaying to its
// address, p + n (Hex-Rays' pointer arithmetic), through casts
std::optional<std::pair<int, int64_t>> addressSlot(const cexpr_t* e)
{
	e = stripCasts(e);
	if (e->op == cot_ref)
		return lvalueSlot(e->x);
	if (e->op == cot_var && e->type.is_array())
		return std::pair{e->v.idx, int64_t{0}};
	if (e->op == cot_add && e->y->op == cot_num && e->x->type.is_ptr())
	{
		size_t element = e->x->type.get_pointed_object().get_size();
		std::optional<std::pair<int, int64_t>> slot = element == BADSIZE ? std::nullopt : addressSlot(e->x);
		if (slot)
			slot->second += static_cast<int64_t>(e->y->numval() * element);
		return slot;
	}
	return std::nullopt;
}

// The lvar an lvalue designates part of and the byte offset there: v, v[n], v.m, *address
std::optional<std::pair<int, int64_t>> lvalueSlot(const cexpr_t* e)
{
	if (e->op == cot_var)
		return std::pair{e->v.idx, int64_t{0}};
	if (e->op == cot_idx && e->y->op == cot_num && e->x->op == cot_var && e->x->type.is_array())
	{
		size_t element = e->x->type.get_array_element().get_size();
		if (element == BADSIZE)
			return std::nullopt;
		return std::pair{e->x->v.idx, static_cast<int64_t>(e->y->numval() * element)};
	}
	if (e->op == cot_memref)
	{
		std::optional<std::pair<int, int64_t>> slot = lvalueSlot(e->x);
		if (slot)
			slot->second += e->m;
		return slot;
	}
	if (e->op == cot_ptr)
		return addressSlot(e->x);
	return std::nullopt;
}

// The stack bytes a call through the function (pointer) type passes; nullopt for no function type or a C-variadic one
std::optional<int64_t> passedBytes(const tinfo_t& callee)
{
	tinfo_t type = callee.is_ptr() ? callee.get_pointed_object() : callee;
	func_type_data_t fd;
	if (!type.get_func_details(&fd) || fd.is_vararg_cc())
		return std::nullopt;
	return static_cast<int64_t>(fd.stkargs);
}

// The function (pointer) type without its last stack arguments, passing the bytes; nullopt where no cut does
std::optional<tinfo_t> withStackBytes(const tinfo_t& callee, int64_t bytes)
{
	tinfo_t type = callee.is_ptr() ? callee.get_pointed_object() : callee;
	func_type_data_t fd;
	if (!type.get_func_details(&fd))
		return std::nullopt;
	while (static_cast<int64_t>(fd.stkargs) > bytes)
	{
		size_t last = fd.size();
		while (last > 0 && !fd[last - 1].argloc.is_stkoff())
			--last;
		if (last == 0)
			return std::nullopt;
		fd.erase(fd.begin() + (last - 1));
		// create_func takes the arguments out of its input; the details read back recompute the stack bytes
		func_type_data_t cut = fd;
		if (!type.create_func(cut) || !type.get_func_details(&fd))
			return std::nullopt;
	}
	if (static_cast<int64_t>(fd.stkargs) != bytes)
		return std::nullopt;
	if (!callee.is_ptr())
		return type;
	tinfo_t pointer;
	pointer.create_ptr(type);
	return pointer;
}

// What one occurrence of a local asks of its type U (valueLocals, Unfold::askedTypes)
struct Want
{
	enum Kind
	{
		none,    // nothing
		into,    // t converts to U
		outof,   // U converts to t
		object,  // the object is a t or derives from it
		exact,   // U is the type the local has
		element  // U is the local's array type, or a record with a scalar member of t's size at offset
	} kind = exact;
	tinfo_t t;
	// a store of a whole value into the local (an assignment, `*(T *)&v = x` of its width): its instruction, and the
	// type the value is stored as (its own where it has the store's size, else the one it is converted to: a double
	// stored into a float)
	ea_t store = BADADDR;
	tinfo_t as{};
	// the local's address passed on (to a parameter, a C-variadic argument, a pointer): the receiver may touch bytes
	// past the type it takes the address as (an array's elements, a derived object's members)
	bool unbounded = false;
	// an element access's byte offset in the local
	int64_t offset = 0;
};

// Each occurrence's Want, by lvar: of the locals Hex-Rays typed (no argument, no lvar setting holding its type), or
// with userTyped of every local but the arguments
struct Occurrences : ctree_visitor_t
{
	const lvars_t& lvars;
	tinfo_t returned;
	// a callee that is a class template instance's constructor or destructor (templateStructor)
	std::function<bool(ea_t)> templateStructor;
	// a callee whose src declaration returns `void *` (untypedResult)
	std::function<bool(ea_t)> untypedResult;
	bool userTyped = false;
	std::map<int, std::vector<Want>> wants;

	Occurrences(const lvars_t& l, const tinfo_t& r, std::function<bool(ea_t)> t, std::function<bool(ea_t)> u,
				bool all = false)
		: ctree_visitor_t(CV_PARENTS), lvars(l), returned(r), templateStructor(std::move(t)),
		  untypedResult(std::move(u)), userTyped(all)
	{
	}

	const cexpr_t* parentExpr(size_t at) const
	{
		return at < parents.size() && parents[at]->is_expr() ? static_cast<const cexpr_t*>(parents[at]) : nullptr;
	}

	// the parameter type an argument of a call is passed to; nullopt past the fixed ones (a C-variadic call's)
	static std::optional<tinfo_t> parameter(const cexpr_t* call, const cexpr_t* argument, bool& blocked)
	{
		tinfo_t callee = call->x->type;
		if (callee.is_ptr())
			callee = callee.get_pointed_object();
		func_type_data_t fd;
		if (call->x == argument || !callee.get_func_details(&fd))
		{
			blocked = true;
			return std::nullopt;
		}
		for (size_t k = 0; k < call->a->size(); ++k)
			if (&(*call->a)[k] == argument)
				return k < fd.size() ? std::optional<tinfo_t>(fd[k].type) : std::nullopt;
		blocked = true;
		return std::nullopt;
	}

	// a store of the expression into the local or its object as `as` at the instruction (BADADDR: a part of it only):
	// what the value asks; a call src declares returning `void *` (an allocation, __RTDynamicCast, which IDA's library
	// type has return `char *`) nothing: its value is untyped memory, of the type it is used as; a floating value
	// stored as a floating type that type: C++ converts between floating types itself
	Want stored(const cexpr_t* value, const tinfo_t& as, ea_t at) const
	{
		value = stripCasts(value);
		const tinfo_t& kept = value->type.get_size() == as.get_size() ? value->type : as;
		if (value->op == cot_num ||
			(value->op == cot_call && value->x->op == cot_obj && untypedResult(value->x->obj_ea)))
			return Want{Want::none, {}, at, kept};
		if (value->type.is_floating() && as.is_floating())
			return Want{Want::into, as, at, as};
		return Want{Want::into, value->type, at, kept};
	}

	// the address of the local's object, the item under parents[at]
	Want address(const cexpr_t* item, size_t at) const
	{
		const cexpr_t* p = parentExpr(at);
		if (p == nullptr)
			return {};
		// the address passed on: a void pointer or an integer asks nothing
		auto object = [](const tinfo_t& pointer) {
			if (!pointer.is_ptr() || pointer.get_pointed_object().is_void())
				return Want{Want::none, {}, BADADDR, {}, true};
			return Want{Want::object, pointer.get_pointed_object(), BADADDR, {}, true};
		};
		// an element of an array local read or written at a constant index
		if (p->op == cot_idx && p->x == item && item->op == cot_var && item->type.is_array() &&
			stripCasts(p->y)->op == cot_num)
		{
			Want want{Want::element, p->type};
			want.offset = static_cast<int64_t>(stripCasts(p->y)->numval() * p->type.get_size());
			return want;
		}
		// the address cast: to a pointer, through which a whole access reads or writes the object as the pointee
		// (`*(T *)&v` a T); to an integer, which a parameter of that type takes (a C-variadic call's)
		if (p->op == cot_cast)
		{
			if (const cexpr_t* access = parentExpr(at - 1);
				p->type.is_ptr() && access != nullptr && at > 0 && access->op == cot_ptr)
			{
				const cexpr_t* store = at > 1 ? parentExpr(at - 2) : nullptr;
				const cexpr_t* var = item->op == cot_ref ? item->x : item;
				if (store != nullptr && store->op == cot_asg && store->x == access)
					return stored(store->y, access->type,
								  access->type.get_size() == static_cast<size_t>(lvars[var->v.idx].width)
									  ? store->ea
									  : BADADDR);
				return {Want::outof, p->type.get_pointed_object()};
			}
			item = p;
			p = at > 0 ? parentExpr(--at) : nullptr;
			if (p == nullptr)
				return {};
		}
		if (p->op == cot_call)
		{
			// the object of a constructor or destructor the template's instances share asks no instance
			const cexpr_t* callee = stripCasts(p->x);
			if (!p->a->empty() && stripCasts(&(*p->a)[0]) == stripCasts(item) && callee->op == cot_obj &&
				templateStructor(callee->obj_ea))
				return {Want::none, {}, BADADDR, {}, true};
			bool blocked = false;
			std::optional<tinfo_t> type = parameter(p, item, blocked);
			if (blocked)
				return {};
			return type ? object(*type) : Want{Want::none, {}, BADADDR, {}, true};
		}
		if (p->op == cot_asg && p->y == item)
			return object(p->x->type);
		return {};
	}

	// the local's value, e under parents[at]
	Want value(const cexpr_t* e, size_t at, const tinfo_t& own) const
	{
		const citem_t* parent = parents[at];
		if (!parent->is_expr())
		{
			switch (parent->op)
			{
			case cit_return: return {Want::outof, returned};
			case cit_if: case cit_while: case cit_do: case cit_for: case cit_expr: return {Want::none, {}};
			default: return {};
			}
		}
		const cexpr_t* p = static_cast<const cexpr_t*>(parent);
		switch (p->op)
		{
		case cot_cast:
			return {Want::outof, p->type};
		case cot_asg:
			return p->y == e ? Want{Want::outof, p->x->type} : stored(p->y, p->x->type, p->ea);
		case cot_call:
		{
			bool blocked = false;
			std::optional<tinfo_t> type = parameter(p, e, blocked);
			if (blocked)
				return {};
			return type ? Want{Want::outof, *type} : Want{Want::none, {}};
		}
		case cot_eq: case cot_ne:
		{
			const cexpr_t* other = stripCasts(p->x == e ? p->y : p->x);
			return other->op == cot_num ? Want{Want::none, {}} : Want{Want::outof, other->type};
		}
		case cot_memptr:
			return {Want::outof, own};
		// a member of the local's object (a lane of an `__m128` local, `v.m128_f32[0]`): a record holding the object's
		// type whole at offset 0 has it too, through its member (`v.m_quad.m128_f32[0]`)
		case cot_memref:
			return {Want::outof, own};
		case cot_lnot: case cot_land: case cot_lor:
			return {Want::none, {}};
		case cot_tern:
			return p->x == e ? Want{Want::none, {}} : Want{};
		default:
			return {};
		}
	}

	int idaapi visit_expr(cexpr_t* e) override
	{
		if (e->op != cot_var || parents.empty())
			return 0;
		const lvar_t& local = lvars[e->v.idx];
		if (local.is_arg_var() || (!userTyped && local.has_user_type()))
			return 0;
		const size_t at = parents.size() - 1;
		const cexpr_t* p = parentExpr(at);
		Want want;
		if (p != nullptr && p->op == cot_ref)
			want = at > 0 ? address(p, at - 1) : Want{};
		else if (local.type().is_array())
			want = address(e, at);
		else
			want = value(e, at, local.type());
		wants[e->v.idx].push_back(want);
		return 0;
	}
};

// IDA's sized unknowns (_BYTE, _WORD, _DWORD, _QWORD), which Hex-Rays does not take as a local's type (it keeps the
// type it had), as the unsigned integer of their size
tinfo_t plain(const tinfo_t& t)
{
	static constexpr std::pair<size_t, type_t> kSizes[] = {{1, BT_INT8}, {2, BT_INT16}, {4, BT_INT32}, {8, BT_INT64}};
	if (get_base_type(t.get_realtype()) != BT_VOID || t.is_void())
		return t;
	for (const auto& [size, base] : kSizes)
		if (t.get_size() == size)
			return tinfo_t(base | BTMT_USIGNED);
	return t;
}

}  // namespace

Unfold::Unfold(const SrcModel& model, const ExeVtables& exe, exefacts::Facts& facts, SrcTypes& types,
			   bool localsSession)
	: SrcPush(localsSession ? "locals" : "unfold", model, exe, facts, types), m_localsSession(localsSession)
{
	findFolds();
	// every function's, the CRT's (sprintf) included
	for (size_t n = 0, quantity = get_func_qty(); n < quantity; ++n)
		if (func_t* f = getn_func(n); f != nullptr)
			if (tinfo_t type; get_tinfo(&type, f->start_ea) && type.is_vararg_cc())
				m_variadic.insert(f->start_ea);
	// in name order: of two names with one class key, the first's class
	std::map<std::string, std::string> pdb;
	for (const auto& [name, functions] : model.pdbFunctions())
		pdb.emplace(name, functions.front().cls);
	for (const auto& [name, cls] : pdb)
	{
		std::string last = lastComponent(name);
		if (constructorName(name) ||
			(last.starts_with('~') && constructorName(scopeOf(name) + "::" + last.substr(1))))
			m_pdbStructors.emplace(ExeVtables::classKey(name), cls);
	}
}

void Unfold::log(const std::string& category, const std::string& key, const std::string& detail)
{
	// idakit locals puts the call sites back: their rows would describe nothing it saves
	static constexpr std::string_view kLocalRows[] = {"merged-array-",		"frame-local-",		"passed-object-",
													  "constructed-local-", "value-local-", "reused-argument-", "locals-"};
	if (m_localsSession && std::none_of(std::begin(kLocalRows), std::end(kLocalRows), [&](std::string_view prefix) {
			return category.starts_with(prefix);
		}))
		return;
	SrcPush::log(category, key, detail);
}

bool Unfold::locals(func_t* function, std::string& why)
{
	const ea_t start = function->start_ea;
	udcall_map_t calls;
	restore_user_defined_calls(&calls, start);
	lvar_uservec_t settings;
	restore_user_lvar_settings(&settings, start);
	const std::vector<ea_t> items = itemHeads(function);
	std::map<std::pair<ea_t, int>, tinfo_t> operands;
	for (ea_t item : items)
		for (int n = 0; n < UA_MAXOP; ++n)
			if (tinfo_t type; get_op_tinfo(&type, item, n))
				operands.emplace(std::pair{item, n}, type);

	cfuncptr_t cfunc = rebuild(function, why);
	if (cfunc == nullptr)
	{
		save_user_lvar_settings(start, settings);
		log("locals-decompile-failed", hex(start), why);
	}
	// the call sites as they were: the database gains the lvar settings alone
	save_user_defined_calls(start, calls);
	for (ea_t item : items)
	{
		m_operandTyped.erase(item);
		for (int n = 0; n < UA_MAXOP; ++n)
		{
			tinfo_t now;
			bool has = get_op_tinfo(&now, item, n);
			auto before = operands.find({item, n});
			if (before == operands.end())
			{
				if (has)
					del_op_tinfo(item, n);
			}
			else if (!has || text(now) != text(before->second))
				set_op_tinfo(item, n, &before->second);
		}
	}
	return cfunc != nullptr;
}

cfuncptr_t Unfold::rebuild(func_t* function, std::string& why)
{
	const ea_t start = function->start_ea;
	// the exe facts' cached decodes kept small over a long session (no reference to them is held between functions)
	m_facts.trim();
	// the variadic sites merged into the function's user-defined calls: an entry at any other call stays
	if (udcall_map_t typed = variadicSites(function); !typed.empty())
	{
		udcall_map_t calls;
		restore_user_defined_calls(&calls, start);
		for (const auto& [call, udc] : typed)
			calls[call] = udc;
		save_user_defined_calls(start, calls);
	}
	m_undone.clear();
	m_split.clear();
	// a fold's own body is its owner's, for this rebuild only
	std::optional<StoredType> folded = ownerPrototype(function);
	cfuncptr_t cfunc(nullptr);
	for (int round = 0; round <= kRounds; ++round)
	{
		cfunc = decompile(function, why);
		if (cfunc == nullptr || round == kRounds)
			break;
		// every step runs on the round's decompile; the next round sees what any of them changed
		m_kept.clear();
		bool changed = false;
		changed |= narrowMergedArrays(function, *cfunc);
		changed |= frameLocals(function, *cfunc);
		changed |= passedObjects(function, *cfunc);
		changed |= constructedLocals(function, *cfunc);
		changed |= valueLocals(function, *cfunc);
		changed |= reusedArguments(function, *cfunc);
		changed |= vtableCalls(function, *cfunc);
		changed |= indirectCalls(function, *cfunc);
		changed |= foldOwners(function, *cfunc);
		if (!changed)
			break;
	}
	// what the steps left as it is, from the last round's decompile
	for (const auto& [category, detail] : m_kept)
		log(category, hex(start), detail);
	m_kept.clear();
	// the decompile keeps the owner's type; the database holds the fold's again for the session's other functions
	if (folded)
		putBack(start, *folded);
	return cfunc;
}

cfuncptr_t Unfold::decompile(func_t* function, std::string& why)
{
	// every item whose type changes during the decompile, as it was before its first change
	struct Changes : event_listener_t
	{
		std::map<ea_t, StoredType>& before;

		explicit Changes(std::map<ea_t, StoredType>& b) : before(b) {}

		ssize_t idaapi on_event(ssize_t code, va_list va) override
		{
			if (code == idb_event::changing_ti)
			{
				ea_t ea = va_arg(va, ea_t);
				if (!before.contains(ea))
					before.emplace(ea, storedType(ea));
			}
			return 0;
		}
	} changes(m_before);

	m_before.clear();
	m_before.emplace(function->start_ea, storedType(function->start_ea));
	for (ea_t item : itemHeads(function))
	{
		xrefblk_t xb;
		for (bool ok = xb.first_from(item, XREF_NOFLOW); ok; ok = xb.next_from())
			if (!m_before.contains(xb.to))
				m_before.emplace(xb.to, storedType(xb.to));
	}
	hexrays_failure_t failure;
	cfuncptr_t cfunc(nullptr);
	hook_event_listener(HT_IDB, &changes, nullptr);
	try
	{
		cfunc = decompile_func(function, &failure, DECOMP_NO_WAIT | DECOMP_NO_CACHE);
		if (cfunc == nullptr)
			why = hex(failure.errea) + ": " + failure.desc().c_str();
	}
	catch (const vd_failure_t& thrown)
	{
		why = hex(thrown.hf.errea) + ": " + thrown.hf.desc().c_str();
	}
	unhook_event_listener(HT_IDB, &changes);
	restoreTypes(function);
	return cfunc;
}

Unfold::StoredType Unfold::storedType(ea_t address)
{
	StoredType out;
	if (tinfo_t type; get_tinfo(&type, address))
		out.type = type;
	out.source = get_aflags(address) & AFL_TYPE_GUESSED;
	return out;
}

void Unfold::restoreTypes(func_t* function)
{
	// by serialized type: equals_to ignores argument names, which Hex-Rays stores too
	auto serialized = [](const std::optional<tinfo_t>& type) {
		qtype bytes, fields;
		if (type)
			type->serialize(&bytes, &fields);
		return std::pair{bytes, fields};
	};
	for (const auto& [address, before] : m_before)
	{
		StoredType now = storedType(address);
		if (now.source == before.source && now.type.has_value() == before.type.has_value() &&
			serialized(now.type) == serialized(before.type))
			continue;
		putBack(address, before);
		count("guess-restored");
		log("guess-restored", hex(address),
			"decompiling " + hex(function->start_ea) + ": " + (now.type ? text(*now.type) : "none") + " back to " +
				(before.type ? text(*before.type) : "none"));
	}
	m_before.clear();
}

void Unfold::putBack(ea_t address, const StoredType& stored)
{
	if (stored.type)
		set_tinfo(address, &*stored.type);
	else
		del_tinfo(address);
	upd_abits(address, AFL_TYPE_GUESSED, stored.source);
}

// Hex-Rays merges neighbouring stack slots into one array local (three locals as `float v107[3]`, the middle one a
// pointer read as `LODWORD(v107[1])`). The array locals a part helper reads or writes, that the code only indexes
// with constants and whose address (or an element's) it never takes, are narrowed to their element type: Hex-Rays
// then gives the other slots locals of their own.
bool Unfold::narrowMergedArrays(func_t* function, cfunc_t& cfunc)
{
	const lvars_t& lvars = *cfunc.get_lvars();
	auto arrayLocal = [&](const cexpr_t* e) {
		return e->op == cot_var && lvars[e->v.idx].type().is_array() ? e->v.idx : -1;
	};
	std::set<int> helped, kept;
	std::map<int, int> uses, indexed;
	for (const cexpr_t* e : expressions(cfunc))
	{
		if (int k = arrayLocal(e); k >= 0)
			++uses[k];
		else if (e->op == cot_idx && (k = arrayLocal(e->x)) >= 0 && e->y->op == cot_num)
			++indexed[k];
		else if (e->op == cot_ref && e->x->op == cot_idx && (k = arrayLocal(e->x->x)) >= 0)
			kept.insert(k);
		else if (e->op == cot_call && e->x->op == cot_helper && isPartHelper(e->x->helper) && e->a->size() == 1)
		{
			const cexpr_t* part = stripCasts(&(*e->a)[0]);
			if (part->op == cot_idx && (k = arrayLocal(part->x)) >= 0)
				helped.insert(k);
		}
	}
	bool changed = false;
	for (int k : helped)
	{
		// a bare use (the array passed as a whole) or a variable index (a buffer) keeps the array
		if (kept.count(k) || uses[k] != indexed[k])
			continue;
		lvar_saved_info_t info;
		info.ll = lvars[k];
		info.type = lvars[k].type().get_ptrarr_object();
		if (!modify_user_lvar_info(function->start_ea, MLI_TYPE, info))
			continue;
		changed = true;
		log("merged-array-narrowed", hex(function->start_ea),
			std::string(lvars[k].name.c_str()) + " (" + text(lvars[k].type()) + ") as " + text(info.type));
	}
	return changed;
}

// Hex-Rays types a stack local after the pointee of the parameter it is passed to, and a struct larger than the local
// merges the frame objects behind it into one lvar (PlayerCharacter::Update 0x93E860: the folded empty
// nullsub_483710, typed `BaseProcess *this` from src, made 48-byte BaseProcess lvars of a BSSoundHandle and of the EH
// record; empty_constructor 0x6815C0's IDA-guessed `NiNode *` did the same with 172 bytes). Where the code takes the
// address of another frame object inside the pointee, the local gets, in the caller's lvar settings, the type of
// IDA's frame variable at its offset (which ends at the next offset the code accesses), so the callee keeps its type.
// A typed local can expose the next one (a callee's lvars show only once the slots around them are free): the next
// round sees it.
bool Unfold::frameLocals(func_t* function, cfunc_t& cfunc)
{
	std::vector<ea_t> code = codeItems(function);
	// the frame offsets (entry esp based, as IDA's spd) whose address the code takes: `lea r, [ebp+d]` / `[esp+d]`
	std::optional<int64_t> ebpOffset;
	if (function->flags & FUNC_FRAME)
		ebpOffset = -4;
	else
		for (size_t k = 0; k < code.size() && k < 24 && !ebpOffset; ++k)
		{
			Insn insn(code[k]);
			if (insn.mnem() == "mov" && insn.opText(0) == "ebp" && insn.opText(1) == "esp")
				ebpOffset = spDelta(code[k]);
		}
	auto based = [](const std::string& operand, const char* reg) {
		size_t at = operand.find(std::string("[") + reg);
		return at != std::string::npos && at + 4 < operand.size() &&
			   (operand[at + 4] == '+' || operand[at + 4] == '-' || operand[at + 4] == ']');
	};
	std::set<int64_t> taken;
	for (ea_t ea : code)
	{
		Insn insn(ea);
		if (insn.mnem() != "lea" || (insn.opType(1) != o_displ && insn.opType(1) != o_phrase))
			continue;
		std::string operand = withoutBlanks(insn.opText(1));
		if (operand.find('*') != std::string::npos)
			continue;
		int64_t disp = insn.opType(1) == o_displ ? signed32(insn.opValue(1)) : 0;
		if (based(operand, "ebp") && ebpOffset)
			taken.insert(*ebpOffset + disp);
		else if (based(operand, "esp"))
			taken.insert(spDelta(ea).value_or(0) + disp);
	}

	struct Candidate
	{
		ea_t callee;
		size_t arg;
		int lvar;
		tinfo_t type;  // IDA's frame variable's
		std::string why;
	};
	struct Calls : ctree_visitor_t
	{
		cfunc_t& cfunc;
		func_t* function;
		const std::set<int64_t>& taken;
		tinfo_t frame;
		std::vector<Candidate> found;

		Calls(cfunc_t& c, func_t* f, const std::set<int64_t>& t)
			: ctree_visitor_t(CV_FAST), cfunc(c), function(f), taken(t)
		{
			get_func_frame(&frame, f);
		}

		int idaapi visit_expr(cexpr_t* e) override
		{
			if (e->op != cot_call || e->x->op != cot_obj)
				return 0;
			ea_t callee = e->x->obj_ea;
			tinfo_t type;
			func_type_data_t fd;
			if (!get_tinfo(&type, callee) || !type.get_func_details(&fd))
				return 0;
			for (size_t k = 0; k < e->a->size() && k < fd.size(); ++k)
			{
				const cexpr_t* a = stripCasts(&(*e->a)[k]);
				if (a->op != cot_ref || a->x->op != cot_var || !fd[k].type.is_ptr())
					continue;
				const lvar_t& local = cfunc.get_lvars()->at(a->x->v.idx);
				tinfo_t pointee = fd[k].type.get_pointed_object();
				if (!local.is_stk_var() || !pointee.is_udt())
					continue;
				sval_t offset = cfunc.mba->stkoff_vd2ida(local.get_stkoff());
				int64_t start = offset - function->frsize - function->frregs;
				int64_t size = static_cast<int64_t>(pointee.get_size());
				auto inside = taken.upper_bound(start);
				if (inside == taken.end() || *inside >= start + size)
					continue;
				udm_t variable;
				variable.offset = uint64(offset) * 8;
				if (frame.find_udm(&variable, STRMEM_OFFSET) < 0 || variable.offset != uint64(offset) * 8 ||
					variable.type.get_size() >= local.type().get_size())
					continue;
				found.push_back({callee, k, a->x->v.idx, variable.type,
								 text(pointee) + " (" + std::to_string(size) + " bytes) over " + std::to_string(*inside)});
			}
			return 0;
		}
	};

	const std::string key = hex(function->start_ea);
	Calls calls(cfunc, function, taken);
	calls.apply_to(&cfunc.body, nullptr);
	std::map<int, const Candidate*> typed;
	std::vector<std::string> kept;
	for (const Candidate& c : calls.found)
	{
		std::string callee = hex(c.callee) + " " + get_name(c.callee).c_str() + " parameter " + std::to_string(c.arg);
		// a constructor's object (its first argument) is constructedLocals' (its class is evidence), whoever declares
		// the constructor; its other arguments are any callee's
		if (Structor structor = c.arg == 0 ? structorOf(c.callee) : Structor{};
			!structor.cls.empty() && !structor.destructor)
			kept.push_back(callee + ": constructs " + structor.cls + "; " + c.why);
		else if (typeIsEvidence(c.callee))
			kept.push_back(callee + ": src declares the callee; " + c.why);
		else
			typed.try_emplace(c.lvar, &c);
	}
	bool changed = false;
	for (const auto& [index, c] : typed)
	{
		const lvar_t& local = cfunc.get_lvars()->at(index);
		lvar_saved_info_t info;
		info.ll = local;
		info.type = c->type;
		if (!modify_user_lvar_info(function->start_ea, MLI_TYPE, info))
		{
			m_kept.emplace_back("frame-local-failed", local.name.c_str());
			continue;
		}
		changed = true;
		log("frame-local-typed", key,
			std::string(local.name.c_str()) + " (" + text(local.type()) + ") as " + text(info.type) + ": passed to " +
				hex(c->callee) + " " + get_name(c->callee).c_str() + " parameter " + std::to_string(c->arg) + ", " +
				c->why + (dereferencesNothing(c->callee) ? "; empty body" : "; IDA-guessed type"));
	}
	for (const std::string& k : kept)
		m_kept.emplace_back("frame-local-kept", k);
	return changed;
}

bool Unfold::typeIsEvidence(ea_t callee)
{
	return srcDeclares(callee) && !dereferencesNothing(callee);
}

// The exe builds an object in its frame and passes its address to a callee, but Hex-Rays types the object's slot from
// its first access (an `__m128` from a movaps, `char[4]` from a `char *` parameter) and gives its other members locals
// of their own, which src's frame lays out apart from it: the callee reads other bytes there. The passed local (no
// struct already, its address the call's argument, `&v` or an array, in the frame's locals) gets the object's type,
// so that Hex-Rays merges the members into it. Each local's calls are weighed together, whatever their order:
// - a constructor's object (the first argument of a constructor or a constructor fold) is constructedLocals';
// - a local any call passes to a struct parameter gets the largest such struct T (the first of equal ones) where the
//   callee's type is evidence (typeIsEvidence, or the call is indirect, typed through its vtbl member), T is larger
//   than the local and ends inside the frame's locals, and the frame from the local to T's end holds no struct local
//   and no local whose address the code takes; else it is left as it is;
// - else, for a local that holds no pointer (a pointer is the value passed by its address, an out parameter), an
//   array over the locals right behind it that the code only assigns (never reads, never takes the address of) and
//   that hold no pointer, as far as the exe states the object's extent: up to the call's largest constant argument,
//   or over locals assigned only on the way to the call (the call follows each store: a dead store after the call
//   is not):
//   bytes the exe writes for the callee alone (a NiPoint3 as `char apData[4]` and two floats written for
//   `SaveDataEndian(apData, 12)`); and, up to the call's largest constant argument only, the locals right behind it
//   that the code never writes (no assignment, increment or part store, never its address taken) and that hold no
//   pointer: bytes the callee fills for the code (`GetChunkData(&apData, 8)`, whose second float Hex-Rays reads as a
//   local nothing writes). The largest such array of the local's calls wins; its element type is the local's own
//   where every part's offset and size are whole elements, else bytes.
bool Unfold::passedObjects(func_t* function, cfunc_t& cfunc)
{
	lvars_t& lvars = *cfunc.get_lvars();
	std::vector<const cexpr_t*> all = expressions(cfunc);
	// an array local (Hex-Rays types its uses as the pointer it decays to)
	auto arrayVar = [&](const cexpr_t* e) { return e->op == cot_var && lvars[e->v.idx].type().is_array(); };
	// the lvars read (any occurrence but a plain assignment's left side) and those whose address the code takes; the
	// instructions each lvar is assigned at
	std::set<const cexpr_t*> assigned;
	std::map<int, std::vector<ea_t>> stores;
	for (const cexpr_t* e : all)
		if (e->op == cot_asg && e->x->op == cot_var)
		{
			assigned.insert(e->x);
			stores[e->x->v.idx].push_back(e->ea);
		}
	std::set<int> read;
	for (const cexpr_t* e : all)
		if (e->op == cot_var && !assigned.contains(e))
			read.insert(e->v.idx);
	// the lvars the code writes: an assignment or increment of the lvar or a part of it (`v.m`, `v[2]` of an array
	// local, `LOBYTE(v)`, through casts)
	std::set<int> written;
	for (const cexpr_t* e : all)
		if (is_assignment(e->op) || is_prepost(e->op))
		{
			const cexpr_t* x = e->x;
			while (x->op == cot_cast || x->op == cot_memref || (x->op == cot_idx && x->x->type.is_array()) ||
				   (x->op == cot_call && x->x->op == cot_helper && x->a->size() == 1))
				x = x->op == cot_call ? &(*x->a)[0] : x->x;
			if (x->op == cot_var)
				written.insert(x->v.idx);
		}
	// a local the code assigns only on the way to the call: the call follows every store (a dead store after the
	// call, which never reaches it, is no part of what the call is passed; in a loop, a store after one call is on the
	// way to the next)
	FlowChart chart(function);
	auto builtFor = [&](int w, ea_t call) {
		auto found = stores.find(w);
		return found != stores.end() && std::all_of(found->second.begin(), found->second.end(), [&](ea_t at) {
				   return at != BADADDR && reachesFrom(chart, at, call);
			   });
	};
	const std::set<int> taken = addressTakenLocals(cfunc);
	// the stack locals by frame offset
	std::map<sval_t, std::vector<int>> byOffset;
	for (size_t k = 0; k < lvars.size(); ++k)
		if (lvars[k].is_stk_var() && !lvars[k].is_arg_var())
			byOffset[cfunc.mba->stkoff_vd2ida(lvars[k].get_stkoff())].push_back(static_cast<int>(k));
	auto offsetOf = [&](int k) { return cfunc.mba->stkoff_vd2ida(lvars[k].get_stkoff()); };
	// in the frame's locals: below them (a negative offset) is the outgoing argument area, whose by-value arguments
	// are no locals, above them the saved registers, the return address and the arguments
	auto inFrame = [&](sval_t start, sval_t end) { return start >= 0 && end <= static_cast<sval_t>(function->frsize); };

	struct Candidate
	{
		tinfo_t type;
		sval_t size;
		std::string why;
	};
	// per local: its best struct (present, possibly empty, for every local passed to a struct parameter), its best
	// array, and whether a constructor builds it
	std::map<int, std::optional<Candidate>> structs;
	std::map<int, Candidate> arrays;
	std::set<int> constructed;
	for (const cexpr_t* e : all)
	{
		if (e->op != cot_call)
			continue;
		tinfo_t type = e->x->type;
		if (type.is_ptr())
			type = type.get_pointed_object();
		func_type_data_t fd;
		if (!type.get_func_details(&fd))
			continue;
		const bool direct = e->x->op == cot_obj;
		const bool evidence = !direct || typeIsEvidence(e->x->obj_ea);
		// the extent the call states: its largest constant argument that fits the frame's locals
		std::optional<uint64_t> extent;
		for (const carg_t& a : *e->a)
			if (const cexpr_t* n = stripCasts(&a); n->op == cot_num && n->numval() <= function->frsize)
				extent = std::max<uint64_t>(extent.value_or(0), n->numval());
		for (size_t a = 0; a < e->a->size() && a < fd.size(); ++a)
		{
			const cexpr_t* arg = stripCasts(&(*e->a)[a]);
			int v = arg->op == cot_ref && arg->x->op == cot_var ? arg->x->v.idx : arrayVar(arg) ? arg->v.idx : -1;
			if (v < 0 || !lvars[v].is_stk_var() || lvars[v].is_arg_var() || lvars[v].type().is_struct())
				continue;
			if (a == 0 && direct)
				if (Structor s = structorOf(e->x->obj_ea); !s.destructor && (!s.cls.empty() || !s.fold.empty()))
				{
					constructed.insert(v);
					continue;
				}
			const sval_t start = offsetOf(v);
			tinfo_t pointee = fd[a].type.is_ptr() ? fd[a].type.get_pointed_object() : tinfo_t();
			if (pointee.is_struct())
			{
				std::optional<Candidate>& best = structs[v];
				const sval_t size = static_cast<sval_t>(pointee.get_size());
				if (!evidence || size <= static_cast<sval_t>(lvars[v].width) || !inFrame(start, start + size) ||
					(best && best->size >= size))
					continue;
				bool free = true;
				for (auto at = byOffset.upper_bound(start); at != byOffset.end() && at->first < start + size; ++at)
					for (int w : at->second)
						free = free && !taken.contains(w) && !lvars[w].type().is_struct();
				if (free)
					best = Candidate{pointee, size,
									 "the struct parameter " + std::to_string(a) + " of the call at " + hex(e->ea)};
				continue;
			}
			// the locals right behind it that the code only assigns, contiguous up to alignment padding (every lvar
			// Hex-Rays has at an offset: MSVC reuses a slot for other objects), within an extent the exe states: the
			// call's constant extent, or stores into the part only on the way to the call (builtFor); or that the code
			// never writes, within the call's constant extent (the callee fills them)
			if (lvars[v].type().is_ptr())
				continue;
			sval_t end = start + static_cast<sval_t>(lvars[v].width);
			std::vector<int> parts{v};
			size_t filledParts = 0;
			for (auto at = byOffset.upper_bound(start); at != byOffset.end(); ++at)
			{
				if (at->first < end || at->first - end >= 4)
					break;
				bool only = true, filled = true;
				sval_t past = at->first;
				for (int w : at->second)
				{
					const bool plain = !taken.contains(w) && !lvars[w].type().is_struct() && !lvars[w].type().is_ptr();
					only = only && plain && !read.contains(w);
					filled = filled && plain && !written.contains(w);
					past = std::max(past, at->first + static_cast<sval_t>(lvars[w].width));
				}
				const bool inExtent = extent && past <= start + static_cast<sval_t>(*extent);
				const bool stated = inExtent || std::all_of(at->second.begin(), at->second.end(),
															[&](int w) { return builtFor(w, e->ea); });
				if (!(only && stated) && !(filled && inExtent))
					break;
				filledParts += only && stated ? 0 : at->second.size();
				parts.insert(parts.end(), at->second.begin(), at->second.end());
				end = past;
			}
			if (parts.size() == 1 || !inFrame(start, end))
				continue;
			if (auto found = arrays.find(v); found != arrays.end() && found->second.size >= end - start)
				continue;
			// the local's element type, where each part is a whole number of elements at an element's offset
			const tinfo_t& own = lvars[v].type();
			tinfo_t of = own.is_array() ? own.get_ptrarr_object() : own;
			const sval_t size = static_cast<sval_t>(of.get_size());
			bool whole = of.get_size() != BADSIZE && size > 0 && (end - start) % size == 0;
			for (int k : parts)
				whole = whole && (offsetOf(k) - start) % size == 0 && static_cast<sval_t>(lvars[k].width) % size == 0;
			if (!whole)
				of = tinfo_t(BTF_UCHAR);
			tinfo_t array;
			if (!array.create_array(of, static_cast<uint32_t>((end - start) / static_cast<sval_t>(of.get_size()))))
				continue;
			arrays[v] = Candidate{array, end - start,
								  std::to_string(parts.size() - 1 - filledParts) + " assigned-only and " +
									  std::to_string(filledParts) + " callee-filled locals behind it, passed to the "
																	"call at " + hex(e->ea)};
		}
	}
	std::map<int, const Candidate*> typed;
	for (const auto& [v, best] : structs)
		if (best && !constructed.contains(v))
			typed.emplace(v, &*best);
	for (const auto& [v, best] : arrays)
		if (!structs.contains(v) && !constructed.contains(v))
			typed.emplace(v, &best);
	bool changed = false;
	const std::string key = hex(function->start_ea);
	for (const auto& [index, c] : typed)
	{
		const lvar_t& local = lvars[index];
		lvar_saved_info_t info;
		info.ll = local;
		info.type = c->type;
		if (!modify_user_lvar_info(function->start_ea, MLI_TYPE, info))
		{
			m_kept.emplace_back("passed-object-failed", key + " " + local.name.c_str() + " as " + text(c->type));
			continue;
		}
		changed = true;
		log("passed-object-typed", key,
			std::string(local.name.c_str()) + " (" + text(local.type()) + ") as " + text(c->type) + ": " + c->why);
	}
	return changed;
}

Unfold::Structor Unfold::structorOf(ea_t callee)
{
	Structor out;
	for (const SrcModel::Ref& r : m_model.refsAt(callee))
	{
		const SrcModel::Function* f = m_model.function(r.function);
		if (f &&
			((r.kind == "body" && (r.form == "CONSTRUCT" || r.form == "CONSTRUCT_OVERLOAD")) ||
			 (r.kind == "constructor" && f->structor == "ctor")))
			out = {f->cls, false, {}};
		else if (f && ((r.kind == "body" && r.form == "DESTRUCT") || (r.kind == "destructor" && f->structor == "dtor")))
			out = {f->cls, true, {}};
		if (!out.cls.empty())
			break;
	}
	if (out.cls.empty())
	{
		std::string name = pdbSpelling(callee);
		auto found = m_pdbStructors.find(ExeVtables::classKey(name));
		if (found == m_pdbStructors.end())
			return {};
		out = {found->second, lastComponent(name).starts_with('~'), {}};
	}
	// a fold (src names the address under several functions) says nothing of the class
	std::set<std::string> names = chooseRef(m_model, callee).names;
	if (names.size() > 1)
	{
		out.cls.clear();
		for (const std::string& n : names)
			out.fold += (out.fold.empty() ? "" : ", ") + n;
	}
	return out;
}

const SrcModel::Record* Unfold::recordOfStructor(const std::string& cls)
{
	if (cls.empty())
		return nullptr;
	// src's record of the name, else of IDA's struct of the name (a template instance, spelled as the PDB does)
	if (const SrcModel::Record* record = m_model.record(cls))
		return record;
	tinfo_t named;
	return named.get_named_type(get_idati(), cls.c_str()) && named.is_udt() ? m_types.recordOf(named) : nullptr;
}

bool Unfold::templateStructor(ea_t callee)
{
	const SrcModel::Record* record = recordOfStructor(structorOf(callee).cls);
	return record != nullptr && !record->pattern.empty();
}

std::optional<ea_t> Unfold::argumentsAt(func_t* function, sval_t start, sval_t end)
{
	for (ea_t item : itemHeads(function))
	{
		insn_t insn;
		if (decode_insn(&insn, item) <= 0 || !is_call_insn(insn))
			continue;
		std::optional<CallBytes> bytes = callBytes(function, item, {});
		// esp at the call instruction, as an IDA frame offset
		sval_t esp = get_spd(function, item) + frame_off_retaddr(function);
		if (bytes && start >= esp && end <= esp + bytes->pops + bytes->takes)
			return item;
	}
	return std::nullopt;
}

bool Unfold::splitLocal(ea_t start, lvar_uservec_t& saved, const lvar_t& local, ea_t at, const tinfo_t& type,
						const std::string& step, const std::string& reason)
{
	const std::string name = std::string(local.name.c_str()) + " (" + text(local.type()) + ")";
	if (m_undone.contains({start, at}))
	{
		m_kept.emplace_back(step + "-kept", name + ": Hex-Rays merges it again after a split at " + hex(at) + " (" +
												reason + ")");
		return false;
	}
	lvar_saved_info_t info;
	info.ll = lvar_locator_t(local.location, at);
	if (const lvar_saved_info_t* done = saved.find_info(info.ll); done && done->is_split_lvar())
	{
		// a split the database holds (the debug build's) is left as it is
		if (!m_split.contains({start, at}))
		{
			m_kept.emplace_back(step + "-kept", name + ": Hex-Rays merges it over the database's split at " + hex(at) +
													" (" + reason + ")");
			return false;
		}
		// Hex-Rays merged the pieces again: the split is no fact about the exe
		lvar_uservec_t current;
		restore_user_lvar_settings(&current, start);
		if (const lvar_saved_info_t* split = current.find_info(info.ll))
			current.lvvec.erase(current.lvvec.begin() + (split - current.lvvec.begin()));
		save_user_lvar_settings(start, current);
		m_undone.insert({start, at});
		m_split.erase({start, at});
		log(step + "-undone", hex(start), name + ": the split at " + hex(at) + " left it merged (" + reason + ")");
		return true;
	}
	info.type = type;
	info.set_split_lvar();
	if (!modify_user_lvar_info(start, (type.empty() ? 0 : MLI_TYPE) | MLI_SET_FLAGS, info))
	{
		m_kept.emplace_back(step + "-failed", name + " split at " + hex(at));
		return false;
	}
	m_split.insert({start, at});
	log(step + "-split", hex(start),
		name + " at " + hex(at) + (type.empty() ? "" : " as " + text(type)) + ": " + reason);
	return true;
}

// A stack object the exe builds by calling a constructor on its address is an object of the constructor's class, but
// Hex-Rays types the slot from its accesses (bytes, an int array) and merges every object the frame keeps there into
// one lvar (NiBlendAccumTransformInterpolator::ComputeNextValue 0xA39F60: a NiQuatTransform built in one branch, an
// operator* result in the other). Each lvar whose address a constructor call takes (the call's first argument, `&v`
// or the array decaying to it) is typed as the class, through the most-derived of the classes constructed on it when
// they are bases at offset 0 of one another (a base's constructor called on the same slot builds the same object).
// Where the slot holds other objects too (a constructor of an unrelated class, or a use some path from the entry
// reaches before any constructor call; a destructor call of one of its classes is the object's own, whatever the
// flag guarding it), the lvar is split (LVINF_SPLIT) at each constructor call that starts an object (no other of its
// classes' constructor calls on the slot precede it on every path), the new lvar typed there; the next round sees the
// result, and a split of its own that left the objects merged is undone (one the database held is left). The class's
// IDA struct must be src's record at src's size and fit the frame: in the locals, over no other object Hex-Rays has (a
// constructed or struct-typed lvar at another offset that is not the class's member there). An object a call takes as
// its by-value stack arguments (built in place: `sub esp, 1Ch; mov ecx, esp`, in what IDA's frame counts as locals
// where the area is allocated right after the prologue) ends there: a use of the slot some path from that call reaches
// before a constructor call is another object (an argument built inline), which a split or type would merge into it,
// and the lvar is left as it is (Hex-Rays hangs decompiling .text_AB7E30 0xAB7E30 with its std::string split so).
// An object whose constructor the exe inlines shows only its vptr stores: a store of an exe vtable (`v = &X::vftable`,
// `v[3] = ...`, `*(_DWORD *)&v.m = ...`) into a stack slot is a site as a constructor call is, of the class the
// table's RTTI names, on the lvar starting the subobject's offset before the slot (staticGetPenetrationsFunc
// 0xD34790's hkpSymmetricAgentFlipBodyCollector, hkCpuWorldRayCastCollector's four tables in 0xE2B510). The table
// proves the instance, a template's too, and the inlined constructor's stores into the object's members, in whatever
// order, are its own; only a primary table's store into the lvar's first bytes starts an object (a split point).
bool Unfold::constructedLocals(func_t* function, cfunc_t& cfunc)
{
	FlowChart chart(function);
	struct Call
	{
		ea_t call, callee;
		int lvar;
		const cexpr_t* var;
		bool address;  // `&v`, else the array v decaying to its address
	};
	struct Use
	{
		int lvar;
		const cexpr_t* var;
		ea_t ea;
		// a store into a member or element of it (`v.m = x`, `v[1] = x`): an inlined constructor's work on the object
		bool partStore;
	};
	// an assignment of an address in data (`v = &X::vftable`): the value's address, the instruction
	struct Store
	{
		ea_t ea, value;
		const cexpr_t* target;
	};
	// a local's address passed to a call: the parameter's type, the callee (BADADDR: indirect)
	struct Passed
	{
		int lvar;
		tinfo_t parameter;
		ea_t callee;
	};
	// a call's hidden result pointer (FAI_RETPTR): the call, the address, the record its callee builds there
	struct Result
	{
		ea_t call, callee;  // callee: BADADDR for an indirect call
		const cexpr_t* address;
		tinfo_t type;
	};
	struct Occurrences : ctree_visitor_t
	{
		std::vector<Call> calls;
		std::vector<Use> uses;
		std::vector<Passed> passed;
		std::vector<Store> stores;
		std::vector<Result> results;

		Occurrences() : ctree_visitor_t(CV_PARENTS) {}

		// the instruction of the nearest item that has one
		ea_t instruction(const cexpr_t* e) const
		{
			ea_t ea = e->ea;
			for (size_t k = parents.size(); ea == BADADDR && k-- > 0;)
				ea = parents[k]->ea;
			return ea;
		}

		int idaapi visit_expr(cexpr_t* e) override
		{
			if (e->op == cot_var)
			{
				// `v.m`, `v[n]` up to the outermost part; a store into it is a part store
				const citem_t* part = e;
				size_t k = parents.size();
				auto partOf = [&](size_t up) {
					const citem_t* p = parents[up - 1];
					return p != nullptr && (p->op == cot_memref || p->op == cot_idx) &&
						   static_cast<const cexpr_t*>(p)->x == part;
				};
				while (k > 0 && partOf(k))
					part = parents[--k];
				bool partStore = part != e && k > 0 && parents[k - 1] != nullptr && parents[k - 1]->op == cot_asg &&
								 static_cast<const cexpr_t*>(parents[k - 1])->x == part;
				// the object a call takes the address of is used at the call, not where MSVC pushes the address
				// (0xD34790 pushes `&a4` before its vtable store)
				ea_t ea = instruction(e);
				if (!parents.empty() && parents.back()->op == cot_ref)
					for (size_t up = parents.size(); up-- > 0 && parents[up] != nullptr && parents[up]->is_expr();)
						if (parents[up]->op == cot_call && parents[up]->ea != BADADDR)
						{
							ea = parents[up]->ea;
							break;
						}
				uses.push_back({e->v.idx, e, ea, partStore});
			}
			if (e->op == cot_asg)
			{
				const cexpr_t* value = stripCasts(e->y);
				if (value->op == cot_ref && value->x->op == cot_obj)
					stores.push_back({instruction(e), value->x->obj_ea, e->x});
				else if (value->op == cot_obj)
					stores.push_back({instruction(e), value->obj_ea, e->x});
			}
			if (e->op != cot_call || e->a->empty())
				return 0;
			const cexpr_t* callee = stripCasts(e->x);
			const cexpr_t* memory = stripCasts(&(*e->a)[0]);
			bool address = memory->op == cot_ref;
			const cexpr_t* var = address ? memory->x : memory;
			if (callee->op == cot_obj && var->op == cot_var)
				calls.push_back({e->ea, callee->obj_ea, var->v.idx, var, address});
			tinfo_t type = e->x->type.is_ptr() ? e->x->type.get_pointed_object() : e->x->type;
			func_type_data_t fd;
			if (!type.get_func_details(&fd))
				return 0;
			for (size_t k = 0; k < e->a->size() && k < fd.size(); ++k)
			{
				const cexpr_t* a = stripCasts(&(*e->a)[k]);
				if (a->op == cot_ref && a->x->op == cot_var)
					passed.push_back({a->x->v.idx, fd[k].type, callee->op == cot_obj ? callee->obj_ea : BADADDR});
				if ((fd[k].flags & FAI_RETPTR) && fd[k].type.is_ptr() && fd[k].type.get_pointed_object().is_udt() &&
					e->ea != BADADDR)
					results.push_back({e->ea, callee->op == cot_obj ? callee->obj_ea : BADADDR, &(*e->a)[k],
									   fd[k].type.get_pointed_object()});
			}
			return 0;
		}
	};

	const ea_t start = function->start_ea;
	const std::string key = hex(start);
	lvars_t& lvars = *cfunc.get_lvars();
	Occurrences found;
	found.apply_to(&cfunc.body, nullptr);
	// each stack lvar's constructor calls, vtable stores and hidden results and their classes; its destructor calls
	struct Site
	{
		ea_t call, callee;  // a vtable store: the store's instruction, BADADDR; a hidden result: the call, BADADDR
		const SrcModel::Record* record;
		// an object starts there: a constructor call, a call building its result there, or a store of a primary
		// vtable into the lvar's first bytes (a secondary table's store writes inside the object)
		bool starts = true;
		bool vtable = false;
		// a call building its result there
		bool result = false;
	};
	std::map<int, std::vector<Site>> sites;
	std::vector<std::pair<const Call*, const SrcModel::Record*>> destructors;
	std::set<const cexpr_t*> structorVars;
	// the lvars left as they are
	std::vector<std::string> kept;
	for (const Call& c : found.calls)
	{
		const lvar_t& local = lvars[c.lvar];
		Structor structor = local.is_stk_var() && !local.is_arg_var() && (c.address || local.type().is_array())
								? structorOf(c.callee)
								: Structor{};
		if (!structor.fold.empty() && !structor.destructor)
			kept.push_back(std::string(local.name.c_str()) + ": " + hex(c.call) + " calls a constructor folded as " +
						   structor.fold);
		if (structor.cls.empty())
			continue;
		const SrcModel::Record* record = recordOfStructor(structor.cls);
		if (!record)
		{
			if (!structor.destructor)
				kept.push_back(std::string(local.name.c_str()) + ": " + hex(c.call) + " constructs " + structor.cls +
							   ", no src record");
			continue;
		}
		if (structor.destructor)
			destructors.push_back({&c, record});
		else
		{
			sites[c.lvar].push_back({c.call, c.callee, record});
			structorVars.insert(c.var);
		}
	}
	// A store of an exe vtable (the RTTI's class and the subobject's offset) into a stack slot is an inlined
	// constructor's (or destructor's) vptr store: the object of the table's class starts the subobject's offset
	// before the slot (a secondary table's store into another lvar names the object's lvar too)
	for (const Store& s : found.stores)
	{
		const ExeVtables::Table* table = nullptr;
		for (const ExeVtables::Table& t : m_exe.tables())
			if (t.address == s.value)
				table = &t;
		std::optional<std::pair<int, int64_t>> slot = table ? lvalueSlot(s.target) : std::nullopt;
		if (!slot || !lvars[slot->first].is_stk_var() || lvars[slot->first].is_arg_var())
			continue;
		const lvar_t& target = lvars[slot->first];
		sval_t at = cfunc.mba->stkoff_vd2ida(target.get_stkoff()) + slot->second - table->offset;
		// the store's own lvar where it starts the object, else the one lvar starting there (pieces of a split lvar
		// share the slot: none is the object's)
		int object = slot->second == table->offset ? slot->first : -1;
		int starting = 0;
		for (size_t k = 0; k < lvars.size() && object < 0; ++k)
			if (lvars[k].is_stk_var() && !lvars[k].is_arg_var() &&
				cfunc.mba->stkoff_vd2ida(lvars[k].get_stkoff()) == at)
				starting += 1;
		for (size_t k = 0; k < lvars.size() && object < 0 && starting == 1; ++k)
			if (lvars[k].is_stk_var() && !lvars[k].is_arg_var() &&
				cfunc.mba->stkoff_vd2ida(lvars[k].get_stkoff()) == at)
				object = static_cast<int>(k);
		std::string where = std::string(target.name.c_str()) + ": " + hex(s.ea) + " stores " + table->cls +
							"'s vtable for the subobject at +" + std::to_string(table->offset);
		if (object < 0)
		{
			kept.push_back(where + (starting > 1 ? ", several locals at the object's start" :
												   ", no local at the object's start"));
			continue;
		}
		const SrcModel::Record* record = m_types.recordOfClass(table->cls);
		if (!record)
		{
			kept.push_back(where + ", no src record");
			continue;
		}
		bool own = slot->first == object && slot->second == 0 && table->offset == 0;
		sites[object].push_back({s.ea, BADADDR, record, own, true});
		if (slot->first == object)
		{
			const cexpr_t* var = s.target;
			while (var->op != cot_var && (var->op == cot_memref || var->op == cot_idx || var->op == cot_ptr ||
										  var->op == cot_cast || var->op == cot_ref || var->op == cot_add))
				var = var->x;
			if (var->op == cot_var)
				structorVars.insert(var);
		}
	}
	// A call building its record result through the hidden result pointer (FAI_RETPTR) into a stack slot constructs
	// the object there as a constructor does: a site of the callee's declared result class on the lvar starting the
	// slot (the slot's own where it starts it, else the one lvar starting there), where Hex-Rays has the slot as
	// another type (a lvar of the class there, or a member or element of it, is already the object). Such sites only
	// type a lvar no constructor or vtable names, that no lvar setting types (the debug build's: MSVC reuses a slot for
	// objects of disjoint scopes, 0xC79340's hkQsTransform worldFromModel and a NiMatrix3 product) and that holds
	// nothing else; they split none (Hex-Rays hangs on 0xA39F60 with its NiQuaternion slot split at its operator*
	// results)
	lvar_uservec_t saved;
	restore_user_lvar_settings(&saved, start);
	for (const Result& r : found.results)
	{
		std::optional<std::pair<int, int64_t>> slot = addressSlot(r.address);
		if (!slot || !lvars[slot->first].is_stk_var() || lvars[slot->first].is_arg_var())
			continue;
		const lvar_t& target = lvars[slot->first];
		if ((slot->second == 0 && sameType(target.type(), r.type)) ||
			holdsAt(target.type(), static_cast<uint64>(slot->second), r.type))
			continue;
		sval_t at = cfunc.mba->stkoff_vd2ida(target.get_stkoff()) + slot->second;
		int object = slot->second == 0 ? slot->first : -1;
		int starting = 0;
		for (size_t k = 0; k < lvars.size() && object < 0; ++k)
			if (lvars[k].is_stk_var() && !lvars[k].is_arg_var() &&
				cfunc.mba->stkoff_vd2ida(lvars[k].get_stkoff()) == at)
				starting += 1;
		for (size_t k = 0; k < lvars.size() && object < 0 && starting == 1; ++k)
			if (lvars[k].is_stk_var() && !lvars[k].is_arg_var() &&
				cfunc.mba->stkoff_vd2ida(lvars[k].get_stkoff()) == at)
				object = static_cast<int>(k);
		std::string where =
			std::string(target.name.c_str()) + ": " + hex(r.call) + " builds its " + text(r.type) + " result there";
		if (object < 0)
		{
			kept.push_back(where + (starting > 1 ? ", several locals at the result's start" :
												   ", no local at the result's start"));
			continue;
		}
		const SrcModel::Record* record = m_types.recordOf(r.type);
		if (!record)
		{
			kept.push_back(where + ", no src record");
			continue;
		}
		// a fold's result is its owner's type, and a template instance's result proves the template: the exe's
		// instances share one copy where their code is the same (NiTPointerList<NiPointer<T>>::RemoveTail)
		if ((r.callee != BADADDR && folds().contains(r.callee)) || !record->pattern.empty())
		{
			kept.push_back(where + (record->pattern.empty() ? ", a fold's" : ", a template instance's, which proves "
																			  "the template only"));
			continue;
		}
		if (const lvar_saved_info_t* setting = saved.find_info(lvars[object]); setting && !setting->type.empty())
		{
			kept.push_back(where + ", whose local a setting types");
			continue;
		}
		// its address passed for a pointer to an unrelated record: another object MSVC keeps in the slot (0xC79340's
		// NI2HK out parameter)
		auto unrelated = [&](const Passed& p) {
			if (p.lvar != object || !p.parameter.is_ptr() || !p.parameter.get_pointed_object().is_udt())
				return false;
			const SrcModel::Record* other = m_types.recordOf(p.parameter.get_pointed_object());
			return other == nullptr || (!m_types.derivesAtZero(other->name, record->name) &&
										!m_types.derivesAtZero(record->name, other->name));
		};
		if (std::any_of(found.passed.begin(), found.passed.end(), unrelated))
		{
			kept.push_back(where + ", whose address a call takes as another record's");
			continue;
		}
		if (auto other = sites.find(object); other != sites.end() &&
			std::any_of(other->second.begin(), other->second.end(), [](const Site& s) { return !s.result; }))
		{
			kept.push_back(where + ", where a constructor or vtable names another object");
			continue;
		}
		sites[object].push_back({r.call, BADADDR, record, true, false, true});
		if (slot->first == object)
		{
			const cexpr_t* var = stripCasts(r.address);
			while (var->op != cot_var && (var->op == cot_memref || var->op == cot_idx || var->op == cot_ptr ||
										  var->op == cot_cast || var->op == cot_ref || var->op == cot_add))
				var = var->x;
			if (var->op == cot_var)
				structorVars.insert(var);
		}
	}
	bool changed = false;
	for (const auto& [v, calls] : sites)
	{
		lvar_t& local = lvars[v];
		std::string name = local.name.c_str();
		auto keep = [&](const std::string& why) { kept.push_back(name + " (" + text(local.type()) + "): " + why); };
		// the classes constructed on it, in groups of bases at offset 0 of one another, each group's most-derived
		std::vector<std::vector<const Site*>> groups;
		for (const Site& s : calls)
		{
			auto related = [&](const Site* t) {
				return m_types.derivesAtZero(s.record->name, t->record->name) ||
					   m_types.derivesAtZero(t->record->name, s.record->name);
			};
			auto group = std::find_if(groups.begin(), groups.end(), [&](const std::vector<const Site*>& g) {
				return std::any_of(g.begin(), g.end(), related);
			});
			if (group == groups.end())
				groups.push_back({&s});
			else
				group->push_back(&s);
		}
		struct Object
		{
			const SrcModel::Record* record;
			tinfo_t type;
			std::set<ea_t> calls, starts;
			// a vtable stored on it: the table proves the class, a template's instance included
			bool vtable = false;
		};
		std::vector<Object> objects;
		std::string why;
		for (const std::vector<const Site*>& group : groups)
		{
			// of a base and a class deriving from it at one size, the derived one
			const SrcModel::Record* most = group.front()->record;
			for (const Site* s : group)
				if (s->record->size > most->size ||
					(s->record->size == most->size && m_types.derivesAtZero(s->record->name, most->name)))
					most = s->record;
			Object object{most, {}, {}, {}};
			for (const Site* s : group)
			{
				object.calls.insert(s->call);
				if (s->starts)
					object.starts.insert(s->call);
				object.vtable |= s->vtable;
				if (!m_types.derivesAtZero(most->name, s->record->name))
					why = most->name + " and " + s->record->name + " constructed on it: neither derives from the other";
			}
			std::optional<int64_t> size = m_model.sizeOf("record:" + most->name);
			if (!object.type.get_named_type(get_idati(), m_structNames.of(most->name).c_str()) ||
				!object.type.is_udt() || !size || static_cast<int64_t>(object.type.get_size()) != *size)
				why = most->name + ": no IDA struct at src's size";
			else if (!local.accepts_type(object.type))
				why = most->name + ": Hex-Rays does not take its IDA struct as a local's type";
			// in the frame's locals, over no other object Hex-Rays has. Below them (a negative IDA frame offset)
			// is the outgoing argument area: an object built there (`sub esp, 0xc; mov ecx, esp`, a by-value
			// argument: TESObjectREFR::SayForceGoodbyeGreetingToPlayer 0x57B7C0, a BSSoundHandle for
			// Interface::ShowSubtitle) is no local, and Hex-Rays hangs decompiling the function with it split
			sval_t offset = cfunc.mba->stkoff_vd2ida(local.get_stkoff());
			int64_t end = offset + static_cast<int64_t>(object.type.get_size());
			if (why.empty() && offset < 0)
				why = most->name + ": built in the outgoing arguments, not in the frame's locals";
			if (why.empty() && end > static_cast<int64_t>(function->frsize))
				why = most->name + " (" + std::to_string(*size) + " bytes) runs past the frame's locals";
			for (size_t k = 0; k < lvars.size() && why.empty(); ++k)
			{
				const lvar_t& other = lvars[k];
				sval_t at = other.is_stk_var() ? cfunc.mba->stkoff_vd2ida(other.get_stkoff()) : offset;
				if (at != offset && at < end && at + other.width > offset &&
					(sites.contains(static_cast<int>(k)) || other.type().is_udt()) &&
					!holdsAt(object.type, at - offset, other.type()))
					why = most->name + " (" + std::to_string(*size) + " bytes) would take in " + other.name.c_str() +
						  " (" + text(other.type()) + ") at +" + std::to_string(at - offset);
			}
			objects.push_back(std::move(object));
		}
		if (!why.empty())
		{
			keep(why);
			continue;
		}
		// the uses some path reaches before any constructor call on the slot, its classes' structor calls aside
		std::set<ea_t> every;
		for (const Site& s : calls)
			every.insert(s.call);
		std::set<const cexpr_t*> own = structorVars;
		for (const auto& [call, record] : destructors)
			if (call->lvar == v && std::any_of(calls.begin(), calls.end(), [&](const Site& s) {
					return m_types.derivesAtZero(s.record->name, record->name) ||
						   m_types.derivesAtZero(record->name, s.record->name);
				}))
				own.insert(call->var);
		// the uses some path reaches before any constructor call on the slot, its classes' structor calls aside, and
		// where a vtable is stored on it, the stores into its members (an inlined constructor's, in any order)
		bool stored = std::any_of(calls.begin(), calls.end(), [](const Site& s) { return s.vtable; });
		std::vector<ea_t> before;
		for (const Use& u : found.uses)
			if (u.lvar == v && !own.contains(u.var) && !(stored && u.partStore) && u.ea != BADADDR &&
				reachesAvoiding(function, chart, every, u.ea))
				before.push_back(u.ea);
		// calls building their results there only type a slot that holds nothing else
		if (std::all_of(calls.begin(), calls.end(), [](const Site& s) { return s.result; }) &&
			(objects.size() > 1 || !before.empty()))
		{
			keep(objects.size() > 1 ? std::to_string(objects.size()) + " unrelated classes built there by results"
									: "used at " + hex(before.front()) + " before the result built there");
			continue;
		}
		// the slot after a call that takes the object as its by-value arguments: another object
		if (objects.size() == 1)
		{
			sval_t offset = cfunc.mba->stkoff_vd2ida(local.get_stkoff());
			std::optional<ea_t> taken =
				argumentsAt(function, offset, offset + static_cast<sval_t>(objects[0].type.get_size()));
			auto after = std::find_if(found.uses.begin(), found.uses.end(), [&](const Use& u) {
				return taken && u.lvar == v && !own.contains(u.var) && u.ea != BADADDR &&
					   reachesFrom(chart, *taken, u.ea, every);
			});
			if (after != found.uses.end())
			{
				keep(objects[0].record->name + ": the call at " + hex(*taken) +
					 " takes it as its stack arguments, and the slot holds another object at " + hex(after->ea));
				continue;
			}
		}
		// a constructor of a class template's instance proves the template, not the instance: the exe's instances
		// share it where their code is the same (NiPointer's 0x633C90, which src registers on
		// NiPointer<ActorPathingMessageQueue> and every NiPointer<T> of a NiRefObject calls). A local already of an
		// instance of the template stays as it is (the debug build's); another takes the one instance the calls its
		// address is passed to name (a parameter pointing to an instance of the template, a template structor's
		// aside), else stays as it is. A vtable is the instance's own
		const std::string& pattern = objects[0].record->pattern;
		if (objects.size() == 1 && before.empty() && !pattern.empty() && !objects[0].vtable)
		{
			auto instanceOf = [&](const tinfo_t& t) {
				const SrcModel::Record* r = t.is_udt() ? m_types.recordOf(t) : nullptr;
				return r != nullptr && r->pattern == pattern;
			};
			if (instanceOf(local.type()))
				continue;
			std::vector<tinfo_t> named;
			for (const Passed& p : found.passed)
				if (p.lvar == v && p.parameter.is_ptr() && instanceOf(p.parameter.get_pointed_object()) &&
					(p.callee == BADADDR || !templateStructor(p.callee)) &&
					std::none_of(named.begin(), named.end(),
								 [&](const tinfo_t& t) { return sameType(t, p.parameter.get_pointed_object()); }))
					named.push_back(p.parameter.get_pointed_object());
			std::string list;
			for (const tinfo_t& t : named)
				list += (list.empty() ? "" : ", ") + text(t);
			if (named.size() != 1 || named[0].get_size() != objects[0].type.get_size() ||
				!local.accepts_type(named[0]))
			{
				keep("constructed by " + objects[0].record->name + "'s constructor at " +
					 hex(*objects[0].calls.begin()) + ", which proves the template " + pattern +
					 " only, and the calls taking its address name " +
					 (named.empty() ? std::string("no instance") : list));
				continue;
			}
			objects[0].type = named[0];
		}
		if (objects.size() == 1 && before.empty())
		{
			if (sameType(local.type(), objects[0].type))
				continue;
			lvar_saved_info_t info;
			info.ll = local;
			info.type = objects[0].type;
			if (!modify_user_lvar_info(start, MLI_TYPE, info))
			{
				m_kept.emplace_back("constructed-local-failed", name);
				continue;
			}
			changed = true;
			log("constructed-local-typed", key,
				name + " (" + text(local.type()) + ") as " + text(info.type) + ": constructed at " +
					hex(*objects[0].calls.begin()));
			continue;
		}
		std::string reason = objects.size() > 1 ? std::to_string(objects.size()) + " unrelated classes constructed"
												: "used at " + hex(before.front()) + " before any constructor call";
		for (const Object& object : objects)
			for (ea_t call : object.starts)
			{
				// an object starts at the call: no other constructor call of its classes precedes it on every path
				std::set<ea_t> others = object.calls;
				others.erase(call);
				if (reachesAvoiding(function, chart, others, call))
					changed |= splitLocal(start, saved, local, call,
										  object.record->pattern.empty() || object.vtable ? object.type : tinfo_t(),
										  "constructed-local", reason);
			}
	}
	for (const std::string& k : kept)
		m_kept.emplace_back("constructed-local-kept", k);
	return changed;
}

// Hex-Rays types a local from the first access it meets (an int for a dword slot, `char[4]` for a slot whose address
// goes to a C-variadic parameter, the pointee of one callee's guess), while the exe's code uses the value as one other
// type everywhere: the script commands' `char a8[4]` that ParseParameters fills and every read takes as `TESForm *`,
// an int assigned a call's pointer and passed on as one. Each occurrence of a local Hex-Rays typed (no lvar setting
// gives its type: the debug build's and the other steps' stay) asks something of its type U:
// - a store, that the stored value (past Hex-Rays' casts) converts to U; a number asks nothing, nor does a call src
//   declares returning `void *` (an allocation, __RTDynamicCast): untyped memory, of the type the local is used as; a
//   floating value stored as a floating type (`*(float *)a8 = d`) asks that type: C++ converts between them itself;
// - a read, that U converts to what takes it: a cast's type, a parameter's, an assigned local's or field's, the other
//   side of an equality, the function's return; a member access the record U points to has (U derives from it); a
//   truth test, a C-variadic argument ask nothing;
// - its address (`&v`, or the array decaying to it): a whole access through a cast (`*(T *)&v`) a store or read of
//   the object as above; passed to a pointer parameter or assigned to a pointer, that the object is the pointee or
//   derives from it (a void pointer, an integer asks nothing);
// - an array local's element at a constant index (`v[2]`), that U is the array type or a record with a member of the
//   element's type, or a scalar one of its size, at that offset (`_DWORD v[4]` filled member by member and passed
//   as a `hkpCdBody *`);
// - anything else (arithmetic, a part, a dereference), that U is the type it has.
// A conversion here is one C++ writes without a cast: the same type, integers of one size, a pointer to and from a void
// pointer, a record pointer to its base's at offset 0. Where the type the local has does not meet every ask and
// exactly one of the types asked for (of the local's size) does, or one of those that do converts to each other one,
// the local gets it: none of its casts are left. An asked IDA sized unknown (`_DWORD`) is asked as the unsigned
// integer of its size: Hex-Rays ignores it as a local's type. A local Hex-Rays typed as an array or record
// whose address no use passes on (every use a whole access through a cast of its address, or a store of a number)
// may take a smaller asked type: no use touches the bytes past it. A typed pointer is not weakened to an integer or a
// void pointer. A local no asked type meets is left as it is (logged with the types asked).
bool Unfold::valueLocals(func_t* function, cfunc_t& cfunc)
{
	// the src record a type is
	auto recordOf = [&](const tinfo_t& t) -> const SrcModel::Record* {
		return t.is_udt() ? m_types.recordOf(t) : nullptr;
	};
	auto derives = [&](const tinfo_t& a, const tinfo_t& b) {
		const SrcModel::Record* from = recordOf(a);
		const SrcModel::Record* to = recordOf(b);
		return from && to && m_types.derivesAtZero(from->name, to->name);
	};
	// C++'s conversion of a value of type a to type b without a cast
	auto converts = [&](const tinfo_t& a, const tinfo_t& b) {
		if (sameType(a, b))
			return true;
		if (a.is_integral() && b.is_integral())
			return a.get_size() == b.get_size();
		if (!a.is_ptr() || !b.is_ptr())
			return false;
		const tinfo_t from = a.get_pointed_object(), to = b.get_pointed_object();
		return from.is_void() || to.is_void() || derives(from, to);
	};
	// a store or read across a record pointer's base at offset 0 (an upcast, or a static_cast down) is no
	// reinterpretation either
	auto related = [&](const tinfo_t& a, const tinfo_t& b) {
		return a.is_ptr() && b.is_ptr() && (converts(a, b) || converts(b, a));
	};
	// the record u has a member of the element type t at offset (a union such as `__m128` as itself), or a scalar
	// one (an array's element) of t's size there, through nested records
	auto memberAt = [](tinfo_t u, int64_t offset, const tinfo_t& t) {
		while (offset >= 0)
		{
			if (offset == 0 && sameType(u, t))
				return true;
			if (u.is_array())
			{
				const tinfo_t element = u.get_array_element();
				const size_t step = element.get_size();
				if (step == 0 || step == BADSIZE || static_cast<uint64_t>(offset) >= u.get_size())
					return false;
				offset %= static_cast<int64_t>(step);
				u = element;
				continue;
			}
			if (!u.is_udt())
				return offset == 0 && u.get_size() == t.get_size();
			udm_t member;
			member.offset = static_cast<uint64>(offset) * 8;
			if (u.find_udm(&member, STRMEM_OFFSET) < 0 || member.is_bitfield())
				return false;
			offset -= static_cast<int64_t>(member.offset / 8);
			u = member.type;
		}
		return false;
	};
	// the struct u is t held whole: its one member (no base, no bit-field) at offset 0 of u's size is t, or such a
	// struct holding t in turn (hkVector4's `__m128 m_quad`; hkQuaternion's `hkVector4 m_vec`). Hex-Rays reads and
	// writes a value of t in such a local through that member (`v.m_quad`, `q.m_vec.m_quad`), with no cast
	auto holds = [](tinfo_t u, const tinfo_t& t) {
		while (u.is_struct())
		{
			udt_type_data_t udt;
			if (!u.get_udt_details(&udt) || udt.size() != 1 || udt[0].offset != 0 || udt[0].is_bitfield() ||
				udt[0].is_baseclass() || udt[0].size != u.get_size() * 8)
				return false;
			u = udt[0].type;
			if (sameType(u, t))
				return true;
		}
		return false;
	};
	auto meets = [&](const tinfo_t& u, const tinfo_t& own, const Want& w) {
		switch (w.kind)
		{
		case Want::none: return true;
		case Want::into: return converts(w.t, u) || related(w.t, u) || holds(u, w.t);
		case Want::outof: return converts(u, w.t) || related(w.t, u) || holds(u, w.t);
		case Want::object: return sameType(u, w.t) || derives(u, w.t);
		case Want::exact: return sameType(u, own);
		case Want::element: return sameType(u, own) || (u.is_udt() && memberAt(u, w.offset, w.t));
		}
		return false;
	};

	lvars_t& lvars = *cfunc.get_lvars();
	tinfo_t self;
	Occurrences found(lvars, cfunc.get_func_type(&self) ? self.get_rettype() : tinfo_t(),
					  [this](ea_t callee) { return templateStructor(callee); },
					  [this](ea_t callee) { return untypedResult(callee); });
	found.apply_to(&cfunc.body, nullptr);
	const ea_t start = function->start_ea;
	bool changed = false;
	std::optional<FlowChart> chart;
	std::optional<lvar_uservec_t> saved;
	// the slots whose values were weighed for a split this round
	std::vector<const lvar_t*> weighed;
	// The stack objects whose address the code takes, each over its bytes: the local's, or more where the address
	// converts to a pointer to a larger pointee (`(hkpCdBody *)&v144` over v144 and the locals after it), except a
	// whole access of the local alone (`*(float *)&v = x`), which is a store or read of it. A callee that takes such
	// an address reads and writes the object's slots unseen by Hex-Rays: the values there are the object's
	// members, not values of their own (0xD302D0, whose hkpCdBody at v144 holds v145 and `unused`: Hex-Rays hangs
	// with the slot split at v144's store and at either of theirs).
	struct Taken
	{
		sval_t begin, end;
		std::string name;
	};
	std::optional<std::vector<Taken>> taken;
	auto addressTaken = [&]() -> const std::vector<Taken>& {
		if (taken)
			return *taken;
		struct Refs : ctree_visitor_t
		{
			const lvars_t& lvars;
			std::vector<Taken> out;
			Refs(const lvars_t& l) : ctree_visitor_t(CV_PARENTS), lvars(l) {}
			int idaapi visit_expr(cexpr_t* e) override
			{
				if (e->op != cot_ref || e->x->op != cot_var || !lvars[e->x->v.idx].is_stk_var())
					return 0;
				const lvar_t& local = lvars[e->x->v.idx];
				const size_t width = static_cast<size_t>(local.width);
				size_t size = width;
				auto pointee = [&](const tinfo_t& t) {
					if (size_t s = t.is_ptr() ? t.get_pointed_object().get_size() : BADSIZE; s != BADSIZE)
						size = std::max(size, s);
				};
				pointee(e->type);
				const cexpr_t* p = !parents.empty() && parents.back()->is_expr()
									   ? static_cast<const cexpr_t*>(parents.back())
									   : nullptr;
				const cexpr_t* access = p;
				if (p != nullptr && p->op == cot_cast)
				{
					pointee(p->type);
					access = parents.size() > 1 && parents[parents.size() - 2]->is_expr()
								 ? static_cast<const cexpr_t*>(parents[parents.size() - 2])
								 : nullptr;
				}
				// a whole access of the local alone
				if (access != nullptr && access->op == cot_ptr && access->type.get_size() <= width && size == width)
					return 0;
				sval_t begin = local.location.stkoff();
				out.push_back({begin, begin + static_cast<sval_t>(size), local.name.c_str()});
				return 0;
			}
		} refs(lvars);
		refs.apply_to(&cfunc.body, nullptr);
		taken = std::move(refs.out);
		return *taken;
	};
	// A slot the exe keeps values of unrelated types in (stores of two type groups: no conversion between them) is
	// split at each store that starts a value of another group than the one before it (some path from a store of
	// another group reaches it with no store of a group between), where Hex-Rays has that store in one local with a
	// store of another group, and the slot lies in no stack object whose address the code takes; the next round
	// types each piece from its own occurrences. Returns the reason a slot is left as it is, empty where it was
	// weighed (split, or each piece already of one group).
	auto split = [&](const lvar_t& local) -> std::string {
		if (!local.is_stk_var())
			return "a register";
		if (std::any_of(weighed.begin(), weighed.end(), [&](const lvar_t* o) { return o->location == local.location; }))
			return {};
		weighed.push_back(&local);
		const sval_t at = local.location.stkoff();
		for (const Taken& t : addressTaken())
			if (at < t.end && at + local.width > t.begin)
				return "in the object at " + t.name + " whose address the code takes (" +
					   std::to_string(t.end - t.begin) + " bytes)";
		struct Store
		{
			ea_t ea;
			int lvar;
			size_t group;
		};
		// the slot's typed stores, of each local Hex-Rays has on it, in groups of the types they are stored as that
		// convert to one another (an enum to and from the integers of its size: one value's numbers)
		auto number = [](const tinfo_t& t) { return t.is_integral() || t.is_enum(); };
		std::vector<Store> stores;
		std::vector<tinfo_t> groups;
		for (const auto& [o, ws] : found.wants)
			if (lvars[o].location == local.location && lvars[o].width == local.width)
				for (const Want& w : ws)
					if (w.store != BADADDR && w.kind == Want::into)
					{
						auto g = std::find_if(groups.begin(), groups.end(), [&](const tinfo_t& t) {
							return converts(t, w.as) || converts(w.as, t) ||
								   (number(t) && number(w.as) && t.get_size() == w.as.get_size());
						});
						if (g == groups.end())
							g = groups.insert(groups.end(), w.as);
						stores.push_back({w.store, o, static_cast<size_t>(g - groups.begin())});
					}
		if (groups.size() < 2)
			return "its stores are of one type group";
		std::string reason;
		for (const Store& s : stores)
			if (reason.size() < 400)
				reason += (reason.empty() ? "stores " : ", ") + text(groups[s.group]) + " at " + hex(s.ea);
		if (!chart)
			chart.emplace(function);
		if (!saved)
		{
			saved.emplace();
			restore_user_lvar_settings(&*saved, start);
		}
		std::set<ea_t> every;
		for (const Store& s : stores)
			every.insert(s.ea);
		// the stores some path from a store of another group reaches with no store between: each starts a value
		std::set<ea_t> begins;
		const std::vector<FlowChart::Block>& blocks = chart->blocks();
		for (const Store& o : stores)
		{
			std::vector<bool> seen(blocks.size());
			std::vector<size_t> pending;
			// the first store at or after ea in the block, else its successors
			auto walk = [&](size_t k, ea_t ea) {
				auto next = every.lower_bound(ea);
				if (next != every.end() && *next < blocks[k].end)
				{
					if (const Store& s = *std::find_if(stores.begin(), stores.end(),
													  [&](const Store& t) { return t.ea == *next; });
						s.group != o.group)
						begins.insert(s.ea);
					return;
				}
				for (int n : blocks[k].succs)
					pending.push_back(static_cast<size_t>(n));
			};
			for (size_t k = 0; k < blocks.size(); ++k)
				if (o.ea >= blocks[k].start && o.ea < blocks[k].end)
					walk(k, o.ea + 1);
			while (!pending.empty())
			{
				size_t k = pending.back();
				pending.pop_back();
				if (!seen[k])
				{
					seen[k] = true;
					walk(k, blocks[k].start);
				}
			}
		}
		// one split per slot and round, at its first store starting a value that Hex-Rays still merges with a store
		// of another group: the next round sees the pieces (Hex-Rays hangs on some functions given many at once)
		std::sort(stores.begin(), stores.end(), [](const Store& a, const Store& b) { return a.ea < b.ea; });
		bool any = false;
		// the instruction at ea writes a memory operand at the slot's frame offset
		const ea_t slot = static_cast<ea_t>(cfunc.mba->stkoff_vd2ida(local.get_stkoff()));
		auto writesSlot = [&](ea_t ea) {
			insn_t insn;
			if (decode_insn(&insn, ea) <= 0)
				return false;
			uint32 feature = insn.get_canon_feature(PH);
			for (int n = 0; n < UA_MAXOP && insn.ops[n].type != o_void; ++n)
				if (has_cf_chg(feature, n) && (insn.ops[n].type == o_displ || insn.ops[n].type == o_phrase) &&
					calc_stkvar_struc_offset(function, insn, n) == slot)
					return true;
			return false;
		};
		for (const Store& s : stores)
		{
			bool merged = std::any_of(stores.begin(), stores.end(),
									  [&](const Store& o) { return o.lvar == s.lvar && o.group != s.group; });
			if (!begins.contains(s.ea) || !merged)
				continue;
			any = true;
			const lvar_t& holder = lvars[s.lvar];
			if (holder.defea == s.ea && !m_split.contains({start, s.ea}))
				continue;
			// a definition only where the exe's instruction there has a memory operand it writes at the slot: Hex-Rays
			// may put a store's ea on another instruction of the statement, or on a push into an argument area IDA's
			// frame counts as locals (0xD62780's `push esi`)
			if (!writesSlot(s.ea))
			{
				m_kept.emplace_back("value-local-split-rejected",
									std::string(holder.name.c_str()) + " (" + text(holder.type()) +
										"): the instruction at " + hex(s.ea) + " writes no memory operand at its slot");
				continue;
			}
			if (splitLocal(start, *saved, holder, s.ea, tinfo_t(), "value-local", reason))
			{
				changed = true;
				break;
			}
		}
		return any ? std::string() : "no store of another group starts a value in one local with it (" + reason + ")";
	};
	for (const auto& [v, wants] : found.wants)
	{
		lvar_t& local = lvars[v];
		const tinfo_t own = local.type();
		auto meetsAll = [&](const tinfo_t& u) {
			return std::all_of(wants.begin(), wants.end(), [&](const Want& w) { return meets(u, own, w); });
		};
		if (meetsAll(own))
			continue;
		// the types asked for, of the local's size, and those of them meeting every ask. A local Hex-Rays typed as an
		// array or record whose address no use passes on with no extent may take a smaller type: the bytes past it are
		// touched by no use
		const bool narrows = (own.is_array() || own.is_udt()) &&
							 std::none_of(wants.begin(), wants.end(), [](const Want& w) { return w.unbounded; });
		std::vector<tinfo_t> asked, fit;
		for (const Want& w : wants)
		{
			if (w.kind == Want::none || w.kind == Want::exact || w.kind == Want::element)
				continue;
			const tinfo_t t = plain(w.t);
			const size_t size = t.get_size();
			if (size == BADSIZE || size == 0 || size > static_cast<size_t>(local.width) ||
				(size < static_cast<size_t>(local.width) && !narrows) || t.is_void() || t.is_func() || t.is_array() ||
				std::any_of(asked.begin(), asked.end(), [&](const tinfo_t& u) { return sameType(u, t); }))
				continue;
			asked.push_back(t);
			if (meetsAll(t))
				fit.push_back(t);
		}
		// of several, the one converting to each other one
		auto best = std::find_if(fit.begin(), fit.end(), [&](const tinfo_t& u) {
			return std::all_of(fit.begin(), fit.end(), [&](const tinfo_t& o) { return converts(u, o); });
		});
		if (best == fit.end())
		{
			if (!asked.empty())
			{
				std::string list;
				for (const tinfo_t& t : asked)
					list += (list.empty() ? "" : ", ") + text(t);
				std::string why = split(local);
				if (!why.empty())
					m_kept.emplace_back("value-local-kept", std::string(local.name.c_str()) + " (" + text(own) +
																 "): no one type meets its occurrences, asked " + list +
																 "; not split: " + why);
			}
			continue;
		}
		const tinfo_t to = *best;
		if ((untyped(to) && !untyped(own) && own.is_ptr()) || !local.accepts_type(to))
			continue;
		lvar_saved_info_t info;
		info.ll = local;
		info.type = to;
		if (!modify_user_lvar_info(start, MLI_TYPE, info))
		{
			m_kept.emplace_back("value-local-failed", local.name.c_str());
			continue;
		}
		changed = true;
		log("value-local-typed", hex(start),
			std::string(local.name.c_str()) + " (" + text(own) + ") as " + text(to) + ": it meets every occurrence");
	}
	return changed;
}

bool Unfold::untypedResult(ea_t callee)
{
	auto [at, added] = m_voidResults.emplace(callee, false);
	if (added)
		if (const SrcModel::Function* f = declared(callee))
			if (std::optional<tinfo_t> type = prototype(*f, callee))
			{
				const tinfo_t result = type->get_rettype();
				at->second = result.is_ptr() && result.get_pointed_object().is_void();
			}
	return at->second;
}

std::map<int, Unfold::Asked> Unfold::askedTypes(cfunc_t& cfunc)
{
	tinfo_t self;
	Occurrences found(*cfunc.get_lvars(), cfunc.get_func_type(&self) ? self.get_rettype() : tinfo_t(),
					  [this](ea_t callee) { return templateStructor(callee); },
					  [this](ea_t callee) { return untypedResult(callee); }, true);
	found.apply_to(&cfunc.body, nullptr);
	std::map<int, Asked> out;
	for (const auto& [v, wants] : found.wants)
	{
		Asked& asked = out[v];
		asked.bounded = std::none_of(wants.begin(), wants.end(), [](const Want& w) { return w.unbounded; });
		for (const Want& w : wants)
		{
			if (w.kind != Want::into && w.kind != Want::outof && w.kind != Want::object)
				continue;
			const tinfo_t t = plain(w.t);
			const size_t size = t.get_size();
			if (size == BADSIZE || size == 0 || t.is_void() || t.is_func() ||
				std::any_of(asked.types.begin(), asked.types.end(), [&](const tinfo_t& u) { return sameType(u, t); }))
				continue;
			asked.types.push_back(t);
		}
	}
	return out;
}
// MSVC keeps other values in a stack argument's slot once the argument is dead: a read's byte count for
// NiBinaryStream's m_pfnRead (`mov [esp+arg_0], 4`, its uint32_t* out; NiTCBFloatKey::LoadBinary 0xA2C9D0), a viewer
// string SetAt takes by reference (NiObjectNET::GetViewerStrings 0xA5CA80). Hex-Rays reads the slot as the argument,
// so every use of the other value is a cast of the argument (`*(char**)&apStrings`, `(uint32_t*)&pStream`). A store
// into a stack argument of a value that is not of its type starts another variable: an assignment to the argument
// of a value Hex-Rays casts to its type from a type it does not convert to by itself (a pointer to an unrelated
// pointee, an integer or float for a pointer, a pointer for an integer, a float for an integer or back; a nonzero
// number for a pointer), a store of a type T through the argument's address (`*(T*)&arg = v`), or a call's hidden
// result of a type T built there (`isEnabled((hkBool*)&aiId, ...)`, or `&aiId` uncast for the callee's `T*`: the
// callee's return pointer argument, a C++ temporary MSVC places in a dead argument's slot). The lvar is split
// (LVINF_SPLIT) at the function's first such store, typed as the stored value (a number as the T the code takes the
// argument's address as, `(T*)&arg`, where every such cast agrees; else an int), no wider than the argument's slot (a
// dword at least); the next round sees the result and the next store. A split Hex-Rays leaves merged is left as it
// is. Hex-Rays may start the piece at another instruction than the split's (0xCF7080's call at 0xCF70BB, split at
// 0xCF70B6), where the split's type does not reach it: a piece of an argument's slot that holds a call's hidden result
// of another type, with no type setting at its own start, gets the result's type there.
bool Unfold::reusedArguments(func_t* function, cfunc_t& cfunc)
{
	const ea_t start = function->start_ea;
	lvars_t& lvars = *cfunc.get_lvars();
	auto argument = [&](const cexpr_t* e) {
		return e->op == cot_var && lvars[e->v.idx].is_arg_var() && lvars[e->v.idx].is_stk_var() ? e->v.idx : -1;
	};
	// a piece split off a stack argument: a split lvar on an argument's slot
	auto piece = [&](const cexpr_t* e) {
		if (e->op != cot_var || !lvars[e->v.idx].is_split_var() || !lvars[e->v.idx].is_stk_var() ||
			lvars[e->v.idx].is_arg_var())
			return -1;
		sval_t offset = lvars[e->v.idx].location.stkoff();
		bool slot = std::any_of(lvars.begin(), lvars.end(), [&](const lvar_t& a) {
			return a.is_arg_var() && a.is_stk_var() && a.location.stkoff() == offset;
		});
		return slot ? e->v.idx : -1;
	};
	// the value's type reaches the argument's only through a cast that reinterprets it
	auto foreign = [&](const tinfo_t& to, const cexpr_t* value) {
		const tinfo_t& from = value->type;
		if (value->op == cot_num)
			return to.is_ptr() && value->n->_value != 0;
		if (to.is_ptr() != from.is_ptr())
			return true;
		if (!to.is_ptr())
			return to.is_floating() != from.is_floating();
		tinfo_t a = to.get_pointed_object(), b = from.get_pointed_object();
		if (a.is_void() || b.is_void())
			return false;
		if (a.is_udt() && b.is_udt())
		{
			const SrcModel::Record* x = m_types.recordOf(a);
			const SrcModel::Record* y = m_types.recordOf(b);
			if (x && y && (m_types.derivesAtZero(x->name, y->name) || m_types.derivesAtZero(y->name, x->name)))
				return false;
		}
		a.clr_const_volatile();
		b.clr_const_volatile();
		return !sameType(a, b);
	};
	struct Store
	{
		ea_t ea;
		tinfo_t type;
		bool number;
	};
	std::map<int, Store> first;
	std::map<int, std::vector<tinfo_t>> addressedAs;
	// the pieces holding a call's hidden result of another type, with that type
	std::map<int, tinfo_t> pieces;
	for (const cexpr_t* e : expressions(cfunc))
	{
		// `(T*)&arg`
		if (e->op == cot_cast && e->type.is_ptr() && e->x->op == cot_ref)
			if (int v = argument(e->x->x); v >= 0)
				addressedAs[v].push_back(e->type.get_pointed_object());
		if (e->ea == BADADDR)
			continue;
		int v = -1;
		std::optional<Store> store;
		if (e->op == cot_call)
		{
			// the slot as a call's hidden result (`isEnabled((hkBool*)&aiId, ...)`, `isEnabled(&aiId, ...)` for
			// its hkBool*): the callee builds a T there
			tinfo_t callee = e->x->type.is_ptr() ? e->x->type.get_pointed_object() : e->x->type;
			func_type_data_t fd;
			if (!callee.get_func_details(&fd))
				continue;
			for (size_t k = 0; k < fd.size() && k < e->a->size() && !store; ++k)
			{
				const cexpr_t* result = &(*e->a)[k];
				bool cast = result->op == cot_cast && result->type.is_ptr();
				const cexpr_t* address = cast ? result->x : result;
				tinfo_t built = (cast ? result->type : fd[k].type).get_pointed_object();
				if (!(fd[k].flags & FAI_RETPTR) || address->op != cot_ref || built.empty())
					continue;
				if ((v = argument(address->x)) >= 0 && !sameType(built, lvars[v].type()))
					store = Store{e->ea, built, false};
				else if (int p = piece(address->x); p >= 0 && !sameType(built, lvars[p].type()))
					pieces.emplace(p, built);
			}
		}
		else if (e->op == cot_asg && (v = argument(e->x)) >= 0)
		{
			// a number Hex-Rays may type as the argument's pointer itself (`arStream = 4`)
			const cexpr_t* value = stripCasts(e->y);
			if ((e->y->op == cot_cast || e->y->op == cot_num) && foreign(lvars[v].type(), value))
				store = Store{e->ea, value->type, value->op == cot_num};
		}
		else if (const cexpr_t* through = e->op == cot_asg && e->x->op == cot_ptr ? e->x->x : nullptr;
				 through && through->op == cot_cast && through->x->op == cot_ref &&
				 (v = argument(through->x->x)) >= 0 && !sameType(e->x->type, lvars[v].type()))
			store = Store{e->ea, e->x->type, false};
		if (store && (!first.contains(v) || store->ea < first[v].ea))
			first[v] = *store;
	}
	lvar_uservec_t saved;
	restore_user_lvar_settings(&saved, start);
	bool changed = false;
	for (auto& [v, store] : first)
	{
		const lvar_t& local = lvars[v];
		std::string name = local.name.c_str();
		if (store.number)
		{
			// the number's own type is no evidence where Hex-Rays types it as the argument's pointer: an int
			const std::vector<tinfo_t>& as = addressedAs[v];
			if (!as.empty() && std::all_of(as.begin(), as.end(), [&](const tinfo_t& t) { return sameType(t, as[0]); }))
				store.type = as[0];
			else if (store.type.is_ptr())
				store.type = tinfo_t(BT_INT32 | BTMT_SIGNED);
		}
		// a stack argument narrower than a dword has a whole dword slot (the caller pushes one)
		if (store.type.empty() || store.type.get_size() > std::max<size_t>(static_cast<size_t>(local.width), 4))
		{
			m_kept.emplace_back("reused-argument-kept", name + " (" + text(local.type()) + "): the store at " +
															hex(store.ea) + " is wider than the slot");
			continue;
		}
		lvar_saved_info_t info;
		info.ll = lvar_locator_t(local.location, store.ea);
		if (const lvar_saved_info_t* done = saved.find_info(info.ll); done && done->is_split_lvar())
		{
			m_kept.emplace_back("reused-argument-kept", name + " (" + text(local.type()) +
															"): Hex-Rays keeps it merged over the split at " +
															hex(store.ea));
			continue;
		}
		info.type = store.type;
		info.set_split_lvar();
		if (!modify_user_lvar_info(start, MLI_TYPE | MLI_SET_FLAGS, info))
		{
			m_kept.emplace_back("reused-argument-failed", name + " split at " + hex(store.ea));
			continue;
		}
		changed = true;
		log("reused-argument-split", hex(start),
			name + " (" + text(local.type()) + ") at " + hex(store.ea) + " as " + text(info.type));
	}
	for (const auto& [v, type] : pieces)
	{
		lvar_t& local = lvars[v];
		std::string name = local.name.c_str();
		lvar_saved_info_t info;
		info.ll = lvar_locator_t(local.location, local.defea);
		if (const lvar_saved_info_t* done = saved.find_info(info.ll); done && !done->type.empty())
		{
			m_kept.emplace_back("reused-argument-kept", name + " (" + text(local.type()) + "): typed at " +
															hex(local.defea) + ", a call builds " + text(type) +
															" there");
			continue;
		}
		if (type.get_size() > std::max<size_t>(static_cast<size_t>(local.width), 4) || !local.accepts_type(type))
		{
			m_kept.emplace_back("reused-argument-kept", name + " (" + text(local.type()) + "): takes no " + text(type));
			continue;
		}
		info.type = type;
		if (!modify_user_lvar_info(start, MLI_TYPE, info))
		{
			m_kept.emplace_back("reused-argument-failed", name + " typed at " + hex(local.defea));
			continue;
		}
		changed = true;
		log("reused-argument-piece-typed", hex(start),
			name + " (" + text(local.type()) + ") at " + hex(local.defea) + " as " + text(type));
	}
	return changed;
}

// Hex-Rays guesses an indirect call's stack arguments from the pushes around it and only casts the member type over
// its guess: a push meant for a later call becomes an argument of the virtual (AlchemyItem::InitItem 0x4030F0:
// GetFormID's result for Warning taken by GetFormEditorID). A call-site operand type of the vtbl member's own type
// makes it pass exactly the member's arguments. A C-variadic member's call (cdecl, `this` pushed) gets the type a
// direct variadic call's site gets (variadicSiteType): the member's parameters, then the further arguments the exe
// passes there by the stack-slot model (a double where it stores one: IDebugText::PrintLine's `%f` arguments).
bool Unfold::vtableCalls(func_t* function, cfunc_t& cfunc)
{
	struct Site
	{
		ea_t call;
		std::string cls;
		int64_t offset;
		tinfo_t type;
	};
	std::vector<Site> sites;
	for (const cexpr_t* e : expressions(cfunc))
	{
		if (e->op != cot_call)
			continue;
		const cexpr_t* target = e->x->op == cot_cast ? e->x->x : e->x;
		if (auto slot = vtableSlot(target))
			sites.push_back({e->ea, slot->first, slot->second, target->type});
	}
	bool changed = false;
	// the operand type t at the site, unless it has it already (by text: equals_to ignores parameter names, and calls
	// an enum or typedef return the same as int)
	auto type = [&](const Site& site, const tinfo_t& t, const std::string& detail, const char* typed) {
		tinfo_t old;
		if (get_op_tinfo(&old, site.call, 0) && text(old) == text(t))
			return;
		if (!set_op_tinfo(site.call, 0, &t))
		{
			m_kept.emplace_back("vtable-call-failed", detail);
			return;
		}
		m_operandTyped.insert(site.call);
		changed = true;
		log(typed, hex(function->start_ea), detail);
	};
	std::vector<const Site*> variadic;
	for (const Site& site : sites)
	{
		tinfo_t member = site.type.is_ptr() ? site.type.get_pointed_object() : site.type;
		if (member.is_func() && member.is_vararg_cc())
		{
			variadic.push_back(&site);
			continue;
		}
		std::optional<int64_t> want = slotPops(site.cls, site.offset), have = stackBytes(site.type);
		std::string detail = hex(site.call) + " " + site.cls + "+" + hex(site.offset) + " exe pops " +
							 (want ? std::to_string(*want) : "?") + ", type passes " +
							 (have ? std::to_string(*have) : "?") + ": " + text(site.type);
		if (!want)
			m_kept.emplace_back("vtable-call-implementations-unknown", detail);
		else if (!have)
			m_kept.emplace_back("vtable-call-not-thiscall-or-stdcall", detail);
		else if (*have != *want)
			m_kept.emplace_back("vtable-call-member-type-wrong", detail);
		else
			type(site, site.type, detail, "vtable-call-typed");
	}
	if (variadic.empty())
		return changed;
	std::set<ea_t> calls;
	for (const Site* site : variadic)
		calls.insert(site->call);
	std::map<ea_t, StackArguments> arguments = stackArguments(function, calls, {});
	for (const Site* site : variadic)
	{
		std::string detail = hex(site->call) + " " + site->cls + "+" + hex(site->offset) + ": " + text(site->type);
		std::pair<std::string, std::string> why;
		tinfo_t member = site->type.is_ptr() ? site->type.get_pointed_object() : site->type;
		std::optional<tinfo_t> typed = variadicSiteType(member, arguments.at(site->call), why);
		tinfo_t pointer;
		if (!typed || !pointer.create_ptr(*typed))
		{
			m_kept.emplace_back("vtable-call-" + (typed ? std::string("failed") : why.first), detail + " " + why.second);
			continue;
		}
		type(*site, pointer, detail + " as " + text(pointer), "vtable-call-variadic-typed");
	}
	return changed;
}

// Hex-Rays reads a local the code keeps at the top of the frame, where esp is at an indirect call, as one of the
// call's stack arguments (IOManager::GetPostProcessingCount 0x446DA0: `this` spilled to [ebp-4] passed on to the
// queue's GetCount), so C++ pushes a dword the exe does not. Where the exe's pushes there (the stack-slot model) are
// fewer bytes than Hex-Rays passes, the call's operand type passes the exe's bytes: the callee's own type under
// Hex-Rays' casts (a vtbl member's) where that passes them, else Hex-Rays' type without its last stack arguments. A
// callee that is a member of a function type src declares (a vtbl member, a function-pointer field) passing other
// bytes than the exe pushes contradicts src: no type, logged. A count includes the slots pending for an enclosing
// call (`f(g(), x)`: g's holds x), never fewer than the exe pushes. An operand type an earlier round wrote that this
// round's count does not give (unknown, or Hex-Rays passing fewer bytes than the exe) goes. A vtable call whose
// slot's exe implementations give its pops is vtableCalls'; for the model, those pops, else the bytes the slot's vtbl
// member passes, are the pops of the vtable calls, and an import's pops those of an import called through a register.
bool Unfold::indirectCalls(func_t* function, cfunc_t& cfunc)
{
	struct Site
	{
		ea_t call;
		tinfo_t type;  // the callee's type Hex-Rays calls with
		tinfo_t own;   // the callee's own type, under Hex-Rays' casts
		bool member;   // the callee is a member of a function type src declares (srcMember)
		int64_t passes;
	};
	std::vector<Site> sites;
	std::set<ea_t> calls;
	std::map<ea_t, int64_t> pops;
	for (const cexpr_t* e : expressions(cfunc))
	{
		if (e->op != cot_call || e->x->op == cot_helper)
			continue;
		insn_t insn;
		if (decode_insn(&insn, e->ea) <= 0 || !is_call_insn(insn) || insn.ops[0].type == o_near ||
			insn.ops[0].type == o_far)
			continue;
		const cexpr_t* own = e->x;
		while (own->op == cot_cast)
			own = own->x;
		// an import called through a register it was loaded into (`mov ebx, ds:TlsGetValue` ... `call ebx`), or a
		// function Hex-Rays resolved the register to; a global function pointer is a site like any other
		if (own->op == cot_obj)
		{
			if (std::optional<uint32_t> known = m_facts.program().importPops(static_cast<uint32_t>(own->obj_ea)))
			{
				pops.emplace(e->ea, *known);
				continue;
			}
			if (get_func(own->obj_ea))
				continue;
		}
		if (auto slot = vtableSlot(own))
		{
			if (std::optional<int64_t> known = slotPops(slot->first, slot->second))
			{
				pops.emplace(e->ea, *known);
				continue;
			}
			if (std::optional<int64_t> member = stackBytes(own->type))
				pops.emplace(e->ea, *member);
		}
		if (std::optional<int64_t> passes = passedBytes(e->x->type); passes && *passes > 0)
		{
			sites.push_back({e->ea, e->x->type, own->type, srcMember(own), *passes});
			calls.insert(e->ea);
		}
	}
	if (sites.empty())
		return false;
	std::map<ea_t, StackArguments> pushed = stackArguments(function, calls, pops);
	bool changed = false;
	for (const Site& site : sites)
	{
		const StackArguments& exe = pushed.at(site.call);
		int64_t bytes = 0;
		for (int64_t piece : exe.pieces)
			bytes += piece;
		std::string detail = hex(site.call) + " Hex-Rays passes " + std::to_string(site.passes) + ", the exe pushes " +
							 (exe.known ? std::to_string(bytes) : "?") + ": " + text(site.type);
		// Hex-Rays passing what the exe pushes, with this step's type or its own
		if (exe.known && bytes == site.passes)
			continue;
		// the type passing the exe's bytes where Hex-Rays passes more; none where Hex-Rays passes fewer
		std::optional<tinfo_t> type;
		std::optional<int64_t> ownPasses = passedBytes(site.own);
		if (!exe.known)
			m_kept.emplace_back("indirect-call-count-unknown", detail + " (" + exe.why + ")");
		else if (bytes < site.passes)
		{
			if (site.member && ownPasses && *ownPasses != bytes)
				m_kept.emplace_back("indirect-call-member-contradicted", detail + " member " + text(site.own));
			else if (ownPasses == bytes)
				type = site.own;
			else if (!(type = withStackBytes(site.type, bytes)))
				m_kept.emplace_back("indirect-call-not-cut", detail);
		}
		tinfo_t old;
		bool has = get_op_tinfo(&old, site.call, 0);
		if (!type)
		{
			// an earlier round's type this round's count does not give: Hex-Rays' guess again next round. Only a type
			// this session wrote: the database's own entries are left as they are
			if (has && m_operandTyped.erase(site.call))
			{
				del_op_tinfo(site.call, 0);
				changed = true;
				log("indirect-call-untyped", hex(function->start_ea), detail);
			}
			continue;
		}
		// by text, as vtableCalls compares
		if (has && text(old) == text(*type))
			continue;
		if (!set_op_tinfo(site.call, 0, &*type))
		{
			m_kept.emplace_back("indirect-call-failed", detail);
			continue;
		}
		m_operandTyped.insert(site.call);
		changed = true;
		log("indirect-call-typed", hex(function->start_ea), detail + " -> " + text(*type));
	}
	return changed;
}

}  // namespace idakit
