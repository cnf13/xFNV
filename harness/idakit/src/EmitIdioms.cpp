// What the idiom passes share: the checks a rewrite needs (no label, nothing unresolved) and the erasing of what the
// layers marked inside the statements it replaces. Where statements and uses are relative to each other: Scopes.

#include "EmitPasses.hpp"

#include <set>

namespace idakit
{

bool isZero(const cexpr_t* e)
{
	e = stripCasts(e);
	return e->op == cot_num && e->numval() == 0;
}

const cinsn_t* soleStatement(const cinsn_t* s)
{
	if (s && s->op == cit_block)
		return s->cblock->size() == 1 ? &s->cblock->front() : nullptr;
	return s;
}

bool truthTested(const cexpr_t* e, const std::unordered_map<const cexpr_t*, const citem_t*>& parents)
{
	auto parent = parents.find(e);
	for (; parent != parents.end() && parent->second->op == cot_cast; parent = parents.find(e))
		e = static_cast<const cexpr_t*>(parent->second);
	if (parent == parents.end())
		return false;
	const citem_t* p = parent->second;
	switch (p->op)
	{
	case cot_lnot:
	case cot_land:
	case cot_lor:
	case cit_if:
	case cit_while:
	case cit_do:
		return true;
	case cit_for:
		return &static_cast<const cinsn_t*>(p)->cfor->expr == e;
	case cot_tern:
		return static_cast<const cexpr_t*>(p)->x == e;
	case cot_eq:
	case cot_ne:
	{
		const cexpr_t* binary = static_cast<const cexpr_t*>(p);
		return isZero(binary->x == e ? binary->y : binary->x);
	}
	default:
		return false;
	}
}

bool labelledInside(const std::vector<const cinsn_t*>& statements, const cinsn_t* except)
{
	bool labelled = false;
	for (const cinsn_t* s : statements)
		forEachStatement(s, [&](const cinsn_t* t) { labelled |= t != except && t->label_num >= 0; });
	return labelled;
}

bool unresolvedIn(const Function& function, const cexpr_t* e)
{
	bool missing = false;
	forEachSubexpression(e, [&](const cexpr_t* x) {
		auto found = function.meaning.find(x);
		missing |= found != function.meaning.end() && std::holds_alternative<Missing>(found->second);
	});
	return missing;
}

void eraseReplaced(Function& function, const std::vector<const cinsn_t*>& replaced,
				   const std::vector<const citem_t*>& kept)
{
	std::set<const citem_t*> keep;
	for (const citem_t* item : kept)
		if (item->is_expr())
			forEachSubexpression(static_cast<const cexpr_t*>(item), [&](const cexpr_t* e) { keep.insert(e); });
		else
			keep.insert(item);
	// a statement item keeps its statements: the walk does not enter it
	auto erase = [&](auto& self, const cinsn_t* s) -> void {
		if (keep.count(s))
			return;
		function.meaning.erase(s);
		for (const cexpr_t* own : ownExpressions(s))
			forEachSubexpression(own, [&](const cexpr_t* e) {
				if (!keep.count(e))
					function.meaning.erase(e);
			});
		for (const cinsn_t* child : childStatements(s))
			self(self, child);
	};
	for (const cinsn_t* s : replaced)
		erase(erase, s);
}

}  // namespace idakit
