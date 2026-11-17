// idakit infer's facts (Infer.hpp), read from the emitter's analysis of one function: at each call of a target, where
// each argument's value comes from (a source) and what the caller does with the result; in a target's own body,
// what it does with each parameter (uses) and where each returned value comes from. The ctree is read for its
// shapes (offsets, widths, operations), never for Hex-Rays' types of the values, which a guess declaration gave.

#include "EmitPasses.hpp"

#include "idakit/Infer.hpp"

namespace idakit
{

namespace
{

using Source = Infer::Source;
using Use = Infer::Use;

int64_t sizeOf(const tinfo_t& type)
{
	size_t size = type.get_size();
	return size == BADSIZE ? 0 : static_cast<int64_t>(size);
}

// the bytes one step of pointer arithmetic on e moves by: its pointee's size, 1 for an integer (bytes)
int64_t scale(const cexpr_t* e)
{
	if (e->type.is_ptr())
	{
		size_t size = e->type.get_ptrarr_objsize();
		return size == BADSIZE || size == 0 ? 1 : static_cast<int64_t>(size);
	}
	return 1;
}

// a value plus constants: the value past its casts and the bytes added (`(char*)p + 8`, `p + 2` of a DWORD*)
std::pair<const cexpr_t*, int64_t> pointerPlus(const cexpr_t* e)
{
	int64_t offset = 0;
	e = stripCasts(e);
	while ((e->op == cot_add || e->op == cot_sub) && e->y->op == cot_num)
	{
		int64_t n = static_cast<int64_t>(e->y->numval()) * scale(e->x);
		offset += e->op == cot_add ? n : -n;
		e = stripCasts(e->x);
	}
	return {e, offset};
}

// The storage an lvalue names: through a pointer (pointer set) or inside a local (local set), at offset bytes.
struct Place
{
	const cexpr_t* pointer = nullptr;
	int local = -1;
	int64_t offset = 0;
};

std::optional<Place> placeOf(const cexpr_t* e)
{
	switch (e->op)
	{
	case cot_memptr:
	{
		auto [pointer, offset] = pointerPlus(e->x);
		return Place{pointer, -1, offset + static_cast<int64_t>(e->m)};
	}
	case cot_ptr:
	{
		auto [pointer, offset] = pointerPlus(e->x);
		return Place{pointer, -1, offset};
	}
	case cot_memref:
	{
		if (e->x->op == cot_var)
			return Place{nullptr, e->x->v.idx, static_cast<int64_t>(e->m)};
		std::optional<Place> inner = placeOf(e->x);
		if (inner)
			inner->offset += static_cast<int64_t>(e->m);
		return inner;
	}
	case cot_idx:
	{
		if (e->y->op != cot_num)
			return std::nullopt;
		int64_t step = sizeOf(e->type) * static_cast<int64_t>(e->y->numval());
		if (e->x->type.is_array())
		{
			std::optional<Place> inner = e->x->op == cot_var ? Place{nullptr, e->x->v.idx, 0} : placeOf(e->x);
			if (inner)
				inner->offset += step;
			return inner;
		}
		auto [pointer, offset] = pointerPlus(e->x);
		return Place{pointer, -1, offset + step};
	}
	default:
		return std::nullopt;
	}
}

class Reader
{
public:
	// target: a target's declaration by address, nullptr for any other function
	Reader(const Function& function, Context& context,
		   const std::function<const SrcModel::Function*(uint64_t)>& target)
		: m_function(function), m_context(context), m_target(target), m_parents(parentsOf(function))
	{
		// the declaration the function's own parameters are read through: a target's, else the one Self takes
		const SrcModel::Function* declaration = target(function.ea);
		if (!declaration)
		{
			RefChoice choice = chooseRef(context.model, function.ea);
			if (choice.ref && (choice.names.size() <= 1 || choice.ref->kind == "body"))
				declaration = context.model.function(choice.ref->function);
		}
		if (declaration)
			if (std::optional<ArgumentLvars> lvars = argumentLvars(function, context.model, *declaration))
				m_lvars = *lvars;
		const lvars_t& lvars = *function.cfunc->get_lvars();
		m_writes.assign(lvars.size(), 0);
		m_assignment.assign(lvars.size(), nullptr);
		m_occurrences.resize(lvars.size());
		forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
			if (e->op == cot_var)
				m_occurrences[root(e->v.idx)].push_back(e);
			// a write to the local or a part of it, or its address taken (but to read it as another type,
			// `*(float*)&v`)
			if ((!written(e) && e->op != cot_ref) || (e->op == cot_ref && reinterpreted(e)))
				return;
			const cexpr_t* x = e->x;
			while (x->op == cot_memref || x->op == cot_cast || (x->op == cot_idx && x->x->type.is_array()))
				x = x->x;
			if (x->op != cot_var)
				return;
			int v = root(x->v.idx);
			++m_writes[v];
			if (e->op == cot_asg && e->x->op == cot_var)
				m_assignment[v] = e;
		});
	}

	// the lvar of a parameter (src's index), -1 for none
	int parameterLvar(int slot) const
	{
		return slot >= 0 && static_cast<size_t>(slot) < m_lvars.parameters.size() ? m_lvars.parameters[slot] : -1;
	}

	// the local a copy of a local stands for (the Aliases pass's original)
	int root(int v) const
	{
		while (v >= 0 && static_cast<size_t>(v) < m_function.locals.size() && m_function.locals[v].aliasOf >= 0)
			v = m_function.locals[v].aliasOf;
		return v;
	}

	bool rebound(int v) const { return m_writes[root(v)] > 0; }

	const std::vector<const cexpr_t*>& occurrences(int v) const { return m_occurrences[root(v)]; }

	// where the value of e comes from
	Source source(const cexpr_t* e, int depth = 0) const
	{
		Source out;
		out.at = e->ea;
		if (depth > 6)
			return out;
		e = stripCasts(e);
		switch (e->op)
		{
		case cot_var:
		{
			int v = root(e->v.idx);
			const lvar_t& lvar = (*m_function.cfunc->get_lvars())[v];
			if (lvar.is_arg_var())
			{
				if (m_writes[v] != 0)
					return out;
				if (v == m_lvars.thisLvar)
				{
					out.kind = Source::Kind::self;
					out.function = m_function.ea;
					return out;
				}
				for (size_t k = 0; k < m_lvars.parameters.size(); ++k)
					if (m_lvars.parameters[k] == v)
					{
						out.kind = Source::Kind::parameter;
						out.function = m_function.ea;
						out.slot = static_cast<int>(k);
						return out;
					}
				return out;
			}
			// a local written once, by a plain assignment: that value
			if (m_writes[v] == 1 && m_assignment[v])
				return source(m_assignment[v]->y, depth + 1);
			return out;
		}
		case cot_call:
		{
			const cexpr_t* callee = stripCasts(e->x);
			if (callee->op != cot_obj || !functionStart(callee))
				return out;
			if (std::optional<AccessorCall> accessor = accessorCall(m_context, e))
			{
				if (accessor->value || accessor->x87)
					return out;
				auto receiver = std::make_shared<Source>(source(accessor->receiver, depth + 1));
				if (accessor->load)
				{
					out.kind = Source::Kind::field;
					out.width = accessor->width;
				}
				else if (accessor->offset == 0)
					return *receiver;
				else
					out.kind = Source::Kind::address;
				out.receiver = receiver;
				out.offset = accessor->offset;
				return out;
			}
			out.kind = Source::Kind::result;
			out.function = callee->obj_ea;
			return out;
		}
		case cot_memptr:
		case cot_ptr:
		case cot_memref:
		case cot_idx:
		{
			std::optional<Place> place = placeOf(e);
			if (!place)
				return out;
			out.kind = Source::Kind::field;
			out.receiver = std::make_shared<Source>(placeSource(*place, depth));
			out.offset = place->offset;
			out.width = sizeOf(e->type);
			return out;
		}
		case cot_ref:
		{
			const cexpr_t* x = e->x;
			if (x->op == cot_var)
				return object(root(x->v.idx), e->ea);
			if (x->op == cot_obj)
			{
				out.global = globalRef(x);
				if (out.global)
					out.kind = Source::Kind::globalAddress;
				return out;
			}
			std::optional<Place> place = placeOf(x);
			if (!place)
				return out;
			out.kind = Source::Kind::address;
			out.receiver = std::make_shared<Source>(placeSource(*place, depth));
			out.offset = place->offset;
			out.width = sizeOf(x->type);
			return out;
		}
		case cot_add:
		case cot_sub:
		{
			if (e->y->op != cot_num)
				return out;
			auto [base, offset] = pointerPlus(e);
			Source inner = source(base, depth + 1);
			inner.adjust += offset;
			return inner;
		}
		case cot_tern:
			// `p ? p + 8 : 0`: the conversion to a base at a nonzero offset
			if (isZero(e->z))
				return source(e->y, depth + 1);
			if (isZero(e->y))
				return source(e->z, depth + 1);
			return out;
		case cot_num:
			out.kind = Source::Kind::number;
			out.offset = static_cast<int64_t>(e->numval());
			return out;
		case cot_obj:
			if (functionStart(e))
				return out;
			out.global = globalRef(e);
			if (out.global)
				out.kind = Source::Kind::global;
			return out;
		default:
			return out;
		}
	}

	// what is done with the value of e (plus adjust bytes); eax: e is a call's result Hex-Rays has in eax (an integer
	// or pointer), whose bits read as a float (`COERCE_FLOAT(f())`, `*(float*)&v` of a local holding it) are no float
	// use: the exe returns a float in ST0, never in eax, so that read is Hex-Rays' reuse of the register's local
	void uses(const cexpr_t* e, int64_t adjust, int depth, std::vector<Use>& out, bool eax = false) const
	{
		// what e itself is plus: an access's place counts the constants added past e (placeOf), not those before
		const int64_t entry = adjust;
		const cexpr_t* cur = e;
		while (true)
		{
			auto found = m_parents.find(cur);
			if (found == m_parents.end() || !found->second)
				return;
			const citem_t* parent = found->second;
			if (!parent->is_expr())
			{
				const cinsn_t* s = static_cast<const cinsn_t*>(parent);
				if (s->op == cit_return)
					out.push_back(use(Use::Kind::returned, cur, adjust, [&](Use& u) { u.callee = m_function.ea; }));
				else if (s->op == cit_switch)
					out.push_back(integer(cur, 0));
				return;
			}
			const cexpr_t* p = static_cast<const cexpr_t*>(parent);
			const cexpr_t* other = p->x == cur ? (op_uses_y(p->op) ? p->y : nullptr) : p->x;
			switch (p->op)
			{
			case cot_cast:
			{
				bool from = cur->type.is_floating(), to = p->type.is_floating();
				if (from != to && (cur->type.is_integral() || p->type.is_integral()))
				{
					// a conversion between an integer and a float: the value is the type it converts from
					out.push_back(from ? floating(cur) : integer(cur, 0));
					return;
				}
				if (cur->type.is_integral() && p->type.is_integral() && sizeOf(p->type) < sizeOf(cur->type))
				{
					out.push_back(integer(cur, 0, sizeOf(p->type)));
					return;
				}
				cur = p;
				continue;
			}
			case cot_add:
			case cot_sub:
				if (p->x == cur && p->y->op == cot_num)
				{
					int64_t n = static_cast<int64_t>(p->y->numval()) * scale(cur);
					adjust += p->op == cot_add ? n : -n;
					cur = p;
					continue;
				}
				// a pointer plus a variable index is an element at an unknown offset
				if (cur->type.is_ptr())
					out.push_back(use(Use::Kind::access, cur, adjust, nullptr));
				else if (cur->type.is_floating())
					out.push_back(floating(cur));
				else
					out.push_back(integer(cur, 0));
				return;
			case cot_ptr:
			case cot_memptr:
			case cot_idx:
				if (p->x != cur)
				{
					if (p->op == cot_idx)
						out.push_back(integer(cur, 0));
					return;
				}
				access(p, entry, out);
				return;
			case cot_ref:
				// the parameter's own storage read as another type (`*(float*)&a2`)
				if (const cexpr_t* read = reinterpreted(p); read && !(eax && read->type.is_floating()))
					out.push_back(read->type.is_floating() ? floating(cur) : integer(cur, 0, sizeOf(read->type)));
				return;
			case cot_call:
			{
				if (p->x == cur)
					return;
				if (p->x->op == cot_helper)
				{
					std::string_view helper = p->x->helper;
					if (helper.starts_with("COERCE_FLOAT") || helper.starts_with("COERCE_DOUBLE"))
					{
						if (!eax)
							out.push_back(floating(cur));
					}
					else if (helper.starts_with("LO") || helper.starts_with("HI") || helper.starts_with("BYTE") ||
							 helper.starts_with("WORD") || helper.starts_with("SLO") || helper.starts_with("SHI"))
						out.push_back(integer(cur, 0, sizeOf(p->type)));
					return;
				}
				pass(p, cur, adjust, out);
				return;
			}
			case cot_asg:
			{
				if (p->y != cur)
					return;
				const cexpr_t* left = p->x;
				if (left->op == cot_var)
				{
					int v = root(left->v.idx);
					const lvar_t& lvar = (*m_function.cfunc->get_lvars())[v];
					if (!lvar.is_arg_var() && m_writes[v] == 1 && depth < 3)
						for (const cexpr_t* occurrence : occurrences(v))
							if (occurrence != left)
								uses(occurrence, adjust, depth + 1, out, eax);
					return;
				}
				auto destination = std::make_shared<Source>(source(left));
				if (destination->kind != Source::Kind::none)
					out.push_back(use(Use::Kind::store, cur, adjust, [&](Use& u) { u.destination = destination; }));
				return;
			}
			case cot_eq:
			case cot_ne:
			{
				if (cur->type.is_floating())
				{
					out.push_back(floating(cur));
					return;
				}
				if (stripCasts(other)->op == cot_num)
				{
					if (!isZero(other))
						out.push_back(integer(cur, 0));
					return;
				}
				auto destination = std::make_shared<Source>(source(other));
				if (destination->kind != Source::Kind::none)
					out.push_back(use(Use::Kind::compare, cur, adjust, [&](Use& u) { u.destination = destination; }));
				return;
			}
			case cot_slt:
			case cot_sle:
			case cot_sgt:
			case cot_sge:
				out.push_back(cur->type.is_floating() ? floating(cur) : integer(cur, 1));
				return;
			case cot_ult:
			case cot_ule:
			case cot_ugt:
			case cot_uge:
				// pointers compare unsigned: only a compare with a number or an integer says integer
				if (cur->type.is_floating())
					out.push_back(floating(cur));
				else if (stripCasts(other)->op == cot_num || !stripCasts(other)->type.is_ptr())
					out.push_back(integer(cur, -1));
				return;
			case cot_sdiv:
			case cot_smod:
			case cot_sshr:
				out.push_back(integer(cur, 1));
				return;
			case cot_udiv:
			case cot_umod:
			case cot_ushr:
				out.push_back(integer(cur, -1));
				return;
			case cot_mul:
			case cot_shl:
			case cot_band:
			case cot_bor:
			case cot_xor:
			case cot_bnot:
			case cot_neg:
			case cot_asgadd:
			case cot_asgsub:
				out.push_back(cur->type.is_floating() ? floating(cur) : integer(cur, 0));
				return;
			case cot_fadd:
			case cot_fsub:
			case cot_fmul:
			case cot_fdiv:
			case cot_fneg:
				out.push_back(floating(cur));
				return;
			case cot_tern:
			case cot_comma:
				if (p->x == cur)
					return;
				cur = p;
				continue;
			default:
				return;
			}
		}
	}

	const Function& function() const { return m_function; }

	// the src slot a call's argument k fills (Infer::kThis, a parameter's index), -3 for none (the hidden result
	// pointer, an argument past the parameters)
	int slotOfArgument(const cexpr_t* call, const SrcModel::Function& f, size_t k) const
	{
		const SrcModel::Type* type = m_context.model.type(f.type);
		if (!type)
			return -3;
		if (auto found = m_function.meaning.find(call); found != m_function.meaning.end())
			if (const Call* resolved = std::get_if<Call>(&found->second); resolved && resolved->function == &f)
			{
				int parameter = parameterIndex(*resolved, k);
				if (parameter < 0)
					return k == 0 && !f.cls.empty() && !f.isStatic ? Infer::kThis : -3;
				return static_cast<size_t>(parameter) < type->params.size() ? parameter : -3;
			}
		int member = !f.cls.empty() && !f.isStatic ? 1 : 0;
		if (static_cast<int>(k) < member)
			return Infer::kThis;
		const SrcModel::Abi* abi = m_context.model.abi(f.mangled);
		int index = static_cast<int>(k) - member;
		if (abi && abi->ret == "indirect" && index-- == 0)
			return -3;
		return static_cast<size_t>(index) < type->params.size() ? index : -3;
	}

private:
	Use use(Use::Kind kind, const cexpr_t* at, int64_t adjust, const std::function<void(Use&)>& fill) const
	{
		Use u;
		u.kind = kind;
		u.at = at->ea != BADADDR ? at->ea : m_function.ea;
		u.adjust = adjust;
		if (fill)
			fill(u);
		return u;
	}

	Use integer(const cexpr_t* at, int signedness, int64_t width = 0) const
	{
		return use(Use::Kind::integer, at, 0, [&](Use& u) {
			u.signedness = signedness;
			u.width = width ? width : sizeOf(at->type);
		});
	}

	Use floating(const cexpr_t* at) const
	{
		return use(Use::Kind::floating, at, 0, [&](Use& u) { u.width = sizeOf(at->type); });
	}

	// the first parent of e that is no cast, and the outermost cast below it (e where there is none)
	std::pair<const citem_t*, const cexpr_t*> above(const cexpr_t* e) const
	{
		while (true)
		{
			auto found = m_parents.find(e);
			const citem_t* parent = found == m_parents.end() ? nullptr : found->second;
			if (!parent || !parent->is_expr() || static_cast<const cexpr_t*>(parent)->op != cot_cast)
				return {parent, e};
			e = static_cast<const cexpr_t*>(parent);
		}
	}

	// `*(T*)&v`: the dereference reading v's storage as another type; nullptr for another shape
	const cexpr_t* reinterpreted(const cexpr_t* ref) const
	{
		auto [parent, top] = above(ref);
		if (!parent || !parent->is_expr())
			return nullptr;
		const cexpr_t* p = static_cast<const cexpr_t*>(parent);
		// not a write through it (`*(int*)&v = x`)
		if (p->op != cot_ptr || p->x != top || top == ref)
			return nullptr;
		auto found = m_parents.find(p);
		const citem_t* above = found == m_parents.end() ? nullptr : found->second;
		bool write = above && above->is_expr() && written(static_cast<const cexpr_t*>(above)) &&
					 static_cast<const cexpr_t*>(above)->x == p;
		return write ? nullptr : p;
	}

	// a read or write through the value: the innermost member the access names (a memptr's memrefs), its offset
	// from the value and its width; a dword read that is then a vtable's slot called is a virtual call
	void access(const cexpr_t* p, int64_t entry, std::vector<Use>& out) const
	{
		std::optional<Place> place = placeOf(p);
		if (!place)
		{
			// an index by a variable: an element at an unknown offset
			out.push_back(use(Use::Kind::access, p, 0, nullptr));
			return;
		}
		// the value's own offset is the walk's adjust, which placeOf has from the pointer plus constants; place's
		// pointer is the value itself past them
		const cexpr_t* node = p;
		int64_t offset = entry + place->offset;
		while (true)
		{
			auto found = m_parents.find(node);
			const citem_t* parent = found == m_parents.end() ? nullptr : found->second;
			if (!parent || !parent->is_expr())
				break;
			const cexpr_t* q = static_cast<const cexpr_t*>(parent);
			if (q->op != cot_memref || q->x != node)
				break;
			offset += static_cast<int64_t>(q->m);
			node = q;
		}
		auto found = m_parents.find(node);
		const citem_t* parent = found == m_parents.end() ? nullptr : found->second;
		bool address = parent && parent->is_expr() && static_cast<const cexpr_t*>(parent)->op == cot_ref;
		bool write = parent && parent->is_expr() && (written(static_cast<const cexpr_t*>(parent))) &&
					 static_cast<const cexpr_t*>(parent)->x == node;
		int64_t width = sizeOf(node->type);
		if (!address && !write && width == 4)
			if (std::optional<std::pair<int64_t, const cexpr_t*>> slot = calledSlot(node))
			{
				out.push_back(use(Use::Kind::vcall, p, 0, [&](Use& u) {
					u.offset = offset;
					u.slotOffset = slot->first;
				}));
				// a virtual whose result picks a branch may be a class test (IsActor, GetType)
				if (decides(slot->second, true))
					out.push_back(use(Use::Kind::typeTest, p, 0, nullptr));
				return;
			}
		out.push_back(use(Use::Kind::access, p, 0, [&](Use& u) {
			u.offset = offset;
			u.width = address ? 0 : width;
			u.write = write;
		}));
		// a member switched on or compared with a constant may be a class's type tag (hkpShape's m_type)
		if (!address && !write && decides(node, false))
			out.push_back(use(Use::Kind::typeTest, p, 0, nullptr));
	}

	// The value picks a branch by what it is: a switch on it, a compare with a nonzero number; truth: also a test
	// of it (an if's condition, `!v`, `v && ...`).
	bool decides(const cexpr_t* value, bool truth) const
	{
		auto [parent, top] = above(value);
		if (!parent)
			return false;
		if (!parent->is_expr())
		{
			int op = static_cast<const cinsn_t*>(parent)->op;
			return op == cit_switch || (truth && (op == cit_if || op == cit_while || op == cit_do || op == cit_for));
		}
		const cexpr_t* p = static_cast<const cexpr_t*>(parent);
		if (p->op == cot_eq || p->op == cot_ne)
		{
			const cexpr_t* other = stripCasts(p->x == top ? p->y : p->x);
			return other->op == cot_num && !isZero(other);
		}
		return truth && (p->op == cot_lnot || p->op == cot_land || p->op == cot_lor || p->op == cot_tern);
	}

	// the loaded dword is a vtable whose slot (its byte offset) a call calls: `(*(F**)(*p + 8))(...)`,
	// `p->__vftable->f(...)`; the slot and the call
	std::optional<std::pair<int64_t, const cexpr_t*>> calledSlot(const cexpr_t* vtable) const
	{
		auto [parent, top] = above(vtable);
		if (!parent || !parent->is_expr())
			return std::nullopt;
		const cexpr_t* p = static_cast<const cexpr_t*>(parent);
		const cexpr_t* slotRead = nullptr;
		int64_t slot = 0;
		if (p->op == cot_memptr && p->x == top)
		{
			slotRead = p;
			slot = static_cast<int64_t>(p->m);
		}
		else if (p->op == cot_ptr && p->x == top)
			slotRead = p;
		else if (p->op == cot_idx && p->x == top && p->y->op == cot_num)
		{
			slotRead = p;
			slot = static_cast<int64_t>(p->y->numval()) * sizeOf(p->type);
		}
		else if (p->op == cot_add && p->x == top && p->y->op == cot_num)
		{
			slot = static_cast<int64_t>(p->y->numval()) * scale(top);
			auto [grand, top2] = above(p);
			if (grand && grand->is_expr() && static_cast<const cexpr_t*>(grand)->op == cot_ptr &&
				static_cast<const cexpr_t*>(grand)->x == top2)
				slotRead = static_cast<const cexpr_t*>(grand);
		}
		if (!slotRead)
			return std::nullopt;
		auto [call, callee] = above(slotRead);
		if (!call || !call->is_expr() || static_cast<const cexpr_t*>(call)->op != cot_call ||
			static_cast<const cexpr_t*>(call)->x != callee)
			return std::nullopt;
		return std::pair(slot, static_cast<const cexpr_t*>(call));
	}

	// the value passed as argument of a direct call of a src function: that function's slot
	void pass(const cexpr_t* call, const cexpr_t* argument, int64_t adjust, std::vector<Use>& out) const
	{
		const cexpr_t* callee = stripCasts(call->x);
		if (callee->op != cot_obj || !functionStart(callee))
			return;
		size_t k = 0;
		while (k < call->a->size() && &(*call->a)[k] != argument)
			++k;
		if (k == call->a->size())
			return;
		Use u = use(Use::Kind::pass, argument, adjust, nullptr);
		u.callee = callee->obj_ea;
		// a class test of the value (NiIsKindOf with a class's ms_RTTI, a dynamic cast): a downcast may follow
		qstring calleeName;
		get_func_name(&calleeName, callee->obj_ea);
		std::string name = calleeName.c_str();
		bool rtti = name.find("IsKindOf") != std::string::npos || name.find("IsExactKindOf") != std::string::npos ||
					name.find("DynamicCast") != std::string::npos;
		for (const carg_t& other : *call->a)
			if (const cexpr_t* o = stripCasts(&other); o->op == cot_ref && o->x->op == cot_obj)
			{
				qstring global;
				get_name(&global, o->x->obj_ea);
				rtti = rtti || std::string_view(global.c_str()).find("ms_RTTI") != std::string_view::npos;
			}
		if (rtti)
			out.push_back(use(Use::Kind::typeTest, argument, 0, nullptr));
		// a target's slots are each round's proposals, read through its declaration's shape (u.function null);
		// any other callee is the src function Calls chooses (chosenCallee reads call->x as the obj)
		const SrcModel::Function* shape = m_target(u.callee);
		if (!shape)
		{
			if (call->x->op != cot_obj)
				return;
			ChosenCallee chosen = chosenCallee(m_function, m_context.model, call);
			if (!chosen.function)
				return;
			u.function = shape = chosen.function;
		}
		u.slot = slotOfArgument(call, *shape, k);
		if (u.slot == -3)
			return;
		// a fold owner chosen by the receiver's Hex-Rays type says nothing of the receiver
		if (u.slot == Infer::kThis && m_function.namedCallees.count(call))
			return;
		out.push_back(u);
	}

	Source placeSource(const Place& place, int depth) const
	{
		if (place.pointer)
			return source(place.pointer, depth + 1);
		return object(root(place.local), m_function.ea);
	}

	// The address of a local object a constructor call builds (FNVD_CONSTRUCTED_LOCAL's X::Construct): the
	// constructor's address goes with it (a fold is no evidence), and only a class with a vtable is one (a
	// constructor without may be the linker's fold of another class's of the same code: NiPoint3's and NiColor's).
	// The vtable stores of an inlined constructor are not taken: the local's class there is Hex-Rays' type.
	Source object(int v, uint64_t at) const
	{
		Source out;
		out.at = at;
		if (v < 0 || static_cast<size_t>(v) >= m_function.locals.size())
			return out;
		const Local& local = m_function.locals[v];
		if (!local.inlined || !local.inlined->construct)
			return out;
		const cexpr_t* callee = stripCasts(local.inlined->construct->x);
		SrcTypes::Parts p = m_context.types.parts(m_context.types.canonical(local.type));
		const SrcModel::Record* record = p.kind == "record" ? m_context.model.record(p.name) : nullptr;
		if (callee->op != cot_obj || !record || record->vtables.empty())
			return out;
		out.kind = Source::Kind::object;
		out.key = local.type;
		out.function = callee->obj_ea;
		return out;
	}

	const SrcModel::Ref* globalRef(const cexpr_t* obj) const
	{
		if (auto found = m_function.meaning.find(obj); found != m_function.meaning.end())
			if (const SrcModel::Ref* const* ref = std::get_if<const SrcModel::Ref*>(&found->second))
				return *ref;
		for (const SrcModel::Ref& ref : m_context.model.refsAt(obj->obj_ea))
			if (ref.kind == "global" || ref.kind == "static_member")
				return &ref;
		return nullptr;
	}

	const Function& m_function;
	Context& m_context;
	const std::function<const SrcModel::Function*(uint64_t)>& m_target;
	std::unordered_map<const cexpr_t*, const citem_t*> m_parents;
	std::vector<int> m_writes;
	std::vector<const cexpr_t*> m_assignment;
	std::vector<std::vector<const cexpr_t*>> m_occurrences;
	ArgumentLvars m_lvars;
};

}  // namespace

Infer::Infer(Context& context, std::set<uint64_t> hidden) : m_context(context), m_hidden(std::move(hidden)) {}

void Infer::target(uint64_t address, const SrcModel::Function& declaration)
{
	Target& t = m_targets[address];
	t.declaration = &declaration;
	// the slots srcdb's function_slot lists (a record returned by value is the hidden result pointer the caller
	// passes, the porting forms' Construct `apMemory` and DeletingDestruct's `this` are none); `this` a note (an
	// owner question)
	for (const std::string& slot : declaration.slots)
		t.slots[slot == "ret" ? kReturn : std::stoi(slot)];
	if (!declaration.cls.empty() && !declaration.isStatic)
		t.slots[kThis];
}

void Infer::read(const Function& function)
{
	std::function<const SrcModel::Function*(uint64_t)> declared = [&](uint64_t address) -> const SrcModel::Function* {
		auto found = m_targets.find(address);
		return found == m_targets.end() ? nullptr : found->second.declaration;
	};
	Reader reader(function, m_context, declared);
	// the calls of targets: each argument's source, the result's uses
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
		if (e->op != cot_call)
			return;
		const cexpr_t* callee = stripCasts(e->x);
		if (callee->op != cot_obj)
			return;
		auto found = m_targets.find(callee->obj_ea);
		if (found == m_targets.end() || !found->second.declaration)
			return;
		Target& target = found->second;
		if (callee->obj_ea != function.ea)
			target.callers.insert(function.ea);
		for (size_t k = 0; k < e->a->size(); ++k)
		{
			int slot = reader.slotOfArgument(e, *target.declaration, k);
			auto s = target.slots.find(slot);
			if (slot < 0 || s == target.slots.end())
				continue;
			Source source = reader.source(&(*e->a)[k]);
			source.at = e->ea;
			s->second.below.push_back(std::move(source));
		}
		if (auto r = target.slots.find(kReturn); r != target.slots.end())
			reader.uses(e, 0, 0, r->second.uses, !e->type.is_floating());
	});
	// a target's own body
	auto self = m_targets.find(function.ea);
	if (self == m_targets.end())
		return;
	Target& target = self->second;
	for (auto& [slot, facts] : target.slots)
	{
		if (slot == kReturn)
		{
			forEachStatement(&function.cfunc->body, [&](const cinsn_t* s) {
				if (s->op == cit_return && s->creturn)
				{
					Source source = reader.source(&s->creturn->expr);
					source.at = s->ea;
					facts.below.push_back(std::move(source));
				}
			});
			continue;
		}
		int v = reader.parameterLvar(slot);
		if (v < 0)
			continue;
		if (reader.rebound(v))
		{
			facts.rebound = true;
			continue;
		}
		for (const cexpr_t* occurrence : reader.occurrences(v))
			reader.uses(occurrence, 0, 0, facts.uses);
	}
}

}  // namespace idakit
