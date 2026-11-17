// Fields: each memptr (`p->m`) and memref (`x.m`) as the src field of the operand's record at exactly that offset and
// size, of the type Hex-Rays reads. A base's fields count at the base's offset; an anonymous member's fields are the
// record's own. IDA's member for a base class is the src base subobject at that offset (its record's key); a field
// read through base subobjects is the object's member (`p->field`), qualified by the class declaring it
// (`p->Base::field`) where name lookup from the object's class finds another member of that name. An integer member
// of a union the code converts to or from a pointer is the union's pointer member (`u.pData = p`). A lane of a compiler
// vector (IDA's `v.m128_f32[i]`) is the vector subscripted (`v[i]`). A record's pointer cast to a pointer to its member
// at offset 0 is that member's address (`&p->m`), a record's value cast to that member's type is the member
// (`v.m_quad`), a record assigned a value of that type cast to it is the member's assignment (`v.m_quad = q`), and
// `*p` through a record's pointer reading that member's type is the member (`p->m`).
// Pointer arithmetic by a constant from a record's pointer or past a field's end is the field at that byte offset
// (`*((float*)p + 4)` is `p->m_f`), else marked past a field.

#include "EmitPasses.hpp"

#include "IdaSupport.hpp"

#include "idakit/SrcIda.hpp"

#include <algorithm>

namespace idakit
{

// the record has base at that offset (directly or through its bases)
bool baseAt(const SrcModel& model, const SrcModel::Record& record, const std::string& base, int64_t offset)
{
	for (const SrcModel::Base& b : record.bases)
	{
		if (b.isVirtual || b.offset > offset)
			continue;
		if (b.record == base && b.offset == offset)
			return true;
		if (const SrcModel::Record* r = model.record(b.record); r && baseAt(model, *r, base, offset - b.offset))
			return true;
	}
	return false;
}

int basePaths(const SrcModel& model, const std::string& record, const std::string& base)
{
	if (record == base)
		return 1;
	const SrcModel::Record* r = model.record(record);
	int n = 0;
	for (const SrcModel::Base& b : r ? r->bases : std::vector<SrcModel::Base>{})
		n += basePaths(model, b.record, base);
	return n;
}

namespace
{

struct Placed
{
	const SrcModel::Field* field;
	std::string owner;  // the named record declaring it
	int64_t offsetBits;
};

// every field reachable by name from the record (its own, its bases', its anonymous members'), at its offset
void fieldsOf(const SrcModel& model, const SrcModel::Record& record, const std::string& owner, int64_t offsetBits,
			  std::vector<Placed>& out)
{
	for (const SrcModel::Field& f : record.fields)
	{
		const SrcModel::Type* type = f.name.empty() && !f.bits ? model.type(f.type) : nullptr;
		const SrcModel::Record* inner = type && type->kind == "record" ? model.record(type->name) : nullptr;
		if (inner && unnamedRecord(inner->name))
			fieldsOf(model, *inner, owner, offsetBits + f.offsetBits, out);
		else
			out.push_back({&f, owner, offsetBits + f.offsetBits});
	}
	for (const SrcModel::Base& base : record.bases)
		if (const SrcModel::Record* b = model.record(base.record); b && !base.isVirtual)
			fieldsOf(model, *b, b->name, offsetBits + base.offset * 8, out);
}

std::string hexOffset(int64_t value)
{
	char text[32];
	std::snprintf(text, sizeof text, "0x%llX", static_cast<long long>(value));
	return text;
}

// the type of the object a memptr/memref reads from
tinfo_t ownerType(const cexpr_t* e)
{
	return e->op == cot_memptr ? e->x->type.get_pointed_object() : e->x->type;
}

class Fields
{
public:
	explicit Fields(Context& context) : m_context(context), m_types(context.types) {}

	// IDA's member at the item is a base class, read whole: the src base subobject's key, or what is missing;
	// nullopt for any other member; through: the record the object is in src (a folded accessor's access)
	std::optional<Meaning> base(const cexpr_t* e, const SrcModel::Record* through = nullptr)
	{
		tinfo_t owner = ownerType(e);
		udm_t member;
		uint64 offsetBits = uint64(e->m) * 8;
		if (owner.is_union() || owner.get_udm_by_offset(&member, offsetBits) < 0 || member.offset != offsetBits ||
			!member.is_baseclass() || !e->type.is_udt() || member.size != e->type.get_size() * 8)
			return std::nullopt;
		const SrcModel::Record* record = through ? through : m_types.recordOf(owner);
		const SrcModel::Record* sub = m_types.recordOf(member.type);
		if (record && sub && baseAt(m_context.model, *record, sub->name, int64_t(e->m)))
			return Meaning("record:" + sub->name);
		return Meaning(Missing{"no src base " + text(member.type) + " of " + text(owner) + " at " + hexOffset(e->m)});
	}

	// the field, or what is missing; through: the record the object is in src where IDA's type says otherwise (a
	// folded accessor's access, the FoldedAccessors pass's), whose field is read whatever type Hex-Rays reads, if a
	// scalar of its kind and signedness (at)
	Meaning field(const cexpr_t* e, const SrcModel::Record* through = nullptr)
	{
		tinfo_t owner = ownerType(e);
		const SrcModel::Record* record = through ? through : m_types.recordOf(owner);
		// an IDA union is src's where src has its record: the member of the type Hex-Rays reads (at)
		if (owner.is_union() && !record)
			return Missing{"union member of IDA's " + text(owner)};
		if (!record)
			return Missing{"no src record for IDA's " + text(owner)};
		// an array member reads as the array (Hex-Rays types the expression as the pointer it decays to)
		tinfo_t held = memberType(e);
		if (!held.is_array() || e->type.is_array())
			held = e->type;
		// a union's member number is no offset: every member is at 0
		int64_t offset = owner.is_union() ? 0 : int64_t(e->m);
		if (through)
			return at(*record, offset, held, true, nullptr);
		const cexpr_t* object = e;
		while ((object->x->op == cot_memptr || object->x->op == cot_memref) && base(object->x))
			object = object->x;
		return at(*record, offset, held, false, m_types.recordOf(ownerType(object)));
	}

	// the field of the record at the byte offset, of the type Hex-Rays reads there (held; anyType: whatever it is,
	// but a scalar of the read's kind and signedness where held is a scalar), else what is missing; object: the class
	// of the object it is read from, whose lookup of the field's name may find another member (nullptr: not known;
	// the field is qualified by the class declaring it)
	Meaning at(const SrcModel::Record& record, int64_t offset, const tinfo_t& held, bool anyType,
			   const SrcModel::Record* object)
	{
		int64_t offsetBits = offset * 8;
		int64_t size = static_cast<int64_t>(held.get_size());
		std::string where = record.name + "+" + hexOffset(offset) + " (" + std::to_string(size) + " bytes)";
		std::string why;
		std::optional<std::string> read = m_types.keyOf(held, why);
		std::vector<Placed> placed;
		fieldsOf(m_context.model, record, record.name, 0, placed);
		// the fields there of that size (several in a union): the one of the type Hex-Rays reads
		const Placed* found = nullptr;
		for (const Placed& p : placed)
			if (p.offsetBits == offsetBits && !p.field->bits && !p.field->name.empty() &&
				m_context.model.sizeOf(p.field->type) == size &&
				(!found || (read && m_types.canonical(p.field->type) == m_types.canonical(*read))))
				found = &p;
		if (!found)
			return Missing{"no src field at " + where};
		if (anyType)
		{
			// C++ reads the field with its own type: a float's bytes are no integer's value, a pointer moves by its
			// pointee's size, and an integer's signedness decides its extension, compares, shifts and divisions
			if (!readsAlike(found->field->type, held))
				return Missing{"src field " + found->owner + "::" + found->field->name + " is " +
							   found->field->type + ", Hex-Rays reads " + text(held)};
			return Member{found->field, found->owner, ""};
		}
		if (!read)
			return Missing{"field " + found->field->name + " read as " + why};
		// a function pointer whose references Hex-Rays reads as pointers is src's field: calls and stores through it
		// are checked against src's type
		std::string declared = m_types.canonical(found->field->type), as = m_types.canonical(*read);
		if (declared != as && !functionReferencesAsPointers(declared, as))
			return Missing{"src field " + found->owner + "::" + found->field->name + " is " + found->field->type +
						   ", Hex-Rays reads " + *read};
		return Member{found->field, found->owner, qualifier(object, *found)};
	}

	// the field's name as read from the object's class: qualified by the class declaring it where lookup finds another
	std::string named(const SrcModel::Record& object, const Member& member)
	{
		std::string by = qualifier(&object, Placed{member.field, member.owner, 0});
		return (by.empty() ? "" : by + "::") + member.field->name;
	}

	// a field of the type reads the bytes as held does: of held's kind (floating, pointer, integer, the integer's
	// signedness; an enum by its underlying type) where held is a scalar; where it is an object (a struct, union or
	// array, whose members the body reads next), an object too, not a scalar C++ has no members of
	bool readsAlike(const std::string& type, const tinfo_t& held) const
	{
		if (!held.is_ptr() && !held.is_floating() && !held.is_integral() && !held.is_enum() && !held.is_bool())
		{
			SrcTypes::Parts f = m_types.parts(m_types.canonical(type));
			if (f.kind == "const" || f.kind == "volatile")
				f = m_types.parts(m_types.canonical(f.of));
			return !held.is_udt() || f.kind == "record" || f.kind == "array" || f.kind == "vector";
		}
		std::string key = m_types.canonical(type);
		SrcTypes::Parts p = m_types.parts(key);
		if (p.kind == "ptr")
			return held.is_ptr();
		if (p.kind == "enum")
		{
			const SrcModel::Enum* e = m_context.model.enumeration(p.name);
			if (!e || e->underlying.empty())
				return false;
			key = m_types.canonical(e->underlying);
			p = m_types.parts(key);
		}
		const SrcModel::Type* t = p.kind == "builtin" ? m_context.model.type(key) : nullptr;
		if (!t || held.is_ptr())
			return false;
		if (t->isFloat || held.is_floating())
			return t->isFloat && held.is_floating();
		// bool is unsigned in both
		bool heldSigned = !held.is_bool() && held.is_signed();
		return t->isSigned == heldSigned;
	}

private:
	// the class the field is read through when lookup from the object's class finds another member of its name
	std::string qualifier(const SrcModel::Record* object, const Placed& found)
	{
		if (!object)
			return found.owner;
		std::vector<Placed> placed;
		fieldsOf(m_context.model, *object, object->name, 0, placed);
		for (const Placed& p : placed)
			if (p.field->name == found.field->name && p.field != found.field)
				return found.owner;
		return "";
	}

	Context& m_context;
	SrcTypes& m_types;
};

// the src record a folded accessor's access (the FoldedAccessors pass's) points to, for a memptr read through it;
// nullptr for any other memptr/memref
const SrcModel::Record* foldedRecord(const Function& function, Context& context, const cexpr_t* e)
{
	if (e->op != cot_memptr || e->x->op != cot_call)
		return nullptr;
	auto folded = function.meaning.find(e->x);
	if (folded == function.meaning.end() || !std::holds_alternative<Access>(folded->second))
		return nullptr;
	SrcTypes::Parts p = context.types.parts(context.types.canonical(srcType(function, context, e->x)));
	return p.kind == "ptr" ? context.model.record(context.types.parts(p.of).name) : nullptr;
}

// The src record whose vector the memref or memptr reads, where src gives it a lane accessor the exe's inlined lane
// accesses are: the vector its member at offset 0 of the record's whole size (Havok's `hkVector4::m_quad`), the record
// declaring `operator()` of one parameter whose non-const overload returns a reference to the lane (the SDK's
// `hkReal& hkVector4::operator()(int)`), each overload transparent (porting rules "Transparent functions"; checkbody.py
// `transparent`): defined in a header, with no body src registers outside the headers at an address of it, so that
// a src call of it and the exe's inlined lane access compare alike. nullptr otherwise.
const SrcModel::Record* laneAccessor(Context& context, const cexpr_t* vector, const std::string& lane)
{
	if (vector->op != cot_memref && vector->op != cot_memptr)
		return nullptr;
	tinfo_t holder = ownerType(vector);
	const SrcModel::Record* record = context.types.recordOf(holder);
	if (!record || vector->m != 0 || holder.get_size() != vector->type.get_size())
		return nullptr;
	const SrcModel& model = context.model;
	const std::vector<const SrcModel::Function*>& overloads = model.functionsNamed(record->name + "::operator()");
	bool reference = false;
	for (const SrcModel::Function* f : overloads)
	{
		const SrcModel::Definition* definition = model.definition(f->mangled);
		const SrcModel::Type* type = model.type(f->type);
		if (!definition || !isHeader(definition->file) || !type || type->params.size() != 1)
			return nullptr;
		for (uint64_t address : model.bodyAddresses(f->mangled))
			if (bodyOutsideHeaders(model, address))
				return nullptr;
		reference |= context.types.canonical(type->ret) == context.types.canonical("lref(" + lane + ")");
	}
	return reference ? record : nullptr;
}

// IDA's lane array of a compiler vector (`v.m128_f32`, a member of IDA's `__m128` union) indexed (`v.m128_f32[i]`, not
// its address: a vector element has none): the vector subscripted by clang's vector extension (`v[i]`, an rvalue's
// lanes included). A lane array of another type of the lane's width (`v.m128_u32[i]`) is that lane's bits: read,
// `std::bit_cast<uint32_t>(v[i])` (on the index); written, as a statement, with an integer (an enum) or a zero,
// `v[i] = std::bit_cast<float>(static_cast<uint32_t>(x))` (on the assignment), the value converted to the lane's
// type as Hex-Rays' assignment converts it. A lane of the vector of a record with a lane accessor (laneAccessor) is
// that accessor's call, the source's form (`x(i)`, its address `&x(i)`, the lane array decayed to its first lane's
// address `&x(0)`). False for any other member, lane array or use (a plain vector's lane address, a compound
// assignment, an increment), whose bytes are read in place.
bool vectorLane(Function& function, Context& context,
				const std::unordered_map<const cexpr_t*, const citem_t*>& parents, const cexpr_t* e)
{
	tinfo_t owner = ownerType(e);
	tinfo_t held = memberType(e);
	auto parent = parents.find(e);
	if (!owner.is_union() || !held.is_array() || parent == parents.end() || !parent->second->is_expr())
		return false;
	std::string why;
	std::optional<std::string> vector = context.types.keyOf(owner, why);
	std::optional<std::string> lane = context.types.keyOf(held.get_array_element(), why);
	const SrcModel::Type* type = vector ? context.model.type(context.types.canonical(*vector)) : nullptr;
	if (!type || type->kind != "vector" || !lane || type->count != static_cast<int64_t>(held.get_array_nelems()))
		return false;
	const bool own = context.types.canonical(*lane) == type->of;
	const SrcModel::Record* accessor = own && e->op == cot_memref ? laneAccessor(context, e->x, type->of) : nullptr;
	const std::string call = accessor && e->x->op == cot_memptr ? "(*$0)" : "$0";
	const auto* index = static_cast<const cexpr_t*>(parent->second);
	if (index->op != cot_idx || index->x != e)
	{
		// the lane array decayed to a pointer to its first lane
		if (!accessor || index->op == cot_ref)
			return false;
		function.meaning[e] = Rewrite{1, "&" + call + "(0)", {e->x->x}, {}, "ptr(" + type->of + ")"};
		return true;
	}
	auto grandparent = parents.find(index);
	const auto* user = grandparent != parents.end() && grandparent->second->is_expr()
						   ? static_cast<const cexpr_t*>(grandparent->second)
						   : nullptr;
	if (accessor)
	{
		function.meaning[index] = Rewrite{1, call + "($1)", {e->x->x, index->y}, {}, type->of};
		return true;
	}
	if (user && user->op == cot_ref)
		return false;
	std::string object = e->op == cot_memptr ? "(*$0)" : "$0";
	if (own)
	{
		function.meaning[e] = Rewrite{1, object, {e->x}, {}, ""};
		return true;
	}
	std::optional<std::string> spelled = context.types.spell(*lane);
	std::optional<std::string> element = context.types.spell(type->of);
	if (!spelled || !element || context.model.sizeOf(*lane) != context.model.sizeOf(type->of))
		return false;
	if (user && written(user) && user->x == index)
	{
		// the value: an integer or an enum, or a zero (the same bits in any lane type)
		auto statement = parents.find(user);
		if (user->op != cot_asg || statement == parents.end() || statement->second->op != cit_expr)
			return false;
		std::string stored = context.types.canonical(srcType(function, context, user->y));
		SrcTypes::Parts p = context.types.parts(stored);
		const SrcModel::Type* value = p.kind == "builtin" ? context.model.type(stored) : nullptr;
		bool integer = p.kind == "enum" || (value && !value->isFloat && stored != "builtin:void");
		double zero = 1;
		bool floatZero = user->y->op == cot_fnum && user->y->fpc->fnum.to_double(&zero) == REAL_ERROR_OK && zero == 0;
		if (!integer && !isZero(user->y) && !floatZero)
			return false;
		function.meaning[user] = Rewrite{
			1, object + "[$1] = std::bit_cast<" + *element + ">(static_cast<" + *spelled + ">($2))",
			{e->x, index->y, user->y}, {}, type->of, "<bit>"};
		function.meaning[e] = Rewrite{1, object, {e->x}, {}, ""};
		return true;
	}
	function.meaning[index] =
		Rewrite{1, "std::bit_cast<" + *spelled + ">(" + object + "[$1])", {e->x, index->y}, {}, *lane, "<bit>"};
	function.meaning[e] = Rewrite{1, object, {e->x}, {}, ""};
	return true;
}

// Hex-Rays' cast of a field read through a folded accessor's access to the field's own type goes
void dropReadCast(Function& function, Context& context,
				  const std::unordered_map<const cexpr_t*, const citem_t*>& parents, const cexpr_t* e)
{
	const Member* member = std::get_if<Member>(&function.meaning[e]);
	auto parent = parents.find(e);
	if (!member || parent == parents.end() || parent->second->op != cot_cast)
		return;
	const cexpr_t* cast = static_cast<const cexpr_t*>(parent->second);
	std::string why;
	std::optional<std::string> key = context.types.keyOf(cast->type, why);
	if (key && context.types.canonical(*key) == context.types.canonical(member->field->type))
		function.meaning[cast] = Implicit{};
}

// A member of a src union that Hex-Rays reads as an integer the code converts to or from a pointer of the integer's
// size (`u.iData = (int)p`, `(T*)u.iData`): the exe stores or loads the pointer itself, so the member is the union's
// pointer member at 0 of the pointer's own type, else its `void*` one (`u.pData = p`, `static_cast<T*>(u.pData)`),
// and Hex-Rays' cast is C++'s own conversion (from `void*` a static_cast; into it no const dropped). On x64 only the
// pointer member holds the pointer whole. False for any other member, or a union without such a member.
bool unionPointer(Function& function, Context& context,
				  const std::unordered_map<const cexpr_t*, const citem_t*>& parents, const cexpr_t* e)
{
	tinfo_t owner = ownerType(e);
	auto parent = parents.find(e);
	if (!owner.is_union() || !e->type.is_integral() || e->type.is_bool() || e->type.is_enum() ||
		parent == parents.end() || !parent->second->is_expr())
		return false;
	const SrcModel::Record* record = context.types.recordOf(owner);
	const auto* user = static_cast<const cexpr_t*>(parent->second);
	const cexpr_t* cast = nullptr;
	tinfo_t pointer;
	bool store = false;
	if (user->op == cot_cast && user->x == e && user->type.is_ptr())
	{
		cast = user;
		pointer = user->type;
	}
	else if (user->op == cot_asg && user->x == e && user->y->op == cot_cast && user->y->x->type.is_ptr())
	{
		cast = user->y;
		pointer = user->y->x->type;
		store = true;
	}
	std::string why;
	std::optional<std::string> key = cast ? context.types.keyOf(pointer, why) : std::nullopt;
	if (!record || !key || pointer.get_size() != e->type.get_size())
		return false;
	std::string wanted = context.types.canonical(*key);
	std::vector<Placed> placed;
	fieldsOf(context.model, *record, record->name, 0, placed);
	const Placed *exact = nullptr, *untyped = nullptr;
	for (const Placed& p : placed)
	{
		std::string type = context.types.canonical(p.field->type);
		if (p.offsetBits != 0 || p.field->bits || p.field->name.empty() || context.types.parts(type).kind != "ptr")
			continue;
		if (type == wanted)
			exact = &p;
		else if (context.types.parts(type).of == "builtin:void")
			untyped = &p;
	}
	const Placed* chosen = exact ? exact : untyped;
	if (!chosen || (store && !exact && pointer.get_pointed_object().is_const()))
		return false;
	function.meaning[e] = Member{chosen->field, chosen->owner, ""};
	function.meaning[cast] = exact || store ? Meaning(Implicit{}) : Meaning(*key);
	return true;
}

// A member src has no field for that is a vtable pointer of the object's src record (one of its vtables is at that
// offset): C++ has no expression for the pointer itself, so the mark names what the code does with it rather than
// `no src field`. A member of `p[k]` (k not 0) is no vtable pointer of R's: the bytes past the end of R are a derived
// class's (Hex-Rays indexes the pointer as an array). A read through it is a call through the vtable that Calls did
// not resolve (its own mark says why); one indexed past IDA's vtable struct (`vptr[k]`, k not 0, src's vtable as sync
// writes it) reads a slot past the end of src's vtable of R: a derived class's virtual, or a slot src lacks.
// A store (VtableStores names a missing constant), a compare, a copy into a local (the exe's one vtable read for
// several calls) and any other read of the pointer's value have no src form either. Every vtable call Calls resolves
// erases these.
void markVtablePointer(Function& function, Context& context,
					   const std::unordered_map<const cexpr_t*, const citem_t*>& parents, const cexpr_t* e)
{
	auto found = function.meaning.find(e);
	const Missing* missing = found != function.meaning.end() ? std::get_if<Missing>(&found->second) : nullptr;
	if (!missing || !missing->what.starts_with("no src field at ") || e->type.get_size() != 4)
		return;
	const SrcModel::Record* record = context.types.recordOf(ownerType(e));
	if (!record || ownerType(e).is_union() ||
		std::none_of(record->vtables.begin(), record->vtables.end(),
					 [&](const SrcModel::Vtable& v) { return v.offset == int64_t(e->m); }))
		return;
	std::string vptr = record->name + "'s vtable pointer at " + hexOffset(e->m);
	auto parentOf = [&](const cexpr_t* x) -> const cexpr_t* {
		auto p = parents.find(x);
		return p != parents.end() && p->second->is_expr() ? static_cast<const cexpr_t*>(p->second) : nullptr;
	};
	const cexpr_t* parent = parentOf(e);
	std::string why;
	if (e->op == cot_memref && e->x->op == cot_idx && !(e->x->y->op == cot_num && e->x->y->numval() == 0))
	{
		found->second = Missing{record->name + "+" + hexOffset(e->m) + " of " +
								(e->x->y->op == cot_num ? "p[k], k not 0" : "p[i]") + ": past the end of " +
								record->name + ", a derived class's bytes"};
		return;
	}
	if (!parent)
		why = vptr + " read as a value";
	else if (parent->op == cot_idx && parent->x == e && parent->y->op == cot_num && parent->y->numval() != 0)
	{
		found->second = Missing{vptr + " indexed past the end of src's vtable of " + record->name +
								": a derived class's virtual (the object's class is not " + record->name +
								"), or a slot src's " + record->name + " lacks"};
		return;
	}
	else if (parent->op == cot_memptr || (parent->op == cot_idx && parent->x == e))
		why = vptr + " read for a call through it that Calls did not resolve";
	else if (parent->op == cot_asg && parent->x == e)
		why = "store into " + vptr;
	else if ((parent->op == cot_eq || parent->op == cot_ne) ||
			 (parent->op == cot_cast && parentOf(parent) &&
			  (parentOf(parent)->op == cot_eq || parentOf(parent)->op == cot_ne)))
		why = vptr + " compared";
	else if (const cexpr_t* user = parent->op == cot_cast ? parentOf(parent) : parent;
			 user && user->op == cot_asg && user->x->op == cot_var && (user->y == e || user->y == parent))
		why = vptr + " copied into a local";
	else
		why = vptr + " read as a value";
	found->second = Missing{why + ": C++ has no expression for it"};
}

// src's member at offset 0 of the record (a member's member at 0 included; a base there C++ converts by itself) whose
// type is target (canonical), or, where array, whose type is an array of target; with no target, the innermost one
// member at 0 that is no record: its path from the record (`m_vec.`) and the member. None where the outermost
// member's name is not unique in the record (lookup from the object's class would find another).
struct AtZero
{
	std::string path;
	const SrcModel::Field* field = nullptr;
	bool array = false;
};
std::optional<AtZero> memberAtZero(Context& context, const SrcModel::Record& record, const std::string& target)
{
	const SrcModel& model = context.model;
	std::vector<Placed> top;
	fieldsOf(model, record, record.name, 0, top);
	auto unique = [&](const std::string& name) {
		return std::count_if(top.begin(), top.end(), [&](const Placed& q) { return q.field->name == name; }) == 1;
	};
	std::string path;
	for (const SrcModel::Record* r = &record; r;)
	{
		std::vector<Placed> placed;
		fieldsOf(model, *r, r->name, 0, placed);
		std::vector<const Placed*> first;
		for (const Placed& p : placed)
			if (p.offsetBits == 0 && !p.field->bits && !p.field->name.empty())
				first.push_back(&p);
		for (const Placed* p : first)
		{
			std::string type = context.types.canonical(p->field->type);
			SrcTypes::Parts parts = context.types.parts(type);
			// no target: the one member at 0 that is no record
			bool leaf = target.empty() && first.size() == 1 && parts.kind != "record" && parts.kind != "array";
			bool same = type == target || leaf;
			bool array = parts.kind == "array" && context.types.canonical(parts.of) == target;
			if (!same && !array)
				continue;
			if (path.empty() && !unique(p->field->name))
				return std::nullopt;
			return AtZero{path, p->field, array};
		}
		// the one member at 0 is a record: its own member at 0
		if (first.size() != 1 || (path.empty() && !unique(first[0]->field->name)))
			return std::nullopt;
		SrcTypes::Parts parts = context.types.parts(context.types.canonical(first[0]->field->type));
		if (parts.kind != "record")
			return std::nullopt;
		path += first[0]->field->name + ".";
		r = model.record(parts.name);
	}
	return std::nullopt;
}

// a cast no earlier pass resolved other than with its src type (resolveTypes' key, or the default Missing)
bool unresolvedCast(Function& function, const cexpr_t* cast)
{
	auto found = function.meaning.find(cast);
	return found != function.meaning.end() && (std::holds_alternative<Missing>(found->second) ||
												std::holds_alternative<std::string>(found->second));
}

// The value read through the pointer a cast gives (`*(T*)p`) is only tested for zero: an if's or loop's condition, the
// operand of `!`, `&&` or `||`, a `?:`'s condition, a compare with 0. Its signedness then reads nothing.
bool readAsTruth(const std::unordered_map<const cexpr_t*, const citem_t*>& parents, const cexpr_t* cast)
{
	auto up = parents.find(cast);
	if (up == parents.end() || up->second->op != cot_ptr)
		return false;
	const cexpr_t* read = static_cast<const cexpr_t*>(up->second);
	auto use = parents.find(read);
	if (use == parents.end())
		return false;
	const citem_t* p = use->second;
	switch (p->op)
	{
	case cot_lnot:
	case cot_land:
	case cot_lor:
	case cit_if:
	case cit_while:
	case cit_do:
		return true;
	case cot_tern:
		return static_cast<const cexpr_t*>(p)->x == read;
	case cot_eq:
	case cot_ne:
	{
		const cexpr_t* compare = static_cast<const cexpr_t*>(p);
		return isZero(compare->x == read ? compare->y : compare->x);
	}
	case cit_for:
		return &static_cast<const cinsn_t*>(p)->cfor->expr == read;
	default:
		return false;
	}
}

// Hex-Rays' cast of a pointer to a src record (valueType) to a pointer to the type of its member at offset 0
// (memberAtZero) is that member's address (`&p->m`; `p->m` for an array of that type, which decays to it): the same
// address on any layout. Only a cast an earlier pass left unresolved, and none that drops a const. A cast of a
// record's value to the type of its member at 0, of the record's size (`(__m128)v`: the exe reads the record's bytes
// as that member; a pointer too, `(hkMotionState*)body.m_motion` of a union), is that member (`v.m_quad`,
// `body.m_motion.pState`).
void firstMember(Function& function, Context& context,
				 const std::unordered_map<const cexpr_t*, const citem_t*>& parents, const cexpr_t* cast)
{
	auto found = function.meaning.find(cast);
	if (found == function.meaning.end())
		return;
	std::string why;
	if (cast->x->type.is_udt() && unresolvedCast(function, cast))
	{
		const SrcModel::Record* record = context.types.recordOf(cast->x->type);
		std::optional<std::string> key = context.types.keyOf(cast->type, why);
		// a Hex-Rays helper's value (`LODWORD(x)`) is no record in src, whatever type Hex-Rays gives it
		if (!record || !key || cast->type.get_size() != cast->x->type.get_size() ||
			(cast->x->op == cot_ptr && isThis(function, cast->x->x)) ||
			(cast->x->op == cot_call && cast->x->x->op == cot_helper))
			return;
		std::optional<AtZero> at = memberAtZero(context, *record, context.types.canonical(*key));
		if (at && !at->array)
			function.meaning[cast] = Rewrite{1, "$0." + at->path + at->field->name, {cast->x}, {}, at->field->type};
		return;
	}
	tinfo_t operand = valueType(cast->x);
	if (!std::holds_alternative<Missing>(found->second) || !cast->type.is_ptr() || !operand.is_ptr())
		return;
	tinfo_t from = operand.get_pointed_object(), to = cast->type.get_pointed_object();
	if (from.is_const() && !to.is_const())
		return;
	const SrcModel::Record* record = context.types.recordOf(from);
	std::optional<std::string> key = context.types.keyOf(to, why);
	if (!record || !key)
		return;
	std::optional<AtZero> at = memberAtZero(context, *record, context.types.canonical(*key));
	// a byte member read through a pointer to a byte of the other signedness only to test it for zero (an hkBool's
	// m_bool through Hex-Rays' `*(_BYTE*)`, the exe's inlined operator bool): the read is that member, of its own type
	// (no pointer to it is printed, so none of another signedness is read through)
	auto byte = [&](const std::string& type) {
		std::string t = context.types.canonical(type);
		return t == "builtin:char" || t == "builtin:signed char" || t == "builtin:unsigned char";
	};
	if (!at && byte(*key) && readAsTruth(parents, cast))
	{
		std::optional<AtZero> one = memberAtZero(context, *record, "");
		if (one && byte(one->field->type) && !isThis(function, cast->x))
		{
			function.meaning.erase(cast);
			function.meaning[parents.at(cast)] =
				Rewrite{1, "$0->" + one->path + one->field->name, {cast->x}, {}, one->field->type};
		}
		return;
	}
	if (!at)
		return;
	// a member of `this` is named alone where no local or parameter hides its name
	std::string outer = at->path.empty() ? at->field->name : at->path.substr(0, at->path.find('.'));
	bool alone = isThis(function, cast->x) && std::none_of(function.locals.begin(), function.locals.end(),
														   [&](const Local& local) { return local.name == outer; });
	std::string form = (at->array ? "" : "&") + std::string(alone ? "" : "$0->") + at->path + at->field->name;
	std::string type = at->array ? context.types.parts(context.types.canonical(at->field->type)).of : at->field->type;
	function.meaning[cast] = Rewrite{1, form, {cast->x}, {}, "ptr(" + type + ")"};
}

// An assignment statement of Hex-Rays' cast to a src record of a value of the type of the record's member at offset 0
// (memberAtZero), of the record's size (`v = (hkVector4)q`: the exe writes the member's bytes), is that member's
// assignment (`v.m_quad = q`). A number so assigned, cast or not (`v = (hkVector4)0`, `v = 0`), is that member's
// bytes: the number (or its cast) becomes one of the member's type (`v.m_quad = (__m128)0`), which a later pass may
// spell; not a number but 0 for a floating member, which C++ would give the number's value, not its bits. Not where
// the left side is a local (or an assignment that is no statement of its own), which placeDeclarations may declare
// at the assignment.
void firstMemberStore(Function& function, Context& context,
					  const std::unordered_map<const cexpr_t*, const citem_t*>& parents, const cexpr_t* e)
{
	const cexpr_t* y = e->y;
	bool cast = y->op == cot_cast && y->type.is_udt() && unresolvedCast(function, y);
	const cexpr_t* operand = cast ? y->x : y;
	bool number = operand->op == cot_num;
	const tinfo_t& stored = cast ? y->type : e->x->type;
	auto parent = parents.find(e);
	if ((!cast && !(number && stored.is_udt())) ||
		(e->x->op == cot_var && !function.cfunc->get_lvars()->at(e->x->v.idx).is_arg_var()) ||
		parent == parents.end() || parent->second->op != cit_expr || function.meaning.count(e) != 0)
		return;
	const SrcModel::Record* record = context.types.recordOf(stored);
	if (!record || (!number && stored.get_size() != operand->type.get_size()))
		return;
	std::string why;
	std::optional<std::string> key = number ? std::nullopt : context.types.keyOf(operand->type, why);
	std::optional<AtZero> at;
	if (number || key)
		at = memberAtZero(context, *record, number ? "" : context.types.canonical(*key));
	if (!at || at->array || context.model.sizeOf(at->field->type) != static_cast<int64_t>(stored.get_size()))
		return;
	if (number && operand->numval() != 0)
		if (const SrcModel::Type* t = context.model.type(context.types.canonical(at->field->type)); t && t->isFloat)
			return;
	std::string form = "$0." + at->path + at->field->name + " = $1";
	if (number)
		function.meaning[y] = at->field->type;
	function.meaning[e] = Rewrite{1, form, {e->x, number ? y : operand}, {}, ""};
}

// A dereference of a pointer src has as a record's (`*p`, where Hex-Rays reads another type through it), reading the
// type of the record's member at offset 0 (memberAtZero), is that member (`p->m`): the same bytes on any layout. Only
// where no earlier layer resolved the dereference.
void firstMemberRead(Function& function, Context& context, const cexpr_t* e)
{
	auto found = function.meaning.find(e);
	if ((found != function.meaning.end() && !std::holds_alternative<Missing>(found->second)) || !e->x->type.is_ptr())
		return;
	std::string pointer = context.types.canonical(srcType(function, context, e->x));
	SrcTypes::Parts parts = context.types.parts(pointer);
	if (parts.kind != "ptr" || context.types.parts(parts.of).kind != "record")
		return;
	const SrcModel::Record* record = context.model.record(context.types.parts(parts.of).name);
	std::string why;
	std::optional<std::string> read = context.types.keyOf(e->type, why);
	if (!record || !read)
		return;
	std::string target = context.types.canonical(*read);
	if (context.types.parts(target).kind == "record")
		return;  // a record read whole is the object itself (a copy), not its member
	std::optional<AtZero> at = memberAtZero(context, *record, target);
	if (at && !at->array)
		function.meaning[e] = Rewrite{1, "$0->" + at->path + at->field->name, {e->x}, {}, at->field->type};
}

// a src field a byte offset of a record names: its spelling from the object (`m_a.m_b`, `m_arr[2]`; a name lookup
// from the record finds another member of is qualified by the class declaring it), its type key and size
struct FieldPath
{
	std::string text, type;
	int64_t size = 0;
};

// the field of the record at the byte offset of the type key read (canonical; "" for any type: the member there,
// outermost), a member's member or an array's element included; nullopt for none
std::optional<FieldPath> fieldPath(Context& context, const SrcModel::Record& record, int64_t offset,
								   const std::string& read)
{
	const SrcModel& model = context.model;
	std::vector<Placed> placed;
	fieldsOf(model, record, record.name, 0, placed);
	for (const Placed& p : placed)
	{
		std::optional<int64_t> size = model.sizeOf(p.field->type);
		int64_t start = p.offsetBits / 8;
		if (p.field->bits || p.field->name.empty() || p.offsetBits % 8 || !size || offset < start ||
			offset >= start + *size)
			continue;
		std::string name = p.field->name;
		if (std::count_if(placed.begin(), placed.end(), [&](const Placed& q) { return q.field->name == name; }) > 1)
		{
			const SrcModel::Record* owner = model.record(p.owner);
			name = (owner && !owner->spelled.empty() ? owner->spelled : p.owner) + "::" + name;
		}
		std::string type = context.types.canonical(p.field->type);
		if (offset == start && (read.empty() || type == read))
			return FieldPath{name, p.field->type, *size};
		SrcTypes::Parts parts = context.types.parts(type);
		if (parts.kind == "record")
		{
			if (const SrcModel::Record* inner = model.record(parts.name))
				if (std::optional<FieldPath> deeper = fieldPath(context, *inner, offset - start, read))
					return FieldPath{name + "." + deeper->text, deeper->type, deeper->size};
		}
		else if (parts.kind == "array" && context.types.canonical(parts.of) == read)
		{
			std::optional<int64_t> element = model.sizeOf(parts.of);
			if (element && *element > 0 && (offset - start) % *element == 0)
				return FieldPath{name + "[" + std::to_string((offset - start) / *element) + "]", parts.of, *element};
		}
	}
	return std::nullopt;
}

// the src record a pointer expression points to: its src type's (a local's, a resolved call's result), else IDA's
const SrcModel::Record* pointedRecord(const Function& function, Context& context, const cexpr_t* pointer)
{
	SrcTypes::Parts p = context.types.parts(context.types.canonical(srcType(function, context, pointer)));
	if (p.kind == "ptr")
	{
		SrcTypes::Parts of = context.types.parts(p.of);
		return of.kind == "record" ? context.model.record(of.name) : nullptr;
	}
	return pointer->type.is_ptr() ? context.types.recordOf(pointer->type.get_pointed_object()) : nullptr;
}

// the locals Hex-Rays types as a pointer to a record's first member, written once with Fields' first member address
// (`float* p = &q->m_normal.m_quad.x`, its cast's firstMember rewrite) and never otherwise: that cast, by lvar
using Holders = std::map<int, const cexpr_t*>;

// Pointer arithmetic by a constant from a src record's pointer cast to another pointer (`(char*)p + N`,
// `*((T*)p + k)`, `((T*)p)[k]`), from a local src types as a record's pointer, or from a field's address past that
// field's end (`&p->m + k`; a holder's first member included): the src field at that byte offset of the record,
// where it has one of the type read there (`p->m_f`, its address `&p->m_f` where the result is a pointer; none past
// the record's end). Else a field's address stays as it is within the field,
// and past it is marked `offset N past field F of R` (a cast an earlier layer left unresolved keeps its marker); a
// holder's arithmetic only resolves once the holder is the record's pointer (dry: whether it would, nothing written).
bool rawOffset(Function& function, Context& context,
			   const std::unordered_map<const cexpr_t*, const citem_t*>& parents, const Holders& holders,
			   const cexpr_t* e, bool dry)
{
	bool index = e->op == cot_idx;
	if ((e->op != cot_add && e->op != cot_sub && !index) || e->y->op != cot_num || !e->x->type.is_ptr())
		return false;
	size_t step = e->x->type.get_pointed_object().get_size();
	if (step == BADSIZE || step == 0)
		return false;
	int64_t k = static_cast<int64_t>(e->y->numval());
	if (e->y->type.is_signed() && e->y->type.get_size() == 4)
		k = static_cast<int32_t>(k);
	if (e->op == cot_sub)
		k = -k;
	// where the arithmetic starts: the object (a record's pointer, or the record itself), the byte offset in it, the
	// field there (path; nullopt for none) where the start is a field's address
	const cexpr_t* start = e->x;
	const cexpr_t* object = nullptr;
	bool arrow = true, holder = false;
	int64_t base = 0;
	std::optional<FieldPath> from;
	const SrcModel::Record* record = nullptr;
	std::string why;
	auto meaning = [&](const cexpr_t* x) {
		auto found = function.meaning.find(x);
		return found == function.meaning.end() ? nullptr : &found->second;
	};
	// a record's pointer cast to another pointer: Fields' first member address, or a cast left unresolved
	auto castFrom = [&](const cexpr_t* cast) {
		const Meaning* m = meaning(cast);
		bool first = m && std::holds_alternative<Rewrite>(*m);
		if (!m || (!first && !std::holds_alternative<Missing>(*m)))
			return false;
		record = pointedRecord(function, context, cast->x);
		std::optional<std::string> element = context.types.keyOf(cast->type.get_pointed_object(), why);
		if (!record || !element || context.types.canonical(*element) == "record:" + record->name)
			return false;
		// Fields' member at offset 0 the cast is the address of
		return !first || (from = fieldPath(context, *record, 0, context.types.canonical(*element))).has_value();
	};
	if (start->op == cot_cast && start->x->type.is_ptr())
	{
		if (!castFrom(start))
			return false;
		object = start->x;
	}
	else if (start->op == cot_ref && (start->x->op == cot_memptr || start->x->op == cot_memref))
	{
		for (const cexpr_t* a = start->x;; a = a->x)
		{
			const Meaning* m = meaning(a);
			if (!m || !std::holds_alternative<Member>(*m))
				return false;
			base += a->m;
			if (a->op == cot_memptr || (a->x->op != cot_memref && a->x->op != cot_memptr))
			{
				object = a->x;
				arrow = a->op == cot_memptr;
				break;
			}
		}
		record = context.types.recordOf(arrow ? object->type.get_pointed_object() : object->type);
		std::optional<std::string> held = context.types.keyOf(start->x->type, why);
		if (!record || !held || !(from = fieldPath(context, *record, base, context.types.canonical(*held))))
			return false;
		// the address of an array's element (`&p->m[2]`) is no field's: pointer arithmetic within the array
		if (tinfo_t member = memberType(start->x); member.is_array())
			return false;
	}
	else if (auto held = holders.find(lvarIndex(start)); held != holders.end())
	{
		if (!castFrom(held->second))
			return false;
		object = start;
		holder = true;
	}
	// a local src types as a record's pointer, Hex-Rays as another pointer (bytes)
	else if (start->op == cot_var && (record = pointedRecord(function, context, start)) &&
			 !context.types.recordOf(start->type.get_pointed_object()))
		object = start;
	else
		return false;
	int64_t offset = base + k * static_cast<int64_t>(step);
	// the read: a pointer cast of the result, a dereference of it or of that cast, an element
	const cexpr_t* target = e;
	tinfo_t read = e->x->type.get_pointed_object();
	bool address = !index;
	auto parentOf = [&](const cexpr_t* x) -> const cexpr_t* {
		auto found = parents.find(x);
		return found != parents.end() && found->second->is_expr() ? static_cast<const cexpr_t*>(found->second)
																   : nullptr;
	};
	if (!index)
	{
		const cexpr_t* parent = parentOf(e);
		if (parent && parent->op == cot_cast && parent->type.is_ptr())
		{
			target = parent;
			read = parent->type.get_pointed_object();
			parent = parentOf(parent);
		}
		if (parent && parent->op == cot_ptr)
		{
			target = parent;
			address = false;
		}
	}
	// within the field it starts from: C++'s own pointer arithmetic there (a holder's too, which once the record's
	// pointer is that field read)
	if (from && offset >= base && offset + static_cast<int64_t>(read.get_size()) <= base + from->size &&
		!(holder && dry))
		return true;
	std::string at = offset < 0 ? "-" + hexOffset(-offset) : hexOffset(offset);
	// the field there. Past the end of the record there is none: whether the bytes there are the next element of an
	// array or a derived class's member is not known (Hex-Rays would index a pointer it knew to be an array's)
	std::optional<std::string> key = context.types.keyOf(read, why);
	std::optional<FieldPath> path =
		key ? fieldPath(context, *record, offset, context.types.canonical(*key)) : std::nullopt;
	if (dry || !path || holder)
	{
		if (!dry && from)
			function.meaning[index ? e->y : e] =
				Missing{"offset " + at + " past field " + from->text + " of " + record->name};
		return path.has_value();
	}
	// the object's member: on `this` alone where no local or parameter hides its outermost name
	std::string outer = path->text.substr(0, path->text.find_first_of(".["));
	outer = outer.substr(outer.rfind(':') == std::string::npos ? 0 : outer.rfind(':') + 1);
	bool alone = arrow && isThis(function, object) &&
				 std::none_of(function.locals.begin(), function.locals.end(),
							  [&](const Local& local) { return local.name == outer; });
	std::string through = alone ? "" : arrow ? "$0->" : "$0.";
	std::string form = (address ? "&" : "") + through + path->text;
	// what the form replaces is not printed: its meanings go (the object's stay)
	std::set<const cexpr_t*> kept;
	forEachSubexpression(object, [&](const cexpr_t* x) { kept.insert(x); });
	forEachSubexpression(target, [&](const cexpr_t* x) {
		if (!kept.count(x))
			function.meaning.erase(x);
	});
	// the member access needs the record complete: its header
	std::string header = isHeader(record->file) && record->file.starts_with("src/") ? record->file.substr(4) : "";
	function.meaning[target] =
		Rewrite{1, form, {object}, {}, address ? "ptr(" + path->type + ")" : path->type, header};
	return true;
}

// The holders (above). One whose every other use is pointer arithmetic by a constant that resolves to the record's
// fields is the record's pointer: its src type that pointer, the cast of its write printed as its operand (C++'s own
// conversion); the others stay holders, their arithmetic past the first member marked.
Holders recordHolders(Function& function, Context& context,
					  const std::unordered_map<const cexpr_t*, const citem_t*>& parents)
{
	const lvars_t& lvars = *function.cfunc->get_lvars();
	std::map<int, std::vector<const cexpr_t*>> writes, uses;
	std::set<int> other;
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
		int v = lvarIndex(e);
		if (v < 0)
		{
			if ((written(e) || e->op == cot_ref) && lvarIndex(e->x) >= 0 && e->op != cot_asg)
				other.insert(lvarIndex(e->x));
			else if (e->op == cot_asg && lvarIndex(e->x) >= 0)
				writes[lvarIndex(e->x)].push_back(e);
			return;
		}
		auto parent = parents.find(e);
		const citem_t* p = parent == parents.end() ? nullptr : parent->second;
		if (!(p && p->op == cot_asg && static_cast<const cexpr_t*>(p)->x == e))
			uses[v].push_back(e);
	});
	Holders holders;
	for (auto& [v, assignments] : writes)
	{
		const cexpr_t* cast = assignments.size() == 1 ? assignments[0]->y : nullptr;
		auto found = cast ? function.meaning.find(cast) : function.meaning.end();
		if (lvars[v].is_arg_var() || other.count(v) || !cast || cast->op != cot_cast ||
			found == function.meaning.end() || !std::holds_alternative<Rewrite>(found->second))
			continue;
		holders[v] = cast;
	}
	for (auto it = holders.begin(); it != holders.end();)
	{
		bool resolves = !uses[it->first].empty();
		for (const cexpr_t* use : uses[it->first])
		{
			auto parent = parents.find(use);
			const citem_t* p = parent == parents.end() ? nullptr : parent->second;
			resolves = resolves && p && p->is_expr() && static_cast<const cexpr_t*>(p)->x == use &&
					   rawOffset(function, context, parents, holders, static_cast<const cexpr_t*>(p), true);
		}
		const SrcModel::Record* record = pointedRecord(function, context, it->second->x);
		if (!resolves || !record)
		{
			++it;
			continue;
		}
		function.locals[it->first].type = "ptr(record:" + record->name + ")";
		function.meaning[it->second] = Implicit{};
		it = holders.erase(it);
	}
	return holders;
}

// A member Hex-Rays reads past the end of a pointer's record (`p[k].m`, k > 0; IDA indexes the pointer as an array):
// where the record is a base at offset 0 of the class the body is a member of, and that class has a field at the byte
// offset of the size read, the field of the pointer cast to that class (`static_cast<D*>(p)->m_x`): the exe reads the
// object as the body's own class, at the same address. Read through a cast of its address (`*(float*)&p[k].m`, and
// that address alone) the field is of the type read; read as IDA's member (the base's member at the folded offset, no
// type of the bytes there) it is any non-floating scalar field of the member's kind (pointer or integer) and an
// integer's signedness, whose C++ type then means what Hex-Rays' does. False for anything else.
bool ownClassField(Function& function, Context& context, Fields& fields,
				   const std::unordered_map<const cexpr_t*, const citem_t*>& parents, const cexpr_t* e)
{
	if (e->op != cot_memref || e->x->op != cot_idx || e->x->y->op != cot_num || !e->x->x->type.is_ptr() ||
		!function.self || function.self->cls.empty() || e->type.is_udt() || e->type.is_array())
		return false;
	const cexpr_t* object = e->x->x;
	tinfo_t pointee = object->type.get_pointed_object();
	int64_t k = static_cast<int64_t>(e->x->y->numval());
	const SrcModel::Record* base = context.types.recordOf(pointee);
	const SrcModel::Record* own = context.model.record(function.self->cls);
	if (k <= 0 || !base || !own || own == base || !baseAt(context.model, *own, base->name, 0))
		return false;
	auto parentOf = [&](const cexpr_t* x) -> const cexpr_t* {
		auto found = parents.find(x);
		return found != parents.end() && found->second->is_expr() ? static_cast<const cexpr_t*>(found->second)
																   : nullptr;
	};
	// the read: the member, or a pointer cast of its address (dereferenced, or that address)
	const cexpr_t* target = e;
	tinfo_t read = e->type;
	bool address = false;
	const cexpr_t* ref = parentOf(e);
	const cexpr_t* cast = ref && ref->op == cot_ref ? parentOf(ref) : nullptr;
	if (cast && cast->op == cot_cast && cast->type.is_ptr())
	{
		read = cast->type.get_pointed_object();
		const cexpr_t* deref = parentOf(cast);
		target = deref && deref->op == cot_ptr ? deref : cast;
		address = target == cast;
	}
	int64_t offset = k * static_cast<int64_t>(pointee.get_size()) + static_cast<int64_t>(e->m);
	Meaning found = fields.at(*own, offset, read, false, own);
	// IDA's member read: any field of the size that is a scalar but a floating one (whose value is no copy of the bits),
	// of the read's kind and signedness (at)
	if (target == e && !std::holds_alternative<Member>(found))
	{
		found = fields.at(*own, offset, read, true, own);
		const Member* any = std::get_if<Member>(&found);
		std::string type = any ? context.types.canonical(any->field->type) : "";
		std::string kind = any ? context.types.parts(type).kind : "";
		if (!any || kind == "record" || kind == "array" || type == "builtin:float" || type == "builtin:double" ||
			type == "builtin:long double")
			return false;
	}
	const Member* member = std::get_if<Member>(&found);
	std::optional<std::string> spelled = context.types.spell("record:" + own->name);
	if (!member || !spelled || offset + static_cast<int64_t>(read.get_size()) > own->size)
		return false;
	bool isConst = pointee.is_const() || context.types.constPointee(srcType(function, context, object));
	std::string form = std::string(address ? "&" : "") + "static_cast<" + (isConst ? "const " : "") + *spelled +
					   "*>($0)->" + fields.named(*own, *member);
	std::set<const cexpr_t*> kept;
	forEachSubexpression(object, [&](const cexpr_t* x) { kept.insert(x); });
	forEachSubexpression(target, [&](const cexpr_t* x) {
		if (!kept.count(x))
			function.meaning.erase(x);
	});
	std::string header = isHeader(own->file) && own->file.starts_with("src/") ? own->file.substr(4) : "";
	std::string type = address ? "ptr(" + member->field->type + ")" : member->field->type;
	function.meaning[target] = Rewrite{1, form, {object}, {}, type, header};
	return true;
}

// IDA's type of a src record's anonymous member (an anonymous union or struct sync writes inline, which IDA names
// `Outer::$<hash>`)
bool anonymousType(const tinfo_t& type)
{
	qstring name;
	if (!type.is_udt() || !type.get_type_name(&name))
		return false;
	std::string_view spelled = name.c_str();
	size_t scope = spelled.rfind("::");
	return spelled.substr(scope == spelled.npos ? 0 : scope + 2).starts_with('$');
}

// A member of IDA's anonymous type inside a src record (`eo.$22CA...eoDouble`, Hex-Rays' `eo.eoDouble` through the
// unnamed member): the record's own field at the two offsets added, as C++ names it through the anonymous member
// (`eo.eoDouble`, `p->eoDouble`; the anonymous member itself is not printed). Only where the outer object is a src
// record and the field is there of the type read.
void anonymousMember(Function& function, Context& context, Fields& fields, const cexpr_t* e)
{
	if (e->op != cot_memref || (e->x->op != cot_memptr && e->x->op != cot_memref) || !anonymousType(ownerType(e)) ||
		!anonymousType(memberType(e->x)))
		return;
	const cexpr_t* outer = e->x;
	tinfo_t object = ownerType(outer);
	const SrcModel::Record* record = context.types.recordOf(object);
	if (!record)
		return;
	int64_t offset = (object.is_union() ? 0 : int64_t(outer->m)) + (ownerType(e).is_union() ? 0 : int64_t(e->m));
	tinfo_t held = memberType(e);
	if (!held.is_array() || e->type.is_array())
		held = e->type;
	Meaning found = fields.at(*record, offset, held, false, record);
	const Member* member = std::get_if<Member>(&found);
	if (!member)
		return;
	function.meaning.erase(outer);
	if (outer->op == cot_memref)
	{
		function.meaning[e] = found;
		function.meaning[outer] = Rewrite{1, "$0", {outer->x}, {}, "record:" + record->name};
		return;
	}
	std::string header = isHeader(record->file) && record->file.starts_with("src/") ? record->file.substr(4) : "";
	function.meaning[e] =
		Rewrite{1, "$0->" + fields.named(*record, *member), {outer->x}, {}, member->field->type, header};
}

// The debug build's downcast locals (user, 2026-11-12). A parameter whose src type is a reference or pointer to a
// class B that Hex-Rays reads past B's end (`*(float*)&p[k].m`, `&p[k]`, k not 0: IDA indexes the pointer as an array)
// is read as a class D derived from B at offset 0 where the PDB declares, in the function, a reference or pointer local
// of D's (srcdb's pdb_typed local) that the debug build's code assigns once, from that parameter's value
// (Context::boundLocals, the debug build's events): 0xE9CD10's `kSphABV`, `const NiSphereBV&`, from `kABV1`. Every
// such read (through the parameter or a local only ever assigned it) must lie inside D, else D is not the class read
// and nothing changes; each that names a field of D of the type read (fieldPath) is that field through the local
// (`kSphABV.m_kSphere.m_fRadius`, an address `&kSphABV.m_kSphere.m_kCenter`; read as the address of an element,
// `&p[k]` passed to a call, the type is the callee's parameter's pointee), and the local is declared first thing in
// the body (Function::downcasts). Not for a parameter the body rebinds (an entry copy), a local whose name another
// local or parameter has, a const parameter's non-const local, or a class whose src size is not IDA's.
void downcastLocals(Function& function, Context& context,
					const std::unordered_map<const cexpr_t*, const citem_t*>& parents)
{
	const SrcModel::Function* self = function.self;
	auto bound = self ? context.boundLocals.find(self->name) : context.boundLocals.end();
	if (bound == context.boundLocals.end())
		return;
	SrcTypes& types = context.types;
	const lvars_t& lvars = *function.cfunc->get_lvars();
	auto parentOf = [&](const cexpr_t* x) -> const cexpr_t* {
		auto found = parents.find(x);
		return found != parents.end() && found->second->is_expr() ? static_cast<const cexpr_t*>(found->second)
																   : nullptr;
	};
	for (const auto& [name, parameterName] : bound->second)
	{
		const std::string* localType = context.model.pdbTyped("local", self->mangled, name);
		int parameter = -1;
		for (int index : function.cfunc->argidx)
			if (function.locals[index].parameter >= 0 && function.locals[index].name == parameterName &&
				!function.locals[index].entryCopy)
				parameter = index;
		bool taken = std::any_of(function.locals.begin(), function.locals.end(),
								 [&](const Local& local) { return local.name == name; }) ||
					 std::any_of(function.downcasts.begin(), function.downcasts.end(),
								 [&](const Function::Downcast& d) { return d.name == name; });
		if (!localType || parameter < 0 || taken)
			continue;
		std::optional<PointeeCv> from = pointeeOf(types, function.locals[parameter].type);
		std::optional<PointeeCv> to = pointeeOf(types, *localType);
		if (!from || !to || (from->isConst && !to->isConst) || types.parts(from->type).kind != "record" ||
			types.parts(to->type).kind != "record")
			continue;
		const SrcModel::Record* base = context.model.record(types.parts(from->type).name);
		const SrcModel::Record* derived = context.model.record(types.parts(to->type).name);
		tinfo_t pointee = lvars[parameter].type().get_pointed_object();
		if (!base || !derived || base == derived || !baseAt(context.model, *derived, base->name, 0) ||
			pointee.get_size() == BADSIZE || static_cast<int64_t>(pointee.get_size()) != base->size)
			continue;
		// the parameter and the locals only ever assigned it (never addressed)
		std::set<int> holders{parameter};
		std::map<int, bool> copies;
		forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
			if ((!written(e) && e->op != cot_ref) || lvarIndex(e->x) < 0 || lvars[lvarIndex(e->x)].is_arg_var())
				return;
			int v = lvarIndex(e->x);
			bool copy = e->op == cot_asg && lvarIndex(e->y) == parameter && lvars[v].type() == lvars[parameter].type();
			copies.emplace(v, true).first->second &= copy;
		});
		for (const auto& [v, only] : copies)
			if (only)
				holders.insert(v);
		// each read past B's end: the expression it prints in place of, its byte offset and size, the type read
		// ("" where none is known) and whether it is an address
		struct Read
		{
			const cexpr_t* target;
			int64_t offset, size;
			std::string type;
			bool address;
		};
		std::vector<Read> reads;
		bool fits = true;
		std::string why;
		auto keyOf = [&](const tinfo_t& t) {
			std::optional<std::string> key = types.keyOf(t, why);
			return key ? types.canonical(*key) : std::string();
		};
		forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
			const cexpr_t* index = holders.count(lvarIndex(e)) ? parentOf(e) : nullptr;
			if (!index || index->op != cot_idx || index->x != e || index->y->op != cot_num || index->y->numval() == 0)
				return;
			int64_t offset = static_cast<int64_t>(index->y->numval()) * base->size;
			const cexpr_t* up = parentOf(index);
			Read read{nullptr, offset, base->size, "", false};
			// a member of the element: `p[k].m`, read, or its address cast and read or passed on
			const cexpr_t* address = up && up->op == cot_ref ? up : nullptr;
			if (up && up->op == cot_memref && up->x == index)
			{
				read = Read{up, offset + int64_t(up->m), static_cast<int64_t>(up->type.get_size()), keyOf(up->type),
							false};
				const cexpr_t* ref = parentOf(up);
				address = ref && ref->op == cot_ref ? ref : nullptr;
			}
			if (address)
			{
				const cexpr_t* cast = parentOf(address);
				if (cast && cast->op == cot_cast && cast->type.is_ptr())
				{
					tinfo_t held = cast->type.get_pointed_object();
					const cexpr_t* deref = parentOf(cast);
					bool loaded = deref && deref->op == cot_ptr;
					read = Read{loaded ? deref : cast, read.offset, static_cast<int64_t>(held.get_size()),
								keyOf(held), !loaded};
				}
				else if (cast && cast->op == cot_call && cast->a)
				{
					// passed to a call: the callee's parameter's pointee
					tinfo_t callee = cast->x->type.is_funcptr() ? cast->x->type.get_pointed_object() : cast->x->type;
					func_type_data_t data;
					read.target = nullptr;
					for (size_t k = 0; k < cast->a->size() && callee.get_func_details(&data); ++k)
						if (&(*cast->a)[k] == address && k < data.size() && data[k].type.is_ptr())
						{
							tinfo_t held = data[k].type.get_pointed_object();
							read = Read{address, read.offset, static_cast<int64_t>(held.get_size()), keyOf(held), true};
						}
				}
				else
					read.target = nullptr;
			}
			if (read.size <= 0 || read.offset + read.size > derived->size)
				fits = false;
			else if (read.target && !read.type.empty())
				reads.push_back(read);
		});
		if (!fits)
			continue;
		bool reference = types.parts(types.bare(*localType)).kind == "lref";
		std::string header = isHeader(derived->file) && derived->file.starts_with("src/") ? derived->file.substr(4) : "";
		bool any = false;
		for (const Read& read : reads)
		{
			std::optional<FieldPath> path = fieldPath(context, *derived, read.offset, read.type);
			if (!path)
				continue;
			std::string form = (read.address ? "&" : "") + name + (reference ? "." : "->") + path->text;
			std::string type = read.address ? "ptr(" + (to->isConst ? "const(" + path->type + ")" : path->type) + ")"
											: path->type;
			forEachSubexpression(read.target, [&](const cexpr_t* x) { function.meaning.erase(x); });
			function.meaning[read.target] = Rewrite{1, form, {}, {}, type, header};
			any = true;
		}
		if (any)
			function.downcasts.push_back(Function::Downcast{name, *localType, parameter});
	}
}

}  // namespace

void resolveFields(Function& function, Context& context)
{
	Fields fields(context);
	std::unordered_map<const cexpr_t*, const citem_t*> parents = parentsOf(function);
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
		// a record's pointer cast to its first member's: that member's address
		if (e->op == cot_cast)
			firstMember(function, context, parents, e);
		// a record assigned the bytes of its member at 0: that member's assignment
		if (e->op == cot_asg)
			firstMemberStore(function, context, parents, e);
		// a plain read through a record's pointer of its member at 0's type: that member
		if (e->op == cot_ptr)
			firstMemberRead(function, context, e);
		if (e->op != cot_memptr && e->op != cot_memref)
			return;
		// a compiler vector's lane: the vector subscripted
		if (vectorLane(function, context, parents, e))
			return;
		// read through a folded accessor's access: the record of its src result
		if (const SrcModel::Record* record = foldedRecord(function, context, e))
		{
			std::optional<Meaning> base = fields.base(e, record);
			function.meaning[e] = base ? *base : fields.field(e, record);
			dropReadCast(function, context, parents, e);
			return;
		}
		// past the end of a base of the body's own class: the own class's field
		if (ownClassField(function, context, fields, parents, e))
			return;
		// an integer member of a union converted to or from a pointer: the union's pointer member
		if (unionPointer(function, context, parents, e))
			return;
		std::optional<Meaning> base = fields.base(e);
		function.meaning[e] = base ? *base : fields.field(e);
		// a vtable pointer src has no field for: what the code does with it
		markVtablePointer(function, context, parents, e);
	});
	// a parameter read past its class's end as the debug build's downcast local of a derived class
	downcastLocals(function, context, parents);
	// a member of a src record's anonymous member: the record's field (after the loop above judged the members)
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) { anonymousMember(function, context, fields, e); });
	// pointer arithmetic from a record's pointer or a field's address: the src field there (once the members it
	// starts from are resolved, and the locals holding a first member's address that are the record's pointer retyped)
	Holders holders = recordHolders(function, context, parents);
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
		rawOffset(function, context, parents, holders, e, false);
	});
}

}  // namespace idakit
