// Overloads: a call Calls resolved prints so that C++'s overload resolution picks the callee Calls chose, whatever
// overloads src has. The overload set is srcdb's: the functions of the callee's qualified name and, for a member,
// those its class's using-declarations bring in (using_member); lookup from the receiver reaches that class (Calls
// qualifies or casts the receiver where it would not). Another overload competes unless an argument, as printed,
// cannot convert to its parameter at all (overload resolution drops it), or it takes more arguments than the call
// passes (beyond its default arguments) or fewer (not C-variadic). Where one competes, each argument whose printed
// type is not exactly its parameter's (cv of the pointee included) is cast to the parameter's type: the chosen callee
// then matches every argument exactly, which no other overload beats (a tie is ambiguous: a compile error, never
// another callee). A number for an `int` or `unsigned int` parameter is that type's literal instead (`0`, `0u`;
// user). A const member called on a non-const receiver, beside a non-const overload of the same parameters, gets its
// receiver cast to the pointer to const. What no cast pins (an argument bound to a reference, a qualified call's
// receiver) is marked: a Missing, printed raw, except on a call the printer prints from its Call alone, built on it by
// the passes before (an inlined local's Construct call, which its FNVD_CONSTRUCTED_LOCAL declaration prints; a by-value
// call, whose hidden result is folded back into a local, its declaration or C++'s temporary): that one keeps its Call,
// marked (Call::unpinned). Decided on the arguments' final printed types, after convertValues: an argument it converts
// explicitly is of its parameter's type already, and one it leaves alone C++ converts by itself, so the static_cast
// to the parameter's type is one C++ makes.

#include "EmitPasses.hpp"

#include "IdaSupport.hpp"

#include <set>

namespace idakit
{

namespace
{

// a null or zero argument, which converts where its type would not: `nullptr` (a number Hex-Rays types as a
// pointer) to any pointer, `0` (a number, under any casts) to any pointer or arithmetic type
enum class Literal
{
	none,
	null,
	zero,
};

class Overloads
{
public:
	Overloads(Function& function, Context& context)
		: m_function(function), m_context(context), m_model(context.model), m_types(context.types)
	{
	}

	// the call's pins: castTo entries, the numbers typed as their parameter's literal (literals), or why the call
	// cannot be pinned ("": it is)
	std::string pin(const cexpr_t* call, Call& c)
	{
		const SrcModel::Function& target = *c.function;
		const SrcModel::Type* type = c.through ? nullptr : m_model.type(target.type);
		if (!type)
			return "";
		std::vector<const SrcModel::Function*> others = overloads(target);
		if (others.empty())
			return "";
		const carglist_t& args = *call->a;
		bool member = !target.cls.empty() && !target.isStatic;
		// each parameter's argument as printed: its type and literal form (src's apEdx prints `nullptr`)
		size_t n = type->params.size();
		std::vector<std::string> printed(n);
		std::vector<Literal> literal(n, Literal::none);
		std::vector<int> argument(n, -1);
		for (size_t k = 0; k < args.size(); ++k)
			if (int p = parameterIndex(c, k); p >= 0 && static_cast<size_t>(p) < n)
				argument[static_cast<size_t>(p)] = static_cast<int>(k);
		for (size_t p = 0; p < n; ++p)
		{
			if (static_cast<int>(p) == c.edxParameter)
			{
				literal[p] = Literal::null;
				continue;
			}
			if (argument[p] < 0)
				return "";
			size_t k = static_cast<size_t>(argument[p]);
			const cexpr_t* arg = &args[k];
			literal[p] = literalOf(arg);
			// an argument cast to its parameter's type (an enum's number) or converted to it explicitly
			// (convertValues: what C++ does not convert by itself, a same-size integer of another type) is of that
			// type; a reference's object is its referent's; one C++ converts implicitly (convertValues' implicit, a
			// Hex-Rays cast printed as its operand) is its operand's
			if (!c.castTo[k].empty() || (m_function.conversions.count(arg) && !implicitCast(arg)))
				printed[p] = type->params[p];
			else if (c.byReference[k])
				printed[p] = referred(printedType(arg));
			else
				printed[p] = evaluated(arg, printedType(arg));
		}
		// the overloads that may beat the callee or tie with it: they take as many arguments as the call passes
		// (default arguments for more, a C-variadic one fewer), and for each argument as printed have a conversion
		// at all (else C++ drops them); where the callee's parameter is an exact match and theirs is not, they rank
		// worse there and threaten only where they rank better on another argument (an exact match where the
		// callee's is not: ambiguous at least, a compile error, `pow(double, float)`)
		auto threatens = [&](const SrcModel::Function* other) {
			const SrcModel::Type* o = m_model.type(other->type);
			size_t required = other->required >= 0 ? static_cast<size_t>(other->required) : o ? o->params.size() : 0;
			if (!o || required > n || (o->params.size() < n && !o->variadic))
				return false;
			bool worse = false;
			for (size_t p = 0; p < n; ++p)
			{
				bool own = exact(printed[p], type->params[p], literal[p]);
				if (p >= o->params.size())
				{
					worse = worse || own;
					continue;
				}
				if (cannotConvert(printed[p], literal[p], o->params[p]))
					return false;
				bool theirs = exact(printed[p], o->params[p], literal[p]);
				int tie = own && theirs ? referenceTie(o->params[p], type->params[p]) : 0;
				if ((theirs && !own) || tie > 0)
					return true;
				worse = worse || (own && !theirs) || tie < 0;
			}
			return !worse;
		};
		auto threat = [&]() -> const SrcModel::Function* {
			for (const SrcModel::Function* other : others)
				if (threatens(other))
					return other;
			return nullptr;
		};
		// the arguments not exactly of their parameter's type cast to it, a null or zero first (the usual pin:
		// `static_cast<NiFile*>(0)` beside `Load(char*, int)`), until no overload threatens: each cast an exact
		// match where it was none
		std::vector<size_t> order;
		for (int pass = 0; pass < 2; ++pass)
			for (size_t p = 0; p < n; ++p)
				if (argument[p] >= 0 && (literal[p] != Literal::none) == (pass == 0) &&
					!exact(printed[p], type->params[p], literal[p]))
					order.push_back(p);
		for (size_t p : order)
		{
			const SrcModel::Function* other = threat();
			if (!other)
				return "";
			size_t k = static_cast<size_t>(argument[p]);
			if (c.byReference[k])
				return "argument " + std::to_string(p + 1) + " of " + target.name + " binds a reference that " +
					   other->name + " (" + other->type + ") may take as well: no cast pins the overload";
			printed[p] = type->params[p];
			// a number for an int or unsigned int parameter: that type's literal, which matches it exactly (and is
			// still a null pointer constant if 0); not a type whose width differs on x64 (size_t: its static_cast
			// stays) or any other integer (no suffix names it)
			std::string key = m_types.canonical(type->params[p]);
			if (args[k].op == cot_num && !hostWidth(m_context, type->params[p]) &&
				(key == "builtin:int" || key == "builtin:unsigned int"))
			{
				literals.emplace_back(&args[k], key);
				continue;
			}
			c.castTo[k] = type->params[p];
			literal[p] = Literal::none;
		}
		// every argument an exact match: what still threatens has the callee's parameters, and beats it only on the
		// implicit object, a non-const member on a non-const receiver beside the const callee
		const SrcModel::Function* other = threat();
		std::optional<SrcTypes::Function> self = SrcTypes::function(target.type);
		if (!other || !member || !self || !self->isConst || args.empty())
			return "";
		std::string receiver = c.castTo[0].empty() ? printedType(&args[0]) : c.castTo[0];
		if (m_types.constPointee(m_types.bare(receiver)))
			return "";
		for (const SrcModel::Function* f : others)
			if (std::optional<SrcTypes::Function> o = SrcTypes::function(f->type);
				o && !o->isConst && !f->isStatic && threatens(f))
			{
				if (c.qualified)
					return "a qualified call of the const " + target.name + " on a non-const receiver, beside " +
						   f->name + " (" + f->type + "): no cast pins the overload";
				c.castTo[0] = "ptr(const(record:" + target.cls + "))";
				break;
			}
		return "";
	}

	// the numbers pinned as their parameter's literal: their src type key (the printer's suffix)
	std::vector<std::pair<const cexpr_t*, std::string>> literals;

private:
	// the other functions overload resolution considers beside the target: those of its qualified name, and for a
	// member those of its name its class's using-declarations bring in; no C++ structor (lookup finds none)
	std::vector<const SrcModel::Function*> overloads(const SrcModel::Function& target) const
	{
		std::vector<const SrcModel::Function*> out;
		std::set<std::string> seen{target.mangled};
		auto add = [&](const SrcModel::Function* f) {
			if (f && f->structor.empty() && seen.insert(f->mangled).second)
				out.push_back(f);
		};
		for (const SrcModel::Function* f : m_model.functionsNamed(target.name))
			add(f);
		if (const SrcModel::Record* record = target.cls.empty() ? nullptr : m_model.record(target.cls))
		{
			std::string name = lastComponent(target.name);
			for (const std::string& mangled : record->usingMembers)
				if (const SrcModel::Function* f = m_model.function(mangled); f && lastComponent(f->name) == name)
					add(f);
		}
		return out;
	}

	// the argument's literal form: a number under any casts, `nullptr` where Hex-Rays types the number a pointer
	static Literal literalOf(const cexpr_t* e)
	{
		bool cast = false;
		while (e->op == cot_cast)
		{
			e = e->x;
			cast = true;
		}
		if (e->op != cot_num || e->numval() != 0)
			return Literal::none;
		return e->type.is_ptr() && !cast ? Literal::null : Literal::zero;
	}

	// the argument's type as it prints: its src type, the address of an object of one a pointer to it (`&aGlobal`,
	// a `const char` global, is a `const char*`; `&p->m` of a pointer to const a pointer to const)
	std::string printedType(const cexpr_t* e) const
	{
		if (implicitCast(e))
			return printedType(e->x);
		if (e->op == cot_ref)
			if (std::string object = srcType(m_function, m_context, e->x); !object.empty())
			{
				bool constant = srcConstObject(m_function, m_context, e->x) && !m_types.constPointee("ptr(" + object + ")");
				return constant ? "ptr(const(" + object + "))" : "ptr(" + object + ")";
			}
		return srcType(m_function, m_context, e);
	}

	// Hex-Rays' cast convertValues prints as its operand, C++'s implicit conversion
	bool implicitCast(const cexpr_t* e) const
	{
		auto found = m_function.conversions.find(e);
		return found != m_function.conversions.end() && found->second.kind == Conversion::Kind::implicit &&
			   found->second.replacesCast;
	}

	// The type C++ gives the printed argument: an integer operator's result is its operands' promoted type (`u + 500`
	// of a uint32_t u: unsigned int, whatever Hex-Rays types it), where both are integers or unscoped enums of src's
	// (under Hex-Rays' casts that print as their operand, or as the one value a rewrite prints, foldOutgoingArguments'
	// stored argument).
	std::string evaluated(const cexpr_t* e, const std::string& key) const
	{
		for (;;)
		{
			auto found = m_function.meaning.find(e);
			const Meaning* meaning = found == m_function.meaning.end() ? nullptr : &found->second;
			const Rewrite* rewrite = meaning ? std::get_if<Rewrite>(meaning) : nullptr;
			if (e->op == cot_cast && ((meaning && std::holds_alternative<Implicit>(*meaning)) || implicitCast(e)))
				e = e->x;
			else if (rewrite && rewrite->form == "$0" && rewrite->items.size() == 1 && rewrite->items[0]->is_expr())
				e = static_cast<const cexpr_t*>(rewrite->items[0]);
			else
				break;
		}
		if (!m_function.meaning.count(e) && !m_function.conversions.count(e) && integerOperator(e->op))
		{
			bool unary = e->op == cot_bnot || e->op == cot_neg;
			std::string x = evaluated(e->x, printedType(e->x)), y = unary ? "" : evaluated(e->y, printedType(e->y));
			if (std::string result = promotedResult(m_context, e->op, x, y); !result.empty())
				return result;
		}
		return key;
	}

	// what a pointer argument for a reference parameter refers to (Hex-Rays passes the object's address; a src
	// reference's value is its referent)
	std::string referred(const std::string& key) const
	{
		SrcTypes::Parts p = m_types.parts(m_types.bare(key));
		return p.kind == "ptr" || p.kind == "lref" ? p.of : key;
	}

	// the type of the value C++ passes for a printed argument of the key: a reference's referent, an array's pointer
	// to its first element (both exact matches in overload resolution), typedefs resolved at the top
	std::string passed(const std::string& key) const
	{
		std::string bare = m_types.bare(key);
		SrcTypes::Parts p = m_types.parts(bare);
		if (p.kind == "lref")
			return passed(p.of);
		bool constant = p.kind == "const";
		SrcTypes::Parts array = constant ? m_types.parts(m_types.bare(p.of)) : p;
		if (array.kind == "array")
			return "ptr(" + (constant ? "const(" + array.of + ")" : array.of) + ")";
		return bare;
	}

	// the printed argument is the parameter's type: the same type with the same const pointee (`char*` is no
	// `const char*`, which C++ ranks apart); a reference parameter's object of its referent, or of its referent
	// without const (binding `const T&` to a T lvalue is the identity conversion, an exact match; which of two such
	// bindings is better is referenceTie's) (an `int` literal is an `int`; `nullptr` is no parameter's type)
	bool exact(const std::string& printed, const std::string& declared, Literal literal) const
	{
		if (printed.empty() || literal == Literal::null)
			return false;
		std::string to = m_types.bare(declared), from = passed(printed);
		if (m_types.parts(to).kind == "lref")
		{
			std::string of = m_types.bare(m_types.parts(to).of);
			return m_types.canonical(of) == m_types.canonical(from) && (constant(of) || !constant(from));
		}
		return m_types.canonical(to) == m_types.canonical(from) &&
			   m_types.constPointee(to) == m_types.constPointee(from);
	}

	// Two exact reference bindings of one argument ([over.ics.rank] 3.2.6): the one to the referent with less const
	// is better. 1 where theirs is (`T&` beside our `const T&`), -1 where ours is, 0 for neither.
	int referenceTie(const std::string& theirs, const std::string& ours) const
	{
		std::string a = m_types.bare(theirs), b = m_types.bare(ours);
		if (m_types.parts(a).kind != "lref" || m_types.parts(b).kind != "lref")
			return 0;
		bool constA = constant(m_types.bare(m_types.parts(a).of)), constB = constant(m_types.bare(m_types.parts(b).of));
		return constA == constB ? 0 : constB ? 1 : -1;
	}

	// the key, typedefs resolved at the top, is const
	bool constant(const std::string& key) const { return m_types.parts(key).kind == "const"; }

	// the two parameter lists are of the same types
	bool sameParameters(const std::vector<std::string>& a, const std::vector<std::string>& b) const
	{
		if (a.size() != b.size())
			return false;
		for (size_t k = 0; k < a.size(); ++k)
			if (m_types.canonical(a[k]) != m_types.canonical(b[k]))
				return false;
		return true;
	}

	// C++ has no implicit conversion of the printed argument to the parameter, so an overload taking it there is not
	// viable. Only what is certain: a pointer for an arithmetic type or an enum (but bool), an arithmetic value or
	// enum (not the literal 0) for a pointer, a pointer for one whose pointee it is not, does not derive from and
	// that drops its const (no void* for a function pointer), a scoped enum or `nullptr` for an arithmetic type,
	// anything but the enum itself for an enum; an object of a class with no conversion function for any of these;
	// for a record or a reference to one, what mayInitialise() rules out. A reference to another type or an unknown
	// type may convert (binding): false.
	bool cannotConvert(const std::string& from, Literal literal, const std::string& to) const
	{
		if (from.empty() && literal != Literal::null)
			return false;
		std::string toBare = m_types.bare(to);
		SrcTypes::Parts q = m_types.parts(m_types.canonical(toBare));
		if (q.kind == "rref" || q.kind == "typedef")
			return false;
		// the value C++ passes: a reference's referent, an array's pointer
		std::string value = from.empty() ? from : passed(from);
		SrcTypes::Parts p = literal == Literal::null ? SrcTypes::Parts{"nullptr", "", ""}
													 : m_types.parts(m_types.canonical(value));
		if (q.kind == "lref" || q.kind == "record")
		{
			SrcTypes::Parts referent = q.kind == "lref" ? m_types.parts(q.of) : q;
			if (referent.kind != "record")
				return false;
			bool mutableReference = q.kind == "lref" && !constant(m_types.bare(m_types.parts(toBare).of));
			return !mayInitialise(p, referent.name, mutableReference);
		}
		if (p.kind == "record")
			return !convertsByFunction(p.name);
		if (p.kind == "rref" || p.kind == "typedef")
			return false;
		bool pointer = p.kind == "ptr" || p.kind == "fn";
		if (q.kind == "ptr")
		{
			if (literal != Literal::none)
				return false;
			if (!pointer)
				return !p.kind.empty();
			std::string pointee = p.kind == "fn" ? m_types.canonical(value) : p.of;
			bool function = m_types.parts(pointee).kind == "fn";
			if (m_types.constPointee(value) && !m_types.constPointee(toBare))
				return true;
			if (q.of == "builtin:void")
				return function;
			if (pointee == q.of)
				return false;
			SrcTypes::Parts a = m_types.parts(pointee), b = m_types.parts(q.of);
			return !(a.kind == "record" && b.kind == "record" && derives(a.name, b.name));
		}
		if (q.kind == "enum")
			return !(p.kind == "enum" && p.name == q.name);
		const SrcModel::Type* builtin = q.kind == "builtin" ? m_model.type(m_types.canonical(toBare)) : nullptr;
		if (!builtin || m_types.canonical(toBare) == "builtin:void")
			return false;
		const SrcModel::Enum* enumeration = p.kind == "enum" ? m_model.enumeration(p.name) : nullptr;
		if (m_types.canonical(toBare) == "builtin:bool")
			return p.kind == "nullptr" || (enumeration && enumeration->scoped);
		return pointer || p.kind == "nullptr" || (enumeration && enumeration->scoped);
	}

	// the record is base or derives from it, at any offset
	bool derives(const std::string& record, const std::string& base) const
	{
		if (record == base)
			return true;
		const SrcModel::Record* r = m_model.record(record);
		if (!r)
			return false;
		for (const SrcModel::Base& b : r->bases)
			if (derives(b.record, base))
				return true;
		return false;
	}

	// The argument (its type's parts) may initialise a parameter of the record or a reference to it: an object of the
	// record or of a class deriving from it, an object of a class with a conversion function, or, for a by-value or
	// const reference parameter, whatever a converting constructor of the record may take. An unknown type may.
	bool mayInitialise(const SrcTypes::Parts& p, const std::string& record, bool mutableReference) const
	{
		if (p.kind.empty() || p.kind == "typedef" || p.kind == "rref")
			return true;
		if (p.kind == "record" && (derives(p.name, record) || convertsByFunction(p.name)))
			return true;
		return !mutableReference && convertingConstructor(record);
	}

	// the record or a base declares a conversion function (srcdb lists no template of one, and src has none); an
	// unknown record may
	bool convertsByFunction(const std::string& record) const
	{
		const SrcModel::Record* r = m_model.record(record);
		if (!r)
			return true;
		if (!m_model.conversionsOf(record).empty())
			return true;
		for (const SrcModel::Base& b : r->bases)
			if (convertsByFunction(b.record))
				return true;
		return false;
	}

	// the record has a constructor callable with one argument that is no copy or move constructor (it may take any
	// value: its parameter's type is not judged); a class template's instance may have constructor templates srcdb
	// does not list, and an unknown record may have anything
	bool convertingConstructor(const std::string& record) const
	{
		const SrcModel::Record* r = m_model.record(record);
		if (!r || !r->pattern.empty())
			return true;
		for (const SrcModel::Function* f : m_model.structorsOf(record))
		{
			std::optional<SrcTypes::Function> type = SrcTypes::function(f->type);
			if (f->structor != "ctor" || !type || type->params.empty())
				continue;
			size_t required = f->required >= 0 ? static_cast<size_t>(f->required) : type->params.size();
			SrcTypes::Parts first = m_types.parts(m_types.canonical(type->params[0]));
			bool copy = (first.kind == "lref" || first.kind == "rref") && first.of == "record:" + record;
			if (required <= 1 && !copy)
				return true;
		}
		return false;
	}

	Function& m_function;
	Context& m_context;
	const SrcModel& m_model;
	SrcTypes& m_types;
};

}  // namespace

void pinOverloads(Function& function, Context& context)
{
	Overloads overloads(function, context);
	std::vector<std::pair<const cexpr_t*, std::string>> unpinned;
	std::set<const cexpr_t*> constructs;
	for (const Local& local : function.locals)
		if (local.inlined && local.inlined->construct)
			constructs.insert(local.inlined->construct);
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
		auto found = e->op == cot_call ? function.meaning.find(e) : function.meaning.end();
		if (found == function.meaning.end())
			return;
		if (Call* c = std::get_if<Call>(&found->second))
			if (std::string why = overloads.pin(e, *c); !why.empty())
			{
				if (constructs.count(e) || c->resultArgument >= 0)
					c->unpinned = why;
				else
					unpinned.emplace_back(e, why);
			}
	});
	for (const auto& [call, why] : unpinned)
		function.meaning[call] = Missing{why};
	// a number pinned as its parameter's literal: its src type key (the printer's suffix)
	for (const auto& [number, key] : overloads.literals)
		function.meaning[number] = key;
}

}  // namespace idakit
