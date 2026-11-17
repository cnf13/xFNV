// Values: what an expression reads, as an address expression, for the idiom passes' shape matching. Hex-Rays shows
// one value many ways (`&old->m_uiRefCount`, `old + 1`, a cast, a copy of a local, a call of a trivial accessor);
// equal keys read the same value wherever nothing in between writes, which each idiom's shape guarantees.

#include "EmitPasses.hpp"

namespace idakit
{

std::optional<Value> Values::value(const cexpr_t* e) const
{
	switch (e->op)
	{
	case cot_cast:
		return value(e->x);
	case cot_var:
	{
		// a copy of a local is that local
		int v = e->v.idx;
		while (m_function.locals[v].aliasOf >= 0)
			v = m_function.locals[v].aliasOf;
		return Value::local(v);
	}
	case cot_ref:
		return location(e->x);
	case cot_add:
	{
		std::optional<Value> base = value(e->x);
		if (!base || e->y->op != cot_num)
			return std::nullopt;
		int64_t size = e->x->type.is_ptr() ? e->x->type.get_ptrarr_objsize() : e->x->type.is_integral() ? 1 : 0;
		if (size <= 0)
			return std::nullopt;
		return base->plus(static_cast<int64_t>(e->y->numval()) * size);
	}
	case cot_call:
	{
		// a trivial accessor: its object plus the offset, loaded (a whole dword) or not (a setter has no value)
		std::optional<AccessorCall> accessor = accessorCall(m_context, e);
		if (!accessor || accessor->value || accessor->x87 || (accessor->load && accessor->width != 4))
			return std::nullopt;
		std::optional<Value> object = value(accessor->receiver);
		if (!object)
			return std::nullopt;
		Value at = object->plus(accessor->offset);
		return accessor->load ? at.load() : at;
	}
	case cot_ptr:
	case cot_memptr:
	case cot_memref:
	{
		std::optional<Value> where = location(e);
		return where ? std::optional(where->load()) : std::nullopt;
	}
	default:
		return std::nullopt;
	}
}

std::optional<Value> Values::location(const cexpr_t* e) const
{
	switch (e->op)
	{
	case cot_ptr:
		return value(e->x);
	case cot_memptr:
	{
		std::optional<Value> object = value(e->x);
		return object ? std::optional(object->plus(e->m)) : std::nullopt;
	}
	case cot_memref:
	{
		std::optional<Value> object = location(e->x);
		return object ? std::optional(object->plus(e->m)) : std::nullopt;
	}
	case cot_var:
		// the local's own storage (a copy's is its own, unlike its value)
		return Value{"&v" + std::to_string(e->v.idx), 0};
	default:
		return std::nullopt;
	}
}

}  // namespace idakit
