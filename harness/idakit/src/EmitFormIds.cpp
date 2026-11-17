// FormIds: a pointer field the exe reads or writes as a form ID during load (user, 2026-11-12). While a form loads,
// the 4-byte form ID of what a pointer field will point at is kept in the pointer's storage until the pointer is
// resolved (InitItem, a save game's InitLoadGame). Hex-Rays casts between the pointer and the integer there, which
// resolveTypes marks; where the exe's code shows the integer is a form ID, the cast is the exe's reading or writing of
// the pointer's bytes as one (ExeCast), printed under `// EXE-CAST: <field type> as <ID type>`, found by grep for the
// x64 port. The integer is a form ID where it goes to (or comes from) a form ID parameter of a load function
// (kConsumers) or, written into a field pointing to a form (TESForm or a class deriving from it), was read from the
// plugin file (source). The field may be an array of pointers or a pointer to them, read at an element. Three forms:
// - a read: `(uint32_t)p->pForm`, passed to a consumer's parameter, or assigned to a local passed to one;
// - a write through the field's address: `(uint32_t*)&p->pForm` (or `*(uint32_t*)&p->pForm` for a reference
//   parameter) passed to a consumer's parameter, which loads the ID into it (or, by value, reads it);
// - a write: `p->pForm = (TESForm*)id` of a local passed to a consumer, or filled by a source, into a form pointer.

#include "EmitPasses.hpp"

#include "IdaSupport.hpp"

#include <map>
#include <set>

namespace idakit
{

namespace
{

// The load functions taking a form ID (the src parameter's index, `this` aside): its lookups, the file's compile index
// fixed in it, the save game's loads of one into the pointer's storage and its map of changed IDs
const std::map<std::string, int> kConsumers = {
	{"TESForm::GetFormByNumericID", 0},
	{"TESForm::AddCompileIndex", 0},
	{"TESForm::LoadNumericID", 0},
	{"TESSaveLoadGame::LoadNumericID", 0},
	{"BGSLoadGameBuffer::LoadFormID", 0},
	{"BGSSaveLoadGame::GetChangedFormID", 0},
	{"MagicTarget::GetMagicTargetByNumericID", 0},
	{"MagicCaster::GetMagicCasterByNumericID", 0},
};

// The plugin file's reads of a chunk's data into an object (its first parameter)
bool source(const std::string& name)
{
	return name.starts_with("TESFile::GetChunkData") || name == "TESFile::ReadData";
}

class FormIds
{
public:
	FormIds(Function& function, Context& context)
		: m_function(function), m_context(context), m_types(context.types), m_parents(parentsOf(function))
	{
	}

	void run()
	{
		// the locals passed to a consumer's form ID parameter, and those a source fills
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			const Call* call = e->op == cot_call ? meaningOf<Call>(e) : nullptr;
			if (!call)
				return;
			for (size_t k = 0; k < e->a->size(); ++k)
			{
				const cexpr_t* arg = stripCasts(&(*e->a)[k]);
				if (arg->op == cot_ref)
					arg = stripCasts(arg->x);
				int v = lvarIndex(arg);
				if (v < 0)
					continue;
				if (consumed(*call, k))
					m_consumed.insert(v);
				else if (parameterIndex(*call, k) == 0 && source(call->function->name))
					m_read.insert(v);
			}
		});
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			if (e->op != cot_cast || !meaningOf<Missing>(e) || e->type.get_size() != 4)
				return;
			if (!e->type.is_ptr())
			{
				if (idInteger(e->type) && !pointerSlot(e->x).empty() && readAsId(e))
					m_function.meaning[e] = ExeCast{integerKey(e->type)};
			}
			else if (!e->x->type.is_ptr())
				written(e);
			else if (idInteger(e->type.get_pointed_object()) && e->x->op == cot_ref && !pointerSlot(e->x->x).empty() &&
					 loadedInto(e))
				m_function.meaning[e] = ExeCast{"ptr(" + integerKey(e->type.get_pointed_object()) + ")"};
		});
	}

private:
	// the call's argument k is a consumer's form ID parameter
	bool consumed(const Call& call, size_t k) const
	{
		auto found = kConsumers.find(call.function->name);
		return found != kConsumers.end() && parameterIndex(call, k) == found->second;
	}

	// a 4-byte integer (no bool or enum): a form ID's type
	static bool idInteger(const tinfo_t& t)
	{
		return t.is_integral() && !t.is_bool() && !t.is_enum() && t.get_size() == 4;
	}

	std::string integerKey(const tinfo_t& t) const
	{
		std::string why;
		return m_types.keyOf(t, why).value_or(t.is_signed() ? "typedef:int32_t" : "typedef:uint32_t");
	}

	// The src type of the pointer slot e reads: a field of pointer type (`p->m`, `x.m`), or an element of a field that
	// is an array of pointers or a pointer to them (`p->a[i]`); "" for any other
	std::string pointerSlot(const cexpr_t* e) const
	{
		bool element = e->op == cot_idx;
		const cexpr_t* access = element ? e->x : e;
		if (access->op != cot_memptr && access->op != cot_memref)
			return "";
		const Member* member = meaningOf<Member>(access);
		if (!member || !member->field)
			return "";
		std::string key = member->field->type;
		if (element)
		{
			SrcTypes::Parts holder = m_types.parts(m_types.canonical(key));
			if (holder.kind != "array" && holder.kind != "ptr")
				return "";
			key = holder.of;
		}
		return m_types.parts(m_types.canonical(key)).kind == "ptr" ? key : "";
	}

	// the pointer's pointee is a form: TESForm or a class deriving from it
	bool formPointer(const std::string& pointer) const
	{
		SrcTypes::Parts pointee = m_types.parts(m_types.parts(m_types.canonical(pointer)).of);
		return pointee.kind == "record" && derives(pointee.name);
	}

	bool derives(const std::string& record, int depth = 0) const
	{
		if (record == "TESForm")
			return true;
		const SrcModel::Record* r = depth < 32 ? m_context.model.record(record) : nullptr;
		if (!r)
			return false;
		for (const SrcModel::Base& base : r->bases)
			if (derives(base.record, depth + 1))
				return true;
		return false;
	}

	// The read's integer goes to a consumer's form ID parameter: as the call's argument (through Hex-Rays' casts), or
	// assigned whole to a local passed to one
	bool readAsId(const cexpr_t* cast) const
	{
		const cexpr_t* value = cast;
		auto up = m_parents.find(value);
		while (up != m_parents.end() && up->second->op == cot_cast)
		{
			value = static_cast<const cexpr_t*>(up->second);
			up = m_parents.find(value);
		}
		if (up == m_parents.end() || !up->second->is_expr())
			return false;
		const cexpr_t* parent = static_cast<const cexpr_t*>(up->second);
		if (parent->op == cot_asg && parent->y == value)
			return idLocal(parent->x) && m_consumed.count(lvarIndex(parent->x));
		const Call* call = parent->op == cot_call && parent->x != value ? meaningOf<Call>(parent) : nullptr;
		for (size_t k = 0; call && k < parent->a->size(); ++k)
			if (&(*parent->a)[k] == value)
				return consumed(*call, k);
		return false;
	}

	// a local of the form ID's integer type
	bool idLocal(const cexpr_t* e) const
	{
		int v = lvarIndex(e);
		return v >= 0 && idInteger((*m_function.cfunc->get_lvars())[v].type());
	}

	// The field's address cast to a 4-byte integer's pointer is a consumer's form ID parameter: the argument itself
	// (a pointer parameter), or the integer it points to (`*(uint32_t*)&p->pRef`: a reference parameter, which the
	// consumer loads, or one by value, which reads it)
	bool loadedInto(const cexpr_t* cast) const
	{
		const cexpr_t* value = cast;
		auto up = m_parents.find(value);
		if (up != m_parents.end() && up->second->op == cot_ptr)
		{
			value = static_cast<const cexpr_t*>(up->second);
			up = m_parents.find(value);
		}
		if (up == m_parents.end() || up->second->op != cot_call)
			return false;
		const cexpr_t* parent = static_cast<const cexpr_t*>(up->second);
		const Call* call = parent->x != value ? meaningOf<Call>(parent) : nullptr;
		for (size_t k = 0; call && k < parent->a->size(); ++k)
			if (&(*parent->a)[k] == value)
				return consumed(*call, k);
		return false;
	}

	// `field = (T*)id` (or an element of one): a local of the ID's integer type, passed to a consumer, or a source
	// filled and the field a form pointer
	void written(const cexpr_t* cast)
	{
		auto up = m_parents.find(cast);
		if (up == m_parents.end() || up->second->op != cot_asg)
			return;
		const cexpr_t* store = static_cast<const cexpr_t*>(up->second);
		std::string slot = store->y == cast ? pointerSlot(store->x) : "";
		if (slot.empty() || !idLocal(cast->x))
			return;
		int v = lvarIndex(cast->x);
		if (m_consumed.count(v) || (m_read.count(v) && formPointer(slot)))
			m_function.meaning[cast] = ExeCast{slot};
	}

	template <class T>
	const T* meaningOf(const citem_t* item) const
	{
		auto found = m_function.meaning.find(item);
		return found == m_function.meaning.end() ? nullptr : std::get_if<T>(&found->second);
	}

	Function& m_function;
	Context& m_context;
	SrcTypes& m_types;
	std::unordered_map<const cexpr_t*, const citem_t*> m_parents;
	std::set<int> m_consumed;  // locals passed to a consumer's form ID parameter (or their address)
	std::set<int> m_read;	   // locals whose address a source fills
};

}  // namespace

void markFormIds(Function& function, Context& context)
{
	FormIds(function, context).run();
}

}  // namespace idakit
