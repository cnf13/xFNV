#include "EmitScopes.hpp"

#include <algorithm>

namespace idakit
{

Scopes::Scopes(const Function& function)
	: m_function(function), m_body(&function.cfunc->body), m_expressionParents(parentsOf(function)),
	  m_dropped(function.dropped())
{
	// an alias's copy statement prints nothing either (Aliases): an original read only there is unused
	std::set<const cinsn_t*> copies = function.copies();
	m_dropped.insert(copies.begin(), copies.end());
	walk(m_body, nullptr, true);
}

const std::vector<const cinsn_t*>& Scopes::uses(int lvar) const
{
	static const std::vector<const cinsn_t*> none;
	auto found = m_uses.find(lvar);
	return found == m_uses.end() ? none : found->second;
}

bool Scopes::usedOnlyIn(int lvar, const std::vector<const cinsn_t*>& statements) const
{
	for (const cinsn_t* use : uses(lvar))
		if (std::none_of(statements.begin(), statements.end(), [&](const cinsn_t* s) { return inside(use, s); }))
			return false;
	return true;
}

const cinsn_t* Scopes::parent(const cinsn_t* s) const
{
	auto found = m_parents.find(s);
	return found == m_parents.end() ? nullptr : found->second;
}

const cinsn_t* Scopes::blockOf(const cinsn_t* s) const
{
	while (s && s->op != cit_block)
		s = parent(s);
	return s;
}

bool Scopes::inside(const cinsn_t* s, const cinsn_t* outer) const
{
	for (; s; s = parent(s))
		if (s == outer)
			return true;
	return false;
}

const cinsn_t* Scopes::common(const cinsn_t* a, const cinsn_t* b) const
{
	for (const cinsn_t* block = blockOf(a); block; block = blockOf(parent(block)))
		if (inside(b, block))
			return block;
	return m_body;
}

std::vector<const cinsn_t*> Scopes::gotosTo(int label) const
{
	std::vector<const cinsn_t*> out;
	for (const cinsn_t* g : m_gotos)
		if (g->cgoto->label_num == label)
			out.push_back(g);
	return out;
}

bool Scopes::jumpedPast(const cinsn_t* block, size_t at) const
{
	std::vector<const cinsn_t*> after;
	size_t k = 0;
	for (const cinsn_t& item : *block->cblock)
		if (k++ > at)
			after.push_back(&item);
	for (const cinsn_t* later : after)
	{
		bool crossed = false;
		forEachStatement(later, [&](const cinsn_t* s) {
			if (s->label_num < 0)
				return;
			for (const cinsn_t* g : gotosTo(s->label_num))
			{
				bool fromAfter = false;
				for (const cinsn_t* other : after)
					fromAfter |= inside(g, other);
				crossed |= !fromAfter;
			}
		});
		if (crossed)
			return true;
	}
	return false;
}

bool Scopes::plainReturn(const cinsn_t* s) const
{
	if (s->op != cit_return)
		return false;
	const cexpr_t* e = s->creturn->expr.op == cot_empty ? nullptr : stripCasts(&s->creturn->expr);
	return !e || e->op == cot_num || (lvarIndex(e) >= 0 && !addressTaken(lvarIndex(e)));
}

bool Scopes::leaves(const std::vector<const cinsn_t*>& statements)
{
	std::set<int> labels;
	std::vector<int> targets;
	for (const cinsn_t* s : statements)
	{
		if (s->op == cit_return || s->op == cit_break || s->op == cit_continue || s->contains_insn(cit_return) ||
			s->contains_free_break() || s->contains_free_continue())
			return true;
		forEachStatement(s, [&](const cinsn_t* x) {
			if (x->label_num >= 0)
				labels.insert(x->label_num);
			if (x->op == cit_goto)
				targets.push_back(x->cgoto->label_num);
		});
	}
	for (int label : targets)
		if (!labels.count(label))
			return true;
	return false;
}

Scopes::Scope Scopes::scopeOf(int lvar) const
{
	const std::vector<const cinsn_t*>& statements = uses(lvar);
	const cinsn_t* block = blockOf(statements[0]);
	for (const cinsn_t* use : statements)
		block = common(block, use);
	Scope scope{block, childStatements(block), 0, 0};
	bool found = false;
	for (size_t k = 0; k < scope.items.size(); ++k)
		for (const cinsn_t* use : statements)
			if (inside(use, scope.items[k]))
			{
				scope.first = found ? scope.first : k;
				scope.last = k;
				found = true;
			}
	return scope;
}

void Scopes::walk(const cinsn_t* s, const cinsn_t* parent, bool printed)
{
	m_parents[s] = parent;
	printed = printed && !m_dropped.count(s) && !m_function.omitted(s);
	if (printed && s->op == cit_goto)
		m_gotos.push_back(s);
	for (const cexpr_t* own : printed ? ownExpressions(s) : std::vector<const cexpr_t*>{})
		count(own, s);
	for (const cinsn_t* child : childStatements(s))
		walk(child, s, printed);
}

// the uses in e, printed in s: an expression rewrite's in its items only (its form prints nothing else), none in the
// hidden result pointer of a call whose result is C++'s temporary (no local holds it)
void Scopes::count(const cexpr_t* e, const cinsn_t* s)
{
	auto found = m_function.meaning.find(e);
	const Rewrite* rewrite = found != m_function.meaning.end() ? std::get_if<Rewrite>(&found->second) : nullptr;
	if (rewrite)
	{
		for (const citem_t* item : rewrite->items)
			if (item->is_expr())
				count(static_cast<const cexpr_t*>(item), s);
		return;
	}
	if (int v = lvarIndex(e); v >= 0)
	{
		// an alias's uses are its original's
		while (m_function.locals[v].aliasOf >= 0)
			v = m_function.locals[v].aliasOf;
		std::vector<const cinsn_t*>& list = m_uses[v];
		if (list.empty() || list.back() != s)
			list.push_back(s);
	}
	if (int v = e->op == cot_ref ? lvarIndex(e->x) : -1; v >= 0 && escapes(e))
	{
		while (m_function.locals[v].aliasOf >= 0)
			v = m_function.locals[v].aliasOf;
		m_taken.insert(v);
	}
	if (e->op == cot_call)
	{
		const Call* resolved = found != m_function.meaning.end() ? std::get_if<Call>(&found->second) : nullptr;
		// a resolved call prints without its callee, but for a call through a pointer (Call::through)
		if (!resolved || resolved->through)
			count(e->x, s);
		const Call* call = m_function.temporaries.count(e) ? resolved : nullptr;
		for (size_t k = 0; k < e->a->size(); ++k)
			if (!call || static_cast<int>(k) != call->resultArgument)
				count(&(*e->a)[k], s);
		return;
	}
	if (op_uses_x(e->op) && e->x)
		count(e->x, s);
	if (op_uses_y(e->op) && e->y)
		count(e->y, s);
	if (op_uses_z(e->op) && e->z)
		count(e->z, s);
}

bool Scopes::escapes(const cexpr_t* e) const
{
	auto up = m_expressionParents.find(e);
	if (up == m_expressionParents.end() || !up->second->is_expr())
		return true;
	const auto* parent = static_cast<const cexpr_t*>(up->second);
	auto found = m_function.meaning.find(parent);
	const Meaning* meaning = found == m_function.meaning.end() ? nullptr : &found->second;
	// an implicit conversion prints as its operand
	if (parent->op == cot_cast)
		return !meaning || !std::holds_alternative<Implicit>(*meaning) || escapes(parent);
	if (parent->op != cot_call || e == parent->x)
		return true;
	size_t k = static_cast<const carg_t*>(e) - &(*parent->a)[0];
	if (const Call* call = meaning ? std::get_if<Call>(meaning) : nullptr)
	{
		if (parameterIndex(*call, k) >= 0 && !call->byReference[k])
			return true;
		// a record returned by value: the result pointer is the result local's, never another argument's
		if (call->resultArgument >= 0 && static_cast<int>(k) != call->resultArgument)
			return false;
	}
	// a folded accessor's receiver: its field's load or a setter's store reads v; its address or the receiver itself
	// is a pointer result like a call's
	else if (const Access* access = meaning ? std::get_if<Access>(meaning) : nullptr)
	{
		if (k != 0)
			return true;
		if (access->value || (access->field && access->load))
			return false;
	}
	else
		return true;
	// a pointer result may be v's address (the hidden result pointer, a returned `this` or member): unused, or read
	// through at once, it goes nowhere; else it is v's address in turn
	if (!parent->type.is_ptr())
		return false;
	auto next = m_expressionParents.find(parent);
	if (next == m_expressionParents.end())
		return true;
	if (!next->second->is_expr())
		return next->second->op != cit_expr;
	const auto* use = static_cast<const cexpr_t*>(next->second);
	if (use->op == cot_ptr || use->op == cot_memptr)
	{
		auto above = m_expressionParents.find(use);
		return above != m_expressionParents.end() && above->second->op == cot_ref;
	}
	return escapes(parent);
}

}  // namespace idakit
