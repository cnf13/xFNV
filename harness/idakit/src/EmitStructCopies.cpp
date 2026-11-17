// Struct copies: the exe's inlined copy of a whole record, which Hex-Rays prints `qmemcpy(&a, &b, sizeof(a))`, is
// the assignment the source wrote: `a = b` (`*p = *q` through pointers, `a = (v = F())` from a record F returns,
// `a = p->GetT()` from a reference F returns). A copy of a record's leading member, or into it, is that member's
// assignment: `m = t.m_Rotate` (`t.m_Rotate = m`) for a NiTransform t whose member at offset 0 is the NiMatrix3 m.
// A copy of a run of arithmetic fields through one wider scalar (`*(_QWORD*)&a.x = *(_QWORD*)&b.x`) is its bytes'
// copy, `std::memcpy(&a.x, &b.x, 8)`; of a run holding a pointer each field's assignment. A number stored through one
// wider integer over a run of integer and pointer fields is each field's assignment of its bytes.

#include "EmitPasses.hpp"

#include <algorithm>
#include <set>
#include <string_view>

namespace idakit
{

namespace
{

// the byte count a size argument says: a number, or Hex-Rays' sizeof of an object or type
std::optional<uint64_t> byteCount(const cexpr_t* e)
{
	if (e->op == cot_num)
		return e->numval();
	if (e->op == cot_sizeof)
		return e->x->type.get_size();
	return std::nullopt;
}

class StructCopies
{
public:
	StructCopies(Function& function, Context& context) : m_function(function), m_context(context) {}

	void run()
	{
		std::unordered_map<const cexpr_t*, const citem_t*> parents = parentsOf(m_function);
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			if (e->op == cot_call && e->x->op == cot_helper && std::string_view(e->x->helper) == "qmemcpy" &&
				e->a->size() == 3)
				rewrite(e);
			if (auto parent = parents.find(e); e->op == cot_asg && parent != parents.end() &&
											   parent->second->op == cit_expr)
				fieldBytes(e, parent->second);
		});
	}

private:
	// A store of N bytes through an integer or floating type at an object's src field (Hex-Rays' `*(T*)&a.f = x`;
	// an array field through its decay, `*(double*)a.m_af`), which MSVC makes over the field and the ones after it:
	// - a copy from another object's same field (`*(T*)&a.f = *(T*)&b.f`), the exe's inlined copy of those bytes,
	//   which MSVC moves at once (a NiPoint3's x and y in one qword, a float[2] whole). Where they are, from the field
	//   on, the fields of its record of fixed-size arithmetic types, with no padding, bit-field or pointer among them
	//   (the same bytes on any layout), the copy is `std::memcpy(&a.f, &b.f, N)` (`a.m_af` for an array), a statement
	//   of its own; where they are such fields and pointers, each field's assignment (`a.f = b.f; a.g = b.g;`), a
	//   pointer's copy on any layout;
	// - a number of an integer type stored as a statement over such fields of integer types and pointers, without
	//   arrays (hkReferencedObject's 16-bit `m_memSizeAndFlags` and `m_referenceCount` written by one dword): each
	//   field's assignment of its bytes of the number, little-endian (`a.f = 0; a.g = 1;`), a pointer's only where
	//   its bytes are zero (`nullptr`).
	// Per-field assignments only on an object every field of the run is found by name from (its src class's lookup
	// finds that field alone) and whose expression has no side effects, which they read once per field.
	void fieldBytes(const cexpr_t* e, const citem_t* statement)
	{
		if (e->x->op != cot_ptr || e->x->x->op != cot_cast)
			return;
		const cexpr_t* to = e->x->x;
		tinfo_t t = e->x->type;
		bool arithmetic = t.is_floating() || (t.is_integral() && !t.is_bool() && !t.is_enum());
		if (!arithmetic || to->type.get_pointed_object().is_const())
			return;
		const cexpr_t* a = addressed(to->x);
		const Member* ma = a ? member(a) : nullptr;
		if (!ma)
			return;
		const int64_t count = static_cast<int64_t>(t.get_size());
		const cexpr_t* number = e->y;
		while (number->op == cot_cast)
			number = number->x;
		if (number->op == cot_num && t.is_integral())
		{
			splitNumber(e, statement, a, *ma, count, number->numval());
			return;
		}
		if (e->y->op != cot_ptr || e->y->x->op != cot_cast || !e->y->type.compare_with(t))
			return;
		const cexpr_t* b = addressed(e->y->x->x);
		const Member* mb = b ? member(b) : nullptr;
		if (!mb || ma->field != mb->field)
			return;
		if (std::optional<std::vector<const SrcModel::Field*>> fields = run(*ma, count, true, false))
		{
			// the member accesses themselves, so that no address prints parenthesised (an array decays by itself)
			std::string address = a == to->x->x ? "&" : "";
			replace(e, e, Rewrite{1, "std::memcpy(" + address + "$0, " + address + "$1, " + std::to_string(count) + ")",
								  {a, b}, {}, "", "<cstring>"});
			return;
		}
		std::optional<std::vector<const SrcModel::Field*>> fields = run(*ma, count, false, true);
		if (!fields || a == to->x || !named(a, *fields) || !named(b, *fields))
			return;
		std::string form;
		for (const SrcModel::Field* f : *fields)
			form += (form.empty() ? "" : "\n") + spelled(a, 0, f) + " = " + spelled(b, 1, f) + ";";
		replace(e, statement, Rewrite{1, form, {a->x, b->x}, {}});
	}

	// the number's store over the fields from the member's on, each assigned its bytes (fieldBytes)
	void splitNumber(const cexpr_t* e, const citem_t* statement, const cexpr_t* a, const Member& m, int64_t count,
					 uint64_t value)
	{
		std::optional<std::vector<const SrcModel::Field*>> fields = run(m, count, false, true);
		if (!fields || fields->size() < 2 || a != e->x->x->x->x || !named(a, *fields))
			return;
		std::string form;
		int64_t at = m.field->offsetBits;
		for (const SrcModel::Field* f : *fields)
		{
			const int64_t bits = *m_context.model.sizeOf(f->type) * 8;
			uint64_t part = bits >= 64 ? value >> (f->offsetBits - at) : (value >> (f->offsetBits - at)) &
																			 ((uint64_t(1) << bits) - 1);
			std::string key = m_context.types.canonical(f->type);
			std::string text;
			if (m_context.types.parts(key).kind == "ptr")
			{
				if (part != 0)
					return;
				text = "nullptr";
			}
			else if (key == "builtin:bool")
			{
				if (part > 1)
					return;
				text = part ? "true" : "false";
			}
			else
			{
				// a floating field's value is no integer's: its bytes would need a bit cast
				const SrcModel::Type* type = m_context.model.type(key);
				if (!type || type->isFloat)
					return;
				// a signed field's bytes as its two's complement value
				int64_t v = static_cast<int64_t>(part);
				if (type->isSigned && bits < 64 && (part >> (bits - 1)) != 0)
					v -= int64_t(1) << bits;
				text = type->isSigned ? std::to_string(v) : std::to_string(part) + (part > 0x7FFFFFFF ? "u" : "");
			}
			form += (form.empty() ? "" : "\n") + spelled(a, 0, f) + " = " + text + ";";
		}
		replace(e, statement, Rewrite{1, form, {a->x}, {}});
	}

	// the rewrite on the item that prints it (the assignment, or its statement): what the layers found in the
	// assignment outside the form's items goes
	void replace(const cexpr_t* e, const citem_t* on, Rewrite rewrite)
	{
		std::set<const cexpr_t*> kept;
		for (const citem_t* item : rewrite.items)
			forEachSubexpression(static_cast<const cexpr_t*>(item), [&](const cexpr_t* x) { kept.insert(x); });
		forEachSubexpression(e, [&](const cexpr_t* x) {
			if (!kept.count(x))
				m_function.meaning.erase(x);
		});
		m_function.meaning[on] = std::move(rewrite);
	}

	// the field read on the member access's object, form item k (`$k.g`, `$k->g`)
	static std::string spelled(const cexpr_t* access, size_t k, const SrcModel::Field* field)
	{
		return "$" + std::to_string(k) + (access->op == cot_memptr ? "->" : ".") + field->name;
	}

	// the member access's object has no side effects and prints once per field as it is: a variable, a global, members
	// of one and its array members' elements at constant indexes (not a pointer's `p[k]`, which past p's own record
	// reads another object's bytes), through pointers, with no cast and nothing marked in it (each copy would carry
	// the marker); and name lookup from its src class finds each field alone
	bool named(const cexpr_t* access, const std::vector<const SrcModel::Field*>& fields) const
	{
		const cexpr_t* object = access->x;
		for (const cexpr_t* x = object;;)
		{
			if (auto found = m_function.meaning.find(x);
				found != m_function.meaning.end() && std::holds_alternative<Missing>(found->second))
				return false;
			if (x->op == cot_var || x->op == cot_obj)
				break;
			if (x->op == cot_memref || x->op == cot_memptr || x->op == cot_ptr || x->op == cot_ref)
				x = x->x;
			else if (x->op == cot_idx && x->y->op == cot_num && x->x->type.is_array())
				x = x->x;
			else
				return false;
		}
		std::string key = srcType(m_function, m_context, object);
		SrcTypes::Parts p = m_context.types.parts(m_context.types.canonical(key));
		if (access->op == cot_memptr && p.kind == "ptr")
			p = m_context.types.parts(m_context.types.canonical(p.of));
		if (p.kind == "const" || p.kind == "volatile")
			p = m_context.types.parts(m_context.types.canonical(p.of));
		const SrcModel::Record* record = p.kind == "record" ? m_context.model.record(p.name) : nullptr;
		if (!record)
			return false;
		for (const SrcModel::Field* f : fields)
			if (countNamed(*record, f->name) != 1)
				return false;
		return true;
	}

	// the fields of the name a record has, its bases' included
	int countNamed(const SrcModel::Record& record, const std::string& name) const
	{
		int n = 0;
		for (const SrcModel::Field& f : record.fields)
			n += f.name == name;
		for (const SrcModel::Base& base : record.bases)
			if (const SrcModel::Record* b = m_context.model.record(base.record))
				n += countNamed(*b, name);
		return n;
	}

	// the member access whose address the operand is: `&a.f`, or an array member that decays to its first element;
	// nullptr for any other operand
	static const cexpr_t* addressed(const cexpr_t* operand)
	{
		if (operand->op == cot_ref && (operand->x->op == cot_memref || operand->x->op == cot_memptr))
			return operand->x;
		if ((operand->op == cot_memref || operand->op == cot_memptr) && memberType(operand).is_array())
			return operand;
		return nullptr;
	}

	// the src field Fields resolved the access to; nullptr for none
	const Member* member(const cexpr_t* access) const
	{
		auto found = m_function.meaning.find(access);
		return found == m_function.meaning.end() ? nullptr : std::get_if<Member>(&found->second);
	}

	// the fields covering the bytes from the member's field on, count of them: its record's own named fields, each
	// right after the one before, the last ending at count, no bit-field among them, each of a fixed-size
	// arithmetic type (an array of one where arrays) or a pointer (where pointers); nullopt otherwise
	std::optional<std::vector<const SrcModel::Field*>> run(const Member& m, int64_t count, bool arrays,
															bool pointers) const
	{
		const SrcModel::Record* record = m_context.model.record(m.owner);
		if (!record)
			return std::nullopt;
		int64_t at = m.field->offsetBits;
		int64_t end = at + count * 8;
		std::vector<const SrcModel::Field*> fields;
		for (const SrcModel::Field& f : record->fields)
			fields.push_back(&f);
		std::sort(fields.begin(), fields.end(), [](const SrcModel::Field* x, const SrcModel::Field* y) {
			return x->offsetBits < y->offsetBits;
		});
		std::vector<const SrcModel::Field*> covered;
		for (const SrcModel::Field* f : fields)
		{
			if (f->offsetBits < at || at == end)
				continue;
			std::optional<int64_t> size = m_context.model.sizeOf(f->type);
			std::string key = m_context.types.canonical(f->type);
			SrcTypes::Parts p = m_context.types.parts(key);
			bool fits = p.kind == "array" ? arrays && fixedArithmetic(p.of)
										  : (pointers && p.kind == "ptr") || fixedArithmetic(key);
			if (f->offsetBits != at || f->bits || f->name.empty() || !size || !fits)
				return std::nullopt;
			covered.push_back(f);
			at += *size * 8;
		}
		if (at != end)
			return std::nullopt;
		return covered;
	}

	// a type of one size on every layout that holds no pointer: a fixed-width integer, char, bool, float or double, or
	// an array of one (not long, wchar_t or an enum, whose sizes the layouts may disagree on)
	bool fixedArithmetic(const std::string& type) const
	{
		static const std::set<std::string> kFixed = {
			"builtin:bool", "builtin:char", "builtin:signed char", "builtin:unsigned char", "builtin:short",
			"builtin:unsigned short", "builtin:int", "builtin:unsigned int", "builtin:long long",
			"builtin:unsigned long long", "builtin:float", "builtin:double", "builtin:char16_t"};
		SrcTypes::Parts p = m_context.types.parts(m_context.types.canonical(type));
		if (p.kind == "array")
			return fixedArithmetic(p.of);
		return kFixed.count(m_context.types.canonical(type)) != 0;
	}

	void rewrite(const cexpr_t* call)
	{
		const carglist_t& args = *call->a;
		const cexpr_t* to = strip(&args[0]);
		const cexpr_t* from = strip(&args[1]);
		// only a record whose assignment is the byte copy the exe made (no operator= of its own or a member's)
		std::string record = pointedRecord(to);
		const SrcModel::Record* r = record.empty() ? nullptr : m_context.model.record(record);
		std::optional<uint64_t> count = byteCount(&args[2]);
		std::string source = pointedRecord(from);
		if (!r || !r->trivialCopy || !count || source.empty())
			return;
		std::string form;
		if (static_cast<uint64_t>(r->size) == *count && source == record)
			form = "*$0 = *$1";
		else if (std::string member = leadingMember(source, record);
				 !member.empty() && static_cast<uint64_t>(r->size) == *count)
			form = "*$0 = *$1." + member;
		else if (const SrcModel::Record* s = m_context.model.record(source); s && s->trivialCopy)
		{
			std::string into = leadingMember(record, source);
			if (into.empty() || static_cast<uint64_t>(s->size) != *count)
				return;
			form = "*$0." + into + " = *$1";
		}
		else
			return;
		Rewrite copy{1, form, {to, from}, {}};
		// what the layers found in the call's other parts (the helper, the sizeof) goes
		std::set<const cexpr_t*> kept;
		for (const citem_t* item : copy.items)
			forEachSubexpression(static_cast<const cexpr_t*>(item), [&](const cexpr_t* e) { kept.insert(e); });
		forEachSubexpression(call, [&](const cexpr_t* e) {
			if (!kept.count(e))
				m_function.meaning.erase(e);
		});
		m_function.meaning[call] = copy;
	}

	// e without Hex-Rays' casts to void pointers (qmemcpy takes `void *`)
	static const cexpr_t* strip(const cexpr_t* e)
	{
		while (e->op == cot_cast)
			e = e->x;
		return e;
	}

	// the record a pointer expression points at (const dropped): `&x` x's, a call returning a record by value (its
	// hidden result pointer) that record, else its src pointer type's (a reference's, for a local src has as the
	// object where Hex-Rays has its address: a reference parameter, a record returned by value); empty for another type
	std::string pointedRecord(const cexpr_t* e) const
	{
		if (e->op == cot_ref)
			return recordOf(srcType(m_function, m_context, e->x));
		auto found = m_function.meaning.find(e);
		const Call* byValue = found == m_function.meaning.end() ? nullptr : std::get_if<Call>(&found->second);
		if (byValue && byValue->resultArgument >= 0)
			return recordOf(srcType(m_function, m_context, e));
		std::string key = srcType(m_function, m_context, e);
		if (key.empty())
			return "";
		SrcTypes::Parts p = m_context.types.parts(m_context.types.canonical(key));
		return p.kind == "ptr" || p.kind == "lref" ? recordOf(p.of) : "";
	}

	// the record a src type key names, through a reference (an object a call returns by reference) and const;
	// empty for another type
	std::string recordOf(const std::string& key) const
	{
		if (key.empty())
			return "";
		SrcTypes::Parts p = m_context.types.parts(m_context.types.canonical(key));
		if (p.kind == "lref")
			p = m_context.types.parts(m_context.types.canonical(p.of));
		if (p.kind == "const")
			p = m_context.types.parts(m_context.types.canonical(p.of));
		return p.kind == "record" ? p.name : "";
	}

	// the name of the outer record's own member at offset 0 that is an inner record (not a bit-field); empty for none
	std::string leadingMember(const std::string& outer, const std::string& inner) const
	{
		const SrcModel::Record* r = m_context.model.record(outer);
		if (!r || outer == inner)
			return "";
		for (const SrcModel::Field& field : r->fields)
			if (field.offsetBits == 0 && !field.bits && !field.name.empty() && recordOf(field.type) == inner &&
				m_context.types.parts(m_context.types.canonical(field.type)).kind == "record")
				return field.name;
		return "";
	}

	Function& m_function;
	Context& m_context;
};

}  // namespace

void rewriteStructCopies(Function& function, Context& context)
{
	StructCopies(function, context).run();
}

}  // namespace idakit
