// Declarations: each local (not a parameter) that the body uses is declared in the innermost block holding all its
// uses (a record's: the innermost such block no goto from outside enters, as C++ does not jump past its constructor).
// When the first statement of that block to use it is `v = expr;` (expr not reading v), or a call returning a record
// by value into v through the hidden result pointer (`F(&v, ...)`, printed `v = F(...)`; no other argument reading
// v), v's address does not escape, and no goto jumps from outside past that statement into the rest of the block,
// the statement becomes `T v = expr;` (the call builds v in place, as the exe's does). Otherwise v is declared
// without an initialiser at the top of that block (address escaping, Scopes' addressTaken: of the function; `v.F()`,
// `v = F()` and `F(v)` are plain uses). A loop counter (every use in the for statement whose init assigns it, nothing
// jumping into the loop) is declared in that init: `for (T i = 0; ...)`. Uses in the statements a rewrite drops or
// src leaves out (not printed) do not count.
//
// A call returning a record by value into an object that no declaration it initialises is (`w = F(...)` on an
// object declared before, a member, an inlined local) passes C++'s temporary, where the exe builds the result in the
// object. Where C++ copies the record as its bytes (SrcModel::byteCopied: no destructor runs either) and the object
// is a whole one (a pointer variable's pointee, a local, a reference's referent: the argument Hex-Rays passes,
// uncast and unconverted, `p` or `&v`), it is printed `::new (p) T(F(...))`, which passes the object itself
// (Function::placements), as a statement or as the value the exe's call returns (`&(v = F())`: the object's
// address, the new-expression's value); else marked.

#include "EmitScopes.hpp"

namespace idakit
{

namespace
{

// the resolved call returning a record by value into the object its hidden result pointer points at (printed
// `w = F(...)`), not C++'s temporary; nullptr for any other expression
const Call* byValueInto(const Function& function, const cexpr_t* e)
{
	auto found = e->op == cot_call ? function.meaning.find(e) : function.meaning.end();
	const Call* c = found == function.meaning.end() ? nullptr : std::get_if<Call>(&found->second);
	if (!c || c->resultArgument < 0 || static_cast<size_t>(c->resultArgument) >= e->a->size() ||
		function.temporaries.count(e))
		return nullptr;
	return c;
}

// `v = expr;` with expr not reading v, or `F(&v, ...)` returning a record into v with no other argument reading v
bool initialises(const Function& function, const cinsn_t* s, int lvar)
{
	if (s->op != cit_expr)
		return false;
	const cexpr_t* e = s->cexpr;
	bool reads = false;
	if (const Call* c = byValueInto(function, e))
	{
		const cexpr_t* result = stripCasts(&(*e->a)[c->resultArgument]);
		if (result->op != cot_ref || lvarIndex(result->x) != lvar)
			return false;
		for (size_t k = 0; k < e->a->size(); ++k)
			if (static_cast<int>(k) != c->resultArgument)
				forEachSubexpression(&(*e->a)[k], [&](const cexpr_t* x) { reads |= lvarIndex(x) == lvar; });
		return !reads;
	}
	if (e->op != cot_asg || lvarIndex(e->x) != lvar)
		return false;
	// an assignment an idiom prints in its own form (`fnvd::setVtable(&v, X_vftable)`) is no `v = expr`
	if (auto found = function.meaning.find(e); found != function.meaning.end() &&
											   std::holds_alternative<Rewrite>(found->second))
		return false;
	forEachSubexpression(e->y, [&](const cexpr_t* x) { reads |= lvarIndex(x) == lvar; });
	return !reads;
}

// a goto from outside s jumps to a label inside it
bool entered(const Scopes& scopes, const cinsn_t* s)
{
	bool from = false;
	forEachStatement(s, [&](const cinsn_t* inner) {
		if (inner->label_num >= 0)
			for (const cinsn_t* g : scopes.gotosTo(inner->label_num))
				from |= !scopes.inside(g, s);
	});
	return from;
}

// a loop counter: every use of lvar is in the for statement whose init assigns it (`for (i = 0; ...)`), and no goto
// from outside the loop enters it
bool loopCounter(const Scopes& scopes, const cinsn_t* loop, int lvar)
{
	if (loop->op != cit_for || loop->cfor->init.op != cot_asg || lvarIndex(loop->cfor->init.x) != lvar)
		return false;
	bool reads = false;
	forEachSubexpression(loop->cfor->init.y, [&](const cexpr_t* e) { reads |= lvarIndex(e) == lvar; });
	for (const cinsn_t* use : scopes.uses(lvar))
		reads |= !scopes.inside(use, loop);
	return !reads && !entered(scopes, loop);
}

// list[index] is in a frame of the block (Function's frames: a block of its own) that some use is not inside
bool leavesFrame(const Function& function, const Scopes& scopes, const std::vector<const cinsn_t*>& list,
				 size_t index, const std::vector<const cinsn_t*>& uses)
{
	for (size_t n = 0; n <= index; ++n)
	{
		auto frame = function.frames.find(list[n]);
		if (frame == function.frames.end() || index >= n + frame->second)
			continue;
		for (const cinsn_t* use : uses)
		{
			bool in = false;
			for (size_t k = n; k < std::min(list.size(), n + frame->second); ++k)
				in |= scopes.inside(use, list[k]);
			if (!in)
				return true;
		}
	}
	return false;
}

}  // namespace

void placeDeclarations(Function& function, Context& context)
{
	Scopes scopes(function);
	const lvars_t& lvars = *function.cfunc->get_lvars();
	for (size_t k = 0; k < lvars.size(); ++k)
	{
		int lvar = static_cast<int>(k);
		const std::vector<const cinsn_t*>& uses = scopes.uses(lvar);
		if (lvars[k].is_arg_var())
			continue;
		Local& local = function.locals[k];
		// placed by an earlier pass (an inlined local)
		if (function.placed(k))
			continue;
		// never declared: unused (its uses all dropped or left out by the passes), an alias, an EH frame's local
		if (uses.empty() || local.aliasOf >= 0 || local.ehFrame)
		{
			local.declaredAt = nullptr;
			continue;
		}
		if (scopes.addressTaken(lvar))
		{
			local.declaredAt = scopes.body();
			continue;
		}
		// a loop counter is declared in its for statement's init
		if (!local.storage && !local.inlined && loopCounter(scopes, uses[0], lvar))
		{
			local.declaredAt = uses[0];
			local.initialised = true;
			continue;
		}
		const cinsn_t* block = scopes.blockOf(uses[0]);
		for (const cinsn_t* use : uses)
			block = scopes.common(block, use);
		// a record's constructor may not be jumped past (C++): not in a block a goto enters
		if (lvars[k].type().is_udt())
			while (block != scopes.body() && entered(scopes, block))
				block = scopes.blockOf(scopes.parent(block));
		local.declaredAt = block;
		// the block's first statement holding a use
		std::vector<const cinsn_t*> list;
		for (const cinsn_t& item : *block->cblock)
			list.push_back(&item);
		for (size_t index = 0; index < list.size(); ++index)
		{
			const cinsn_t* item = list[index];
			bool holds = false;
			for (const cinsn_t* use : uses)
				holds |= scopes.inside(use, item);
			if (!holds)
				continue;
			// raw storage and an inlined local's macro take no initialiser (their assignments go through the object);
			// nor a local used past the frame (a block of its own) the statement is in
			if (!local.storage && !local.inlined && initialises(function, item, lvar) &&
				!scopes.jumpedPast(block, index) && !leavesFrame(function, scopes, list, index, uses))
			{
				local.declaredAt = item;
				local.initialised = true;
			}
			break;
		}
	}
	// a record returned by value into an object no declaration initialises: built in it with placement new where the
	// record is byte-copied and the object a whole one (the new-expression's value is the object's address, which the
	// exe's call returns), else marked
	std::vector<std::pair<const cexpr_t*, std::string>> assigned;
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t* s) {
		const Call* c = byValueInto(function, e);
		if (!c)
			return;
		const cexpr_t* argument = &(*e->a)[c->resultArgument];
		const cexpr_t* result = stripCasts(argument);
		int v = result->op == cot_ref ? lvarIndex(result->x) : -1;
		if (v >= 0 && function.locals[v].declaredAt == s && function.locals[v].initialised)
			return;
		// the record src's callee returns (Hex-Rays' pointee is no evidence: a slot it typed otherwise)
		const SrcModel::Type* returned = context.model.type(c->function->type);
		const SrcModel::Record* record =
			returned ? context.model.record(context.types.parts(context.types.canonical(returned->ret)).name) : nullptr;
		if (!record || !context.model.byteCopied(*record))
		{
			assigned.emplace_back(e, "the record the call returns is assigned to an object built before: C++ runs its "
									 "copy assignment and destroys the temporary, where the exe builds the result in "
									 "the object");
			return;
		}
		// the object as Hex-Rays passes it, uncast and unconverted: a pointer variable (its pointee) or `&v`
		bool converted = function.conversions.count(argument) || function.meaning.count(argument);
		bool whole = !converted && ((argument->op == cot_var && argument->type.is_ptr()) ||
									(argument->op == cot_ref && argument->x->op == cot_var &&
									 !function.meaning.count(argument->x) && !function.conversions.count(argument->x)));
		if (whole)
			function.placements.insert(e);
		else
			assigned.emplace_back(e, "the record the call returns is assigned to a member, an element or a converted "
									 "pointer's object: C++'s assignment passes a temporary, where the exe builds the "
									 "result in the object");
	});
	for (const auto& [e, why] : assigned)
		function.meaning[e] = Missing{why};
}

}  // namespace idakit
