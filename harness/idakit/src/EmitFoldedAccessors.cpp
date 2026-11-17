// Folded accessors: a call of a trivial accessor (exefacts' trivialAccessor) that Calls cannot make on its receiver
// prints as its access on the receiver's src class (`node->m_pkNext`, `effect->data.iRange`; the receiver itself for
// `return this`; `form->iFormFlags = flags` for a setter), which the porting rules let src write for any trivial
// accessor's call. Calls cannot make it where src names several functions at the address (Calls' choice,
// chosenCallee: a linker fold src registers no body for), or where the src function Calls chose is a member of a
// class the receiver's src class does not derive from at offset 0 (a fold whose owner is another class, or an
// address src declares once in another class). A fold's call Calls makes on a fitting receiver (a `Same function
// as` fold ref names the address) prints as the receiver's own field too, not one inside a named member. Any other
// call is left to Calls. The field's name must find that member alone from each class the printer may read it
// through (a base subobject's objects included).
//
// It runs before Fields: the access's field is what fields read through the call resolve against. Types a later layer
// gives the receiver or a setter's value (Const's const locals, a call's or global's src type) are judged there:
// Const and Conversions read the access as the member access it prints as.

#include "EmitPasses.hpp"

#include "idakit/SrcIda.hpp"

#include <algorithm>

namespace idakit
{

namespace
{

// the object argument without Hex-Rays' casts
const cexpr_t* receiverOf(const cexpr_t* call)
{
	const cexpr_t* e = &(*call->a)[0];
	while (e->op == cot_cast)
		e = e->x;
	return e;
}

// how a value's bits are read: a number's bits as an integer or a floating value, a pointer's, or an aggregate's (a
// record, an array, a vector), which no register holds
enum class Shape
{
	integer,
	floating,
	pointer,
	aggregate,
};

Shape shapeOf(const SrcTypes& types, const std::string& key)
{
	std::string canonical = types.canonical(key);
	std::string kind = types.parts(canonical).kind;
	if (canonical == "builtin:float" || canonical == "builtin:double" || canonical == "builtin:long double")
		return Shape::floating;
	if (kind == "ptr")
		return Shape::pointer;
	return kind == "builtin" || kind == "enum" ? Shape::integer : Shape::aggregate;
}

Shape shapeOf(const tinfo_t& type)
{
	if (type.is_floating())
		return Shape::floating;
	if (type.is_ptr())
		return Shape::pointer;
	return type.is_integral() || type.is_enum() || type.is_bool() ? Shape::integer : Shape::aggregate;
}

// a field found at an offset of a record: the field, the named by-value members it lies in (outermost first), and the
// named record declaring the outermost of them (the field itself where there is none)
struct Found
{
	const SrcModel::Field* field = nullptr;
	std::vector<const SrcModel::Field*> through;
	std::string owner;
};

class FoldedAccessors
{
public:
	FoldedAccessors(Function& function, Context& context)
		: m_function(function), m_context(context), m_types(context.types), m_parents(parentsOf(function))
	{
	}

	void run()
	{
		std::vector<const cexpr_t*> calls;
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			if (e->op == cot_call && e->x->op == cot_obj && !e->a->empty())
				calls.push_back(e);
		});
		for (const cexpr_t* call : calls)
			resolve(call);
	}

private:
	// the address is a linker fold: a fold ref names it, or src names it several times with no registered body
	bool folded(ea_t ea) const
	{
		const std::vector<SrcModel::Ref>& refs = m_context.model.refsAt(ea);
		if (std::any_of(refs.begin(), refs.end(), [](const SrcModel::Ref& ref) { return ref.kind == "fold"; }))
			return true;
		RefChoice choice = chooseRef(m_context.model, ea);
		return choice.names.size() > 1 && !choice.ref;
	}

	// Calls cannot make the call on its receiver: src names several functions there (Calls' choice, chosenCallee: a
	// user-defined call naming several, a linker fold src names several times with no registered body), or the src
	// function Calls chose is a non-static member of a class the receiver's src record (record; "" for none) does not
	// derive from at offset 0
	bool uncallable(const cexpr_t* call, const std::string& record) const
	{
		ChosenCallee chosen = chosenCallee(m_function, m_context.model, call);
		if (chosen.several)
			return true;
		const SrcModel::Function* f = chosen.function;
		return f && !f->cls.empty() && !f->isStatic && (record.empty() || !m_types.derivesAtZero(record, f->cls));
	}

	// the receiver's src type is evidence, not IDA's guess: the body's own parameter (`this` included) as its src
	// declaration types it, never reassigned
	bool srcTyped(const cexpr_t* object) const
	{
		int v = lvarIndex(object);
		return v >= 0 && m_function.defines && m_function.cfunc->get_lvars()->at(v).is_arg_var() &&
			   !m_function.locals[v].entryCopy;
	}

	// the record a pointer key points to ("" for any other key)
	std::string pointee(const std::string& key) const
	{
		SrcTypes::Parts p = m_types.parts(m_types.canonical(key));
		return p.kind == "ptr" && m_types.parts(p.of).kind == "record" ? m_types.parts(p.of).name : "";
	}

	// whether an integer key is signed (an enum by its underlying type); nullopt for any other key
	std::optional<bool> signedness(const std::string& key) const
	{
		std::string canonical = m_types.canonical(key);
		if (SrcTypes::Parts p = m_types.parts(canonical); p.kind == "enum")
		{
			const SrcModel::Enum* e = m_context.model.enumeration(p.name);
			if (!e)
				return std::nullopt;
			canonical = m_types.canonical(e->underlying);
		}
		const SrcModel::Type* type = m_context.model.type(canonical);
		if (!type || type->kind != "builtin" || type->isFloat || type->size == 0)
			return std::nullopt;
		return type->isSigned;
	}

	// the signedness of IDA's integer type as src keys it; nullopt for another type
	std::optional<bool> signedness(const tinfo_t& type) const
	{
		std::string why;
		std::optional<std::string> key = m_types.keyOf(type, why);
		return key ? signedness(*key) : std::nullopt;
	}

	// the record's field at the byte offset that accept takes (through its bases, its anonymous members, whose fields
	// are the record's own: `bound.m_fRadius`, and its named by-value members and their bases: `data.iRange`,
	// `CombatGroups.m_usSize` of a NiTPrimitiveArray member whose NiTArray base declares it); field null for none
	template <class Accept>
	Found memberAt(const std::string& record, int64_t offset, const Accept& accept) const
	{
		const SrcModel::Record* r = m_context.model.record(record);
		if (!r)
			return {};
		if (Found m = memberIn(*r, record, offset * 8, accept); m.field)
			return m;
		for (const SrcModel::Base& b : r->bases)
			if (!b.isVirtual && b.offset <= offset)
				if (Found m = memberAt(b.record, offset - b.offset, accept); m.field)
					return m;
		return {};
	}

	// the field of r's own at the bit offset that accept takes, declared by owner: one of its anonymous members', or
	// one inside a named by-value member (its own, or its bases') that accept does not take itself
	template <class Accept>
	Found memberIn(const SrcModel::Record& r, const std::string& owner, int64_t offsetBits, const Accept& accept) const
	{
		for (const SrcModel::Field& f : r.fields)
		{
			if (f.bits || f.offsetBits > offsetBits)
				continue;
			const SrcModel::Type* type = m_context.model.type(m_types.canonical(f.type));
			const SrcModel::Record* inner =
				type && type->kind == "record" ? m_context.model.record(type->name) : nullptr;
			if (f.name.empty() && inner && unnamedRecord(inner->name))
			{
				if (Found m = memberIn(*inner, owner, offsetBits - f.offsetBits, accept); m.field)
					return m;
				continue;
			}
			if (f.offsetBits == offsetBits && accept(f))
				return {&f, {}, owner};
			if (inner && !f.name.empty() && offsetBits - f.offsetBits < int64_t(inner->size) * 8)
				if (Found m = memberAt(inner->name, (offsetBits - f.offsetBits) / 8, accept); m.field)
				{
					m.through.insert(m.through.begin(), &f);
					m.owner = owner;
					return m;
				}
		}
		return {};
	}

	// How the access's outermost name is spelled so that name lookup finds the owner's member: "" where it finds that
	// member alone (C++'s lookup: the class declaring a field or member function of the name, else each base's
	// lookup) from each class the printer may look it up in (lookupClasses); the owner, as the qualifier, where a
	// class of the receiver's hides it by another member of the name and the receiver is read as its own class only
	// (`effect->BSTempEffect::fAge` beside MagicHitEffect's own fAge, as Fields qualifies a field: the owner a base
	// the receiver's class reaches along one path); nullopt where neither reads the owner's member.
	std::optional<std::string> spelling(const cexpr_t* object, const std::string& record, const Found& found) const
	{
		const std::string& name = found.through.empty() ? found.field->name : found.through.front()->name;
		std::vector<std::string> classes = lookupClasses(object, record);
		if (std::all_of(classes.begin(), classes.end(), [&](const std::string& from) {
				std::vector<std::string> declaring = nameLookup(m_context, from, name);
				return declaring.size() == 1 && declaring.front() == found.owner;
			}))
			return "";
		if (classes.size() == 1 && found.owner != record && basePaths(m_context.model, record, found.owner) == 1)
			return found.owner;
		return std::nullopt;
	}

	// The classes C++ may look the access's name up in as the printer prints the receiver (EmitPrinter's memberOf):
	// the receiver's src record, and for the address of a base subobject (`&p->Base`, `&v.Base`: IDA's member a base
	// class, which Fields makes src's base subobject), each object it is a base subobject of, out to the outermost,
	// through which the printer reads the name (`p->name`).
	std::vector<std::string> lookupClasses(const cexpr_t* object, const std::string& record) const
	{
		std::vector<std::string> classes{record};
		if (object->op != cot_ref)
			return classes;
		for (const cexpr_t* b = object->x; (b->op == cot_memptr || b->op == cot_memref) && baseMember(b); b = b->x)
		{
			std::string outer = b->op == cot_memptr ? pointee(receiverType(m_function, m_context, b->x))
													: pointee("ptr(" + srcType(m_function, m_context, b->x) + ")");
			if (outer.empty())
				break;
			classes.push_back(outer);
			if (b->op == cot_memptr)
				break;
		}
		return classes;
	}

	// IDA's member a memptr/memref names is a base class of its struct
	static bool baseMember(const cexpr_t* b)
	{
		tinfo_t owner = b->op == cot_memptr ? b->x->type.get_pointed_object() : b->x->type;
		udm_t member;
		return !owner.is_union() && owner.get_udm_by_offset(&member, uint64(b->m) * 8) >= 0 &&
			   member.offset == uint64(b->m) * 8 && member.is_baseclass();
	}

	void resolve(const cexpr_t* call)
	{
		std::optional<AccessorCall> accessor = accessorCall(m_context, call);
		if (!accessor || accessor->receiver != &(*call->a)[0])
			return;
		const cexpr_t* object = receiverOf(call);
		std::string receiver = receiverType(m_function, m_context, object);
		std::string record = pointee(receiver);
		// a call Calls makes on a fitting receiver stays Calls', a fold's aside: it prints as the receiver's own field
		// (not one inside a named member: that stays the call)
		bool callable = !uncallable(call, record);
		if (callable && !folded(call->x->obj_ea))
			return;
		bool constReceiver = m_types.constPointee(receiver);
		if (accessor->value)
		{
			store(call, *accessor, record, constReceiver, callable);
			return;
		}
		// a trivial accessor of `this`: the access itself (not an address in a const receiver: the exe's result is
		// not const; a load only of a scalar field as wide as the load, floating where the exe loads it into ST0, of
		// the kind its use is, pointer or not, unless the receiver's src type is evidence (srcTyped: the use's type
		// is then Conversions' to judge): another is not the field the exe reads (a union offers several), the
		// receiver's type being IDA's guess). A narrower load than a dword only where the caller reads it at that
		// width (eax's upper bytes are the callee's otherwise, porting rules "Return widths") or the callee extends it
		// (movzx, movsx) to the dword the caller reads, of a field C++ extends as the exe does: as signed as the
		// caller's type, or the callee's extension; of any signedness where the caller reads it at that width only to
		// test it for zero (truthTested), which no extension changes. `return this` is the receiver itself whatever
		// its type (porting rules "Calls").
		bool returnsThis = !accessor->load && accessor->offset == 0;
		if ((record.empty() && !returnsThis) || (!accessor->load && accessor->offset != 0 && constReceiver))
			return;
		std::optional<bool> used = usedAsPointer(call);
		size_t read = call->type.get_size();
		bool narrow = accessor->load && !accessor->x87 && accessor->width < 4;
		bool tested = narrow && read == accessor->width && truthTested(call, m_parents);
		std::optional<bool> sign = !narrow						   ? std::nullopt
								   : read == accessor->width		   ? signedness(call->type)
								   : read == 4 && accessor->extension ? std::optional<bool>(accessor->extension < 0)
																	  : std::nullopt;
		auto loaded = [&](const SrcModel::Field& f, bool useKind) {
			Shape shape = shapeOf(m_types, f.type);
			return shape != Shape::aggregate &&
				   m_context.model.sizeOf(f.type) == std::optional<int64_t>(accessor->width) &&
				   (shape == Shape::floating) == accessor->x87 &&
				   !(useKind && used && *used != (shape == Shape::pointer)) &&
				   (!narrow || tested || signedness(f.type) == sign);
		};
		Found found;
		if (!accessor->load && accessor->offset != 0)
			found = memberAt(record, accessor->offset, [](const SrcModel::Field&) { return true; });
		else if (accessor->load && (!narrow || sign || tested))
		{
			found = memberAt(record, accessor->offset, [&](const SrcModel::Field& f) { return loaded(f, true); });
			if (!found.field && srcTyped(object))
				found = memberAt(record, accessor->offset, [&](const SrcModel::Field& f) { return loaded(f, false); });
		}
		std::optional<std::string> qualifier =
			found.field ? spelling(object, record, found) : std::optional<std::string>("");
		if (found.field && (!qualifier || (callable && !found.through.empty())))
			return;
		if (!found.field && !returnsThis)
			return;
		m_function.meaning[call] =
			Access{accessor->load, found.field, nullptr, found.through, found.owner, *qualifier};
		implicitReceiver(call);
		if (!found.field)
			firstField(call, record);
	}

	// A trivial setter of the receiver, called as a statement of its own (its eax is the body's leftover): the store of
	// its argument into the receiver's field at the offset (not in a const receiver), the field as wide as the store,
	// the argument one C++ assigns as the same bytes (assignsBytes), without Hex-Rays' casts between pointers and
	// between a pointer and a dword integer (the exe stores the same dword either way); where that is the `void*`
	// result of a call whose src declaration returns `void*` and is no such argument, Hex-Rays' cast of it to a
	// pointer, which C++ writes as a static_cast (`data.pLight = static_cast<TESObjectLIGH*>(__RTDynamicCast(...))`,
	// DynamicCasts' form). Not a function's address: how it prints is not known before Globals names it (unnamed, as
	// a byte pointer)
	void store(const cexpr_t* call, const AccessorCall& accessor, const std::string& record, bool constReceiver,
			   bool callable)
	{
		auto parent = m_parents.find(call);
		if (record.empty() || constReceiver || parent == m_parents.end() || parent->second->op != cit_expr)
			return;
		auto dword = [](const tinfo_t& t) { return t.is_ptr() || (t.is_integral() && t.get_size() == 4); };
		// the argument without those casts, then, where that is the `void*` result of a call whose src declaration
		// returns `void*`, the argument with the cast directly over it (C++'s static_cast from void*)
		std::vector<const cexpr_t*> values{accessor.value};
		while (values.back()->op == cot_cast && dword(values.back()->type) && dword(values.back()->x->type) &&
			   (values.back()->type.is_ptr() || values.back()->x->type.is_ptr()))
			values.push_back(values.back()->x);
		if (functionStart(values.back()))
			return;
		std::vector<const cexpr_t*> candidates{values.back()};
		if (values.size() >= 2 && returnsUntyped(values.back()))
			candidates.push_back(values[values.size() - 2]);
		for (const cexpr_t* value : candidates)
		{
			Found found = memberAt(record, accessor.offset, [&](const SrcModel::Field& f) {
				return m_context.model.sizeOf(f.type) == std::optional<int64_t>(accessor.width) &&
					   assignsBytes(f.type, accessor.width, value);
			});
			if (!found.field)
				continue;
			std::optional<std::string> qualifier = spelling(receiverOf(call), record, found);
			if (!qualifier || (callable && !found.through.empty()))
				return;
			m_function.meaning[call] = Access{false, found.field, value, found.through, found.owner, *qualifier};
			implicitReceiver(call);
			return;
		}
	}

	// a direct call of a function whose src declaration returns `void*` (`__RTDynamicCast`)
	bool returnsUntyped(const cexpr_t* e) const
	{
		if (e->op != cot_call || e->x->op != cot_obj)
			return false;
		RefChoice choice = chooseRef(m_context.model, e->x->obj_ea);
		const SrcModel::Function* f = choice.ref ? m_context.model.function(choice.ref->function) : nullptr;
		const SrcModel::Type* type = f ? m_context.model.type(f->type) : nullptr;
		return type && m_types.canonical(type->ret) == "ptr(builtin:void)";
	}

	// `field = value` stores the low width bytes of the value the exe passes: a value of the field's type; a pointer
	// to a class at offset 0 of the field's pointee (any pointer to non-const for a void*), not dropping const; an
	// integer at least as wide for an integer field (bool aside: C++ tests it); a number the field holds as it is (0
	// for a pointer, 0 or 1 for a bool, any for an integer, one of its enumerators' values for an enum, which Enums
	// prints as that enumerator)
	bool assignsBytes(const std::string& fieldType, uint8_t width, const cexpr_t* value) const
	{
		std::string to = m_types.canonical(fieldType);
		SrcTypes::Parts target = m_types.parts(to);
		auto integer = [&](const std::string& key) {
			return m_types.parts(key).kind == "builtin" && key != "builtin:bool" && key != "builtin:float" &&
				   key != "builtin:double" && key != "builtin:void";
		};
		if (value->op == cot_num)
		{
			uint64_t n = value->numval();
			if (target.kind == "enum")
			{
				const SrcModel::Enum* e = m_context.model.enumeration(target.name);
				uint64_t mask = width >= 8 ? ~0ull : (1ull << (width * 8)) - 1;
				return e && std::any_of(e->enumerators.begin(), e->enumerators.end(),
										[&](const SrcModel::Enumerator& k) {
											return (static_cast<uint64_t>(k.value) & mask) == (n & mask);
										});
			}
			return target.kind == "ptr" ? n == 0 : to == "builtin:bool" ? n <= 1 : integer(to);
		}
		std::string type = srcType(m_function, m_context, value);
		std::string from = type.empty() ? "" : m_types.canonical(type);
		if (from.empty())
			return false;
		if (from == to)
			return !m_types.constPointee(type) || m_types.constPointee(fieldType);
		SrcTypes::Parts source = m_types.parts(from);
		if (target.kind == "ptr" && source.kind == "ptr")
		{
			if (m_types.constPointee(type) && !m_types.constPointee(fieldType))
				return false;
			std::string derived = pointee(from), base = pointee(to);
			return target.of == "builtin:void" ||
				   (!derived.empty() && !base.empty() && m_types.derivesAtZero(derived, base));
		}
		std::optional<int64_t> size = m_context.model.sizeOf(from);
		return integer(to) && integer(from) && size && *size >= width;
	}

	// whether the call's result is used as a pointer: by the other side of the assignment or comparison it is in, else
	// by the type it is cast to; nullopt where either fits (compared with 0)
	std::optional<bool> usedAsPointer(const cexpr_t* call) const
	{
		const cexpr_t* e = call;
		auto parent = m_parents.find(e);
		for (; parent != m_parents.end() && parent->second->op == cot_cast; parent = m_parents.find(e))
			e = static_cast<const cexpr_t*>(parent->second);
		const citem_t* p = parent == m_parents.end() ? nullptr : parent->second;
		if (!p || !(p->op == cot_asg || p->op == cot_eq || p->op == cot_ne || (p->op >= cot_sge && p->op <= cot_ult)))
			return e->type.is_ptr();
		const cexpr_t* binary = static_cast<const cexpr_t*>(p);
		const cexpr_t* other = binary->x == e ? binary->y : binary->x;
		while (other->op == cot_cast)
			other = other->x;
		if (other->op == cot_num)
			return other->n->_value == 0 ? std::nullopt : std::optional<bool>(false);
		return other->type.is_ptr();
	}

	// `*(T*)fold(receiver)` of a `return this` fold: the read of the receiver's own field at 0 (`node->m_item`), a
	// field of the read's size and shape, a pointer standing for an integer and the reverse (C++ converts those two
	// only explicitly, Conversions' judgement; a floating field read as an integer would convert as a value), an
	// integer narrower than a dword as signed as the read (C++ extends it by the field's type), Hex-Rays' cast between
	// going
	void firstField(const cexpr_t* call, const std::string& record)
	{
		const cexpr_t* e = call;
		std::vector<const cexpr_t*> casts;
		for (auto parent = m_parents.find(e); parent != m_parents.end() && parent->second->op == cot_cast;
			 parent = m_parents.find(e))
			casts.push_back(e = static_cast<const cexpr_t*>(parent->second));
		auto deref = m_parents.find(e);
		if (deref == m_parents.end() || deref->second->op != cot_ptr)
			return;
		const cexpr_t* read = static_cast<const cexpr_t*>(deref->second);
		auto bits = [](Shape shape) { return shape == Shape::pointer ? Shape::integer : shape; };
		bool narrow = shapeOf(read->type) == Shape::integer && read->type.get_size() < 4;
		auto same = [&](const SrcModel::Field& f) {
			return m_context.model.sizeOf(f.type) == static_cast<int64_t>(read->type.get_size()) &&
				   bits(shapeOf(m_types, f.type)) == bits(shapeOf(read->type)) &&
				   (!narrow || signedness(f.type) == signedness(read->type));
		};
		Found first = memberAt(record, 0, same);
		if (!first.field || !first.through.empty() || spelling(receiverOf(call), record, first) != "")
			return;
		m_function.meaning[read] = Member{first.field, first.owner, ""};
		for (const cexpr_t* cast : casts)
			m_function.meaning[cast] = Implicit{};
	}

	// Hex-Rays' casts of the receiver to the class of the name it shows go, and a cast of the result back to the type
	// the access gives it. Any other cast of the result keeps its src key: the Conversions pass judges it from the
	// access's type (an upcast at offset 0 or a void* to T* converts, an unrelated pointer reinterprets)
	void implicitReceiver(const cexpr_t* call)
	{
		for (const cexpr_t* e = &(*call->a)[0]; e->op == cot_cast; e = e->x)
			m_function.meaning[e] = Implicit{};
		auto parent = m_parents.find(call);
		if (parent == m_parents.end() || parent->second->op != cot_cast)
			return;
		const cexpr_t* cast = static_cast<const cexpr_t*>(parent->second);
		std::string why, result = srcType(m_function, m_context, call);
		std::optional<std::string> key = m_types.keyOf(cast->type, why);
		if (key && !result.empty() && m_types.canonical(*key) == m_types.canonical(result))
			m_function.meaning[cast] = Implicit{};
	}

	Function& m_function;
	Context& m_context;
	SrcTypes& m_types;
	std::unordered_map<const cexpr_t*, const citem_t*> m_parents;
};

}  // namespace

void resolveFoldedAccessors(Function& function, Context& context)
{
	FoldedAccessors(function, context).run();
}

}  // namespace idakit
