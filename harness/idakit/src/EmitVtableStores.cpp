// Vtable stores: a store of an exe vtable into an object's vfptr (Hex-Rays' `*(_DWORD *)this = &X::vftable;`,
// `this->__vftable = ...`, at a base subobject's offset for a secondary table) is RULES'
// `fnvd::setVtable(pThis, X_vftable);` (`fnvd::setVtable(static_cast<Base*>(pThis), X_vftable_Base);`), the table
// the src FNVD_VFTABLE constant at its address. The object is any the body reaches: a constructor's or destructor's
// own, an inlined constructor's in any function (NiTMap::Init's this, a factory's or CreateClone's new object, a
// stack local, `&v`), or an embedded member's (`&pThis->m_kArray`); not raw storage (InlinedLocals' bytes, marked).
// Where src knows the object's record, it holds a vptr at offset 0, else the store is at one of its base subobjects.

#include "EmitPasses.hpp"

#include "idakit/SrcIda.hpp"

#include <optional>
#include <set>

namespace idakit
{

namespace
{

// The object a store reaches and a byte offset into it: item is a pointer to it (object false: `this`, `v`,
// `&pThis->m_kArray`) or the object itself (object true: a local `v`, printed `&v`).
struct Place
{
	const cexpr_t* item = nullptr;
	bool object = false;
	int64_t offset = 0;
};

std::optional<Place> objectAt(const Function& function, const cexpr_t* e);

// a member access src resolved to a field (its address is a pointer to that member object)
bool srcField(const Function& function, const cexpr_t* e)
{
	auto found = function.meaning.find(e);
	return (e->op == cot_memptr || e->op == cot_memref) && found != function.meaning.end() &&
		   std::holds_alternative<Member>(found->second);
}

// what an address expression points at: v, v + n (Hex-Rays' pointer arithmetic), &object, &field, through casts
std::optional<Place> addressOf(const Function& function, const cexpr_t* e)
{
	int64_t offset = 0;
	for (;;)
		switch (e->op)
		{
		case cot_cast:
			e = e->x;
			break;
		case cot_var:
			return Place{e, false, offset};
		case cot_add:
		{
			if (e->y->op != cot_num || !e->x->type.is_ptr())
				return std::nullopt;
			size_t element = e->x->type.get_pointed_object().get_size();
			if (element == BADSIZE)
				return std::nullopt;
			offset += static_cast<int64_t>(e->y->numval() * element);
			e = e->x;
			break;
		}
		case cot_ref:
		{
			if (srcField(function, e->x))
				return Place{e, false, offset};
			std::optional<Place> place = objectAt(function, e->x);
			if (place)
				place->offset += offset;
			return place;
		}
		default:
			return std::nullopt;
		}
}

// what an lvalue designates: a local, *p, p->m, x.m; a member src resolved to a field of record type is that member
// object (`pThis->m_kArray`, printed `&pThis->m_kArray`)
std::optional<Place> objectAt(const Function& function, const cexpr_t* e)
{
	std::optional<Place> place;
	if (e->op == cot_var || (srcField(function, e) && e->type.is_udt()))
		place = Place{e, true, 0};
	else if (e->op == cot_ptr)
		place = addressOf(function, e->x);
	else if (e->op == cot_memptr && (place = addressOf(function, e->x)))
		place->offset += e->m;
	else if (e->op == cot_memref && (place = objectAt(function, e->x)))
		place->offset += e->m;
	return place;
}

}  // namespace

void rewriteVtableStores(Function& function, Context& context)
{
	if (!function.defines)
		return;
	std::string source = function.self->cls.empty() ? "" : context.model.classSource(function.self->cls);
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t* s) {
		if (e->op != cot_asg || e->x->type.get_size() != 4)
			return;
		// a store FNVD_INLINED_LOCAL's macro makes
		auto statement = function.meaning.find(s);
		const InlinedStatement* inlined =
			statement != function.meaning.end() ? std::get_if<InlinedStatement>(&statement->second) : nullptr;
		if (inlined && inlined->macro)
			return;
		std::optional<Place> place = objectAt(function, e->x);
		int v = place ? lvarIndex(place->item) : -1;
		// an integer holding the address (Hex-Rays' `*(_DWORD *)v`) is no object pointer src passes; a local that is
		// no record (`void** v72 = &X::vftable`, a piece of an object Hex-Rays split into scalars) holds the table's
		// address as its value, no object's vptr: the store stays the vtable address Globals marks
		if (!place || (!place->object && !place->item->type.is_ptr()) || (v >= 0 && function.locals[v].storage) ||
			(place->object && place->item->op == cot_var && !place->item->type.is_udt()))
			return;
		const cexpr_t* value = e->y;
		while (value->op == cot_cast)
			value = value->x;
		if (value->op == cot_ref)
			value = value->x;
		if (value->op != cot_obj)
			return;
		const SrcModel::Ref* table = nullptr;
		for (const SrcModel::Ref& ref : context.model.refsAt(value->obj_ea))
			if (ref.kind == "vftable" && !placeholder(ref))
				table = &ref;
		// the vfptr Fields marked (markVtablePointer): the store is setVtable's once src has the constant where the
		// body sees it
		auto vfptr = function.meaning.find(e->x);
		Missing* marked = vfptr != function.meaning.end() ? std::get_if<Missing>(&vfptr->second) : nullptr;
		if (marked && !marked->what.starts_with("store into "))
			marked = nullptr;
		if (!table)
		{
			qstring name;
			if (marked && get_name(&name, value->obj_ea) > 0 && std::string_view(name.c_str()).starts_with("??_7"))
				marked->what = "store of exe vtable " + std::string(name.c_str()) + " (" + hex(value->obj_ea) +
							   "): no src FNVD_VFTABLE constant at its address";
			return;
		}
		// the constant is the merged body's to name: declared in a header, or in the class's own .cpp
		if (!isHeader(table->file) && table->file != source)
		{
			function.meaning[value] = Missing{"vtable constant " + table->target + " is declared in " + table->file};
			if (marked)
				marked->what = "store of vtable constant " + table->target + ", declared in " + table->file;
			return;
		}
		// the object's src record (none for an untyped pointer: the store is the exe's at that address)
		tinfo_t type = place->object ? place->item->type : place->item->type.get_pointed_object();
		const SrcModel::Record* record = type.is_udt() ? context.types.recordOf(type) : nullptr;
		std::string pointer = place->object ? "&$0" : "$0";
		std::string form = "fnvd::setVtable(" + pointer + ", $1)";
		if (place->offset != 0)
		{
			std::string base = record ? baseAt(context.model, *record, place->offset) : "";
			std::optional<std::string> spelled =
				base.empty() ? std::nullopt : context.types.spell("ptr(record:" + base + ")");
			if (!spelled)
				return;
			form = "fnvd::setVtable(static_cast<" + *spelled + ">(" + pointer + "), $1)";
		}
		else if (record && !record->vfptr && record->vtables.empty() && record->bases.empty())
			return;
		// into a src field that is no record (an object typed as its member at 0): no object src knows takes a vptr
		else if (!record && place->item->op == cot_ref)
		{
			function.meaning[value] = Missing{"vtable stored into a src member that is no record"};
			return;
		}
		// what the layers found inside the store (the vfptr field, the vtable's IDA type) goes; the object's own
		// meanings stay
		std::set<const cexpr_t*> kept{value};
		forEachSubexpression(place->item, [&](const cexpr_t* inner) { kept.insert(inner); });
		forEachSubexpression(e, [&](const cexpr_t* inner) {
			if (!kept.count(inner))
				function.meaning.erase(inner);
		});
		function.meaning[value] = table;
		function.meaning[e] = Rewrite{1, form, {place->item, value}, {}};
	});
}

}  // namespace idakit
