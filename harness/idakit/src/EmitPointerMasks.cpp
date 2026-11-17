// Pointer masks: a flag the exe keeps in a pointer's low bits (hkStringPtr's owned flag in m_stringAndFlag), masked
// off by Hex-Rays' `(int)p & 0xFFFFFFFE`: the pointer's bits as a 4-byte integer, which C++ writes through uintptr_t
// on any layout (src's hkStringPtr::cString). Where p prints as a src pointer, the mask clears only bits below 16
// (an alignment's: `~k`, k < 16, as the exe's 4-byte constant) and the masked value is only tested for zero or cast
// back to a pointer to p's pointee (its cv apart), the mask prints `(reinterpret_cast<uintptr_t>(p) & ~uintptr_t(k))`
// and the cast back `reinterpret_cast<P>(reinterpret_cast<uintptr_t>(p) & ~uintptr_t(k))`, P p's own type: the
// pointer the exe reads through. Any other use (a 4-byte local or parameter holding the masked bits) stays marked.

#include "EmitPasses.hpp"

#include <set>

namespace idakit
{

namespace
{

class PointerMasks
{
public:
	PointerMasks(Function& function, Context& context)
		: m_function(function), m_context(context), m_types(context.types), m_parents(parentsOf(function))
	{
	}

	void run()
	{
		std::vector<const cexpr_t*> masks;
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			if (e->op == cot_band && !m_function.meaning.count(e))
				masks.push_back(e);
		});
		for (const cexpr_t* mask : masks)
			rewrite(mask);
	}

private:
	void rewrite(const cexpr_t* mask)
	{
		const cexpr_t* bits = mask->x->op == cot_cast ? mask->x : mask->y;
		const cexpr_t* number = bits == mask->x ? mask->y : mask->x;
		if (bits->op != cot_cast || number->op != cot_num || !bits->x->type.is_ptr() || !integer(bits->type) ||
			!integer(mask->type))
			return;
		uint32_t k = ~static_cast<uint32_t>(number->numval());
		if (k == 0 || k >= 16)
			return;
		const cexpr_t* pointer = bits->x;
		std::string key = srcType(m_function, m_context, pointer);
		std::optional<std::string> spelled = key.empty() ? std::nullopt : m_types.spell(key);
		if (!spelled || m_types.parts(m_types.canonical(key)).kind != "ptr" || unresolvedIn(m_function, pointer))
			return;
		std::string masked = "reinterpret_cast<uintptr_t>($0) & ~uintptr_t(" + std::to_string(k) + ")";
		auto parent = m_parents.find(mask);
		const cexpr_t* back = parent != m_parents.end() && parent->second->op == cot_cast
								  ? static_cast<const cexpr_t*>(parent->second)
								  : nullptr;
		if (back && pointee(back, key))
			replace(back, pointer, Rewrite{1, "reinterpret_cast<" + *spelled + ">(" + masked + ")", {pointer}, {},
										   key, ""});
		else if (truthTested(mask, m_parents))
			replace(mask, pointer, Rewrite{1, "(" + masked + ")", {pointer}, {}, "typedef:uintptr_t", ""});
	}

	// a 4-byte integer (not bool or enum): Hex-Rays' reading of a pointer's bits on the exe's layout
	static bool integer(const tinfo_t& type)
	{
		return type.is_integral() && !type.is_bool() && !type.is_enum() && type.get_size() == 4;
	}

	// the cast back is to a pointer to the masked pointer's own pointee, its cv apart
	bool pointee(const cexpr_t* cast, const std::string& key) const
	{
		std::string why;
		std::optional<std::string> to = cast->type.is_ptr() ? m_types.keyOf(cast->type, why) : std::nullopt;
		return to && (m_types.canonical(*to) == m_types.canonical(key) || cvApart(m_types, key, *to));
	}

	// the form replaces item: what it does not print (all but the pointer's own items) loses its meaning
	void replace(const cexpr_t* item, const cexpr_t* pointer, Rewrite rewrite)
	{
		std::set<const cexpr_t*> kept;
		forEachSubexpression(pointer, [&](const cexpr_t* x) { kept.insert(x); });
		forEachSubexpression(item, [&](const cexpr_t* x) {
			if (!kept.count(x))
				m_function.meaning.erase(x);
		});
		m_function.meaning[item] = std::move(rewrite);
	}

	Function& m_function;
	Context& m_context;
	SrcTypes& m_types;
	std::unordered_map<const cexpr_t*, const citem_t*> m_parents;
};

}  // namespace

void rewritePointerMasks(Function& function, Context& context)
{
	PointerMasks(function, context).run();
}

}  // namespace idakit
