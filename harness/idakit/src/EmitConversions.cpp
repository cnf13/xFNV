// Conversions: every place where the printed C++ types disagree and C++ does not convert by itself gets the explicit
// conversion C++ needs (Emit.hpp's Conversion), decided in one place (decide) from the value's printed type and the
// target's. The places: an assignment's left side (a declaration's too), a resolved call's parameter and the object a
// member is called on, a raw call's formal, the return type, the other side of a comparison, an enum stepped by ++ or
// --, a Hex-Rays cast C++ does not make as printed (or makes only across const), the pointer an access reads through
// (IDA's type, where src's pointee is another), and the pointer to a const object that a write or a base subobject's
// reference goes through.
//
// A void* converted to another object pointer at an assignment, argument or return is the static_cast src writes
// (C converts it implicitly); so is a same-size integer conversion both types prove the original's (proof), and one
// of an integer only IDA or Hex-Rays types to a src declaration's integer of its size is C++'s implicit conversion
// (implicitInteger); every other conversion is a marked spot. Where Hex-Rays casts an integer to a pointer
// that goes on as an integer (or back), its cast is replaced by the conversion of its operand. Runs last: the printed
// types are final once every other pass has decided.

#include "EmitPasses.hpp"

#include "IdaSupport.hpp"

#include <Facts.hpp>

#include <algorithm>
#include <map>
#include <regex>
#include <set>

namespace idakit
{

namespace
{

using Kind = Conversion::Kind;

// a type as printed: its src key, else IDA's type printed raw (key empty)
struct Printed
{
	std::string key;
	tinfo_t ida;
};

enum class Category
{
	other,
	pointer,
	integer,
	boolean,
	floating,
	enumeration,
	record,
	vector,	 // an SSE vector (`__m128`)
	opaque	 // IDA's struct or union (or an integer wider than 8 bytes) src has no key for: gen::Blob<N> (gen::Wide<N>)
};

// how far a conversion between integers of one size and two signednesses is proved the original source's own
// (Conversions::proof): none where the conversion is no such one; else proved, or which side's type is no evidence
// (value, destination) or neither
enum class Proof
{
	none,
	proved,
	value,
	destination,
	neither
};

// what a declaration's type is taken as evidence of: an integer's signedness (Conversions::proof's two sides), or
// the class of an object a member is read in (provenObject)
enum class Evidence
{
	signedness,
	object
};

// a value C++ converts to another type only by copying its bytes
bool bytes(Category c)
{
	return c == Category::record || c == Category::vector || c == Category::opaque;
}

// a conversion that makes a value of another type from the value's bytes (gen::bitcast, std::bit_cast, a vector's
// low lane)
bool copiesBytes(Conversion::Kind k)
{
	return k == Conversion::Kind::bits || k == Conversion::Kind::sameBits || k == Conversion::Kind::lowLane;
}

bool comparison(ctype_t op)
{
	return op == cot_eq || op == cot_ne || (op >= cot_sge && op <= cot_ult);
}

bool step(ctype_t op)
{
	return op == cot_preinc || op == cot_predec || op == cot_postinc || op == cot_postdec;
}

// an operation Hex-Rays has as signed or unsigned (its ctree op), which C++ decides from its operands' types
bool signedOperation(ctype_t op)
{
	switch (op)
	{
	case cot_sdiv: case cot_smod: case cot_sshr: case cot_sge: case cot_sle: case cot_sgt: case cot_slt:
	case cot_asgsdiv: case cot_asgsmod: case cot_asgsshr:
		return true;
	default:
		return false;
	}
}

bool unsignedOperation(ctype_t op)
{
	switch (op)
	{
	case cot_udiv: case cot_umod: case cot_ushr: case cot_uge: case cot_ule: case cot_ugt: case cot_ult:
	case cot_asgudiv: case cot_asgumod: case cot_asgushr:
		return true;
	default:
		return false;
	}
}

class Conversions
{
public:
	Conversions(Function& function, Context& context)
		: m_function(function), m_context(context), m_types(context.types), m_model(context.model),
		  m_parents(parentsOf(function)), m_skipped(function.copies())
	{
		const SrcModel::Type* self = function.self ? m_model.type(function.self->type) : nullptr;
		m_constThis = self && self->isConst;
		std::set<const cinsn_t*> dropped = function.dropped();
		m_skipped.insert(dropped.begin(), dropped.end());
	}

	void run() { statement(&m_function.cfunc->body); }

private:
	// --- the walk: the printed statements, each expression after the expressions inside it

	void statement(const cinsn_t* s)
	{
		if (m_skipped.count(s) || m_function.omitted(s))
			return;
		// an idiom's form prints its items
		if (const Rewrite* rewrite = meaningOf<Rewrite>(s))
		{
			for (const citem_t* item : rewrite->items)
				if (item->is_expr())
					expression(static_cast<const cexpr_t*>(item));
				else
					statement(static_cast<const cinsn_t*>(item));
			return;
		}
		for (const cexpr_t* e : ownExpressions(s))
		{
			bool unused = s->op == cit_expr || (s->op == cit_for && (e == &s->cfor->init || e == &s->cfor->step));
			expression(e, unused);
		}
		if (s->op == cit_return)
			returned(s);
		// a statement's form stands for the next ones of its block too
		std::vector<const cinsn_t*> children = childStatements(s);
		for (size_t k = 0; k < children.size(); ++k)
		{
			statement(children[k]);
			if (const Rewrite* rewrite = meaningOf<Rewrite>(children[k]); rewrite && rewrite->count > 1)
				k += rewrite->count - 1;
		}
	}

	// unused: e's value is not used (an expression statement, a for's init or step, a comma's left side)
	void expression(const cexpr_t* e, bool unused = false)
	{
		// an idiom's form prints its items; a write's (Fields' lane write `p->m_quad[0] = std::bit_cast<float>(...)`)
		// through the pointer to const its lvalue goes through, without the const, where it prints that pointer
		if (const Rewrite* rewrite = meaningOf<Rewrite>(e))
		{
			if (written(e))
				writeThrough(e->x);
			for (const citem_t* item : rewrite->items)
				if (item->is_expr())
					expression(static_cast<const cexpr_t*>(item));
			return;
		}
		if (const Access* access = e->op == cot_call ? meaningOf<Access>(e) : nullptr)
		{
			// an accessor's access prints its receiver (without Hex-Rays' casts) and a setter's value
			expression(receiverOf(e));
			if (access->value)
				expression(access->value);
		}
		else if (e->op == cot_call)
		{
			// a resolved call through the vtable prints as `obj->F(...)`: its callee (the slot read, Hex-Rays' casts
			// of it) is not printed
			bool resolved = meaningOf<Call>(e) || meaningOf<Rewrite>(e);
			if (!(resolved && e->x->op != cot_obj && !m_function.meaning.count(e->x)))
				expression(e->x);
			for (const carg_t& argument : *e->a)
				expression(&argument);
		}
		else
		{
			if (op_uses_x(e->op) && e->x)
				expression(e->x, e->op == cot_comma);
			if (op_uses_y(e->op) && e->y)
				expression(e->y, e->op == cot_comma && unused);
			if (op_uses_z(e->op) && e->z)
				expression(e->z);
		}
		if (e->op == cot_asg)
			assigned(e);
		else if (is_assignment(e->op))
			compound(e);
		else if (e->op == cot_call)
			call(e);
		else if (e->op == cot_cast)
			cast(e);
		// a difference of pointers (an array decaying to one) needs one pointee type, as a comparison does
		else if (comparison(e->op) || (e->op == cot_sub && (e->x->type.is_ptr() || e->x->type.is_array()) &&
									   (e->y->type.is_ptr() || e->y->type.is_array())))
			compared(e);
		else if (step(e->op))
			stepped(e, unused);
		else if (e->op == cot_ptr && !m_function.meaning.count(e))
			access(e->x, nullptr);
		else if (e->op == cot_idx && e->x->type.is_ptr())
			access(e->x, nullptr);
		else if (e->op == cot_memptr || e->op == cot_memref)
			member(e);
		if (e->op == cot_add || e->op == cot_sub || e->op == cot_asgadd || e->op == cot_asgsub || step(e->op))
			arithmetic(e);
		if (e->op == cot_asgadd || e->op == cot_asgsub || step(e->op))
			byteStep(e, unused);
		if (signedOperation(e->op) || unsignedOperation(e->op))
			signedness(e);
	}

	// --- the sites

	void assigned(const cexpr_t* e)
	{
		// an assignment converted already (an enum's number) or an idiom's
		if (meaningOf<std::string>(e) || meaningOf<Rewrite>(e))
			return;
		writeThrough(e->x);
		Printed to = partTarget(e->x).value_or(printed(e->x));
		// Hex-Rays' cast of the value to an abstract class, which no value is of (cast leaves it, or decides a bits
		// conversion this replaces): its operand is the value, converted where it goes
		const cexpr_t* y = e->y;
		bool abstractCast = y->op == cot_cast && (meaningOf<Missing>(y) || meaningOf<std::string>(y)) &&
							abstractRecord(idaType(y->type));
		const cexpr_t* source = abstractCast ? y->x : y;
		// an object of an abstract class has no value of its class to be assigned: the value's bytes stored into it
		if (abstractRecord(to))
			if (std::optional<Conversion> d = decide(printed(source), to, source); d && d->kind == Kind::bits)
			{
				Conversion c = make(Kind::storeBits, to, printed(source), "stores the bytes of");
				c.replacesCast = abstractCast;
				record(e, c);
				return;
			}
		if (abstractCast)
		{
			std::optional<Conversion> d = decide(printed(source), to, source);
			replace(y, d ? *d : make(Kind::staticCast, to, printed(source), "converts"));
			return;
		}
		value(e->y, to, true);
	}

	bool abstractRecord(const Printed& p) const
	{
		if (category(p) != Category::record)
			return false;
		const SrcModel::Record* r = m_model.record(m_types.parts(m_types.canonical(p.key)).name);
		return r && r->abstract;
	}

	// A part helper written (`LODWORD(x) = v`) prints as GenSupport's `_ref` form, a reference to the unsigned
	// integer of the part's width (a signed form's cast to the signed one), whatever type Hex-Rays gives the call (the
	// value's, `NiBoundingVolume` for a 4-byte record): the value goes there. nullopt for any other target.
	std::optional<Printed> partTarget(const cexpr_t* x) const
	{
		if (x->op != cot_call || x->x->op != cot_helper || !isPartHelper(x->x->helper))
			return std::nullopt;
		std::string_view name = x->x->helper;
		bool isSigned = name.starts_with('S');
		std::string unit = name.ends_with("DWORD") ? "int" : name.find("BYTE") != name.npos ? "char" : "short";
		std::string key = !isSigned ? "builtin:unsigned " + unit : unit == "char" ? "builtin:signed char" : "builtin:" + unit;
		return Printed{key, tinfo_t()};
	}

	// a compound assignment of a number (`x |= 4`) to a record or an enum, which C++ has no such operator for: to the
	// object's bytes as an unsigned integer of its size
	void compound(const cexpr_t* e)
	{
		writeThrough(e->x);
		Printed to = printed(e->x);
		Category c = category(to), y = category(printed(e->y));
		if ((c != Category::record && c != Category::enumeration) ||
			(y != Category::integer && y != Category::enumeration && y != Category::boolean))
			return;
		static const std::map<int64_t, std::string> kUnsigned = {{1, "builtin:unsigned char"},
																 {2, "builtin:unsigned short"},
																 {4, "builtin:unsigned int"},
																 {8, "builtin:unsigned long long"}};
		auto integer = kUnsigned.find(sizeOf(to));
		if (integer != kUnsigned.end())
			record(e->x, make(Kind::reinterpret, {"lref(" + integer->second + ")", {}}, to, "operates on the bytes of"));
	}

	void returned(const cinsn_t* s)
	{
		if (meaningOf<std::string>(s))
			return;
		tinfo_t type;
		m_function.cfunc->get_func_type(&type);
		// a reference returned where the exe returns null (`return *0;`, which prints through the pointer): the null
		// pointer of the referent's type, marked (C++ binds no reference to it)
		if (m_function.returnsReference && zero(&s->creturn->expr))
		{
			Printed to{"ptr(" + m_types.parts(m_types.bare(m_function.returnType)).of + ")", type.get_rettype()};
			record(&s->creturn->expr, make(Kind::staticCast, to, printed(&s->creturn->expr), "returns a null "
																							   "reference: converts"));
			return;
		}
		Destination to = destinationOf(m_function, m_context, m_parents, &s->creturn->expr);
		if (to.type.empty())
			return;
		value(&s->creturn->expr, {to.type, type.get_rettype()}, true);
	}

	void call(const cexpr_t* e)
	{
		if (const Access* access = meaningOf<Access>(e))
		{
			accessed(e, *access);
			return;
		}
		const carglist_t& args = *e->a;
		const Call* resolved = meaningOf<Call>(e);
		if (!resolved)
		{
			rawCall(e);
			return;
		}
		const SrcModel::Type* type = m_model.type(resolved->function->type);
		if (!type)
			return;
		bool member = !resolved->function->cls.empty() && !resolved->function->isStatic;
		if (member && !args.empty() && resolved->resultArgument != 0)
			object(&args[0], resolved->function->cls, type->isConst);
		vectorArray(e, *resolved);
		for (size_t k = 0; k < args.size(); ++k)
		{
			Destination declared = destinationOf(m_function, m_context, m_parents, &args[k]);
			if (declared.type.empty() || (k < resolved->castTo.size() && !resolved->castTo[k].empty()))
				continue;
			// Hex-Rays passes the pointer where src takes the reference: the pointer converts; a pointer for a
			// reference to a const pointer it converts to by itself binds that pointer (Calls)
			if (declared.byReference && bindsConverted(&args[k], declared.type))
				continue;
			std::string target = declared.byReference ? "ptr(" + m_types.parts(m_types.bare(declared.type)).of + ")"
													  : declared.type;
			value(&args[k], {target, args[k].formal_type}, true);
			// a same-size integer or enum of another type than the parameter (Calls' mismatch), which C++ converts by
			// itself: its static_cast, marked with the mismatch as the reason (user), and what the proof lacks where
			// the two differ in signedness; unmarked where both types prove the conversion the original source's
			// (user, 2026-11-11: an explicit static_cast, never C++'s implicit conversion); C++'s implicit conversion,
			// unmarked, of an integer only IDA or Hex-Rays types to the parameter's integer of its size (user,
			// 2026-11-12). None where the argument prints as the parameter's own type already (a src field Hex-Rays
			// reads as IDA's int, `pForm->iFormID` for a uint32_t): the mismatch is Hex-Rays' type, not the value's
			if (k < resolved->mismatch.size() && !resolved->mismatch[k].empty() &&
				!m_function.conversions.count(&args[k]) &&
				m_types.canonical(printed(&args[k]).key) != m_types.canonical(target))
			{
				Proof proven = proof(&args[k], target, &args[k]);
				if (proven != Proof::proved && implicitInteger(&args[k], target))
					continue;
				Conversion c = make(Kind::staticCast, {target, args[k].formal_type}, printed(&args[k]), "");
				switch (proven)
				{
				case Proof::proved:
					c.what.clear();
					break;
				case Proof::value:
					c.what = resolved->mismatch[k] + "; unproved: the argument's type is no evidence";
					break;
				case Proof::destination:
					c.what = resolved->mismatch[k] + "; unproved: the parameter's type is no evidence";
					break;
				case Proof::neither:
					c.what = resolved->mismatch[k] + "; unproved: neither type is evidence";
					break;
				case Proof::none:
					c.what = resolved->mismatch[k];
					break;
				}
				record(&args[k], c);
			}
		}
	}

	// An eh vector helper's array (ehvec.h's apArray, astSize, aiCount: the call's first three arguments, the two
	// numbers) that Hex-Rays passes as the pointer to a record whose elements are not astSize bytes, and whose member
	// at offset 0 (through first members) is an array of aiCount elements of astSize bytes: that array's first
	// element's address, `&pThis->mean[0]` (a FanRaceDistS* for its `FR2MatrixVTC<float> mean[2]`), the same address
	// on any layout
	void vectorArray(const cexpr_t* e, const Call& resolved)
	{
		const carglist_t& args = *e->a;
		if (resolved.function->file != "src/SDK/MSVC/ehvec.h" || args.size() < 3 || args[1].op != cot_num ||
			args[2].op != cot_num || m_function.conversions.count(&args[0]))
			return;
		Printed array = printed(&args[0]);
		SrcTypes::Parts p = m_types.parts(m_types.canonical(array.key));
		SrcTypes::Parts pointee = p.kind == "ptr" ? m_types.parts(p.of) : SrcTypes::Parts{};
		const SrcModel::Record* r = pointee.kind == "record" ? m_model.record(pointee.name) : nullptr;
		int64_t size = static_cast<int64_t>(args[1].numval()), count = static_cast<int64_t>(args[2].numval());
		if (!r || r->size == size || m_types.constPointee(array.key))
			return;
		if (std::optional<std::string> path = arrayAtZero(pointee.name, size, count, 0))
		{
			Conversion c = make(Kind::memberAddress, array, array, "");
			c.what.clear();
			c.form = *path;
			record(&args[0], c);
		}
	}

	// the path from a record to its member at offset 0 (through first members, as memberAtZero) that is an array of
	// count elements of size bytes each (src's sizeof): `m[0]`, its first element; nullopt for none
	std::optional<std::string> arrayAtZero(const std::string& name, int64_t size, int64_t count, int depth) const
	{
		const SrcModel::Record* r = depth < 8 ? m_model.record(name) : nullptr;
		if (!r)
			return std::nullopt;
		for (const SrcModel::Field& field : r->fields)
		{
			if (field.offsetBits != 0 || field.bits || m_types.parts(m_types.bare(field.type)).kind == "const")
				continue;
			SrcTypes::Parts p = m_types.parts(m_types.canonical(field.type));
			if (p.kind == "array" && !field.name.empty() && p.count == count && m_model.sizeOf(p.of) == size)
				return field.name + "[0]";
			if (p.kind == "record")
				if (std::optional<std::string> inner = arrayAtZero(p.name, size, count, depth + 1))
					return field.name.empty() ? *inner : field.name + "." + *inner;
		}
		return std::nullopt;
	}

	// An accessor's access (FoldedAccessors' Access) as the member access it prints as: the receiver a field is read
	// through, as a member's pointer (access: to the field's record or a class deriving from it), the receiver a
	// setter's store writes through, without its const, and a setter's value assigned to the field. A field's address
	// through a const object prints as a pointer to const (printed), and `return this` as its receiver: values judged
	// where they go.
	void accessed(const cexpr_t* call, const Access& accessor)
	{
		if (!accessor.field)
			return;
		bool writes = accessor.value != nullptr;
		const cexpr_t* receiver = receiverOf(call);
		if (receiver->op == cot_ref)
		{
			// a member of the object: through its base subobjects to the object's pointer
			if (writes)
				writeThrough(receiver->x);
		}
		else
		{
			access(receiver, &accessor.owner);
			Printed from = printed(receiver);
			if (writes && !from.key.empty() && category(from) == Category::pointer && m_types.constPointee(from.key))
				record(receiver, make(Kind::constCast, {m_types.canonical(from.key), {}}, from, "writes through"));
		}
		if (accessor.value)
			value(accessor.value, {accessor.field->type, accessor.value->type}, true);
	}

	// the object argument without Hex-Rays' casts (as the printer prints an access's receiver)
	static const cexpr_t* receiverOf(const cexpr_t* call)
	{
		const cexpr_t* e = &(*call->a)[0];
		while (e->op == cot_cast)
			e = e->x;
		return e;
	}

	// the argument is the address of a pointer that C++ converts by itself to the const pointer the declared reference
	// refers to (`void* const&` for a `T*`, a base's at offset 0): the reference binds the converted pointer
	bool bindsConverted(const cexpr_t* argument, const std::string& declared) const
	{
		SrcTypes::Parts referent = m_types.parts(m_types.bare(m_types.parts(m_types.bare(declared)).of));
		if (referent.kind != "const" || m_types.parts(m_types.canonical(referent.of)).kind != "ptr")
			return false;
		Printed from = printed(argument);
		SrcTypes::Parts address = m_types.parts(m_types.bare(from.key));
		if (from.key.empty() || address.kind != "ptr" || m_types.parts(m_types.canonical(address.of)).kind != "ptr")
			return false;
		Printed object{address.of, from.ida.is_ptr() ? from.ida.get_pointed_object() : tinfo_t()};
		return !pointers(object, {m_types.bare(referent.of), tinfo_t()});
	}

	// the arguments of a call the printer prints raw: those whose printed type does not convert to the callee's formal
	// (a C-style cast of one cannot make a record's bytes another type's). A part helper's argument (`LOBYTE(p)`) is
	// none: GenSupport's part functions take the object as it is, the bytes of a pointer to const too, and their
	// `_ref` forms bind it in place, which a conversion's value cannot be.
	void rawCall(const cexpr_t* e)
	{
		if (e->x->op == cot_helper && isPartHelper(e->x->helper))
			return;
		func_type_data_t params;
		tinfo_t function = e->x->type.is_ptr() ? e->x->type.get_pointed_object() : e->x->type;
		if (!function.get_func_details(&params))
			params.clear();
		// a direct call of a function's start prints as fnvd::original's template, which takes each argument's type
		// as it is (a cast pointer's call takes its parameters')
		func_t* start = e->x->op == cot_obj ? get_func(e->x->obj_ea) : nullptr;
		bool original = start && start->start_ea == e->x->obj_ea;
		for (size_t k = 0; k < e->a->size(); ++k)
		{
			const carg_t& arg = (*e->a)[k];
			if (std::optional<Conversion> copied = original ? copiedRecord(&arg) : std::nullopt)
			{
				record(&arg, *copied);
				continue;
			}
			const tinfo_t& formal = k < params.size() ? params[k].type : arg.formal_type;
			if (formal.empty())
				continue;
			if (std::optional<Conversion> unconst = constObjectBase(&arg, formal))
			{
				record(&arg, *unconst);
				continue;
			}
			std::optional<Conversion> d = decide(printed(&arg), idaType(formal), &arg);
			// the printer casts it C-style to the formal where IDA's types differ: that makes all but a bit copy
			if (d && (copiesBytes(d->kind) || text(formal) == text(arg.type)))
				record(&arg, *d);
		}
	}

	// Implicit's `&p->Base` (printed p, judged for the resolved call a later layer left raw) of a const object, for a
	// raw formal to a non-const base: C++ drops no const by itself. p's const dropped, marked; the conversion to the
	// base is then C++'s own, adjusting the pointer as the exe's `lea` does. nullopt for any other argument.
	std::optional<Conversion> constObjectBase(const cexpr_t* arg, const tinfo_t& formal) const
	{
		auto base = [&](const cexpr_t* e) {
			return (e->op == cot_memptr || e->op == cot_memref) && meaningOf<std::string>(e);
		};
		if (arg->op != cot_ref || !meaningOf<Implicit>(arg) || !base(arg->x) || !formal.is_ptr() ||
			formal.get_pointed_object().is_const())
			return std::nullopt;
		const cexpr_t* b = arg->x;
		while (base(b->x))
			b = b->x;
		if (b->op != cot_memptr)
			return std::nullopt;
		Printed object = printed(b->x);
		SrcTypes::Parts pointer = m_types.parts(m_types.bare(object.key));
		SrcTypes::Parts pointee = pointer.kind == "ptr" ? m_types.parts(m_types.bare(pointer.of)) : SrcTypes::Parts{};
		if (pointee.kind != "const")
			return std::nullopt;
		return make(Kind::constCast, {"ptr(" + pointee.of + ")", {}}, object, "drops const of");
	}

	// A src record passed by value to a raw call (fnvd::original's template takes it by value) that is not trivially
	// copyable in src: C++ would copy it through its copy constructor and destroy the copy, where the exe passes its
	// bytes. Its bytes instead, as many as src's record has: gen::Blob<N>, marked; nullopt for any other argument.
	std::optional<Conversion> copiedRecord(const cexpr_t* arg) const
	{
		Printed from = printed(arg);
		if (from.key.empty() || category(from) != Category::record)
			return std::nullopt;
		std::string name = m_types.parts(m_types.canonical(from.key)).name;
		const SrcModel::Record* r = m_model.record(name);
		if (!r || r->size <= 0 || triviallyCopyable(name))
			return std::nullopt;
		Conversion c;
		c.kind = Kind::bits;
		c.ida = byteBlock(static_cast<size_t>(r->size));
		c.what = "passes " + name + ", not trivially copyable in src, by value to a raw call";
		return c;
	}

	// src copies the record as bytes and destroys it with nothing: a byte copy assignment (srcdb's, which a vptr or a
	// user-declared one in it, a base or a member rules out) and no destructor with a body in it, a base or a member
	bool triviallyCopyable(const std::string& name) const
	{
		const SrcModel::Record* r = m_model.record(name);
		if (!r)
			return true;
		if (!r->trivialCopy)
			return false;
		for (const SrcModel::Function* f : m_model.structorsOf(name))
			if (f->structor == "dtor" && !f->empty)
				return false;
		for (const SrcModel::Base& base : r->bases)
			if (!triviallyCopyable(base.record))
				return false;
		for (const SrcModel::Field& field : r->fields)
		{
			const SrcModel::Type* held = m_model.type(m_model.storage(field.type));
			if (held && held->kind == "record" && held->name != name && !triviallyCopyable(held->name))
				return false;
		}
		return true;
	}

	// an unnamed IDA struct of the size, which no src record is: the printer's gen::Blob<size>
	static tinfo_t byteBlock(size_t size)
	{
		udt_type_data_t udt;
		udm_t bytes;
		bytes.name = "bytes";
		bytes.type.create_array(tinfo_t(BT_INT8 | BTMT_USIGNED), static_cast<uint32_t>(size));
		bytes.offset = 0;
		bytes.size = size * 8;
		udt.push_back(bytes);
		udt.total_size = size;
		tinfo_t t;
		t.create_udt(udt, BTF_STRUCT);
		return t;
	}

	// the object a member is called on: Hex-Rays' pointer to it, to the class (a base of it, or itself) without a
	// const the callee does not take; a member of an object reached through a pointer to const that a non-const
	// member is called on (`v72->label.assign(...)`) through that pointer without its const
	void object(const cexpr_t* pointer, const std::string& cls, bool constCallee)
	{
		if (pointer->op == cot_ref && !constCallee && objectConst(pointer->x))
			writeThrough(pointer->x);
		Printed from = printed(pointer);
		if (from.key.empty() || category(from) != Category::pointer)
			return;
		std::string canonical = m_types.canonical(from.key);
		SrcTypes::Parts pointee = m_types.parts(m_types.parts(canonical).of);
		bool drops = m_types.constPointee(from.key) && !constCallee;
		if (pointee.kind == "record" && (pointee.name == cls || isBase(pointee.name, cls)))
		{
			if (drops)
				record(pointer, make(Kind::constCast, {canonical, {}}, from, "calls non-const " + cls + " member on"));
			return;
		}
		// a `void*` parameter Calls casts to the class (its castTo): C++'s static_cast from void*
		if (canonical == "ptr(builtin:void)")
			return;
		Conversion c = make(Kind::reinterpret, {"ptr(record:" + cls + ")", {}}, from, "calls a " + cls + " member on");
		if (m_types.constPointee(from.key))
			c.unconst = canonical;
		record(pointer, c);
	}

	// a Hex-Rays cast C++ does not make as printed: a static_cast (src's key) where only another cast converts, a
	// reinterpret_cast of an exe's bytes (ExeCast) from a pointer to const (to the bytes as const where they are only
	// read through it; else it drops the const), a C-style cast that makes a record of other bytes, or the printer's
	// `reinterpret_cast<uintptr_t>` of what IDA has as a pointer and src does not. A static_cast between same-size
	// integers (or enums, or bool) of another type is printed marked (user: a same-size integer mismatch stays marked,
	// as an argument's does), unless both types prove the conversion the original source's (proof): printed as the
	// cast is; or its operand is an integer only IDA or Hex-Rays types and the cast's value goes to a src
	// declaration's integer of its size (user, 2026-11-12): the operand alone, C++'s implicit conversion
	void cast(const cexpr_t* e)
	{
		Printed from = printed(e->x);
		if (const std::string* key = meaningOf<std::string>(e))
		{
			Printed to{*key, e->type};
			std::optional<Conversion> d = decide(from, to, e->x);
			if (d && (d->kind != Kind::staticCast || !d->unconst.empty()))
				replace(e, *d);
			else if (sameSizeInteger(from, to, e->x) && proof(e->x, to.key, e) != Proof::proved)
			{
				if (implicitInteger(e->x, to.key) && implicitInteger(e->x, srcDestination(e)))
				{
					Conversion c = make(Kind::implicit, from, from, "");
					c.what.clear();
					replace(e, c);
				}
				else
					replace(e, make(Kind::staticCast, to, from, "converts"));
			}
		}
		else if (const ExeCast* bytes = meaningOf<ExeCast>(e))
		{
			if (!from.key.empty() && dropsConst(from.key, bytes->type))
			{
				// read through: the bytes as const, with every const below that from has (`const void* const*`
				// for the `void**` read of a `const uint16_t**`, which reinterpret_cast may not cast away)
				std::string of = m_types.parts(keepConsts(from.key, bytes->type)).of;
				bool read = readThrough(e);
				std::string asConst = m_types.parts(of).kind == "const" ? of : "const(" + of + ")";
				Printed to = read ? Printed{"ptr(" + asConst + ")", e->type} : Printed{bytes->type, e->type};
				Conversion c = make(Kind::reinterpret, to, from, "reads const bytes through");
				if (read)
					c.what.clear();
				else
					c.unconst = m_types.canonical(from.key);
				replace(e, c);
			}
		}
		else if (meaningOf<Missing>(e))
		{
			Printed to = idaType(e->type);
			// no value is of an abstract class: what the cast's value goes to decides (assigned)
			if (abstractRecord(to))
				return;
			std::optional<Conversion> d = decide(from, to, e->x);
			Category f = category(from);
			bool number = f == Category::integer || f == Category::enumeration || f == Category::boolean;
			if (d && copiesBytes(d->kind))
				replace(e, *d);
			// IDA's operand is a pointer (a fold's class-neutral void* result) and src's value a number: an integer
			// conversion as any other, src's static_cast, a same-size one marked (user)
			else if (e->x->type.is_ptr() && !e->type.is_ptr() && number)
			{
				Conversion c = d ? *d : make(Kind::staticCast, to, from, "converts");
				if (!d && (!sameSizeInteger(from, to, e->x) || proof(e->x, to.key, e) == Proof::proved))
					c.what.clear();
				replace(e, c);
			}
		}
	}

	// the value is an argument of a direct call the printer prints raw, through fnvd::original's template, which takes
	// each argument's type as it is
	bool rawArgument(const cexpr_t* value) const
	{
		auto up = value ? m_parents.find(value) : m_parents.end();
		if (up == m_parents.end() || up->second->op != cot_call)
			return false;
		const cexpr_t* call = static_cast<const cexpr_t*>(up->second);
		func_t* start = call->x != value && call->x->op == cot_obj ? get_func(call->x->obj_ea) : nullptr;
		return start && start->start_ea == call->x->obj_ea && !meaningOf<Call>(call) && !meaningOf<Rewrite>(call) &&
			   !meaningOf<Intrinsic>(call);
	}

	// the pointer is only read through: dereferenced (`*p`, `p[i]`, `p->m`), the lvalue it reaches through members
	// and array elements neither written nor addressed
	bool readThrough(const cexpr_t* pointer) const
	{
		auto up = m_parents.find(pointer);
		if (up == m_parents.end() || !up->second->is_expr())
			return false;
		const cexpr_t* lvalue = static_cast<const cexpr_t*>(up->second);
		if ((lvalue->op != cot_ptr && lvalue->op != cot_idx && lvalue->op != cot_memptr) || lvalue->x != pointer)
			return false;
		for (up = m_parents.find(lvalue); up != m_parents.end() && up->second->is_expr(); up = m_parents.find(lvalue))
		{
			const cexpr_t* p = static_cast<const cexpr_t*>(up->second);
			if (p->x != lvalue || (p->op != cot_memref && !(p->op == cot_idx && !p->x->type.is_ptr())))
				break;
			lvalue = p;
		}
		if (up == m_parents.end() || !up->second->is_expr())
			return true;
		const cexpr_t* p = static_cast<const cexpr_t*>(up->second);
		return !((written(p) || p->op == cot_ref) && p->x == lvalue);
	}

	// a comparison of a pointer with an integer, of unrelated pointers (or a difference of them; an array as its first
	// element's pointer), or of a record's bytes with a number: Hex-Rays' side (a cast or number; else the right)
	// converts to the other's type
	void compared(const cexpr_t* e)
	{
		auto hexrays = [&](const cexpr_t* x) {
			return (x->op == cot_cast && !meaningOf<Implicit>(x)) || x->op == cot_num;
		};
		const cexpr_t* side = hexrays(e->y) || !hexrays(e->x) ? e->y : e->x;
		const cexpr_t* other = side == e->y ? e->x : e->y;
		// an array is compared (or subtracted) as the pointer to its first element
		auto decayed = [&](Printed p) {
			SrcTypes::Parts parts = p.key.empty() ? SrcTypes::Parts{} : m_types.parts(m_types.bare(p.key));
			if (parts.kind == "array")
				p.key = "ptr(" + parts.of + ")";
			return p;
		};
		Printed from = decayed(printed(side)), to = decayed(printed(other));
		Category a = category(from), b = category(to);
		auto number = [](Category c) {
			return c == Category::integer || c == Category::enumeration || c == Category::boolean;
		};
		if (a == Category::pointer && b == Category::pointer)
		{
			std::optional<Conversion> d = decide(from, to, side);
			if (d && (d->kind == Kind::reinterpret || d->kind == Kind::memberAddress))
				record(side, *d);
		}
		else if ((a == Category::pointer && number(b)) || (number(a) && b == Category::pointer))
			value(side, to, false);
		else if (bytes(a) != bytes(b))
		{
			// the record's side is the one converted
			if (bytes(b))
				std::swap(side, other), std::swap(from, to);
			std::optional<Conversion> d = decide(from, to, side);
			if (d && copiesBytes(d->kind))
				record(side, *d);
		}
	}

	// ++ or -- of an enum (C++ steps none): `v = static_cast<E>(v + 1)`, where the value is unused or the new one; the
	// old value where it is used (`a[v++]`): `std::exchange(v, static_cast<E>(v + 1))`. It is the exe's step exactly
	// (the next value stored at the enum's width) where src's enum is as wide as what the exe reads and writes
	// (Hex-Rays' type of the operand); otherwise marked.
	void stepped(const cexpr_t* e, bool unused)
	{
		writeThrough(e->x);
		Printed type = printed(e->x);
		bool pre = e->op == cot_preinc || e->op == cot_predec;
		if (category(type) != Category::enumeration)
			return;
		Conversion c = make(unused || pre ? Kind::enumStep : Kind::enumPostStep, type, type, "steps enum");
		if (std::optional<int64_t> size = m_context.model.sizeOf(type.key);
			size && *size == static_cast<int64_t>(e->x->type.get_size()))
			c.what.clear();
		m_function.conversions[e] = c;
	}

	// ++, --, += or -= of a pointer to void, which C++ steps by no size, where the exe steps it by bytes (arithmetic
	// converted none: Hex-Rays' stride is 1 too): the step on the pointer to char of its const, `v =
	// static_cast<char*>(v) + 1`, the old value through std::exchange where it is used (`v++` read), clean
	void byteStep(const cexpr_t* e, bool unused)
	{
		if (m_function.conversions.count(e) || m_function.conversions.count(e->x) || m_function.meaning.count(e))
			return;
		Printed type = printed(e->x);
		if (type.key.empty() || category(type) != Category::pointer)
			return;
		std::optional<PointeeCv> pointee = pointeeOf(m_types, type.key);
		if (!pointee || m_types.canonical(pointee->type) != "builtin:void" || pointee->isVolatile)
			return;
		bool post = (e->op == cot_postinc || e->op == cot_postdec) && !unused;
		Conversion c = make(post ? Kind::bytePostStep : Kind::byteStep,
							{pointee->isConst ? "ptr(const(builtin:char))" : "ptr(builtin:char)", {}}, type, "");
		c.what.clear();
		m_function.conversions[e] = c;
	}

	// Pointer arithmetic (`p + n`, `p - n`, `p += n`, `++p`) moves by the size of what the pointer points to:
	// Hex-Rays' by its type's pointee (an integer's by 1), C++'s by the printed type's (void's by 1: the printer's byte
	// arithmetic). Where the two differ (src's pointer where Hex-Rays has an integer, or a pointee of another size), the
	// operand is converted to Hex-Rays' type, marked: as printed, C++ would move it by other bytes than the exe (`(int)
	// p + 4` for Hex-Rays' `m_item + 4` of an int member src declares a pointer). An lvalue the arithmetic writes is
	// read and written as Hex-Rays' type (`reinterpret_cast<int&>(p) += 4`). A difference of two pointers is
	// compared's.
	void arithmetic(const cexpr_t* e)
	{
		if (m_function.meaning.count(e) || ((e->op == cot_sub || e->op == cot_asgsub) && e->y->type.is_ptr()))
			return;
		bool writes = e->op != cot_add && e->op != cot_sub;
		std::vector<const cexpr_t*> operands = {e->x};
		if (e->op == cot_add)
			operands.push_back(e->y);
		for (const cexpr_t* operand : operands)
		{
			Printed from = printed(operand);
			if (from.key.empty())
				continue;
			std::optional<int64_t> exe = idaStride(operand->type), src = srcStride(from);
			if (!exe || !src || *exe == *src)
				continue;
			Printed to = idaType(operand->type);
			if (writes && operand == e->x)
			{
				if (!to.key.empty())
					record(operand, make(Kind::reinterpret, {"lref(" + to.key + ")", {}}, from,
										 "moves by " + std::to_string(*exe) + " bytes, not " + std::to_string(*src) +
											 ", the bytes of"));
				else
					record(operand, make(Kind::reinterpret, {"lref(" + from.key + ")", {}}, from,
										 "moves by " + std::to_string(*exe) + " bytes, not " + std::to_string(*src) +
											 ": no src type for Hex-Rays' " + text(operand->type) + " of"));
				continue;
			}
			std::optional<Conversion> d = decide(from, to, operand);
			Conversion c = d ? *d : make(Kind::reinterpret, to, from, "");
			c.what = "moves by " + std::to_string(*exe) + " bytes, not " + std::to_string(*src) + ": converts";
			record(operand, c);
		}
	}

	// the bytes one step of Hex-Rays' arithmetic on a value of the type moves by: its pointee's size (void's 1), an
	// array's element's, an integer's 1; nullopt where it is none of those or the size is unknown
	static std::optional<int64_t> idaStride(const tinfo_t& type)
	{
		if (type.is_integral() || type.is_enum() || type.is_bool())
			return 1;
		if (!type.is_ptr() && !type.is_array())
			return std::nullopt;
		tinfo_t of = type.is_ptr() ? type.get_pointed_object() : type.get_array_element();
		if (of.is_void())
			return 1;
		size_t size = of.get_size();
		return size == BADSIZE || size == 0 ? std::nullopt : std::optional<int64_t>(size);
	}

	// the bytes one step of C++'s arithmetic on the printed value moves by, as idaStride
	std::optional<int64_t> srcStride(const Printed& p) const
	{
		Category c = category(p);
		if (c == Category::integer || c == Category::enumeration || c == Category::boolean)
			return 1;
		SrcTypes::Parts parts = m_types.parts(m_types.canonical(p.key));
		if (parts.kind != "ptr" && parts.kind != "array")
			return std::nullopt;
		std::string of = m_types.bare(parts.of);
		if (m_types.parts(of).kind == "const" || m_types.parts(of).kind == "volatile")
			of = m_types.bare(m_types.parts(of).of);
		if (m_types.canonical(of) == "builtin:void")
			return 1;
		std::optional<int64_t> size = m_model.sizeOf(of);
		return size && *size > 0 ? size : std::nullopt;
	}

	// An integer division, remainder, right shift or ordering comparison is signed or unsigned in the exe (Hex-Rays'
	// op: `idiv` or `div`, `sar` or `shr`, `jl` or `jb`), in C++ by its operands' types after the usual arithmetic
	// conversions (a shift: by its left operand's). Where the printed types make C++'s operation the other one (an
	// operand src or resolveTypes types unsigned that Hex-Rays reads signed: `*(_DWORD *)p / 88` of an `idiv`), each
	// operand whose promoted type has the other signedness is converted to the exe's, at its promoted size, marked;
	// an operand the operation writes (`x /= n`) as a reference to it. Not where every operand is non-negative either
	// way (an unsigned type narrower than int, which C++ promotes to int, a bool, a number below 2^31): the two
	// operations agree there. Not where an operand is an enum: its comparisons are the source's own (clang gives an
	// enum without a fixed type and without negative enumerators an unsigned type, MSVC int: src's enum, not the
	// body, would carry the exe's width and signedness).
	void signedness(const cexpr_t* e)
	{
		if (m_function.meaning.count(e))
			return;
		bool exeSigned = signedOperation(e->op);
		bool shift = e->op == cot_sshr || e->op == cot_ushr || e->op == cot_asgsshr || e->op == cot_asgushr;
		Printed px = printed(e->x), py = printed(e->y);
		if (category(px) == Category::enumeration || (!shift && category(py) == Category::enumeration))
			return;
		std::optional<Promoted> x = promoted(px), y = promoted(py);
		if (!x || (!shift && !y))
			return;
		if (nonNegative(e->x, px) && (shift || nonNegative(e->y, py)))
			return;
		bool cppSigned = shift ? x->isSigned
					   : x->size != y->size ? (x->size > y->size ? x->isSigned : y->isSigned)
											: x->isSigned && y->isSigned;
		if (cppSigned == exeSigned)
			return;
		std::string what = std::string("the exe's ") + opName(e->op) + " is " + (exeSigned ? "signed" : "unsigned") +
						   ": converts";
		auto convert = [&](const cexpr_t* operand, const Promoted& p, bool lvalue) {
			if (p.isSigned == exeSigned)
				return;
			std::string key = std::string("builtin:") + (exeSigned ? "" : "unsigned ") +
							  (p.size == 8 ? "long long" : "int");
			if (lvalue)
			{
				if (p.size != sizeOf(printed(operand)))
					return;
				key = "lref(" + key + ")";
			}
			Conversion c = make(lvalue ? Kind::reinterpret : Kind::staticCast, {key, {}}, printed(operand), "");
			c.what = what;
			record(operand, c);
		};
		bool writes = is_assignment(e->op);
		convert(e->x, *x, writes);
		if (!shift)
			convert(e->y, *y, false);
	}

	// an integer operand's type after C++'s integral promotions: its size and signedness
	struct Promoted
	{
		int64_t size = 0;
		bool isSigned = false;
	};
	std::optional<Promoted> promoted(const Printed& p) const
	{
		Category c = category(p);
		if (c == Category::boolean)
			return Promoted{4, true};
		if (c != Category::integer)
			return std::nullopt;
		int64_t size = 0;
		bool isSigned = false;
		if (!p.key.empty())
		{
			const SrcModel::Type* type = m_model.type(m_types.canonical(p.key));
			if (!type || type->size <= 0)
				return std::nullopt;
			size = type->size, isSigned = type->isSigned;
		}
		else
		{
			size_t s = p.ida.get_size();
			if (s == BADSIZE)
				return std::nullopt;
			size = static_cast<int64_t>(s), isSigned = p.ida.is_signed();
		}
		if (size < 4)
			return Promoted{4, true};
		return Promoted{size, isSigned};
	}

	// whether the integer type `to` holds the number's value: its literal converts to it exactly
	bool holdsNumber(const Printed& to, const cexpr_t* number) const
	{
		if (category(to) != Category::integer || to.key.empty())
			return false;
		const SrcModel::Type* type = m_model.type(m_types.canonical(to.key));
		if (!type || type->size <= 0 || type->size > 8)
			return false;
		int bits = static_cast<int>(type->size) * 8;
		uint64_t value = number->numval();
		bool negative = number->type.is_signed() && static_cast<int64_t>(value) < 0;
		if (!type->isSigned)
			return !negative && (bits == 64 || value < (uint64_t{1} << bits));
		if (negative)
			return bits == 64 || static_cast<int64_t>(value) >= -(int64_t{1} << (bits - 1));
		return value < (uint64_t{1} << (bits - 1));
	}

	// a value that is the same number read signed or unsigned at its promoted size: a bool, an unsigned integer
	// narrower than int, a number below 2^31
	bool nonNegative(const cexpr_t* e, const Printed& p) const
	{
		if (e->op == cot_num)
			return e->numval() < 0x80000000u;
		if (category(p) == Category::boolean)
			return true;
		if (category(p) != Category::integer)
			return false;
		if (p.key.empty())
			return p.ida.get_size() < 4 && p.ida.is_unsigned();
		const SrcModel::Type* type = m_model.type(m_types.canonical(p.key));
		return type && type->size > 0 && type->size < 4 && !type->isSigned;
	}

	static const char* opName(ctype_t op)
	{
		switch (op)
		{
		case cot_sdiv: case cot_udiv: case cot_asgsdiv: case cot_asgudiv: return "division";
		case cot_smod: case cot_umod: case cot_asgsmod: case cot_asgumod: return "remainder";
		case cot_sshr: case cot_ushr: case cot_asgsshr: case cot_asgushr: return "right shift";
		default: return "comparison";
		}
	}

	// the pointer an access reads through (`*p`, `p[i]`, `p->f`), where src's pointee is not IDA's: IDA's type (the
	// type of what Hex-Rays reads); owner: a field's record (the pointee converts where it is that record or derives
	// from it)
	void access(const cexpr_t* pointer, const std::string* owner)
	{
		Printed from = printed(pointer);
		Category c = category(from);
		if (c != Category::pointer && c != Category::integer)
			return;
		if (owner && c == Category::pointer && !from.key.empty())
		{
			SrcTypes::Parts pointee = m_types.parts(m_types.parts(m_types.canonical(from.key)).of);
			if (pointee.kind == "record" && (pointee.name == *owner || isBase(pointee.name, *owner)))
				return;
		}
		Printed to = idaType(pointer->type);
		if (owner)
		{
			SrcTypes::Parts pointee = to.key.empty() ? SrcTypes::Parts{}
													 : m_types.parts(m_types.parts(m_types.canonical(to.key)).of);
			if (pointee.kind != "record" || (pointee.name != *owner && !isBase(pointee.name, *owner)))
				to = {"ptr(record:" + *owner + ")", {}};
		}
		std::optional<Conversion> d = decide(from, to, pointer);
		// reading through a pointer to const needs no conversion; a write is writeThrough's
		if (d && d->kind == Kind::constCast)
			return;
		if (d)
		{
			if (!d->what.empty())
				d->what = "reads " + describe(to) + " through " + describe(from);
			record(pointer, *d);
		}
	}

	// a field access: its pointer (access), or a base subobject's reference to a const object (the printer spells a
	// base subobject it prints as a value `static_cast<Base&>(object)`, and calls a member on it through the object):
	// the object's pointer, or the object, without its const; not where a field of it is read (`object.field`), nor
	// where the base is the const one (Const's: `static_cast<const Base&>(object)`)
	void member(const cexpr_t* e)
	{
		if (const Member* m = meaningOf<Member>(e))
		{
			if (e->op == cot_memptr)
				access(e->x, &m->owner);
			return;
		}
		auto base = [&](const cexpr_t* x) {
			return (x->op == cot_memptr || x->op == cot_memref) && meaningOf<std::string>(x) != nullptr;
		};
		if (!base(e) || base(e->x) || !objectConst(e) || m_types.bare(*meaningOf<std::string>(e)).starts_with("const("))
			return;
		const cexpr_t* outer = e;
		for (auto up = m_parents.find(outer); up != m_parents.end() && up->second->is_expr();
			 up = m_parents.find(outer))
		{
			const cexpr_t* p = static_cast<const cexpr_t*>(up->second);
			if (!base(p) || p->x != outer)
				break;
			outer = p;
		}
		if (auto up = m_parents.find(outer); up != m_parents.end() && up->second->is_expr())
		{
			const cexpr_t* p = static_cast<const cexpr_t*>(up->second);
			if ((p->op == cot_memptr || p->op == cot_memref) && p->x == outer && meaningOf<Member>(p))
				return;
		}
		Printed object = printed(e->x);
		if (object.key.empty())
			return;
		std::string canonical = m_types.canonical(object.key);
		if (e->op == cot_memref)
			canonical = "lref(" + canonical + ")";
		record(e->x, make(Kind::constCast, {canonical, {}}, object, "a base subobject of const"));
	}

	// a write to the lvalue: the pointer to const it goes through, without the const
	void writeThrough(const cexpr_t* lvalue)
	{
		const cexpr_t* l = lvalue;
		while (l->op == cot_memref || (l->op == cot_idx && !l->x->type.is_ptr()))
			l = l->x;
		if (l->op != cot_memptr && l->op != cot_ptr && l->op != cot_idx)
			return;
		const cexpr_t* pointer = l->x;
		while ((pointer->op == cot_memptr || pointer->op == cot_memref) && meaningOf<std::string>(pointer))
			pointer = pointer->x;  // through base subobjects to the object's pointer
		Printed from = printed(pointer);
		if (!from.key.empty() && m_types.constPointee(from.key) && category(from) == Category::pointer)
			record(pointer, make(Kind::constCast, {m_types.canonical(from.key), {}}, from, "writes through"));
	}

	// a value going where its printed type does not convert to the target's (unsure: decide only what is sure from
	// both types). A Hex-Rays cast between a pointer and an integer, whose operand goes on as the target's kind, is
	// replaced by the operand's conversion.
	void value(const cexpr_t* v, const Printed& to, bool clean)
	{
		if (v->op == cot_cast && (meaningOf<std::string>(v) || meaningOf<Missing>(v)))
		{
			Category c = category(printed(v)), x = category(printed(v->x)), t = category(to);
			auto number = [](Category k) { return k == Category::integer || k == Category::enumeration; };
			if ((c == Category::pointer && number(x) && number(t)) ||
				(number(c) && x == Category::pointer && t == Category::pointer))
			{
				Printed operand = printed(v->x);
				std::optional<Conversion> d = decide(operand, to, v->x);
				Conversion c = d ? *d : make(Kind::staticCast, to, operand, "Hex-Rays' cast to " + text(v->type));
				// the operand is of the target's type already, or a pointer C++ converts to it by itself at the same
				// address (to void*, an upcast at offset 0): Hex-Rays' cast and back again is no conversion
				if (!d && !operand.key.empty() && !to.key.empty() &&
					(m_types.canonical(operand.key) == m_types.canonical(to.key) ||
					 convertsAtSameAddress(operand.key, to.key)))
					c.what.clear();
				// a number the target's integer type holds: its literal is exact, so Hex-Rays' cast of it to a pointer
				// (the type it read the other side as) and back is no conversion
				if (!d && v->x->op == cot_num && holdsNumber(to, v->x))
					c.what.clear();
				replace(v, c);
				return;
			}
		}
		std::optional<Conversion> d = decide(printed(v), to, v);
		if (!d)
			return;
		// a value cast already (a Hex-Rays cast C++ does not make: `reinterpret_cast<char**>(p)`) to the target's
		// type but for a const below the first pointee (`const char**`), which no cast but const_cast adds: the cast
		// and a const_cast after it, rather than the const_cast alone that record would drop
		if (auto found = m_function.conversions.find(v);
			found != m_function.conversions.end() && d->kind == Kind::constCast && d->what.empty() &&
			(found->second.kind == Kind::reinterpret || found->second.kind == Kind::staticCast) &&
			!to.key.empty() && m_types.canonical(found->second.type) == m_types.canonical(to.key))
		{
			found->second.addConst = to.key;
			return;
		}
		// C converts a void* to another object pointer by itself: src's static_cast
		if (clean && d->kind == Kind::staticCast && d->unconst.empty() && voidPointer(printed(v)))
			d->what.clear();
		record(v, *d);
	}

	// --- the decision

	// the key with its typedefs resolved and its const kept (canonical drops it), as C++ compares function types
	std::string withConst(const std::string& key) const
	{
		if (std::optional<SrcTypes::Function> f = SrcTypes::function(key))
		{
			f->ret = withConst(f->ret);
			for (std::string& param : f->params)
				param = withConst(param);
			return SrcTypes::functionKey(*f);
		}
		SrcTypes::Parts p = m_types.parts(key);
		if (p.kind == "typedef")
			return m_types.bare(key) == key ? key : withConst(m_types.bare(key));
		if (p.kind == "const" || p.kind == "volatile" || p.kind == "ptr" || p.kind == "lref" || p.kind == "rref")
			return p.kind + "(" + withConst(p.of) + ")";
		if (p.kind == "array")
			return "array(" + withConst(p.of) + "," + std::to_string(p.count) + ")";
		return m_types.canonical(key);
	}

	// the conversion from's value needs to become to's type, where C++ makes none by itself; nullopt for none (or
	// none that is sure)
	std::optional<Conversion> decide(const Printed& from, const Printed& to, const cexpr_t* value) const
	{
		// an array is the pointer to its first element
		std::string bare = from.key.empty() ? "" : m_types.bare(from.key);
		bool constArray = m_types.parts(bare).kind == "const";
		std::string array = constArray ? m_types.bare(m_types.parts(bare).of) : bare;
		if (!array.empty() && m_types.parts(array).kind == "array")
		{
			std::string element = m_types.parts(array).of;
			return decide({"ptr(" + (constArray ? "const(" + element + ")" : element) + ")", from.ida}, to, value);
		}
		Category f = category(from), t = category(to);
		if (f == Category::other || t == Category::other)
			return std::nullopt;
		if (!from.key.empty() && !to.key.empty() && m_types.canonical(from.key) == m_types.canonical(to.key))
		{
			// a function pointer converts to no other function type: canonical's dropped consts (a `const T*`
			// parameter) are part of a function's type
			std::string a = pointedFunction(from.key), b = pointedFunction(to.key);
			if (!a.empty() && !b.empty() && a != b)
				return make(Kind::reinterpret, to, from, "reinterprets");
			// a const dropped where the destination's src type has none; a raw call's argument drops none:
			// fnvd::original's template takes the value's own type
			if (dropsConst(from.key, to.key))
				return rawArgument(value) ? std::nullopt
										  : std::optional(make(Kind::constCast, to, from, "drops const of"));
			// a const added below the first pointee (`T**` to `const T**`): const_cast, clean
			if (addsDeepConst(from.key, to.key))
			{
				Conversion c = make(Kind::constCast, to, from, "");
				c.what.clear();
				return c;
			}
			// pointers to functions whose parameters or result differ in const: no conversion C++ makes by itself
			if (category(from) == Category::pointer &&
				m_types.canonical(m_types.parts(m_types.canonical(to.key)).of).starts_with("fn(") &&
				withConst(from.key) != withConst(to.key))
				return make(Kind::reinterpret, to, from, "reinterprets");
			return std::nullopt;
		}
		// a number prints as a literal: an int at least
		int64_t size = value && value->op == cot_num ? std::max<int64_t>(sizeOf(from), 4) : sizeOf(from);
		bool fits = size > 0 && size == sizeOf(to);
		switch (t)
		{
		case Category::pointer:
			if (f == Category::pointer)
				return pointers(from, to);
			if ((f == Category::integer || f == Category::enumeration || f == Category::boolean) && !zero(value))
			{
				// a sentinel the exe compares the pointer with (comparedSentinel): the same bits on x64
				Conversion c = make(Kind::integerToPointer, to, from, "converts");
				auto up = value ? m_parents.find(value) : m_parents.end();
				if (up != m_parents.end() && up->second->op == cot_cast &&
					comparedSentinel(m_types, static_cast<const cexpr_t*>(up->second), m_parents))
					c.what.clear();
				return c;
			}
			if (bytes(f) && fits)
				return make(Kind::bits, to, from, "reads the bytes of");
			return std::nullopt;
		case Category::integer:
		case Category::enumeration:
			if (f == Category::pointer)
				return make(Kind::pointerToInteger, to, from, "converts");
			if (bytes(f))
				return fits ? std::optional(bitsOf(to, from)) : std::nullopt;
			if (t == Category::enumeration && (f == Category::integer || f == Category::floating ||
											   f == Category::boolean || f == Category::enumeration))
				return make(Kind::staticCast, to, from, "converts");
			if (t == Category::integer && f == Category::enumeration && scoped(from))
				return make(Kind::staticCast, to, from, "converts");
			return std::nullopt;
		case Category::floating:
			if (bytes(f))
				return fits ? std::optional(bitsOf(to, from)) : std::nullopt;
			if (f == Category::enumeration && scoped(from))
				return make(Kind::staticCast, to, from, "converts");
			return std::nullopt;
		case Category::boolean:
			if (bytes(f))
				return fits ? std::optional(make(Kind::bits, to, from, "reads the bytes of")) : std::nullopt;
			// C++ makes any nonzero value 1 where the exe keeps the byte it stores or passes: none only where the
			// value is 0 or 1 already
			if (f == Category::boolean || truthValue(value))
				return std::nullopt;
			return make(Kind::staticCast, to, from, "makes 0 or 1 of");
		case Category::record:
		case Category::vector:
		case Category::opaque:
		{
			if (f == Category::record && t == Category::record &&
				isBase(m_types.parts(m_types.canonical(from.key)).name, m_types.parts(m_types.canonical(to.key)).name))
				return std::nullopt;
			// IDA's structs src has no key for print as gen::Blob<N>, its integers wider than 8 bytes as gen::Wide<N,
			// signed>: one of a size, kind and signedness is one type
			if (f == Category::opaque && t == Category::opaque && fits && from.ida.is_udt() == to.ida.is_udt() &&
				(from.ida.is_udt() || from.ida.is_unsigned() == to.ida.is_unsigned()))
				return std::nullopt;
			// the number 0 for an object of an empty class: the exe's zero byte (MSVC's value-initialised temporary,
			// `hkContainerHeapAllocator()`), which C++ writes as value-initialisation, of no bytes on any layout
			if (t == Category::record && f == Category::integer && zero(value))
				if (const SrcModel::Record* r = m_model.record(m_types.parts(m_types.canonical(to.key)).name);
					r && m_model.isEmpty(*r))
				{
					Conversion c = make(Kind::valueInit, to, from, "");
					c.what.clear();
					return c;
				}
			if (std::optional<Conversion> lane = t == Category::vector ? lowLane(from, to, value) : std::nullopt)
				return lane;
			// a scalar's bytes, a pointer's included (a record of a pointer's size Hex-Rays passes as the pointer,
			// `(NiFixedString)p`; the pointer case above reads a record's bytes the other way)
			bool value = bytes(f) || f == Category::integer || f == Category::floating || f == Category::pointer;
			if (value && size > 0 && size == sizeOf(to))
				return bitsOf(to, from);
			if (value && size > 0 && size < sizeOf(to))
				return make(Kind::bits, to, from, "reads the bytes of");
			return std::nullopt;
		}
		default:
			return std::nullopt;
		}
	}

	// a pointer to another: to void and up to a base at offset 0 by itself (const kept), from void and down by a
	// static_cast, else a reinterpret_cast; a const dropped through a const_cast
	std::optional<Conversion> pointers(const Printed& from, const Printed& to) const
	{
		if (from.key.empty() || to.key.empty())
		{
			// IDA's pointer printed raw: one printed alike converts
			tinfo_t a = from.key.empty() ? from.ida : tinfo_t(), b = to.key.empty() ? to.ida : tinfo_t();
			if (!a.empty() && !b.empty() && text(a) == text(b))
				return std::nullopt;
			if (!a.empty() && !b.empty() && a.get_pointed_object().get_size() == b.get_pointed_object().get_size() &&
				a.get_pointed_object().is_udt() && b.get_pointed_object().is_udt())
				return std::nullopt;
			// an object pointer to void* or const void*, its const kept: C++ converts it by itself
			Pointee source = pointee(from), target = pointee(to);
			if (target.isVoid && !source.isFunction && (!source.isConst || target.isConst))
				return std::nullopt;
			return make(Kind::reinterpret, to, from, "reinterprets");
		}
		std::string a = m_types.canonical(from.key), b = m_types.canonical(to.key);
		SrcTypes::Parts pa = m_types.parts(m_types.parts(a).of), pb = m_types.parts(m_types.parts(b).of);
		bool drops = dropsConst(from.key, to.key);
		auto through = [&](Kind kind, const std::string& what) {
			Conversion c = make(kind, to, from, what);
			if (drops)
				c.unconst = a;
			return c;
		};
		// an object pointer to void* converts by itself; a function pointer only under MSVC's extension, which
		// native C++ has not
		if (m_types.parts(b).of == "builtin:void")
			return pa.kind == "fn" ? std::optional(through(Kind::reinterpret, "reinterprets"))
				   : drops		   ? std::optional(make(Kind::constCast, {a, {}}, from, "drops const of"))
								   : std::nullopt;
		if (m_types.parts(a).of == "builtin:void")
			return pb.kind == "fn" ? through(Kind::reinterpret, "reinterprets") : through(Kind::staticCast, "converts");
		if (pa.kind == "record" && pb.kind == "record")
		{
			// C++'s upcast to a base at another offset adds the offset (and a null test) to a value that is the
			// exe's unadjusted pointer (Hex-Rays writes an adjusting conversion as `&p->Base`, Implicit's): the same
			// address read as the base, marked
			if (isBase(pa.name, pb.name) && !m_types.derivesAtZero(pa.name, pb.name))
				return through(Kind::reinterpret, "reinterprets (a base at a nonzero offset)");
			if (isBase(pa.name, pb.name))
				return drops ? std::optional(through(Kind::staticCast, "converts")) : std::nullopt;
			if (isBase(pb.name, pa.name))
				return through(Kind::staticCast, "converts down");
		}
		// a record's pointer used as the pointer to its member at offset 0: that member's address, the same address
		// (the member of the target's pointee with its consts: `&s->m_kHandle` is a `const char**`, no `char**`)
		std::string target = withConst(to.key);
		if (m_types.parts(target).kind == "const" || m_types.parts(target).kind == "volatile")
			target = m_types.parts(target).of;
		if (pa.kind == "record" && !drops)
			if (std::optional<std::string> path = memberAtZero(pa.name, m_types.parts(target).of, 0))
			{
				Conversion c = make(Kind::memberAddress, to, from, "");
				c.what.clear();
				c.form = *path;
				return c;
			}
		return through(Kind::reinterpret, "reinterprets");
	}

	// the path from a record to its member at offset 0 of the type (withConst's: typedefs resolved, consts kept):
	// `m`, `m.n` through a by-value record member, `a[0]` for an array's first element; an anonymous member's members
	// are the record's own (no bit-field, no const member: its address is a pointer to const). nullopt where no
	// member at 0 is of the type.
	std::optional<std::string> memberAtZero(const std::string& record, const std::string& type, int depth) const
	{
		const SrcModel::Record* r = depth < 8 ? m_model.record(record) : nullptr;
		if (!r)
			return std::nullopt;
		for (const SrcModel::Field& field : r->fields)
		{
			if (field.offsetBits != 0 || field.bits || m_types.parts(m_types.bare(field.type)).kind == "const")
				continue;
			std::string held = m_types.canonical(field.type);
			const std::string& name = field.name;
			if (!name.empty() && withConst(field.type) == type)
				return name;
			SrcTypes::Parts p = m_types.parts(held);
			if (p.kind == "array" && !name.empty() &&
				withConst(m_types.parts(withConst(field.type)).of) == type)
				return name + "[0]";
			if (p.kind == "record")
				if (std::optional<std::string> inner = memberAtZero(p.name, type, depth + 1))
					return name.empty() ? *inner : name + "." + *inner;
		}
		return std::nullopt;
	}

	// the same bytes as another type of their size: std::bit_cast, clean, where both are fixed-width scalars or
	// compiler vectors (plainBits), whose bits are the exe's on any layout; else gen::bitcast, marked
	Conversion bitsOf(const Printed& to, const Printed& from) const
	{
		if (!plainBits(from) || !plainBits(to))
			return make(Kind::bits, to, from, "reads the bytes of");
		Conversion c = make(Kind::sameBits, to, from, "");
		c.what.clear();
		return c;
	}

	// a src integer of a width the same on x64 (no bool, enum or pointer-sized one), a floating type or a compiler
	// vector
	bool plainBits(const Printed& p) const
	{
		if (p.key.empty())
			return false;
		Category c = category(p);
		return c == Category::floating || c == Category::vector ||
			   (c == Category::integer && !hostWidth(m_context, p.key));
	}

	// Hex-Rays' cast of a 4-byte value to a 16-byte compiler vector is a zero extension (microcode's xdu: movss or
	// movd from memory clear the other lanes): the vector of the value in its low lane and zeros, `_mm_set_ss` of
	// its bits (`_mm_setzero_ps()` for the number 0), `_mm_cvtsi32_si128` for an integer vector. Only an unsigned
	// integer or a number without its sign bit, of the cast's operand's size: a signed one's cast may be a sign
	// extension (xds). The value is the cast's operand, or the cast itself where it prints as its operand
	// (Implicit); the number 0 anywhere (a vector of zeros however Hex-Rays widened it). A value Hex-Rays passes to
	// a vector parameter without a cast proves no zero extension. nullopt for any other.
	std::optional<Conversion> lowLane(const Printed& from, const Printed& to, const cexpr_t* value) const
	{
		auto up = value ? m_parents.find(value) : m_parents.end();
		bool widened = up != m_parents.end() && up->second->op == cot_cast &&
					   static_cast<const cexpr_t*>(up->second)->type.get_size() == 16;
		const cexpr_t* operand = widened || (value && value->op == cot_num && value->n->_value == 0) ? value
								 : value && value->op == cot_cast && meaningOf<Implicit>(value) &&
										 value->type.get_size() == 16
									 ? value->x
									 : nullptr;
		bool number = operand && operand->op == cot_num;
		if (!operand || category(from) != Category::integer || sizeOf(from) != 4 ||
			(operand->type.get_size() != 4 && !number) || sizeOf(to) != 16 || to.key.empty() ||
			hostWidth(m_context, from.key))
			return std::nullopt;
		uint64_t bits = number ? operand->n->_value : 0;
		if (number ? (bits & 0x80000000u) != 0 && !operand->type.is_unsigned() : !operand->type.is_unsigned())
			return std::nullopt;
		SrcTypes::Parts vector = m_types.parts(m_types.canonical(to.key));
		bool floats = m_types.canonical(vector.of) == "builtin:float";
		Conversion c = make(Kind::lowLane, to, from, "");
		c.what.clear();
		bool zero = number && (bits & 0xFFFFFFFFu) == 0;
		c.form = floats ? (zero ? "_mm_setzero_ps()" : "_mm_set_ss(std::bit_cast<float>($0))")
						: (zero ? "_mm_setzero_si128()" : "_mm_cvtsi32_si128($0)");
		c.headers.push_back(floats ? "<xmmintrin.h>" : "<emmintrin.h>");
		if (floats && !zero)
			c.headers.push_back("<bit>");
		return c;
	}

	// the function type a pointer key points to, typedefs resolved and consts kept but a parameter's own; "" for none
	std::string pointedFunction(std::string key) const
	{
		for (int depth = 0; depth < 16; ++depth)
		{
			SrcTypes::Parts p = m_types.parts(key);
			if (p.kind == "typedef")
			{
				const SrcModel::Typedef* alias = m_model.typedefNamed(p.name);
				if (!alias)
					return "";
				key = alias->target;
			}
			else if (p.kind == "const" || p.kind == "volatile")
				key = p.of;
			else if (p.kind == "ptr")
			{
				std::string of = exactType(p.of);
				return SrcTypes::function(of) ? of : "";
			}
			else
				return "";
		}
		return "";
	}
	std::string exactType(const std::string& key) const
	{
		if (std::optional<SrcTypes::Function> f = SrcTypes::function(key))
		{
			f->ret = exactType(f->ret);
			for (std::string& param : f->params)
			{
				param = exactType(param);
				if (m_types.parts(param).kind == "const")
					param = m_types.parts(param).of;
			}
			return SrcTypes::functionKey(*f);
		}
		SrcTypes::Parts p = m_types.parts(key);
		if (p.kind == "typedef")
		{
			const SrcModel::Typedef* alias = m_model.typedefNamed(p.name);
			return alias ? exactType(alias->target) : key;
		}
		if (p.kind == "const" || p.kind == "volatile" || p.kind == "ptr" || p.kind == "lref" || p.kind == "rref")
			return p.kind + "(" + exactType(p.of) + ")";
		if (p.kind == "array")
			return "array(" + exactType(p.of) + "," + std::to_string(p.count) + ")";
		return m_types.canonical(key);
	}

	// what a pointer printed with the type points to: void, const, a function
	struct Pointee
	{
		bool isVoid = false, isConst = false, isFunction = false;
	};
	Pointee pointee(const Printed& pointer) const
	{
		Pointee out;
		if (!pointer.key.empty())
		{
			SrcTypes::Parts p = m_types.parts(m_types.canonical(pointer.key));
			std::string of = m_types.bare(p.of);
			out.isConst = m_types.constPointee(pointer.key);
			if (m_types.parts(of).kind == "const")
				of = m_types.bare(m_types.parts(of).of);
			out.isVoid = of == "builtin:void";
			out.isFunction = m_types.parts(of).kind == "fn";
			return out;
		}
		if (!pointer.ida.is_ptr())
			return out;
		tinfo_t object = pointer.ida.get_pointed_object();
		out.isVoid = object.is_void();
		out.isConst = object.is_const();
		out.isFunction = object.is_func();
		return out;
	}

	// --- types

	// the type e has as printed: its conversion's, else src's (a const `this` of a const member, the address of a
	// const object to const; a part helper's read, GenSupport's integer of its width, whatever Hex-Rays types the
	// call), else IDA's
	Printed printed(const cexpr_t* e) const
	{
		if (auto found = m_function.conversions.find(e); found != m_function.conversions.end())
			return {found->second.addConst.empty() ? found->second.type : found->second.addConst, found->second.ida};
		if (std::optional<Printed> part = partTarget(e))
			return {part->key, e->type};
		if (e->op == cot_cast && meaningOf<Implicit>(e))
			return printed(e->x);
		if (const Enumerators* names = meaningOf<Enumerators>(e))
			return {names->type, e->type};
		if (const ConstCast* drop = meaningOf<ConstCast>(e))
			return {drop->type, e->type};
		if (const ExeCast* bytes = meaningOf<ExeCast>(e))
			return {bytes->type, e->type};
		// a reference parameter is Hex-Rays' pointer: printed as the object's address
		if (int v = lvarIndex(e); v >= 0 && m_function.locals[v].reference)
			return {"ptr(" + m_types.parts(m_types.bare(m_function.locals[v].type)).of + ")", e->type};
		// a function's address prints as its bytes' (`reinterpret_cast<uint8_t*>(0x...u)`), unless Globals made it
		// src's `&F`
		if (e->op == cot_obj && functionStart(e) && !meaningOf<Rewrite>(e))
			return {"ptr(builtin:unsigned char)", e->type};
		// what a pointer points to, an element: as the pointer's (or array's) printed type has it (the pointer's own
		// const aside; an element of a const array const)
		if ((e->op == cot_ptr || e->op == cot_idx) && !m_function.meaning.count(e))
		{
			Printed pointer = printed(e->x);
			SrcTypes::Parts p = m_types.parts(m_types.bare(pointer.key));
			bool constant = p.kind == "const";
			if (constant)
				p = m_types.parts(m_types.bare(p.of));
			if (!pointer.key.empty() && (p.kind == "ptr" || (e->op == cot_idx && p.kind == "array")))
			{
				bool constElement =
					constant && p.kind == "array" && m_types.parts(m_types.bare(p.of)).kind != "const";
				return {constElement ? "const(" + p.of + ")" : p.of, e->type};
			}
		}
		// a pointer moved by a number points to what it pointed to
		if ((e->op == cot_add || e->op == cot_sub) && e->x->type.is_ptr() && !e->y->type.is_ptr() &&
			!m_function.meaning.count(e))
		{
			Printed pointer = printed(e->x);
			if (!pointer.key.empty() && m_types.parts(m_types.bare(pointer.key)).kind == "ptr")
				return {pointer.key, e->type};
		}
		// a record returned by value into a local, where its value is used: printed `&(local = F())`, the local's
		// address; a reference returned where Hex-Rays has the pointer: printed `&F()`
		if (const Call* call = e->op == cot_call ? meaningOf<Call>(e) : nullptr)
			if (const SrcModel::Type* type = m_model.type(call->function->type))
			{
				if (call->resultArgument >= 0)
					return {"ptr(" + type->ret + ")", e->type};
				if (call->returnsReference && e->type.is_ptr())
					return {"ptr(" + m_types.parts(m_types.bare(type->ret)).of + ")", e->type};
			}
		std::string key = srcType(m_function, m_context, e);
		// arithmetic Hex-Rays types as an enum (`GetMonth() << 9`) is C++'s integer: its operands' promoted type
		if (integerOperator(e->op) && !m_function.meaning.count(e) &&
			category(Printed{key, e->type}) == Category::enumeration)
			if (std::string result = promotedResult(m_context, e->op, printed(e->x).key,
													op_uses_y(e->op) ? printed(e->y).key : "");
				!result.empty())
				key = result;
		// an accessor's field address through a const object (a const `this` too): a pointer to const
		if (const Access* access = e->op == cot_call ? meaningOf<Access>(e) : nullptr;
			access && access->field && !access->load && !access->value && !m_types.constPointee(key))
		{
			const cexpr_t* receiver = receiverOf(e);
			bool constant = receiver->op == cot_ref ? objectConst(receiver->x)
													: m_types.constPointee(printed(receiver).key);
			if (constant && !m_types.bare(access->field->type).starts_with("const("))
				key = "ptr(const(" + access->field->type + "))";
		}
		if (m_constThis && isThis(m_function, e) && m_types.parts(m_types.bare(key)).kind == "ptr" &&
			!m_types.constPointee(key))
			key = "ptr(const(" + m_types.parts(m_types.bare(key)).of + "))";
		// a record or array field of a const object is const
		if ((e->op == cot_memptr || e->op == cot_memref) && meaningOf<Member>(e) && objectConst(e))
		{
			std::string kind = m_types.parts(m_types.canonical(key)).kind;
			if ((kind == "record" || kind == "array") && m_types.parts(m_types.bare(key)).kind != "const")
				key = "const(" + key + ")";
		}
		bool lvalue = e->op == cot_ref && !m_function.meaning.count(e) &&
					  ((e->x->op == cot_memptr || e->x->op == cot_memref)
						   ? meaningOf<Member>(e->x) != nullptr
					   : e->x->op == cot_var ? !m_function.locals[e->x->v.idx].reference
											 : e->x->op == cot_idx || e->x->op == cot_ptr);
		if (lvalue)
		{
			Printed object = printed(e->x);
			if (!object.key.empty())
			{
				if (objectConst(e->x) && m_types.parts(m_types.bare(object.key)).kind != "const")
					object.key = "const(" + object.key + ")";
				key = "ptr(" + object.key + ")";
			}
		}
		return {key, e->type};
	}

	// the object the lvalue names is const: reached through a pointer to const, a const reference parameter or a
	// const object
	bool objectConst(const cexpr_t* x) const
	{
		switch (x->op)
		{
		case cot_memptr:
		case cot_ptr:
			return m_types.constPointee(printed(x->x).key);
		case cot_memref:
			return objectConst(x->x);
		case cot_idx:
			return x->x->type.is_ptr() ? m_types.constPointee(printed(x->x).key) : objectConst(x->x);
		case cot_var:
		{
			const Local& local = m_function.locals[x->v.idx];
			return local.reference && m_types.constPointee(local.type);
		}
		default:
			return false;
		}
	}

	Category category(const Printed& p) const
	{
		if (!p.key.empty())
		{
			SrcTypes::Parts parts = m_types.parts(m_types.canonical(p.key));
			if (parts.kind == "ptr")
				return Category::pointer;
			if (parts.kind == "enum")
				return Category::enumeration;
			if (parts.kind == "record")
				return Category::record;
			if (parts.kind == "vector")
				return Category::vector;
			if (parts.kind != "builtin")
				return Category::other;
			const SrcModel::Type* type = m_model.type(m_types.canonical(p.key));
			if (parts.name == "void" || p.key == "builtin:void" || !type)
				return Category::other;
			if (type->isFloat)
				return Category::floating;
			return m_types.canonical(p.key) == "builtin:bool" ? Category::boolean : Category::integer;
		}
		const tinfo_t& t = p.ida;
		if (t.empty())
			return Category::other;
		if (t.is_ptr())
			return Category::pointer;
		if (t.is_bool())
			return Category::boolean;
		if (t.is_floating())
			return Category::floating;
		if ((t.is_integral() || t.is_enum()) && t.get_size() <= 8)
			return Category::integer;
		if (t.is_udt() || t.is_integral())
			return Category::opaque;
		return Category::other;
	}

	int64_t sizeOf(const Printed& p) const
	{
		if (!p.key.empty())
			return m_model.sizeOf(p.key).value_or(0);
		size_t size = p.ida.empty() ? BADSIZE : p.ida.get_size();
		return size == BADSIZE ? 0 : static_cast<int64_t>(size);
	}

	// to's pointer type with a const at each pointee level where from's has one (to's own consts kept), so that a
	// reinterpret_cast from from to it casts no const away: `const void**` for `void**` of a `const uint16_t**`
	std::string keepConsts(const std::string& from, const std::string& to) const
	{
		SrcTypes::Parts pf = m_types.parts(m_types.bare(from)), pt = m_types.parts(m_types.bare(to));
		if ((pf.kind != "ptr" && pf.kind != "lref") || pt.kind != "ptr")
			return to;
		std::string a = m_types.bare(pf.of), b = m_types.bare(pt.of);
		bool ca = m_types.parts(a).kind == "const", cb = m_types.parts(b).kind == "const";
		std::string inner = keepConsts(ca ? m_types.parts(a).of : a, cb ? m_types.parts(b).of : b);
		return "ptr(" + (ca || cb ? "const(" + inner + ")" : inner) + ")";
	}

	// the key of an IDA type (empty where src has none: printed raw)
	Printed idaType(const tinfo_t& type) const
	{
		std::string why;
		return {m_types.keyOf(type, why).value_or(""), type};
	}

	bool voidPointer(const Printed& p) const
	{
		return !p.key.empty() && m_types.canonical(p.key) == "ptr(builtin:void)";
	}

	// a const dropped at any level of pointers or references (`const T*` to `T*`, `T* const*` to `T**`)
	bool dropsConst(const std::string& from, const std::string& to) const
	{
		std::string a = m_types.bare(from), b = m_types.bare(to);
		for (;;)
		{
			SrcTypes::Parts pa = m_types.parts(a), pb = m_types.parts(b);
			auto indirect = [](const SrcTypes::Parts& p) { return p.kind == "ptr" || p.kind == "lref"; };
			if (!indirect(pa) || !indirect(pb))
				return false;
			std::string x = m_types.bare(pa.of), y = m_types.bare(pb.of);
			bool cx = m_types.parts(x).kind == "const", cy = m_types.parts(y).kind == "const";
			if (cx && !cy)
				return true;
			a = cx ? m_types.bare(m_types.parts(x).of) : x;
			b = cy ? m_types.bare(m_types.parts(y).of) : y;
		}
	}

	// to's type adds a const below the first pointee where a level above it is not const (`T**` to `const T**`), a
	// qualification conversion C++ does not make by itself (it would let a `const T*` be stored through the result);
	// const_cast adds it, no reinterpretation
	bool addsDeepConst(const std::string& from, const std::string& to) const
	{
		std::string a = m_types.bare(from), b = m_types.bare(to);
		bool allConst = true;
		for (int level = 1;; ++level)
		{
			SrcTypes::Parts pa = m_types.parts(a), pb = m_types.parts(b);
			if (pa.kind != "ptr" || pb.kind != "ptr")
				return false;
			std::string x = m_types.bare(pa.of), y = m_types.bare(pb.of);
			bool cx = m_types.parts(x).kind == "const", cy = m_types.parts(y).kind == "const";
			if (cy && !cx && level > 1 && !allConst)
				return true;
			allConst = allConst && cy;
			a = cx ? m_types.bare(m_types.parts(x).of) : x;
			b = cy ? m_types.bare(m_types.parts(y).of) : y;
		}
	}

	bool scoped(const Printed& p) const
	{
		const SrcModel::Enum* e = m_model.enumeration(m_types.parts(m_types.canonical(p.key)).name);
		return e && e->scoped;
	}

	// base is derived or one of its bases, at any offset
	bool isBase(const std::string& derived, const std::string& base) const
	{
		if (derived == base)
			return true;
		const SrcModel::Record* record = m_model.record(derived);
		if (!record)
			return false;
		for (const SrcModel::Base& b : record->bases)
			if (isBase(b.record, base))
				return true;
		return false;
	}

	// a pointer C++ converts to the other by itself at the same address, keeping its const: to void*, or from a src
	// class deriving at offset 0 from the class the other points to
	bool convertsAtSameAddress(const std::string& from, const std::string& to) const
	{
		SrcTypes::Parts f = m_types.parts(m_types.canonical(from)), t = m_types.parts(m_types.canonical(to));
		if (f.kind != "ptr" || t.kind != "ptr" || (m_types.constPointee(from) && !m_types.constPointee(to)))
			return false;
		if (t.of == "builtin:void")
			return m_types.parts(f.of).kind != "fn";
		SrcTypes::Parts derived = m_types.parts(m_types.bare(f.of)), base = m_types.parts(m_types.bare(t.of));
		return derived.kind == "record" && base.kind == "record" && m_types.derivesAtZero(derived.name, base.name);
	}

	// a value of one integer, enum or bool type to another of the same size (not a number: its literal is exact)
	bool sameSizeInteger(const Printed& from, const Printed& to, const cexpr_t* value) const
	{
		auto integral = [](Category c) {
			return c == Category::integer || c == Category::enumeration || c == Category::boolean;
		};
		if (value->op == cot_num || !integral(category(from)) || !integral(category(to)))
			return false;
		int64_t size = sizeOf(from);
		if (size <= 0 || size != sizeOf(to))
			return false;
		if (!from.key.empty() && !to.key.empty())
			return m_types.canonical(from.key) != m_types.canonical(to.key);
		return text(from.ida) != text(to.ida);
	}

	// --- C++'s implicit conversion (user, 2026-11-12): where the value is a local or an expression only IDA or
	// Hex-Rays types (no src declaration gives its type), its conversion to a same-size integer of a src declaration
	// (a parameter, a field or global assigned, the return) prints unmarked, as C++'s implicit conversion. Where both
	// are src declarations it stays the proof's (above): one of the two declarations is wrong.

	// the value converts implicitly to the integer to: both integers of one size (char included; no bool or enum, none
	// of a type whose width differs on x64, hostWidth: there it would widen) and the value only IDA or Hex-Rays types.
	// A size_t (hostWidth) takes it too: on the exe it is the dword the caller pushes, and the implicit conversion of
	// an int or unsigned count is what the source wrote (`memset(p, 0, 4 * n)`); so does a difference of pointers,
	// C++'s ptrdiff_t, which is size_t's width on any layout (`deallocate(_Myfirst, _Myend - _Myfirst)`). Not the
	// other pointer-sized types (ptrdiff_t, intptr_t, uintptr_t): there the value may be an address's bits.
	bool implicitInteger(const cexpr_t* value, const std::string& to) const
	{
		std::string from = printed(value).key;
		if (from.empty() || to.empty())
			return false;
		bool count = sizeType(to);
		if (count && pointerDifference(value))
			return true;
		if (hostWidth(m_context, from) || (hostWidth(m_context, to) && !count))
			return false;
		std::optional<std::pair<int64_t, bool>> a = integerOf(from), b = integerOf(to);
		return a && b && a->first == b->first && hexRaysTyped(value);
	}

	// the key is size_t (its cv aside, through typedefs naming it)
	bool sizeType(const std::string& key) const
	{
		std::string k = key;
		for (int depth = 0; depth < 16; ++depth)
		{
			SrcTypes::Parts p = m_types.parts(k);
			if (p.kind == "const" || p.kind == "volatile")
			{
				k = p.of;
				continue;
			}
			if (!k.starts_with("typedef:"))
				return false;
			std::string name = k.substr(8);
			if (name == "size_t")
				return true;
			const SrcModel::Typedef* t = m_model.typedefNamed(name);
			if (!t)
				return false;
			k = t->target;
		}
		return false;
	}

	// a difference of two pointers that print as pointers to one src type (cv apart), of Hex-Rays' pointee size: the
	// count of elements between them, C++'s ptrdiff_t, on any layout
	bool pointerDifference(const cexpr_t* e) const
	{
		if (e->op != cot_sub || !e->x->type.is_ptr() || !e->y->type.is_ptr() || m_function.meaning.count(e) ||
			m_function.conversions.count(e->x) || m_function.conversions.count(e->y))
			return false;
		std::optional<PointeeCv> x = pointeeOf(m_types, printed(e->x).key), y = pointeeOf(m_types, printed(e->y).key);
		if (!x || !y || x->type.empty() || m_types.canonical(x->type) != m_types.canonical(y->type))
			return false;
		std::optional<int64_t> size = m_model.sizeOf(x->type);
		size_t ida = e->x->type.get_ptrarr_objsize();
		return size && *size > 0 && ida != BADSIZE && static_cast<size_t>(*size) == ida;
	}

	// The value's type is IDA's or Hex-Rays' alone, no src declaration gives it: a local of the body (not a parameter
	// or a copy of one, raw storage or a reference; an alias as the local it prints as) or an element of a local array,
	// a number (no enumerator) or a `sizeof`, Hex-Rays' cast of such a value (one printed as its operand too), such a
	// local stepped (`i++`), or arithmetic, a bitwise operation, a negation or a choice of such values only. An operand
	// C++ promotes to int (promotedOperand) is such a value too: the operation's type is C++'s int, which no
	// declaration gives (`SetSize(m_usSize + m_usGrowBy)`). A parameter, field, global, call's result or accessor's
	// load is a src declaration's value; anything a pass gave another meaning is not judged here.
	bool hexRaysTyped(const cexpr_t* e) const
	{
		if (auto found = m_function.conversions.find(e); found != m_function.conversions.end())
			return found->second.kind == Kind::implicit && found->second.replacesCast && hexRaysTyped(e->x);
		auto found = m_function.meaning.find(e);
		bool plain = found == m_function.meaning.end() || std::holds_alternative<std::monostate>(found->second);
		auto operand = [&](const cexpr_t* x) { return hexRaysTyped(x) || promotedOperand(x); };
		switch (e->op)
		{
		case cot_num:
		case cot_sizeof:
			return !meaningOf<Enumerators>(e) && !meaningOf<Missing>(e);
		case cot_var:
		{
			int v = lvarIndex(e);
			for (int depth = 0; v >= 0 && m_function.locals[v].aliasOf >= 0 && depth < 64; ++depth)
				v = m_function.locals[v].aliasOf;
			if (v < 0 || (*m_function.cfunc->get_lvars())[v].is_arg_var())
				return false;
			const Local& local = m_function.locals[v];
			return local.parameter < 0 && !local.entryCopy && !local.storage && !local.reference && !local.inlined &&
				   !local.type.empty() && plain;
		}
		case cot_cast:
			return (meaningOf<std::string>(e) || meaningOf<Implicit>(e)) && hexRaysTyped(e->x);
		case cot_neg: case cot_bnot:
			return plain && operand(e->x);
		case cot_preinc: case cot_predec: case cot_postinc: case cot_postdec:
			return plain && hexRaysTyped(e->x);
		case cot_idx:
			return plain && e->x->op == cot_var &&
				   (*m_function.cfunc->get_lvars())[lvarIndex(e->x)].type().is_array() && hexRaysTyped(e->x);
		case cot_tern:
			return plain && operand(e->y) && operand(e->z);
		case cot_add: case cot_sub: case cot_mul: case cot_sdiv: case cot_udiv: case cot_smod: case cot_umod:
		case cot_band: case cot_bor: case cot_xor: case cot_shl: case cot_sshr: case cot_ushr:
			return plain && !e->x->type.is_ptr() && !e->y->type.is_ptr() && operand(e->x) && operand(e->y);
		default:
			return false;
		}
	}

	// an operand C++ promotes to int whatever declares it: an integer narrower than int (no enum) or a bool
	bool promotedOperand(const cexpr_t* e) const
	{
		Printed p = printed(e);
		Category c = category(p);
		int64_t size = sizeOf(p);
		return (c == Category::integer || c == Category::boolean) && size > 0 && size < 4;
	}

	// The src type of the declaration the value goes to as it is: a resolved call's parameter (by value, no cast of
	// Calls'), a src field or global assigned, the function's return (not a reference or a record by value). "" for any
	// other place.
	std::string srcDestination(const cexpr_t* value) const
	{
		auto up = m_parents.find(value);
		if (!m_function.self || up == m_parents.end() || !up->second)
			return "";
		const citem_t* parent = up->second;
		if (parent->op == cit_return)
			return !m_function.returnsReference && m_function.resultArgument < 0 ? m_function.returnType : "";
		if (parent->op >= cit_empty)
			return "";
		const cexpr_t* p = static_cast<const cexpr_t*>(parent);
		if (p->op == cot_asg && p->y == value)
		{
			if (const Member* member = p->x->op == cot_memptr || p->x->op == cot_memref ? meaningOf<Member>(p->x)
																					   : nullptr)
				return member->field ? member->field->type : "";
			if (const SrcModel::Ref* const* ref = p->x->op == cot_obj ? meaningOf<const SrcModel::Ref*>(p->x) : nullptr)
				return (*ref)->type;
			return "";
		}
		const Call* call = p->op == cot_call && p->x != value ? meaningOf<Call>(p) : nullptr;
		const SrcModel::Type* type = call ? m_model.type(call->function->type) : nullptr;
		for (size_t k = 0; type && k < p->a->size(); ++k)
		{
			if (&(*p->a)[k] != value)
				continue;
			int parameter = parameterIndex(*call, k);
			if (parameter < 0 || static_cast<size_t>(parameter) >= type->params.size() ||
				(k < call->castTo.size() && !call->castTo[k].empty()) ||
				(k < call->byReference.size() && call->byReference[k]))
				return "";
			return type->params[parameter];
		}
		return "";
	}

	// --- the proof (user, 2026-11-10: "if it can be proved, it can be cast"; 2026-11-11: "only if its provably
	// correct that a static_cast was used originally"): a same-size integer conversion between two signednesses is
	// the original source's own, which C++ makes as the exe did, where the value and where it goes are both
	// declarations whose src type is evidence of the original's: the debug build's PDB's (SrcModel::pdbTyped), an
	// `// Inferred type` clause's, or a library's own declaration (libraryDeclared). A type a `// PDB:` line records
	// for a reason of the exe's proves an object's class but no signedness: the record says the exe contradicts the
	// PDB's form, which proves a width or a kind, not the original's signedness (user, 2026-11-11). A type from
	// Hex-Rays, IDA's guess, a src declaration no evidence confirms or an expression's result proves nothing.

	// how far the value's conversion to (the type) to at site (the value, or the cast of it) is proved the original
	// source's own: none where the two are no integers of one size and two signednesses outside the types whose
	// width differs on x64 (hostWidth: there the conversion would widen); proved where the value's declaration
	// (provenDeclared) and the site's destination (provenDestination, of the type to) both prove it; else which of
	// the two does not
	Proof proof(const cexpr_t* value, const std::string& to, const cexpr_t* site) const
	{
		std::string from = printed(value).key;
		if (from.empty() || to.empty() || hostWidth(m_context, from) || hostWidth(m_context, to))
			return Proof::none;
		std::optional<std::pair<int64_t, bool>> a = integerOf(from), b = integerOf(to);
		if (!a || !b || a->first != b->first || a->second == b->second)
			return Proof::none;
		std::string destination = provenDestination(site);
		bool declared = !provenDeclared(value, Evidence::signedness).empty();
		bool goes = !destination.empty() && m_types.canonical(destination) == m_types.canonical(to);
		return declared && goes ? Proof::proved : declared ? Proof::destination : goes ? Proof::value : Proof::neither;
	}

	// the function's slot (a parameter's index from 0, `ret`, `this`) is of the type the original source declared,
	// as evidence of what (above): the PDB's, (not `this`) a Havok library symbol's (a declaration of provenance
	// Library, typed as the PDB's are) or one an `// Inferred type` clause names, or, as an object's class only, one a
	// `// PDB:` line records for a reason of the exe's
	bool slotProven(const SrcModel::Function& f, const std::string& slot, Evidence what) const
	{
		if (m_model.pdbTyped("param", f.mangled, slot) ||
			(what == Evidence::object && m_model.pdbTyped("recorded", f.mangled, slot)))
			return true;
		return slot != "this" &&
			   (f.provenance == SrcModel::Provenance::Inferred || f.provenance == SrcModel::Provenance::Library ||
				f.inferredSlots.count(slot) != 0);
	}

	// the record's field is of the type the original source declared, as evidence of what (above): the PDB's, or, as
	// an object's class only, one a `// PDB:` line above it records for a reason of the exe's
	bool fieldProven(const std::string& record, const std::string& field, Evidence what) const
	{
		return m_model.pdbTyped("field", record, field) ||
			   (what == Evidence::object && m_model.pdbTyped("field_recorded", record, field));
	}

	// The callee's declaration (its parameters and its result) is the library's own (user, 2026-11-11): a call
	// through a Windows import's IAT slot (exefacts' import table) of the src global declared there, or a CRT function
	// src declares by hand under its VC9 name (namespace msvc9 or a header of src/SDK/MSVC). Not one the model has as
	// a guess (an IDA name, IDA's parameters: not provably the library's), but the compiler's eh vector helpers of
	// ehvec.h, whose IDA name only stands in for one C++ cannot spell; and not a copy src names after its exe address
	// (`abs_72FA80`, `_sub_60C410`): that is the exe's own code, typed as src guessed it.
	bool libraryDeclared(const SrcModel::Function& f, const cexpr_t* callee) const
	{
		if (callee->op == cot_obj && m_context.exe.program().importPops(static_cast<uint32_t>(callee->obj_ea)))
			return true;
		if (f.name.compare(0, 7, "msvc9::") != 0 && f.file.compare(0, 13, "src/SDK/MSVC/") != 0)
			return false;
		if (f.provenance == SrcModel::Provenance::Guess && f.file != "src/SDK/MSVC/ehvec.h")
			return false;
		static const std::regex addressed(R"((^|_)sub_[0-9A-Fa-f]+$|_[0-9A-F]{6,8}$)");
		size_t scope = f.name.rfind("::");
		return !std::regex_search(f.name.substr(scope == std::string::npos ? 0 : scope + 2), addressed);
	}

	// The src type of the declaration e is, as it is printed (with no conversion of its own), where it is the original
	// source's, as evidence of what (above): a parameter of the function (slotProven), a local the locals pass paired
	// with the debug build's (its user name the debug local's, or that and `_N` for one of a split, Hex-Rays' type the
	// debug local's), a field (fieldProven) of an object of a proven type (provenObject), a global or static member
	// the PDB types, the result of a named src function (provenCallee). "" for any other value.
	std::string provenDeclared(const cexpr_t* e, Evidence what) const
	{
		const SrcModel::Function* self = m_function.self;
		if (m_function.conversions.count(e))
			return "";
		std::string key;
		if (int v = lvarIndex(e); v >= 0)
		{
			const Local& local = m_function.locals[v];
			if (!self || local.reference || local.storage || local.type.empty())
				return "";
			if (local.parameter >= 0)
			{
				const SrcModel::Type* type = m_model.type(self->type);
				if (!type || static_cast<size_t>(local.parameter) >= type->params.size() ||
					!slotProven(*self, std::to_string(local.parameter), what))
					return "";
				key = type->params[local.parameter];
			}
			else
			{
				const lvar_t& lvar = (*m_function.cfunc->get_lvars())[v];
				const std::string* pdb = lvar.has_user_name() ? pairedLocal(self->mangled, lvar.name.c_str()) : nullptr;
				if (!pdb || !samePdb(local.type, *pdb))
					return "";
				key = local.type;
			}
		}
		else if (const Member* member = e->op == cot_memptr || e->op == cot_memref ? meaningOf<Member>(e) : nullptr)
		{
			if (!member->field || !fieldProven(member->owner, member->field->name, what) ||
				!provenObject(e->x, e->op == cot_memptr))
				return "";
			key = member->field->type;
		}
		// a trivial accessor's load printed as the field it reads (FoldedAccessors), outside any by-value member
		else if (const Access* access = e->op == cot_call ? meaningOf<Access>(e) : nullptr)
		{
			if (!access->load || !access->field || !access->through.empty() ||
				!fieldProven(access->owner, access->field->name, what) || !provenObject(receiverOf(e), true))
				return "";
			key = access->field->type;
		}
		else if (const SrcModel::Ref* const* ref = e->op == cot_obj ? meaningOf<const SrcModel::Ref*>(e) : nullptr)
		{
			if (!m_model.pdbTyped("global", (*ref)->target, ""))
				return "";
			key = (*ref)->type;
		}
		else if (const Call* call = e->op == cot_call ? meaningOf<Call>(e) : nullptr)
		{
			if (call->through || call->resultArgument >= 0 || call->returnsReference)
				return "";
			key = provenCallee(e, *call, "ret", what);
		}
		Printed as = printed(e);
		return !key.empty() && !as.key.empty() && m_types.canonical(as.key) == m_types.canonical(key) ? key : "";
	}

	// the PDB's type of the debug local the locals pass named the function's local after: its name, or the name
	// before a split's `_N`
	const std::string* pairedLocal(const std::string& function, const std::string& name) const
	{
		if (const std::string* type = m_model.pdbTyped("local", function, name))
			return type;
		size_t underscore = name.rfind('_');
		if (underscore == std::string::npos || underscore == 0 || underscore + 1 == name.size() ||
			name.find_first_not_of("0123456789", underscore + 1) != std::string::npos)
			return nullptr;
		return m_model.pdbTyped("local", function, name.substr(0, underscore));
	}

	// The src type of the declaration the value goes to, where it is the original source's (above): a resolved call's
	// parameter (not by reference, no cast of Calls'), the function's return (not a reference or a record by value),
	// an assignment's left side (provenDeclared). "" for any other place.
	std::string provenDestination(const cexpr_t* value) const
	{
		const SrcModel::Function* self = m_function.self;
		auto up = m_parents.find(value);
		if (!self || up == m_parents.end() || !up->second)
			return "";
		const citem_t* parent = up->second;
		if (parent->op == cit_return)
			return !m_function.returnsReference && m_function.resultArgument < 0 &&
						   slotProven(*self, "ret", Evidence::signedness)
					   ? m_function.returnType
					   : "";
		if (parent->op >= cit_empty)
			return "";
		const cexpr_t* p = static_cast<const cexpr_t*>(parent);
		if (p->op == cot_asg && p->y == value)
			return provenDeclared(p->x, Evidence::signedness);
		const Call* call = p->op == cot_call ? meaningOf<Call>(p) : nullptr;
		for (size_t k = 0; call && !call->through && k < p->a->size(); ++k)
		{
			if (&(*p->a)[k] != value)
				continue;
			int parameter = parameterIndex(*call, k);
			if (parameter < 0 || (k < call->castTo.size() && !call->castTo[k].empty()) ||
				(k < call->byReference.size() && call->byReference[k]))
				return "";
			return provenCallee(p, *call, std::to_string(parameter), Evidence::signedness);
		}
		return "";
	}

	// The src type of the resolved callee's parameter (item: its index) or result (`ret`) where it is the original
	// source's as evidence of what (slotProven; or a library's own declaration, libraryDeclared), and is so
	// alike for every src function a direct call's address is (a body, structor or fold there, a forwarder whose
	// function's own body is not elsewhere): a linker-folded address names several, of which Calls' choice need not
	// be the one the original source called. A member's object must print as one of the callee's class (or a class
	// deriving from it at offset 0) without a conversion: one Hex-Rays casts to the class is an object of another,
	// whose member the original source called (a fold src does not know). "" for any other.
	std::string provenCallee(const cexpr_t* e, const Call& call, const std::string& item, Evidence what) const
	{
		if (!call.function->cls.empty() && !call.function->isStatic)
		{
			const cexpr_t* receiver = e->a->empty() ? nullptr : &(*e->a)[0];
			std::optional<PointeeCv> object = receiver ? pointeeOf(m_types, printed(receiver).key) : std::nullopt;
			SrcTypes::Parts record = object ? m_types.parts(object->type) : SrcTypes::Parts{};
			if (!object || record.kind != "record" || m_function.conversions.count(receiver) ||
				(receiver->op == cot_cast && !meaningOf<Implicit>(receiver)) ||
				!m_types.derivesAtZero(record.name, call.function->cls))
				return "";
		}
		const cexpr_t* callee = stripCasts(e->x);
		auto typeOf = [&](const SrcModel::Function& f) -> std::string {
			const SrcModel::Type* type = m_model.type(f.type);
			if (!type || (!slotProven(f, item, what) && !libraryDeclared(f, callee)))
				return "";
			if (item == "ret")
				return type->ret;
			size_t k = std::stoul(item);
			return k < type->params.size() ? type->params[k] : "";
		};
		std::string key = typeOf(*call.function);
		if (key.empty() || callee->op != cot_obj)
			return key;
		for (const SrcModel::Ref& ref : m_model.refsAt(callee->obj_ea))
		{
			static const std::set<std::string> declaring = {"body", "constructor", "destructor", "fold", "forwarder"};
			if (ref.function.empty() || ref.function == call.function->mangled || !declaring.count(ref.kind))
				continue;
			if (ref.kind == "forwarder")
			{
				const std::vector<uint64_t>& own = m_model.bodyAddresses(ref);
				if (!own.empty() && std::find(own.begin(), own.end(), callee->obj_ea) == own.end())
					continue;
			}
			const SrcModel::Function* other = m_model.function(ref.function);
			std::string its = other ? typeOf(*other) : "";
			if (its.empty() || m_types.canonical(its) != m_types.canonical(key))
				return "";
		}
		return key;
	}

	// a builtin integer's size and signedness (char and wchar_t included; not bool); nullopt for any other
	std::optional<std::pair<int64_t, bool>> integerOf(const std::string& key) const
	{
		const SrcModel::Type* type = key.empty() ? nullptr : m_model.type(m_types.canonical(key));
		if (!type || type->kind != "builtin" || type->isFloat || type->size <= 0 || type->name == "bool" ||
			type->name == "void")
			return std::nullopt;
		return std::make_pair(type->size, type->isSigned);
	}

	// a src local's type is the PDB's local's: an integer of the same size and signedness (the PDB's long is src's
	// int32_t), char and wchar_t only as themselves; any other type the same once canonical, spaces aside (the PDB
	// spells a template's arguments `<A *,B>`)
	bool samePdb(const std::string& local, const std::string& pdb) const
	{
		std::string x = m_types.canonical(local), y = m_types.canonical(pdb);
		auto character = [](const std::string& k) { return k == "builtin:char" || k == "builtin:wchar_t"; };
		std::optional<std::pair<int64_t, bool>> a = integerOf(local), b = integerOf(pdb);
		if (!character(x) && !character(y) && (a || b))
			return a == b;
		std::erase(x, ' ');
		std::erase(y, ' ');
		return x == y;
	}

	// The object a member is read in (pointer: e is the pointer to it) is of a proven type: reached through base
	// subobjects, implicit upcasts, `&`, `*` and elements, from `this` of a member the PDB declares for its class, a
	// reference parameter of a proven type (slotProven), a local the locals pass paired with a debug reference to the
	// object (Hex-Rays' pointer to its referent), or a declaration provenDeclared proves.
	bool provenObject(const cexpr_t* e, bool pointer) const
	{
		const SrcModel::Function* self = m_function.self;
		for (int depth = 0; depth < 32 && self; ++depth)
		{
			int v = pointer ? lvarIndex(e) : -1;
			if (e->op == cot_cast && meaningOf<Implicit>(e))
				e = e->x;
			else if (pointer && e->op == cot_ref)
				e = e->x, pointer = false;
			else if (pointer && isThis(m_function, e))
				return slotProven(*self, "this", Evidence::object);
			else if (v >= 0 && m_function.locals[v].reference)
			{
				int parameter = m_function.locals[v].parameter;
				return parameter >= 0 && slotProven(*self, std::to_string(parameter), Evidence::object);
			}
			else if (pointer && !provenDeclared(e, Evidence::object).empty())
				return true;
			else if (v >= 0 && m_function.locals[v].parameter < 0 && !m_function.locals[v].storage &&
					 !m_function.conversions.count(e))
			{
				const lvar_t& lvar = (*m_function.cfunc->get_lvars())[v];
				const std::string* pdb = lvar.has_user_name() ? pairedLocal(self->mangled, lvar.name.c_str()) : nullptr;
				SrcTypes::Parts referenced = pdb ? m_types.parts(*pdb) : SrcTypes::Parts{};
				return referenced.kind == "lref" && samePdb(m_function.locals[v].type, "ptr(" + referenced.of + ")");
			}
			else if (pointer)
				return false;
			else if ((e->op == cot_memptr || e->op == cot_memref) && meaningOf<std::string>(e))
				pointer = e->op == cot_memptr, e = e->x;  // a base subobject
			else if (e->op == cot_ptr)
				e = e->x, pointer = true;
			else if (e->op == cot_idx)
				pointer = e->x->type.is_ptr(), e = e->x;
			else
				return !provenDeclared(e, Evidence::object).empty();
		}
		return false;
	}

	static bool zero(const cexpr_t* e)
	{
		return e && e->op == cot_num && e->numval() == 0;
	}

	// A value that is 0 or 1: a comparison's or a logical operator's, the number 0 or 1, a choice or comma of those,
	// one cast to an integer, enum or bool (which keeps 0 and 1), a bitwise and with the number 1, or a local holding
	// only such values (truthLocal)
	bool truthValue(const cexpr_t* e) const
	{
		std::set<int> visiting;
		return truthValue(e, visiting);
	}

	bool truthValue(const cexpr_t* e, std::set<int>& visiting) const
	{
		if (!e)
			return false;
		if (comparison(e->op) || e->op == cot_lnot || e->op == cot_land || e->op == cot_lor)
			return true;
		if (e->op == cot_num)
			return e->numval() <= 1;
		if (e->op == cot_tern)
			return truthValue(e->y, visiting) && truthValue(e->z, visiting);
		if (e->op == cot_comma)
			return truthValue(e->y, visiting);
		if (e->op == cot_cast && (e->type.is_integral() || e->type.is_enum() || e->type.is_bool()))
			return truthValue(e->x, visiting);
		if (e->op == cot_band)
			return (e->x->op == cot_num && e->x->numval() == 1) || (e->y->op == cot_num && e->y->numval() == 1);
		if (e->op == cot_var)
			return truthLocal(e->v.idx, visiting);
		return false;
	}

	// A local (not a parameter, not one Hex-Rays or markUndefined reports possibly undefined) whose every write is an
	// assignment of a truth value: its address never taken, never stepped, compound-assigned or written in part (a
	// helper over it, a member of it). Cycles of locals assigned each other prove nothing.
	bool truthLocal(int k, std::set<int>& visiting) const
	{
		const lvars_t& lvars = *m_function.cfunc->get_lvars();
		if (k < 0 || static_cast<size_t>(k) >= lvars.size() || lvars[k].is_arg_var() ||
			(static_cast<size_t>(k) < m_function.locals.size() && m_function.locals[k].undefined) ||
			!visiting.insert(k).second)
			return false;
		bool truth = true, written = false;
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			if (!truth || lvarIndex(e) != k)
				return;
			auto up = m_parents.find(e);
			if (up == m_parents.end() || !up->second->is_expr())
				return;
			const cexpr_t* parent = static_cast<const cexpr_t*>(up->second);
			if (parent->op == cot_asg && parent->x == e)
			{
				written = true;
				truth = truthValue(parent->y, visiting);
			}
			else if (((is_assignment(parent->op) || is_prepost(parent->op)) && parent->x == e) ||
					 parent->op == cot_ref || parent->op == cot_memref ||
					 (parent->op == cot_call && parent->x->op == cot_helper))
				truth = false;
		});
		visiting.erase(k);
		return truth && written;
	}

	std::string describe(const Printed& p) const
	{
		return p.key.empty() ? "IDA's " + text(p.ida) : p.key;
	}

	Conversion make(Kind kind, const Printed& to, const Printed& from, const std::string& what) const
	{
		Conversion c;
		c.kind = kind;
		c.type = to.key;
		c.ida = to.ida;
		c.what = what + " " + describe(from) + " to " + describe(to);
		return c;
	}

	// --- the overlay

	template <class T>
	const T* meaningOf(const citem_t* item) const
	{
		auto found = m_function.meaning.find(item);
		return found == m_function.meaning.end() ? nullptr : std::get_if<T>(&found->second);
	}

	// one conversion per value: a later site's keeps an earlier one's, except in place of a cast (from its operand)
	void record(const cexpr_t* v, const Conversion& c)
	{
		auto found = m_function.conversions.find(v);
		if (found == m_function.conversions.end())
			m_function.conversions.emplace(v, c);
	}

	void replace(const cexpr_t* cast, Conversion c)
	{
		c.replacesCast = true;
		m_function.conversions[cast] = c;
	}

	Function& m_function;
	Context& m_context;
	SrcTypes& m_types;
	const SrcModel& m_model;
	std::unordered_map<const cexpr_t*, const citem_t*> m_parents;
	std::set<const cinsn_t*> m_skipped;	 // statements not printed (aliases' copies, what idioms drop)
	bool m_constThis = false;			 // a const member: its `this` points to const
};

}  // namespace

void convertValues(Function& function, Context& context)
{
	Conversions(function, context).run();
}

}  // namespace idakit
