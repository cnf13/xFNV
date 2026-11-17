// Structor locals: a local of a src class template whose constructors and destructor are inline in its header (none
// specialized for the instance) and do exactly what the exe inlined, printed as a plain C++ local, declared where the
// exe's constructor steps are and scoped so the destructor's steps fall where the exe does them (the end of the
// block). The class, and its src steps:
//
//     NiPointer<T> v;        `v.m_pObject = 0;`                                                     NiPointer()
//     NiPointer<T> v = p;    `v.m_pObject = p;` and the reference NiIncRefCount(p) takes           NiPointer(T*)
//     NiPointer<T> v = F();  `F(&v, ...);`, a resolved call returning NiPointer<T> by value through MSVC's hidden
//                            result pointer (C++ builds v from the result in place, the exe has F build it there)
//     ~NiPointer             `if (v.m_pObject) NiDecRefCount(v.m_pObject);` as the last statements of the block
//                            holding v's uses, or the ones before a plain return ending it; or, where statements
//                            follow them, as the last of a block of its own (Function's frames: `{ NiPointer<T> v
//                            ...; ... }`) holding the declaration through the destructor's steps, where every other
//                            local first used in it is used only in it or is still to be placed (declared before the
//                            frame: no pass declares a local inside a frame its uses outlast), no goto from outside
//                            jumps in, and it nests with the block's other frames
//
// The constructors' and the destructor's steps are the instance's own as src defines them inline (srcdb's steps
// rows, matched by EmitRefCounts' Steps): whatever NiIncRefCount / NiDecRefCount overload the instance's T takes
// (NiRefObject's atomic count and DeleteThis, a pool object's plain count and its return to the pool, an out-of-line
// DecRefCount), so an instance src specializes a structor of has none. The block holding all of v's uses: v is
// declared at its first statement using v, which must be the constructor's; no goto from outside jumps past the
// declaration, and nothing between the declaration and the destructor's steps leaves the block (return, a goto out, a
// break or continue of an enclosing loop). Runs before the inlined-locals pass, which leaves the locals declared here
// alone; any other local keeps that pass's forms (FNVD_INLINED_LOCAL, raw storage).
//
// Any other record C++ would run a user destructor for (user, 2026-11-08): `X v = F();` where v's first write is a
// resolved call returning X by value into it through the hidden result pointer, no other call returns into v, and
// the exe's end of v's block (as above, framed where statements follow) is exactly the destructor C++ runs there: the
// instance's inline destructor steps (srcdb's steps row; only where no base or member destroys anything), or a call on
// v of the exe function src registers X's C++ destructor at, or of X::Destruct where that destructor is src's inline
// template member. Otherwise it is left to the inlined-locals pass.

#include "EmitRefCounts.hpp"
#include "EmitScopes.hpp"
#include "IdaSupport.hpp"

namespace idakit
{

namespace
{

class StructorLocals
{
public:
	StructorLocals(Function& function, Context& context)
		: m_function(function), m_context(context), m_steps(function, context), m_scopes(function),
		  m_frame(ehFrameStatements(function, context))
	{
	}

	void run()
	{
		const lvars_t& lvars = *m_function.cfunc->get_lvars();
		for (size_t k = 0; k < lvars.size(); ++k)
		{
			int v = static_cast<int>(k);
			const Local& local = m_function.locals[k];
			// resolveTypes' raw storage of a record C++ runs user structors for holds the key
			const std::string& key = local.type.empty() && local.storage ? local.holds : local.type;
			if (lvars[k].is_arg_var())
			{
				if (key.starts_with("record:") && !m_scopes.uses(v).empty())
					byValueParameter(v, key);
				continue;
			}
			if (key.empty() || m_function.placed(k) || m_scopes.uses(v).empty())
				continue;
			const SrcModel::Record* record = m_context.types.templateRecord(key, "NiPointer", false);
			bool smart = record != nullptr;
			// any other record C++ would run a user destructor for: only as a by-value call's result
			if (!record && key.starts_with("record:") && destroys(m_context.model, key.substr(7)))
				record = m_context.model.record(key.substr(7));
			if (record)
			{
				Local raw = local;
				std::unordered_map<const citem_t*, Meaning> meanings = m_function.meaning;
				restoreStructorLocal(m_function, k);
				m_temporary = false;
				structorLocal(v, *record, smart);
				bool temporary = m_temporary;
				// not declared here (nor a temporary): as resolveTypes left it
				if (!m_function.placed(k) && !temporary && raw.storage)
				{
					m_function.locals[k] = raw;
					m_function.meaning = std::move(meanings);
				}
			}
		}
	}

	// A by-value parameter of a record C++ destroys: the callee destroys it, in the exe (MSVC's convention) as in
	// C++, which runs the parameter's destructor where the function returns. The exe's destruction of it is that one
	// where it is the body's last statements (or those before a plain return ending the body), no other statement
	// returns, and it is the destructor C++ runs (destructCall's out-of-line copy, or the instance's inline steps
	// where no base or member destroys anything): not printed. Any other call destroying it (its Destruct, its C++
	// destructor's address) is marked: printed, C++ would destroy the parameter a second time.
	void byValueParameter(int v, const std::string& key)
	{
		const SrcModel::Record* record = m_context.types.templateRecord(key, "NiPointer", false);
		bool smart = record != nullptr;
		if (!record && destroys(m_context.model, key.substr(7)))
			record = m_context.model.record(key.substr(7));
		if (!record)
			return;
		std::vector<const cinsn_t*> items;
		for (const cinsn_t& s : *m_function.cfunc->body.cblock)
			items.push_back(&s);
		size_t returns = 0;
		std::vector<const cinsn_t*> destructions;
		forEachStatement(&m_function.cfunc->body, [&](const cinsn_t* s) {
			returns += s->op == cit_return;
			if (destroysParameter(s, v, *record))
				destructions.push_back(s);
		});
		// the body's last statement, or the one before the plain return ending it
		size_t last = items.size();
		while (last > 0 && m_frame.count(items[last - 1]))
			--last;
		bool finalReturn = last > 0 && items[last - 1]->op == cit_return && m_scopes.plainReturn(items[last - 1]);
		if (finalReturn)
			--last;
		std::vector<const cinsn_t*> dtor;
		if (last > 0 && returns == (finalReturn ? 1u : 0u))
		{
			std::vector<std::pair<Step, size_t>> dtors;
			if (smart || ownDestructorOnly(*record))
				dtors = structorSteps(record->name, "dtor");
			std::vector<int> copies;
			for (size_t n = 1; n <= 3 && dtor.empty(); ++n)
			{
				std::vector<const cinsn_t*> window = backward(items, 0, last - 1, n);
				if (window.size() < n)
					break;
				copies.clear();
				if (structor(window, dtors, v, std::nullopt, &copies) &&
					std::all_of(copies.begin(), copies.end(), [&](int t) { return m_scopes.usedOnlyIn(t, window); }))
					dtor = window;
			}
			if (dtor.empty() && destructCall(items[last - 1], v, *record, smart))
				dtor = {items[last - 1]};
		}
		bool own = !dtor.empty() && destructions.size() <= 1 &&
				   (destructions.empty() || destructions.front() == dtor.back()) && !labelledInside(dtor);
		if (own)
		{
			eraseReplaced(m_function, dtor, {});
			m_function.noOps.insert(dtor.begin(), dtor.end());
			return;
		}
		for (const cinsn_t* s : destructions)
			m_function.meaning[s->cexpr] = Missing{"destroys the by-value parameter " + m_function.locals[v].name +
												   ", which C++ destroys again where the function returns"};
	}

	// `F(&v);`, a resolved call of the record's Destruct or its C++ destructor
	bool destroysParameter(const cinsn_t* s, int v, const SrcModel::Record& record) const
	{
		if (s->op != cit_expr || s->cexpr->op != cot_call || s->cexpr->a->size() != 1)
			return false;
		const cexpr_t* argument = stripCasts(&s->cexpr->a->front());
		if (argument->op != cot_ref || lvarIndex(stripCasts(argument->x)) != v)
			return false;
		auto found = m_function.meaning.find(s->cexpr);
		const Call* call = found != m_function.meaning.end() ? std::get_if<Call>(&found->second) : nullptr;
		if (!call || !call->function || call->function->cls != record.name)
			return false;
		return structorName(lastComponent(call->function->name)) == "Destruct" || call->function->structor == "dtor";
	}

private:
	// the steps of the instance's user-declared structors of a kind ("ctor", "dtor") that src defines inline from the
	// class template (srcdb's steps rows: none for a specialization with a body of its own, as NiPointer<ActorCause>'s
	// destructor), each with its parameter count
	std::vector<std::pair<Step, size_t>> structorSteps(const std::string& record, const char* kind) const
	{
		std::vector<std::pair<Step, size_t>> found;
		for (const SrcModel::Function* f : m_context.model.structorsOf(record))
			if (const std::string* text = m_context.model.steps(f->mangled); text && f->structor == kind)
				if (std::optional<Step> steps = Step::parse(*text))
					found.emplace_back(std::move(*steps), f->params.size());
		return found;
	}

	// statements match one of the structors' steps, `this` being v's address and the parameter (one where the
	// structor has one) the given value; known: locals holding a value there (keptCopy)
	bool structor(const std::vector<const cinsn_t*>& statements, const std::vector<std::pair<Step, size_t>>& structors,
				  int v, const std::optional<Step>& parameter, std::vector<int>* copies,
				  const std::unordered_map<int, Step>& known = {}) const
	{
		std::unordered_map<std::string, Step> bound{{"this", Step{"", "&v" + std::to_string(v), {}}}};
		if (parameter)
			bound.emplace("p0", *parameter);
		for (const auto& [steps, params] : structors)
			if (params == (parameter ? 1u : 0u) && m_steps.match(statements, steps.bind(bound), v, copies, known))
				return true;
		return false;
	}

	// `t = e;` (t a local): e, else nullptr
	static const cexpr_t* assigned(const cinsn_t* s, int t)
	{
		return s->op == cit_expr && s->cexpr->op == cot_asg && lvarIndex(s->cexpr->x) == t ? s->cexpr->y : nullptr;
	}

	// s (and the statements inside it) use v only to read v.m_pObject (no store, no address, no other use)
	bool readsOnly(const cinsn_t* s, int v) const
	{
		int all = 0, reads = 0, writes = 0;
		forEachStatement(s, [&](const cinsn_t* inner) {
			for (const cexpr_t* own : ownExpressions(inner))
				forEachSubexpression(own, [&](const cexpr_t* e) {
					auto object = [&](const cexpr_t* x) {
						return x && (lvarIndex(x) == v || (x->op == cot_memref && lvarIndex(x->x) == v));
					};
					all += lvarIndex(e) == v;
					reads += e->op == cot_memref && lvarIndex(e->x) == v;
					writes += (e->op == cot_asg || e->op == cot_ref || e->op == cot_preinc || e->op == cot_predec ||
							   e->op == cot_postinc || e->op == cot_postdec) &&
							  object(e->x);
				});
		});
		return all == reads && writes == 0;
	}

	// Hex-Rays' copy t of v.m_pObject that the destructor's steps (window, from items[at]) use where the exe keeps the
	// pointer in a register across v's writes (a loop's): every assignment of t is a top-level `t = v.m_pObject;` or a
	// `t = 0;` in v's block before v's null constructor (nothing between using v or t); every statement writing v
	// (other than the constructor's) is followed in its block by such a copy, nothing between using v or t, leaving
	// the block or carrying a label; and t holds a copy on every path to the window: the `t = 0;`, or a copy in v's
	// block, or in the body of a do-while loop there, between the constructor and the window. t holds v.m_pObject at
	// the window then. -1: none.
	int keptCopy(int v, const Scopes::Scope& scope, size_t count, bool fromNull, size_t at,
				 const std::vector<const cinsn_t*>& window) const
	{
		const std::vector<const cinsn_t*>& items = scope.items;
		const lvars_t& lvars = *m_function.cfunc->get_lvars();
		auto inWindow = [&](const cinsn_t* s) {
			return std::any_of(window.begin(), window.end(), [&](const cinsn_t* w) { return m_scopes.inside(s, w); });
		};
		auto copy = [&](const cinsn_t* s, int t) {
			const cexpr_t* value = assigned(s, t);
			std::optional<Step> step = value ? m_steps.lower(value) : std::nullopt;
			return step && step->text() == "(ld 4 &v" + std::to_string(v) + ")";
		};
		// the statements of the block holding s, and s's index there (nullopt: s is no block's item)
		auto position = [&](const cinsn_t* s) -> std::optional<std::pair<std::vector<const cinsn_t*>, size_t>> {
			const cinsn_t* block = m_scopes.parent(s);
			if (!block || block->op != cit_block)
				return std::nullopt;
			std::vector<const cinsn_t*> list;
			size_t index = 0;
			for (const cinsn_t& item : *block->cblock)
			{
				if (&item == s)
					index = list.size();
				list.push_back(&item);
			}
			return std::pair{std::move(list), index};
		};
		auto untouched = [&](const cinsn_t* s, int t) {
			return !std::any_of(m_scopes.uses(v).begin(), m_scopes.uses(v).end(),
								[&](const cinsn_t* u) { return m_scopes.inside(u, s); }) &&
				   !std::any_of(m_scopes.uses(t).begin(), m_scopes.uses(t).end(),
								[&](const cinsn_t* u) { return m_scopes.inside(u, s); });
		};
		for (size_t k = 0; k < lvars.size(); ++k)
		{
			int t = static_cast<int>(k);
			const std::vector<const cinsn_t*>& uses = m_scopes.uses(t);
			if (t == v || lvars[k].is_arg_var() || m_function.locals[k].aliasOf >= 0 ||
				!std::any_of(uses.begin(), uses.end(), inWindow))
				continue;
			bool ok = true, initialised = false;
			for (const cinsn_t* use : uses)
			{
				if (inWindow(use))
					continue;
				const cexpr_t* value = assigned(use, t);
				std::optional<std::pair<std::vector<const cinsn_t*>, size_t>> where = position(use);
				if (value && isZero(value) && fromNull && m_scopes.parent(use) == scope.block)
				{
					// `t = 0;` before the null constructor, nothing between using v or t
					size_t z = where ? where->second : items.size();
					for (size_t n = z + 1; ok && n < scope.first; ++n)
						ok = untouched(items[n], t);
					ok = ok && z < scope.first;
					initialised = true;
				}
				else if (value)
				{
					ok = copy(use, t) && where;
					// a copy dominating the window: in v's block, or in the body of a do-while loop there
					const cinsn_t* holder = m_scopes.parent(use);
					const cinsn_t* loop = holder ? m_scopes.parent(holder) : nullptr;
					const cinsn_t* top = holder == scope.block ? use : loop && loop->op == cit_do ? loop : nullptr;
					for (size_t n = scope.first + count; top && n < at; ++n)
						initialised |= items[n] == top;
				}
				else
				{
					// any other use only reads t
					forEachStatement(use, [&](const cinsn_t* inner) {
						for (const cexpr_t* own : ownExpressions(inner))
							forEachSubexpression(own, [&](const cexpr_t* e) {
								bool write = e->op == cot_asg || e->op == cot_ref || e->op == cot_preinc ||
											 e->op == cot_predec || e->op == cot_postinc || e->op == cot_postdec;
								ok = ok && !(write && lvarIndex(e->x) == t);
							});
					});
				}
				if (!ok)
					break;
			}
			// every statement writing v, the constructor's aside, followed by a copy
			for (const cinsn_t* use : m_scopes.uses(v))
			{
				if (!ok)
					break;
				bool constructor = false;
				for (size_t n = scope.first; n < scope.first + count; ++n)
					constructor |= items[n] == use;
				if (constructor || inWindow(use) || readsOnly(use, v))
					continue;
				std::optional<std::pair<std::vector<const cinsn_t*>, size_t>> where = position(use);
				ok = where && !std::any_of(uses.begin(), uses.end(), [&](const cinsn_t* u) {
					return m_scopes.inside(u, use);
				});
				size_t n = where ? where->second + 1 : 0;
				for (; ok && n < where->first.size() && !copy(where->first[n], t); ++n)
					ok = untouched(where->first[n], t) && where->first[n]->label_num < 0 &&
						 !Scopes::leaves({where->first[n]});
				ok = ok && n < where->first.size() && where->first[n]->label_num < 0;
			}
			if (ok && initialised)
				return t;
		}
		return -1;
	}

	// `v.m_pObject = value;`: the value, else nullptr
	const cexpr_t* store(const cinsn_t* s, int v) const
	{
		if (s->op != cit_expr || s->cexpr->op != cot_asg || s->cexpr->x->op != cot_memref ||
			lvarIndex(s->cexpr->x->x) != v)
			return nullptr;
		auto found = m_function.meaning.find(s->cexpr->x);
		const Member* member = found == m_function.meaning.end() ? nullptr : std::get_if<Member>(&found->second);
		return member && member->field->name == "m_pObject" ? s->cexpr->y : nullptr;
	}

	// `p = &v;` into a local p that no other statement uses (Hex-Rays' dead copy of v's address)
	bool deadAddressStore(const cinsn_t* s, int v) const
	{
		if (s->op != cit_expr || s->cexpr->op != cot_asg)
			return false;
		int p = lvarIndex(s->cexpr->x);
		const cexpr_t* address = stripCasts(s->cexpr->y);
		return p >= 0 && p != v && address->op == cot_ref && lvarIndex(stripCasts(address->x)) == v &&
			   m_scopes.usedOnlyIn(p, {s});
	}

	// the record's out-of-line destructor called on v, a copy of the destructor C++ runs there: for a smart pointer
	// `Record::Destruct(&v);` (porting rules: transparent functions); for any other record the exe function src
	// registers the record's C++ destructor at, or `Record::Destruct(&v);` where that destructor is src's inline
	// template member (its steps row: the call is its out-of-line copy)
	bool destructCall(const cinsn_t* s, int v, const SrcModel::Record& record, bool smart) const
	{
		if (s->op != cit_expr || s->cexpr->op != cot_call || s->cexpr->a->size() != 1)
			return false;
		const cexpr_t* argument = stripCasts(&s->cexpr->a->front());
		if (argument->op != cot_ref || lvarIndex(stripCasts(argument->x)) != v)
			return false;
		auto found = m_function.meaning.find(s->cexpr);
		const Call* call = found != m_function.meaning.end() ? std::get_if<Call>(&found->second) : nullptr;
		bool destruct = call && call->function && call->function->cls == record.name &&
						structorName(lastComponent(call->function->name)) == "Destruct";
		if (smart)
			return destruct;
		const SrcModel::Function* dtor = nullptr;
		for (const SrcModel::Function* f : m_context.model.structorsOf(record.name))
			if (f->structor == "dtor")
				dtor = f;
		if (!dtor)
			return false;
		if (destruct && m_context.model.steps(dtor->mangled))
			return true;
		if (s->cexpr->x->op != cot_obj)
			return false;
		const std::vector<uint64_t>& at = m_context.model.bodyAddresses(dtor->mangled);
		return std::find(at.begin(), at.end(), static_cast<uint64_t>(s->cexpr->x->obj_ea)) != at.end();
	}

	// destroying an object of the record runs its own destructor only (no base or by-value member destroys anything)
	bool ownDestructorOnly(const SrcModel::Record& record) const
	{
		for (const SrcModel::Base& base : record.bases)
			if (destroys(m_context.model, base.record))
				return false;
		for (const SrcModel::Field& field : record.fields)
		{
			const SrcModel::Type* held = m_context.model.type(m_context.model.storage(field.type));
			if (held && held->kind == "record" && held->name != record.name && destroys(m_context.model, held->name))
				return false;
		}
		return true;
	}

	// the last n statements of items[first..last] ending at last, the EH frame's skipped (fewer: not that many)
	std::vector<const cinsn_t*> backward(const std::vector<const cinsn_t*>& items, size_t first, size_t last,
										 size_t n) const
	{
		std::vector<const cinsn_t*> window;
		for (size_t k = last + 1; k > first && window.size() < n; --k)
			if (!m_frame.count(items[k - 1]))
				window.insert(window.begin(), items[k - 1]);
		return window;
	}

	// items[last] ends the block, or comes right before a plain return ending it (the EH frame's statements aside)
	bool endsBlock(const std::vector<const cinsn_t*>& items, size_t last) const
	{
		std::vector<const cinsn_t*> after;
		for (size_t k = last + 1; k < items.size(); ++k)
			if (!m_frame.count(items[k]))
				after.push_back(items[k]);
		return after.empty() ||
			   (after.size() == 1 && after.back() == items.back() && m_scopes.plainReturn(items.back()));
	}

	// an expression statement, unlabelled, that neither uses v nor calls anything; or one of the EH frame's (not
	// printed)
	bool inert(const cinsn_t* s, int v) const
	{
		if (m_frame.count(s))
			return true;
		if (s->op != cit_expr || s->label_num >= 0)
			return false;
		bool calls = false;
		forEachSubexpression(s->cexpr, [&](const cexpr_t* e) { calls |= e->op == cot_call; });
		const std::vector<const cinsn_t*>& uses = m_scopes.uses(v);
		return !calls && std::find(uses.begin(), uses.end(), s) == uses.end();
	}

	// a resolved call other than first returns a record by value into v: C++ would assign it from a temporary it
	// then destroys, which the exe's fill in place does not do
	bool refilled(int v, const cexpr_t* first) const
	{
		bool found = false;
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			auto meaning = e->op == cot_call && e != first ? m_function.meaning.find(e) : m_function.meaning.end();
			const Call* call = meaning != m_function.meaning.end() ? std::get_if<Call>(&meaning->second) : nullptr;
			if (!call || call->resultArgument < 0 || static_cast<size_t>(call->resultArgument) >= e->a->size())
				return;
			const cexpr_t* argument = stripCasts(&(*e->a)[call->resultArgument]);
			found |= argument->op == cot_ref && lvarIndex(stripCasts(argument->x)) == v;
		});
		return found;
	}

	// v as a plain C++ local of the record; smart: the smart pointer's constructors and destructor, any other record
	// only as a by-value call's result (its first write) and its destructor
	void structorLocal(int v, const SrcModel::Record& record, bool smart)
	{
		Scopes::Scope scope = m_scopes.scopeOf(v);
		const std::vector<const cinsn_t*>& items = scope.items;
		std::vector<std::pair<Step, size_t>> ctors = structorSteps(record.name, "ctor");
		// the destructor's steps are the whole destruction where no base or member destroys anything
		std::vector<std::pair<Step, size_t>> dtors;
		if (smart || ownDestructorOnly(record))
			dtors = structorSteps(record.name, "dtor");
		// the destructor's steps end the block (or come right before a plain return ending it), else the lifetime
		// prints in a block of its own
		bool framed = !endsBlock(items, scope.last);
		// Hex-Rays' dead copy of v's address first (`p = &v;`, p read nowhere): dropped with the constructor's steps
		size_t lead = scope.first < scope.last && deadAddressStore(items[scope.first], v) ? 1 : 0;
		const cinsn_t* ctor = items[scope.first + lead];
		size_t count = 0;
		std::string form;
		std::vector<const citem_t*> kept;
		const cexpr_t* filled = nullptr;
		if (const cexpr_t* value = smart ? store(ctor, v) : nullptr)
		{
			// the constructor: the default one (`v.m_pObject = 0;`), or one from the pointer stored (with the
			// reference it takes on it)
			std::optional<Step> pointer = m_steps.lower(value);
			const cinsn_t* next = scope.first + lead + 1 < items.size() ? items[scope.first + lead + 1] : nullptr;
			if (structor({ctor}, ctors, v, std::nullopt, nullptr))
				count = lead + 1, form = "$0;", kept = {ctor->cexpr->x->x};
			else if (pointer && next && structor({ctor, next}, ctors, v, pointer, nullptr) &&
					 !unresolvedIn(m_function, ctor->cexpr))
				count = lead + 2, form = "$0 = $1;", kept = {ctor->cexpr->x->x, value};
			else
				return;
		}
		else
		{
			// the instance a call returns by value into v (MSVC's hidden result pointer): C++ constructs v from the
			// call's result in place, as the exe has the callee build it there
			filled = byValue(ctor, v);
			if (!filled || (!smart && refilled(v, filled)))
				return;
			int result = std::get<Call>(m_function.meaning.at(filled)).resultArgument;
			count = lead + 1;
			form = "$0 = $1;";
			kept = {stripCasts(stripCasts(&(*filled->a)[result])->x), filled};
		}
		// the destructor: the block's last statements doing the destructor's steps (with Hex-Rays' copies of values
		// they use, dropped with them); or the call of the record's out-of-line destructor (its Destruct) on v, a copy
		// of the destructor C++ runs there (porting rules: transparent functions)
		std::vector<const cinsn_t*> dtor;
		std::vector<int> copies;
		for (size_t n = 1; n <= 3 && dtor.empty(); ++n)
		{
			std::vector<const cinsn_t*> window = backward(items, scope.first + count, scope.last, n);
			if (window.size() < n)
				break;
			copies.clear();
			if (structor(window, dtors, v, std::nullopt, &copies) &&
				std::all_of(copies.begin(), copies.end(), [&](int t) { return m_scopes.usedOnlyIn(t, window); }))
				dtor = window;
		}
		// MSVC scheduled Hex-Rays' copy of v.m_pObject the release uses ahead of statements that neither use v nor
		// the copy nor call anything (`t = v.m_pObject; g = 0; if (v.m_pObject) { ...t... }`): the copy and the release
		// are the destructor's steps, the statements between stay where they are, before them as in the exe
		if (dtor.empty() && scope.last > scope.first + count)
		{
			size_t at = scope.last;
			while (at > scope.first + count && inert(items[at - 1], v))
				--at;
			if (at < scope.last && at > scope.first + count)
			{
				std::vector<const cinsn_t*> window{items[at - 1], items[scope.last]};
				copies.clear();
				if (structor(window, dtors, v, std::nullopt, &copies) && copies.size() == 1 &&
					m_scopes.usedOnlyIn(copies.front(), window))
					dtor = window;
			}
		}
		if (dtor.empty() && scope.last >= scope.first + count && destructCall(items[scope.last], v, record, smart))
			dtor = {items[scope.last]};
		// the destructor's steps on Hex-Rays' copy of v.m_pObject the exe kept across v's writes: right after v's last
		// use, v's lifetime running through them
		for (size_t n = 1; n <= 2 && smart && dtor.empty(); ++n)
		{
			std::vector<const cinsn_t*> window;
			size_t end = scope.last + 1;
			for (; end < items.size() && window.size() < n; ++end)
				if (!m_frame.count(items[end]))
					window.push_back(items[end]);
			if (window.size() < n)
				break;
			int t = keptCopy(v, scope, count, form == "$0;", scope.last + 1, window);
			Step object{"ld", "", {Step{"", "4", {}}, Step{"", "&v" + std::to_string(v), {}}}};
			copies.clear();
			if (t >= 0 && structor(window, dtors, v, std::nullopt, &copies, {{t, object}}) && copies.empty())
			{
				dtor = window;
				scope.last = end - 1;
				framed = !endsBlock(items, scope.last);
			}
		}
		if (dtor.empty() && !filled)
			argument(v, scope, lead, count, kept.size() > 1 ? static_cast<const cexpr_t*>(kept[1]) : nullptr);
		if (dtor.empty() || (framed && !frames(v, scope)) || !declare(v, scope, count, form, kept, dtor, framed))
			return;
		if (framed)
			m_function.frames[items[scope.first]] = scope.last + 1 - scope.first;
		// the call's result is v's initialiser, no assignment
		if (filled)
			m_function.temporaries.insert(filled);
	}

	// v built only to be passed by value to the call its last statement is (MSVC builds a by-value argument in place
	// and the callee destroys it): the argument prints as the temporary C++ builds there, `NiPointer<T>()` or
	// `NiPointer<T>(p)` (value: the pointer the constructor stores), and the constructor's statements (and Hex-Rays'
	// dead copies of v's address) are not printed. Moved to the call, a constructor taking a reference crosses no
	// other call; the default one stores only to v, which nothing else reads.
	void argument(int v, const Scopes::Scope& scope, size_t lead, size_t count, const cexpr_t* value)
	{
		const std::vector<const cinsn_t*>& items = scope.items;
		const cinsn_t* last = items[scope.last];
		if (scope.last < scope.first + count || last->op != cit_expr || last->cexpr->op != cot_call ||
			m_function.meaning.count(last))
			return;
		const cexpr_t* call = last->cexpr;
		auto found = m_function.meaning.find(call);
		const Call* resolved = found != m_function.meaning.end() ? std::get_if<Call>(&found->second) : nullptr;
		const SrcModel::Type* signature = resolved && resolved->function && !resolved->through
											  ? m_context.model.type(resolved->function->type)
											  : nullptr;
		if (!signature)
			return;
		// v by value, the call's only use of it
		const cexpr_t* passed = nullptr;
		int seen = 0;
		forEachSubexpression(call, [&](const cexpr_t* e) { seen += lvarIndex(e) == v; });
		for (size_t k = 0; k < call->a->size(); ++k)
		{
			const cexpr_t* argument = &(*call->a)[k];
			int parameter = parameterIndex(*resolved, k);
			if (lvarIndex(stripCasts(argument)) == v && parameter >= 0 &&
				static_cast<size_t>(parameter) < signature->params.size() &&
				m_context.types.canonical(signature->params[parameter]) ==
					m_context.types.canonical(m_function.locals[v].type))
				passed = argument;
		}
		if (!passed || seen != 1)
			return;
		auto calls = [](const cinsn_t* s) {
			bool found = false;
			forEachStatement(s, [&](const cinsn_t* inner) {
				for (const cexpr_t* e : ownExpressions(inner))
					forEachSubexpression(e, [&](const cexpr_t* x) { found |= x->op == cot_call; });
			});
			return found;
		};
		// the reference a constructor from a pointer takes stays before every call it preceded (the call's other
		// operands included)
		int inner = 0;
		forEachSubexpression(call, [&](const cexpr_t* e) { inner += e->op == cot_call; });
		if (value && inner != 1)
			return;
		// every other statement using v: the constructor's, or a dead copy of v's address
		std::vector<const cinsn_t*> dropped(items.begin() + scope.first + lead, items.begin() + scope.first + count);
		std::vector<const cinsn_t*> between(items.begin() + scope.first, items.begin() + scope.last);
		const std::vector<const cinsn_t*>& uses = m_scopes.uses(v);
		for (size_t n = scope.first; n < scope.last; ++n)
		{
			if (n >= scope.first + lead && n < scope.first + count)
				continue;
			if (deadAddressStore(items[n], v))
				dropped.push_back(items[n]);
			else if (std::any_of(uses.begin(), uses.end(), [&](const cinsn_t* use) {
						 return m_scopes.inside(use, items[n]);
					 }) ||
					 (value && calls(items[n])))
				return;
		}
		std::optional<std::string> type = m_context.types.spell(m_function.locals[v].type);
		if (!type || labelledInside(between) || m_scopes.jumpedPast(scope.block, scope.first))
			return;
		std::vector<const citem_t*> kept;
		if (value)
			kept.push_back(value);
		eraseReplaced(m_function, dropped, kept);
		m_function.meaning[passed] = Rewrite{1, *type + (value ? "($0)" : "()"), kept, {}, m_function.locals[v].type};
		m_function.meaning[last] = Rewrite{1, "$0;", {call}, dropped};
		m_temporary = true;
	}

	// `F(..., &v, ...);`, a resolved call returning v's NiPointer by value through the hidden result pointer: the call
	// (nullptr for any other statement)
	const cexpr_t* byValue(const cinsn_t* s, int v) const
	{
		if (s->op != cit_expr || s->cexpr->op != cot_call)
			return nullptr;
		auto found = m_function.meaning.find(s->cexpr);
		const Call* call = found != m_function.meaning.end() ? std::get_if<Call>(&found->second) : nullptr;
		if (!call || call->resultArgument < 0 || !call->function ||
			static_cast<size_t>(call->resultArgument) >= s->cexpr->a->size())
			return nullptr;
		const cexpr_t* argument = stripCasts(&(*s->cexpr->a)[call->resultArgument]);
		const SrcModel::Type* type = m_context.model.type(call->function->type);
		if (argument->op != cot_ref || lvarIndex(stripCasts(argument->x)) != v || !type ||
			m_context.types.canonical(type->ret) != m_context.types.canonical(m_function.locals[v].type))
			return nullptr;
		return s->cexpr;
	}

	// v's lifetime, the scope's statements first..last, can print as a block of its own: every other local first used
	// in them is used only in them (its declaration goes inside), no goto from outside them jumps in, and no frame
	// already in the block overlaps them without nesting
	bool frames(int v, const Scopes::Scope& scope) const
	{
		std::vector<const cinsn_t*> lifetime(scope.items.begin() + scope.first, scope.items.begin() + scope.last + 1);
		auto within = [&](const cinsn_t* s) {
			return std::any_of(lifetime.begin(), lifetime.end(), [&](const cinsn_t* t) { return m_scopes.inside(s, t); });
		};
		const lvars_t& lvars = *m_function.cfunc->get_lvars();
		for (size_t k = 0; k < lvars.size(); ++k)
		{
			const std::vector<const cinsn_t*>& uses = m_scopes.uses(static_cast<int>(k));
			if (static_cast<int>(k) == v || lvars[k].is_arg_var() || uses.empty() || !within(uses.front()))
				continue;
			// one used past the frame too is declared outside it, unless an earlier pass placed it (placeDeclarations
			// declares it before the frame, the inlined-locals pass and this one declare nothing a frame splits)
			if (!std::all_of(uses.begin(), uses.end(), within) && m_function.placed(k))
				return false;
		}
		for (const cinsn_t* s : lifetime)
		{
			bool entered = false;
			forEachStatement(s, [&](const cinsn_t* inner) {
				if (inner->label_num >= 0)
					for (const cinsn_t* g : m_scopes.gotosTo(inner->label_num))
						entered |= !within(g);
			});
			if (entered)
				return false;
		}
		for (size_t n = 0; n < scope.items.size(); ++n)
		{
			auto frame = m_function.frames.find(scope.items[n]);
			if (frame == m_function.frames.end())
				continue;
			size_t end = n + frame->second;
			bool nested = (n >= scope.first && end <= scope.last + 1) || (n <= scope.first && end > scope.last);
			bool apart = end <= scope.first || n > scope.last;
			if (!nested && !apart)
				return false;
		}
		return true;
	}

	// v declared by the rewrite of the scope's first `count` statements (form after the type, $0 v), the destructor's
	// steps (dropped) not printed; not where a goto jumps into the scope past the declaration, where control leaves
	// the scope before the destructor's steps, or where another local's frame holds the declaration but not v's end.
	// Framed: a goto past the whole frame jumps past no declaration (frames() keeps gotos out of the lifetime).
	bool declare(int v, const Scopes::Scope& scope, size_t count, const std::string& form,
				 std::vector<const citem_t*> kept, const std::vector<const cinsn_t*>& dropped, bool framed)
	{
		std::vector<const cinsn_t*> lifetime(scope.items.begin() + scope.first, scope.items.begin() + scope.last + 1);
		std::vector<const cinsn_t*> replaced(scope.items.begin() + scope.first,
											 scope.items.begin() + scope.first + count);
		std::optional<std::string> type = m_context.types.spell(m_function.locals[v].type);
		if (!type || Scopes::leaves(lifetime) || (!framed && m_scopes.jumpedPast(scope.block, scope.first)) ||
			labelledInside(replaced, replaced.front()) || labelledInside(dropped) ||
			splitByFrame(m_function, scope.items, scope.first, scope.last))
			return false;
		replaced.insert(replaced.end(), dropped.begin(), dropped.end());
		eraseReplaced(m_function, replaced, kept);
		Local& local = m_function.locals[v];
		local.declaredAt = scope.items[scope.first];
		local.initialised = true;
		m_function.meaning[scope.items[scope.first]] = Rewrite{count, *type + " " + form, kept, dropped};
		return true;
	}

	Function& m_function;
	Context& m_context;
	Steps m_steps;
	Scopes m_scopes;
	// the EH frame's statements (dropEhFrame leaves them out): not part of any structor's steps
	std::set<const cinsn_t*> m_frame;
	bool m_temporary = false;  // the local niPointer looked at became a by-value argument's temporary
};

}  // namespace

void resolveStructorLocals(Function& function, Context& context)
{
	StructorLocals(function, context).run();
}

}  // namespace idakit
