// Pointer calls: an indirect call the Calls pass left (no vtable slot, no named function) whose callee, once Hex-Rays'
// casts are peeled, is a src field (the Fields pass's), a src header global or a local of src function-pointer type,
// whose convention is the one Hex-Rays calls with, is a call through it: its arguments and result checked against the
// pointee's fn type as the Calls pass checks a function's, printed `m_pfnRead(this, ...)`. Any other stays as the
// Calls pass left it.

#include "EmitPasses.hpp"

#include "idakit/SrcIda.hpp"

#include <unordered_set>

namespace idakit
{

namespace
{

class PointerCalls
{
public:
	PointerCalls(Function& function, Context& context)
		: m_function(function), m_context(context), m_types(context.types)
	{
	}

	void run()
	{
		// the calls whose result nothing reads: a statement of their own
		std::unordered_set<const cexpr_t*> statements;
		forEachStatement(&m_function.cfunc->body, [&](const cinsn_t* s) {
			if (s->op == cit_expr)
				statements.insert(s->cexpr);
		});
		std::vector<const cexpr_t*> calls;
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			if (e->op != cot_call)
				return;
			auto found = m_function.meaning.find(e);
			const Missing* missing = found == m_function.meaning.end() ? nullptr : std::get_if<Missing>(&found->second);
			if (missing && missing->what.starts_with("indirect call through "))
				calls.push_back(e);
		});
		for (const cexpr_t* call : calls)
			resolve(call, statements.count(call) == 0);
	}

private:
	void resolve(const cexpr_t* call, bool resultUsed)
	{
		const cexpr_t* callee = call->x;
		while (callee->op == cot_cast)
			callee = callee->x;
		std::string key = fnKey(callee);
		if (key.empty())
			return;
		const SrcModel::Type* fn = m_context.model.type(key);
		tinfo_t called = call->x->type.is_ptr() ? call->x->type.get_pointed_object() : call->x->type;
		if (fn->cc != conventionName(called.get_cc()))
			return;
		auto [target, added] = m_context.calledPointers.try_emplace(key);
		if (added)
			target->second.type = key;
		Meaning meaning = checkCall(m_function, m_context, call, target->second, resultUsed);
		Call* resolved = std::get_if<Call>(&meaning);
		if (!resolved)
			return;
		resolved->through = callee;
		m_function.meaning[call] = std::move(meaning);
		// Hex-Rays' casts of the callee are not printed: the call is through the src lvalue
		for (const cexpr_t* e = call->x; e != callee; e = e->x)
			m_function.meaning.erase(e);
	}

	// the model's fn key a pointer key (typedefs at its top resolved) points to; "" for another
	std::string fnOf(const std::string& key) const
	{
		SrcTypes::Parts p = m_types.parts(m_types.bare(key));
		if (p.kind != "ptr")
			return "";
		std::string of = m_types.bare(p.of);
		const SrcModel::Type* t = m_context.model.type(of);
		return t && t->kind == "fn" ? of : "";
	}

	// the src type key of a field or global lvalue: a resolved field's, the first src header global's or static
	// member's at the address, an element of an array one of those is; "" for any other
	std::string lvalueKey(const cexpr_t* e) const
	{
		if (e->op == cot_memptr || e->op == cot_memref)
		{
			auto found = m_function.meaning.find(e);
			const Member* member = found == m_function.meaning.end() ? nullptr : std::get_if<Member>(&found->second);
			return member ? member->field->type : "";
		}
		if (e->op == cot_obj && !get_func(e->obj_ea))
		{
			for (const SrcModel::Ref& ref : m_context.model.refsAt(e->obj_ea))
				if (!placeholder(ref) && (ref.kind == "global" || ref.kind == "static_member") && isHeader(ref.file))
					return ref.type;
			return "";
		}
		if (e->op == cot_idx)
			if (SrcTypes::Parts p = m_types.parts(m_types.bare(lvalueKey(e->x))); p.kind == "array")
				return p.of;
		return "";
	}

	// The model's fn key of the callee lvalue's type: a field's, global's or array element's (lvalueKey), a local's
	// (not raw storage). A local Hex-Rays types from IDA's spelling (no model key) takes the key of what it holds:
	// every write to it is an assignment from such a field or global of the same type (canonically), its address
	// never taken. "" for any other.
	std::string fnKey(const cexpr_t* callee) const
	{
		if (callee->op != cot_var)
			return fnOf(lvalueKey(callee));
		int v = lvarIndex(callee);
		if (v < 0)
			return "";
		const Local& local = m_function.locals[v];
		if (local.storage || local.reference || local.type.empty())
			return "";
		if (std::string fn = fnOf(local.type); !fn.empty())
			return fn;
		std::string held;
		bool other = false;
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			if (e->op == cot_ref && lvarIndex(e->x) == v)
				other = true;
			if (!written(e) || lvarIndex(e->x) != v)
				return;
			const cexpr_t* value = e->op == cot_asg ? e->y : nullptr;
			while (value && value->op == cot_cast)
				value = value->x;
			std::string fn = value && value->op != cot_var ? fnKey(value) : "";
			if (fn.empty() || (!held.empty() && fn != held) ||
				m_types.canonical("ptr(" + fn + ")") != m_types.canonical(local.type))
				other = true;
			else
				held = fn;
		});
		return other ? "" : held;
	}

	Function& m_function;
	Context& m_context;
	SrcTypes& m_types;
};

}  // namespace

void resolvePointerCalls(Function& function, Context& context)
{
	PointerCalls(function, context).run();
}

}  // namespace idakit
