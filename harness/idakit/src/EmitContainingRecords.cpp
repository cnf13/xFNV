// Containing records: Hex-Rays' CONTAINING_RECORD(p, T, f), the T whose member f p points at, where f is T's base
// subobject B in src (at the offset IDA's T has f at) and p is a B pointer, is the downcast the source wrote:
// `static_cast<T*>(p)` (`const T*` for a pointer to a const B), which subtracts the base's offset as the exe does.
// At a nonzero offset only where p is never null (neverNull: `this`, a structor's object, a reference, a local tested
// nonzero on the way to it, a variable's address): C++'s downcast maps null to null, the exe's `lea` subtracts the
// offset whatever p is. Any other member (a field, an offset src's layout does not have) or pointer stays the helper,
// marked. Hex-Rays' ADJ(p) of a shifted pointer p (`B *__shifted(D, N)`, a _VIA virtual's this and the locals it is
// copied to) is the same downcast to D, by the same rules; of an alias of a _VIA virtual's this, C++'s this.

#include "EmitPasses.hpp"

#include "IdaSupport.hpp"

#include <string_view>

namespace idakit
{

namespace
{

class ContainingRecords
{
public:
	ContainingRecords(Function& function, Context& context)
		: m_function(function), m_context(context), m_parents(parentsOf(function))
	{
	}

	void run()
	{
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			if (e->op == cot_call && e->x->op == cot_helper && std::string_view(e->x->helper) == "CONTAINING_RECORD" &&
				e->a->size() == 3 && (*e->a)[2].op == cot_helper && e->type.is_ptr())
				rewrite(e);
			else if (e->op == cot_call && e->x->op == cot_helper && std::string_view(e->x->helper) == "ADJ" &&
					 e->a->size() == 1 && e->type.is_ptr() && !isThis(m_function, e))
				adjusted(e);
		});
	}

private:
	// ADJ(p) of a shifted pointer (`B *__shifted(D, N)`): the D whose base subobject B at N p points at. Of a _VIA
	// virtual's this through a local alias of it (Self rewrites ADJ(this) itself), C++'s this; else the downcast,
	// as CONTAINING_RECORD's
	void adjusted(const cexpr_t* call)
	{
		const cexpr_t* pointer = &(*call->a)[0];
		ptr_type_data_t shifted;
		if (!pointer->type.get_ptr_details(&shifted) || !(shifted.taptr_bits & TAPTR_SHIFTED))
			return;
		const SrcModel::Record* record = m_context.types.recordOf(call->type.get_pointed_object());
		const SrcModel::Record* base = m_context.types.recordOf(shifted.obj_type);
		std::optional<int64_t> offset = record && base ? baseOffset(record->name, base->name) : std::nullopt;
		if (!offset || *offset != shifted.delta || unresolvedIn(m_function, pointer))
			return;
		std::string key = "record:" + record->name;
		std::optional<std::string> spelled = m_context.types.spell(key);
		bool isConst = m_context.types.constPointee(srcType(m_function, m_context, pointer));
		if (!spelled || !neverNull(m_function, m_parents, pointer, call))
			return;
		std::string type = isConst ? "ptr(const(" + key + "))" : "ptr(" + key + ")";
		if (viaThis(m_function, pointer) && m_function.self && m_context.model.record(m_function.self->cls) == record)
		{
			m_function.meaning[call] = Rewrite{1, "this", {}, {}, type};
			return;
		}
		std::string to = isConst ? "const " + *spelled : *spelled;
		m_function.meaning[call] = Rewrite{1, "static_cast<" + to + "*>($0)", {pointer}, {}, type};
	}

	void rewrite(const cexpr_t* call)
	{
		const carglist_t& args = *call->a;
		tinfo_t container = call->type.get_pointed_object();
		const SrcModel::Record* record = m_context.types.recordOf(container);
		std::optional<uint64_t> at = record ? containingOffset(call) : std::nullopt;
		if (!at)
			return;
		// the pointer past Hex-Rays' casts (to its `char*` or another pointer), as src types it
		const cexpr_t* pointer = &args[0];
		while (pointer->op == cot_cast && pointer->x->type.is_ptr())
			pointer = pointer->x;
		if (unresolvedIn(m_function, pointer))
			return;
		// (a src reference Hex-Rays has as a pointer: the object it refers to)
		std::string from = srcType(m_function, m_context, pointer);
		SrcTypes::Parts p = m_context.types.parts(m_context.types.canonical(from));
		if (p.kind != "ptr" && p.kind != "lref")
			return;
		SrcTypes::Parts pointee = m_context.types.parts(m_context.types.canonical(p.of));
		// a reference's const, as the pointer to its object's
		bool reference = p.kind == "lref";
		std::string asPointer = from.starts_with("lref(") ? "ptr(" + from.substr(5) : from;
		bool isConst = m_context.types.constPointee(asPointer);
		if (pointee.kind == "const")
			pointee = m_context.types.parts(m_context.types.canonical(pointee.of));
		std::optional<int64_t> offset =
			pointee.kind == "record" ? baseOffset(record->name, pointee.name) : std::nullopt;
		if (!offset || static_cast<uint64_t>(*offset) != *at ||
			(*offset != 0 && !reference && !neverNull(m_function, m_parents, pointer, call)))
			return;
		std::string key = "record:" + record->name;
		std::optional<std::string> spelled = m_context.types.spell(key);
		if (!spelled)
			return;
		forEachSubexpression(call, [&](const cexpr_t* e) {
			if (e != call && !insideOf(e, pointer))
				m_function.meaning.erase(e);
		});
		std::string to = isConst ? "const " + *spelled : *spelled;
		std::string type = isConst ? "ptr(const(" + key + "))" : "ptr(" + key + ")";
		// the address of a reference's object or of `&x` (`&x`), else the pointer itself; no cast where the pointer
		// is a T pointer already (IDA's container is src's record under another name), unless it is spelled `&x`,
		// which a member access after it would bind wrongly
		std::string operand = reference || pointer->op == cot_ref ? "&*$0" : "$0";
		std::string form = pointee.name == record->name && operand == "$0"
							   ? operand
							   : "static_cast<" + to + "*>(" + operand + ")";
		m_function.meaning[call] = Rewrite{1, form, {pointer}, {}, type};
	}

	// e is the pointer item or inside it
	static bool insideOf(const cexpr_t* e, const cexpr_t* pointer)
	{
		bool found = false;
		forEachSubexpression(pointer, [&](const cexpr_t* sub) { found |= sub == e; });
		return found;
	}

	// the offset of the base (a non-virtual base, directly or through bases) in the record; none where it is no base
	std::optional<int64_t> baseOffset(const std::string& record, const std::string& base) const
	{
		if (record == base)
			return 0;
		const SrcModel::Record* r = m_context.model.record(record);
		if (!r)
			return std::nullopt;
		std::optional<int64_t> found;
		for (const SrcModel::Base& b : r->bases)
			if (std::optional<int64_t> inner = b.isVirtual ? std::nullopt : baseOffset(b.record, base))
			{
				// a base reached twice is ambiguous: no static_cast
				if (found)
					return std::nullopt;
				found = b.offset + *inner;
			}
		return found;
	}

	Function& m_function;
	Context& m_context;
	std::unordered_map<const cexpr_t*, const citem_t*> m_parents;
};

}  // namespace

void rewriteContainingRecords(Function& function, Context& context)
{
	ContainingRecords(function, context).run();
}

}  // namespace idakit
