// Rules: what this milestone's src form does not take. Hex-Rays helpers (but the msvc9 math calls resolveCalls and
// the compiler intrinsics resolveIntrinsics gave a meaning, and the inlined memset, memcpy and strlen resolveCalls made
// msvc9 calls) and its other non-C++ constructs (they would need
// EXE-INLINED / EXE-CAST markers, .omp/RULES.md), inline asm, and an exception frame with a handler (a cleanup-only
// C++ frame may be left out of a body; RULES.md), an SEH frame marked at its try-level stores. A helper's call
// carries the verdict (it prints the call's marker); no other layer judges helpers. A sizeof of the very object its
// call takes by address passes (sizeOfArgument).

#include "EmitPasses.hpp"
#include "idakit/Address.hpp"

#include <Facts.hpp>

#include <set>

namespace idakit
{

// a sizeof of an object (not a type) that the call it is an argument of also takes, as itself, its address or its
// array decayed (`memset(v7, 0, sizeof(v7))`): the whole object's size on any layout, plain C++. A local or member
// array is known by its lvar's or IDA member's type: Hex-Rays types the argument expression as the parameter it
// decays to.
static bool sizeOfArgument(const cexpr_t* e, const std::unordered_map<const cexpr_t*, const citem_t*>& parents,
						   const lvars_t& lvars)
{
	auto parent = parents.find(e);
	while (parent != parents.end() && parent->second->op == cot_cast)
		parent = parents.find(static_cast<const cexpr_t*>(parent->second));
	if (e->x->op == cot_type || parent == parents.end() || parent->second->op != cot_call)
		return false;
	auto strip = [](const cexpr_t* a) {
		while (a->op == cot_cast || a->op == cot_ref)
			a = a->x;
		return a;
	};
	// the call takes the object itself: its address, or an array decaying to it (a pointer's sizeof is no size of
	// what it points to)
	const cexpr_t* object = strip(e->x);
	for (const carg_t& argument : *static_cast<const cexpr_t*>(parent->second)->a)
	{
		const cexpr_t* a = &argument;
		while (a->op == cot_cast)
			a = a->x;
		bool address = a->op == cot_ref;
		a = strip(a);
		bool array = a->type.is_array() || (a->op == cot_var && lvars[a->v.idx].type().is_array()) ||
					 ((a->op == cot_memref || a->op == cot_memptr) && memberType(a).is_array());
		if (a != object && (address || array) &&
			(a->op == cot_var && object->op == cot_var ? a->v.idx == object->v.idx : a->equal_effect(*object)))
			return true;
	}
	return false;
}

// a sizeof of a src field is src's size of its type, which the exe's must equal (an x86 layout of the field's type)
static bool sizeAgrees(const Function& function, const Context& context, const cexpr_t* e)
{
	const cexpr_t* object = e->x;
	while (object->op == cot_cast || object->op == cot_ref)
		object = object->x;
	if (object->op != cot_memref && object->op != cot_memptr)
		return true;
	auto found = function.meaning.find(object);
	const Member* member = found == function.meaning.end() ? nullptr : std::get_if<Member>(&found->second);
	if (!member || !member->field)
		return false;
	std::optional<int64_t> size = context.model.sizeOf(member->field->type);
	return size && *size == static_cast<int64_t>(memberType(object).get_size());
}

// a sizeof of a type that is the byte count, or a factor of it (`sizeof(T) * n`), of an inlined memset or memcpy
// (Calls' msvc9 call under EXE-INLINED) whose destination points to that type, and for memcpy whose source points to
// it or to a class deriving from it at offset 0 (`sGeometry = *pLod`'s bytes, an SLeaf of a derived LOD): the whole
// destination's size on any layout (user, 2026-11-12: the count prints as the destination's sizeof where the sizes
// agree in src's types), src's size of the type equal to the exe's
static bool sizeOfDestination(const Function& function, Context& context, const cexpr_t* e,
							  const std::unordered_map<const cexpr_t*, const citem_t*>& parents)
{
	if (e->x->op != cot_type)
		return false;
	auto up = [&](const cexpr_t* x) {
		auto parent = parents.find(x);
		while (parent != parents.end() && parent->second->op == cot_cast)
			parent = parents.find(static_cast<const cexpr_t*>(parent->second));
		return parent == parents.end() ? nullptr : parent->second;
	};
	const citem_t* parent = up(e);
	if (parent && parent->op == cot_mul)
		parent = up(static_cast<const cexpr_t*>(parent));
	auto found = parent && parent->op == cot_call ? function.meaning.find(static_cast<const cexpr_t*>(parent))
												  : function.meaning.end();
	const Call* call = found == function.meaning.end() ? nullptr : std::get_if<Call>(&found->second);
	if (!call || (call->exeInlined != "none: memcpy" && call->exeInlined != "none: memset"))
		return false;
	const carglist_t& args = *static_cast<const cexpr_t*>(parent)->a;
	std::string why;
	std::optional<std::string> type = context.types.keyOf(e->x->type, why);
	if (!type || args.size() != 3)
		return false;
	std::string own = context.types.canonical(*type);
	std::optional<int64_t> size = context.model.sizeOf(own);
	if (!size || *size != static_cast<int64_t>(e->x->type.get_size()))
		return false;
	auto points = [&](const cexpr_t* pointer, bool derived) {
		std::string to = context.types.canonical(pointedType(function, context, pointer));
		SrcTypes::Parts a = context.types.parts(to), b = context.types.parts(own);
		return to == own || (derived && a.kind == "record" && b.kind == "record" &&
							 context.types.derivesAtZero(a.name, b.name));
	};
	return points(&args[0], false) && (call->exeInlined == "none: memset" || points(&args[1], true));
}

void checkRules(Function& function, Context& context)
{
	auto missing = [&](const citem_t* item, std::string what) { function.meaning[item] = Missing{std::move(what)}; };
	std::unordered_map<const cexpr_t*, const citem_t*> parents = parentsOf(function);
	// the helpers called (a call is visited before its callee): their calls carry the verdict
	std::set<const cexpr_t*> called;
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t* s) {
		// a statement src leaves out is not printed (a LOBYTE target, NtCurrentTeb included)
		if (function.omitted(s))
			return;
		switch (e->op)
		{
		case cot_call:
			if (e->x->op != cot_helper)
				break;
			called.insert(e->x);
			// (an intrinsic's call, or Intrinsics' form of one, `_mm_cvtss_f32(_mm_sqrt_ss(...))`; an inlined CRT
			// function's msvc9 call, Calls')
			if (auto found = function.meaning.find(e);
				found == function.meaning.end() ||
				(!std::holds_alternative<Intrinsic>(found->second) && !std::holds_alternative<Rewrite>(found->second) &&
				 !std::holds_alternative<Call>(found->second)))
				missing(e, std::string("Hex-Rays helper ") + e->x->helper);
			break;
		case cot_helper:
			if (!called.count(e))
				missing(e, std::string("Hex-Rays helper ") + e->helper);
			break;
		case cot_str:
			missing(e, "string constant");
			break;
		case cot_sizeof:
			// plain C++: the overlay's default goes
			if ((sizeOfArgument(e, parents, *function.cfunc->get_lvars()) && sizeAgrees(function, context, e)) ||
				sizeOfDestination(function, context, e, parents))
			{
				function.meaning.erase(e);
				break;
			}
			[[fallthrough]];
		case cot_type:
			// the type a passed sizeof names (sizeOfDestination) prints as src's type
			if (auto parent = parents.find(e); e->op == cot_type && parent != parents.end() &&
											   parent->second->op == cot_sizeof &&
											   sizeOfDestination(function, context,
																 static_cast<const cexpr_t*>(parent->second), parents))
			{
				function.meaning.erase(e);
				break;
			}
			missing(e, "Hex-Rays sizeof/type expression");
			break;
		case cot_insn:
			missing(e, "statement inside an expression");
			break;
		default:
			break;
		}
	});
	forEachStatement(&function.cfunc->body, [&](const cinsn_t* s) {
		if (s->op == cit_asm)
			missing(s, "inline asm");
	});
	const std::optional<exefacts::EhFrame>* eh = nullptr;
	try
	{
		eh = &context.exe.ehFrame(uint32_t(function.ea));
	}
	catch (const exefacts::Raised&)
	{
		function.blockers.push_back("exception frame unread: exefacts cannot decode the function");
		return;
	}
	if (!*eh || (*eh)->kind == exefacts::EhFrame::kNone)
		return;
	if ((*eh)->kind == exefacts::EhFrame::kSeh)
	{
		// an SEH frame (exefacts: a scope table at the registration, which MSVC builds for every function with a
		// `__try` and no other): its __except or __finally runs where the body raises or leaves a `__try`, which
		// nothing printed says. Marked at the scope, each try-level store Hex-Rays prints (entering or leaving a
		// `__try`), and blocked as a whole, so that a body whose stores Hex-Rays dropped is never clean either
		std::string why = "SEH __try scope (scope table " + hexText((*eh)->scopeTable) +
						  "): the exe's __except/__finally is not printed";
		std::set<ea_t> stores((*eh)->stores.begin(), (*eh)->stores.end());
		forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
			if (e->op == cot_asg && stores.count(e->ea))
				missing(e, why);
		});
		function.blockers.push_back(why);
	}
	else if ((*eh)->kind == exefacts::EhFrame::kCxx && (*eh)->tryCount > 0)
		function.blockers.push_back("C++ try/catch");
	else if ((*eh)->kind == exefacts::EhFrame::kCxx)
		function.notes.push_back("The exe's cleanup-only C++ EH frame is left out.");
	else
		function.blockers.push_back("exception frame of kind other");
}

}  // namespace idakit
