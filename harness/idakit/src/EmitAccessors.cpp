// Accessors: a field access that is exactly what an inline src accessor of the field's class returns (srcdb's
// accessor rows) prints as its call:
//
//     x.m_usSize      -> x.GetSize()     load: `return m_usSize;`
//     &x.m_kList      -> x.GetList()     address: `return &m_kList;`
//
// Only reads: nothing assigned to it, incremented or decremented, no address taken of it (but the address form's own).
// On a const object (read through a pointer to const, or a const object's member) only a const accessor is callable;
// on another the non-const overload is the one C++ calls. The accessor returns a scalar (no copy of a class, no
// reference): the field's type (load) or a pointer to it
// (address); for a load of an integer field the accessor widens (m_usSize as uint32_t) also, where the value is
// converted at once (assigned, cast, passed, returned), which the widening cannot change. Index accessors
// (`return m_pBase[i];`, NiTArray::GetAt) wait for src's GetAt to return a reference (Main, 2026-11-06): an element
// read stays as it is, its array too. A field read through a base subobject calls the accessor on the object the
// base is of (`x.begin()`), qualified by the accessor's class where lookup from the object's class needs it.

#include "EmitPasses.hpp"

namespace idakit
{

namespace
{

class Accessors
{
public:
	Accessors(Function& function, Context& context)
		: m_function(function), m_context(context), m_parents(parentsOf(function))
	{
	}

	void run()
	{
		std::vector<std::pair<const cexpr_t*, Rewrite>> found;
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			if (std::optional<Rewrite> rewrite = spelled(e))
				found.emplace_back(e, std::move(*rewrite));
		});
		for (auto& [e, rewrite] : found)
			m_function.meaning[e] = std::move(rewrite);
	}

private:
	std::optional<Rewrite> spelled(const cexpr_t* e) const
	{
		if (e->op == cot_ref)
			return call(e->x, "address", false);
		auto found = m_parents.find(e);
		const citem_t* parent = found == m_parents.end() ? nullptr : found->second;
		const cexpr_t* p = parent && parent->is_expr() ? static_cast<const cexpr_t*>(parent) : nullptr;
		// the address form's field, an indexed array, anything written, also through a Hex-Rays part helper
		// (`HIBYTE(v.m) = 1`, printed as its `_ref` form, which writes the field: an accessor's value is no lvalue)
		if (p && (p->op == cot_ref || p->op == cot_idx || (written(p) && p->x == e)))
			return std::nullopt;
		if (p && p->op == cot_call && p->x->op == cot_helper)
		{
			auto outer = m_parents.find(p);
			const citem_t* q = outer == m_parents.end() ? nullptr : outer->second;
			if (q && q->is_expr() && written(static_cast<const cexpr_t*>(q)) &&
				static_cast<const cexpr_t*>(q)->x == p)
				return std::nullopt;
		}
		return call(e, "load", converted(parent, e));
	}

	// parent converts e's value at once: assigns it, casts it, passes it to a call, returns it
	static bool converted(const citem_t* parent, const cexpr_t* e)
	{
		if (!parent)
			return false;
		if (parent->op == cit_return)
			return true;
		const auto* p = static_cast<const cexpr_t*>(parent);
		return parent->is_expr() &&
			   ((p->op == cot_asg && p->y == e) || p->op == cot_cast || (p->op == cot_call && p->x != e));
	}

	// the field access (memref/memptr) as the call of its declaring class's accessor of that kind ("load",
	// "address")
	std::optional<Rewrite> call(const cexpr_t* access, const char* kind, bool widens) const
	{
		if (access->op != cot_memref && access->op != cot_memptr)
			return std::nullopt;
		auto found = m_function.meaning.find(access);
		const Member* member = found == m_function.meaning.end() ? nullptr : std::get_if<Member>(&found->second);
		if (!member)
			return std::nullopt;
		// a load or address accessor takes nothing (one that takes a parameter returns another element); on a const
		// object only a const member is callable, and on another C++ calls the non-const overload where there is one
		bool constant = constObject(access);
		const SrcModel::Accessor* chosen = nullptr;
		bool exact = false;
		for (const SrcModel::Accessor& accessor : m_context.model.accessorsOf(member->owner))
			if (const SrcModel::Type* type = m_context.model.type(accessor.function->type);
				!exact && type && type->params.empty() && accessor.kind == kind &&
				accessor.field == member->field->name && (type->isConst || !constant) &&
				returns(*accessor.function, accessor.kind == "address", member->field->type, widens))
			{
				chosen = &accessor;
				exact = type->isConst == constant;
			}
		if (!chosen)
			return std::nullopt;
		// a base subobject the field is read through (Fields' record key on IDA's base member) is no receiver src
		// writes: the accessor is called on the object it is a base of (`x.begin()`, not
		// `static_cast<hkArrayBase<T>&>(x).begin()`), qualified where lookup from that object's class finds another
		// member of the accessor's name
		const cexpr_t* receiver = access->x;
		bool arrow = access->op == cot_memptr;
		while ((receiver->op == cot_memref || receiver->op == cot_memptr) && baseSubobject(receiver))
		{
			arrow = receiver->op == cot_memptr;
			receiver = receiver->x;
		}
		std::string name = chosen->function->name.substr(chosen->function->name.rfind("::") + 2);
		if (receiver != access->x && !foundAlone(receiver, arrow, chosen->function->cls, name))
		{
			const SrcModel::Record* owner = m_context.model.record(chosen->function->cls);
			name = (owner && !owner->spelled.empty() ? owner->spelled : chosen->function->cls) + "::" + name;
		}
		const SrcModel::Type* fn = m_context.model.type(chosen->function->type);
		// the call needs the accessor's class complete: its declaring header (the member access had its owner's)
		const std::string& file = chosen->function->file;
		std::string header = isHeader(file) && file.starts_with("src/") ? file.substr(4) : "";
		return Rewrite{1, "$0" + std::string(arrow ? "->" : ".") + name + "()", {receiver}, {}, fn ? fn->ret : "",
					   header};
	}

	// a memptr/memref Fields resolved to a base subobject read whole (its record's key)
	bool baseSubobject(const cexpr_t* e) const
	{
		auto found = m_function.meaning.find(e);
		return found != m_function.meaning.end() && std::holds_alternative<std::string>(found->second);
	}

	// lookup of the name from the class of the object (receiver, a pointer to it where arrow) finds the member of
	// owner alone: owner is the one subobject of that class declaring a member (function or field) of the name
	bool foundAlone(const cexpr_t* receiver, bool arrow, const std::string& owner, const std::string& name) const
	{
		SrcTypes& types = m_context.types;
		SrcTypes::Parts parts = types.parts(types.canonical(srcType(m_function, m_context, receiver)));
		if (arrow || parts.kind == "lref")
			parts = parts.kind == "ptr" || parts.kind == "lref" ? types.parts(parts.of) : SrcTypes::Parts{};
		const SrcModel::Record* object = parts.kind == "record" ? m_context.model.record(parts.name) : nullptr;
		if (!object)
			return false;
		int owners = 0, others = 0;
		declaring(*object, owner, name, owners, others);
		return owners == 1 && others == 0;
	}

	// counts the subobjects of the record (itself and its non-virtual bases) declaring a member of the name: owner's,
	// any other class's
	void declaring(const SrcModel::Record& record, const std::string& owner, const std::string& name, int& owners,
				   int& others) const
	{
		const SrcModel& model = m_context.model;
		if (!model.functionsNamed(record.name + "::" + name).empty() || record.field(name))
			++(record.name == owner ? owners : others);
		for (const SrcModel::Base& base : record.bases)
			if (const SrcModel::Record* b = model.record(base.record))
			{
				if (base.isVirtual)
					++others;
				else
					declaring(*b, owner, name, owners, others);
			}
	}

	// the object a field access reads from is const: the pointer it is read through points to const, or the object
	// (a local, or the object holding the member it is, or the array member holding the element it is: a const
	// member's `m_axis[0].m_endPoints`) is const, in src's type or IDA's
	bool constObject(const cexpr_t* access) const
	{
		const cexpr_t* e = access;
		while (e->op == cot_memref || (e->op == cot_idx && e->x->type.is_array()))
			e = e->x;
		const cexpr_t* pointer = e->op == cot_memptr || e->op == cot_ptr ? e->x : nullptr;
		if (pointer)
			return m_context.types.constPointee(srcType(m_function, m_context, pointer)) ||
				   (pointer->type.is_ptr() && pointer->type.get_pointed_object().is_const());
		return m_context.types.bare(srcType(m_function, m_context, e)).rfind("const(", 0) == 0 || e->type.is_const();
	}

	// the accessor returns a scalar that is the field (a pointer to it, for an address accessor; a pointer's pointee
	// const included: `begin() const`'s `const T*` is not the field `T* m_data`); a widening load: an integer holding
	// every value of the field's integer type
	bool returns(const SrcModel::Function& accessor, bool address, const std::string& field, bool widens) const
	{
		const SrcModel::Type* fn = m_context.model.type(accessor.type);
		std::string ret = fn ? m_context.types.canonical(fn->ret) : "";
		const SrcModel::Type* r = m_context.model.type(ret);
		if (!r || (r->kind != "builtin" && r->kind != "ptr" && r->kind != "enum"))
			return false;
		std::string type = m_context.types.canonical(field);
		if (address)
			return r->kind == "ptr" && m_context.types.canonical(r->of) == type;
		if (ret == type)
			return m_context.types.constPointee(m_context.types.bare(fn->ret)) ==
				   m_context.types.constPointee(m_context.types.bare(field));
		const SrcModel::Type* f = m_context.model.type(type);
		return widens && f && r->kind == "builtin" && f->kind == "builtin" && !r->isFloat && !f->isFloat &&
			   r->size > f->size && (r->isSigned || !f->isSigned);
	}

	Function& m_function;
	Context& m_context;
	std::unordered_map<const cexpr_t*, const citem_t*> m_parents;
};

}  // namespace

void spellAccessors(Function& function, Context& context)
{
	Accessors(function, context).run();
}

}  // namespace idakit
