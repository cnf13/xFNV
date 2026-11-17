// Const: a src callee that returns a pointer to const, where Hex-Rays' result type has no const, gives the local
// Hex-Rays assigns the result to that const (`const T* v = F();`, through Hex-Rays' pointer casts, which become casts
// to the pointer to const); a result only read through or compared, or going where src's type keeps the const (a
// const parameter, a const member's object, a variadic argument, a field or global of a pointer to const), keeps its
// const where it is. So does a read src has as a pointer to const (a const parameter or local, a field, a global, a
// C string literal, an array element or array member of a const object, the address of an element or member of a
// const object, such a pointer moved by a number). Any other use of such a result, and any use of the local that
// drops the const again (a non-const argument or method, a write through it, an assignment to a destination whose
// src type is a non-const pointer, a return as non-const), is unresolved. An inlined local's Construct call, its
// FNVD_CONSTRUCTED_LOCAL declaration, stays resolved: a drop in its arguments is convertValues' marked const_cast.
//
// A Hex-Rays cast that only drops a const (`(char*)"text"`, `(char*)(const CHAR*)p`, `(char*)v` of a const local) is
// no conversion of its own: it prints as its operand (Implicit), a local it is assigned to becomes const, and the
// value is judged where it goes, as one without the cast (here for a const local, by convertValues for any other: a
// drop only where the destination's src type is not const). A cast of a const local to another type is
// convertValues' to judge.
//
// A trivial accessor's access (FoldedAccessors') is the member access it prints as: its receiver read through (a
// load), written through (a setter's store), or the result (a field's address, a pointer to const into a const object,
// and `return this`: judged where it goes); a setter's value assigned to the field.

#include "EmitPasses.hpp"

#include "IdaSupport.hpp"

#include <map>
#include <set>

namespace idakit
{

namespace
{

class Const
{
public:
	Const(Function& function, Context& context)
		: m_function(function), m_context(context), m_types(context.types), m_parents(parentsOf(function))
	{
	}

	void run()
	{
		constFunctionPointers();
		// a base subobject of a const object is a const base (`static_cast<const B*>(p)`), before its uses are judged,
		// and again once locals are made const (a base of what such a local points to)
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) { constBase(e); });
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			if (e->op == cot_call)
				carry(e);
			else if (dropsConst(m_types, e))
				castAway(e);
			else if (e->op == cot_asg)
				carryRead(e);
		});
		if (m_constLocals.empty())
			return;
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) { constBase(e); });
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			if (int v = lvarIndex(e); v >= 0 && m_constLocals.count(v))
				checkUse(e);
		});
	}

	// a base subobject (Fields' record key on a member access) of an object src has as const: reached through a pointer
	// or reference to const, or a base of such a base; its key the const record, as C++'s `p->Base` of a const p is
	void constBase(const cexpr_t* e)
	{
		if (e->op != cot_memptr && e->op != cot_memref)
			return;
		auto found = m_function.meaning.find(e);
		std::string* key = found == m_function.meaning.end() ? nullptr : std::get_if<std::string>(&found->second);
		if (!key || m_types.bare(*key).starts_with("const(") || !srcConstObject(m_function, m_context, e))
			return;
		*key = "const(" + *key + ")";
	}

private:
	// a cast of Hex-Rays' that drops const: printed as its operand, the const carried onto the local it is assigned to
	void castAway(const cexpr_t* cast)
	{
		auto found = m_function.meaning.find(cast);
		if (found == m_function.meaning.end() || !std::holds_alternative<std::string>(found->second))
			return;  // no src type (unresolved already)
		m_function.meaning[cast] = Implicit{};
		const citem_t* parent = m_parents[cast];
		const cexpr_t* p = parent && parent->op < cit_empty ? static_cast<const cexpr_t*>(parent) : nullptr;
		if (p && p->op == cot_asg && p->y == cast && m_types.constPointee(srcType(m_function, m_context, cast->x)))
			constLocal(lvarIndex(p->x));
	}

	// the local (not a parameter) of a pointer type made a pointer to const; false where it is none
	bool constLocal(int v)
	{
		Local* local = v >= 0 ? &m_function.locals[v] : nullptr;
		if (!local || m_function.cfunc->get_lvars()->at(v).is_arg_var() || local->type.empty() ||
			m_types.parts(m_types.bare(local->type)).kind != "ptr")
			return false;
		if (!m_types.constPointee(local->type))
			local->type = "ptr(const(" + m_types.parts(m_types.bare(local->type)).of + "))";
		m_constLocals.insert(v);
		return true;
	}

	// A local of a pointer to function that Hex-Rays assigns only src reads (a field, a global) of one type that is the
	// local's but for cv and typedefs takes that type, a copy of what it reads: IDA's types carry no const (sync drops
	// it, SrcPush::buildType), so `NIBINARYSTREAMWRITEFN f = s->m_pfnWrite;`, whose pvBuffer is `const void*`, is no
	// reinterpretation. Not where its address is taken, nor a read const itself.
	void constFunctionPointers()
	{
		std::map<int, std::vector<const cexpr_t*>> assigned;
		std::set<int> addressed;
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			if (e->op == cot_asg && lvarIndex(e->x) >= 0)
				assigned[lvarIndex(e->x)].push_back(e->y);
			else if (e->op == cot_ref && lvarIndex(e->x) >= 0)
				addressed.insert(lvarIndex(e->x));
		});
		const lvars_t& lvars = *m_function.cfunc->get_lvars();
		for (const auto& [v, values] : assigned)
		{
			Local& local = m_function.locals[v];
			if (lvars[v].is_arg_var() || addressed.count(v) || local.type.empty())
				continue;
			std::string own = m_types.canonical(local.type);
			SrcTypes::Parts parts = m_types.parts(own);
			if (parts.kind != "ptr" || m_types.parts(parts.of).kind != "fn" ||
				local.type.find("const(") != std::string::npos)
				continue;
			// canonical drops cv at every level: the reads' type is the local's but for const (and typedefs)
			std::optional<std::string> type;
			for (const cexpr_t* value : values)
			{
				std::string read = srcType(m_function, m_context, value);
				if (read.empty() || m_types.parts(m_types.bare(read)).kind == "const" ||
					m_types.canonical(read) != own || (type && *type != read))
				{
					type.reset();
					break;
				}
				type = read;
			}
			if (type)
				local.type = *type;
		}
	}

	// the const of a call's result onto the local it is assigned to, through Hex-Rays' casts to another pointer
	// (each then a cast to the pointer to const); a result only read through (`F()->m`, `*F()` not written),
	// compared, or going where src's type keeps the const (keeps) keeps its const as it is
	void carry(const cexpr_t* call)
	{
		const Call* resolved = std::get_if<Call>(&m_function.meaning[call]);
		const SrcModel::Type* type = resolved ? m_context.model.type(resolved->function->type) : nullptr;
		std::string why;
		std::optional<std::string> result = m_types.keyOf(call->type, why);
		if (!type || !m_types.constPointee(type->ret) || !result)
			return;
		// Hex-Rays' result const already (the database's prototype is src's) and assigned to a local Hex-Rays types
		// without it, which C does and C++ does not: the local is const
		if (m_types.constPointee(*result))
		{
			const citem_t* parent = m_parents[call];
			if (parent && parent->op == cot_asg && static_cast<const cexpr_t*>(parent)->y == call)
				constLocal(lvarIndex(static_cast<const cexpr_t*>(parent)->x));
			return;
		}
		const cexpr_t* value = call;
		std::vector<const cexpr_t*> casts;
		const citem_t* parent = m_parents[call];
		// Hex-Rays' casts it goes through: one printed as its operand (Implicit: a variadic argument's, resolveCalls')
		// as it is, a cast to another pointer (src's static_cast) made a cast to the pointer to const
		while (parent && parent->op == cot_cast)
		{
			const cexpr_t* cast = static_cast<const cexpr_t*>(parent);
			auto key = m_function.meaning.find(cast);
			bool implicit = key != m_function.meaning.end() && std::holds_alternative<Implicit>(key->second);
			if (!implicit && (!cast->type.is_ptr() || key == m_function.meaning.end() ||
							  !std::holds_alternative<std::string>(key->second) ||
							  m_types.parts(m_types.bare(std::get<std::string>(key->second))).kind != "ptr"))
				break;
			value = cast;
			if (!implicit)
				casts.push_back(value);
			parent = m_parents[value];
		}
		auto constant = [&]() {
			for (const cexpr_t* cast : casts)
			{
				std::string& key = std::get<std::string>(m_function.meaning[cast]);
				if (!m_types.constPointee(key))
					key = "ptr(const(" + m_types.parts(m_types.bare(key)).of + "))";
			}
		};
		if (parent && parent->op < cit_empty)
		{
			const cexpr_t* p = static_cast<const cexpr_t*>(parent);
			if ((p->op == cot_asg && p->y == value && constLocal(lvarIndex(p->x))) || readOnly(p, value) ||
				keeps(p, value))
			{
				constant();
				return;
			}
		}
		else if (!parent || parent->op != cit_return || m_types.constPointee(m_function.returnType))
			return;  // a statement of its own (the result is dropped), or returned as const
		m_function.meaning[call] = Missing{"the const " + resolved->function->name + " returns is dropped"};
	}

	// the parent uses the pointer value only to read through it, to compare it (equality, order) or to take its
	// difference from another pointer; the address of what it reaches counts as a read where it goes where src's type
	// keeps the const (`&F()->m` as a const member's object)
	bool readOnly(const cexpr_t* parent, const cexpr_t* value)
	{
		if (parent->op == cot_eq || parent->op == cot_ne || parent->op == cot_lnot || parent->op == cot_sge ||
			parent->op == cot_uge || parent->op == cot_sle || parent->op == cot_ule || parent->op == cot_sgt ||
			parent->op == cot_ugt || parent->op == cot_slt || parent->op == cot_ult)
			return true;
		if (parent->op == cot_sub && parent->x->type.is_ptr() && parent->y->type.is_ptr())
			return true;
		if ((parent->op != cot_memptr && parent->op != cot_ptr && parent->op != cot_idx) || parent->x != value)
			return false;
		// the lvalue it reaches through members and elements is neither written nor addressed
		const cexpr_t* lvalue = parent;
		for (const citem_t* up = m_parents[lvalue]; up && up->op < cit_empty; up = m_parents[lvalue])
		{
			const cexpr_t* p = static_cast<const cexpr_t*>(up);
			if (p->op != cot_memref && !(p->op == cot_idx && p->x == lvalue))
				break;
			lvalue = p;
		}
		const citem_t* up = m_parents[lvalue];
		if (!up || up->op >= cit_empty)
			return true;
		const cexpr_t* p = static_cast<const cexpr_t*>(up);
		if (p->op == cot_ref && p->x == lvalue)
		{
			const cexpr_t* address = p;
			const citem_t* user = implicitUser(address);
			return user && user->op < cit_empty && keeps(static_cast<const cexpr_t*>(user), address);
		}
		return !(written(p) && p->x == lvalue);
	}

	// the destination of the value (the parent's operand) keeps a pointer's const, as src declares it: an assignment
	// to a pointer to const (a local's, a member's, a global's, through typedefs), a resolved call's const parameter
	// or const member's object, a variadic argument
	bool keeps(const cexpr_t* parent, const cexpr_t* value)
	{
		if (parent->op == cot_call)
			return argumentKeeps(parent, value).value_or(false);
		if (parent->op != cot_asg || parent->y != value)
			return false;
		int v = lvarIndex(parent->x);
		std::string to = v >= 0 ? m_function.locals[v].type : srcType(m_function, m_context, parent->x);
		return !to.empty() && m_types.constPointee(to);
	}

	// whether the resolved call's parameter the value is passed for keeps its const (a const member's object, a
	// variadic argument too), or the accessor's access the call prints as does (accessKeeps); nullopt where the call is
	// neither, or the value is no argument of it or for no parameter (a static's or free function's hidden result
	// pointer)
	std::optional<bool> argumentKeeps(const cexpr_t* call, const cexpr_t* value)
	{
		auto found = m_function.meaning.find(call);
		if (found != m_function.meaning.end())
			if (const Access* access = std::get_if<Access>(&found->second))
				return accessKeeps(call, *access, value);
		const Call* resolved = found == m_function.meaning.end() ? nullptr : std::get_if<Call>(&found->second);
		const SrcModel::Type* type = resolved ? m_context.model.type(resolved->function->type) : nullptr;
		if (!type)
			return std::nullopt;  // printed raw: fnvd::original takes any pointer
		bool member = !resolved->function->cls.empty() && !resolved->function->isStatic;
		for (size_t k = 0; k < call->a->size(); ++k)
		{
			if (&(*call->a)[k] != value)
				continue;
			int parameter = parameterIndex(*resolved, k);
			if (parameter < 0)
				return member && k == 0 ? std::optional<bool>(type->isConst) : std::nullopt;
			// a variadic argument (past the declared parameters) takes a const pointer as any other
			return static_cast<size_t>(parameter) >= type->params.size() ||
				   m_types.constPointee(type->params[parameter]);
		}
		return std::nullopt;
	}

	// what a value is to an accessor's access (FoldedAccessors' Access, printed as the member access it is): its
	// receiver (the object argument, or a cast of it), or the value a setter stores (the argument, or a cast the
	// access prints it without, down to the value it prints)
	enum class Role
	{
		none,
		receiver,
		stored,
	};
	Role role(const cexpr_t* call, const Access& access, const cexpr_t* value)
	{
		for (const cexpr_t* e = &(*call->a)[0];; e = e->x)
		{
			if (e == value)
				return Role::receiver;
			if (e->op != cot_cast)
				break;
		}
		for (const cexpr_t* e = access.value; e && e != call;)
		{
			if (e == value)
				return Role::stored;
			const citem_t* up = m_parents[e];
			e = up && up->is_expr() ? static_cast<const cexpr_t*>(up) : nullptr;
		}
		return Role::none;
	}

	// whether the access keeps the const of a pointer that is its receiver or stored value, as the member access it
	// prints as: a receiver read through (a load) keeps it, one a setter's store writes through drops it, and one whose
	// field's address (a pointer to const into the const object) or itself (`return this`) is the result keeps it
	// where that result goes keeping it; a stored value keeps it where the field is a pointer to const. nullopt for a
	// value that is neither.
	std::optional<bool> accessKeeps(const cexpr_t* call, const Access& access, const cexpr_t* value)
	{
		switch (role(call, access, value))
		{
		case Role::receiver:
			if (access.value)
				return false;
			return access.load || keptWhereUsed(call);
		case Role::stored:
			return m_types.constPointee(access.field->type);
		default:
			return std::nullopt;
		}
	}

	// the accessor's access a call prints as; nullptr for none
	const Access* accessOf(const citem_t* call)
	{
		auto found = call->op == cot_call ? m_function.meaning.find(call) : m_function.meaning.end();
		return found == m_function.meaning.end() ? nullptr : std::get_if<Access>(&found->second);
	}

	// the value (a `return this` access's result) keeps the const where it goes, past the casts printed as their
	// operand: a statement's own value or condition, read through or compared, or a destination keeping it
	bool keptWhereUsed(const cexpr_t* value)
	{
		const citem_t* parent = implicitUser(value);
		if (!parent)
			return true;
		if (parent->op == cit_return)
			return m_types.constPointee(m_function.returnType);
		if (parent->op >= cit_empty)
			return true;
		const cexpr_t* p = static_cast<const cexpr_t*>(parent);
		return readOnly(p, value) || keeps(p, value);
	}

	// the item the value goes to past the casts printed as their operand (Implicit), value moved to the outermost
	const citem_t* implicitUser(const cexpr_t*& value)
	{
		const citem_t* parent = m_parents[value];
		while (parent && parent->op == cot_cast)
		{
			auto found = m_function.meaning.find(parent);
			if (found == m_function.meaning.end() || !std::holds_alternative<Implicit>(found->second))
				break;
			value = static_cast<const cexpr_t*>(parent);
			parent = m_parents[value];
		}
		return parent;
	}

	// the accessor's access that stores the value (a setter's, which prints it without Hex-Rays' casts around it);
	// nullptr for none
	const cexpr_t* storeOf(const cexpr_t* value)
	{
		const citem_t* up = m_parents[value];
		while (up && up->op == cot_cast)
			up = m_parents[static_cast<const cexpr_t*>(up)];
		if (!up || up->op != cot_call)
			return nullptr;
		const cexpr_t* call = static_cast<const cexpr_t*>(up);
		auto found = m_function.meaning.find(call);
		const Access* access = found == m_function.meaning.end() ? nullptr : std::get_if<Access>(&found->second);
		return access && role(call, *access, value) == Role::stored ? call : nullptr;
	}

	// the const of a read src has as a pointer to const where Hex-Rays' type has none (a const parameter or local, a
	// field, a global, an array's element, an array member of a const object, which decays to a pointer to const, an
	// accessor's access to one) onto the local it is assigned to
	void carryRead(const cexpr_t* assignment)
	{
		const cexpr_t* read = assignment->y;
		std::string why;
		std::optional<std::string> ida = m_types.keyOf(read->type, why);
		if ((read->op == cot_call && !accessOf(read)) || !ida || m_types.constPointee(*ida) || !constRead(read))
			return;
		constLocal(lvarIndex(assignment->x));
	}

	// src reads a pointer to const there
	bool constRead(const cexpr_t* read)
	{
		auto found = m_function.meaning.find(read);
		const Meaning* meaning = found == m_function.meaning.end() ? nullptr : &found->second;
		switch (read->op)
		{
		case cot_var:
			return m_types.constPointee(srcType(m_function, m_context, read));
		case cot_call:
		{
			// an accessor's access (FoldedAccessors'): a field of a pointer to const, a field's address in a const
			// object, a const receiver itself; a `return this` access is its receiver, whatever its src type reads
			const Access* access = accessOf(read);
			if (!access)
				return false;
			if (!access->field && !access->load && !read->a->empty())
			{
				const cexpr_t* receiver = &(*read->a)[0];
				while (receiver->op == cot_cast)
					receiver = receiver->x;
				if (constRead(receiver))
					return true;
			}
			return m_types.constPointee(srcType(m_function, m_context, read));
		}
		case cot_obj:
		{
			// a src global, or the C string literal resolveGlobals made of the object (`const char*`)
			const Rewrite* literal = meaning ? std::get_if<Rewrite>(meaning) : nullptr;
			if (literal)
				return m_types.constPointee(literal->type);
			return meaning && std::holds_alternative<const SrcModel::Ref*>(*meaning) &&
				   m_types.constPointee(srcType(m_function, m_context, read));
		}
		case cot_memptr:
		case cot_memref:
		{
			const Member* member = meaning ? std::get_if<Member>(meaning) : nullptr;
			if (!member)
				return false;
			if (m_types.constPointee(member->field->type))
				return true;
			// an array member of a const object (read through a pointer or reference to const)
			return read->op == cot_memptr &&
				   m_types.parts(m_types.bare(member->field->type)).kind == "array" &&
				   m_types.constPointee(srcType(m_function, m_context, read->x));
		}
		case cot_idx:
		{
			// an element of a src array or pointer whose elements are pointers to const
			SrcTypes::Parts parts = m_types.parts(m_types.bare(srcType(m_function, m_context, read->x)));
			return (parts.kind == "array" || parts.kind == "ptr") && m_types.constPointee(parts.of);
		}
		case cot_tern:
			// either value a pointer to const (`c ? apConst : &v[i]`): C++'s result points to const
			return constRead(read->y) || constRead(read->z);
		case cot_cast:
		{
			// a cast to void* of an object pointer (`(void*)apText`): C++ converts by itself, keeping the const
			const std::string* key = meaning ? std::get_if<std::string>(meaning) : nullptr;
			return key && m_types.canonical(*key) == "ptr(builtin:void)" && read->x->type.is_ptr() &&
				   !read->x->type.get_pointed_object().is_func() && constRead(read->x);
		}
		case cot_ref:
		{
			// the address of a const object (a const global, `&aEnabled` of a C string src declares as its first
			// `const char`), of an element of a pointer to const or of a const array (`&p[i]`), of a member of a
			// const object (`&p->m`)
			const cexpr_t* x = read->x;
			if (std::string object = srcType(m_function, m_context, x);
				!object.empty() && m_types.bare(object).starts_with("const("))
				return true;
			if (x->op == cot_idx)
			{
				std::string base = srcType(m_function, m_context, x->x);
				SrcTypes::Parts parts = m_types.parts(m_types.bare(base));
				return (parts.kind == "ptr" && m_types.constPointee(base)) ||
					   (parts.kind == "array" && m_types.bare(parts.of).starts_with("const("));
			}
			auto member = m_function.meaning.find(x);
			return x->op == cot_memptr && member != m_function.meaning.end() &&
				   std::holds_alternative<Member>(member->second) &&
				   m_types.constPointee(srcType(m_function, m_context, x->x));
		}
		case cot_add:
		case cot_sub:
			// a pointer to const moved by a number (`p + 1`)
			return read->x->type.is_ptr() && !read->y->type.is_ptr() && constRead(read->x);
		default:
			return false;
		}
	}

	// the use of a const local, where it drops the const; a cast printed as its operand (Implicit: a variadic
	// argument's, resolveCalls') is the local itself, used where the cast is, as is Hex-Rays' cast that only drops its
	// const (to src's pointer without it), which then prints as its operand too. A cast to another type is
	// convertValues' (its conversion from the pointer to const).
	void checkUse(const cexpr_t* var)
	{
		const Local& local = m_function.locals[lvarIndex(var)];
		const std::string& name = local.name;
		const citem_t* parent = m_parents[var];
		auto implicit = [&](const citem_t* item) {
			auto found = m_function.meaning.find(item);
			if (found == m_function.meaning.end())
				return false;
			if (std::holds_alternative<Implicit>(found->second))
				return true;
			const std::string* key = std::get_if<std::string>(&found->second);
			if (!key || !cvApart(m_types, local.type, *key))
				return false;
			found->second = Implicit{};
			return true;
		};
		// a setter's value, assigned to the field (the access prints it without Hex-Rays' casts)
		if (const cexpr_t* store = storeOf(var))
		{
			const Access& access = std::get<Access>(m_function.meaning[store]);
			if (!m_types.constPointee(access.field->type))
				mark(store, "stores const local " + name + " in non-const field " + access.field->name);
			return;
		}
		// the local is used where a cast printed as its operand is, and where a `return this` accessor's access it is
		// the receiver of is (which prints as the local)
		for (;;)
		{
			if (parent && parent->op == cot_cast && implicit(parent))
			{
				var = static_cast<const cexpr_t*>(parent);
				parent = m_parents[var];
				continue;
			}
			const Access* access = parent ? accessOf(parent) : nullptr;
			const cexpr_t* call = static_cast<const cexpr_t*>(parent);
			if (!access || access->field || role(call, *access, var) != Role::receiver)
				break;
			var = call;
			parent = m_parents[var];
		}
		if (!parent)
			return;
		if (parent->op == cit_return)
		{
			if (!m_types.constPointee(m_function.returnType))
				mark(parent, "returns const local " + name + " as non-const");
			return;
		}
		if (parent->op >= cit_empty)
			return;
		const cexpr_t* p = static_cast<const cexpr_t*>(parent);
		if (const Access* access = accessOf(p))
		{
			// a store writes through the receiver; a load reads through it, a field's address is a pointer to const
			// judged where it goes (convertValues: its printed type), a stored value is storeOf's
			if (access->value && role(p, *access, var) == Role::receiver)
				mark(p, "writes through const local " + name + " (an accessor's store)");
		}
		else if (p->op == cot_call)
		{
			// an inlined local's Construct call prints as the local's FNVD_CONSTRUCTED_LOCAL declaration, from its
			// Call: no raw form of it to mark, so the drop is the argument's (convertValues' const_cast, marked)
			if (const Call* resolved = std::get_if<Call>(&m_function.meaning[p]);
				resolved && !constructs(p) && !argumentKeeps(p, var).value_or(true))
				mark(p, "passes const local " + name + " to " + resolved->function->name + " as non-const");
		}
		else if (p->op == cot_asg && p->y == var)
		{
			if (!keeps(p, var))
				mark(p, "assigns const local " + name + " to a non-const pointer");
		}
		else if (p->op == cot_memptr || p->op == cot_ptr || p->op == cot_idx)
			checkWrite(p, name);
		else if (p->op == cot_cast)
			keepConst(p);
	}

	// the call is an inlined local's Construct call (InlinedLocals' FNVD_CONSTRUCTED_LOCAL)
	bool constructs(const cexpr_t* call) const
	{
		for (const Local& local : m_function.locals)
			if (local.inlined && local.inlined->construct == call)
				return true;
		return false;
	}

	// Hex-Rays' cast of a const local to another pointer (src's static_cast), whose value is only read through or
	// goes where src's type keeps the const: a cast to the pointer to const (`static_cast<const float*>(pv)` for a
	// const parameter); any other is convertValues' (its conversion from the pointer to const)
	void keepConst(const cexpr_t* cast)
	{
		std::string* key = std::get_if<std::string>(&m_function.meaning[cast]);
		if (!key || m_types.parts(m_types.bare(*key)).kind != "ptr" || m_types.constPointee(*key))
			return;
		const citem_t* parent = m_parents[cast];
		bool keep = false;
		if (parent && parent->op == cit_return)
			keep = m_types.constPointee(m_function.returnType);
		else if (parent && parent->op < cit_empty)
		{
			const cexpr_t* p = static_cast<const cexpr_t*>(parent);
			keep = readOnly(p, cast) || keeps(p, cast);
		}
		if (keep)
			*key = "ptr(const(" + m_types.parts(m_types.bare(*key)).of + "))";
	}

	// a write through the local: the lvalue it reaches (through members and elements) assigned, stepped or addressed
	void checkWrite(const cexpr_t* access, const std::string& name)
	{
		const cexpr_t* lvalue = access;
		for (const citem_t* up = m_parents[lvalue]; up && up->op < cit_empty; up = m_parents[lvalue])
		{
			const cexpr_t* p = static_cast<const cexpr_t*>(up);
			if (p->op != cot_memref && !(p->op == cot_idx && p->x == lvalue))
				break;
			lvalue = p;
		}
		const citem_t* up = m_parents[lvalue];
		if (!up || up->op >= cit_empty)
			return;
		const cexpr_t* p = static_cast<const cexpr_t*>(up);
		// an address going where src's type keeps the const (a const local assigned it, a const parameter, an
		// accessor's receiver read through)
		const cexpr_t* address = p;
		const citem_t* user = p->op == cot_ref ? implicitUser(address) : nullptr;
		if (user && user->op < cit_empty && keeps(static_cast<const cexpr_t*>(user), address))
			return;
		if ((written(p) || p->op == cot_ref) && p->x == lvalue)
			mark(p, "writes through const local " + name);
	}

	// the const drop at the item, unless an earlier layer left it unresolved for its own reason, which stays
	// (the item prints marked either way; its reason is that layer's spot)
	void mark(const citem_t* item, const std::string& what)
	{
		auto found = m_function.meaning.find(item);
		if (found == m_function.meaning.end() || !std::holds_alternative<Missing>(found->second) ||
			unlooked(found->second))
			m_function.meaning[item] = Missing{what};
	}

	Function& m_function;
	Context& m_context;
	SrcTypes& m_types;
	std::unordered_map<const cexpr_t*, const citem_t*> m_parents;
	std::set<int> m_constLocals;
};

}  // namespace

void carryConst(Function& function, Context& context)
{
	Const(function, context).run();
}

}  // namespace idakit
