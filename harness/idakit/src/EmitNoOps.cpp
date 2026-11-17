// No-op calls: a trivial accessor's call whose result is unused does nothing (0x6815C0 as a statement, the folded
// empty constructor of NiPoint3, NiMatrix3, ...), so src writes nothing for it. Not an x87 load (its result stays in
// ST0 for the caller to pop) or a store, and only where the arguments themselves do nothing (no assignment, increment,
// decrement or call; Hex-Rays' helpers that only compute a value, ADJ(p), LOBYTE(x), COERCE_FLOAT(x), __PAIR64__,
// are no calls in the exe). Nor does an empty function's call (exefacts' emptyFunction: 0x483710, which the linker
// folded for many classes), which src may make or leave out: one no layer resolved to a src call (a fold, a receiver
// of another class) is left out, on the same condition; one resolved prints as that call.

#include "EmitPasses.hpp"

#include <Facts.hpp>

#include <algorithm>

namespace idakit
{

namespace
{

// a call of an empty function that is still unresolved (its meaning Missing: Calls' own, or the default)
bool unresolvedEmptyCall(const Function& function, Context& context, const cexpr_t* call)
{
	if (call->x->op != cot_obj || !context.exe.emptyFunction(static_cast<uint32_t>(call->x->obj_ea)))
		return false;
	auto meaning = function.meaning.find(call);
	return meaning != function.meaning.end() && std::holds_alternative<Missing>(meaning->second);
}

}  // namespace

bool sideEffects(Context& context, const cexpr_t* argument)
{
	bool found = false;
	forEachSubexpression(argument, [&](const cexpr_t* e) {
		if (written(e))
			found = true;
		else if (e->op == cot_call)
		{
			std::string_view helper = e->x->op == cot_helper ? std::string_view(e->x->helper) : std::string_view();
			std::optional<AccessorCall> accessor = helper.empty() ? accessorCall(context, e) : std::nullopt;
			found |= !(helper == "ADJ" || helper == "__PAIR64__" || helper.starts_with("COERCE_") ||
					   isPartHelper(helper) || (accessor && !accessor->value));
		}
	});
	return found;
}

void dropNoOpCalls(Function& function, Context& context)
{
	// a call that does nothing whose result nothing reads, with arguments that do nothing either
	auto noOp = [&](const cexpr_t* call) {
		if (call->op != cot_call)
			return false;
		std::optional<AccessorCall> accessor = accessorCall(context, call);
		if ((!accessor || accessor->x87 || accessor->value) && !unresolvedEmptyCall(function, context, call))
			return false;
		const carglist_t& args = *call->a;
		return std::none_of(args.begin(), args.end(), [&](const carg_t& a) { return sideEffects(context, &a); });
	};
	std::vector<const cinsn_t*> dropped;
	forEachStatement(&function.cfunc->body, [&](const cinsn_t* s) {
		if (s->op == cit_expr && noOp(s->cexpr))
		{
			function.noOps.insert(s);
			dropped.push_back(s);
		}
	});
	// what the layers found in them goes with them
	eraseReplaced(function, dropped, {});
	// the left operand of a comma (`(f(p), v7)`, Hex-Rays' sequence inside a condition): the comma prints as its right
	// operand
	std::vector<const cexpr_t*> commas;
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
		if (e->op == cot_comma && !function.meaning.count(e) && noOp(e->x))
			commas.push_back(e);
	});
	for (const cexpr_t* comma : commas)
	{
		forEachSubexpression(comma->x, [&](const cexpr_t* e) { function.meaning.erase(e); });
		function.meaning[comma] = Rewrite{1, "$0", {comma->y}, {}, "", ""};
	}
}

}  // namespace idakit
