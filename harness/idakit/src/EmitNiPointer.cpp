// NiPointer assignment: the exe's inlined `NiPointer<T>::operator=` printed as src writes it, under the EXE-INLINED
// marker src uses for the engine's out-of-line copy. Three shapes, each in one block:
//
//     old = sp.m_pObject;                                   optional, right before the if
//     if (old != p)                                         old: that local, or sp.m_pObject read again
//     {
//         if (old)                                          or `if (old && !InterlockedDecrement(...))`
//             if (!InterlockedDecrement(&old->m_uiRefCount))
//                 old->DeleteThis();
//         sp.m_pObject = p;
//         if (p)
//             InterlockedIncrement(&p->m_uiRefCount);
//     }
//
// is `sp = p;` (operator=(T*), 0x66B0D0): p is a local, read once as the by-value argument is;
//
//     if (old != other.m_pObject) { release old; q = other.m_pObject; sp.m_pObject = q; if (q) hold q; }
//
// is `sp = other;` (operator=(const NiPointer&), 0xC663D0), the other pointer read again after the release, both
// the same NiPointer<T>;
//
//     if (old) { if (!InterlockedDecrement(&old->m_uiRefCount)) old->DeleteThis(); sp.m_pObject = 0; }
//
// is `sp = nullptr;` (operator=(T*) with a null pointer, as the compiler folds it).
//
// That is src's operator= (NiPointer.h/.inl, NiRefCount.h): `if (m_pObject != p) { if (m_pObject)
// NiDecRefCount(m_pObject); m_pObject = p; if (m_pObject) NiIncRefCount(m_pObject); }`, the refcount at +4 and
// DeleteThis in vtable slot 1 as NiRefObject has them (EmitRefCounts). The store resolves to src's
// NiPointer<T>::m_pObject (sp is a src NiPointer), old is used nowhere else, and no statement replaced carries a
// label. Matched by value (EmitValue): through casts, copies of locals (aliases; their copy statements between old's
// load and the if are part of the rewrite, and print nothing anyway) and Hex-Rays' views of the same address
// (`&old->m_uiRefCount`, `old + 1`, `*(*old + 4)`), so Hex-Rays' typing of old does not matter.
//
// Any instance whose inline operator= srcdb has steps rows for (EmitRefCounts' Steps): an if (with Hex-Rays' copies
// before it) doing exactly the steps of operator=(T*) on the NiPointer it stores to, with the local it stores or
// null, or of operator=(const NiPointer&) with the other NiPointer it compares, prints the same way, under the marker
// of the out-of-line copy src names for that operator (a body, forwarder or fold ref; else one of an operator with the
// same steps, the linker folding identical code), else `none:`. Tried where the NiRefObject shapes above do not match.
//
// Then the other transparent functions of the porting rules the exe inlines, on any value the statements read (a
// local, a member, a global or static member): NiPointer's assignments, and its destructor on a member of the object
// a destructor destroys (`apThis->spX.~NiPointer<T>();`, the exe's member destruction); NiRefCount.h's and
// NiRefObject's count helpers (`NiDecRefCount(p);`); NiFixedString's assignments and NiGlobalStringTable's handle
// helpers (`NiGlobalStringTable::ReleaseHandle(kName.GetHandle());`). Each where a run of statements (one, with up to
// three of Hex-Rays' copies before it) does exactly the helper's steps with values of the run for its parameters,
// under the marker of its out-of-line copy (else `none:`).
//
// Last in the pass list: the layers' meanings inside the statements it replaces are erased (the form prints only sp
// and the value), and the declarations of the locals used only there go.

#include "EmitRefCounts.hpp"
#include "EmitScopes.hpp"
#include "IdaSupport.hpp"

namespace idakit
{

namespace
{

class Assignments
{
public:
	Assignments(Function& function, Context& context)
		: m_function(function), m_context(context), m_counts(function, context), m_steps(function, context),
		  m_scopes(function), m_copies(function.copies())
	{
	}

	void run()
	{
		forEachStatement(&m_function.cfunc->body, [&](const cinsn_t* s) {
			if (s->op != cit_block || m_done.count(s))
				return;
			std::vector<const cinsn_t*> items = childStatements(s);
			for (size_t k = 0; k < items.size(); ++k)
			{
				// the statement before, past aliases' copy statements
				size_t before = k;
				while (before > 0 && m_copies.count(items[before - 1]))
					--before;
				std::vector<const cinsn_t*> run(items.begin() + (before > 0 ? before - 1 : k), items.begin() + k + 1);
				if (!match(before > 0 ? items[before - 1] : nullptr, run) && !bySteps(items, k))
					byHelper(items, k);
			}
		});
	}

private:
	// the NiPointer a statement `sp.m_pObject = ...;` stores to, and the old pointer the if compares: the local
	// `old = sp.m_pObject;` the statement before the if sets, else sp.m_pObject read again
	struct Target
	{
		const cexpr_t* sp = nullptr;  // the NiPointer (or a pointer to it: deref true)
		bool deref = false;
		const SrcModel::Record* record = nullptr;  // its src record, `NiPointer<T>`
		Value reread;							   // sp.m_pObject
		int local = -1;							   // old's lvar; -1: none
		Value old;								   // the value old stands for
	};

	// the if ending run (the statements from the one before it, past copy statements), previous that one
	bool match(const cinsn_t* previous, const std::vector<const cinsn_t*>& run)
	{
		const cinsn_t* s = run.back();
		if (s->op != cit_if || s->cif->ielse || s->cif->ithen->op != cit_block)
			return false;
		std::vector<const cinsn_t*> then = childStatements(s->cif->ithen);
		const cexpr_t& test = s->cif->expr;
		if (test.op == cot_ne && then.size() == 3)
			return assignment(previous, run, then[0], then[1], then[2]);
		if (test.op == cot_ne && then.size() == 4)
			return copying(previous, run, then[0], then[1], then[2], then[3]);
		if (then.size() == 2)
			return clearing(previous, run, then[0], then[1]);
		return false;
	}

	// The if items[k] (with up to three statements before it that only copy values: Hex-Rays' temporaries, used
	// nowhere else) does exactly the steps of the instance's inline operator= (srcdb's steps row; none where src
	// specializes it), its result unused, `this` the NiPointer whose m_pObject the if stores to: operator=(T*) with
	// the pointer stored (a local, read once as the by-value argument is; or null: `sp = nullptr;`), or
	// operator=(const NiPointer&) with the other NiPointer the if compares with. Printed as `apply` prints, under the
	// marker of the out-of-line copy src names for that operator= (its body, forwarder or fold ref), else `none:`.
	bool bySteps(const std::vector<const cinsn_t*>& items, size_t k)
	{
		const cinsn_t* s = items[k];
		if (s->op != cit_if || s->cif->ielse)
			return false;
		const cinsn_t* store = nullptr;
		forEachStatement(s->cif->ithen, [&](const cinsn_t* t) {
			if (!store && t->op == cit_expr && t->cexpr->op == cot_asg && niPointer(t->cexpr->x))
				store = t;
		});
		const cexpr_t* field = store ? store->cexpr->x : nullptr;
		const SrcModel::Record* record = field ? niPointer(field) : nullptr;
		std::optional<Step> location = record ? m_steps.lower(field) : std::nullopt;
		if (!location || location->op != "ld" || location->kids.size() != 2 || unresolvedIn(m_function, store->cexpr))
			return false;
		const cexpr_t* stored = stripCasts(store->cexpr->y);
		const cexpr_t* other = s->cif->expr.op == cot_ne ? stripCasts(s->cif->expr.y) : nullptr;
		for (const SrcModel::Function* f : m_context.model.functionsNamed(record->name + "::operator="))
		{
			const SrcModel::Type* type = m_context.model.type(f->type);
			const std::string* text = m_context.model.steps(f->mangled);
			std::optional<Step> steps = text ? Step::parse(*text) : std::nullopt;
			if (!type || type->params.size() != 1 || !steps)
				continue;
			// the parameter's value, the form after sp, the items it prints, the local the value is in
			std::optional<Step> parameter;
			std::string rest;
			std::vector<const citem_t*> printed{field->x};
			int except = -1;
			if (type->params[0].starts_with("ptr(") && isZero(stored))
			{
				parameter = Step{"", "#0", {}};
				rest = " = nullptr;";
			}
			else if (type->params[0].starts_with("ptr(") && lvarIndex(stored) >= 0)
			{
				except = lvarIndex(stored);
				parameter = Step{"", "v" + std::to_string(except), {}};
				rest = " = $1;";
				printed.push_back(store->cexpr->y);
			}
			else if (!type->params[0].starts_with("ptr(") && other && niPointer(other) == record)
			{
				std::optional<Step> read = m_steps.lower(other);
				if (!read || read->op != "ld" || read->kids.size() != 2)
					continue;
				parameter = read->kids[1];
				rest = other->op == cot_memref ? " = $1;" : " = *$1;";
				printed.push_back(other->x);
			}
			else
				continue;
			Step bound = withoutReturn(*steps).bind({{"this", location->kids[1]}, {"p0", *parameter}});
			for (size_t n = 0; n <= 3 && n <= k; ++n)
			{
				std::vector<const cinsn_t*> run(items.begin() + (k - n), items.begin() + k + 1);
				std::vector<int> copies;
				if (!fresh(run) || !m_steps.match(run, bound, except, &copies) ||
					!std::all_of(copies.begin(), copies.end(), [&](int t) { return m_scopes.usedOnlyIn(t, run); }) ||
					labelledInside(run, run.front()))
					continue;
				eraseReplaced(m_function, run, printed);
				for (int local : copies)
					m_function.locals[local].declaredAt = nullptr;
				std::string sp = field->op == cot_memptr ? "*$0" : "$0";
				m_function.meaning[run.front()] =
					Rewrite{run.size(), "// EXE-INLINED " + outOfLine(*f, *text) + "\n" + sp + rest, printed, {}};
				done(run);
				return true;
			}
		}
		return false;
	}

	// The statements are no other fold's: none inside one this pass made, none another pass prints otherwise (a
	// rewrite's, a dropped one, one a rewrite before it in its block covers). Left-out statements (an EH frame's
	// state stores, no-op calls) are no steps and print nothing anyway.
	bool fresh(const std::vector<const cinsn_t*>& run) const
	{
		std::set<const cinsn_t*> dropped = m_function.dropped();
		const cinsn_t* block = m_scopes.parent(run.front());
		std::vector<const cinsn_t*> items = block ? childStatements(block) : std::vector<const cinsn_t*>{};
		std::set<const cinsn_t*> covered;
		for (size_t k = 0; k < items.size(); ++k)
			if (auto found = m_function.meaning.find(items[k]); found != m_function.meaning.end())
				if (const Rewrite* rewrite = std::get_if<Rewrite>(&found->second))
					for (size_t n = 1; n < rewrite->count && k + n < items.size(); ++n)
						covered.insert(items[k + n]);
		bool clean = true;
		for (const cinsn_t* s : run)
			forEachStatement(s, [&](const cinsn_t* t) {
				clean = clean && !m_done.count(t) && !m_function.meaning.count(t) && !dropped.count(t) &&
						!covered.count(t);
			});
		return clean;
	}

	// the statements a fold replaced (and those inside them) are done
	void done(const std::vector<const cinsn_t*>& run)
	{
		for (const cinsn_t* s : run)
			forEachStatement(s, [&](const cinsn_t* t) { m_done.insert(t); });
	}

	// A src helper the porting rules' transparent functions name, where the exe inlines it: the run (items[k] with up
	// to three statements before it, Hex-Rays' copies used only there) does exactly the helper's steps (srcdb's
	// steps row), its result unused, with each of its parameters (`this` first) bound to a value the run reads: an
	// object or a pointer of the parameter's class (or one deriving from it at offset 0), a pointer that converts
	// to the parameter's type implicitly, null for a pointer. Printed as src calls it, `p->f(...)`, `F(...)`, `a =
	// b;` for an assignment, `x.~X();` for NiPointer's destructor (only on a member of the object a destructor
	// destroys: the exe's member destruction), under the marker of the out-of-line copy src names (else `none:`).
	bool byHelper(const std::vector<const cinsn_t*>& items, size_t k)
	{
		if (!countsReferences(items[k]))
			return false;
		for (size_t n = 0; n <= 3 && n <= k; ++n)
		{
			std::vector<const cinsn_t*> run(items.begin() + (k - n), items.begin() + k + 1);
			if (!fresh(run) || labelledInside(run, run.front()))
				continue;
			std::vector<const cexpr_t*> values = valuesIn(run);
			for (const SrcModel::Function* f : helpers(values))
				if (helper(*f, run, values))
					return true;
		}
		return false;
	}

	// a statement (or one inside it) counts a reference: an Interlocked import's call, or a counter step
	bool countsReferences(const cinsn_t* s) const
	{
		bool found = false;
		for (const cexpr_t* e : expressionsIn({s}))
			found = found || is_prepost(e->op) || (e->op == cot_call && stripCasts(e->x)->op == cot_obj &&
												   !functionStart(stripCasts(e->x)));
		return found;
	}

	std::vector<const cexpr_t*> expressionsIn(const std::vector<const cinsn_t*>& run) const
	{
		std::vector<const cexpr_t*> all;
		for (const cinsn_t* s : run)
			forEachStatement(s, [&](const cinsn_t* t) {
				for (const cexpr_t* own : ownExpressions(t))
					forEachSubexpression(own, [&](const cexpr_t* e) { all.push_back(e); });
			});
		return all;
	}

	// the run's expressions of a src type that read without an effect, one per value
	std::vector<const cexpr_t*> valuesIn(const std::vector<const cinsn_t*>& run) const
	{
		std::vector<const cexpr_t*> values;
		std::set<std::string> seen;
		for (const cexpr_t* e : expressionsIn(run))
		{
			bool effect = false;
			forEachSubexpression(e, [&](const cexpr_t* x) { effect = effect || x->op == cot_call || written(x); });
			std::optional<Step> step = effect ? std::nullopt : m_steps.lower(e);
			if (step && !srcType(m_function, m_context, e).empty() && seen.insert(step->text()).second)
				values.push_back(e);
		}
		return values;
	}

	// the helpers that can take a value of the run: the assignments and the destructor of each NiPointer the run
	// reads, then the other transparent functions the porting rules name
	std::vector<const SrcModel::Function*> helpers(const std::vector<const cexpr_t*>& values) const
	{
		std::vector<const SrcModel::Function*> found;
		std::set<std::string> records;
		for (const cexpr_t* e : values)
			if (const SrcModel::Record* record =
					m_context.types.templateRecord(srcType(m_function, m_context, e), "NiPointer", false))
				if (records.insert(record->name).second)
					for (const char* name : {"::operator=", "::~NiPointer"})
						for (const SrcModel::Function* f : m_context.model.functionsNamed(record->name + name))
							found.push_back(f);
		for (const char* name : {"NiRefObject::DecRefCount", "NiRefObject::IncRefCount", "NiDecRefCount", "NiIncRefCount",
								 "NiFixedString::operator=", "NiGlobalStringTable::ReleaseHandle",
								 "NiGlobalStringTable::AcquireHandle"})
			for (const SrcModel::Function* f : m_context.model.functionsNamed(name))
				found.push_back(f);
		return found;
	}

	// a value bound to a parameter and how the call prints it
	struct Argument
	{
		Step step;
		const cexpr_t* item = nullptr;  // nullptr: null, printed `nullptr`
		bool object = false;			// an lvalue of the class, bound to its address (else a pointer's value)
	};

	// the values of the run that can be the parameter of type key (this: of the class): an object of the class (or
	// of one deriving from it at offset 0) for a reference or `this`, bound to its address; a pointer to one for
	// `this`; a pointer converting to the parameter's type implicitly, or null
	std::vector<Argument> argumentsFor(const std::string& key, const std::string& cls,
									   const std::vector<const cexpr_t*>& values) const
	{
		const SrcTypes& types = m_context.types;
		std::vector<Argument> found;
		auto derives = [&](const std::string& type, const std::string& base) {
			SrcTypes::Parts parts = types.parts(types.canonical(type));
			return parts.kind == "record" && types.derivesAtZero(parts.name, base);
		};
		std::optional<PointeeCv> to = cls.empty() ? pointeeOf(types, key) : std::nullopt;
		bool reference = !cls.empty() || key.starts_with("lref(");
		std::string base = !cls.empty() ? cls : to ? types.parts(types.canonical(to->type)).name : "";
		for (const cexpr_t* e : values)
		{
			std::string type = srcType(m_function, m_context, e);
			std::optional<PointeeCv> from = pointeeOf(types, type);
			bool lvalue = e->op == cot_var || e->op == cot_memref || e->op == cot_memptr || e->op == cot_ptr ||
						  e->op == cot_obj;
			std::optional<Step> step;
			if (reference && lvalue && !base.empty() && derives(type, base) && (step = m_steps.address(e)))
				found.push_back({*step, e, true});
			else if (!cls.empty() && from && derives(from->type, cls) && (step = m_steps.lower(e)))
				found.push_back({*step, e, false});
			else if (cls.empty() && !reference && to && from && (step = m_steps.lower(e)) &&
					 (types.canonical(from->type) == types.canonical(to->type) || derives(from->type, base)) &&
					 (!from->isConst || to->isConst))
				found.push_back({*step, e, false});
		}
		if (cls.empty() && !reference && to)
			found.push_back({Step{"", "#0", {}}, nullptr, false});
		return found;
	}

	// the run does the helper's steps with values of the run for its parameters: printed as src calls it
	bool helper(const SrcModel::Function& f, const std::vector<const cinsn_t*>& run,
				const std::vector<const cexpr_t*>& values)
	{
		const std::string* text = m_context.model.steps(f.mangled);
		std::optional<Step> steps = text ? Step::parse(*text) : std::nullopt;
		const SrcModel::Type* type = m_context.model.type(f.type);
		if (!steps || !type)
			return false;
		bool member = !f.cls.empty() && !f.isStatic;
		// each parameter's candidates, `this` first
		std::vector<std::vector<Argument>> candidates;
		std::vector<std::string> names;
		if (member)
		{
			candidates.push_back(argumentsFor("", f.cls, values));
			names.push_back("this");
		}
		for (size_t p = 0; p < type->params.size(); ++p)
		{
			candidates.push_back(argumentsFor(type->params[p], "", values));
			names.push_back("p" + std::to_string(p));
		}
		Step body = withoutReturn(*steps);
		std::vector<size_t> pick(candidates.size(), 0);
		for (;;)
		{
			if (std::any_of(candidates.begin(), candidates.end(), [](const auto& c) { return c.empty(); }))
				return false;
			std::unordered_map<std::string, Step> bound;
			std::vector<const Argument*> chosen;
			for (size_t p = 0; p < candidates.size(); ++p)
			{
				chosen.push_back(&candidates[p][pick[p]]);
				bound.emplace(names[p], chosen.back()->step);
			}
			std::vector<int> copies;
			if (m_steps.match(run, body.bind(bound), -1, &copies) && print(f, *text, run, chosen, member, copies))
				return true;
			// the next combination
			size_t p = 0;
			while (p < pick.size() && ++pick[p] == candidates[p].size())
				pick[p++] = 0;
			if (p == pick.size())
				return false;
		}
	}

	// The run printed as the call of f with the chosen arguments; false where a printed value reads a copy the run
	// makes (gone with it), is unresolved, or the copies are used past the run
	bool print(const SrcModel::Function& f, const std::string& text, const std::vector<const cinsn_t*>& run,
			   const std::vector<const Argument*>& chosen, bool member, const std::vector<int>& copies)
	{
		if (!std::all_of(copies.begin(), copies.end(), [&](int t) { return m_scopes.usedOnlyIn(t, run); }))
			return false;
		std::vector<const citem_t*> items;
		std::vector<std::string> spelled;
		for (const Argument* argument : chosen)
		{
			if (!argument->item)
			{
				spelled.push_back("nullptr");
				continue;
			}
			bool readsCopy = false;
			forEachSubexpression(argument->item, [&](const cexpr_t* e) {
				readsCopy = readsCopy || std::find(copies.begin(), copies.end(), lvarIndex(e)) != copies.end();
			});
			if (readsCopy || unresolvedIn(m_function, argument->item))
				return false;
			spelled.push_back("$" + std::to_string(items.size()));
			items.push_back(argument->item);
		}
		std::string form;
		std::string name = lastComponent(f.name);
		size_t first = member ? 1 : 0;
		std::string arguments;
		for (size_t p = first; p < spelled.size(); ++p)
			arguments += (p > first ? ", " : "") + spelled[p];
		if (member && (name == "operator=" || f.structor == "dtor") && !chosen[0]->object)
			return false;
		if (member && name == "operator=")
			form = spelled[0] + " = " + arguments + ";";
		else if (member && f.structor == "dtor")
		{
			// only the exe's destruction of a member of the object a destructor destroys
			const cexpr_t* object = objectOf(m_function, chosen[0]->item);
			if ((m_function.structor != "Destruct" && m_function.structor != "DeletingDestruct") || !object ||
				!structorObject(m_function, lvarIndex(object)) || chosen[0]->item == object)
				return false;
			std::optional<std::string> cls = m_context.types.spell("record:" + f.cls);
			if (!cls)
				return false;
			form = spelled[0] + ".~" + *cls + "();";
		}
		else if (member)
			form = spelled[0] + (chosen[0]->object ? "." : "->") + name + "(" + arguments + ");";
		else
			form = f.name + "(" + arguments + ");";
		eraseReplaced(m_function, run, items);
		for (int local : copies)
			m_function.locals[local].declaredAt = nullptr;
		std::string header = f.file.rfind("src/", 0) == 0 ? f.file.substr(4) : f.file;
		m_function.meaning[run.front()] = Rewrite{
			run.size(), "// EXE-INLINED " + outOfLine(f, text) + "\n" + form, items, {}, "", member ? "" : header};
		done(run);
		return true;
	}

	// steps without the value they return (a statement's call)
	static Step withoutReturn(Step steps)
	{
		if (steps.op != "seq")
			return steps;
		while (!steps.kids.empty() && steps.kids.back().op == "ret")
			steps.kids.pop_back();
		for (Step& kid : steps.kids)
			kid = withoutReturn(std::move(kid));
		return steps;
	}

	// The exe's out-of-line copy of the operator: the address of src's body, forwarder or fold ref of it, else of an
	// instance's operator with the same steps (the linker folds identical code); `none: NAME` for none
	std::string outOfLine(const SrcModel::Function& f, const std::string& text) const
	{
		std::optional<uint64_t> same;
		for (const auto& [address, refs] : m_context.model.refs())
			for (const SrcModel::Ref& ref : refs)
			{
				if (ref.kind != "body" && ref.kind != "forwarder" && ref.kind != "fold")
					continue;
				if (ref.function == f.mangled)
					return hexAddress(address);
				const std::string* other = same ? nullptr : m_context.model.steps(ref.function);
				const SrcModel::Function* g = other ? m_context.model.function(ref.function) : nullptr;
				if (g && *other == text && lastComponent(g->name) == "operator=")
					same = address;
			}
		return same ? hexAddress(*same) : "none: " + f.name;
	}

	static std::string hexAddress(uint64_t address)
	{
		char text[32];
		std::snprintf(text, sizeof text, "0x%llX", static_cast<unsigned long long>(address));
		return text;
	}

	// `if (old != p) { release; sp.m_pObject = p; hold; }`: `sp = p;`
	bool assignment(const cinsn_t* previous, const std::vector<const cinsn_t*>& run, const cinsn_t* dec,
					const cinsn_t* store, const cinsn_t* inc)
	{
		std::optional<Target> target = targetOf(previous, store);
		std::optional<Value> p = target ? m_counts.value(store->cexpr->y) : std::nullopt;
		if (!p || lvarIndex(stripCasts(store->cexpr->y)) < 0)
			return false;
		const cexpr_t& test = run.back()->cif->expr;
		return isOld(*target, test.x) && m_counts.is(test.y, *p) && m_counts.release(dec, target->old) &&
			   m_counts.hold(inc, *p) &&
			   apply(run, *target, store, "0x66B0D0", " = $1;", {target->sp, store->cexpr->y}, -1);
	}

	// `if (old != other.m_pObject) { release; p = other.m_pObject; sp.m_pObject = p; hold; }`, the other pointer
	// read again after the release: src's operator=(const NiPointer&) (the engine's 0xC663D0), `sp = other;`. Both
	// are the same NiPointer<T> (from another, C++ would take operator=(T*), which reads the other pointer once).
	bool copying(const cinsn_t* previous, const std::vector<const cinsn_t*>& run, const cinsn_t* dec,
				 const cinsn_t* reload, const cinsn_t* store, const cinsn_t* inc)
	{
		std::optional<Target> target = targetOf(previous, store);
		const cinsn_t* s = run.back();
		const cexpr_t& test = s->cif->expr;
		const cexpr_t* other = stripCasts(test.y);
		if (!target || niPointer(other) != target->record || reload->op != cit_expr || reload->cexpr->op != cot_asg)
			return false;
		int p = lvarIndex(reload->cexpr->x);
		std::optional<Value> read = m_counts.value(other);
		if (p < 0 || !read || !m_counts.is(reload->cexpr->y, *read) || lvarIndex(stripCasts(store->cexpr->y)) != p)
			return false;
		if (!isOld(*target, test.x) || !m_counts.release(dec, target->old) || !m_counts.hold(inc, Value::local(p)) ||
			!m_scopes.usedOnlyIn(p, {s}))
			return false;
		std::string form = other->op == cot_memref ? " = $1;" : " = *$1;";
		return apply(run, *target, store, "0xC663D0", form, {target->sp, other->x}, p);
	}

	// `if (old) { if (!InterlockedDecrement(...)) old->DeleteThis(); sp.m_pObject = 0; }`, operator= with a null
	// pointer as the compiler folds it: `sp = nullptr;`
	bool clearing(const cinsn_t* previous, const std::vector<const cinsn_t*>& run, const cinsn_t* dec,
				  const cinsn_t* store)
	{
		std::optional<Target> target = targetOf(previous, store);
		if (!target || !isZero(store->cexpr->y) || !isOld(*target, &run.back()->cif->expr))
			return false;
		return dec->op == cit_if && !dec->cif->ielse &&
			   m_counts.decrement(&dec->cif->expr, dec->cif->ithen, target->old, target->old) &&
			   apply(run, *target, store, "0x66B0D0", " = nullptr;", {target->sp}, -1);
	}

	bool isOld(const Target& target, const cexpr_t* e) const
	{
		return m_counts.is(e, target.old) || m_counts.is(e, target.reread);
	}

	std::optional<Target> targetOf(const cinsn_t* previous, const cinsn_t* store) const
	{
		if (store->op != cit_expr || store->cexpr->op != cot_asg)
			return std::nullopt;
		const cexpr_t* field = store->cexpr->x;
		std::optional<Value> where = m_counts.location(field);
		const SrcModel::Record* record = niPointer(field);
		if (!record || !where)
			return std::nullopt;
		Target target{field->x, field->op == cot_memptr, record, where->load(), -1, {}};
		if (previous && previous->op == cit_expr && previous->cexpr->op == cot_asg &&
			m_counts.is(previous->cexpr->y, target.reread))
			target.local = lvarIndex(previous->cexpr->x);
		target.old = target.local >= 0 ? Value::local(target.local) : target.reread;
		return target;
	}

	// The run (from old's load when there is one, else the if alone) prints as `sp<rest>` under the EXE-INLINED
	// marker of the engine's out-of-line copy (marker). Locals used only there (old, and dropped, -1 for none) are no
	// longer declared.
	bool apply(std::vector<const cinsn_t*> run, const Target& target, const cinsn_t* store, const char* marker,
			   const std::string& rest, std::vector<const citem_t*> items, int dropped)
	{
		if (target.local < 0)
			run.erase(run.begin(), run.end() - 1);
		if (labelledInside(run, run.front()) || (target.local >= 0 && !m_scopes.usedOnlyIn(target.local, run)) ||
			unresolvedIn(m_function, store->cexpr))
			return false;
		eraseReplaced(m_function, run, items);
		for (int local : {target.local, dropped})
			if (local >= 0)
				m_function.locals[local].declaredAt = nullptr;
		std::string sp = target.deref ? "*$0" : "$0";
		m_function.meaning[run.front()] =
			Rewrite{run.size(), std::string("// EXE-INLINED ") + marker + "\n" + sp + rest, items, {}};
		return true;
	}

	// The src record `NiPointer<T>` whose m_pObject `field` (`sp.m_pObject`, `psp->m_pObject`) is, as the Fields
	// pass resolved it; nullptr for any other access.
	const SrcModel::Record* niPointer(const cexpr_t* field) const
	{
		if (field->op != cot_memref && field->op != cot_memptr)
			return nullptr;
		auto found = m_function.meaning.find(field);
		const Member* member = found == m_function.meaning.end() ? nullptr : std::get_if<Member>(&found->second);
		if (!member || member->field->name != "m_pObject" || !member->qualifier.empty())
			return nullptr;
		return m_context.types.templateRecord(srcType(m_function, m_context, field->x), "NiPointer",
											  field->op == cot_memptr);
	}

	Function& m_function;
	Context& m_context;
	RefCounts m_counts;
	Steps m_steps;
	Scopes m_scopes;
	std::set<const cinsn_t*> m_copies;  // the aliases' copy statements
	std::set<const cinsn_t*> m_done;	// the statements a fold of this pass replaced, and those inside them
};

}  // namespace

void rewriteNiPointerAssignments(Function& function, Context& context)
{
	Assignments(function, context).run();
}

}  // namespace idakit
