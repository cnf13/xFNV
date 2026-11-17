// dynamic_cast: the exe's calls of __RTDynamicCast (0xEC43FB) as src writes them (.omp/rules/porting.md "Casts"):
//
//     static_cast<Dst*>(msvc9::__RTDynamicCast(object, 0, &RTTI_Src, &RTTI_Dst, 0))
//
// Hex-Rays passes the object through `(void*)` and the descriptors through casts to `_s_RTTICompleteObjectLocator*` /
// `TypeDescriptor*`; C++ converts all of them to the declared `void*` / `const void*` implicitly, so they print as
// their operands. Each descriptor must be a src global (BSCore/RTTI.h's RTTI_X, by the Globals pass); otherwise the
// call stays as the Calls pass left it. The result, a void*, is static_cast to the type Hex-Rays casts or assigns it
// to; used any other way than a cast, an assignment or a test, the call stays unresolved.

#include "EmitPasses.hpp"

#include "idakit/SrcIda.hpp"

namespace idakit
{

namespace
{

constexpr ea_t kRTDynamicCast = 0xEC43FB;

class DynamicCasts
{
public:
	DynamicCasts(Function& function, Context& context)
		: m_function(function), m_context(context), m_parents(parentsOf(function))
	{
		RefChoice choice = chooseRef(context.model, kRTDynamicCast);
		m_declaration = choice.ref ? context.model.function(choice.ref->function) : nullptr;
	}

	void run()
	{
		if (!m_declaration)
			return;
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			if (e->op == cot_call && e->x->op == cot_obj && e->x->obj_ea == kRTDynamicCast && e->a->size() == 5)
				resolve(e);
		});
	}

private:
	void resolve(const cexpr_t* call)
	{
		const carglist_t& args = *call->a;
		auto pointer = [](const cexpr_t* e) { return e->type.is_ptr() && (e->op != cot_cast || e->x->type.is_ptr()); };
		// the object: no layer left anything unresolved in it (Hex-Rays' cast to void* aside, which C++ does)
		const cexpr_t* object = args[0].op == cot_cast ? args[0].x : &args[0];
		if (unresolvedIn(m_function, object) || !pointer(&args[0]) || !isZero(&args[1]) || args[4].op != cot_num ||
			!pointer(&args[2]) || !pointer(&args[3]) || !descriptor(&args[2]) || !descriptor(&args[3]))
			return;
		// what the void* result becomes: a cast's type, an assignment's target type, or a test (no conversion)
		auto found = m_parents.find(call);
		const citem_t* parent = found == m_parents.end() ? nullptr : found->second;
		const citem_t* conversion = nullptr;
		std::string to;
		if (parent && parent->op == cot_cast)
		{
			conversion = parent;
			to = srcKey(static_cast<const cexpr_t*>(parent)->type);
		}
		else if (parent && parent->op == cot_asg && static_cast<const cexpr_t*>(parent)->y == call)
		{
			conversion = parent;
			to = srcType(m_function, m_context, static_cast<const cexpr_t*>(parent)->x);
		}
		else if (!parent || !test(parent, call))
			return;
		if (conversion && (to.empty() || m_context.types.parts(m_context.types.canonical(to)).kind != "ptr"))
			return;
		for (size_t k : {0, 2, 3})
			if (args[k].op == cot_cast)
				m_function.meaning[&args[k]] = Implicit{};
		m_function.meaning[call] = Call{m_declaration, false, false, std::vector<bool>(5, false),
										std::vector<std::string>(5)};
		if (conversion)
			m_function.meaning[conversion] = to;
	}

	// the parent tests the call's result for null (`if (r)`, `!r`, `r && ...`, `r == 0`)
	static bool test(const citem_t* parent, const cexpr_t* call)
	{
		switch (parent->op)
		{
		case cit_if:
		case cit_while:
		case cit_do:
		case cot_lnot:
		case cot_land:
		case cot_lor:
			return true;
		case cot_eq:
		case cot_ne:
		{
			const cexpr_t* other = static_cast<const cexpr_t*>(parent)->y;
			return other != call && isZero(other);
		}
		default:
			return false;
		}
	}

	// `&RTTI_X`, through casts: the address of one of BSCore/RTTI.h's descriptors
	bool descriptor(const cexpr_t* e) const
	{
		e = stripCasts(e);
		if (e->op != cot_ref || e->x->op != cot_obj)
			return false;
		auto found = m_function.meaning.find(e->x);
		const SrcModel::Ref* const* ref =
			found == m_function.meaning.end() ? nullptr : std::get_if<const SrcModel::Ref*>(&found->second);
		return ref && (*ref)->file.ends_with("BSCore/RTTI.h");
	}

	std::string srcKey(const tinfo_t& type) const
	{
		std::string why;
		return m_context.types.keyOf(type, why).value_or("");
	}

	Function& m_function;
	Context& m_context;
	std::unordered_map<const cexpr_t*, const citem_t*> m_parents;
	const SrcModel::Function* m_declaration = nullptr;
};

}  // namespace

void resolveDynamicCasts(Function& function, Context& context)
{
	DynamicCasts(function, context).run();
}

}  // namespace idakit
