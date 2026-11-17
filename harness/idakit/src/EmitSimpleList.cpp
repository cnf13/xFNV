// BSSimpleList loops: the exe's walks of a list's nodes printed as src's range-for (BSCore/BSSimpleList.h):
//
//     for (n = &list; n; n = n->m_pkNext) { v = n->m_item; ... }               for (T v : list) { ... }
//     for (n = &list; n && n->m_item; n = n->m_pkNext) { v = n->m_item; ... }  for (T v : list.ItemsUntilNull()) { ... }
//     for (n = &list; n; n = n->m_pkNext) { if (!n->m_item) break; ... }       the same, stopping
//     for (n = &list; n; n = n->m_pkNext) { v = n->m_item; if (!v) break; ... }   the same, stopping
//
// `n = plist` for a pointer to the list gives `*plist` / `plist->ItemsUntilNull()`; the init may be the statement
// right before a `for (; n; ...)`. The iterator does exactly these steps: test the node (and its item, stopping),
// read the item at the top of the pass, step to m_pkNext. Where the body reads `n->m_item` itself instead of a local
// v set first, the loop takes the item by reference (`for (T& rpItem : list)`) and each such read prints as it: a
// read of the node's item at that point, as the exe's; a loop nested in or around another such loop names its item
// apart (`rpItem_2`). The reads of m_item / m_pkNext may be calls of trivial
// accessors (GetItem, GetNext and their folded copies), matched by value (EmitValue).
//
// A match only when n is used nowhere else (not in the body, not after the loop), v is the list's item type, used
// only in the body, with its address not taken; the replaced statements carry no label. Any other loop keeps its
// manual form.

#include "EmitScopes.hpp"

namespace idakit
{

namespace
{

class Loops
{
public:
	Loops(Function& function, Context& context)
		: m_function(function), m_context(context), m_values(function, context), m_scopes(function)
	{
	}

	void run()
	{
		forEachStatement(&m_function.cfunc->body, [&](const cinsn_t* block) {
			if (block->op != cit_block)
				return;
			const cinsn_t* previous = nullptr;
			for (const cinsn_t& s : *block->cblock)
			{
				if (s.op == cit_for)
					match(previous, &s);
				previous = &s;
			}
		});
	}

private:
	// the loop's parts, as matched
	struct Loop
	{
		const cinsn_t* first = nullptr;  // the init statement before the for, else the for
		const cinsn_t* s = nullptr;		 // the for
		int node = -1;					 // n
		const cexpr_t* list = nullptr;	 // the list, or a pointer to it
		bool pointer = false;
		std::string itemType;			 // the list's item type key
		Value item;						 // n->m_item
		bool stopping = false;
		std::vector<const cinsn_t*> dropped;
	};

	void match(const cinsn_t* previous, const cinsn_t* s)
	{
		const cfor_t& f = *s->cfor;
		Loop loop{s, s, -1, nullptr, false, "", {}, false, {}};
		const cexpr_t* init = &f.init;
		if (init->op == cot_empty && previous && previous->op == cit_expr && previous->cexpr->op == cot_asg)
		{
			init = previous->cexpr;
			loop.first = previous;
		}
		loop.node = init->op == cot_asg ? lvarIndex(init->x) : -1;
		const cinsn_t* body = f.body;
		if (loop.node < 0 || f.step.op != cot_asg || lvarIndex(f.step.x) != loop.node || body->op != cit_block ||
			body->cblock->empty())
			return;
		// the list: `&list` or a pointer to it, of a src BSSimpleList
		const cexpr_t* head = stripCasts(init->y);
		loop.pointer = head->op != cot_ref;
		loop.list = loop.pointer ? head : head->x;
		const SrcModel::Record* record =
			m_context.types.templateRecord(srcType(m_function, m_context, loop.list), "BSSimpleList", loop.pointer);
		const SrcModel::Field* item = record ? record->field("m_item") : nullptr;
		std::optional<Value> node = m_values.value(init->x);
		if (!item || !node || unresolvedIn(m_function, loop.list) || !m_values.is(f.step.y, node->plus(4).load()))
			return;
		loop.itemType = item->type;
		loop.item = node->load();
		// the loop's test: the node, or the node and its item (stopping)
		if (f.expr.op == cot_land && m_values.is(f.expr.x, *node) && m_values.is(f.expr.y, loop.item))
			loop.stopping = true;
		else if (!m_values.is(&f.expr, *node))
			return;
		std::vector<const cinsn_t*> items = childStatements(body);
		// `if (!n->m_item) break;` first: the stopping walk
		if (!loop.stopping && breaksOnNull(items[0], loop.item))
		{
			loop.stopping = true;
			loop.dropped.push_back(items[0]);
		}
		std::vector<const cinsn_t*> statements{s};
		if (loop.first != s)
			statements.push_back(loop.first);
		// the for itself carries no label when the rewrite starts at the init before it (the body's labels print)
		if ((loop.first != s && s->label_num >= 0) || !m_scopes.usedOnlyIn(loop.node, statements))
			return;
		const cinsn_t* read = loop.dropped.size() < items.size() ? items[loop.dropped.size()] : nullptr;
		if (read && read->op == cit_expr && read->cexpr->op == cot_asg && lvarIndex(read->cexpr->x) >= 0 &&
			m_values.is(read->cexpr->y, loop.item))
			byValue(loop, items, read);
		else
			byReference(loop);
	}

	// `v = n->m_item;` (and, not stopping yet, `if (!v) break;`) at the top: `for (T v : list)`. The item read again in
	// the condition of the if right after (`v = n->m_item; if (n->m_item)`, as Hex-Rays shows a test of v's copy)
	// prints as v: nothing between the two writes memory, so the value is the same whether the exe reloads it or not.
	void byValue(Loop& loop, const std::vector<const cinsn_t*>& items, const cinsn_t* read)
	{
		const cexpr_t* local = read->cexpr->x;
		int v = lvarIndex(local);
		loop.dropped.push_back(read);
		const cinsn_t* next = loop.dropped.size() < items.size() ? items[loop.dropped.size()] : nullptr;
		if (!loop.stopping && next && breaksOnNull(next, Value::local(v)))
		{
			loop.stopping = true;
			loop.dropped.push_back(next);
			next = loop.dropped.size() < items.size() ? items[loop.dropped.size()] : nullptr;
		}
		std::vector<const cexpr_t*> rereads;
		if (next && next->op == cit_if && !writes(&next->cif->expr))
			itemReads(&next->cif->expr, loop.item, rereads);
		const cinsn_t* body = loop.s->cfor->body;
		std::optional<std::string> type = m_context.types.spell(m_function.locals[v].type);
		if (!type || !itemLocal(v, loop.itemType, body) || labelledInside(loop.dropped) || nodeUsed(loop, rereads))
			return;
		eraseReplaced(m_function, loop.dropped, {local});
		for (const cexpr_t* e : rereads)
		{
			forEachSubexpression(e, [&](const cexpr_t* x) { m_function.meaning.erase(x); });
			m_function.meaning[e] = Rewrite{1, "$0", {local}, {}};
		}
		m_function.locals[v].declaredAt = nullptr;
		apply(loop, *type + " $0", local);
	}

	// e calls or assigns (could write memory)
	static bool writes(const cexpr_t* e)
	{
		bool any = false;
		forEachSubexpression(e, [&](const cexpr_t* x) { any |= x->op == cot_call || written(x); });
		return any;
	}

	// the body reads n->m_item where it needs it: `for (T& rpItem : list)`, each read printed as rpItem; a list read
	// through a pointer to const (or a const one) yields its items as const: `for (T* const& rpItem : *pList)`
	void byReference(Loop& loop)
	{
		std::vector<const cexpr_t*> reads;
		forEachStatement(loop.s->cfor->body, [&](const cinsn_t* s) {
			if (!dropped(loop, s))
				for (const cexpr_t* own : ownExpressions(s))
					itemReads(own, loop.item, reads);
		});
		bool pointer = m_context.types.parts(m_context.types.canonical(loop.itemType)).kind == "ptr";
		std::string base = pointer ? "rpItem" : "rItem";
		std::string list = srcType(m_function, m_context, loop.list);
		bool constant = loop.pointer ? m_context.types.constPointee(list)
									 : m_context.types.parts(m_context.types.bare(list)).kind == "const";
		std::optional<std::string> type =
			m_context.types.spell(constant ? "const(" + loop.itemType + ")" : loop.itemType);
		if (reads.empty() || !type || nameTaken(base) || labelledInside(loop.dropped) || nodeUsed(loop, reads))
			return;
		// a loop inside or around another one rewritten so takes a name of its own: the inner item would hide the
		// outer one, whose reads in the inner body would print as the inner's (`rpItem_2`)
		std::string name = base;
		for (int k = 2; nameTaken(name) || nestedName(loop.s, name); ++k)
			name = base + "_" + std::to_string(k);
		m_named.emplace_back(loop.s, name);
		eraseReplaced(m_function, loop.dropped, {});
		// each read is the item, of the list's item type (what the conversions convert it from)
		std::string itemKey = constant ? "const(" + loop.itemType + ")" : loop.itemType;
		for (const cexpr_t* e : reads)
		{
			forEachSubexpression(e, [&](const cexpr_t* x) { m_function.meaning.erase(x); });
			m_function.meaning[e] = Rewrite{1, name, {}, {}, itemKey};
		}
		apply(loop, *type + "& " + name, nullptr);
	}

	// the rewrite of the loop (from its init statement): `for (<declaration> : <range>) { <body> }`, $0 in the
	// declaration the item local (when there is one)
	void apply(const Loop& loop, const std::string& declaration, const cexpr_t* local)
	{
		const cinsn_t* body = loop.s->cfor->body;
		std::string range = loop.stopping ? (loop.pointer ? "$1->ItemsUntilNull()" : "$1.ItemsUntilNull()")
										  : (loop.pointer ? "*$1" : "$1");
		std::vector<const cinsn_t*> replaced{loop.s};
		if (loop.first != loop.s)
			replaced.push_back(loop.first);
		std::vector<const citem_t*> items{local ? static_cast<const citem_t*>(local) : loop.list, loop.list, body};
		eraseReplaced(m_function, replaced, items);
		m_function.locals[loop.node].declaredAt = nullptr;
		m_function.meaning[loop.first] = Rewrite{replaced.size(), "for (" + declaration + " : " + range + ") {\n\t$2\n}",
												 items, loop.dropped};
	}

	bool dropped(const Loop& loop, const cinsn_t* s) const
	{
		for (const cinsn_t* d : loop.dropped)
			if (m_scopes.inside(s, d))
				return true;
		return false;
	}

	// the reads of the node's item in e (outermost; not looking inside them)
	void itemReads(const cexpr_t* e, const Value& item, std::vector<const cexpr_t*>& out) const
	{
		bool read = (e->op == cot_memptr || e->op == cot_memref || e->op == cot_ptr) && m_values.is(e, item);
		if (read)
		{
			out.push_back(e);
			return;
		}
		if (e->op == cot_call)
		{
			itemReads(e->x, item, out);
			for (const carg_t& argument : *e->a)
				itemReads(&argument, item, out);
			return;
		}
		if (op_uses_x(e->op) && e->x)
			itemReads(e->x, item, out);
		if (op_uses_y(e->op) && e->y)
			itemReads(e->y, item, out);
		if (op_uses_z(e->op) && e->z)
			itemReads(e->z, item, out);
	}

	// n appears in the body outside the dropped statements and the item reads
	bool nodeUsed(const Loop& loop, const std::vector<const cexpr_t*>& reads) const
	{
		std::set<const cexpr_t*> inRead;
		for (const cexpr_t* e : reads)
			forEachSubexpression(e, [&](const cexpr_t* x) { inRead.insert(x); });
		bool used = false;
		forEachStatement(loop.s->cfor->body, [&](const cinsn_t* s) {
			if (dropped(loop, s))
				return;
			for (const cexpr_t* own : ownExpressions(s))
				forEachSubexpression(own, [&](const cexpr_t* e) {
					used |= lvarIndex(e) == loop.node && !inRead.count(e);
				});
		});
		return used;
	}

	bool nameTaken(const std::string& name) const
	{
		for (const Local& local : m_function.locals)
			if (local.name == name)
				return true;
		return false;
	}

	// a loop rewritten by reference inside or around the for s already names its item so
	bool nestedName(const cinsn_t* s, const std::string& name) const
	{
		auto inside = [](const cinsn_t* outer, const cinsn_t* inner) {
			bool found = false;
			forEachStatement(outer->cfor->body, [&](const cinsn_t* x) { found |= x == inner; });
			return found;
		};
		return std::any_of(m_named.begin(), m_named.end(), [&](const auto& named) {
			return named.second == name && (inside(named.first, s) || inside(s, named.first));
		});
	}

	// `if (!item) break;`
	bool breaksOnNull(const cinsn_t* s, const Value& item) const
	{
		if (s->op != cit_if || s->cif->ielse || s->cif->expr.op != cot_lnot || !m_values.is(s->cif->expr.x, item))
			return false;
		const cinsn_t* then = soleStatement(s->cif->ithen);
		return then && then->op == cit_break;
	}

	// v, the loop's item local: the list's item type, used only in the body, its address never taken
	bool itemLocal(int v, const std::string& itemType, const cinsn_t* body) const
	{
		const Local& local = m_function.locals[v];
		return !(*m_function.cfunc->get_lvars())[v].is_arg_var() && !local.type.empty() && !local.inlined &&
			   local.aliasOf < 0 && m_context.types.canonical(local.type) == m_context.types.canonical(itemType) &&
			   m_scopes.usedOnlyIn(v, {body}) && !m_scopes.addressTaken(v);
	}

	Function& m_function;
	Context& m_context;
	Values m_values;
	Scopes m_scopes;
	// the loops rewritten by reference, each with its item's name
	std::vector<std::pair<const cinsn_t*, std::string>> m_named;
};

}  // namespace

void rewriteSimpleListLoops(Function& function, Context& context)
{
	Loops(function, context).run();
}

}  // namespace idakit
