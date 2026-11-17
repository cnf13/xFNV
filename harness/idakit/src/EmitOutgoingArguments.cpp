// Outgoing arguments: MSVC builds a by-value argument in place in the outgoing argument area (`sub esp, 0x14; mov ecx,
// esp; call X::X`) and stores a pushed argument there where that area is a local in Hex-Rays' view, which then reads
// the call's arguments as that local's bytes (`_BYTE afMax[24]`):
//
//     *(T **)&afMax[20] = &ret;
//     X::X((X *)afMax, &v);
//     if ( F(a, *(X *)afMax, *(T **)&afMax[20]) )
//
// Where a call's argument is read from exactly its own stack slot at the call (the exe's frame there) and the
// statements right before the call's store into those bytes, the argument is the stored value, `if (F(a, *(X *)afMax,
// &ret))`, where nothing else reads or writes the bytes (the callee takes them). An object a constructor the exe calls
// out of line builds there (`X::X((X *)afMax, &v)`) has no src form: src's X::Construct is no C++ constructor, and
// C++'s own copy of X would not make the exe's call. Its statement stays, and the argument is marked.

#include "EmitPasses.hpp"

#include "IdaSupport.hpp"

#include "idakit/SrcIda.hpp"

#include <map>
#include <set>
#include <unordered_map>
#include <vector>

namespace idakit
{

namespace
{

// bytes of a local: its lvar and the offset into it
struct Slot
{
	int lvar = -1;
	int64_t offset = 0;
	auto operator<=>(const Slot&) const = default;
};

std::optional<Slot> object(const lvars_t& lvars, const cexpr_t* e);

// the local is an array (Hex-Rays types its uses as the pointer it decays to)
bool array(const lvars_t& lvars, const cexpr_t* e)
{
	return e->op == cot_var && lvars[e->v.idx].type().is_array();
}

// the bytes an address points at: `&v`, `&v[k]`, an array local decaying to its address, one plus a number; Hex-Rays'
// casts peeled; nullopt for any other address
std::optional<Slot> address(const lvars_t& lvars, const cexpr_t* e)
{
	e = stripCasts(e);
	if (e->op == cot_ref)
		return object(lvars, e->x);
	if (array(lvars, e))
		return Slot{e->v.idx, 0};
	if (e->op == cot_add && e->y->op == cot_num && e->x->type.is_ptr())
		if (std::optional<Slot> base = address(lvars, e->x))
			return Slot{base->lvar,
						base->offset + static_cast<int64_t>(e->y->numval() * e->x->type.get_ptrarr_objsize())};
	return std::nullopt;
}

// the bytes an lvalue is: a local, an element of a local array at a constant index, `*address`, a member of one
std::optional<Slot> object(const lvars_t& lvars, const cexpr_t* e)
{
	switch (e->op)
	{
	case cot_var:
		return Slot{e->v.idx, 0};
	case cot_idx:
		if (array(lvars, e->x) && e->y->op == cot_num)
			return Slot{e->x->v.idx, static_cast<int64_t>(e->y->numval() * e->type.get_size())};
		return std::nullopt;
	case cot_ptr:
		return address(lvars, e->x);
	case cot_memref:
		if (std::optional<Slot> base = object(lvars, e->x))
			return Slot{base->lvar, base->offset + e->m};
		return std::nullopt;
	default:
		return std::nullopt;
	}
}

class OutgoingArguments
{
public:
	OutgoingArguments(Function& function, Context& context)
		: m_function(function), m_context(context), m_pfn(get_func(function.ea)), m_parents(parentsOf(function))
	{
	}

	void run()
	{
		forEachStatement(&m_function.cfunc->body, [&](const cinsn_t* s) {
			if (s->op == cit_block)
			{
				std::vector<const cinsn_t*> items;
				for (const cinsn_t& item : *s->cblock)
					items.push_back(&item);
				for (size_t n = 1; n < items.size(); ++n)
					statement(items, n);
			}
		});
		const std::set<int> taken = addressTakenLocals(*m_function.cfunc);
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			if (e->op == cot_call)
				builtInPlace(e, taken);
		});
	}

private:
	// A local Hex-Rays reads as a whole by-value record argument from that argument's own stack slot is the object
	// the exe builds in place there (its constructor's steps inlined on the slot). C++ passes a copy of the local
	// instead: the same bytes only where C++ copies the record as its bytes and the body's own stores build it (its
	// address never taken). Where calls build it on its address (`_Tidy(&v); v.assign(...)`), the bytes they leave
	// are the argument area's own, which no copy of a local reproduces.
	void builtInPlace(const cexpr_t* call, const std::set<int>& taken)
	{
		for (size_t k = 0; k < call->a->size(); ++k)
		{
			const cexpr_t* argument = &(*call->a)[k];
			if (argument->op != cot_var || !argument->type.is_udt() || !ownSlot(call, k, Slot{argument->v.idx, 0}))
				continue;
			const SrcModel::Record* record = m_context.types.recordOf(argument->type);
			std::string name = record ? record->name : std::string(argument->type.dstr());
			if (!record || !m_context.model.byteCopied(*record))
				m_function.meaning[argument] = Missing{"the exe builds the by-value " + name +
													   " in place in the outgoing argument area: C++ would copy "
													   "the local into it"};
			else if (taken.count(argument->v.idx))
				m_function.meaning[argument] =
					Missing{"the exe builds the by-value " + name + " in place in the outgoing argument area through "
							"calls on its address: the bytes they leave are the area's own, not the local's"};
		}
	}

	const lvars_t& lvars() const { return *m_function.cfunc->get_lvars(); }

	// the statement's one call, evaluated once right after the statements before it (an expression statement, an
	// if's condition, a return, a switch's expression; no label a jump could reach it by past them; not under a
	// `&&`, `||`, `?:` or comma, which run it on some paths only or after another operand)
	void statement(const std::vector<const cinsn_t*>& items, size_t n)
	{
		const cinsn_t* s = items[n];
		if (s->label_num >= 0 || (s->op != cit_expr && s->op != cit_if && s->op != cit_return && s->op != cit_switch))
			return;
		const cexpr_t* call = nullptr;
		int calls = 0;
		for (const cexpr_t* own : ownExpressions(s))
			forEachSubexpression(own, [&](const cexpr_t* e) {
				if (e->op == cot_call)
				{
					call = e;
					++calls;
				}
			});
		if (calls != 1)
			return;
		for (const citem_t* up = m_parents.at(call); up->is_expr(); up = m_parents.at(static_cast<const cexpr_t*>(up)))
			if (up->op == cot_land || up->op == cot_lor || up->op == cot_tern || up->op == cot_comma)
				return;
		arguments(call, items, n);
	}

	// The statements right before the call's, each writing one of its arguments' own slots as the type the call reads
	// there, back to the first that writes none or to an object a constructor builds there: each store is the
	// argument's value, an object built by an exe call the argument's Missing.
	void arguments(const cexpr_t* call, const std::vector<const cinsn_t*>& items, size_t n)
	{
		std::map<Slot, size_t> slots;
		for (size_t k = 0; k < call->a->size(); ++k)
			if (std::optional<Slot> slot = object(lvars(), &(*call->a)[k]); slot && ownSlot(call, k, *slot))
				slots.emplace(*slot, k);
		while (n-- > 0 && !slots.empty())
		{
			const cinsn_t* s = items[n];
			if (s->op != cit_expr || s->label_num >= 0)
				return;
			if (s->cexpr->op != cot_asg)
			{
				// the exe calls the constructor out of line: src's X::Construct is no C++ constructor, so no src
				// expression builds the by-value argument with that call
				const cexpr_t* argument = constructedArgument(s, call, slots);
				if (argument)
					m_function.meaning[argument] = Missing{
						constructorName(s) + " builds the by-value argument in place: src cannot pass a by-value "
											 "object an exe call builds"};
				return;
			}
			std::optional<Write> written = stored(s);
			auto slot = written ? slots.find(written->slot) : slots.end();
			if (slot == slots.end())
				return;
			const cexpr_t* argument = &(*call->a)[slot->second];
			if (written->rewrite.type.empty() || !sameType(written->type, argument->type) ||
				!onlyHere(written->slot, argument->type.get_size(), s->cexpr->x, argument))
				return;
			written->rewrite.dropped = {s};
			eraseReplaced(m_function, {s}, written->rewrite.items);
			forEachSubexpression(argument, [&](const cexpr_t* e) { m_function.meaning.erase(e); });
			m_function.meaning[argument] = std::move(written->rewrite);
			slots.erase(slot);
		}
	}

	// what a statement writes into a local's bytes: the slot, the type written there and what the argument read
	// from it prints as
	struct Write
	{
		Slot slot;
		tinfo_t type;
		Rewrite rewrite;
	};

	// `slot = value`: the value without the casts that only reinterpret it (integers and pointers of one size), from
	// the outside in; the first cast that changes the value (a narrowing, a widening, a float conversion) stays its
	// own. It writes the bytes of its own type where it is of the slot's size (`*(_DWORD *)&a[20] = &ret`), else of
	// the slot's type, which it converts to as a parameter of that type does.
	std::optional<Write> stored(const cinsn_t* s) const
	{
		const cexpr_t* asg = s->cexpr;
		std::optional<Slot> slot = object(lvars(), asg->x);
		if (!slot || asg->y->has_side_effects())
			return std::nullopt;
		auto reinterprets = [](const cexpr_t* c) {
			return c->op == cot_cast && c->type.get_size() == c->x->type.get_size() && !c->type.is_floating() &&
				   !c->x->type.is_floating();
		};
		const cexpr_t* value = asg->y;
		while (reinterprets(value))
			value = value->x;
		bool reinterpreted = value->type.get_size() == asg->x->type.get_size() && !value->type.is_floating() &&
							 !asg->x->type.is_floating();
		std::string type = srcType(m_function, m_context, value), why;
		if (type.empty())
			type = m_context.types.keyOf(value->type, why).value_or("");
		return Write{*slot, reinterpreted ? value->type : asg->x->type, Rewrite{1, "$0", {value}, {}, type, ""}};
	}

	// The argument whose slot `X::Construct(&slot, ...)` (a src constructor's call) builds; nullptr for none.
	const cexpr_t* constructedArgument(const cinsn_t* s, const cexpr_t* call, const std::map<Slot, size_t>& slots) const
	{
		const cexpr_t* e = s->cexpr;
		auto found = e->op == cot_call ? m_function.meaning.find(e) : m_function.meaning.end();
		const Call* c = found != m_function.meaning.end() ? std::get_if<Call>(&found->second) : nullptr;
		if (!c || !c->function || c->through || structorName(c->function->name) != "Construct" || e->a->empty())
			return nullptr;
		std::optional<Slot> slot = address(lvars(), &(*e->a)[0]);
		auto argument = slot ? slots.find(*slot) : slots.end();
		return argument != slots.end() ? &(*call->a)[argument->second] : nullptr;
	}

	// "X::Construct (0xADDR)" for the constructor call statement
	std::string constructorName(const cinsn_t* s) const
	{
		const Call& c = std::get<Call>(m_function.meaning.at(s->cexpr));
		const cexpr_t* callee = s->cexpr->x;
		return c.function->name + (callee->op == cot_obj ? " (" + hex(callee->obj_ea) + ")" : "");
	}

	// No locals pass owns the slot's lvar, and every use of its bytes [offset, offset + size) is the store's left side
	// or the argument, so that nothing else reads or writes what the fold drops. A use is the lvalue a read of the
	// lvar is, climbing from the variable through members, constant indices, addresses and dereferences; one that
	// ends as an address (taken, a non-constant index, an array decaying) reaches unknown bytes and fails.
	bool onlyHere(const Slot& slot, uint64 size, const cexpr_t* left, const cexpr_t* argument) const
	{
		if (m_function.placed(slot.lvar) || m_function.locals[slot.lvar].inlined)
			return false;
		std::set<const cexpr_t*> own;
		for (const cexpr_t* root : {left, argument})
			forEachSubexpression(root, [&](const cexpr_t* e) { own.insert(e); });
		bool alone = true;
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			if (!alone || e->op != cot_var || e->v.idx != slot.lvar || own.contains(e))
				return;
			const cexpr_t* use = e;
			for (const citem_t* up = m_parents.at(use); up->is_expr(); up = m_parents.at(use))
			{
				const cexpr_t* outer = static_cast<const cexpr_t*>(up);
				std::optional<Slot> at = object(lvars(), outer);
				if (!at)
					at = address(lvars(), outer);
				if (!at || at->lvar != slot.lvar)
					break;
				use = outer;
			}
			std::optional<Slot> bytes = array(lvars(), use) ? std::nullopt : object(lvars(), use);
			alone = bytes && (bytes->offset + static_cast<int64_t>(use->type.get_size()) <= slot.offset ||
							  bytes->offset >= slot.offset + static_cast<int64_t>(size));
		});
		return alone;
	}

	// the bytes are argument k's own stack slot at the call: the local's frame offset is the one esp plus the
	// argument's stack offset (its callee type's) has at the call instruction
	bool ownSlot(const cexpr_t* call, size_t k, const Slot& slot) const
	{
		const lvar_t& lvar = m_function.cfunc->get_lvars()->at(slot.lvar);
		tinfo_t type = call->x->type.is_ptr() ? call->x->type.get_pointed_object() : call->x->type;
		func_type_data_t fd;
		insn_t insn;
		if (!m_pfn || !lvar.is_stk_var() || decode_insn(&insn, call->ea) <= 0 || !is_call_insn(insn) ||
			!type.get_func_details(&fd) || k >= fd.size() || !fd[k].argloc.is_stkoff())
			return false;
		sval_t esp = get_spd(m_pfn, call->ea) + frame_off_retaddr(m_pfn);
		sval_t at = m_function.cfunc->mba->stkoff_vd2ida(lvar.get_stkoff()) + slot.offset;
		return at == esp + fd[k].argloc.stkoff() && (*call->a)[k].type.get_size() == fd[k].type.get_size();
	}

	Function& m_function;
	Context& m_context;
	func_t* m_pfn;
	const std::unordered_map<const cexpr_t*, const citem_t*> m_parents;
};

}  // namespace

void foldOutgoingArguments(Function& function, Context& context)
{
	OutgoingArguments(function, context).run();
}

}  // namespace idakit
