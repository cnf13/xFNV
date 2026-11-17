// Aliases: a local that is only a copy of another (`v8 = pkManager;` and v8 read after) is that local. The copy is
// assigned once, by an expression statement, from a plain local; neither is written while both are alive (no write
// of the original after the copy, none in a loop around the copy); neither's address is taken; the copy is not
// possibly undefined (Hex-Rays; its read before the copy stays), nor its copy statement marked (markUnreadStores: a
// store a callee may read through a neighbour's address); and the two have the identical type (IDA's and src's: a
// derived/base pair or any other difference changes name lookup, overloads and, at a non-zero base offset, the
// pointer). The copy prints as the original; its copy statement and declaration go.

#include "EmitScopes.hpp"

#include <map>

namespace idakit
{

namespace
{

// the local e writes (`v = `, `v += `, `++v`) or takes the address of; -1 for none
int writtenLocal(const cexpr_t* e)
{
	return written(e) || e->op == cot_ref ? lvarIndex(e->x) : -1;
}

class Aliases
{
public:
	explicit Aliases(Function& function) : m_function(function), m_scopes(function)
	{
		// statements in source order
		forEachStatement(&function.cfunc->body, [&](const cinsn_t* s) { m_order.emplace(s, m_order.size()); });
		forEachExpression(function, [&](const cexpr_t* e, const cinsn_t* s) {
			if (int v = writtenLocal(e); v >= 0)
				m_writes[v].push_back({e, s});
		});
	}

	void run()
	{
		const lvars_t& lvars = *m_function.cfunc->get_lvars();
		for (int copy = 0; copy < static_cast<int>(m_function.locals.size()); ++copy)
		{
			// assigned once, by `copy = original;`
			const std::vector<Write>& assigned = m_writes[copy];
			if (lvars[copy].is_arg_var() || assigned.size() != 1 || assigned[0].e->op != cot_asg ||
				assigned[0].s->op != cit_expr || assigned[0].s->cexpr != assigned[0].e)
				continue;
			int original = lvarIndex(assigned[0].e->y);
			if (original < 0 || original == copy || m_function.locals[original].aliasOf >= 0)
				continue;
			const Local& a = m_function.locals[copy];
			const Local& b = m_function.locals[original];
			// a copy Hex-Rays reports possibly undefined is read on a path before the copy: as the original, that read
			// would read the original's value, and the uninitialised read's marker would go (RULES' KEEP); a marked
			// copy statement would lose its marker
			auto mark = m_function.meaning.find(assigned[0].e);
			if (a.type.empty() || a.type != b.type || a.reference != b.reference || a.inlined || b.inlined ||
				a.undefined || !lvars[copy].type().equals_to(lvars[original].type()) ||
				(mark != m_function.meaning.end() && std::holds_alternative<Missing>(mark->second)))
				continue;
			if (!unchangedAfter(original, assigned[0].s))
				continue;
			m_function.locals[copy].aliasOf = original;
			m_function.locals[copy].copiedAt = assigned[0].s;
		}
	}

private:
	struct Write
	{
		const cexpr_t* e;
		const cinsn_t* s;
	};

	// v is not written after the statement, nor in a loop around it, nor has its address taken
	bool unchangedAfter(int v, const cinsn_t* at) const
	{
		auto found = m_writes.find(v);
		if (found == m_writes.end())
			return true;
		for (const Write& w : found->second)
		{
			if (w.e->op == cot_ref || m_order.at(w.s) > m_order.at(at))
				return false;
			for (const cinsn_t* p = m_scopes.parent(at); p; p = m_scopes.parent(p))
				if ((p->op == cit_for || p->op == cit_while || p->op == cit_do) && m_scopes.inside(w.s, p))
					return false;
		}
		return true;
	}

	Function& m_function;
	Scopes m_scopes;
	std::unordered_map<const cinsn_t*, size_t> m_order;
	std::map<int, std::vector<Write>> m_writes;
};

}  // namespace

void resolveAliases(Function& function, Context&)
{
	Aliases(function).run();
}

}  // namespace idakit
