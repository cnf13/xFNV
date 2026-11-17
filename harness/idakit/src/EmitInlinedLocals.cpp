// Inlined locals: a local of a class whose user-declared constructor or destructor (its own, a base's or a by-value
// member's) C++ would run, where the exe inlines them down to what the body shows. When every use of the local sits
// in one block from a statement storing its vptr (an exe vtable: the inlined constructor's) on, it is declared there
// as FNVD_INLINED_LOCAL(Class, v, start, end), the macro doing that store and, when the block's last statement using
// the local is another vptr store (the inlined destructor's), that one at scope exit. Writes to the local right after
// the first store or right before the last may be more of that inlined work: they print under a MAYBE-INLINED marker
// for a later sweep. Writes to its members opening its uses right before the first store are the inlined
// constructor's that MSVC scheduled ahead of its vptr store (0xD34790): the local is then declared at the top of the
// block (nothing between uses it), those writes print after it under MAYBE-INLINED. Where the exe runs nothing on the
// local before a call its address is passed to (the first
// statement of the block holding its uses), the call fills it (a hidden result, a Construct, a non-const pointer
// parameter; user, 2026-11-07): FNVD_INLINED_LOCAL(Class, v, std::nullopt, std::nullopt), typed storage for which
// C++ runs nothing, declared where placeDeclarations puts it; a call taking it as const reads it uninitialised, and
// it stays raw storage, marked. So is a local of
// a record that holds no vptr and whose destruction runs no user code (NiColorA, NiPlane): the exe's inlined
// constructor is then the body's own writes, and the writes to its members opening the block holding its uses print
// under MAYBE-INLINED. Otherwise the local is unresolved and the marked body declares it as raw bytes (its members at
// their offsets, its address cast to IDA's pointer type); the function cannot be src.
//
// Constructed locals: storage whose address is the memory argument of a resolved call of X::Construct holds an X.
// Unfold's constructed locals step types such a stack local as X (its constructor call names the
// class), so Hex-Rays has it as an X (or a class derived from X at offset 0, the most-derived one built there). It is
// declared FNVD_CONSTRUCTED_LOCAL(X, v, &X::Destruct or nullptr, args...) at that call, which the macro makes, where
// C++ would construct and destroy it as the exe does: the call (its value unused, not EXE-CALLED) is the first
// statement of the block holding v's uses, and X::Destruct on v ends every path through its last (or the one before a
// plain return): that call, or an if/else whose branches each end in one, with nothing leaving the block in between;
// or nothing destroys v and X's destructor is trivial in src. Where the declaration does not fit and C++ would run
// user code for an X, it is FNVD_INLINED_LOCAL(X, v, std::nullopt, std::nullopt) (the body's Construct and Destruct
// calls stay as the exe makes them); otherwise a plain X.

#include "EmitScopes.hpp"

#include "IdaSupport.hpp"

namespace idakit
{

bool destroys(const SrcModel& model, const std::string& record)
{
	for (const SrcModel::Function* f : model.structorsOf(record))
		if (f->structor == "dtor" && !f->empty)
			return true;
	const SrcModel::Record* r = model.record(record);
	if (!r)
		return false;
	for (const SrcModel::Base& base : r->bases)
		if (destroys(model, base.record))
			return true;
	for (const SrcModel::Field& field : r->fields)
	{
		const SrcModel::Type* held = model.type(model.storage(field.type));
		if (held && held->kind == "record" && held->name != record && destroys(model, held->name))
			return true;
	}
	return false;
}

namespace
{

class InlinedLocals
{
public:
	InlinedLocals(Function& function, Context& context) : m_function(function), m_context(context) {}

	void run()
	{
		const lvars_t& lvars = *m_function.cfunc->get_lvars();
		// resolveTypes' raw storage for the locals C++ would run user structors for: decided here
		for (size_t k = 0; k < m_function.locals.size(); ++k)
			restoreStructorLocal(m_function, k);
		// storage an X::Construct builds in is an X: FNVD_CONSTRUCTED_LOCAL where C++ constructs and destroys it as
		// the exe does, else typed storage (FNVD_INLINED_LOCAL)
		std::vector<const SrcModel::Record*> constructed = constructedStorage();
		std::vector<std::string> raw(m_function.locals.size());
		std::optional<Scopes> scopes;
		for (size_t k = 0; k < m_function.locals.size(); ++k)
		{
			int v = static_cast<int>(k);
			// declared already by the structor-locals pass (a plain C++ local)
			if (m_function.placed(k) || constructed[k])
				continue;
			tinfo_t type = lvars[k].type();
			bool array = type.is_array();
			while (type.is_array())
				type = type.get_array_element();
			const SrcModel::Record* record =
				type.is_udt() && !lvars[k].is_arg_var() ? m_context.types.recordOf(type) : nullptr;
			auto [cls, runs] = record ? userStructors(m_context.model, record->name)
									  : std::pair<std::string, std::string>{};
			if (cls.empty() || (!array && declare(v, record->name)))
				continue;
			if (!scopes)
				scopes.emplace(m_function);
			Fill fill = array ? Fill::none : filledByCall(v, *scopes, *record);
			if (fill == Fill::filled && typedStorage(k))
				continue;
			if (fill == Fill::read)
			{
				raw[k] = cls + "'s " + runs + " is user-declared in src, and the call first using the local takes it " +
						 "as const before anything writes it (an uninitialised read)";
				rawStorage(k, raw[k]);
				continue;
			}
			if (!array && !polymorphic(m_context.model, record->name) && !destroys(m_context.model, record->name) &&
				typedStorage(k))
			{
				markConstructorWrites(v, record->name, *scopes);
				continue;
			}
			raw[k] = cls + "'s " + runs + " is user-declared in src (C++ would run it; the exe inlines it)";
			rawStorage(k, raw[k]);
		}
		for (size_t k = 0; k < constructed.size(); ++k)
		{
			const SrcModel::Record* record = constructed[k];
			if (!record)
				continue;
			if (!scopes)
				scopes.emplace(m_function);
			declareConstructed(static_cast<int>(k), *record, *scopes);
			if (!m_function.locals[k].inlined && !userStructors(m_context.model, record->name).first.empty() &&
				!typedStorage(k))
				rawStorage(k, record->name + "'s structors are user-declared in src, and its type has no spelling");
		}
		// a raw local's members and address: raw; each use of it is the object it holds
		std::unordered_map<const cexpr_t*, const citem_t*> parents = parentsOf(m_function);
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			int v = lvarIndex(e);
			if (v < 0)
				return;
			if (constructed[v])
			{
				if (m_function.locals[v].storage)
					m_function.meaning[e] = m_function.locals[v].holds;
				return;
			}
			if (raw[v].empty())
				return;
			if (!m_function.locals[v].holds.empty())
				m_function.meaning[e] = m_function.locals[v].holds;
			const citem_t* parent = parents[e];
			if (parent && (parent->op == cot_memref || parent->op == cot_ref))
				m_function.meaning[parent] = Missing{raw[v]};
		});
	}

private:
	// local v, which Hex-Rays types as its object, as FNVD_INLINED_LOCAL(X, v, std::nullopt, std::nullopt): typed
	// storage for which C++ runs no constructor or destructor, the exe running none on it but the calls the body makes.
	// Not for a type with no spelling (one with a comma the printer passes through a `using` alias).
	bool typedStorage(size_t v)
	{
		std::optional<std::string> type = m_context.types.spell(m_function.locals[v].type);
		if (!type)
			return false;
		m_function.locals[v].inlined = InlinedLocal{};
		return true;
	}

	// local v, which Hex-Rays types as its object, as raw bytes holding it (marked: why)
	void rawStorage(size_t v, const std::string& why)
	{
		Local& local = m_function.locals[v];
		local.storage = true;
		local.holds = local.type;
		local.type.clear();
		local.why = why;
	}

	// Each local a resolved X::Construct builds in (not a parameter, not declared by the structor-locals pass), its
	// address the call's memory argument, that Hex-Rays types as X or a class derived from X at offset 0 (every such
	// call's X): Hex-Rays' record (none where a call builds another class in it).
	std::vector<const SrcModel::Record*> constructedStorage()
	{
		const lvars_t& lvars = *m_function.cfunc->get_lvars();
		std::vector<const SrcModel::Record*> out(m_function.locals.size());
		std::vector<bool> mixed(m_function.locals.size());
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			auto found = e->op == cot_call ? m_function.meaning.find(e) : m_function.meaning.end();
			const Call* call = found != m_function.meaning.end() ? std::get_if<Call>(&found->second) : nullptr;
			const SrcModel::Function* callee = call ? call->function : nullptr;
			if (!callee || callee->name != callee->cls + "::Construct" || !callee->isStatic || e->a->empty())
				return;
			const cexpr_t* memory = stripCasts(&(*e->a)[0]);
			int v = memory->op == cot_ref ? lvarIndex(memory->x) : -1;
			const SrcModel::Record* record = m_context.model.record(callee->cls);
			if (v < 0 || !record || !lvars[v].type().is_udt() || lvars[v].is_arg_var() || m_function.placed(v))
				return;
			const SrcModel::Record* held = m_context.types.recordOf(lvars[v].type());
			mixed[v] = mixed[v] || !held || !m_context.types.derivesAtZero(held->name, record->name);
			out[v] = held;
		});
		for (size_t k = 0; k < out.size(); ++k)
			if (mixed[k])
				out[k] = nullptr;
		return out;
	}

	// call's resolution where it calls the record's static member of that name (Construct, Destruct), else nullptr
	const Call* structorCall(const cexpr_t* call, const SrcModel::Record& record, const std::string& name) const
	{
		auto found = call->op == cot_call ? m_function.meaning.find(call) : m_function.meaning.end();
		const Call* resolved = found != m_function.meaning.end() ? std::get_if<Call>(&found->second) : nullptr;
		return resolved && resolved->function && resolved->function->isStatic &&
					   resolved->function->name == record.name + "::" + name
				   ? resolved
				   : nullptr;
	}

	// v (storage the record's Construct builds in: a record) declared FNVD_CONSTRUCTED_LOCAL where
	// C++ constructs and destroys the local as the exe does: the first statement of the block holding v's uses is the
	// record's Construct call on it (its value unused, not EXE-CALLED), and every path through the block's last
	// statement using v (the block's last, or the one before a plain return ending it) ends in the record's Destruct
	// call on it: that statement is the call, or an if/else whose branches each end in one; with nothing in between
	// leaving the block; or nothing destroys it, the record's destructor being trivial in src. No goto jumps past the
	// declaration.
	void declareConstructed(int v, const SrcModel::Record& record, const Scopes& scopes)
	{
		if (scopes.uses(v).empty())
			return;
		Scopes::Scope scope = scopes.scopeOf(v);
		const std::vector<const cinsn_t*>& items = scope.items;
		const cinsn_t* first = items[scope.first];
		const cexpr_t* construct = first->op == cit_expr ? first->cexpr : nullptr;
		const Call* call = construct ? structorCall(construct, record, "Construct") : nullptr;
		std::optional<std::string> type = m_context.types.spell("record:" + record.name);
		if (!call || call->exeCalled || call->resultArgument >= 0 || usesIn(first, v) != 1 ||
			lvarIndex(stripAddress((*construct->a)[0])) != v || !type ||
			scopes.jumpedPast(scope.block, scope.first) || splitByFrame(m_function, items, scope.first, scope.last))
			return;
		// the statements calling the record's Destruct on v
		std::vector<const cinsn_t*> destructs;
		bool elsewhere = false;
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t* s) {
			const Call* destruct = structorCall(e, record, "Destruct");
			if (!destruct || e->a->empty() || lvarIndex(stripAddress((*e->a)[0])) != v)
				return;
			// the macro's `&X::Destruct` converts to `void (*)(X*)`
			const SrcModel::Type* signature = m_context.model.type(destruct->function->type);
			SrcTypes::Parts parameter = signature && signature->params.size() == 1
											? m_context.types.parts(signature->params[0])
											: SrcTypes::Parts{};
			if (s->op == cit_expr && s->cexpr == e && parameter.kind == "ptr" &&
				m_context.types.canonical(parameter.of) == m_context.types.canonical("record:" + record.name))
				destructs.push_back(s);
			else
				elsewhere = true;
		});
		// the Destruct calls ending every path through the scope's last statement using v (the call itself, or an
		// if/else whose branches each end in one)
		std::vector<const cinsn_t*> ends;
		std::vector<const cinsn_t*> lifetime(items.begin() + scope.first, items.begin() + scope.last + 1);
		bool atEnd = !destructs.empty() && endsInDestruct(items[scope.last], destructs, ends) &&
					 ends.size() == destructs.size() &&
					 (scope.last + 1 == items.size() ||
					  (scope.last + 2 == items.size() && scopes.plainReturn(items.back()))) &&
					 !Scopes::leaves(lifetime) && !labelledInside(ends);
		if (elsewhere || (!destructs.empty() && !atEnd) ||
			(destructs.empty() && destroys(m_context.model, record.name)))
			return;
		Local& local = m_function.locals[v];
		local.type = "record:" + record.name;
		local.inlined = InlinedLocal{nullptr, nullptr, construct, !destructs.empty()};
		local.declaredAt = first;
		local.initialised = true;
		m_function.meaning[first] = InlinedStatement{true, ""};
		for (const cinsn_t* destruct : destructs)
			mark(destruct, {true, ""});
	}

	// s ends every path through it in one of the Destruct statements (collected in ends): s is one, a block whose last
	// statement does, or an if/else whose branches both do
	static bool endsInDestruct(const cinsn_t* s, const std::vector<const cinsn_t*>& destructs,
							   std::vector<const cinsn_t*>& ends)
	{
		if (std::find(destructs.begin(), destructs.end(), s) != destructs.end())
		{
			ends.push_back(s);
			return true;
		}
		if (s->op == cit_block)
			return !s->cblock->empty() && endsInDestruct(&s->cblock->back(), destructs, ends);
		return s->op == cit_if && s->cif->ielse && endsInDestruct(s->cif->ithen, destructs, ends) &&
			   endsInDestruct(s->cif->ielse, destructs, ends);
	}

	// How the first use of v fills it (a local of a record C++ would run user code for, first used by a call its
	// address is passed to, before the exe writes anything to it: the first statement of the block holding v's uses
	// is that call, or `x = call`, using v once, no goto jumping past it). The exe runs nothing on v before the call,
	// so v is storage the call fills: typed storage C++ runs nothing for (user, 2026-11-07), where the call writes it
	// (a non-const pointer or reference parameter, an unresolved call such as an out-of-line structor src has no
	// Construct for, or a record returned by value, `v = F(...)`, where C++'s assignment from the temporary is the
	// exe's fill: a byte copy destroying nothing); read where the call takes it as a const pointer or reference or is
	// a const member called on it (an uninitialised read); none otherwise (a Hex-Rays helper's argument, another use).
	enum class Fill
	{
		none,
		filled,
		read
	};
	Fill filledByCall(int v, const Scopes& scopes, const SrcModel::Record& record) const
	{
		if (scopes.uses(v).empty())
			return Fill::none;
		Scopes::Scope scope = scopes.scopeOf(v);
		const cinsn_t* first = scope.items[scope.first];
		if (first->op != cit_expr || usesIn(first, v) != 1 || scopes.jumpedPast(scope.block, scope.first))
			return Fill::none;
		const cexpr_t* call = first->cexpr->op == cot_asg ? first->cexpr->y : first->cexpr;
		call = stripCasts(call);
		if (call->op != cot_call || call->x->op == cot_helper)
			return Fill::none;
		auto found = m_function.meaning.find(call);
		const Call* resolved = found != m_function.meaning.end() ? std::get_if<Call>(&found->second) : nullptr;
		for (size_t n = 0; n < call->a->size(); ++n)
		{
			const cexpr_t* argument = stripCasts(&(*call->a)[n]);
			if (argument->op != cot_ref || lvarIndex(stripCasts(argument->x)) != v)
				continue;
			if (!resolved)
				return Fill::filled;
			if (resolved->resultArgument == static_cast<int>(n))
				return record.trivialCopy && !destroys(m_context.model, record.name) ? Fill::filled : Fill::none;
			// a trivial accessor's call whose value goes nowhere does nothing (dropNoOpCalls leaves it out)
			std::optional<AccessorCall> accessor = accessorCall(m_context, call);
			if (accessor && !accessor->value && first->cexpr == call)
				return Fill::filled;
			const SrcModel::Type* type = m_context.model.type(resolved->function->type);
			int parameter = parameterIndex(*resolved, n);
			bool constant = !type ? false
							: parameter < 0 ? type->isConst
							: static_cast<size_t>(parameter) < type->params.size() &&
								  m_context.types.constPointee(type->params[parameter]);
			return constant ? Fill::read : Fill::filled;
		}
		return Fill::none;
	}

	// e past its casts and an address taken (`&v`, `(T*)&v`, `(void*)v`)
	static const cexpr_t* stripAddress(const cexpr_t& e)
	{
		const cexpr_t* x = stripCasts(&e);
		return x->op == cot_ref ? stripCasts(x->x) : x;
	}

	// an object of the record holds a vptr: its own, a base's or a by-value member's (its inlined constructor stores
	// one, which the vptr framing above declares the local at)
	static bool polymorphic(const SrcModel& model, const std::string& record)
	{
		const SrcModel::Record* r = model.record(record);
		if (!r)
			return true;
		if (r->vfptr || !r->vtables.empty())
			return true;
		for (const SrcModel::Base& base : r->bases)
			if (polymorphic(model, base.record))
				return true;
		for (const SrcModel::Field& field : r->fields)
		{
			const SrcModel::Type* held = model.type(model.storage(field.type));
			if (held && held->kind == "record" && held->name != record && polymorphic(model, held->name))
				return true;
		}
		return false;
	}

	// v (FNVD_INLINED_LOCAL of a record without a vptr whose destruction runs no user code): the writes to its members
	// that open the block holding its uses may be the record's inlined constructor; they print under MAYBE-INLINED
	// for the sweep that gives the class its real constructor (the macro's parity form `X v;` runs it)
	void markConstructorWrites(int v, const std::string& cls, const Scopes& scopes)
	{
		if (scopes.uses(v).empty())
			return;
		Scopes::Scope scope = scopes.scopeOf(v);
		for (size_t k = scope.first; k < scope.items.size() && writes(scope.items[k], v); ++k)
			mark(scope.items[k], {false, "a write opening " + cls + "'s uses: may be its inlined constructor"});
	}

	// the uses of v in the statement and the statements inside it
	static int usesIn(const cinsn_t* s, int v)
	{
		int count = 0;
		forEachStatement(s, [&](const cinsn_t* inner) {
			for (const cexpr_t* own : ownExpressions(inner))
				forEachSubexpression(own, [&](const cexpr_t* e) { count += lvarIndex(e) == v; });
		});
		return count;
	}

	// the exe vtable a statement stores as v's vptr (`v.__vftable = &X::vftable;`), nullptr for any other statement
	const SrcModel::Ref* vptrStore(const cinsn_t* s, int v) const
	{
		if (s->op != cit_expr || s->cexpr->op != cot_asg)
			return nullptr;
		const cexpr_t* target = s->cexpr->x;
		const cexpr_t* value = s->cexpr->y;
		if (target->op != cot_memref || target->m != 0 || lvarIndex(target->x) != v || !target->type.is_ptr())
			return nullptr;
		while (value->op == cot_cast)
			value = value->x;
		if (value->op == cot_ref)
			value = value->x;
		if (value->op != cot_obj)
			return nullptr;
		for (const SrcModel::Ref& r : m_context.model.refsAt(value->obj_ea))
			if (r.kind == "vftable")
				return &r;
		return nullptr;
	}

	// `v.member = expr;` (members of members too) with expr not reading v
	static bool writes(const cinsn_t* s, int v)
	{
		if (s->op != cit_expr || s->cexpr->op != cot_asg)
			return false;
		const cexpr_t* target = s->cexpr->x;
		while (target->op == cot_memref || (target->op == cot_idx && target->x->type.is_array()))
			target = target->x;
		if (lvarIndex(target) != v || target == s->cexpr->x)
			return false;
		int reads = 0;
		forEachSubexpression(s->cexpr->y, [&](const cexpr_t* e) { reads += lvarIndex(e) == v; });
		return reads == 0;
	}

	void mark(const cinsn_t* s, InlinedStatement statement)
	{
		// the macro does it: what the statement's parts mean does not matter
		if (statement.macro)
			forEachSubexpression(s->cexpr, [&](const cexpr_t* e) { m_function.meaning.erase(e); });
		m_function.meaning[s] = std::move(statement);
	}

	// v declared FNVD_INLINED_LOCAL where its uses allow it
	bool declare(int v, const std::string& cls)
	{
		Local& local = m_function.locals[v];
		if (local.type.empty())
			return false;
		int total = usesIn(&m_function.cfunc->body, v);
		bool declared = false;
		forEachStatement(&m_function.cfunc->body, [&](const cinsn_t* block) {
			if (declared || block->op != cit_block)
				return;
			const cblock_t& items = *block->cblock;
			std::vector<const cinsn_t*> list;
			std::vector<int> uses;
			for (const cinsn_t& item : items)
				list.push_back(&item), uses.push_back(usesIn(&item, v));
			size_t first = 0;
			while (first < list.size() && uses[first] == 0)
				++first;
			int inBlock = 0;
			for (int n : uses)
				inBlock += n;
			// MSVC schedules an inlined constructor's member writes before its vptr store too (0xD34790): those
			// opening the uses (among statements not using it) come before the store, and the local is declared at
			// the top of the block
			size_t store = first;
			while (store < list.size() && !vptrStore(list[store], v) && (uses[store] == 0 || writes(list[store], v)))
				++store;
			const SrcModel::Ref* start = store < list.size() ? vptrStore(list[store], v) : nullptr;
			if (!start || inBlock != total)
				return;
			size_t last = list.size() - 1;
			while (uses[last] == 0)
				--last;
			if (splitByFrame(m_function, list, first, last))
				return;
			const SrcModel::Ref* end = last > store ? vptrStore(list[last], v) : nullptr;
			declared = true;
			local.inlined = InlinedLocal{start, end};
			local.declaredAt = store == first ? list[first] : block;
			local.initialised = store == first;
			mark(list[store], {true, ""});
			for (size_t before = first; before < store; ++before)
				if (uses[before] != 0)
					mark(list[before], {false, "before the vptr store of " + cls + "'s inlined constructor"});
			size_t after = store + 1;
			for (; after < list.size() && writes(list[after], v); ++after)
				mark(list[after], {false, "after the vptr store of " + cls + "'s inlined constructor"});
			if (!end)
				return;
			mark(list[last], {true, ""});
			for (size_t before = last - 1; before >= after && writes(list[before], v); --before)
				mark(list[before], {false, "before the vptr store of " + cls + "'s inlined destructor"});
		});
		return declared;
	}

	Function& m_function;
	Context& m_context;
};

}  // namespace

void resolveInlinedLocals(Function& function, Context& context)
{
	InlinedLocals(function, context).run();
}

}  // namespace idakit
