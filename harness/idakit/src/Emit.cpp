#include "EmitScopes.hpp"

#include "idakit/SrcIda.hpp"
#include "idakit/Unfold.hpp"

#include <Facts.hpp>

#include <algorithm>
#include <iterator>
#include <map>

namespace idakit
{

namespace
{

// The pre-steps, which every emit runs: types first (Self compares the exe's parameter types with src's), then Self,
// which decides what the body is (its head, registration, whether it prints at all). Then the passes in order: folded
// accessors before fields (what fields read through a folded call resolve against is its access's field) and calls
// (which leave those calls to it); allocations after calls (an allocator called through a vtable is known by its
// resolved virtual); form IDs after fields, calls and globals (a pointer field's ID goes to a resolved load
// function); structor locals, then inlined locals, after the layers they override (fields, calls, globals);
// outgoing arguments after them (a copy constructor's call is resolved, and no local claimed it); enums, float
// constants (over Types' width), the rounding conversions (over Types' cast), the variadic peel (on the operands'
// final src types), const and implicit
// conversions after the layers that give values their src types; aliases once the locals' types are final (Const
// retypes them), hidden results after them (a copy's reads
// are its original's); the EH frame, no-op calls and declarations after every layer that resolves (structor and
// inlined locals are placed already; aliases, hidden results and the EH frame need none), shadowing after
// declarations; the vector lanes after the declarations and intrinsics (a lane-written vector is declared at its
// block's top, and reads the intrinsics Hex-Rays names); rules after them; idioms
// last: they override what the layers left unresolved (dynamic casts, Rules' helper markers) or erase what they marked
// replace (NiPointer assignments, BSSimpleList loops, struct copies, containing records, adjusted null tests,
// pointer masks, structors' vtable stores);
// accessors after them, on the field reads those leave; the byte parameters' rereads once every layer that decides
// a parameter's type and meaning has run, before the conversions, which convert the reread as the parameter it is;
// the conversions where printed types disagree after
// everything that decides how a value prints; the literals to review after every layer that gives a number a meaning
// or marks an item (none is reviewed inside a marked item); the target, which reads only the exe's code, anywhere;
// the aligned locals after every layer that decides a local's type (its alignment is its src type's). Each is named
// after its function (emit's --skip-pass takes a pass, never a pre-step).
struct Pass
{
	const char* name;
	void (*run)(Function&, Context&);
};
constexpr Pass kPreSteps[] = {
	{"resolveTypes", resolveTypes},
	{"resolveSelf", resolveSelf},
};
constexpr Pass kPasses[] = {
	{"resolveFoldedAccessors", resolveFoldedAccessors},
	{"resolveFields", resolveFields},
	{"resolveCalls", resolveCalls},
	{"markAllocations", markAllocations},
	{"resolvePointerCalls", resolvePointerCalls},
	{"resolveGlobals", resolveGlobals},
	{"markFormIds", markFormIds},
	{"resolveStructorLocals", resolveStructorLocals},
	{"resolveInlinedLocals", resolveInlinedLocals},
	{"foldOutgoingArguments", foldOutgoingArguments},
	{"resolveEnums", resolveEnums},
	{"resolveFloatConstants", resolveFloatConstants},
	{"markRoundingConversions", markRoundingConversions},
	{"peelVariadicArguments", peelVariadicArguments},
	{"carryConst", carryConst},
	{"markImplicitConversions", markImplicitConversions},
	{"resolveAliases", resolveAliases},
	{"foldHiddenResults", foldHiddenResults},
	{"dropEhFrame", dropEhFrame},
	{"dropNoOpCalls", dropNoOpCalls},
	{"placeDeclarations", placeDeclarations},
	{"markShadowing", markShadowing},
	{"resolveIntrinsics", resolveIntrinsics},
	{"resolveVectorLanes", resolveVectorLanes},
	{"checkRules", checkRules},
	{"resolveDynamicCasts", resolveDynamicCasts},
	{"rewriteThreadData", rewriteThreadData},
	{"rewriteNiPointerAssignments", rewriteNiPointerAssignments},
	{"rewriteSimpleListLoops", rewriteSimpleListLoops},
	{"rewriteStructCopies", rewriteStructCopies},
	{"rewriteContainingRecords", rewriteContainingRecords},
	{"rewriteAdjustedNullTests", rewriteAdjustedNullTests},
	{"rewritePartHelpers", rewritePartHelpers},
	{"rewritePointerMasks", rewritePointerMasks},
	{"rewriteVtableStores", rewriteVtableStores},
	{"spellAccessors", spellAccessors},
	{"rereadByteParameters", rereadByteParameters},
	{"convertValues", convertValues},
	{"pinOverloads", pinOverloads},
	{"flagLiterals", flagLiterals},
	{"alignFrameLocals", alignFrameLocals},
};

// Why the function is not src: its blockers, every item a layer could not resolve or converted explicitly where its
// printed type disagrees (in ctree order), and every local whose type is unresolved or that Hex-Rays reports possibly
// undefined; each reason once. Empty: src. A literal to review is a note, no reason (user, 2026-11-09).
std::vector<std::string> reasons(const Function& function)
{
	std::vector<std::string> out;
	auto add = [&](const std::string& reason) {
		if (std::find(out.begin(), out.end(), reason) == out.end())
			out.push_back(reason);
	};
	for (const std::string& blocker : function.blockers)
		add(blocker);
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
		if (auto conversion = function.conversions.find(e);
			conversion != function.conversions.end() && !conversion->second.what.empty())
			add(conversion->second.what);
		auto found = function.meaning.find(e);
		if (found == function.meaning.end())
			return;
		if (const Missing* missing = std::get_if<Missing>(&found->second))
			add(missing->what);
		else if (const GlobalAs* global = std::get_if<GlobalAs>(&found->second))
			add(global->what);
		else if (const Call* call = std::get_if<Call>(&found->second); call && !call->unpinned.empty())
			add(call->unpinned);
	});
	forEachStatement(&function.cfunc->body, [&](const cinsn_t* s) {
		auto found = function.meaning.find(s);
		if (found != function.meaning.end())
			if (const Missing* missing = std::get_if<Missing>(&found->second))
				add(missing->what);
	});
	const lvars_t& lvars = *function.cfunc->get_lvars();
	for (size_t k = 0; k < function.locals.size(); ++k)
	{
		const Local& local = function.locals[k];
		bool parameter = lvars[k].is_arg_var();
		if (!parameter && !local.declaredAt)
			continue;
		// raw storage is never clean code: marked whatever type it is declared with
		if (local.type.empty() || local.storage)
			add("local " + local.name + ": " + local.why);
		else if (!parameter && local.undefined)
			add("local " + local.name + " is possibly undefined: an uninitialised read");
	}
	return out;
}

// Hex-Rays prints a call a user-defined call types (Unfold's variadic sites and fold owners) as a helper call under
// the user-defined call's name: each such callee becomes the call instruction's target again (CodeRefsFrom), an obj of
// the user-defined call's type, so that every pass reads it as the direct call it is; the src functions the name and
// type stand for (SrcPush::calledAs: a fold owner) are what Calls resolves the call to.
void restoreCallees(Function& function, Unfold& unfold)
{
	udcall_map_t udcalls;
	if (!restore_user_defined_calls(&udcalls, function.ea) || udcalls.empty())
		return;
	struct Callees : ctree_visitor_t
	{
		Callees(Function& function, Unfold& unfold, const udcall_map_t& udcalls)
			: ctree_visitor_t(CV_FAST), function(function), unfold(unfold), udcalls(udcalls)
		{
		}
		int idaapi visit_expr(cexpr_t* e) override
		{
			// the helper Hex-Rays names after the user-defined call, not one of its intrinsics at the call's address
			auto udcall = e->op == cot_call && e->x->op == cot_helper ? udcalls.find(e->ea) : udcalls.end();
			if (udcall == udcalls.end() || std::string_view(e->x->helper) != udcall->second.name.c_str())
				return 0;
			ea_t target = get_first_fcref_from(e->ea);
			if (target == BADADDR)
				return 0;
			cexpr_t* callee = e->x;
			ea_t at = callee->ea;
			callee->cleanup();
			callee->op = cot_obj;
			callee->obj_ea = target;
			callee->ea = at;
			callee->type.create_ptr(udcall->second.tif);
			if (std::vector<const SrcModel::Function*> named =
					unfold.calledAs(target, udcall->second.name.c_str(), udcall->second.tif);
				!named.empty())
				function.namedCallees[e] = std::move(named);
			return 0;
		}
		Function& function;
		Unfold& unfold;
		const udcall_map_t& udcalls;
	} callees(function, unfold, udcalls);
	callees.apply_to(&function.cfunc->body, nullptr);
}

// the local an lvalue is or is a part of (`v`, `v.m`, `v[2]` of an array local, `LOBYTE(v)`, through casts); -1 for any
// other (`*p`, `p->m`, `p[2]`)
int writtenLocal(const cexpr_t* e)
{
	while (e->op == cot_memref || e->op == cot_cast || (e->op == cot_idx && e->x->type.is_array()) ||
		   (e->op == cot_call && e->x->op == cot_helper && e->a->size() == 1))
		e = e->op == cot_call ? &(*e->a)[0] : e->x;
	return lvarIndex(e);
}

// the locals an expression writes on every evaluation (an assignment to the local or a part of it, an increment, its
// address taken; not under `&&`'s or `||`'s right operand, nor a `?:`'s branches)
void definiteWrites(const cexpr_t* e, std::set<int>& out)
{
	if (written(e) || e->op == cot_ref)
		if (int v = writtenLocal(e->x); v >= 0)
			out.insert(v);
	if (e->op == cot_call)
	{
		definiteWrites(e->x, out);
		for (const carg_t& argument : *e->a)
			definiteWrites(&argument, out);
		return;
	}
	bool branches = e->op == cot_land || e->op == cot_lor || e->op == cot_tern;
	if (op_uses_x(e->op) && e->x)
		definiteWrites(e->x, out);
	if (!branches && op_uses_y(e->op) && e->y)
		definiteWrites(e->y, out);
	if (!branches && op_uses_z(e->op) && e->z)
		definiteWrites(e->z, out);
}

// The locals written on every path to each return (definiteWrites'): a walk of the structured statements, a loop's
// body and a switch's cases taken as possibly skipped (but a loop with no condition or a nonzero one is left only by
// its breaks, and a for loop whose init's number its condition holds for runs its body at least once), a loop left
// where its condition fails before the first run or after a run, or by a break, each branch of an if or a loop taken
// with its condition's outcome (the right operand of `a || b` ran where it is false, of `a && b` where it is true),
// each goto's locals met at its label, repeated until the labels' settle.
class Assignments
{
public:
	explicit Assignments(const cinsn_t* body) : m_body(body) {}

	// each local a return reads that some path to the return leaves unwritten
	std::set<int> unwrittenReturns()
	{
		do
		{
			m_labels = std::move(m_gotos);
			m_gotos.clear();
			m_unwritten.clear();
			walk(m_body, State{});
		} while (m_gotos != m_labels);
		return m_unwritten;
	}

private:
	// the locals written on every path to a point; unreachable: no path (every local, the meet's identity)
	struct State
	{
		bool unreachable = false;
		std::set<int> locals;
		bool operator==(const State&) const = default;
	};

	static State unreachable() { return State{true, {}}; }

	static State meet(const State& a, const State& b)
	{
		if (a.unreachable)
			return b;
		if (b.unreachable)
			return a;
		State out;
		std::set_intersection(a.locals.begin(), a.locals.end(), b.locals.begin(), b.locals.end(),
							  std::inserter(out.locals, out.locals.end()));
		return out;
	}

	static State after(State in, const cexpr_t* e)
	{
		if (!in.unreachable && e && e->op != cot_empty)
			definiteWrites(e, in.locals);
		return in;
	}

	// the state after condition e evaluated to value
	static State decided(State in, const cexpr_t* e, bool value)
	{
		if (in.unreachable || !e)
			return in;
		switch (e->op)
		{
		case cot_lnot:
			return decided(in, e->x, !value);
		case cot_comma:
			return decided(after(in, e->x), e->y, value);
		case cot_land:
		case cot_lor:
		{
			// `a && b` true and `a || b` false ran both with that outcome; else a's outcome decided it, or b's did
			bool decisive = e->op == cot_lor;
			if (value != decisive)
				return decided(decided(in, e->x, value), e->y, value);
			return meet(decided(in, e->x, decisive), decided(decided(in, e->x, !decisive), e->y, decisive));
		}
		default:
			return after(in, e);
		}
	}

	// the loop's condition is absent or a nonzero number: it is left only by a break (or a return or goto)
	static bool endless(const cexpr_t* condition)
	{
		return !condition || condition->op == cot_empty || (condition->op == cot_num && condition->numval() != 0);
	}

	// a number's value as its type reads it (sign-extended where the type is signed)
	static int64_t value(const cexpr_t* e)
	{
		uint64_t raw = e->numval();
		size_t size = e->type.get_size();
		if (e->type.is_signed() && size > 0 && size < 8 && (raw >> (size * 8 - 1) & 1))
			raw |= ~uint64_t(0) << (size * 8);
		return static_cast<int64_t>(raw);
	}

	// a for loop's init assigns its local a number that its condition, a comparison of that local with a number,
	// holds for (`for (i = 1000; i >= 1; ...)`, Hex-Rays' form of a do-while whose first test MSVC left out): the
	// body runs at least once
	static bool entered(const cexpr_t* init, const cexpr_t* condition)
	{
		if (!init || init->op != cot_asg || init->y->op != cot_num || !condition)
			return false;
		int v = lvarIndex(init->x);
		if (v < 0 || condition->op < cot_eq || condition->op > cot_ult || condition->y->op != cot_num ||
			lvarIndex(stripCasts(condition->x)) != v)
			return false;
		int64_t a = value(init->y), b = value(condition->y);
		uint64_t ua = init->y->numval(), ub = condition->y->numval();
		switch (condition->op)
		{
		case cot_eq: return a == b;
		case cot_ne: return a != b;
		case cot_sge: return a >= b;
		case cot_sle: return a <= b;
		case cot_sgt: return a > b;
		case cot_slt: return a < b;
		case cot_uge: return ua >= ub;
		case cot_ule: return ua <= ub;
		case cot_ugt: return ua > ub;
		case cot_ult: return ua < ub;
		default: return false;
		}
	}

	State walk(const cinsn_t* s, State in)
	{
		if (s->label_num >= 0)
		{
			auto label = m_labels.find(s->label_num);
			// the gotos' locals of the last round; a label none reached then is taken as reached with every local
			// written (the walk starts there and narrows)
			in = meet(in, label == m_labels.end() ? unreachable() : label->second);
		}
		switch (s->op)
		{
		case cit_block:
			for (const cinsn_t& item : *s->cblock)
				in = walk(&item, in);
			return in;
		case cit_expr:
			return after(in, s->cexpr);
		case cit_if:
		{
			State then = walk(s->cif->ithen, decided(in, &s->cif->expr, true));
			State otherwise = decided(in, &s->cif->expr, false);
			return meet(then, s->cif->ielse ? walk(s->cif->ielse, otherwise) : otherwise);
		}
		case cit_for:
		{
			State init = after(in, &s->cfor->init);
			return loop(s->cfor->body, &s->cfor->expr, &s->cfor->step, decided(init, &s->cfor->expr, true),
						entered(&s->cfor->init, &s->cfor->expr) ? unreachable() : init);
		}
		case cit_while:
			return loop(s->cwhile->body, &s->cwhile->expr, nullptr, decided(in, &s->cwhile->expr, true), in);
		case cit_do:
		{
			m_breaks.push_back(unreachable());
			m_continues.push_back(unreachable());
			State body = walk(s->cdo->body, in);
			State condition = decided(meet(body, m_continues.back()), &s->cdo->expr, false);
			State out = meet(endless(&s->cdo->expr) ? unreachable() : condition, m_breaks.back());
			m_breaks.pop_back();
			m_continues.pop_back();
			return out;
		}
		case cit_switch:
		{
			State value = after(in, &s->cswitch->expr);
			m_breaks.push_back(unreachable());
			State fallthrough = unreachable();
			bool all = false;
			for (const ccase_t& c : s->cswitch->cases)
			{
				all |= c.values.empty();
				fallthrough = walk(&c, meet(value, fallthrough));
			}
			State out = meet(meet(fallthrough, m_breaks.back()), all ? unreachable() : value);
			m_breaks.pop_back();
			return out;
		}
		case cit_break:
			if (!m_breaks.empty())
				m_breaks.back() = meet(m_breaks.back(), in);
			return unreachable();
		case cit_continue:
			if (!m_continues.empty())
				m_continues.back() = meet(m_continues.back(), in);
			return unreachable();
		case cit_return:
		{
			int v = lvarIndex(&s->creturn->expr);
			if (v >= 0 && !in.unreachable && !in.locals.count(v))
				m_unwritten.insert(v);
			return unreachable();
		}
		case cit_goto:
		{
			auto [found, inserted] = m_gotos.emplace(s->cgoto->label_num, in);
			if (!inserted)
				found->second = meet(found->second, in);
			return unreachable();
		}
		default:
			return in;
		}
	}

	// a loop whose body runs from the state after its condition held: left by a break, or (unless endless) where the
	// condition fails, before the first run (from `before`; unreachable where the loop is entered) or after a run and
	// the for loop's step
	State loop(const cinsn_t* body, const cexpr_t* condition, const cexpr_t* step, const State& held,
			   const State& before)
	{
		m_breaks.push_back(unreachable());
		m_continues.push_back(unreachable());
		State ran = after(meet(walk(body, held), m_continues.back()), step);
		State out = m_breaks.back();
		if (!endless(condition))
			out = meet(out, meet(decided(before, condition, false), decided(ran, condition, false)));
		m_breaks.pop_back();
		m_continues.pop_back();
		return out;
	}

	const cinsn_t* m_body;
	std::map<int, State> m_labels, m_gotos;  // each label's locals: the last round's, this round's gotos'
	std::vector<State> m_breaks, m_continues;
	std::set<int> m_unwritten;
};

// an address of a local (`&v`, or an array decaying) that reaches no writer: stored, through casts, by a statement
// into a local the body never reads (Hex-Rays' `v4 = v2` of MSVC's `mov [ebp-N], esp` after the outgoing slot's
// `push ecx`), or passed, through casts, to a call whose value nothing uses of an empty function or of a trivial
// accessor that stores nothing (exefacts'). MSVC passes an empty allocator by value so: `push ecx; mov ecx, esp; call
// 0x6815C0` (its constructor, `return this`) leaves the dword it pushed in the slot, which Hex-Rays reads as a local
// nothing writes (0x657960's `_sub_65D3A0(this, v2[0])`).
bool addressGoesNowhere(Context& context, const std::unordered_map<const cexpr_t*, const citem_t*>& parents,
						const std::set<int>& read, const cexpr_t* address)
{
	const cexpr_t* value = address;
	const citem_t* user = nullptr;
	for (;;)
	{
		auto up = parents.find(value);
		if (up == parents.end() || !up->second->is_expr())
			return false;
		user = up->second;
		if (user->op != cot_cast)
			break;
		value = static_cast<const cexpr_t*>(user);
	}
	const cexpr_t* use = static_cast<const cexpr_t*>(user);
	// the assignment or the call is a statement of its own: its value goes nowhere either
	auto statement = parents.find(use);
	if (statement == parents.end() || statement->second->is_expr())
		return false;
	if (use->op == cot_asg)
	{
		int target = lvarIndex(use->x);
		return use->y == value && target >= 0 && !read.count(target);
	}
	if (use->op != cot_call || use->x == value || use->x->op != cot_obj)
		return false;
	uint32_t callee = static_cast<uint32_t>(use->x->obj_ea);
	if (context.exe.emptyFunction(callee))
		return true;
	std::optional<exefacts::Accessor> fact = context.exe.trivialAccessor(callee);
	return fact && !fact->store;
}

// an address of a local passed, through casts, as an argument the callee takes as a pointer or reference to const:
// by src's declaration of a direct callee (`this` of a const member, whose const IDA's prototype drops, or a
// parameter, where the call passes exactly src's parameters), else by the call's type (IDA's prototype). The callee
// reads the local and writes nothing (0x8F2610's `NiPoint3::Length(&ftargdistance)` of a NiPoint3 the exe never
// writes).
bool addressOnlyRead(Context& context, const std::unordered_map<const cexpr_t*, const citem_t*>& parents,
					 const cexpr_t* address)
{
	const cexpr_t* value = address;
	for (;;)
	{
		auto up = parents.find(value);
		if (up == parents.end() || !up->second->is_expr())
			return false;
		if (up->second->op != cot_cast)
			break;
		value = static_cast<const cexpr_t*>(up->second);
	}
	const cexpr_t* call = static_cast<const cexpr_t*>(parents.at(value));
	if (call->op != cot_call || call->x == value)
		return false;
	size_t index = 0;
	while (index < call->a->size() && &(*call->a)[index] != value)
		++index;
	if (index == call->a->size())
		return false;
	if (call->x->op == cot_obj)
		if (const SrcModel::Ref* ref = chooseRef(context.model, call->x->obj_ea).ref)
			if (const SrcModel::Function* f = context.model.function(ref->function))
				if (const SrcModel::Type* t = context.model.type(f->type))
				{
					const bool member = !f->cls.empty() && !f->isStatic;
					if (call->a->size() != t->params.size() + (member ? 1 : 0))
						return false;
					if (member && index == 0)
						return t->isConst;
					const std::string& param = t->params[index - (member ? 1 : 0)];
					return param.starts_with("ptr(const(") || param.starts_with("lref(const(");
				}
	tinfo_t type = call->x->type;
	if (type.is_ptr())
		type = type.get_pointed_object();
	func_type_data_t fd;
	if (!type.get_func_details(&fd) || index >= fd.size())
		return false;
	return fd[index].type.is_ptr() && fd[index].type.get_pointed_object().is_const();
}

// the locals Hex-Rays reports possibly undefined (its warning names the lvar: "variable 'v3' is possibly undefined"),
// and those it does not report: a local (not a parameter) the body reads that nothing writes, and a local a return
// reads that some path to the return leaves unwritten. A local is written by an assignment to it or a part of it
// (`v.m`, `LOBYTE(v)`), an increment, its address taken, or, an array, by being used as a value at all (passed, cast,
// in pointer arithmetic: its address, through which a callee or a store writes it; only `a[i]` and `sizeof(a)` are
// not); a stack local at or above the return address is the return address (or what the caller pushed), which the
// call writes. Neither rule is applied to a body with inline asm, which may write any local. An address that reaches
// no writer writes nothing (addressGoesNowhere), nor does one a callee takes as a pointer to const (addressOnlyRead).
void markUndefined(Function& function, Context& context)
{
	const lvars_t& lvars = *function.cfunc->get_lvars();
	for (const hexwarn_t& warning : function.cfunc->get_warnings())
	{
		if (warning.id != WARN_UNDEF_LVAR)
			continue;
		std::string text = warning.text.c_str();
		size_t open = text.find('\''), close = text.find('\'', open + 1);
		if (close == std::string::npos)
			continue;
		std::string name = text.substr(open + 1, close - open - 1);
		for (size_t k = 0; k < lvars.size(); ++k)
			if (lvars[k].name.c_str() == name)
				function.locals[k].undefined = true;
	}
	bool inlineAsm = false;
	forEachStatement(&function.cfunc->body, [&](const cinsn_t* s) { inlineAsm |= s->op == cit_asm; });
	if (inlineAsm)
		return;
	std::set<int> written, read;
	std::map<int, int> used;  // each local's occurrences
	std::set<const cexpr_t*> notDecayed;  // the array locals `a[i]` and `sizeof(a)` name
	std::vector<std::pair<int, const cexpr_t*>> addresses;  // each local's address taken, and the address
	std::unordered_map<const cexpr_t*, const citem_t*> parents = parentsOf(function);
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
		if (int v = lvarIndex(e); v >= 0)
		{
			++used[v];
			// read unless it is the left side of a plain assignment
			auto up = parents.find(e);
			if (up == parents.end() || !up->second->is_expr() || up->second->op != cot_asg ||
				static_cast<const cexpr_t*>(up->second)->x != e)
				read.insert(v);
		}
		if (idakit::written(e))
			if (int v = writtenLocal(e->x); v >= 0)
				written.insert(v);
		if (e->op == cot_ref)
			if (int v = writtenLocal(e->x); v >= 0)
				addresses.emplace_back(v, e);
		// parents first: an idx or sizeof is seen before its operand
		if (e->op == cot_idx || e->op == cot_sizeof)
			notDecayed.insert(e->x);
		if (int v = lvarIndex(e); v >= 0 && lvars[v].type().is_array() && !notDecayed.count(e))
			addresses.emplace_back(v, e);
	});
	// an address that reaches no writer writes nothing, nor reads the local; one only read through reads it
	for (const auto& [v, address] : addresses)
		if (addressGoesNowhere(context, parents, read, address))
			--used[v];
		else if (!addressOnlyRead(context, parents, address))
			written.insert(v);
	std::set<int> undefined = Assignments(&function.cfunc->body).unwrittenReturns();
	for (const auto& [v, count] : used)
		if (count > 0 && !written.count(v))
			undefined.insert(v);
	// the return address's own slot, which the call writes (Hex-Rays' `retaddr`), is no uninitialised read; a slot
	// past it that is no parameter is an argument the exe reads and src's declaration does not pass
	func_t* pfn = get_func(function.ea);
	auto returnAddress = [&](const lvar_t& lvar) {
		if (!pfn || !lvar.is_stk_var())
			return false;
		sval_t offset = function.cfunc->mba->stkoff_vd2ida(lvar.get_stkoff());
		return offset >= static_cast<sval_t>(frame_off_retaddr(pfn)) &&
			   offset < static_cast<sval_t>(frame_off_args(pfn));
	};
	for (int v : undefined)
		if (!lvars[v].is_arg_var() && !returnAddress(lvars[v]))
			function.locals[v].undefined = true;
}

// a stack local (not a parameter) the body assigns and never reads, nor takes the address of, that lies inside the
// object a call is passed the address of: `&v` (or an array v decaying) passed to a parameter whose pointee is larger
// than v, or with a constant argument larger than v (the extent the exe states, as Unfold's passed objects read it),
// the local within that extent past v's own bytes. The exe's store is for the callee, which reads the object past the
// type Hex-Rays gave v (0xD96970's hkJob passed as a hkJobQueue::JobQueueEntry, whose hkpSplitSimulationIslandJob
// fields Hex-Rays reads as four locals of their own). C++ drops a store nothing reads, so each such assignment is
// marked, until the database's local types join the two. Not applied to a body with inline asm, which may read any
// local.
void markUnreadStores(Function& function)
{
	bool inlineAsm = false;
	forEachStatement(&function.cfunc->body, [&](const cinsn_t* s) { inlineAsm |= s->op == cit_asm; });
	func_t* pfn = get_func(function.ea);
	if (inlineAsm || pfn == nullptr)
		return;
	const lvars_t& lvars = *function.cfunc->get_lvars();
	std::unordered_map<const cexpr_t*, const citem_t*> parents = parentsOf(function);
	std::set<int> read, taken;
	std::map<int, std::vector<const cexpr_t*>> stores;  // each local's plain assignments
	std::set<const cexpr_t*> notDecayed;
	std::vector<const cexpr_t*> calls;
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
		if (int v = lvarIndex(e); v >= 0)
		{
			auto up = parents.find(e);
			if (up != parents.end() && up->second->is_expr() && up->second->op == cot_asg &&
				static_cast<const cexpr_t*>(up->second)->x == e)
				stores[v].push_back(static_cast<const cexpr_t*>(up->second));
			else
				read.insert(v);
		}
		if (e->op == cot_ref)
			if (int v = writtenLocal(e->x); v >= 0)
				taken.insert(v);
		if (e->op == cot_idx || e->op == cot_sizeof)
			notDecayed.insert(e->x);
		if (int v = lvarIndex(e); v >= 0 && lvars[v].type().is_array() && !notDecayed.count(e))
			taken.insert(v);
		if (e->op == cot_call)
			calls.push_back(e);
	});
	// the frame's locals by offset (below the return address: above it are the arguments)
	const sval_t retaddr = static_cast<sval_t>(frame_off_retaddr(pfn));
	auto offsetOf = [&](int k) { return function.cfunc->mba->stkoff_vd2ida(lvars[k].get_stkoff()); };
	auto local = [&](int k) {
		return k >= 0 && lvars[k].is_stk_var() && !lvars[k].is_arg_var() && offsetOf(k) < retaddr;
	};
	std::map<sval_t, std::vector<int>> byOffset;
	for (size_t k = 0; k < lvars.size(); ++k)
		if (local(static_cast<int>(k)))
			byOffset[offsetOf(static_cast<int>(k))].push_back(static_cast<int>(k));
	for (const cexpr_t* e : calls)
	{
		tinfo_t type = e->x->type;
		if (type.is_ptr())
			type = type.get_pointed_object();
		func_type_data_t fd;
		if (!type.get_func_details(&fd))
			continue;
		// the call's largest constant argument that fits the frame's locals
		uint64_t constant = 0;
		for (const carg_t& a : *e->a)
			if (const cexpr_t* n = stripCasts(&a); n->op == cot_num && n->numval() <= pfn->frsize)
				constant = std::max<uint64_t>(constant, n->numval());
		for (size_t a = 0; a < e->a->size(); ++a)
		{
			const cexpr_t* arg = stripCasts(&(*e->a)[a]);
			int v = arg->op == cot_ref && arg->x->op == cot_var ? arg->x->v.idx
					: arg->op == cot_var && lvars[arg->v.idx].type().is_array() ? arg->v.idx
																				 : -1;
			if (!local(v))
				continue;
			uint64_t extent = constant;
			if (a < fd.size() && fd[a].type.is_ptr())
				if (size_t size = fd[a].type.get_pointed_object().get_size(); size != BADSIZE)
					extent = std::max<uint64_t>(extent, size);
			const sval_t start = offsetOf(v), own = static_cast<sval_t>(lvars[v].width);
			if (extent <= static_cast<uint64_t>(own))
				continue;
			const sval_t end = start + static_cast<sval_t>(extent);
			for (auto at = byOffset.upper_bound(start); at != byOffset.end() && at->first < end; ++at)
				for (int w : at->second)
					if (at->first >= start + own && stores.count(w) &&
						!read.count(w) && !taken.count(w))
						for (const cexpr_t* store : stores[w])
							function.meaning[store] =
								Missing{"a store the body never reads, inside the " + std::to_string(extent) +
										" bytes at " + lvars[v].name.c_str() + " the call at " + hex(e->ea) +
										" is passed: the callee may read it as part of that object (the database's "
										"local types should join them)"};
		}
	}
}

// the overlay's default (kNotResolved) on every item that prints raw unless a pass gives it a meaning: each obj but a
// call's callee (the call prints it, or not at all), each call, each member access, and each item the printer has no
// exact form for, which Rules or Intrinsics judge (a Hex-Rays helper used as a value, printed as its name; a sizeof or
// type expression, printed with IDA's type; a Hex-Rays string constant). An item of a kind the printer has no form
// for at all (a statement inside an expression, `cot_insn`, printed `0`; inline asm, a try block, a throw, printed
// `;`) is Missing too, whatever passes run: no pass gives one a meaning.
void markUnlooked(Function& function)
{
	std::set<const cexpr_t*> callees;
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
		if (e->op == cot_call)
			callees.insert(e->x);
	});
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
		bool raw = e->op == cot_call || e->op == cot_memptr || e->op == cot_memref ||
				   ((e->op == cot_obj || e->op == cot_helper) && !callees.count(e)) || e->op == cot_sizeof ||
				   e->op == cot_type || e->op == cot_str;
		if (raw)
			function.meaning[e] = Missing{kNotResolved};
		else if (e->op == cot_insn)
			function.meaning[e] = Missing{"statement inside an expression: no printed form"};
	});
	// the statements the printer prints (EmitPrinter.cpp's statement()); an empty one prints `;`, which it is
	forEachStatement(&function.cfunc->body, [&](const cinsn_t* s) {
		switch (s->op)
		{
		case cit_empty: case cit_block: case cit_expr: case cit_if: case cit_for: case cit_while: case cit_do:
		case cit_switch: case cit_break: case cit_continue: case cit_return: case cit_goto:
			break;
		case cit_asm:
			function.meaning[s] = Missing{"inline asm: no printed form"};
			break;
		case cit_try:
			function.meaning[s] = Missing{"C++ try block: no printed form"};
			break;
		case cit_throw:
			function.meaning[s] = Missing{"C++ throw: no printed form"};
			break;
		default:
			function.meaning[s] =
				Missing{"Hex-Rays statement kind " + std::to_string(int(s->op)) + ": no printed form"};
			break;
		}
	});
}

// Every local (not a parameter) the body uses is declared at the top of the body, Hex-Rays' own form: the default
// placeDeclarations (and the structor and inlined locals before it) refine, so that with those passes off each local
// the body prints is still declared.
void declareAtTop(Function& function)
{
	Scopes scopes(function);
	const lvars_t& lvars = *function.cfunc->get_lvars();
	for (size_t k = 0; k < lvars.size(); ++k)
		if (!lvars[k].is_arg_var() && !scopes.uses(static_cast<int>(k)).empty())
			function.locals[k].declaredAt = scopes.body();
}

}  // namespace

std::set<const cinsn_t*> Function::copies() const
{
	std::set<const cinsn_t*> out;
	for (const Local& local : locals)
		if (local.copiedAt)
			out.insert(local.copiedAt);
	return out;
}

std::set<const cinsn_t*> Function::dropped() const
{
	std::set<const cinsn_t*> out;
	for (const auto& [item, m] : meaning)
		if (const Rewrite* rewrite = std::get_if<Rewrite>(&m))
			out.insert(rewrite->dropped.begin(), rewrite->dropped.end());
	return out;
}

std::vector<std::string> passNames()
{
	std::vector<std::string> names;
	for (const Pass& pass : kPasses)
		names.push_back(pass.name);
	return names;
}

std::vector<std::string> preStepNames()
{
	std::vector<std::string> names;
	for (const Pass& step : kPreSteps)
		names.push_back(step.name);
	return names;
}

bool analyze(ea_t ea, Context& context, Unfold& unfold, Function& function, std::string& why,
			 const std::set<std::string>& skipped)
{
	func_t* pfn = get_func(ea);
	if (pfn == nullptr || pfn->start_ea != ea)
	{
		why = "not a function start";
		return false;
	}
	function.ea = ea;
	std::string failed;
	function.cfunc = unfold.rebuild(pfn, failed);
	if (function.cfunc == nullptr)
	{
		why = "decompile: " + failed;
		return false;
	}
	function.locals.resize(function.cfunc->get_lvars()->size());
	restoreCallees(function, unfold);
	markUndefined(function, context);
	markUnreadStores(function);
	markUnlooked(function);
	declareAtTop(function);
	for (const Pass& step : kPreSteps)
		step.run(function, context);
	for (const Pass& pass : kPasses)
		if (!skipped.count(pass.name))
			pass.run(function, context);
	return true;
}

Emitted emit(ea_t ea, Context& context, Unfold& unfold, const std::set<std::string>& skipped, cfuncptr_t* decompiled)
{
	Emitted emitted;
	qstring name;
	get_func_name(&name, ea);
	emitted.name = name.c_str();
	Function function;
	std::string why;
	bool analyzed = analyze(ea, context, unfold, function, why, skipped);
	if (decompiled)
		*decompiled = function.cfunc;
	if (!analyzed)
	{
		emitted.blockers.push_back(why);
		return emitted;
	}
	if (!function.registered.empty())
	{
		emitted.blockers.push_back("already in src (registered at " + function.registered + ")");
		return emitted;
	}
	emitted.blockers = reasons(function);
	// only a member definition is printed: a function Self did not match has no place in src
	if (!function.defines)
		return emitted;
	emitted.form = emitted.blockers.empty() ? Emitted::Form::src : Emitted::Form::marked;
	emitted.replaces = function.replaces;
	emitted.source = context.model.classSource(function.self->cls);
	emitted.text = print(function, context, emitted.form, emitted.spots);
	return emitted;
}

}  // namespace idakit
