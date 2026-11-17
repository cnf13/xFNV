// Hidden results: a record a call returns by value (MSVC's hidden result pointer, Hex-Rays' `F(&w, ...)`), used once
// right where C++ would use the call's temporary, is that temporary: `T* p = &(w = F(...)); G(*p);` prints
// `G(F(...));`, `(&(w = F()))->z` prints `F().z`. Conditions (user, 2026-11-07):
// - neither the local w the call fills nor the pointer local p it returns has another use (w's address taken once, by
//   the call; p written once, by `p = F(...);`, and read once); neither is possibly undefined, an alias, raw storage
//   or a local with a vptr store or Construct of its own;
// - p's one read is in the next statement of the same block (an expression statement, a return, an if's or a
//   switch's condition); `p = F(...);`, no longer printed, carries no label;
// - that read binds a temporary: the object of a member call, an argument the callee takes as `const T&` or by
//   value, the right side of a byte-copy assignment, the value returned, or a member read (not written, not
//   addressed, no array); a call taking the temporary returns no pointer or reference, which could point into it
//   past the full expression (the exe's local lives on);
// - folding moves no call with effects across another: nothing in the use's statement outside the read's own
//   ancestors calls (a trivial accessor's load aside, which the checker leaves out of the call comparison) or writes,
//   and the read is in no operand C++ evaluates conditionally (`&&`, `||`, `?:`).
// The call is recorded a temporary; p's read prints as the object the call returns (its declaration statement and
// both locals' declarations go).

#include "EmitScopes.hpp"

#include "idakit/SrcIda.hpp"

namespace idakit
{

namespace
{

class HiddenResults
{
public:
	HiddenResults(Function& function, Context& context)
		: m_function(function), m_context(context), m_scopes(function), m_parents(parentsOf(function))
	{
		forEachExpression(function, [&](const cexpr_t* e, const cinsn_t* s) {
			if (int v = lvarIndex(e); v >= 0)
				++m_occurrences[v];
			m_statement[e] = s;
		});
		for (const Local& local : function.locals)
			if (local.aliasOf >= 0)
				m_aliased.insert(local.aliasOf);
	}

	void run()
	{
		std::vector<const cexpr_t*> calls;
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			if (e->op == cot_call)
				calls.push_back(e);
		});
		for (const cexpr_t* call : calls)
			fold(call);
	}

private:
	void fold(const cexpr_t* call)
	{
		const Call* resolved = callOf(call);
		if (!resolved || resolved->resultArgument < 0)
			return;
		const cexpr_t* result = &(*call->a)[resolved->resultArgument];
		int w = result->op == cot_ref ? lvarIndex(result->x) : -1;
		if (w < 0 || !plainLocal(w, 1))
			return;
		// a local the call fills (InlinedLocals' typed storage) becomes the temporary; one with structor work does not
		const std::optional<InlinedLocal>& inlined = m_function.locals[w].inlined;
		if (inlined && (inlined->atStart || inlined->atEnd || inlined->construct))
			return;
		const citem_t* parent = parentOf(call);
		// `p = F(...);`: p's one read in the next statement
		if (parent && parent->op == cot_asg && static_cast<const cexpr_t*>(parent)->y == call)
		{
			const auto* assignment = static_cast<const cexpr_t*>(parent);
			int p = lvarIndex(assignment->x);
			const cinsn_t* s = m_statement[call];
			if (p < 0 || !plainLocal(p, 2) || parentOf(assignment) != s || s->op != cit_expr || s->label_num >= 0)
				return;
			const cinsn_t* next = nextStatement(s);
			const cexpr_t* read = next ? readOf(next, p) : nullptr;
			if (!read || !binds(read) || !inOrder(next, read))
				return;
			m_function.temporaries.insert(call);
			m_function.meaning[read] = Rewrite{1, "&*$0", {call}, {s}, m_function.locals[p].type};
			m_folded.insert(read);
			return;
		}
		// the call's own pointer read where the temporary is used: nothing moves
		if (binds(call))
			m_function.temporaries.insert(call);
	}

	const Call* callOf(const cexpr_t* call) const
	{
		auto found = m_function.meaning.find(call);
		return found != m_function.meaning.end() ? std::get_if<Call>(&found->second) : nullptr;
	}

	// a local (not a parameter) used exactly that many times, none of them through an alias, not possibly undefined,
	// not raw storage, not an EH frame's
	bool plainLocal(int v, int uses) const
	{
		const Local& local = m_function.locals[v];
		auto found = m_occurrences.find(v);
		return !(*m_function.cfunc->get_lvars())[v].is_arg_var() && found != m_occurrences.end() &&
			   found->second == uses && !local.undefined && local.aliasOf < 0 && !m_aliased.count(v) &&
			   !local.storage && !local.ehFrame && !local.type.empty();
	}

	const citem_t* parentOf(const cexpr_t* e) const
	{
		auto found = m_parents.find(e);
		return found == m_parents.end() ? nullptr : found->second;
	}

	// e's parent past the casts C++ makes by itself (printed as their operand)
	const citem_t* useOf(const cexpr_t*& e) const
	{
		const citem_t* parent = parentOf(e);
		while (parent && parent->op == cot_cast && meaningIs<Implicit>(static_cast<const cexpr_t*>(parent)))
		{
			e = static_cast<const cexpr_t*>(parent);
			parent = parentOf(e);
		}
		return parent;
	}

	template <class T>
	bool meaningIs(const cexpr_t* e) const
	{
		auto found = m_function.meaning.find(e);
		return found != m_function.meaning.end() && std::holds_alternative<T>(found->second);
	}

	// the statement after s in its block, unless something drops or leaves it out
	const cinsn_t* nextStatement(const cinsn_t* s) const
	{
		const cinsn_t* block = m_scopes.parent(s);
		if (!block || block->op != cit_block)
			return nullptr;
		for (auto it = block->cblock->begin(); it != block->cblock->end(); ++it)
			if (&*it == s)
			{
				if (++it == block->cblock->end())
					return nullptr;
				const cinsn_t* next = &*it;
				std::set<const cinsn_t*> dropped = m_function.dropped();
				bool kind =
					next->op == cit_expr || next->op == cit_return || next->op == cit_if || next->op == cit_switch;
				return kind && !dropped.count(next) && !m_function.omitted(next) ? next : nullptr;
			}
		return nullptr;
	}

	// p's read among the statement's own expressions (nullptr where it is in a statement inside it)
	const cexpr_t* readOf(const cinsn_t* s, int p) const
	{
		const cexpr_t* read = nullptr;
		for (const cexpr_t* own : ownExpressions(s))
			forEachSubexpression(own, [&](const cexpr_t* e) {
				if (lvarIndex(e) == p)
					read = e;
			});
		return read;
	}

	// the call returns a pointer or reference (not a record by value through its own hidden result pointer)
	bool mayPointInto(const cexpr_t* call) const
	{
		const Call* resolved = callOf(call);
		if (!resolved)
			return call->type.is_ptr();
		return resolved->resultArgument < 0 && (call->type.is_ptr() || resolved->returnsReference);
	}

	// the pointer read (p, or the call's own result) is used as the object of a temporary C++ binds
	bool binds(const cexpr_t* read) const
	{
		const cexpr_t* e = read;
		const citem_t* parent = useOf(e);
		if (!parent || !parent->is_expr())
			return false;
		const auto* use = static_cast<const cexpr_t*>(parent);
		// the call's result must not be a pointer or reference that may point into the temporary (it ends with the
		// full expression; the exe's local lives on)
		if (use->op == cot_call)
			return boundArgument(use, static_cast<const carg_t*>(e) - &(*use->a)[0]) && !mayPointInto(use);
		if (use->op == cot_memptr)
		{
			const citem_t* above = parentOf(use);
			if (!above || !meaningIs<Member>(use) || memberType(use).is_array())
				return false;
			if (above->is_expr())
			{
				const auto* outer = static_cast<const cexpr_t*>(above);
				return outer->op != cot_ref && !(written(outer) && outer->x == use);
			}
			return true;
		}
		if (use->op != cot_ptr)
			return false;
		// *p: an argument by value, a byte copy's source, the value returned
		const cexpr_t* object = use;
		const citem_t* above = useOf(object);
		if (!above)
			return false;
		if (!above->is_expr())
			return above->op == cit_return && m_function.resultArgument < 0;
		const auto* outer = static_cast<const cexpr_t*>(above);
		if (outer->op == cot_call)
			return boundArgument(outer, static_cast<const carg_t*>(object) - &(*outer->a)[0]);
		if (outer->op == cot_asg && outer->y == object && outer->x->type.is_udt())
		{
			const SrcModel::Record* record = m_context.types.recordOf(outer->x->type);
			return record && record->trivialCopy;
		}
		return false;
	}

	// argument k of the call takes a temporary: the object of a member call, a `const T&` parameter, a by-value one
	bool boundArgument(const cexpr_t* call, ptrdiff_t k) const
	{
		if (k < 0 || static_cast<size_t>(k) >= call->a->size())
			return false;
		auto found = m_function.meaning.find(call);
		if (found == m_function.meaning.end())
			return false;
		if (const Access* access = std::get_if<Access>(&found->second))
			return k == 0 && access->load && access->field;
		const Call* resolved = callOf(call);
		if (!resolved || !resolved->castTo[k].empty() || !resolved->mismatch[k].empty())
			return false;
		int parameter = parameterIndex(*resolved, k);
		if (parameter < 0)
			return k == 0 && k != resolved->resultArgument;
		const SrcModel::Type* type = m_context.model.type(resolved->function->type);
		if (!type || static_cast<size_t>(parameter) >= type->params.size())
			return false;
		const std::string& key = type->params[parameter];
		SrcTypes::Parts parts = m_context.types.parts(m_context.types.bare(key));
		if (resolved->byReference[k])
			return parts.kind == "lref" && m_context.types.constPointee(key);
		return parts.kind == "record";
	}

	// the read can take the call's place in the statement: it is in no operand evaluated conditionally, and nothing
	// else in the statement calls (a trivial accessor's load aside) or writes
	bool inOrder(const cinsn_t* s, const cexpr_t* read) const
	{
		std::set<const cexpr_t*> ancestors;
		for (const cexpr_t* e = read;;)
		{
			const citem_t* parent = parentOf(e);
			if (!parent || !parent->is_expr())
				break;
			const auto* above = static_cast<const cexpr_t*>(parent);
			if (((above->op == cot_land || above->op == cot_lor) && above->y == e) ||
				(above->op == cot_tern && above->x != e))
				return false;
			ancestors.insert(above);
			e = above;
		}
		bool calm = true;
		for (const cexpr_t* own : ownExpressions(s))
			forEachSubexpression(own, [&](const cexpr_t* e) {
				if (ancestors.count(e) || !calm)
					return;
				if (e->op == cot_call)
				{
					std::optional<AccessorCall> accessor = accessorCall(m_context, e);
					calm = accessor && !accessor->value;
				}
				else if ((written(e) && !ancestors.count(e)) || m_folded.count(e))
					calm = false;
			});
		return calm;
	}

	Function& m_function;
	Context& m_context;
	Scopes m_scopes;
	std::unordered_map<const cexpr_t*, const citem_t*> m_parents;
	std::unordered_map<const cexpr_t*, const cinsn_t*> m_statement;
	std::map<int, int> m_occurrences;
	std::set<int> m_aliased;
	std::set<const cexpr_t*> m_folded;  // the pointer reads already printed as a call's temporary
};

}  // namespace

void foldHiddenResults(Function& function, Context& context)
{
	HiddenResults(function, context).run();
}

}  // namespace idakit
