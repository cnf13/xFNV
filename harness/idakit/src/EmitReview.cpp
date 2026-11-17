// Review: integer literals a person should look at, the arbitrary ones (`0x80000000u`): those the printer writes in
// hex, 0x1000 and up in magnitude at the literal's size and sign, that no layer gave a meaning beyond its type
// (enumerators, unresolved). They print with a `/* review: ... */` comment, a note that never keeps the function
// marked (user, 2026-11-09). None inside an item printed with its own marker (an ancestor unresolved, read as another
// type or converted explicitly with a reason): that marker already makes the person look at its text. It runs after
// every layer that marks an item. Off: `idakit emit --skip-pass flagLiterals`.

#include "EmitPasses.hpp"

namespace idakit
{

namespace
{

// the literal's magnitude at its type's size and sign (as the printer writes it)
uint64_t magnitude(const cexpr_t* number)
{
	size_t size = number->type.get_size();
	if (size == 0 || size > 8)
		size = 4;
	uint64_t mask = size == 8 ? ~0ull : (1ull << (size * 8)) - 1;
	uint64_t value = number->n->_value & mask;
	bool isSigned = !number->type.is_unsigned() && !number->type.is_ptr();
	if (isSigned && (value >> (size * 8 - 1)) & 1)
		return static_cast<uint64_t>(-static_cast<int64_t>(value | ~mask));
	return value;
}

// the item prints with a marker of its own around its text (EmitPrinter's mark(), global(), convert(), call()):
// unresolved, a src global read as another type, an explicit conversion with a reason, a call no cast pins to its
// overload; of statements, only a return's text is marked as a whole
bool marked(const Function& function, const citem_t* item)
{
	if (!item->is_expr() && item->op != cit_return)
		return false;
	if (item->is_expr())
		if (auto c = function.conversions.find(static_cast<const cexpr_t*>(item));
			c != function.conversions.end() && !c->second.what.empty())
			return true;
	auto found = function.meaning.find(item);
	if (found == function.meaning.end())
		return false;
	if (const Call* call = std::get_if<Call>(&found->second))
		return !call->unpinned.empty();
	return std::holds_alternative<Missing>(found->second) || std::holds_alternative<GlobalAs>(found->second);
}

}  // namespace

void flagLiterals(Function& function, Context&)
{
	std::unordered_map<const cexpr_t*, const citem_t*> parents = parentsOf(function);
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
		if (e->op != cot_num || e->type.is_ptr() || e->type.is_bool() || magnitude(e) < 0x1000)
			return;
		auto found = function.meaning.find(e);
		bool plain = found == function.meaning.end() || std::holds_alternative<std::monostate>(found->second) ||
					 std::holds_alternative<std::string>(found->second);
		// none on a literal printed inside its own marker (its conversion's), nor inside an ancestor's
		if (!plain || marked(function, e))
			return;
		for (auto up = parents.find(e); up != parents.end(); up = parents.find(static_cast<const cexpr_t*>(up->second)))
		{
			if (marked(function, up->second))
				return;
			if (!up->second->is_expr())
				break;
		}
		function.meaning[e] = Review{"arbitrary literal"};
	});
}

}  // namespace idakit
