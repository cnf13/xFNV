// Allocations: a call of an allocator or a wrapper of one (the allocator table, emit's --allocators) whose size
// argument holds a numeric literal as any term or factor (`12`, `12 * n`, `n << 4`, `n * 12 + 4`): the exe's size,
// which is not src's on another layout. Where it is used as one A (cast or assigned to A*, or passed as apMemory to
// A::Construct), src pins A's sizeof and the literal's bytes are that sizeof, the literal prints as `sizeof(A)`, also
// as the factor of a count (`sizeof(A) * n`: n objects of A); so does one literal alone where the allocation is next
// constructed as A (its Construct takes it, directly or through a local nothing else writes, user 2026-11-12) and
// src's sizeof of A, pinned or not, is the literal's bytes. Otherwise the size follows the type src's declarations use
// the result as (user, 2026-11-12; elementOf): bytes (a 1-byte character or bool, or untyped memory) print as they
// are; n elements of a scalar, enum, pointer or class T print the literal part (the size, a count's factor or a
// count's shift) as `n * sizeof(T)` (`sizeof(T)` for one) where src's sizeof of T divides the literal's bytes; a class
// only where src pins its sizeof (or as one object its Construct takes), more than one object of it only where it has
// no vtable and nothing derives from it (else one object of another class may be built there). Anything else stays
// marked `literal allocation size N` (N: the literal times the table's scale, in bytes) with what stands in the way:
// `no size check for A` where src does not pin A's sizeof, no declaration, two types, a void* result used as a type,
// a remainder, a term of a sum.

#include "EmitPasses.hpp"

namespace idakit
{

namespace
{

// the record is the class or derives from it (through any base)
bool derives(const SrcModel& model, const std::string& record, const std::string& cls)
{
	if (record == cls)
		return true;
	if (const SrcModel::Record* r = model.record(record))
		for (const SrcModel::Base& base : r->bases)
			if (derives(model, base.record, cls))
				return true;
	return false;
}

class Allocations
{
public:
	Allocations(Function& function, Context& context)
		: m_function(function), m_context(context), m_parents(parentsOf(function))
	{
	}

	void run()
	{
		if (m_context.allocators.empty())
			return;
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			if (e->op == cot_call)
				call(e);
		});
	}

private:
	void call(const cexpr_t* call)
	{
		const Allocator* allocator = allocatorOf(call);
		if (!allocator || allocator->sizeArgument >= call->a->size())
			return;
		const cexpr_t* size = &(*call->a)[allocator->sizeArgument];
		std::vector<std::pair<const cexpr_t*, int64_t>> found;
		literals(size, found, false);
		if (found.empty())
			return;
		const cexpr_t* bare = stripCasts(size);
		// the size as one literal, or as one literal times a count (`24 * n`: n objects of the literal's size)
		const cexpr_t* lone = bare->op == cot_num ? bare : nullptr;
		if (!lone && found.size() == 1 && bare->op == cot_mul &&
			(stripCasts(bare->x) == found.front().first || stripCasts(bare->y) == found.front().first))
			lone = found.front().first;
		int64_t bytes = found.front().second * allocator->scale;
		bool constructed = false;
		const SrcModel::Record* record = lone ? usedAs(call, constructed) : nullptr;
		// one object, of src's sizeof where src pins it or where the allocation is next constructed as the class
		// (one literal, no count: a count's objects are an array's)
		if (record && allocator->scale == 1 && bytes == record->size &&
			(record->pinned || (constructed && lone == bare)))
			if (std::optional<std::string> spelled = m_context.types.spell("record:" + record->name))
			{
				std::string header = isHeader(record->file) && record->file.starts_with("src/") ? record->file.substr(4)
																							   : "";
				m_function.meaning[lone] = Rewrite{1, "sizeof(" + *spelled + ")", {}, {}, "", header};
				return;
			}
		std::string what = "literal allocation size " + std::to_string(bytes);
		auto mark = [&](const std::string& why) {
			m_function.meaning[found.front().first] = Missing{why.empty() ? what : what + ": " + why};
		};
		std::string unpinned = record && !record->pinned ? "no size check for " + record->name : "";
		// a count of the allocator's own elements: what they are is the allocator's type, not the result's
		if (allocator->scale != 1)
			return mark(unpinned);
		Element element = elementOf(call);
		if (!element.why.empty())
			return mark(unpinned.empty() ? element.why : unpinned);
		// bytes: the size as it is, the same on any layout
		if (element.bytes)
			return;
		std::string key = m_context.types.canonical(element.type);
		SrcTypes::Parts parts = m_context.types.parts(key);
		const SrcModel::Record* of = parts.kind == "record" ? m_context.model.record(parts.name) : nullptr;
		std::optional<int64_t> width = of ? std::optional<int64_t>(of->size) : m_context.model.sizeOf(key);
		std::optional<std::string> spelled = m_context.types.spell(element.type);
		if (!width || *width <= 0 || !spelled)
			return mark("used as " + element.type + ", which src has no size of");
		// the literal part: the size alone, the factor of a count (`24 * n`) or a count's shift (`n << 4`)
		const cexpr_t* shift = bare->op == cot_shl && found.size() == 1 && stripCasts(bare->y) == found.front().first
								   ? bare
								   : nullptr;
		if (!lone && !shift)
			return mark("one term of a size used as " + *spelled);
		// a class: src's sizeof pinned, else one object its Construct builds there (pinned or not); an unpinned size
		// may be a layout too small, whose multiple only looks like an array
		if (of && !of->pinned && !(element.constructs == of->name && lone == bare && bytes == *width))
			return mark("no size check for " + of->name);
		if (bytes % *width != 0)
			return mark("not a multiple of sizeof(" + *spelled + "), " + std::to_string(*width) + " bytes");
		int64_t count = bytes / *width;
		// a Construct builds one object: more bytes hold it inside another object (a record whose first member it is)
		if (of && count > 1 && !element.constructs.empty())
			return mark("one " + *spelled + " its Construct builds, " + std::to_string(*width) + " bytes");
		// k objects of a class are an array only where the pointer cannot hold one object of another class
		if (of && count > 1 && (of->vfptr || !of->vtables.empty()))
			return mark(std::to_string(count) + " objects of " + *spelled + ", a class with a vtable");
		if (of && count > 1 && derived(of->name))
			return mark(std::to_string(count) + " objects of " + *spelled + ", a class others derive from");
		std::string header = of && isHeader(of->file) && of->file.starts_with("src/") ? of->file.substr(4) : "";
		std::string form = "sizeof(" + *spelled + ")";
		if (count > 1)
			form = std::to_string(count) + " * " + form;
		if (shift)
			m_function.meaning[shift] = Rewrite{1, "$0 * " + form, {shift->x}, {}, "", header};
		else
			m_function.meaning[lone] = Rewrite{1, form, {}, {}, "", header};
	}

	// What src's declarations use the allocation's result as: the pointee of each src declaration it goes to
	// (declared) and each Hex-Rays type it is cast to or held in on the way (seen).
	struct Element
	{
		std::string type;	 // the element: a src type key as src spells it (`uint16_t`)
		bool bytes = false;	 // 1-byte characters, or untyped memory nothing uses as anything else
		std::string why;	 // why it has none: the spot's reason
		std::string constructs;  // the class whose Construct takes it as apMemory ("": none)
	};
	struct Uses
	{
		std::vector<std::string> declared, seen;  // src type keys of pointers
		std::string constructs;
	};

	// The result goes to src declarations past Hex-Rays' casts: the function's return (src's declared return type, of
	// the PDB's or src's evidence), a src field or global it is assigned to, a parameter (of evidence) of a resolved
	// call it is passed to (A::Construct's apMemory: an A), or a local the locals pass paired with the debug build's
	// local of that type. Through a local it is assigned to that nothing else writes and whose address is never
	// taken, every use of that local counts as well. Their pointees, Hex-Rays' (the casts, the local's type) beside
	// them, must agree: one type (Hex-Rays' pointer to a base of a declared class holds that class's object), a 1-byte
	// character or bool for bytes (any of them alike); a void* declaration says nothing but untyped memory, which no
	// use may take as anything else.
	Element elementOf(const cexpr_t* call) const
	{
		Element element;
		auto none = [&](std::string why) {
			element.why = std::move(why);
			return element;
		};
		Uses uses;
		sink(climb(call, uses), uses, true);
		if (uses.declared.empty())
			return none("no src declaration types the result");
		// the distinct pointees, bytes as one, void apart: (the class key, as spelled, a declaration's)
		struct Pointee
		{
			std::string key, type;
			bool declared = false;
		};
		std::vector<Pointee> distinct;
		auto add = [&](const std::string& pointer, bool declared) {
			SrcTypes::Parts p = m_context.types.parts(m_context.types.bare(pointer));
			if (p.kind != "ptr")
				return;
			// the pointee as src spells it (`hkUlong`); a type of the host's width (`uintptr_t`) is another type than
			// a fixed-width one of its size on the exe's target (`uint32_t`)
			std::string type = p.of, key = m_context.types.canonical(type);
			if (key == "builtin:void")
				return;
			if (key == "builtin:char" || key == "builtin:signed char" || key == "builtin:unsigned char" ||
				key == "builtin:bool")
				key = "bytes";
			else if (hostWidth(m_context, type))
				key += " (host width)";
			for (Pointee& d : distinct)
				if (d.key == key)
				{
					d.declared = d.declared || declared;
					return;
				}
			distinct.push_back({key, type, declared});
		};
		for (const std::string& pointer : uses.declared)
			add(pointer, true);
		for (const std::string& pointer : uses.seen)
			add(pointer, false);
		// the most derived declared class, of which every other declared pointee is a base: a pointer to a base of it,
		// Hex-Rays' or a declaration's, holds its object, as C++'s upcast does
		auto record = [&](const Pointee& d) {
			SrcTypes::Parts p = m_context.types.parts(d.key);
			return p.kind == "record" ? p.name : "";
		};
		for (const Pointee& d : distinct)
		{
			std::string most = d.declared ? record(d) : "";
			if (most.empty() || !std::all_of(distinct.begin(), distinct.end(), [&](const Pointee& o) {
					return !o.declared || (!record(o).empty() && derives(m_context.model, most, record(o)));
				}))
				continue;
			Pointee kept = d;
			std::erase_if(distinct, [&](const Pointee& o) {
				return !record(o).empty() && derives(m_context.model, most, record(o));
			});
			distinct.insert(distinct.begin(), kept);
			break;
		}
		auto spell = [&](const Pointee& d) { return m_context.types.spell(d.type).value_or(d.type); };
		// two types: two src declarations disagree, or Hex-Rays' (a cast, the local's type) is not the declared one
		if (distinct.size() > 1 && !distinct[0].declared)
			return none("a void* result used as " + spell(distinct[0]) + " and " + spell(distinct[1]));
		if (distinct.size() > 1 && distinct[1].declared)
			return none("src declarations use the result as " + spell(distinct[0]) + " and " + spell(distinct[1]));
		if (distinct.size() > 1)
			return none("used as " + spell(distinct[0]) + ", which Hex-Rays types " + spell(distinct[1]));
		if (!distinct.empty() && !distinct[0].declared)
			return none("a void* result used as " + spell(distinct[0]));
		element.bytes = distinct.empty() || distinct[0].key == "bytes";
		if (!element.bytes)
		{
			element.type = distinct[0].type;
			element.constructs = uses.constructs;
		}
		return element;
	}

	// the value past the casts Hex-Rays puts around it, each cast's type seen
	const cexpr_t* climb(const cexpr_t* e, Uses& uses) const
	{
		for (auto up = m_parents.find(e); up != m_parents.end() && up->second->is_expr() &&
										  static_cast<const cexpr_t*>(up->second)->op == cot_cast;
			 up = m_parents.find(e))
		{
			e = static_cast<const cexpr_t*>(up->second);
			std::string why;
			if (std::optional<std::string> key = m_context.types.keyOf(e->type, why))
				uses.seen.push_back(*key);
		}
		return e;
	}

	// where the value goes (elementOf); follow: through a local it is assigned to, that local's uses
	void sink(const cexpr_t* value, Uses& uses, bool follow) const
	{
		auto up = m_parents.find(value);
		const SrcModel::Function* self = m_function.self;
		if (up == m_parents.end() || !up->second)
			return;
		if (!up->second->is_expr())
		{
			if (up->second->op == cit_return && self && self->slotEvidence("ret") && !m_function.returnsReference)
				uses.declared.push_back(m_function.returnType);
			return;
		}
		const cexpr_t* parent = static_cast<const cexpr_t*>(up->second);
		if (parent->op == cot_asg && parent->y == value)
		{
			if (int v = lvarIndex(parent->x); v >= 0)
				local(v, parent, uses, follow);
			else if (Destination d = destinationOf(m_function, m_context, m_parents, value); !d.type.empty())
				uses.declared.push_back(d.type);
			return;
		}
		if (parent->op != cot_call || parent->x == value)
			return;
		// the apMemory of A::Construct: one A built there
		if (std::string cls = constructs(parent, value); !cls.empty())
		{
			uses.constructs = cls;
			return uses.declared.push_back("ptr(record:" + cls + ")");
		}
		auto meaning = m_function.meaning.find(parent);
		const Call* resolved = meaning == m_function.meaning.end() ? nullptr : std::get_if<Call>(&meaning->second);
		for (size_t k = 0; resolved && k < parent->a->size(); ++k)
			if (&(*parent->a)[k] == value)
			{
				int parameter = parameterIndex(*resolved, k);
				Destination d = destinationOf(m_function, m_context, m_parents, value);
				if (parameter >= 0 && resolved->function->slotEvidence(std::to_string(parameter)) &&
					!d.byReference && !d.type.empty())
					uses.declared.push_back(d.type);
			}
	}

	// the local v the value is assigned to (sink): its type seen, a declaration where the locals pass paired it with
	// the debug build's local of that type; follow: where the assignment is its only write and its address is never
	// taken, where each of its uses goes
	void local(int v, const cexpr_t* assignment, Uses& uses, bool follow) const
	{
		const Local& local = m_function.locals[v];
		if (local.parameter >= 0 || local.reference || local.storage || local.type.empty())
			return;
		uses.seen.push_back(local.type);
		if (paired(v))
			uses.declared.push_back(local.type);
		if (!follow)
			return;
		Uses more;
		bool fits = true;
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			if (!fits || lvarIndex(e) != v || e == assignment->x)
				return;
			auto up = m_parents.find(e);
			const cexpr_t* parent =
				up != m_parents.end() && up->second && up->second->is_expr() ? static_cast<const cexpr_t*>(up->second)
																			  : nullptr;
			if (parent && (parent->op == cot_ref || (written(parent) && parent->x == e)))
			{
				fits = false;
				return;
			}
			sink(climb(e, more), more, false);
		});
		if (!fits)
			return;
		uses.declared.insert(uses.declared.end(), more.declared.begin(), more.declared.end());
		uses.seen.insert(uses.seen.end(), more.seen.begin(), more.seen.end());
		if (!more.constructs.empty())
			uses.constructs = more.constructs;
	}

	// the local is one the locals pass paired with the debug build's local of its type (its name, or the name before
	// a split's `_N`)
	bool paired(int v) const
	{
		const SrcModel::Function* self = m_function.self;
		const lvar_t& lvar = (*m_function.cfunc->get_lvars())[v];
		if (!self || !lvar.has_user_name())
			return false;
		std::string name = lvar.name.c_str();
		size_t underscore = name.rfind('_');
		bool split = underscore != std::string::npos && underscore > 0 && underscore + 1 < name.size() &&
					 name.find_first_not_of("0123456789", underscore + 1) == std::string::npos;
		const std::string* pdb = m_context.model.pdbTyped("local", self->mangled, name);
		if (!pdb && split)
			pdb = m_context.model.pdbTyped("local", self->mangled, name.substr(0, underscore));
		return pdb && m_context.types.canonical(*pdb) == m_context.types.canonical(m_function.locals[v].type);
	}

	// some src record names the class as a direct base
	bool derived(const std::string& cls) const
	{
		for (const auto& [name, record] : m_context.model.records())
			for (const SrcModel::Base& base : record.bases)
				if (base.record == cls)
					return true;
		return false;
	}

	// the table's row of the callee: a direct call's address, or for a call through a vtable of a src virtual, the
	// row of an override of it (`D::m` where D is or derives from the virtual's class); nullptr for none
	const Allocator* allocatorOf(const cexpr_t* call) const
	{
		const std::map<uint64_t, Allocator>& table = m_context.allocators;
		if (call->x->op == cot_obj)
		{
			auto found = table.find(call->x->obj_ea);
			return found == table.end() ? nullptr : &found->second;
		}
		auto meaning = m_function.meaning.find(call);
		const Call* resolved = meaning == m_function.meaning.end() ? nullptr : std::get_if<Call>(&meaning->second);
		if (!resolved || !resolved->function->isVirtual || resolved->function->cls.empty())
			return nullptr;
		const std::string& name = resolved->function->name;
		std::string member = name.substr(name.rfind("::") + 2);
		for (const auto& [address, allocator] : table)
		{
			size_t colons = allocator.name.rfind("::");
			if (colons != std::string::npos && allocator.name.substr(colons + 2) == member &&
				derives(m_context.model, allocator.name.substr(0, colons), resolved->function->cls))
				return &allocator;
		}
		return nullptr;
	}

	// the numbers the size's +, -, * and << combine (past casts), each with the factor it stands for (a shift's
	// amount: 1 << it)
	static void literals(const cexpr_t* e, std::vector<std::pair<const cexpr_t*, int64_t>>& out, bool shift)
	{
		e = stripCasts(e);
		if (e->op == cot_num)
		{
			int64_t value = static_cast<int64_t>(e->numval());
			out.emplace_back(e, shift ? (value >= 0 && value < 63 ? int64_t(1) << value : 0) : value);
		}
		else if (e->op == cot_add || e->op == cot_sub || e->op == cot_mul || e->op == cot_shl)
		{
			literals(e->x, out, false);
			literals(e->y, out, e->op == cot_shl);
		}
	}

	// the record the allocation is used as, one object of it: the record a cast of it points to, the one the pointer
	// it is assigned to does (src's type), the class whose Construct takes it as apMemory, directly or through the
	// local it is assigned to (constructedAs); all of them agree. constructed: a Construct call is among them. nullptr
	// for none.
	const SrcModel::Record* usedAs(const cexpr_t* call, bool& constructed) const
	{
		std::string record;
		bool disagree = false;
		auto take = [&](const std::string& key) {
			SrcTypes::Parts p = m_context.types.parts(m_context.types.canonical(key));
			SrcTypes::Parts of = p.kind == "ptr" ? m_context.types.parts(p.of) : SrcTypes::Parts{};
			if (of.kind != "record")
				return;
			disagree = disagree || (!record.empty() && record != of.name);
			record = of.name;
		};
		const cexpr_t* e = call;
		for (;;)
		{
			auto found = m_parents.find(e);
			if (found == m_parents.end() || !found->second->is_expr())
				break;
			const cexpr_t* parent = static_cast<const cexpr_t*>(found->second);
			if (parent->op == cot_cast)
			{
				std::string why;
				if (std::optional<std::string> key = m_context.types.keyOf(parent->type, why))
					take(*key);
				e = parent;
				continue;
			}
			if (parent->op == cot_asg && parent->y == e)
			{
				take(srcType(m_function, m_context, parent->x));
				if (std::string cls = constructedAs(parent); !cls.empty())
				{
					take("ptr(record:" + cls + ")");
					constructed = true;
				}
			}
			else if (std::string cls = constructs(parent, e); !cls.empty())
			{
				take("ptr(record:" + cls + ")");
				constructed = true;
			}
			break;
		}
		return disagree || record.empty() ? nullptr : m_context.model.record(record);
	}

	// the class of the resolved `X::Construct` call that takes the value (past casts) as its apMemory; "" for any other
	std::string constructs(const cexpr_t* parent, const cexpr_t* value) const
	{
		if (parent->op != cot_call || parent->x == value || parent->a->empty() || &(*parent->a)[0] != value)
			return "";
		auto meaning = m_function.meaning.find(parent);
		const Call* construct = meaning == m_function.meaning.end() ? nullptr : std::get_if<Call>(&meaning->second);
		const std::string name = construct ? construct->function->name : "";
		return construct && !construct->function->cls.empty() &&
					   structorName(name.substr(name.rfind("::") + 2)) == "Construct"
				   ? construct->function->cls
				   : "";
	}

	// The class the allocation assigned to a local is next constructed as: the local (not a parameter, a reference or
	// raw storage) is written by that assignment alone, its address never taken, and every other use of it is a null
	// test (`if (p)`, `!p`, `p == 0`, `p != 0`) or the apMemory of one class's Construct (past casts), at least one.
	// "" for any other.
	std::string constructedAs(const cexpr_t* assignment) const
	{
		int v = lvarIndex(assignment->x);
		if (v < 0 || (*m_function.cfunc->get_lvars())[v].is_arg_var() || m_function.locals[v].parameter >= 0 ||
			m_function.locals[v].reference || m_function.locals[v].storage)
			return "";
		std::string cls;
		bool fits = true;
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t* s) {
			if (!fits || lvarIndex(e) != v || e == assignment->x)
				return;
			const cexpr_t* use = e;
			auto up = m_parents.find(use);
			while (up != m_parents.end() && up->second->is_expr() &&
				   static_cast<const cexpr_t*>(up->second)->op == cot_cast)
			{
				use = static_cast<const cexpr_t*>(up->second);
				up = m_parents.find(use);
			}
			if (up == m_parents.end())
			{
				fits = false;
				return;
			}
			if (!up->second->is_expr())
			{
				// the condition of an if
				fits = up->second == s && s->op == cit_if && s->cif->expr.op != cot_empty && &s->cif->expr == use;
				return;
			}
			const cexpr_t* parent = static_cast<const cexpr_t*>(up->second);
			auto zero = [](const cexpr_t* x) { return x->op == cot_num && x->numval() == 0; };
			if (parent->op == cot_lnot || parent->op == cot_land || parent->op == cot_lor ||
				(parent->op == cot_tern && parent->x == use) ||
				((parent->op == cot_eq || parent->op == cot_ne) &&
				 zero(stripCasts(parent->x == use ? parent->y : parent->x))))
				return;
			std::string of = constructs(parent, use);
			fits = !of.empty() && (cls.empty() || cls == of);
			cls = of;
		});
		return fits ? cls : "";
	}

	Function& m_function;
	Context& m_context;
	std::unordered_map<const cexpr_t*, const citem_t*> m_parents;
};

}  // namespace

void markAllocations(Function& function, Context& context)
{
	Allocations(function, context).run();
}

}  // namespace idakit
