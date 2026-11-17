// Parts: Hex-Rays' helpers that read or write part of a value or join or reinterpret its bits (defs.h's LOBYTE,
// HIWORD, BYTE2, SLODWORD, ..., __PAIR64__, COERCE_FLOAT, ...), where the value is a scalar whose bits C++ reaches
// exactly, as that C++: shifts and masks of an integer (an enum through its unsigned integer of the same size), and a
// float's bits through `std::bit_cast`. The operand's printed type (its src type, else IDA's) decides, at the size
// Hex-Rays reads it, which places the part.
// - a read, `LOBYTE(x)`: `static_cast<uint8_t>(x)`, `HIWORD(x)` of 4 bytes `static_cast<uint16_t>(x >> 16)`, of a
//   float `static_cast<uint16_t>(std::bit_cast<uint32_t>(f) >> 16)` (an S form cast to the signed integer);
// - a write whose value nothing reads (a statement, a for loop's init or step, a comma's left side), of an integer
//   value: `LOBYTE(x) = v` keeps the other bytes, `x = static_cast<T>((static_cast<uint32_t>(x) & 0xFFFFFF00u) |
//   static_cast<uint32_t>(static_cast<uint8_t>(v)))` (of a float, the same on its bits, `std::bit_cast` back), and
//   `|=`, `^=`, `&=` on an integer the operation on the part's bits in place (`x |= static_cast<uint32_t>(
//   static_cast<uint8_t>(v)) << 8`); x must be a variable, a member or element of one, or a pointer variable's
//   pointee, which reading twice changes nothing;
// - `__PAIR64__(high, low)` of integers: `(static_cast<uint64_t>(static_cast<uint32_t>(high)) << 32) |
//   static_cast<uint32_t>(low)` (PAIR32, PAIR16 alike; __SPAIR as the signed integer);
// - `COERCE_<T>(x)` of an integer or float of T's size, T an integer or float: `std::bit_cast<T>(x)`; a double coerced
//   to 4 bytes, as GenSupport's coerce, the float it rounds to first;
// - `__CFADD__(x, y)`, the carry of an add, of integers, x stable as above: `(static_cast<uint32_t>(
//   static_cast<uint32_t>(x) + static_cast<uint32_t>(y)) < static_cast<uint32_t>(x))` at x's size.
// These helpers of a pointer, a record, an array or a value of another size than Hex-Rays reads (a local or field
// whose type is not what the exe keeps there), and the uses above they do not cover, stay Rules' marked spot, its
// reason saying why (`Hex-Rays helper LOBYTE of a pointer`). Runs after Rules, whose marker it replaces.

#include "EmitPasses.hpp"

#include <regex>
#include <string_view>

namespace idakit
{

namespace
{

constexpr const char* kBit = "<bit>";

// how C++ reaches a value's bits: an integer (bool included), an enum, a float or double; none for anything else
enum class Bits
{
	none,
	integer,
	enumeration,
	floating,
};

std::string unsignedOf(int64_t size)
{
	return size == 1 ? "uint8_t" : size == 2 ? "uint16_t" : size == 4 ? "uint32_t" : size == 8 ? "uint64_t" : "";
}

std::string signedOf(int64_t size)
{
	return size == 1 ? "int8_t" : size == 2 ? "int16_t" : size == 4 ? "int32_t" : size == 8 ? "int64_t" : "";
}

// the src type key of the integer an S form or an unsigned one yields
std::string integerKey(int64_t size, bool isSigned)
{
	static const char* const kUnsigned[] = {"unsigned char", "unsigned short", "unsigned int", "unsigned long long"};
	static const char* const kSigned[] = {"signed char", "short", "int", "long long"};
	int k = size == 1 ? 0 : size == 2 ? 1 : size == 4 ? 2 : 3;
	return std::string("builtin:") + (isSigned ? kSigned[k] : kUnsigned[k]);
}

std::string hexMask(uint64_t value, int64_t size)
{
	char text[32];
	std::snprintf(text, sizeof text, size == 8 ? "0x%llXull" : "0x%llXu", static_cast<unsigned long long>(value));
	return text;
}

// a part helper's name decoded: the part's width and its byte offset in a value of size bytes (nullopt: no part
// helper, or a part outside the value)
struct Part
{
	int64_t width = 0, offset = 0;
	bool isSigned = false;
};
std::optional<Part> partOf(std::string_view helper, int64_t size)
{
	static const std::regex part("(S?)(?:(LO|HI)(BYTE|WORD|DWORD)|(BYTE|WORD|DWORD)([1-9][0-9]?))");
	std::cmatch m;
	if (!std::regex_match(helper.begin(), helper.end(), m, part))
		return std::nullopt;
	std::string unit = m[3].matched ? m[3].str() : m[4].str();
	Part p;
	p.isSigned = m[1].length() != 0;
	p.width = unit == "BYTE" ? 1 : unit == "WORD" ? 2 : 4;
	p.offset = !m[2].matched ? p.width * std::stoi(m[5].str()) : m[2] == "LO" ? 0 : size - p.width;
	if (p.offset < 0 || p.offset + p.width > size || p.offset % p.width != 0)
		return std::nullopt;
	return p;
}

class Parts
{
public:
	Parts(Function& function, Context& context)
		: m_function(function), m_context(context), m_parents(parentsOf(function))
	{
	}

	void run()
	{
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t* s) {
			if (m_function.omitted(s) || e->op != cot_call || e->x->op != cot_helper || !markedByRules(e))
				return;
			std::string_view helper = e->x->helper;
			if (helper.starts_with("COERCE_"))
				coerce(e);
			else if (helper.starts_with("__PAIR") || helper.starts_with("__SPAIR"))
				pair(e, helper);
			else if (isPartHelper(helper))
				part(e, helper);
			else if (helper == "__CFADD__")
				carry(e);
		});
	}

private:
	// Rules' marker of a helper call, which this pass may replace (another pass's meaning stays)
	bool markedByRules(const cexpr_t* call) const
	{
		auto found = m_function.meaning.find(call);
		const Missing* missing = found == m_function.meaning.end() ? nullptr : std::get_if<Missing>(&found->second);
		return missing && missing->what == std::string("Hex-Rays helper ") + call->x->helper;
	}

	// Rules' marker with why this pass leaves the helper (`Hex-Rays helper LOBYTE of a pointer`)
	void decline(const cexpr_t* call, const std::string& why)
	{
		m_function.meaning[call] = Missing{std::string("Hex-Rays helper ") + call->x->helper + " " + why};
	}

	// how the value's printed type reaches its bits, at the size Hex-Rays reads (size: the type's); why: what it is
	// where none
	Bits bitsOf(const cexpr_t* e, int64_t* size, std::string& why) const
	{
		std::string key = srcType(m_function, m_context, e);
		std::string canonical = key.empty() ? "" : m_context.types.canonical(key);
		std::optional<int64_t> bytes = key.empty() ? std::nullopt : m_context.model.sizeOf(canonical);
		SrcTypes::Parts p = m_context.types.parts(canonical);
		const SrcModel::Type* type = p.kind == "builtin" ? m_context.model.type(canonical) : nullptr;
		why = key.empty()						? "of a value of no src type"
			  : p.kind == "ptr" || p.kind == "lref" ? "of a pointer"
			  : p.kind == "record"				? "of a record"
			  : p.kind == "array"				? "of an array"
			  : p.kind != "enum" && (!type || canonical == "builtin:void" || (type->isFloat && bytes != 4 && bytes != 8))
				  ? "of " + key
			  : !bytes || *bytes != static_cast<int64_t>(e->type.get_size())
				  ? "of " + key + " read as " + std::to_string(e->type.get_size()) + " bytes"
				  : "";
		if (!why.empty())
			return Bits::none;
		*size = *bytes;
		return p.kind == "enum" ? Bits::enumeration : type->isFloat ? Bits::floating : Bits::integer;
	}

	// the value's bits as an unsigned integer of its size, a form around `$N`
	static std::string asUnsigned(Bits bits, int64_t size, const std::string& item)
	{
		if (bits == Bits::floating)
			return "std::bit_cast<" + unsignedOf(size) + ">(" + item + ")";
		return "static_cast<" + unsignedOf(size) + ">(" + item + ")";
	}

	// the value nothing reads: a statement's expression, a for loop's init or step, a comma's left side
	bool valueUnused(const cexpr_t* e) const
	{
		auto parent = m_parents.find(e);
		if (parent == m_parents.end())
			return false;
		const citem_t* p = parent->second;
		if (p->op == cot_comma)
			return static_cast<const cexpr_t*>(p)->x == e;
		if (p->op == cit_expr)
			return true;
		if (p->op == cit_for)
		{
			const cfor_t* loop = static_cast<const cinsn_t*>(p)->cfor;
			return &loop->init == e || &loop->step == e;
		}
		return false;
	}

	// an lvalue reading which twice reads the same object and changes nothing: a variable, a member of one, an element
	// of one by a variable or number, the pointee of a pointer variable (each through such operands)
	static bool stable(const cexpr_t* e)
	{
		switch (e->op)
		{
		case cot_var:
		case cot_obj:
		case cot_num:
			return true;
		case cot_memref:
		case cot_memptr:
		case cot_ptr:
		case cot_cast:
			return stable(e->x);
		case cot_idx:
		case cot_add:
		case cot_sub:
			return stable(e->x) && stable(e->y);
		default:
			return false;
		}
	}

	// `LOBYTE(x)`, `LOBYTE(x) = v`, `LOBYTE(x) |= v`
	void part(const cexpr_t* call, std::string_view helper)
	{
		if (call->a->size() != 1)
			return;
		const cexpr_t* value = &(*call->a)[0];
		int64_t size = 0;
		std::string why;
		Bits bits = bitsOf(value, &size, why);
		std::optional<Part> p = bits == Bits::none ? std::nullopt : partOf(helper, size);
		if (!p)
		{
			decline(call, why.empty() ? "outside the value" : why);
			return;
		}
		auto parent = m_parents.find(call);
		const cexpr_t* user = parent != m_parents.end() && parent->second->is_expr()
								  ? static_cast<const cexpr_t*>(parent->second)
								  : nullptr;
		if (user && user->op == cot_ref)
			return decline(call, "whose address is taken");
		if (user && written(user) && user->x == call)
		{
			if (!is_assignment(user->op))
				return decline(call, "incremented");
			return write(call, user, value, bits, size, *p);
		}
		std::string result = p->isSigned ? signedOf(p->width) : unsignedOf(p->width);
		std::string whole = bits == Bits::integer ? "$0" : asUnsigned(bits, size, "$0");
		std::string shifted = p->offset == 0 ? whole : whole + " >> " + std::to_string(8 * p->offset);
		std::string form = bits == Bits::floating && p->width == size && p->offset == 0
							   ? "std::bit_cast<" + result + ">($0)"
							   : "static_cast<" + result + ">(" + shifted + ")";
		m_function.meaning[call] =
			Rewrite{1, form, {value}, {}, integerKey(p->width, p->isSigned), bits == Bits::floating ? kBit : ""};
	}

	// the part written: an integer value into an integer's or float's part, in place
	void write(const cexpr_t* call, const cexpr_t* assignment, const cexpr_t* target, Bits bits, int64_t size,
			   const Part& p)
	{
		int64_t storedSize = 0;
		std::string why;
		Bits stored = bitsOf(assignment->y, &storedSize, why);
		if (stored == Bits::floating)
			why = "of a float";
		if (!why.empty())
			return decline(call, "written with a value " + why);
		if (!valueUnused(assignment))
			return decline(call, "written where the assignment's value is read");
		if (!stable(target))
			return decline(call, "written in an object C++ would evaluate twice");
		if (bits == Bits::enumeration)
			return decline(call, "written in an enum");
		std::string key = srcType(m_function, m_context, target);
		std::optional<std::string> type = m_context.types.spell(key);
		if (!type)
			return decline(call, "written in " + key + ", which src cannot spell");
		// the integer the bits are worked in: the value's own size for a float, at least an int for an integer
		int64_t work = bits == Bits::floating || size == 8 ? size : 4;
		std::string unit = unsignedOf(work);
		int shift = static_cast<int>(8 * p.offset);
		uint64_t partMask = (p.width == 8 ? ~0ull : (1ull << (8 * p.width)) - 1) << shift;
		uint64_t all = work == 8 ? ~0ull : 0xFFFFFFFFull;
		std::string value = "static_cast<" + unit + ">(static_cast<" + unsignedOf(p.width) + ">($1))";
		std::string placed = shift == 0 ? value : value + " << " + std::to_string(shift);
		std::string form;
		if (assignment->op == cot_asg)
		{
			std::string whole = asUnsigned(bits == Bits::floating ? bits : Bits::integer, work, "$0");
			std::string bitsForm = p.width == size ? placed
												   : "(" + whole + " & " + hexMask(~partMask & all, work) + ") | (" +
														 placed + ")";
			form = "$0 = " + std::string(bits == Bits::floating ? "std::bit_cast<" : "static_cast<") + *type + ">(" +
				   bitsForm + ")";
		}
		else if (bits == Bits::integer && (assignment->op == cot_asgbor || assignment->op == cot_asgxor))
			form = std::string("$0 ") + (assignment->op == cot_asgbor ? "|= " : "^= ") + placed;
		else if (bits == Bits::integer && assignment->op == cot_asgband)
			form = "$0 &= (" + placed + ") | " + hexMask(~partMask & all, work);
		else
			return decline(call, std::string("written by ") + (bits == Bits::floating ? "a float's " : "") +
									 get_ctype_name(assignment->op));
		m_function.meaning[assignment] =
			Rewrite{1, form, {target, assignment->y}, {}, key, bits == Bits::floating ? kBit : ""};
		// the part itself is not printed: the assignment's form names its object
		m_function.meaning[call] = Rewrite{1, "$0", {target}, {}, key};
	}

	// `__PAIR64__(high, low)`: the halves joined
	void pair(const cexpr_t* call, std::string_view helper)
	{
		static const std::regex pairs("__(S?)PAIR(16|32|64)__");
		std::cmatch m;
		if (!std::regex_match(helper.begin(), helper.end(), m, pairs) || call->a->size() != 2)
			return;
		int64_t size = std::stoi(m[2].str()) / 8, half = size / 2;
		for (const carg_t& argument : *call->a)
		{
			int64_t bytes = 0;
			std::string why;
			Bits bits = bitsOf(&argument, &bytes, why);
			if (bits == Bits::floating)
				why = "of a float";
			if (!why.empty())
				return decline(call, "with a half " + why);
		}
		std::string whole = unsignedOf(size), part = unsignedOf(half);
		std::string joined = "(static_cast<" + whole + ">(static_cast<" + part + ">($0)) << " +
							 std::to_string(8 * half) + ") | static_cast<" + part + ">($1)";
		bool isSigned = m[1].length() != 0;
		std::string form = "static_cast<" + (isSigned ? signedOf(size) : whole) + ">(" + joined + ")";
		m_function.meaning[call] =
			Rewrite{1, form, {&(*call->a)[0], &(*call->a)[1]}, {}, integerKey(size, isSigned), ""};
	}

	// `__CFADD__(x, y)`: the carry of the unsigned add at x's size, `x + y < x` (x read twice, so a stable one)
	void carry(const cexpr_t* call)
	{
		if (call->a->size() != 2)
			return;
		int64_t size = 0;
		for (const carg_t& argument : *call->a)
		{
			int64_t bytes = 0;
			std::string why;
			Bits bits = bitsOf(&argument, &bytes, why);
			if (bits == Bits::floating)
				why = "of a float";
			if (!why.empty())
				return decline(call, "with an operand " + why);
			size = size ? size : bytes;
		}
		if (!stable(&(*call->a)[0]))
			return decline(call, "of an operand C++ would evaluate twice");
		std::string unit = unsignedOf(size);
		std::string form = "(static_cast<" + unit + ">(static_cast<" + unit + ">($0) + static_cast<" + unit +
						   ">($1)) < static_cast<" + unit + ">($0))";
		m_function.meaning[call] = Rewrite{1, form, {&(*call->a)[0], &(*call->a)[1]}, {}, "builtin:bool", ""};
	}

	// `COERCE_FLOAT(x)`: x's bits as the call's type
	void coerce(const cexpr_t* call)
	{
		if (call->a->size() != 1)
			return;
		const cexpr_t* value = &(*call->a)[0];
		std::string why;
		std::optional<std::string> key = m_context.types.keyOf(call->type, why);
		std::string canonical = key ? m_context.types.canonical(*key) : "";
		const SrcModel::Type* to = key ? m_context.model.type(canonical) : nullptr;
		std::optional<int64_t> toSize = key ? m_context.model.sizeOf(canonical) : std::nullopt;
		std::optional<std::string> spelled = key ? m_context.types.spell(*key) : std::nullopt;
		if (!to || to->kind != "builtin" || !toSize || !spelled || canonical == "builtin:void")
			return decline(call, "to " + text(call->type) + ", no integer or float");
		int64_t size = 0;
		Bits bits = bitsOf(value, &size, why);
		if (bits == Bits::enumeration)
			why = "of an enum";
		if (!why.empty())
			return decline(call, why);
		std::string form;
		if (size == *toSize)
			form = "std::bit_cast<" + *spelled + ">($0)";
		else if (bits == Bits::floating && size == 8 && *toSize == 4)
			form = "std::bit_cast<" + *spelled + ">(static_cast<float>($0))";
		else
			return decline(call, "of " + std::to_string(size) + " bytes to " + std::to_string(*toSize));
		m_function.meaning[call] = Rewrite{1, form, {value}, {}, *key, kBit};
	}

	Function& m_function;
	Context& m_context;
	std::unordered_map<const cexpr_t*, const citem_t*> m_parents;
};

}  // namespace

void rewritePartHelpers(Function& function, Context& context)
{
	Parts(function, context).run();
}

}  // namespace idakit
