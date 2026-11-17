// Enums: a number whose own type is a src enum, or whose place gives it one (compared with or assigned to an enum
// value, passed for an enum parameter, returned as the function's enum, a bitwise operand beside an enum value),
// prints as its enumerator (the first declared one of that value); so does each value of a switch case on an enum.
// `NAME` for an enum at namespace scope, `Scope::NAME` for one in a class, `Enum::NAME` for a scoped enum. A value
// that is no one enumerator of a flag enum but an OR of its flags, every bit covered, prints as that OR: cast to the
// enum in its place (`static_cast<E>(A | B)`), bare as a bitwise operand (`x & (A | B)`); a mask's operand whose
// complement is such an OR prints as its complement (`x & ~A`). A bitwise operand that only the other operand puts in
// the enum's place takes these forms only where the operator's C++ result keeps Hex-Rays' width and signedness or its
// use ignores them. Any other value in an enum's place is unresolved and keeps a cast to the enum; a bitwise operand
// that is none of these keeps its literal. A number in no enum's place beside a field (an operand of &, |, ^, == or !=
// with it, or stored into it) that is exactly one enumerator of the field's record's unnamed member enums, its
// constants, prints as `Record::NAME` (classConstant).

#include "EmitPasses.hpp"

#include "IdaSupport.hpp"

namespace idakit
{

namespace
{

bool comparison(ctype_t op)
{
	return op == cot_eq || op == cot_ne || (op >= cot_sge && op <= cot_ult);
}

bool bitwise(ctype_t op)
{
	return op == cot_band || op == cot_bor || op == cot_xor;
}

int popcount(uint64_t v)
{
	return __builtin_popcountll(v);
}

class Enums
{
public:
	Enums(Function& function, Context& context) : m_function(function), m_context(context)
	{
		// the values setters' accesses store (FoldedAccessors' Access), by the field they go to
		for (const auto& [item, meaning] : m_function.meaning)
			if (const Access* access = std::get_if<Access>(&meaning); access && access->value && access->field)
				m_stored[access->value] = access->field;
	}

	// the enum a key names (typedefs and cv peeled), nullptr for any other type
	const SrcModel::Enum* enumOf(const std::string& key) const
	{
		if (key.empty())
			return nullptr;
		SrcTypes::Parts p = m_context.types.parts(m_context.types.canonical(key));
		return p.kind == "enum" ? m_context.model.enumeration(p.name) : nullptr;
	}

	// the first declared enumerator of the value, or what is missing
	std::variant<std::string, Missing> enumerator(const SrcModel::Enum& e, uint64_t value) const
	{
		std::optional<int64_t> size = m_context.model.sizeOf("enum:" + e.name);
		uint64_t mask = !size || *size >= 8 ? ~0ull : (1ull << (*size * 8)) - 1;
		for (const SrcModel::Enumerator& n : e.enumerators)
			if ((static_cast<uint64_t>(n.value) & mask) == (value & mask))
			{
				std::string scope = e.scoped ? e.name : scopeOf(e.name);
				return scope.empty() ? n.name : scope + "::" + n.name;
			}
		return Missing{"no enumerator of " + e.name + " is " + std::to_string(int64_t(value & mask))};
	}

	// the enum's values as its size keeps them
	uint64_t maskOf(const SrcModel::Enum& e) const
	{
		std::optional<int64_t> size = m_context.model.sizeOf("enum:" + e.name);
		return !size || *size >= 8 ? ~0ull : (1ull << (*size * 8)) - 1;
	}

	// a flag enum: unscoped (C++ ORs its enumerators), two single-bit enumerators at least, every other nonzero one an
	// OR of those (a mask), its values not a count 1, 2, 3, ...
	bool flagEnum(const SrcModel::Enum& e) const
	{
		if (e.scoped)
			return false;
		uint64_t mask = maskOf(e), singles = 0;
		std::set<uint64_t> values;
		int singleCount = 0;
		for (const SrcModel::Enumerator& n : e.enumerators)
			if (uint64_t v = static_cast<uint64_t>(n.value) & mask; v && values.insert(v).second && popcount(v) == 1)
			{
				singles |= v;
				++singleCount;
			}
		if (singleCount < 2 || (values.size() >= 3 && *values.rbegin() == values.size()))
			return false;
		for (uint64_t v : values)
			if (v & ~singles)
				return false;
		return true;
	}

	// the flags of a flag enum whose OR is exactly the value, two or more, in declaration order; none where no OR of
	// its enumerators covers every bit. The widest enumerator inside the value goes first (a named mask before its
	// bits), each one disjoint from those taken; where that leaves bits, the single-bit ones alone.
	std::optional<std::vector<std::string>> flagsOf(const SrcModel::Enum& e, uint64_t value) const
	{
		uint64_t mask = maskOf(e);
		value &= mask;
		if (!value || !flagEnum(e))
			return std::nullopt;
		std::vector<size_t> order(e.enumerators.size());
		for (size_t k = 0; k < order.size(); ++k)
			order[k] = k;
		auto bits = [&](size_t k) { return static_cast<uint64_t>(e.enumerators[k].value) & mask; };
		std::stable_sort(order.begin(), order.end(),
						 [&](size_t a, size_t b) { return popcount(bits(a)) > popcount(bits(b)); });
		uint64_t covered = 0;
		std::vector<size_t> taken;
		auto cover = [&](bool singlesOnly) {
			covered = 0;
			taken.clear();
			for (size_t k : order)
				if (uint64_t v = bits(k); v && !(v & ~value) && !(v & covered) && (!singlesOnly || popcount(v) == 1))
				{
					covered |= v;
					taken.push_back(k);
				}
		};
		cover(false);
		if (covered != value)
			cover(true);
		if (covered != value || taken.size() < 2)
			return std::nullopt;
		std::sort(taken.begin(), taken.end());
		std::string scope = scopeOf(e.name);
		std::vector<std::string> names;
		for (size_t k : taken)
			names.push_back(scope.empty() ? e.enumerators[k].name : scope + "::" + e.enumerators[k].name);
		return names;
	}

	// the enum a number's place gives it
	const SrcModel::Enum* placeOf(const cexpr_t* number, const citem_t* parent) const
	{
		if (const SrcModel::Enum* own = enumOf(srcType(m_function, m_context, number)))
			return own;
		// stored by a setter's access: its field's (Hex-Rays' casts of the value are not printed)
		if (auto stored = m_stored.find(number); stored != m_stored.end())
			return enumOf(stored->second->type);
		if (!parent)
			return nullptr;
		if (parent->op == cit_return)
			return enumOf(m_function.returnType);
		if (parent->op >= cit_empty)
			return nullptr;
		const cexpr_t* p = static_cast<const cexpr_t*>(parent);
		if (comparison(p->op))
			return enumOf(srcType(m_function, m_context, p->x == number ? p->y : p->x));
		if (p->op == cot_asg && p->y == number)
			return enumOf(srcType(m_function, m_context, p->x));
		if (p->op == cot_cast)
			return enumOf(srcType(m_function, m_context, p));
		if (p->op == cot_call)
			if (const Call* call = callAt(p))
				if (int k = argument(*call, p, number); k >= 0)
					return enumOf(m_context.model.type(call->function->type)->params[parameterIndex(*call, k)]);
		// a bitwise operand beside a flag enum's value
		if (bitwise(p->op))
			if (const SrcModel::Enum* e = enumOf(srcType(m_function, m_context, p->x == number ? p->y : p->x));
				e && flagEnum(*e))
				return e;
		return nullptr;
	}

	// a resolved call's meaning, without adding an entry for an item no pass gave one
	Call* callAt(const citem_t* item) const
	{
		auto found = m_function.meaning.find(item);
		return found == m_function.meaning.end() ? nullptr : std::get_if<Call>(&found->second);
	}

	// the argument index of the value in a resolved call, past the object and the hidden result pointer (a declared
	// parameter takes it), else -1
	int argument(const Call& call, const cexpr_t* e, const cexpr_t* value) const
	{
		const SrcModel::Type* type = m_context.model.type(call.function->type);
		for (size_t k = 0; type && k < e->a->size(); ++k)
			if (int parameter = parameterIndex(call, k);
				&(*e->a)[k] == value && parameter >= 0 && static_cast<size_t>(parameter) < type->params.size())
				return static_cast<int>(k);
		return -1;
	}

	// the unnamed member enums each record declares (its constants), by the record's name
	const std::vector<const SrcModel::Enum*>& constantsOf(const std::string& record) const
	{
		static const SrcModel* indexed = nullptr;
		static std::unordered_map<std::string, std::vector<const SrcModel::Enum*>> byRecord;
		if (indexed != &m_context.model)
		{
			indexed = &m_context.model;
			byRecord.clear();
			for (const auto& [name, e] : m_context.model.enums())
				if (size_t at = name.rfind("::(unnamed enum at "); at != std::string::npos && !e.scoped)
					byRecord[name.substr(0, at)].push_back(&e);
		}
		static const std::vector<const SrcModel::Enum*> none;
		auto found = byRecord.find(record);
		return found == byRecord.end() ? none : found->second;
	}

	// a C++ integer type's width and signedness after promotion (an enum by its underlying type)
	struct Arithmetic
	{
		size_t size = 4;
		bool isUnsigned = false;
	};
	static Arithmetic promoted(size_t size, bool isUnsigned)
	{
		return size < 4 ? Arithmetic{4, false} : Arithmetic{size, isUnsigned};
	}
	static Arithmetic common(Arithmetic a, Arithmetic b)
	{
		if (a.size != b.size)
			return a.size > b.size ? a : b;
		return {a.size, a.isUnsigned || b.isUnsigned};
	}

	// what a value of the enum promotes to: its fixed underlying type (src fixes one through a typedef), else the
	// first of int and unsigned int that holds every enumerator; both where a builtin underlying type may be either
	std::vector<Arithmetic> promotions(const SrcModel::Enum& e) const
	{
		std::string underlying = m_context.types.canonical(e.underlying);
		const SrcModel::Type* t = m_context.model.type(underlying);
		bool isUnsigned = t ? !t->isSigned : underlying.find("unsigned") != std::string::npos;
		Arithmetic fixed = promoted(static_cast<size_t>(m_context.model.sizeOf(underlying).value_or(4)), isUnsigned);
		if (e.underlying.starts_with("typedef:") || fixed.size != 4)
			return {fixed};
		bool fitsInt = std::all_of(e.enumerators.begin(), e.enumerators.end(), [](const SrcModel::Enumerator& n) {
			return n.value >= INT32_MIN && n.value <= INT32_MAX;
		});
		if (fixed.isUnsigned == !fitsInt)
			return {fixed};
		return {fixed, Arithmetic{4, !fitsInt}};
	}

	// what the value as printed promotes to: its src type's (an enum's promotions), else Hex-Rays' type's
	std::vector<Arithmetic> promotions(const cexpr_t* x) const
	{
		std::string key = srcType(m_function, m_context, x);
		if (const SrcModel::Enum* e = enumOf(key))
			return promotions(*e);
		const SrcModel::Type* t = key.empty() ? nullptr : m_context.model.type(m_context.types.canonical(key));
		if (t && t->kind == "builtin" && !t->isFloat && t->size > 0)
			return {promoted(static_cast<size_t>(t->size), !t->isSigned)};
		return {promoted(x->type.get_size(), x->type.is_unsigned())};
	}

	// the bitwise operator p's C++ result, with its operand number printed as a value of the enum (an enumerator, an
	// OR or a complement of them, all of which promote as the enum), has Hex-Rays' width and signedness, or its use
	// ignores them (classConstant's test)
	bool keepsArithmetic(const cexpr_t* p, const cexpr_t* number, const SrcModel::Enum& type,
						 std::unordered_map<const cexpr_t*, const citem_t*>& parents) const
	{
		const cexpr_t* other = p->x == number ? p->y : p->x;
		Arithmetic hexRays = promoted(p->type.get_size(), p->type.is_unsigned());
		for (Arithmetic a : promotions(other))
			for (Arithmetic b : promotions(type))
				if (Arithmetic both = common(a, b); both.size != hexRays.size || both.isUnsigned != hexRays.isUnsigned)
					return signednessIgnored(p, parents);
		return true;
	}

	// x's value, whose C++ type has another signedness than Hex-Rays' (an enumerator made it unsigned), is used only
	// where that makes no difference: as bits kept at 4 bytes (stored, converted, passed, compared for equality, an
	// unsigned comparison, division or shift, a condition), through operators that keep the low bits
	bool signednessIgnored(const cexpr_t* x, std::unordered_map<const cexpr_t*, const citem_t*>& parents) const
	{
		for (;;)
		{
			const citem_t* parent = parents[x];
			if (!parent || parent->op >= cit_empty)
				return true;
			const cexpr_t* q = static_cast<const cexpr_t*>(parent);
			const cexpr_t* other = q->x == x ? q->y : q->x;
			bool narrowOther = !other || other->type.get_size() <= 4;
			switch (q->op)
			{
			case cot_eq: case cot_ne: case cot_uge: case cot_ule: case cot_ugt: case cot_ult:
				return narrowOther;
			case cot_udiv: case cot_umod:
				return narrowOther;
			case cot_ushr: case cot_shl: case cot_sshr:
				if (q->y == x)
					return true;
				if (q->op == cot_sshr)
					return false;
				if (q->op == cot_ushr)
					return true;
				break;
			case cot_lnot: case cot_land: case cot_lor:
				return true;
			case cot_tern:
				if (q->x == x)
					return true;
				break;
			case cot_band: case cot_bor: case cot_xor: case cot_add: case cot_sub: case cot_mul: case cot_bnot:
			case cot_neg:
				if (!narrowOther)
					return false;
				break;
			case cot_asg: case cot_asgbor: case cot_asgband: case cot_asgxor: case cot_asgadd: case cot_asgsub:
			case cot_asgmul:
				return q->y == x && q->x->type.get_size() <= 4 && (q->x->type.is_integral() || q->x->type.is_enum());
			case cot_cast:
				return q->type.get_size() <= 4 && (q->type.is_integral() || q->type.is_enum() || q->type.is_bool());
			case cot_call:
				return x->type.get_size() <= 4;
			default:
				return false;
			}
			if (q->type.get_size() > 4)
				return false;
			x = q;
		}
	}

	// A literal beside a field (an operand of &, |, ^, == or != with it, or stored into it) that equals exactly one
	// enumerator of the field's record's unnamed member enums (the record's constants: hkArrayBase's CAPACITY_MASK)
	// prints as that enumerator, `Record::NAME`, where the field holds the enumerator's value as the exe compares it
	// and C++'s result keeps Hex-Rays' signedness or its use ignores it.
	void classConstant(const cexpr_t* number, const citem_t* parent,
					   std::unordered_map<const cexpr_t*, const citem_t*>& parents)
	{
		if (!parent || parent->op >= cit_empty)
			return;
		const cexpr_t* p = static_cast<const cexpr_t*>(parent);
		bool operand = bitwise(p->op) || p->op == cot_eq || p->op == cot_ne;
		bool stored = (p->op == cot_asg || p->op == cot_asgbor || p->op == cot_asgband || p->op == cot_asgxor) &&
					  p->y == number;
		if (!operand && !stored)
			return;
		const cexpr_t* field = p->x == number ? p->y : p->x;
		while (field->op == cot_cast)
			field = field->x;
		auto found = m_function.meaning.find(field);
		const Member* member = (field->op == cot_memptr || field->op == cot_memref) && found != m_function.meaning.end()
								   ? std::get_if<Member>(&found->second)
								   : nullptr;
		size_t size = number->type.get_size(), fieldSize = field->type.get_size();
		if (!member || member->owner.empty() || size == 0 || size > 8 || fieldSize == 0 || fieldSize > 8 ||
			!(field->type.is_integral() || field->type.is_enum()))
			return;
		uint64_t mask = size == 8 ? ~0ull : (1ull << (size * 8)) - 1;
		uint64_t value = number->n->_value & mask;
		if (!value)
			return;
		const SrcModel::Enum* chosen = nullptr;
		const SrcModel::Enumerator* name = nullptr;
		for (const SrcModel::Enum* e : constantsOf(member->owner))
			for (const SrcModel::Enumerator& n : e->enumerators)
			{
				uint64_t v = static_cast<uint64_t>(n.value);
				bool fits = !(v & ~mask) || (v | mask) == ~0ull;
				if (!fits || (v & mask) != value)
					continue;
				if (name && name->name != n.name)
					return;
				chosen = e;
				name = &n;
			}
		if (!name)
			return;
		std::string underlying = m_context.types.canonical(chosen->underlying);
		std::optional<int64_t> enumSize = m_context.model.sizeOf(underlying);
		Arithmetic constant = promoted(enumSize.value_or(4), underlying.find("unsigned") != std::string::npos ||
																underlying == "builtin:bool");
		Arithmetic held = promoted(fieldSize, field->type.is_unsigned());
		if (!stored)
		{
			// compared for equality: the field's value is the enumerator's where the field's type holds it, or where
			// C++ compares the two as one unsigned type of the field's width
			int64_t v = name->value;
			bool representable = field->type.is_unsigned()
									 ? v >= 0 && (fieldSize == 8 || static_cast<uint64_t>(v) >> (fieldSize * 8) == 0)
									 : fieldSize == 8 || (v >= -(1ll << (fieldSize * 8 - 1)) &&
														  v < (1ll << (fieldSize * 8 - 1)));
			Arithmetic both = common(held, constant);
			if (!representable && !(both.isUnsigned && both.size == fieldSize))
				return;
			if (bitwise(p->op))
			{
				Arithmetic hexRays = promoted(p->type.get_size(), p->type.is_unsigned());
				if ((both.size != hexRays.size || both.isUnsigned != hexRays.isUnsigned) &&
					!signednessIgnored(p, parents))
					return;
			}
		}
		m_function.meaning[number] = Enumerators{{member->owner + "::" + name->name}, "enum:" + chosen->name};
	}

	void run()
	{
		std::unordered_map<const cexpr_t*, const citem_t*> parents = parentsOf(m_function);
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			if (e->op != cot_num)
				return;
			const citem_t* parent = parents[e];
			const SrcModel::Enum* type = placeOf(e, parent);
			if (!type)
			{
				classConstant(e, parent, parents);
				return;
			}
			// a bitwise operand: an OR prints bare (the operator's result is an integer anyway); one whose enum only
			// the other operand gives keeps its literal when it is none of the forms
			bool operand = parent && parent->op < cit_empty && bitwise(parent->op);
			bool besideOnly = operand && !enumOf(srcType(m_function, m_context, e));
			// as a value of the enum it would change the operator's C++ width or signedness: the literal stays
			if (besideOnly && !keepsArithmetic(static_cast<const cexpr_t*>(parent), e, *type, parents))
				return;
			std::string key = "enum:" + type->name;
			// a cast of the number to the enum prints as the enumerator as a whole (a setter's stored value prints
			// without Hex-Rays' casts)
			const citem_t* item = parent && parent->op == cot_cast && !m_stored.count(e) ? parent : e;
			uint64_t value = e->n->_value;
			std::variant<std::string, Missing> name = enumerator(*type, value);
			if (std::holds_alternative<Missing>(name))
			{
				using Form = Enumerators::Form;
				if (std::optional<std::vector<std::string>> flags = flagsOf(*type, value))
				{
					m_function.meaning[item] = Enumerators{*flags, key, operand ? Form::combined : Form::cast};
					return;
				}
				// a mask's operand, 4 bytes as the enum is (the complement of a narrower one is another value once
				// promoted): the complement of a flag or of an OR of them
				uint64_t mask = maskOf(*type);
				if (parent && parent->op == cot_band && e->type.get_size() == 4 && mask == 0xFFFFFFFFull &&
					flagEnum(*type))
				{
					std::optional<std::vector<std::string>> flags = flagsOf(*type, ~value);
					std::variant<std::string, Missing> one = enumerator(*type, ~value & mask);
					if (!flags && std::holds_alternative<std::string>(one) && (~value & mask))
						flags = std::vector<std::string>{std::get<std::string>(one)};
					if (flags)
					{
						m_function.meaning[item] = Enumerators{*flags, key, Form::complement};
						return;
					}
				}
				if (besideOnly)
					return;
			}
			if (const Missing* missing = std::get_if<Missing>(&name))
			{
				m_function.meaning[item] = *missing;
				// an integer for an enum parameter, assigned to or returned as an enum keeps a cast to the enum (a
				// number of the enum's own type prints raw as a cast to it): the call's castTo, else the type the
				// assignment or return converts to
				if (e->type.is_enum() || !parent)
					return;
				Call* call = parent->op == cot_call ? callAt(parent) : nullptr;
				int k = call ? argument(*call, static_cast<const cexpr_t*>(parent), e) : -1;
				bool assigned = parent->op == cot_asg && static_cast<const cexpr_t*>(parent)->y == e;
				if (k >= 0 && call->castTo[k].empty())
					call->castTo[k] = key;
				else if ((assigned || parent->op == cit_return) && !m_function.meaning.count(parent))
					m_function.meaning[parent] = key;
			}
			else
				m_function.meaning[item] = Enumerators{{std::get<std::string>(name)}, key};
		});
		// any other integer value assigned to an enum (an unresolved call's result, IDA typing the callee neutrally),
		// or returned as one (src's declaration returns the enum, the database's an integer), keeps the cast to the
		// enum the assignment or return converts to
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			if (e->op != cot_asg || e->y->op == cot_num || m_function.meaning.count(e) || !e->y->type.is_integral() ||
				e->y->type.is_enum())
				return;
			if (const SrcModel::Enum* type = enumOf(srcType(m_function, m_context, e->x)))
				if (enumOf(srcType(m_function, m_context, e->y)) != type)
					m_function.meaning[e] = "enum:" + type->name;
		});
		if (const SrcModel::Enum* type = enumOf(m_function.returnType))
			forEachStatement(&m_function.cfunc->body, [&](const cinsn_t* s) {
				const cexpr_t* value = s->op == cit_return ? &s->creturn->expr : nullptr;
				if (value && value->op != cot_num && !m_function.meaning.count(s) &&
					(value->type.is_integral() || value->type.is_enum()) &&
					enumOf(srcType(m_function, m_context, value)) != type)
					m_function.meaning[s] = "enum:" + type->name;
			});
		forEachStatement(&m_function.cfunc->body, [&](const cinsn_t* s) {
			if (s->op != cit_switch)
				return;
			const SrcModel::Enum* type = enumOf(srcType(m_function, m_context, &s->cswitch->expr));
			if (!type)
				return;
			for (const ccase_t& c : s->cswitch->cases)
			{
				Enumerators labels{{}, "enum:" + type->name};
				for (uint64 value : c.values)
				{
					std::variant<std::string, Missing> name = enumerator(*type, value);
					if (const Missing* missing = std::get_if<Missing>(&name))
					{
						m_function.meaning[&c] = *missing;
						break;
					}
					labels.names.push_back(std::get<std::string>(name));
				}
				// a case left Missing keeps it
				auto found = m_function.meaning.find(&c);
				if (found == m_function.meaning.end() || !std::holds_alternative<Missing>(found->second))
					m_function.meaning[&c] = labels;
			}
		});
	}

private:
	Function& m_function;
	Context& m_context;
	std::unordered_map<const cexpr_t*, const SrcModel::Field*> m_stored;
};

}  // namespace

void resolveEnums(Function& function, Context& context)
{
	Enums(function, context).run();
}

}  // namespace idakit
