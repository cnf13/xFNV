// Implicit: the conversions C++ makes without a cast, printed as their operand. A cast of a pointer to a class to a
// pointer to its base at offset 0 (the same address; const kept); the address of a base subobject (`&p->Base`) where
// the base is at offset 0, or where a declared parameter (a member's object) of the base's class or one it derives
// from at offset 0 takes it (C++ adjusts the pointer or reference to it as the exe does, to a void* it does not; a
// pointer only where it is never null: C++ maps null to null, the exe's `lea` adds the offset whatever p is), unless
// the object's class reaches that parameter's class along several paths (C++'s conversion of p would be ambiguous); a
// cast, or a call argument's cast, to the src type its operand has already; a Hex-Rays pointer cast (src's static_cast
// or an EXE-CAST) whose operand converts by itself to where the value goes, as src declares it (a resolved call's
// parameter, the local, field or global assigned, the return type): to the same pointee, to void, to a base at offset
// 0, its const and volatile kept (a volatile added is no drop). The address of a base subobject that is, unchanged
// (through casts to its own type and `return this` accessors), the object of a member access whose name the object's
// class finds alone (memberObject): `IsNode(&p->Base)->m` reads as `p->m`, which adds the offset untested.
// Any other `&p->Base` at a nonzero offset whose p may be null (neverNull: not `this`, a structor's object, a
// reference, a local tested nonzero on the way to it, a variable's address) is the object of a member called on it
// where it is assigned to a local that call alone uses (foldIntoReceiver: `p->F()`, which adjusts p untested as the
// exe's `lea` does), else, where its value is not only read through (onlyReadThrough, through `return this`
// accessors too), it is marked: C++'s conversion would test it.

#include "EmitScopes.hpp"

#include <algorithm>

namespace idakit
{

namespace
{

class Implicits
{
public:
	Implicits(Function& function, Context& context)
		: m_function(function), m_context(context), m_types(context.types), m_parents(parentsOf(function))
	{
	}

	void run()
	{
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			if ((e->op == cot_cast && (upcast(e) || identity(e) || reaches(e))) ||
				(e->op == cot_ref && (baseAddress(e) || memberObject(e))))
				m_function.meaning[e] = Implicit{};
			else if (e->op == cot_ref && isBase(e->x) && offsetOf(e) != 0 && e->x->op == cot_memptr && !neverNull(e) &&
					 !printedAsObject(e) && !foldIntoReceiver(e) && !onlyReadThrough(e))
				m_function.meaning[e] = Missing{"adjusts a pointer that may be null by its base's offset " +
												std::to_string(offsetOf(e)) + " (C++ tests it, the exe does not)"};
			else if (e->op == cot_call)
				if (Call* call = std::get_if<Call>(&m_function.meaning[e]))
					arguments(e, *call);
		});
	}

private:
	// a resolved call's argument: its castTo dropped where it has that type already
	void arguments(const cexpr_t* e, Call& call)
	{
		for (size_t k = 0; k < call.castTo.size() && k < e->a->size(); ++k)
			if (same(call.castTo[k], srcType(m_function, m_context, &(*e->a)[k])))
				call.castTo[k].clear();
	}

	// Hex-Rays' pointer cast (src's key or an ExeCast) whose operand converts by itself to where the cast's value goes;
	// or one Types marks as reinterpreting a pointer to bytes a src declaration types so (a buffer parameter or global:
	// `char (*aBuffer)[512]`, an RTTI descriptor; an element's address of one, `&aBuffer[4]`) as another unrelated one,
	// where the value goes to a `void*`: the buffer's own address, which C++ converts by itself (a pointer to an object
	// of another type, into a field's bytes or a local IDA types as bytes stays marked: its declared type is the fix)
	bool reaches(const cexpr_t* cast) const
	{
		auto found = m_function.meaning.find(cast);
		if (found == m_function.meaning.end())
			return false;
		const Missing* missing = std::get_if<Missing>(&found->second);
		bool buffer = missing && missing->what.starts_with("cast between unrelated pointers (") && bytes(cast->x);
		if (!buffer && !std::holds_alternative<std::string>(found->second) &&
			!std::holds_alternative<ExeCast>(found->second))
			return false;
		// where the value goes, as src declares it, where C++ converts it to that type; a reference parameter, which
		// Hex-Rays passes the address for, binds the object the pointer points to where the pointer converts to the
		// pointer to its referent (`Init(arInfo)` of a `const D&` for a `const B&`, B at offset 0 of D; a reference
		// operand is the address Hex-Rays holds it as)
		Destination to = destinationOf(m_function, m_context, m_parents, cast);
		std::string type = to.type, from = srcType(m_function, m_context, cast->x);
		if (to.byReference && !type.empty())
		{
			type = "ptr(" + m_types.parts(m_types.bare(type)).of + ")";
			if (SrcTypes::Parts operand = m_types.parts(m_types.bare(from)); operand.kind == "lref")
				from = "ptr(" + operand.of + ")";
		}
		std::optional<PointeeCv> into = pointeeOf(m_types, type);
		return !type.empty() && (!buffer || (!to.byReference && into && into->type == "builtin:void")) &&
			   converts(from, type);
	}

	// a pointer to bytes a src declaration types so: a parameter or global (or an element's address, or the array
	// decaying) of a 1-byte integer or an array of them, at any depth of arrays
	bool bytes(const cexpr_t* pointer) const
	{
		std::optional<PointeeCv> held = pointeeOf(m_types, srcType(m_function, m_context, pointer));
		if (!held)
			return false;
		std::string type = held->type;
		for (SrcTypes::Parts p = m_types.parts(type); p.kind == "array"; p = m_types.parts(type))
			type = m_types.canonical(p.of);
		const SrcModel::Type* t = m_types.parts(type).kind == "builtin" ? m_context.model.type(type) : nullptr;
		if (!t || t->size != 1 || t->isFloat || type == "builtin:bool")
			return false;
		const cexpr_t* root = pointer;
		while (root->op == cot_ref || root->op == cot_idx)
			root = root->x;
		auto found = m_function.meaning.find(root);
		int v = lvarIndex(root);
		return (v >= 0 && m_function.locals[v].parameter >= 0) ||
			   (found != m_function.meaning.end() && std::holds_alternative<const SrcModel::Ref*>(found->second));
	}

	// a pointer C++ converts to the declared pointer by itself: to the same pointee, to void, to a base at offset 0
	// (its const and volatile kept; deeper levels alike)
	bool converts(const std::string& from, const std::string& to) const
	{
		std::optional<PointeeCv> a = pointeeOf(m_types, from), b = pointeeOf(m_types, to);
		if (!a || !b || m_types.parts(m_types.bare(from)).kind != "ptr" ||
			m_types.parts(m_types.bare(to)).kind != "ptr" || (a->isConst && !b->isConst) ||
			(a->isVolatile && !b->isVolatile))
			return false;
		if (a->type == b->type)
			return true;
		SrcTypes::Parts x = m_types.parts(a->type), y = m_types.parts(b->type);
		if (b->type == "builtin:void")
			return x.kind != "fn";
		return x.kind == "record" && y.kind == "record" && m_types.derivesAtZero(x.name, y.name);
	}

	// the same src type (typedefs aside, const of a pointee kept)
	bool same(const std::string& a, const std::string& b) const
	{
		return !a.empty() && !b.empty() && m_types.canonical(a) == m_types.canonical(b) &&
			   m_types.constPointee(a) == m_types.constPointee(b);
	}

	// a cast to the src type its operand has
	bool identity(const cexpr_t* cast) const
	{
		auto found = m_function.meaning.find(cast);
		const std::string* key = found == m_function.meaning.end() ? nullptr : std::get_if<std::string>(&found->second);
		return key && same(*key, srcType(m_function, m_context, cast->x));
	}

	// the record a pointer key points to ("" for any other key), and whether the pointee is const
	std::pair<std::string, bool> pointee(const std::string& key) const
	{
		SrcTypes::Parts p = m_types.parts(m_types.bare(key));
		if (p.kind != "ptr")
			return {"", false};
		SrcTypes::Parts to = m_types.parts(m_types.canonical(p.of));
		return {to.kind == "record" ? to.name : "", m_types.constPointee(key)};
	}

	bool upcast(const cexpr_t* cast) const
	{
		auto found = m_function.meaning.find(cast);
		const std::string* key = found == m_function.meaning.end() ? nullptr : std::get_if<std::string>(&found->second);
		if (!key)
			return false;
		auto [base, baseConst] = pointee(*key);
		auto [derived, derivedConst] = pointee(srcType(m_function, m_context, cast->x));
		return !base.empty() && !derived.empty() && base != derived && (baseConst || !derivedConst) &&
			   m_types.derivesAtZero(derived, base);
	}

	bool isBase(const cexpr_t* e) const
	{
		auto found = m_function.meaning.find(e);
		return (e->op == cot_memptr || e->op == cot_memref) && found != m_function.meaning.end() &&
			   std::holds_alternative<std::string>(found->second);
	}

	// the offset of the base subobject whose address ref is, in the object
	uint64_t offsetOf(const cexpr_t* ref) const
	{
		uint64_t offset = 0;
		for (const cexpr_t* b = ref->x; isBase(b); b = b->x)
			offset += b->m;
		return offset;
	}

	// ref is printed as the object a member is read or called through (`p->field`, `p->F()`: the printer reads through
	// p's base subobjects by name, no pointer conversion): a folded accessor's receiver with a field, or a resolved
	// member call's object, past Hex-Rays' casts
	bool printedAsObject(const cexpr_t* ref) const
	{
		const cexpr_t* c = ref;
		for (;;)
		{
			auto up = m_parents.find(c);
			if (up == m_parents.end() || !up->second->is_expr())
				return false;
			const cexpr_t* q = static_cast<const cexpr_t*>(up->second);
			if (q->op == cot_cast)
			{
				c = q;
				continue;
			}
			if (q->op != cot_call || q->a->empty() || &(*q->a)[0] != c)
				return false;
			auto found = m_function.meaning.find(q);
			if (found == m_function.meaning.end())
				return false;
			if (const Access* access = std::get_if<Access>(&found->second))
				return access->field != nullptr;
			const Call* call = std::get_if<Call>(&found->second);
			return call && !call->function->cls.empty() && !call->function->isStatic && call->resultArgument != 0 &&
				   (call->castTo.empty() || call->castTo[0].empty());
		}
	}

	// `v = &p->Base;` (the base at a nonzero offset, p possibly null) whose one use, in a later statement of the same
	// block, is the object of a resolved member call (`v->F()`, its vtable read included) is the source's `p->F()`:
	// C++ adjusts p to the base untested, as the exe's `lea` does, where `static_cast<Base*>(p)` would test it. Only
	// where moving p's evaluation to the call changes nothing: p does nothing (sideEffects), each statement between
	// assigns a local p does not read and whose address is never taken a value that does nothing, the call is the
	// first thing its statement evaluates (through casts, `!`, a compare with a number, a local's assignment), its other
	// arguments do nothing, and no label lies between. Name lookup of F from p's class must find F's class alone and
	// convert p to it along one path; else a direct call names its class (`p->Base::F()`) and a call through the vtable
	// is left (C++ cannot name a virtual from p without a cast). No operator (it prints infix). The use prints as p;
	// the assignment and v's declaration go.
	bool foldIntoReceiver(const cexpr_t* ref)
	{
		auto up = m_parents.find(ref);
		const cexpr_t* asg =
			up != m_parents.end() && up->second->is_expr() ? static_cast<const cexpr_t*>(up->second) : nullptr;
		int v = asg && asg->op == cot_asg && asg->y == ref ? lvarIndex(asg->x) : -1;
		auto top = asg ? m_parents.find(asg) : m_parents.end();
		if (v < 0 || (*m_function.cfunc->get_lvars())[v].is_arg_var() || top == m_parents.end() ||
			top->second->op != cit_expr)
			return false;
		const cinsn_t* assignment = static_cast<const cinsn_t*>(top->second);
		// v's uses past its assignment: the call's object, and through the vtable the callee's read of it (any other
		// use, a second call's object included, is outside that call's callee: no fold)
		std::vector<const cexpr_t*> uses;
		const cexpr_t* use = nullptr;
		const cinsn_t* statement = nullptr;
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t* s) {
			if (e == asg->x || lvarIndex(e) != v)
				return;
			uses.push_back(e);
			auto p = m_parents.find(e);
			const cexpr_t* q =
				p != m_parents.end() && p->second->is_expr() ? static_cast<const cexpr_t*>(p->second) : nullptr;
			if (q && q->op == cot_call && !q->a->empty() && &(*q->a)[0] == e)
			{
				use = e;
				statement = s;
			}
		});
		if (!use)
			return false;
		const cexpr_t* call = static_cast<const cexpr_t*>(m_parents.at(use));
		auto found = m_function.meaning.find(call);
		Call* resolved = found == m_function.meaning.end() ? nullptr : std::get_if<Call>(&found->second);
		if (!resolved || resolved->function->cls.empty() || resolved->function->isStatic ||
			resolved->resultArgument == 0 || resolved->through ||
			(!resolved->castTo.empty() && !resolved->castTo[0].empty()) ||
			lastComponent(resolved->function->name).starts_with("operator"))
			return false;
		bool direct = call->x->op == cot_obj;
		for (const cexpr_t* e : uses)
			if (e != use && (direct || !inside(e, call->x)))
				return false;
		// lookup and conversion from p's class
		std::string object = objectClass(ref);
		if (object.empty() || basePaths(m_context.model, object, resolved->function->cls) != 1)
			return false;
		std::vector<std::string> classes;
		memberLookup(m_context.model, object, lastComponent(resolved->function->name), classes);
		bool hidden = classes.size() != 1 || classes[0] != resolved->function->cls;
		if (hidden && !direct)
			return false;
		// p evaluated at the call instead: nothing between changes what it reads
		const cexpr_t* pointer = ref->x;
		while (isBase(pointer->x))
			pointer = pointer->x;
		pointer = pointer->x;
		if (sideEffects(m_context, pointer))
			return false;
		for (size_t k = 1; k < call->a->size(); ++k)
			if (sideEffects(m_context, &(*call->a)[k]))
				return false;
		Scopes scopes(m_function);
		const cinsn_t* block = scopes.parent(assignment);
		if (!block || block->op != cit_block)
			return false;
		std::vector<const cinsn_t*> items;
		for (const cinsn_t& item : *block->cblock)
			items.push_back(&item);
		auto from = std::find(items.begin(), items.end(), assignment);
		auto to = std::find(items.begin(), items.end(), statement);
		if (from == items.end() || to == items.end() || to <= from || (*to)->label_num != -1 ||
			!evaluatedFirst(call, *to))
			return false;
		std::set<int> read;
		forEachSubexpression(pointer, [&](const cexpr_t* e) {
			if (int r = lvarIndex(e); r >= 0)
				read.insert(r);
		});
		for (auto k = from + 1; k != to; ++k)
		{
			const cinsn_t* s = *k;
			int w = s->op == cit_expr && s->cexpr->op == cot_asg ? lvarIndex(s->cexpr->x) : -1;
			if (s->label_num != -1 || w < 0 || w == v || read.count(w) || scopes.addressTaken(w) ||
				sideEffects(m_context, s->cexpr->y))
				return false;
		}
		if (hidden)
			resolved->qualified = true;
		m_function.meaning[ref] = Implicit{};
		m_function.meaning[use] = Rewrite{1, "$0", {ref}, {assignment}, "", ""};
		return true;
	}

	// e is inside (or is) outer
	bool inside(const cexpr_t* e, const cexpr_t* outer) const
	{
		for (const citem_t* c = e; c;)
		{
			if (c == outer)
				return true;
			auto up = c->is_expr() ? m_parents.find(static_cast<const cexpr_t*>(c)) : m_parents.end();
			c = up == m_parents.end() ? nullptr : up->second;
		}
		return false;
	}

	// the call is the first thing its statement (an expression, an if's condition, a return) evaluates: the path up
	// to it holds only casts, `!`, a compare with a number, a local's assignment
	bool evaluatedFirst(const cexpr_t* call, const cinsn_t* statement) const
	{
		for (const cexpr_t* c = call;;)
		{
			auto up = m_parents.find(c);
			if (up == m_parents.end())
				return false;
			if (!up->second->is_expr())
				return up->second == statement &&
					   (statement->op == cit_expr || statement->op == cit_if || statement->op == cit_return);
			const cexpr_t* q = static_cast<const cexpr_t*>(up->second);
			bool compare = (q->op == cot_eq || q->op == cot_ne || (q->op >= cot_sge && q->op <= cot_ult)) &&
						   (q->x == c ? q->y : q->x)->op == cot_num;
			if (!(q->op == cot_cast || q->op == cot_lnot || compare ||
				  (q->op == cot_asg && q->y == c && lvarIndex(q->x) >= 0)))
				return false;
			c = q;
		}
	}

	// The value of ref, or of the one local it is assigned to (not a parameter, assigned nothing else, its address
	// never taken), is only read through (readThrough, at each of the local's uses). A null p makes C++'s conversion
	// null and the exe's `lea` the base's offset: either faults at the first use that runs, and none tells the two
	// apart before, so C++'s conversion does what the exe does. A `return this` accessor's call (FoldedAccessors'
	// receiver itself) passes the value on unchanged.
	bool onlyReadThrough(const cexpr_t* ref) const
	{
		const cexpr_t* value = passedOn(ref);
		if (readThrough(value))
			return true;
		auto up = m_parents.find(value);
		const cexpr_t* asg =
			up != m_parents.end() && up->second->is_expr() ? static_cast<const cexpr_t*>(up->second) : nullptr;
		int v = asg && asg->op == cot_asg && asg->y == value ? lvarIndex(asg->x) : -1;
		if (v < 0 || (*m_function.cfunc->get_lvars())[v].is_arg_var())
			return false;
		bool only = true;
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			if (only && e != asg->x && lvarIndex(e) == v)
				only = readThrough(passedOn(e));
		});
		return only;
	}

	// e with the Hex-Rays casts around it and the `return this` accessor calls taking it as their receiver, which
	// print as the receiver (FoldedAccessors' Access without a field)
	const cexpr_t* passedOn(const cexpr_t* e) const
	{
		for (auto up = m_parents.find(e); up != m_parents.end() && up->second->is_expr(); up = m_parents.find(e))
		{
			const cexpr_t* q = static_cast<const cexpr_t*>(up->second);
			if (q->op == cot_call && !q->a->empty() && &(*q->a)[0] == e)
			{
				auto found = m_function.meaning.find(q);
				const Access* access = found != m_function.meaning.end() ? std::get_if<Access>(&found->second) : nullptr;
				if (!access || access->field || access->load || access->value)
					break;
			}
			else if (q->op != cot_cast)
				break;
			e = q;
		}
		return e;
	}

	// The base subobject's address ref is, passed on unchanged (passedOn: its type kept), the object of a src field's
	// member access (`(&p->Base)->m`, `IsNode(&p->Base)->m` of the identity accessor) whose name lookup from the
	// object's own class finds that field alone: C++ reads it as `p->m`, the base's offset added with no null test,
	// as the exe's `lea` does
	bool memberObject(const cexpr_t* ref) const
	{
		if (!isBase(ref->x))
			return false;
		const cexpr_t* value = passedOn(ref);
		auto up = m_parents.find(value);
		if (up == m_parents.end() || up->second->op != cot_memptr ||
			static_cast<const cexpr_t*>(up->second)->x != value)
			return false;
		auto found = m_function.meaning.find(up->second);
		const Member* member = found != m_function.meaning.end() ? std::get_if<Member>(&found->second) : nullptr;
		std::string object = objectClass(ref);
		std::string type = srcType(m_function, m_context, ref);
		if (!member || !member->field || !member->qualifier.empty() || object.empty() || type.empty() ||
			m_types.canonical(srcType(m_function, m_context, value)) != m_types.canonical(type))
			return false;
		std::vector<std::string> classes = nameLookup(m_context, object, member->field->name);
		return classes.size() == 1 && classes.front() == member->owner;
	}

	// The pointer e is dereferenced where it is used: a field's object (`p->m`, its vtable pointer included; not the
	// field's address, `&p->m`), a folded accessor's load or store through it, or the object of a resolved virtual
	// call through the vtable, which reads the vtable pointer first
	bool readThrough(const cexpr_t* e) const
	{
		auto up = m_parents.find(e);
		if (up == m_parents.end() || !up->second->is_expr())
			return false;
		const cexpr_t* q = static_cast<const cexpr_t*>(up->second);
		if (q->op == cot_memptr && q->x == e)
		{
			for (const cexpr_t* c = q;;)
			{
				auto outer = m_parents.find(c);
				if (outer == m_parents.end() || !outer->second->is_expr())
					return true;
				const cexpr_t* p = static_cast<const cexpr_t*>(outer->second);
				if (p->op == cot_ref)
					return false;
				if ((p->op != cot_memref && p->op != cot_idx) || p->x != c)
					return true;
				c = p;
			}
		}
		if (q->op != cot_call || q->a->empty() || &(*q->a)[0] != e)
			return false;
		auto found = m_function.meaning.find(q);
		if (found == m_function.meaning.end())
			return false;
		if (const Access* access = std::get_if<Access>(&found->second))
			return access->field && (access->load || access->value);
		const Call* call = std::get_if<Call>(&found->second);
		return call && call->function->isVirtual && q->x->op != cot_obj;
	}

	// the object whose base subobject's address ref is cannot be null, so C++'s conversion of its pointer adds no test
	// the exe lacks: through a pointer, one never null at ref (neverNull); through an object, a variable's storage
	// (`&v.Base`)
	bool neverNull(const cexpr_t* ref) const
	{
		const cexpr_t* b = ref->x;
		while (isBase(b->x))
			b = b->x;
		return b->op == cot_memptr ? idakit::neverNull(m_function, m_parents, b->x, ref)
								   : variableObject(m_function, b->x);
	}

	// the class of the object whose base subobject's address ref is (through the members `&p->A.B` names)
	std::string objectClass(const cexpr_t* ref) const
	{
		const cexpr_t* b = ref->x;
		while (isBase(b->x))
			b = b->x;
		std::string key = srcType(m_function, m_context, b->x);
		if (b->op == cot_memptr)
		{
			SrcTypes::Parts p = m_types.parts(m_types.bare(key));
			if (p.kind != "ptr")
				return "";
			key = p.of;
		}
		SrcTypes::Parts p = m_types.parts(m_types.canonical(key));
		if (p.kind == "const")
			p = m_types.parts(m_types.canonical(p.of));
		return p.kind == "record" ? p.name : "";
	}

	// The record a declared parameter takes ref as, a pointer or reference to it (the member's object: its class),
	// where ref is a resolved call's argument; "" for any other. pointer: taken as a pointer.
	std::string declaredTarget(const cexpr_t* ref, bool& pointer) const
	{
		pointer = false;
		auto parent = m_parents.find(ref);
		if (parent == m_parents.end() || parent->second->op != cot_call)
			return "";
		const cexpr_t* call = static_cast<const cexpr_t*>(parent->second);
		auto found = m_function.meaning.find(call);
		const Call* resolved = found == m_function.meaning.end() ? nullptr : std::get_if<Call>(&found->second);
		if (!resolved)
			return "";
		size_t k = 0;
		while (k < call->a->size() && &(*call->a)[k] != ref)
			++k;
		int parameter = k < call->a->size() ? parameterIndex(*resolved, k) : -1;
		const SrcModel::Type* type = m_context.model.type(resolved->function->type);
		if (parameter >= 0 && type && static_cast<size_t>(parameter) < type->params.size())
		{
			const std::string& key = type->params[parameter];
			SrcTypes::Parts p = m_types.parts(m_types.bare(key));
			if (p.kind == "lref" || p.kind == "rref")
			{
				SrcTypes::Parts to = m_types.parts(m_types.canonical(p.of));
				return to.kind == "record" ? to.name : "";
			}
			pointer = true;
			return pointee(key).first;
		}
		if (k == 0 && k < call->a->size() && resolved->resultArgument != 0 && !resolved->function->isStatic)
			return resolved->function->cls;
		return "";
	}

	bool baseAddress(const cexpr_t* ref) const
	{
		if (!isBase(ref->x))
			return false;
		bool pointer = false;
		std::string target = declaredTarget(ref, pointer);
		// C++ converts the object's own pointer, not the base's: where the parameter's class is a base of the object's
		// along several paths (TESCreature's BaseFormComponents), the conversion is ambiguous and the base's address
		// prints as it is
		if (std::string object = objectClass(ref);
			!target.empty() && !object.empty() && basePaths(m_context.model, object, target) > 1)
			return false;
		if (offsetOf(ref) == 0)
			return true;
		// an argument a declared parameter takes as a pointer or reference to the base's class or a class it derives
		// from at offset 0 (the member's object for its this): C++ adjusts the pointer or reference to it as the exe
		// does, to any other pointer (a Construct's void* apMemory) it does not. A reference (and the object a member
		// is called on) binds the base subobject untested; a pointer is tested for null, so only one never null.
		std::string base = m_types.parts(m_types.canonical(std::get<std::string>(m_function.meaning.at(ref->x)))).name;
		return !target.empty() && !base.empty() && m_types.derivesAtZero(base, target) && (!pointer || neverNull(ref));
	}

	Function& m_function;
	Context& m_context;
	SrcTypes& m_types;
	std::unordered_map<const cexpr_t*, const citem_t*> m_parents;
};

}  // namespace

void markImplicitConversions(Function& function, Context& context)
{
	Implicits(function, context).run();
}

}  // namespace idakit
