// The printer: the ctree as C++, each item from its meaning, as the definition a porter would write in the class's
// .cpp (a member, static member, free function, explicit specialization or a structor as Construct/Destruct/
// DeletingDestruct), registered as the porting rules' "Registration forms" say; the check file includes that .cpp. A marked body
// (something unresolved) prints every resolved item as in src and every unresolved one raw (FNVD_AT, FNVD_GLOBAL,
// fnvd::original calls, IDA's types) with `/* unresolved: ... */` after it, and includes GenSupport.hpp for them.

#include "EmitPasses.hpp"

#include "IdaSupport.hpp"

#include "idakit/Address.hpp"

#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <map>
#include <optional>
#include <regex>
#include <set>
#include <string_view>

namespace idakit
{

namespace
{

const char* binaryOperator(ctype_t op)
{
	switch (op)
	{
	case cot_asg: return "=";
	case cot_asgbor: return "|=";
	case cot_asgxor: return "^=";
	case cot_asgband: return "&=";
	case cot_asgadd: return "+=";
	case cot_asgsub: return "-=";
	case cot_asgmul: return "*=";
	case cot_asgsshr: case cot_asgushr: return ">>=";
	case cot_asgshl: return "<<=";
	case cot_asgsdiv: case cot_asgudiv: return "/=";
	case cot_asgsmod: case cot_asgumod: return "%=";
	case cot_lor: return "||";
	case cot_land: return "&&";
	case cot_bor: return "|";
	case cot_xor: return "^";
	case cot_band: return "&";
	case cot_eq: return "==";
	case cot_ne: return "!=";
	case cot_sge: case cot_uge: return ">=";
	case cot_sle: case cot_ule: return "<=";
	case cot_sgt: case cot_ugt: return ">";
	case cot_slt: case cot_ult: return "<";
	case cot_sshr: case cot_ushr: return ">>";
	case cot_shl: return "<<";
	case cot_add: case cot_fadd: return "+";
	case cot_sub: case cot_fsub: return "-";
	case cot_mul: case cot_fmul: return "*";
	case cot_sdiv: case cot_udiv: case cot_fdiv: return "/";
	case cot_smod: case cot_umod: return "%";
	default: return nullptr;
	}
}

// the comparison with the other outcome for every operand pair: `a >= b` for `a < b`, `a != b` for `a == b`
ctype_t complementOf(ctype_t op)
{
	switch (op)
	{
	case cot_eq: return cot_ne;
	case cot_ne: return cot_eq;
	case cot_sge: return cot_slt;
	case cot_slt: return cot_sge;
	case cot_sle: return cot_sgt;
	case cot_sgt: return cot_sle;
	case cot_uge: return cot_ult;
	case cot_ult: return cot_uge;
	case cot_ule: return cot_ugt;
	case cot_ugt: return cot_ule;
	default: return cot_empty;
	}
}

// The flags a floating comparison's outcome leaves for the jcc or setcc that reads them.
struct Flags
{
	bool cf, pf, zf;
};

// the condition of a jcc or setcc on those flags (nullopt: another instruction, or one reading other flags)
std::optional<bool> condition(uint16 itype, Flags f)
{
	switch (itype)
	{
	case NN_je: case NN_jz: case NN_sete: case NN_setz: return f.zf;
	case NN_jne: case NN_jnz: case NN_setne: case NN_setnz: return !f.zf;
	case NN_jp: case NN_jpe: case NN_setp: case NN_setpe: return f.pf;
	case NN_jnp: case NN_jpo: case NN_setnp: case NN_setpo: return !f.pf;
	case NN_jb: case NN_jc: case NN_jnae: case NN_setb: case NN_setc: case NN_setnae: return f.cf;
	case NN_jae: case NN_jnb: case NN_jnc: case NN_setae: case NN_setnb: case NN_setnc: return !f.cf;
	case NN_jbe: case NN_jna: case NN_setbe: case NN_setna: return f.cf || f.zf;
	case NN_ja: case NN_jnbe: case NN_seta: case NN_setnbe: return !f.cf && !f.zf;
	default: return std::nullopt;
	}
}

// an x87 instruction other than fcomi and the like (they set EFLAGS): it leaves the flags as they were
bool keepsFlags(const insn_t& insn)
{
	std::string_view mnem = insn.get_canon_mnem(PH);
	return mnem.starts_with("f") && !mnem.starts_with("fcomi") && !mnem.starts_with("fucomi");
}

// The flags for each outcome of the comparison (less, equal, greater, unordered) as a flags setter leaves them:
// `test ah, mask` after fnstsw (the status word's C0, C2, C3 are ah's 0x01, 0x04, 0x40), or sahf, fcomi, comiss
// (C0, C2, C3 as CF, PF, ZF); nullopt for any other instruction.
std::optional<std::array<Flags, 4>> outcomeFlags(const insn_t& insn)
{
	static constexpr uint8_t codes[4] = {0x01, 0x40, 0x00, 0x45};
	bool test = insn.itype == NN_test && insn.ops[0].type == o_reg && insn.ops[0].reg == R_ah &&
				insn.ops[1].type == o_imm;
	bool direct = insn.itype == NN_sahf || insn.itype == NN_fcomi || insn.itype == NN_fcomip ||
				  insn.itype == NN_fucomi || insn.itype == NN_fucomip || insn.itype == NN_comiss ||
				  insn.itype == NN_comisd || insn.itype == NN_ucomiss || insn.itype == NN_ucomisd;
	if (!test && !direct)
		return std::nullopt;
	std::array<Flags, 4> flags;
	for (int k = 0; k < 4; ++k)
	{
		uint8_t c = codes[k];
		if (test)
		{
			uint8_t r = c & static_cast<uint8_t>(insn.ops[1].value);
			flags[k] = {false, std::popcount(r) % 2 == 0, r == 0};
		}
		else
			flags[k] = {(c & 0x01) != 0, (c & 0x04) != 0, (c & 0x40) != 0};
	}
	return flags;
}

// A floating comparison whose exe outcome for a NaN operand is not C++'s (false, true for !=). Hex-Rays prints the
// condition of the jcc or setcc after the compare as an ordered relation, the condition or its negation: the
// outcomes the flags test takes (read at the site) say which, and so what a NaN (unordered) gives. Sites of another
// shape keep C++'s outcome.
bool nanOutcomeDiffers(const cexpr_t* compare)
{
	ctype_t op = compare->op;
	if (complementOf(op) == cot_empty || !(compare->x->type.is_floating() || compare->y->type.is_floating()))
		return false;
	// the jcc or setcc: at the compare's address, or after it past x87 instructions and the flags setter
	insn_t insn;
	ea_t ea = compare->ea;
	std::optional<std::array<Flags, 4>> flags;
	for (int k = 0; k < 8; ++k, ea += insn.size)
	{
		if (decode_insn(&insn, ea) <= 0)
			return false;
		if (condition(insn.itype, {}))
			break;
		if (!keepsFlags(insn) && !outcomeFlags(insn))
			return false;
	}
	uint16 consumer = insn.itype;
	if (!condition(consumer, {}))
		return false;
	// the flags setter: before it, past x87 instructions that leave the flags
	for (ea_t at = ea; !flags;)
	{
		at = decode_prev_insn(&insn, at);
		if (at == BADADDR || at + 16 < ea)
			return false;
		flags = outcomeFlags(insn);
		if (!flags && !keepsFlags(insn))
			return false;
	}
	// the ordered outcomes (less 1, equal 2, greater 4) the instruction takes, and Hex-Rays' relation's
	int taken = 0;
	for (int k = 0; k < 3; ++k)
		taken |= *condition(consumer, (*flags)[k]) ? 1 << k : 0;
	bool unordered = *condition(consumer, (*flags)[3]);
	auto outcomes = [](ctype_t o) {
		switch (o)
		{
		case cot_eq: return 2;
		case cot_ne: return 5;
		case cot_slt: case cot_ult: return 1;
		case cot_sle: case cot_ule: return 3;
		case cot_sgt: case cot_ugt: return 4;
		default: return 6;
		}
	};
	// either operand order (fcom compares ST0 with its operand, Hex-Rays prints either first)
	int relation = outcomes(op), swapped = (relation & 2) | (relation & 1) << 2 | (relation & 4) >> 2;
	bool same = taken == relation || taken == swapped;
	if (!same && taken != (relation ^ 7) && taken != (swapped ^ 7))
		return false;
	return (same ? unordered : !unordered) != (op == cot_ne);
}

// C++'s precedence levels, low to high
enum Precedence
{
	kComma = 1,
	kAssignment,  // and ?:
	kLogicalOr,
	kLogicalAnd,
	kBitOr,
	kBitXor,
	kBitAnd,
	kEquality,
	kRelational,
	kShift,
	kAdditive,
	kMultiplicative,
	kUnary,
	kPostfix,
	kPrimary,
};

// the precedence of an operator as Hex-Rays has it
int precedenceOf(ctype_t op)
{
	if (op == cot_comma)
		return kComma;
	if ((op >= cot_asg && op <= cot_asgumod) || op == cot_tern)
		return kAssignment;
	switch (op)
	{
	case cot_lor: return kLogicalOr;
	case cot_land: return kLogicalAnd;
	case cot_bor: return kBitOr;
	case cot_xor: return kBitXor;
	case cot_band: return kBitAnd;
	case cot_eq: case cot_ne: return kEquality;
	case cot_sge: case cot_uge: case cot_sle: case cot_ule:
	case cot_sgt: case cot_ugt: case cot_slt: case cot_ult: return kRelational;
	case cot_sshr: case cot_ushr: case cot_shl: return kShift;
	case cot_add: case cot_fadd: case cot_sub: case cot_fsub: return kAdditive;
	case cot_mul: case cot_fmul: case cot_sdiv: case cot_udiv: case cot_fdiv:
	case cot_smod: case cot_umod: return kMultiplicative;
	case cot_lnot: case cot_bnot: case cot_neg: case cot_fneg: case cot_preinc: case cot_predec:
	case cot_ptr: case cot_ref: case cot_cast: return kUnary;
	case cot_postinc: case cot_postdec: case cot_idx: case cot_memptr: case cot_memref: case cot_call:
		return kPostfix;
	default: return kPrimary;
	}
}

// clang warns of the child without parentheses (-Wparentheses): && in ||, + or - in a shift, a comparison in a
// bitwise operator, & or ^ in |, & in ^
bool clangWarns(ctype_t parent, ctype_t child)
{
	int p = precedenceOf(parent), c = precedenceOf(child);
	return (parent == cot_lor && child == cot_land) || (p == kShift && c == kAdditive) ||
		   (p >= kBitOr && p <= kBitAnd && (c == kEquality || c == kRelational)) ||
		   (parent == cot_bor && (child == cot_band || child == cot_xor)) ||
		   (parent == cot_xor && child == cot_band);
}

// GenSupport.hpp's spelling of Hex-Rays' helper name (defs.h's macros and intrinsics) called on the printed
// arguments: its value, or for an assignment target (lvalue) the bytes in place; empty where GenSupport has none
std::string genHelper(const std::string& name, const std::string& arguments, bool lvalue)
{
	// partial registers: [S]LOBYTE, [S]HIBYTE, [S]BYTEn and the same for WORD and DWORD (S: signed)
	static const std::regex part("(S?)(?:(LO|HI)(BYTE|WORD|DWORD)|(BYTE|WORD|DWORD)([1-9][0-9]?))");
	if (std::smatch m; std::regex_match(name, m, part))
	{
		std::string unit = m[3].matched ? m[3].str() : m[4].str();
		std::string bits = unit == "BYTE" ? "8" : unit == "WORD" ? "16" : "32";
		std::string lower = unit == "BYTE" ? "byte" : unit == "WORD" ? "word" : "dword";
		std::string function = m[2].matched ? (m[2] == "LO" ? "lo" : "hi") + lower : lower + "n";
		std::string call = "gen::" + function + (lvalue ? "_ref" : "") +
						   (m[5].matched ? "<" + m[5].str() + ">" : "") + "(" + arguments + ")";
		if (m[1].length() == 0)
			return call;
		return lvalue ? "reinterpret_cast<int" + bits + "_t&>(" + call + ")"
					  : "static_cast<int" + bits + "_t>(" + call + ")";
	}
	// two halves joined: __PAIR64__(high, low), __SPAIR64__ the same, signed
	static const std::regex pair("__(S?)PAIR(16|32|64)__");
	if (std::smatch m; std::regex_match(name, m, pair))
	{
		std::string call = "gen::pair" + m[2].str() + "(" + arguments + ")";
		return m[1].length() == 0 ? call : "static_cast<int" + m[2].str() + "_t>(" + call + ")";
	}
	static const std::map<std::string, std::string> functions = {
		{"__ROL4__", "rol"},
		{"__ROR4__", "ror"},
		{"__ROL2__", "rol"},
		{"__ROR2__", "ror"},
		{"__ROL1__", "rol"},
		{"__ROR1__", "ror"},
		{"__SETS__", "sets"},
		{"__SETP__", "setp"},
		{"__OFSUB__", "ofsub"},
		{"__OFADD__", "ofadd"},
		{"__CFADD__", "cfadd"},
		{"__CFSHL__", "cfshl"},
		{"fabs", "fabs_"},
		{"sqrt", "sqrt_"},
		{"fsqrt", "sqrt_"},
		{"abs32", "abs32"},
		{"qmemcpy", "qmemcpy"},
		{"memset32", "memset32"},
		{"JUMPOUT", "jumpout"},
		{"_InterlockedIncrement", "interlocked_increment"},
		{"_InterlockedDecrement", "interlocked_decrement"},
		{"_InterlockedExchange", "interlocked_exchange"},
		{"_InterlockedExchangeAdd", "interlocked_exchange_add"},
		{"NtCurrentTeb", "teb"},
		{"__readfsdword", "readfs32"},
	};
	if (auto found = functions.find(name); found != functions.end())
		return "gen::" + found->second + "(" + arguments + ")";
	return "";
}

// a src name as C++ spells it in its TU (SrcIda's tuSpelling)
std::string inTu(const std::string& name)
{
	return tuSpelling(name);
}

const char* convention(const tinfo_t& function)
{
	std::string cc = conventionName(function.get_cc());
	if (cc == "thiscall")
		return "__thiscall";
	if (cc == "stdcall")
		return "__stdcall";
	if (cc == "fastcall")
		return "__fastcall";
	return "__cdecl";
}

std::string hexUpper(uint64_t value)
{
	char text[32];
	std::snprintf(text, sizeof text, "0x%llX", static_cast<unsigned long long>(value));
	return text;
}

// an integer literal of the type's size and sign
std::string literal(uint64_t value, const tinfo_t& type)
{
	size_t size = type.get_size();
	if (size == 0 || size > 8)
		size = 4;
	// Hex-Rays' untyped data of an integer's size (`_DWORD`) spells as the unsigned integer (SrcTypes' integer)
	type_t full = type.get_realtype();
	bool untyped = full == BT_UNK_BYTE || full == BT_UNK_WORD || full == BT_UNK_DWORD || full == BT_UNK_QWORD;
	bool isSigned = !type.is_unsigned() && !type.is_ptr() && !untyped;
	uint64_t mask = size == 8 ? ~0ull : (1ull << (size * 8)) - 1;
	value &= mask;
	std::string suffix = size == 8 ? (isSigned ? "ll" : "ull") : (!isSigned && size == 4 ? "u" : "");
	auto text = [&](uint64_t magnitude) {
		return magnitude < 0x1000 ? std::to_string(magnitude) : hexUpper(magnitude);
	};
	if (isSigned && (value >> (size * 8 - 1)) & 1)
	{
		int64_t extended = static_cast<int64_t>(value | ~mask);
		if (size == 4 && extended == INT32_MIN)
			return "(-2147483647 - 1)";
		if (size == 8 && extended == INT64_MIN)
			return "(-9223372036854775807ll - 1)";
		return "-" + text(static_cast<uint64_t>(-extended)) + suffix;
	}
	return text(value) + suffix;
}

}  // namespace

std::string floatLiteral(double value, bool single)
{
	char text[64];
	for (int digits = 6; digits <= 17; ++digits)
	{
		std::snprintf(text, sizeof text, "%.*g", digits, value);
		if (single ? std::strtof(text, nullptr) == static_cast<float>(value) : std::strtod(text, nullptr) == value)
			break;
	}
	std::string out = text;
	if (out.find_first_of(".en") == std::string::npos)
		out += ".0";
	return out + (single ? "f" : "");
}

namespace
{

// Where a marked value comes from and where it goes (spots.tsv's origin and target): recorded, not decided on.
// origin is the leaf that supplies the value's type, reached through casts, `&`, `*`, `+`/`-` with an integer and an
// array index's base: `field:<IDA struct>+0x<off>` (a union's member n: `+0x0#n`), `param:<function>:<n>` (n from 0
// in Hex-Rays' argument order), `local:<function>:<name>` (a local with one write, a plain assignment, is `local<-`
// and that right side's origin, one level), `ret:<callee>` (a direct call), `vret:<IDA vtbl struct>:<slot>` (a call
// through a vtable member; a call through any other function pointer has the pointer's origin), `global:<ea>`,
// `func:<ea>`, `number`, `helper:<name>`, else `other:<ctype>`. target, from the value up past its casts:
// `arg:<callee>:<n>` (`arg:indirect:<n>`, `arg:helper:<name>:<n>`), `store:` and the left side's origin (a local's
// unexpanded), `return:<function>`, else `-`. Addresses as hexText.
class Provenance
{
public:
	explicit Provenance(const Function& function) : m_function(function), m_parents(parentsOf(function))
	{
		// each local's writes: assignments, increments and decrements, and taking its address (a possible write)
		forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
			if (!is_assignment(e->op) && !is_prepost(e->op) && e->op != cot_ref)
				return;
			int written = lvarIndex(e->x);
			if (written < 0)
				return;
			Writes& w = m_writes[written];
			++w.count;
			w.assignment = e->op == cot_asg ? e : nullptr;
		});
	}

	// origin and target of the marked item (an expression or a statement)
	std::pair<std::string, std::string> of(const citem_t* item) const
	{
		if (!item->is_expr())
		{
			const cinsn_t* s = static_cast<const cinsn_t*>(item);
			if (s->op == cit_return && s->creturn)
				return {origin(&s->creturn->expr), "return:" + hexText(m_function.ea)};
			if (s->op == cit_expr && s->cexpr)
				return of(s->cexpr);
			return {std::string("other:") + get_ctype_name(s->op), "-"};
		}
		const cexpr_t* e = static_cast<const cexpr_t*>(item);
		if (is_assignment(e->op))
			return {origin(e->y), "store:" + origin(e->x, false)};
		return {origin(e), target(e)};
	}

	// the lvar the marked item's value comes from (origin's local, unexpanded) or is stored into (target's), -1 for
	// none: no local, or an argument
	int local(const citem_t* item) const
	{
		if (!item->is_expr())
		{
			const cinsn_t* s = static_cast<const cinsn_t*>(item);
			if (s->op == cit_return && s->creturn)
				return variable(leaf(&s->creturn->expr));
			return s->op == cit_expr && s->cexpr ? local(s->cexpr) : -1;
		}
		const cexpr_t* e = static_cast<const cexpr_t*>(item);
		if (is_assignment(e->op))
		{
			int from = variable(leaf(e->y));
			return from >= 0 ? from : variable(leaf(e->x));
		}
		if (int from = variable(leaf(e)); from >= 0)
			return from;
		auto parent = m_parents.find(e);
		while (parent != m_parents.end() && parent->second->is_expr() && parent->second->op == cot_cast)
		{
			e = static_cast<const cexpr_t*>(parent->second);
			parent = m_parents.find(e);
		}
		if (parent == m_parents.end() || !parent->second->is_expr())
			return -1;
		const cexpr_t* p = static_cast<const cexpr_t*>(parent->second);
		return is_assignment(p->op) && p->y == e ? variable(leaf(p->x)) : -1;
	}

private:
	struct Writes
	{
		int count = 0;
		const cexpr_t* assignment = nullptr;  // the one write, where it is a plain assignment
	};

	// the leaf an origin is read from: through casts, `&`, `*`, an array index's base and `+`/`-` with an integer; the
	// `+`/`-` itself where neither side is the pointer
	static const cexpr_t* leaf(const cexpr_t* e)
	{
		while (true)
		{
			switch (e->op)
			{
			case cot_cast: case cot_ref: case cot_ptr: case cot_idx:
				e = e->x;
				continue;
			case cot_add: case cot_sub:
				if (e->x->type.is_ptr() || e->x->type.is_array() || e->y->op == cot_num)
					e = e->x;
				else if (e->op == cot_add && (e->y->type.is_ptr() || e->x->op == cot_num))
					e = e->y;
				else
					return e;
				continue;
			default:
				return e;
			}
		}
	}

	// the lvar a leaf is, -1 for none or an argument
	int variable(const cexpr_t* e) const
	{
		if (e->op != cot_var)
			return -1;
		return (*m_function.cfunc->get_lvars())[e->v.idx].is_arg_var() ? -1 : e->v.idx;
	}

	std::string origin(const cexpr_t* e, bool expand = true) const
	{
		e = leaf(e);
		switch (e->op)
		{
		case cot_memptr: case cot_memref:
		{
			// a union's member is its number (Hex-Rays' m), at offset 0
			tinfo_t record = e->op == cot_memptr ? e->x->type.get_pointed_object() : e->x->type;
			if (record.is_union())
				return "field:" + typeName(record) + "+0x0#" + std::to_string(e->m);
			return "field:" + typeName(record) + "+" + hexText(e->m);
		}
		case cot_var:
		{
			const lvars_t& lvars = *m_function.cfunc->get_lvars();
			const lvar_t& v = lvars[e->v.idx];
			if (v.is_arg_var())
			{
				const intvec_t& arguments = m_function.cfunc->argidx;
				for (size_t k = 0; k < arguments.size(); ++k)
					if (arguments[k] == e->v.idx)
						return "param:" + hexText(m_function.ea) + ":" + std::to_string(k);
			}
			auto found = m_writes.find(e->v.idx);
			if (expand && found != m_writes.end() && found->second.count == 1 && found->second.assignment)
				return "local<-" + origin(found->second.assignment->y, false);
			return "local:" + hexText(m_function.ea) + ":" + v.name.c_str();
		}
		case cot_obj:
			return (functionStart(e) ? "func:" : "global:") + hexText(e->obj_ea);
		case cot_num: case cot_fnum:
			return "number";
		case cot_helper:
			return std::string("helper:") + e->helper;
		case cot_call:
		{
			const cexpr_t* callee = stripCasts(e->x);
			if (callee->op == cot_helper)
				return std::string("helper:") + callee->helper;
			if (functionStart(callee))
				return "ret:" + hexText(callee->obj_ea);
			if (callee->op == cot_memptr || callee->op == cot_memref)
			{
				tinfo_t table = callee->op == cot_memptr ? callee->x->type.get_pointed_object() : callee->x->type;
				std::string name = typeName(table);
				if (table.is_vftable() || name.ends_with("_vtbl"))
					return "vret:" + name + ":" + std::to_string(callee->m / 4);
			}
			// a call through a function pointer anywhere else: the pointer's origin
			return origin(callee, expand);
		}
		default:
			return std::string("other:") + get_ctype_name(e->op);
		}
	}

	std::string target(const cexpr_t* e) const
	{
		auto parent = m_parents.find(e);
		while (parent != m_parents.end() && parent->second->is_expr() && parent->second->op == cot_cast)
		{
			e = static_cast<const cexpr_t*>(parent->second);
			parent = m_parents.find(e);
		}
		if (parent == m_parents.end())
			return "-";
		if (!parent->second->is_expr())
			return parent->second->op == cit_return ? "return:" + hexText(m_function.ea) : "-";
		const cexpr_t* p = static_cast<const cexpr_t*>(parent->second);
		if (p->op == cot_call && p->x != e)
		{
			for (size_t k = 0; k < p->a->size(); ++k)
				if (&(*p->a)[k] == e)
				{
					const cexpr_t* callee = stripCasts(p->x);
					std::string n = std::to_string(k);
					if (functionStart(callee))
						return "arg:" + hexText(callee->obj_ea) + ":" + n;
					if (callee->op == cot_helper)
						return std::string("arg:helper:") + callee->helper + ":" + n;
					return "arg:indirect:" + n;
				}
			return "-";
		}
		if (is_assignment(p->op) && p->y == e)
			return "store:" + origin(p->x, false);
		return "-";
	}

	static std::string typeName(const tinfo_t& t)
	{
		qstring name;
		if (t.get_type_name(&name) && !name.empty())
			return name.c_str();
		std::string text = t.dstr();
		return text.empty() ? "?" : text;
	}

	const Function& m_function;
	std::unordered_map<const cexpr_t*, const citem_t*> m_parents;
	std::unordered_map<int, Writes> m_writes;
};

class Printer
{
public:
	Printer(const Function& function, Context& context, Emitted::Form form)
		: m_function(function), m_context(context), m_types(context.types), m_src(form == Emitted::Form::src),
		  m_copies(function.copies()), m_dropped(function.dropped())
	{
		for (size_t k = 0; k < function.locals.size(); ++k)
			if (function.locals[k].declaredAt)
				m_declared[function.locals[k].declaredAt].push_back(static_cast<int>(k));
	}

	// the file's text, its markers appended to spots in print order
	std::string file(std::vector<Emitted::Spot>& spots)
	{
		std::string head = srcHead();
		std::string prologue = this->prologue();
		std::string body;
		std::swap(body, m_out);
		blockItems(&m_function.cfunc->body, 1);
		std::swap(body, m_out);
		// a definition is part of its class's src .cpp: that first, then its own needs
		std::string first = m_context.model.classSource(m_function.self->cls);
		first = first.substr(first.rfind("src/", 0) == 0 ? 4 : 0);
		std::string out;
		if (!first.empty())
			out += "#include \"" + first + "\"\n";
		if (!m_src)
			out += "#include \"GenSupport.hpp\"\n";
		for (const std::string& include : m_includes)
			if (include != first)
				out += include.starts_with('<') ? "#include " + include + "\n" : "#include \"" + include + "\"\n";
		out += "\n";
		if (!m_globals.empty())
		{
			for (const auto& [ea, line] : m_globals)
				out += line + "\n";
			out += "\n";
		}
		if (!m_src)
			for (const std::string& blocker : m_function.blockers)
				out += "// not src: " + blocker + "\n";
		for (const std::string& note : m_function.notes)
			out += "// " + note + "\n";
		out += head + " {\n" + prologue + body + "}\n";
		out += registration();
		return stripped(out, spots);
	}

private:
	// --- meanings

	const Meaning* meaning(const citem_t* item) const
	{
		auto found = m_function.meaning.find(item);
		return found == m_function.meaning.end() ? nullptr : &found->second;
	}
	template <class T>
	const T* as(const citem_t* item) const
	{
		const Meaning* m = meaning(item);
		return m ? std::get_if<T>(m) : nullptr;
	}
	// --- markers
	//
	// Each marker goes into the text after a sentinel (kSpot, its flag, its index in m_spots, kSpotEnd) that file()
	// strips, listing the markers its text keeps as the function's spots, in print order: text printed and then
	// dropped (a precedence probe) leaves none. A marker is a root spot unless the raw text of another item's marker
	// holds it (the item's own conversion is no other item); a local's is a root always, a review's never (a note, no
	// blocker: user, 2026-11-09). This records what is printed and decides nothing.
	static constexpr char kSpot = '\x01', kSpotEnd = '\x02';
	static constexpr char kRoot = 'R', kNested = 'N', kAlways = 'A', kNote = 'V';
	struct Marker
	{
		const citem_t* item;
		std::string reason;  // spots.tsv's: the unresolved one's text, `review: ...` for a review
		bool review;
		int local = -1;  // a local's: its lvar
	};

	// ` /* unresolved: what */` (review: ` /* review: what */`) of the item (nullptr: a local's, the lvar local)
	std::string marker(const citem_t* item, const std::string& what, bool review = false, int local = -1)
	{
		m_markers.push_back({item, review ? "review: " + what : what, review, local});
		char flag = review ? kNote : item == nullptr ? kAlways : kRoot;
		return std::string(1, kSpot) + flag + std::to_string(m_markers.size() - 1) + kSpotEnd +
			   (review ? " /* review: " : " /* unresolved: ") + what + " */";
	}
	// the item's raw text with the root markers of other items in it nested: the item's marker follows it
	std::string nested(const citem_t* item, std::string text) const
	{
		for (size_t at = text.find(kSpot); at != std::string::npos; at = text.find(kSpot, at + 1))
			if (text[at + 1] == kRoot && m_markers[std::strtoul(text.c_str() + at + 2, nullptr, 10)].item != item)
				text[at + 1] = kNested;
		return text;
	}
	// the text without its sentinels, its markers appended to spots: a review's and a local's without an origin or a
	// target (`-`)
	std::string stripped(const std::string& text, std::vector<Emitted::Spot>& spots) const
	{
		std::optional<Provenance> provenance;
		std::string out;
		out.reserve(text.size());
		for (size_t k = 0; k < text.size(); ++k)
		{
			if (text[k] != kSpot)
			{
				out += text[k];
				continue;
			}
			size_t end = text.find(kSpotEnd, k);
			const Marker& marker = m_markers[std::stoul(text.substr(k + 2, end - k - 2))];
			Emitted::Spot spot{text[k + 1] == kRoot || text[k + 1] == kAlways, marker.reason, "-", "-", marker.local};
			if (marker.item && !marker.review)
			{
				if (!provenance)
					provenance.emplace(m_function);
				std::tie(spot.origin, spot.target) = provenance->of(marker.item);
				spot.local = provenance->local(marker.item);
			}
			spots.push_back(std::move(spot));
			k = end;
		}
		return out;
	}

	// the text, with what is missing or to review after it
	std::string mark(const citem_t* item, std::string text)
	{
		if (const Missing* missing = as<Missing>(item))
			return nested(item, text) + marker(item, missing->what);
		if (const Review* review = as<Review>(item))
			return nested(item, text) + marker(item, review->what, true);
		return text;
	}

	// --- types

	// the src key as a declaration of name (its headers included), else IDA's type raw
	std::string declare(const std::string& key, const tinfo_t& ida, const std::string& name = "")
	{
		if (!key.empty())
			if (std::optional<std::string> spelled = m_types.spell(key, name))
			{
				m_types.headers(key, m_includes);
				return *spelled;
			}
		return raw(ida, name);
	}

	// IDA's type as C++ in a marked body: its src key where it has one, else by its size and kind (gen::Blob for a
	// struct)
	std::string raw(const tinfo_t& t, const std::string& inner = "")
	{
		std::string why;
		if (std::optional<std::string> key = m_types.keyOf(t, why))
			if (std::optional<std::string> spelled = m_types.spell(*key, inner))
			{
				m_types.headers(*key, m_includes);
				return *spelled;
			}
		auto join = [&](const std::string& base) {
			if (inner.empty())
				return base;
			return base + (inner[0] == '*' || inner[0] == '&' ? "" : " ") + inner;
		};
		if (t.is_void())
			return join("void");
		if (t.is_bool() && t.get_size() == 1)
			return join("bool");
		if (t.is_floating())
			return join(t.get_size() == 4 ? "float" : "double");
		if (t.is_ptr())
		{
			tinfo_t pointee = t.get_pointed_object();
			std::string next = inner.empty() ? "*" : inner[0] == '*' ? "*" + inner : "* " + inner;
			if (pointee.is_array() || pointee.is_func())
				next = "(*" + inner + ")";
			return raw(pointee, next);
		}
		if (t.is_array())
		{
			int count = t.get_array_nelems();
			// a pointer or reference to the array: `T (*)[N]`
			std::string declarator = !inner.empty() && (inner[0] == '*' || inner[0] == '&') ? "(" + inner + ")" : inner;
			return raw(t.get_array_element(), declarator + "[" + (count > 0 ? std::to_string(count) : "") + "]");
		}
		if (t.is_func())
		{
			func_type_data_t details;
			std::string params;
			if (t.get_func_details(&details))
			{
				for (const funcarg_t& arg : details)
					params += (params.empty() ? "" : ", ") + raw(arg.type);
				// a C-variadic callee: the arguments past the fixed ones
				if (details.is_vararg_cc())
					params += params.empty() ? "..." : ", ...";
			}
			// the convention inside the declarator: R (__thiscall* p)(A)
			std::string declarator = inner;
			if (declarator.size() > 1 && declarator[0] == '(')
				declarator.insert(1, std::string(convention(t)) + " ");
			return raw(t.get_rettype()) + " " + declarator + "(" + params + ")";
		}
		if (t.is_udt())
			return join("gen::Blob<" + std::to_string(t.get_size()) + ">");
		size_t size = t.get_size();
		bool isSigned = !t.is_unsigned();
		if (t.is_char())
			return join("char");
		switch (size)
		{
		case 1: return join(isSigned ? "int8_t" : "uint8_t");
		case 2: return join(isSigned ? "int16_t" : "uint16_t");
		case 4: return join(isSigned ? "int32_t" : "uint32_t");
		case 8: return join(isSigned ? "int64_t" : "uint64_t");
		default:
			// _OWORD, __int128: no integer is as wide; GenSupport's of its size and signedness
			if (size > 8 && size != BADSIZE && size % 4 == 0)
				return join("gen::Wide<" + std::to_string(size) + ", " + (isSigned ? "true" : "false") + ">");
			if (size > 8 && size != BADSIZE)
				return join("gen::Blob<" + std::to_string(size) + ">");
			return join("uint8_t");
		}
	}

	// a cot_var of a src reference parameter, or a use of FNVD_CONSTRUCTED_LOCAL's local as its address (Hex-Rays'
	// array decaying to it): the object, its address printed `&v`
	bool referenceVar(const cexpr_t* e) const
	{
		int v = lvarIndex(e);
		if (v < 0)
			return false;
		const Local& local = m_function.locals[v];
		const std::string* key = local.inlined && local.inlined->construct ? as<std::string>(e) : nullptr;
		return local.reference || (key && key->starts_with("ptr("));
	}
	// a call of a src function returning a reference where Hex-Rays has a pointer
	bool referenceCall(const cexpr_t* e) const
	{
		const Call* c = e->op == cot_call ? as<Call>(e) : nullptr;
		return c && c->returnsReference;
	}
	// a call returning a record by value into a local (`local = F(...)`) where Hex-Rays has the result pointer
	bool byValueCall(const cexpr_t* e) const
	{
		const Call* c = e->op == cot_call ? as<Call>(e) : nullptr;
		return c && c->resultArgument >= 0;
	}
	// a by-value call whose result is C++'s temporary where it is used (foldHiddenResults): printed `F(...)`
	bool temporary(const cexpr_t* e) const
	{
		return byValueCall(e) && m_function.temporaries.count(e);
	}
	// a by-value call built in its object with placement new (placeDeclarations): `::new (p) T(F(...))`, whose value
	// is p
	bool placement(const cexpr_t* e) const
	{
		return byValueCall(e) && m_function.placements.count(e);
	}
	// the operator a record-returning arithmetic operator's call prints as (`*a - b`, `-*a`): cot_add, cot_sub,
	// cot_mul, cot_sdiv, cot_neg for a unary minus; cot_empty for a call that prints as a call. Only where the result
	// goes to a local or is a temporary (resultArgument), so a converted argument keeps the call form (its
	// static_cast).
	ctype_t infixOperator(const cexpr_t* e) const
	{
		static const std::map<std::string, ctype_t> binary = {
			{"operator+", cot_add}, {"operator-", cot_sub}, {"operator*", cot_mul}, {"operator/", cot_sdiv}};
		const Call* resolved = byValueCall(e) ? as<Call>(e) : nullptr;
		auto op = resolved ? binary.find(lastComponent(resolved->function->name)) : binary.end();
		if (op == binary.end())
			return cot_empty;
		bool member = !resolved->function->cls.empty() && !resolved->function->isStatic;
		size_t operands = 0;
		for (size_t k = 0; k < e->a->size(); ++k)
			if (static_cast<int>(k) != resolved->resultArgument)
			{
				if (!((member && k == 0) || resolved->byReference[k]) && !resolved->castTo[k].empty())
					return cot_empty;
				++operands;
			}
		return operands == 2 ? op->second : operands == 1 && op->second == cot_sub ? cot_neg : cot_empty;
	}
	// the call whose temporary a pointer read foldHiddenResults printed as its temporary (`&*$0`) stands for; nullptr
	// for another rewrite
	const cexpr_t* temporaryRead(const Rewrite& rewrite) const
	{
		if (rewrite.form != "&*$0" || rewrite.items.size() != 1)
			return nullptr;
		const auto* call = static_cast<const cexpr_t*>(rewrite.items[0]);
		return temporary(call) ? call : nullptr;
	}
	// the precedence of the object deref prints for e (past the casts C++ makes by itself) where that is a temporary:
	// a temporary call's, or the call's a pointer read foldHiddenResults printed as its temporary (`&*$0`); nullopt
	// for another e
	std::optional<int> temporaryObject(const cexpr_t* e) const
	{
		while (e->op == cot_cast && as<Implicit>(e))
			e = e->x;
		if (const Rewrite* rewrite = as<Rewrite>(e); rewrite && temporaryRead(*rewrite))
			e = temporaryRead(*rewrite);
		if (!temporary(e))
			return std::nullopt;
		ctype_t op = infixOperator(e);
		return op == cot_empty || as<Call>(e)->exeCalled ? kPostfix : op == cot_neg ? kUnary : precedenceOf(op);
	}

	// --- statements

	void line(int depth, const std::string& text)
	{
		m_out.append(depth, '\t');
		m_out += text + "\n";
	}

	// the markers above a statement (RULES.md), for each of its own expressions (an else-if chain's conditions
	// included): `// EXE-CAST: A as B` for a read or write of A's bytes as B (ExeCast; a value's cast, a pointer read
	// as an integer, its two types), `// EXE-INLINED none: F` for a call of src's F the exe inlines (Call's
	// exeInlined). A call the exe makes where clang could inline carries no comment: FNVD_NOINLINE around it is its
	// marker.
	void markers(const cinsn_t* s, int depth)
	{
		std::vector<const cexpr_t*> own = ownExpressions(s);
		for (const cinsn_t* chain = s; chain->op == cit_if && chain->cif->ielse && chain->cif->ielse->op == cit_if &&
									   chain->cif->ielse->label_num < 0;
			 chain = chain->cif->ielse)
			own.push_back(&chain->cif->ielse->cif->expr);
		// the type a pointer's key points to, as src spells it
		auto pointee = [&](const std::string& key, const tinfo_t& pointer) {
			SrcTypes::Parts p = m_types.parts(key);
			return declare(p.kind == "ptr" ? p.of : "", pointer.get_pointed_object());
		};
		// one EXE-CAST line per pair of types
		std::set<std::string> casts;
		for (const cexpr_t* e : own)
			forEachSubexpression(e, [&](const cexpr_t* sub) {
				if (const Call* call = sub->op == cot_call ? as<Call>(sub) : nullptr; call && !call->exeInlined.empty())
					line(depth, "// EXE-INLINED " + call->exeInlined);
				if (const ExeCast* bytes = sub->op == cot_cast ? as<ExeCast>(sub) : nullptr)
				{
					// a pointer's storage read or written as an integer: the pointer's type as the integer's
					std::string from = srcType(m_function, m_context, sub->x);
					std::string cast = "// EXE-CAST: ";
					if (sub->type.is_ptr() == sub->x->type.is_ptr())
						cast += pointee(from, sub->x->type) + " as " + pointee(bytes->type, sub->type);
					else if (sub->type.is_ptr())
						cast += declare(bytes->type, sub->type) + " as " + declare(from, sub->x->type);
					else
						cast += declare(from, sub->x->type) + " as " + declare(bytes->type, sub->type);
					if (casts.insert(cast).second)
						line(depth, cast);
				}
			});
	}

	// a local's declaration: its src type, else IDA's (raw bytes for raw storage of IDA's object; storage Hex-Rays
	// types as bytes keeps them, aligned for the object it holds), after the frame's alignment (AlignedLocals)
	std::string declareLocal(int v)
	{
		const Local& local = m_function.locals[v];
		const tinfo_t& type = (*m_function.cfunc->get_lvars())[v].type();
		if (local.storage && local.type.empty())
			return frameAlignment(v) + "gen::Blob<" + std::to_string(type.get_size()) + "> " + var(v);
		if (local.storage)
			if (std::optional<std::string> held = m_types.spell(local.holds))
			{
				m_types.headers(local.holds, m_includes);
				return frameAlignment(v) + "alignas(" + *held + ") " + declare(local.type, type, var(v));
			}
		return frameAlignment(v) + declare(local.type, type, var(v));
	}

	// `alignas(N) ` where AlignedLocals gives the local the exe frame's alignment, else nothing
	std::string frameAlignment(int v) const
	{
		int64_t align = m_function.locals[v].align;
		return align ? "alignas(" + std::to_string(align) + ") " : "";
	}

	// the locals declared at the top of the block, without an initialiser but a pass's own (Local::initialiser; an
	// inlined local through its macro)
	void topDeclarations(const cinsn_t* block, int depth)
	{
		auto found = m_declared.find(block);
		if (found == m_declared.end())
			return;
		for (int v : found->second)
		{
			const Local& local = m_function.locals[v];
			typeAlias(v, depth);
			std::string text = (local.inlined ? inlinedDeclaration(v) : declareLocal(v));
			if (!local.initialiser.empty())
			{
				text += " = " + local.initialiser;
				m_includes.insert(local.initialiserHeader);
			}
			text += ";";
			if (local.type.empty() || local.storage)
				text += marker(nullptr, local.why, false, v);
			if (local.undefined)
				text += marker(nullptr, "possibly undefined: an uninitialised read");
			line(depth, text);
		}
	}

	void blockItems(const cinsn_t* s, int depth)
	{
		if (s->op != cit_block)
		{
			statement(s, depth);
			return;
		}
		topDeclarations(s, depth);
		std::vector<const cinsn_t*> list;
		for (const cinsn_t& item : *s->cblock)
			list.push_back(&item);
		statements(list, 0, list.size(), depth, list.size());
	}

	// list[from, to) at depth: the statements an idiom's rewrite replaces after its first skipped; a frame (Function's
	// frames) in braces from its first statement (opened: the one whose frame is open already)
	void statements(const std::vector<const cinsn_t*>& list, size_t from, size_t to, int depth, size_t opened)
	{
		for (size_t n = from; n < to;)
		{
			auto frame = n == opened ? m_function.frames.end() : m_function.frames.find(list[n]);
			if (frame != m_function.frames.end())
			{
				size_t end = std::min(to, n + frame->second);
				line(depth, "{");
				statements(list, n, end, depth + 1, n);
				line(depth, "}");
				n = end;
				continue;
			}
			const Rewrite* rewrite = as<Rewrite>(list[n]);
			statement(list[n], depth);
			n += rewrite ? std::max<size_t>(rewrite->count, 1) : 1;
		}
	}

	// an idiom's form at depth, each `$N` items[N]: an expression in place (as an operand of a postfix operator: in
	// parentheses unless it binds as tightly), a statement alone on its line as the statements it holds, at that
	// line's depth
	void rewritten(const Rewrite& rewrite, int depth)
	{
		for (size_t start = 0, end = 0; start <= rewrite.form.size(); start = end + 1)
		{
			end = rewrite.form.find('\n', start);
			if (end == std::string::npos)
				end = rewrite.form.size();
			std::string text = rewrite.form.substr(start, end - start);
			size_t indent = text.find_first_not_of('\t');
			indent = indent == std::string::npos ? text.size() : indent;
			const citem_t* alone = text.size() > indent + 1 && text[indent] == '$' &&
										   text.find_first_not_of("0123456789", indent + 1) == std::string::npos
									   ? rewrite.items.at(std::stoul(text.substr(indent + 1)))
									   : nullptr;
			if (alone && !alone->is_expr())
				blockItems(static_cast<const cinsn_t*>(alone), depth + static_cast<int>(indent));
			else
				line(depth + static_cast<int>(indent), substituted(rewrite, text.substr(indent)));
		}
	}

	// the expression item an expression's form is alone (`$0`), nullptr for a longer form
	static const cexpr_t* loneItem(const Rewrite& rewrite)
	{
		const std::string& form = rewrite.form;
		if (form.size() < 2 || form[0] != '$' || form.find_first_not_of("0123456789", 1) != std::string::npos)
			return nullptr;
		const citem_t* item = rewrite.items.at(std::stoul(form.substr(1)));
		return item->is_expr() ? static_cast<const cexpr_t*>(item) : nullptr;
	}

	// a form line with each `$N` the expression items[N]
	std::string substituted(const Rewrite& rewrite, const std::string& form)
	{
		std::string text;
		if (!rewrite.header.empty())
			m_includes.insert(rewrite.header);
		for (size_t k = 0; k < form.size(); ++k)
		{
			// `*$N`: the object item N points at, as deref spells it (`&x` is `x`)
			bool object = form[k] == '*' && k + 1 < form.size() && form[k + 1] == '$';
			size_t dollar = object ? k + 1 : k;
			size_t digits = dollar + 1;
			while (form[dollar] == '$' && digits < form.size() && std::isdigit(form[digits]))
				++digits;
			if (digits == dollar + 1)
			{
				text += form[k];
				continue;
			}
			const auto* item =
				static_cast<const cexpr_t*>(rewrite.items.at(std::stoul(form.substr(dollar + 1, digits - dollar - 1))));
			std::string spelled = object ? deref(item) : operand(item, cot_memptr);
			// `*$N.m`: a member of that object, `(*p).m` where the object is spelled through `*`
			if (object && digits < form.size() && form[digits] == '.' && spelled.starts_with('*'))
				spelled = "(" + spelled + ")";
			// a member reached through the item (`$N->m`, `$N.m`, `*$N.m`) needs the item's class complete (its
			// header holds the class's by-value members' types too)
			bool arrow = form.compare(digits, 2, "->") == 0, dot = digits < form.size() && form[digits] == '.';
			if (arrow || (dot && object))
				includeObjectClass(item->type.is_ptr() ? item->type.get_pointed_object() : item->type);
			else if (dot)
				includeObjectClass(item->type);
			text += spelled;
			k = digits - 1;
		}
		return text;
	}

	void statement(const cinsn_t* s, int depth)
	{
		// a statement an idiom's form makes moot
		if (m_dropped.count(s))
			return;
		if (s->label_num >= 0)
			line(depth > 0 ? depth - 1 : 0, "LABEL_" + std::to_string(s->label_num) + ":;");
		if (const Rewrite* rewrite = as<Rewrite>(s))
		{
			rewritten(*rewrite, depth);
			return;
		}
		markers(s, depth);
		switch (s->op)
		{
		case cit_block:
			line(depth, "{");
			blockItems(s, depth + 1);
			line(depth, "}");
			break;
		case cit_expr:
			// an alias's copy statement goes, as do the statements src leaves out
			if (m_copies.count(s) || m_function.omitted(s))
				break;
			if (const InlinedStatement* inlined = as<InlinedStatement>(s))
			{
				// the macro does the vptr stores (Construct and Destruct calls): the constructor's becomes the
				// declaration, the destructor's goes
				if (inlined->macro && !m_declared.count(s))
					break;
				if (!inlined->macro)
					line(depth, "// MAYBE-INLINED " + hexUpper(s->ea) + ": " + inlined->what);
			}
			if (auto declared = m_declared.find(s); declared != m_declared.end())
				typeAlias(declared->second.front(), depth);
			line(depth, expressionStatement(s) + ";");
			break;
		case cit_if:
		{
			line(depth, "if (" + expr(&s->cif->expr) + ") {");
			blockItems(s->cif->ithen, depth + 1);
			const cinsn_t* otherwise = s->cif->ielse;
			while (otherwise && otherwise->op == cit_if && otherwise->label_num < 0)
			{
				line(depth, "} else if (" + expr(&otherwise->cif->expr) + ") {");
				blockItems(otherwise->cif->ithen, depth + 1);
				otherwise = otherwise->cif->ielse;
			}
			if (otherwise)
			{
				line(depth, "} else {");
				blockItems(otherwise, depth + 1);
			}
			line(depth, "}");
			break;
		}
		case cit_for:
			line(depth, "for (" + forInit(s) + "; " + expr(&s->cfor->expr) + "; " + expr(&s->cfor->step) + ") {");
			blockItems(s->cfor->body, depth + 1);
			line(depth, "}");
			break;
		case cit_while:
			line(depth, "while (" + expr(&s->cwhile->expr) + ") {");
			blockItems(s->cwhile->body, depth + 1);
			line(depth, "}");
			break;
		case cit_do:
			line(depth, "do {");
			blockItems(s->cdo->body, depth + 1);
			line(depth, "} while (" + expr(&s->cdo->expr) + ");");
			break;
		case cit_switch:
		{
			const cexpr_t& value = s->cswitch->expr;
			line(depth, "switch (" + expr(&value) + ") {");
			for (const ccase_t& c : s->cswitch->cases)
			{
				const Enumerators* names = as<Enumerators>(&c);
				auto label = [&](size_t k) {
					return "case " + (names ? enumerator(*names, k) : literal(c.values[k], value.type));
				};
				for (size_t k = 0; k + 1 < c.values.size(); ++k)
					line(depth, label(k) + ":");
				std::string last = c.values.empty() ? "default" : label(c.values.size() - 1);
				line(depth, mark(&c, last + ": {"));
				blockItems(&c, depth + 1);
				line(depth, "}");
			}
			line(depth, "}");
			break;
		}
		case cit_return:
		{
			const cexpr_t& value = s->creturn->expr;
			if (value.op == cot_empty)
				line(depth, "return;");
			else
			{
				tinfo_t ftype;
				m_function.cfunc->get_func_type(&ftype);
				std::string text = m_function.returnsReference ? deref(&value)
															   : converted(s, &value, ftype.get_rettype());
				line(depth, mark(s, "return " + text + ";"));
			}
			break;
		}
		case cit_goto:
			line(depth, "goto LABEL_" + std::to_string(s->cgoto->label_num) + ";");
			break;
		case cit_break:
			line(depth, "break;");
			break;
		case cit_continue:
			line(depth, "continue;");
			break;
		case cit_asm:
		default:
			// no form (inline asm, a try block, a throw): the overlay's default marks it
			line(depth, mark(s, ";"));
			break;
		}
	}

	// a for statement's init: `T i = expr` where it declares its counter
	std::string forInit(const cinsn_t* s)
	{
		const cexpr_t* init = &s->cfor->init;
		auto found = m_declared.find(s);
		if (found == m_declared.end())
			return expr(init);
		int v = found->second.front();
		const Local& local = m_function.locals[v];
		const tinfo_t& type = (*m_function.cfunc->get_lvars())[v].type();
		std::string text = mark(init, declareLocal(v) + " = " + converted(init, init->y, type));
		return local.type.empty() ? text + marker(nullptr, local.why, false, v) : text;
	}

	// `T v = expr` where the statement declares v (FNVD_INLINED_LOCAL for an inlined local; a call returning the
	// record into v, `T v = F(...)`), a call without the address of a returned reference, else expr
	std::string expressionStatement(const cinsn_t* s)
	{
		const cexpr_t* e = s->cexpr;
		auto found = m_declared.find(s);
		if (found != m_declared.end())
		{
			int v = found->second.front();
			const Local& local = m_function.locals[v];
			if (local.inlined)
				return inlinedDeclaration(v);
			const tinfo_t& type = (*m_function.cfunc->get_lvars())[v].type();
			std::string value;
			if (byValueCall(e))
			{
				m_initialising.insert(e);
				value = call(e);
			}
			else
				value = converted(e, e->y, type);
			std::string text = mark(e, declareLocal(v) + " = " + value);
			return local.type.empty() ? text + marker(nullptr, local.why, false, v) : text;
		}
		// a call prints as one (no address of a returned reference: its result is unused), unless an idiom rewrites
		// it or it is a setter's store
		const Access* access = as<Access>(e);
		bool asCall = e->op == cot_call && !as<Rewrite>(e) && !(access && access->value);
		return asCall ? call(e) : expr(e);
	}

	// FNVD_INLINED_LOCAL(Class, v, start, end): the vtable constants (their headers included), std::nullopt for none;
	// FNVD_CONSTRUCTED_LOCAL(Class, v, &Class::Destruct or nullptr, the Construct call's arguments after the memory)
	std::string inlinedDeclaration(int v)
	{
		const Local& local = m_function.locals[v];
		auto table = [&](const SrcModel::Ref* ref) -> std::string {
			if (!ref)
				return "std::nullopt";
			if (ref->file.rfind("src/", 0) == 0)
				m_includes.insert(ref->file.substr(4));
			return ref->target;
		};
		m_types.headers(local.type, m_includes);
		std::string type = macroType(v);
		if (const cexpr_t* construct = local.inlined->construct)
		{
			const Call& resolved = *as<Call>(construct);
			calleeHeaders(resolved.function);
			std::string list = arguments(construct, resolved, 1);
			std::string declaration = frameAlignment(v) + "FNVD_CONSTRUCTED_LOCAL(" + type + ", " + var(v) + ", " +
									  (local.inlined->destruct ? "&" + type + "::Destruct" : "nullptr") +
									  (list.empty() ? "" : ", ") + list + ")";
			return resolved.unpinned.empty() ? declaration : declaration + marker(construct, resolved.unpinned);
		}
		return frameAlignment(v) + "FNVD_INLINED_LOCAL(" + type + ", " + var(v) + ", " +
			   table(local.inlined->atStart) + ", " + table(local.inlined->atEnd) + ")";
	}

	// the type a macro declaring inlined local v takes: its spelling, or where that has a comma (which would split
	// the macro's arguments) the block-scope alias typeAlias declares (porting rules: a `using` alias)
	std::string macroType(int v)
	{
		std::string type = m_types.spell(m_function.locals[v].type).value_or("?");
		return type.find(',') == std::string::npos ? type : var(v) + "_t";
	}

	// `using v_t = X<A, B>;` on the line before an inlined local's macro whose type has a comma
	void typeAlias(int v, int depth)
	{
		const Local& local = m_function.locals[v];
		std::string type = local.inlined ? m_types.spell(local.type).value_or("?") : "";
		if (type.find(',') != std::string::npos)
			line(depth, "using " + var(v) + "_t = " + type + ";");
	}

	// --- expressions

	// one of an item's enumerators (its enum's header included)
	std::string enumerator(const Enumerators& names, size_t k)
	{
		m_types.headers(names.type, m_includes);
		return names.names[k];
	}

	// a number's (or its cast's) enumerators in their form
	std::string enumerators(const Enumerators& names, const cexpr_t* e)
	{
		using Form = Enumerators::Form;
		if (names.form == Form::one)
			return enumerator(names, 0);
		std::string joined;
		for (size_t k = 0; k < names.names.size(); ++k)
			joined += (k ? " | " : "") + enumerator(names, k);
		if (names.form == Form::cast)
			return "static_cast<" + declare(names.type, e->type) + ">(" + joined + ")";
		if (names.form == Form::complement)
			return names.names.size() > 1 ? "~(" + joined + ")" : "~" + joined;
		return joined;
	}

	static int enumeratorsPrecedence(const Enumerators& names)
	{
		using Form = Enumerators::Form;
		return names.form == Form::cast		  ? kPostfix
			   : names.form == Form::combined   ? kBitOr
			   : names.form == Form::complement ? kUnary
												: kPrimary;
	}

	std::string var(int v) const
	{
		// an alias is its original
		if (m_function.locals[v].aliasOf >= 0)
			return var(m_function.locals[v].aliasOf);
		// a virtual the exe enters with a base subobject in this (registered _VIA): Hex-Rays' this is that subobject
		const Local& local = m_function.locals[v];
		if (!m_function.virtualBase.empty() && local.name == "this" && v == m_function.cfunc->argidx[0])
			return "static_cast<" + m_types.spell(local.type).value_or(m_function.virtualBase + "*") + ">(this)";
		return local.name;
	}

	// e as an operand of the parent operator (right: its right-hand operand), in parentheses where C++'s precedence
	// needs them or clang warns without them
	std::string operand(const cexpr_t* e, ctype_t parent, bool right = false)
	{
		std::string text = expr(e);
		int inner = precedence(e), outer = precedenceOf(parent);
		bool wrap = inner < outer || clangWarns(parent, e->op);
		// a binary operator associates left, an assignment and ?: right
		if (inner == outer && outer < kUnary)
			wrap = outer == kAssignment ? !right : right;
		// `- -x`, not `--x`
		if ((parent == cot_neg || parent == cot_fneg) && !text.empty() && text[0] == '-')
			wrap = true;
		return wrap ? "(" + text + ")" : text;
	}

	// the precedence of e as printed (its operator's, unless the printer spells it otherwise)
	int precedence(const cexpr_t* e)
	{
		// a conversion is a cast or a call; an enum's or a void pointer's step an assignment; a member's address a
		// unary `&`; C++'s implicit conversion the value as it prints
		if (const Conversion* c = conversion(e))
		{
			if (c->kind == Conversion::Kind::implicit)
				return c->replacesCast ? precedence(e->x) : ownPrecedence(e);
			return c->kind == Conversion::Kind::enumStep || c->kind == Conversion::Kind::byteStep ? kAssignment
				   : c->kind == Conversion::Kind::memberAddress									? kUnary
																									: kPostfix;
		}
		return ownPrecedence(e);
	}

	// the precedence of e as its own meaning prints it (plain), without its conversion
	int ownPrecedence(const cexpr_t* e)
	{
		// an idiom's form for an expression is a name or a postfix expression (`rpItem`, `$0.GetSize()`), or one item
		// alone, which prints as that item
		if (const Rewrite* rewrite = as<Rewrite>(e))
			return loneItem(*rewrite) ? precedence(loneItem(*rewrite)) : kPostfix;
		if (as<Implicit>(e))
		{
			if (e->op == cot_cast)
				return precedence(e->x);
			const cexpr_t* b = outermostBase(e->x);
			return b->op == cot_memptr ? precedence(b->x) : kUnary;
		}
		switch (e->op)
		{
		case cot_num:
			if (const Enumerators* names = as<Enumerators>(e))
				return enumeratorsPrecedence(*names);
			return number(e)[0] == '-' || (as<Missing>(e) && e->type.is_enum()) ? kUnary : kPrimary;
		case cot_fnum:
			return expr(e)[0] == '-' ? kUnary : kPrimary;
		case cot_var:
			return referenceVar(e) ? kUnary : kPrimary;
		case cot_call:
			if (const Access* access = as<Access>(e))
				return !access->field ? precedence(receiverOf(e))
					   : access->value ? kAssignment
					   : access->load  ? kPostfix
									   : kUnary;
			// a record returned by value: `&(v = F())`, a bare assignment where Hex-Rays has no result; built in its
			// object (placeDeclarations), the new-expression, whose value is the object's address
			if (byValueCall(e))
				return placement(e) || !e->type.is_void() ? kUnary : precedenceOf(cot_asg);
			return referenceCall(e) ? kUnary : kPostfix;
		case cot_ref:
			// an unresolved address prints as a C cast of IDA's pointer type (a base subobject's included)
			if (as<Missing>(e) && !functionStart(e->x))
				return kUnary;
			return functionStart(e->x) || baseSubobject(e->x) ? kPostfix : kUnary;
		case cot_ptr:
			if (as<Member>(e))
				return kPostfix;
			if (as<Missing>(e) || referenceVar(e->x))
				return kPrimary;
			if (e->x->op == cot_ref)
				return precedence(e->x->x);
			// a temporary's object (foldHiddenResults): the call, or its infix
			if (std::optional<int> object = temporaryObject(e->x))
				return *object;
			return placement(e->x)		 ? kUnary
				   : byValueCall(e->x)	 ? kPrimary
				   : referenceCall(e->x) ? kPostfix
										 : kUnary;
		case cot_cast:
			if (const Enumerators* names = as<Enumerators>(e))
				return enumeratorsPrecedence(*names);
			return as<std::string>(e) || as<ConstCast>(e) || as<ExeCast>(e) ? kPostfix : kUnary;
		default:
			// a comparison printed negated for a NaN (`!(a >= b)`)
			return nanOutcomeDiffers(e) ? kUnary : precedenceOf(e->op);
		}
	}

	// *e
	std::string deref(const cexpr_t* e)
	{
		// a converted pointer: the object it points to (a member's address: the member)
		if (const Conversion* c = conversion(e))
			return c->kind == Conversion::Kind::memberAddress && c->what.empty() ? memberOf(e, *c)
																				  : "*" + operand(e, cot_ptr);
		// a rewrite to an address (`&$0->field`): the object it is the address of
		if (const Rewrite* rewrite = as<Rewrite>(e); rewrite && rewrite->form.starts_with('&'))
			return substituted(*rewrite, rewrite->form.substr(1));
		if (as<Implicit>(e) && e->op == cot_cast)
			return deref(e->x);
		if (as<Implicit>(e))
		{
			const cexpr_t* b = outermostBase(e->x);
			return b->op == cot_memptr ? deref(b->x) : expr(b->x);
		}
		if (e->op == cot_ref)
			return expr(e->x);
		if (referenceVar(e))
			return var(lvarIndex(e));
		if (referenceCall(e))
			return call(e);
		if (placement(e))
			return "*" + call(e);
		if (byValueCall(e))
			return temporary(e) ? call(e) : "(" + call(e) + ")";
		return "*" + operand(e, cot_ptr);
	}

	// an implicit conversion's operand: a cast's, or the pointer to the object of a base subobject's address
	std::string implicitOperand(const cexpr_t* e)
	{
		if (e->op == cot_cast)
			return expr(e->x);
		const cexpr_t* b = outermostBase(e->x);
		return b->op == cot_memptr ? expr(b->x) : "&" + operand(b->x, cot_ref);
	}

	// e as the value an assignment or return (item) gives: through the static_cast a pass recorded on item (a number
	// for an enum), or e's own conversion (a void* to another pointer, a marked one); where a pass left the
	// conversion unresolved, through C++'s spelling of the one C makes implicitly to IDA's type
	std::string converted(const citem_t* item, const cexpr_t* e, const tinfo_t& to)
	{
		if (const std::string* key = as<std::string>(item))
			return "static_cast<" + declare(*key, to) + ">(" + expr(e) + ")";
		if (conversion(e))
			return expr(e);
		pointeeHeaders(e);
		if (as<Missing>(item) && !to.empty())
		{
			// IDA's pointer that prints as a src integer (a field src types int) converts as the integer
			std::string printed = srcType(m_function, m_context, e);
			std::string kind = printed.empty() ? "" : m_types.parts(m_types.canonical(printed)).kind;
			bool pointer = kind.empty() || kind == "ptr" || kind == "array" || kind == "fn";
			if (e->type.is_ptr() && to.is_integral())
				return pointer ? "(" + raw(to) + ")reinterpret_cast<uintptr_t>(" + expr(e) + ")"
							   : "(" + raw(to) + ")" + operand(e, cot_cast);
			if (e->type.is_integral() && e->op != cot_num && to.is_ptr())
				return "reinterpret_cast<" + raw(to) + ">(static_cast<uintptr_t>(" + expr(e) + "))";
			if (e->type.is_ptr() && to.is_ptr())
				return "(" + raw(to) + ")" + operand(e, cot_cast);
		}
		// marked: a local a pass typed from the call it is assigned (a src pointer where Hex-Rays has void* or int, or
		// the owner's result Hex-Rays' cast went for), the call left unresolved by a later layer and printed raw with
		// IDA's result type
		const cexpr_t* local = item->op == cot_asg ? static_cast<const cexpr_t*>(item)->x : nullptr;
		const cexpr_t* result = e;
		while (result->op == cot_cast && as<Implicit>(result))
			result = result->x;
		if (int v = local ? lvarIndex(local) : -1; !m_src && v >= 0 && result->op == cot_call && as<Missing>(result) &&
												   (result->type.is_ptr() || result->type.is_integral()))
		{
			const std::string& type = m_function.locals[v].type;
			std::string why;
			std::optional<std::string> key = m_types.keyOf(result->type, why);
			if (!type.empty() && m_types.parts(m_types.canonical(type)).kind == "ptr" &&
				(!key || m_types.canonical(*key) != m_types.canonical(type)))
			{
				std::string spelled = declare(type, to);
				if (result->type.is_integral())
					return "reinterpret_cast<" + spelled + ">(static_cast<uintptr_t>(" + expr(result) + "))";
				return (result->type.get_pointed_object().is_void() ? "static_cast<" : "reinterpret_cast<") + spelled +
					   ">(" + expr(result) + ")";
			}
		}
		// marked: Hex-Rays assigns between pointer types C allows and C++ may not (not where src's types agree: a
		// local a pass retyped, an element of a src array or pointer)
		if (!m_src && !to.empty() && e->type.is_ptr() && to.is_ptr() && text(e->type) != text(to))
		{
			const cexpr_t* assigned = item->op == cot_asg ? static_cast<const cexpr_t*>(item)->x : nullptr;
			std::string target = assigned				  ? lvalueType(assigned)
								 : item->op == cit_return ? m_function.returnType
														  : "";
			std::string value = srcType(m_function, m_context, e);
			if (target.empty() || value.empty() || m_types.canonical(target) != m_types.canonical(value))
				return "(" + raw(to) + ")" + operand(e, cot_cast);
		}
		return operand(e, cot_asg, true);
	}

	// the src type of an assigned lvalue: an element (`table[i]`, `*p`) that has no meaning of its own is of its
	// operand's src element type, as it prints (src's array or pointer, a src function-pointer table whose references
	// Hex-Rays reads as pointers included); anything else srcType's
	std::string lvalueType(const cexpr_t* lvalue) const
	{
		if ((lvalue->op == cot_idx || lvalue->op == cot_ptr) && !m_function.meaning.count(lvalue))
			if (std::string pointer = srcType(m_function, m_context, lvalue->x); !pointer.empty())
			{
				SrcTypes::Parts p = m_types.parts(m_types.canonical(pointer));
				if (p.kind == "ptr" || (lvalue->op == cot_idx && p.kind == "array"))
					return p.of;
			}
		return srcType(m_function, m_context, lvalue);
	}

	// a memptr/memref the Fields pass resolved to a base subobject
	bool baseSubobject(const cexpr_t* e) const
	{
		return (e->op == cot_memptr || e->op == cot_memref) && as<std::string>(e);
	}

	// the base subobject access b is reached through, outermost (its operand is the object's pointer or object)
	const cexpr_t* outermostBase(const cexpr_t* b) const
	{
		while (baseSubobject(b->x))
			b = b->x;
		return b;
	}

	// what precedes the member name of the object e points to: `p->`, `x.` (e is &x or a reference); for &x of a base
	// subobject x or an implicit conversion, its object's; nothing for C++'s `this` (isThis: in a _VIA virtual
	// Hex-Rays' ADJ(this), not its this, the base subobject) in a member definition where no local shadows the member
	std::string memberOf(const cexpr_t* e, const std::string& name)
	{
		// a converted pointer: a member of the object it points to
		if (conversion(e))
			return operand(e, cot_memptr) + "->";
		// a rewrite to an address (`&$0->field`): a member of the object it is the address of
		if (const Rewrite* rewrite = as<Rewrite>(e); rewrite && rewrite->form.starts_with('&'))
		{
			std::string object = substituted(*rewrite, rewrite->form.substr(1));
			return (temporaryObject(e).value_or(kPostfix) < kPostfix ? "(" + object + ")" : object) + ".";
		}
		if (as<Implicit>(e) && e->op == cot_cast)
			return memberOf(e->x, name);
		if (isThis(m_function, e) && m_function.shadowed && !m_function.shadowed->count(name))
			return "";
		if (e->op == cot_ref)
			return baseSubobject(e->x) ? throughBases(e->x, name) : operand(e->x, cot_memref) + ".";
		if (referenceVar(e))
			return var(lvarIndex(e)) + ".";
		// a call's temporary (foldHiddenResults)
		if (std::optional<int> inner = temporary(e) ? temporaryObject(e) : std::nullopt)
			return (*inner < kPostfix ? "(" + call(e) + ")" : call(e)) + ".";
		return operand(e, cot_memptr) + "->";
	}

	// what precedes the member name reached through the base subobject b: what precedes it in b's object
	std::string throughBases(const cexpr_t* b, const std::string& name)
	{
		b = outermostBase(b);
		return b->op == cot_memptr ? memberOf(b->x, name) : operand(b->x, cot_memref) + ".";
	}

	std::string field(const cexpr_t* e)
	{
		if (const Member* m = as<Member>(e))
		{
			// member access needs the class declaring the member, and the accessed object's class, complete
			m_types.headers("record:" + m->owner, m_includes);
			// (through base subobjects: the object they are bases of)
			const cexpr_t* object = baseSubobject(e->x) ? outermostBase(e->x) : e;
			includeObjectClass(object->op == cot_memptr ? object->x->type.get_pointed_object() : object->x->type);
			const std::string& name = m->field->name;
			std::string prefix = baseSubobject(e->x)	? throughBases(e->x, name)
								 : e->op == cot_memptr ? memberOf(e->x, name)
													   : operand(e->x, cot_memref) + ".";
			// a qualified member of the implicit this keeps `this->`: `&X::f` alone is a pointer to member
			if (prefix.empty() && !m->qualifier.empty())
				prefix = "this->";
			return prefix + (m->qualifier.empty() ? "" : m->qualifier + "::") + name;
		}
		// a base subobject read whole
		if (const std::string* key = as<std::string>(e))
			return "static_cast<" + declare(*key, e->type) + "&>(" + (e->op == cot_memptr ? deref(e->x) : expr(e->x)) +
				   ")";
		// a dword of the thread's TEB (`NtCurrentTeb()->ThreadLocalStoragePointer`): read through fs, as the exe does
		if (e->op == cot_memptr && e->x->op == cot_call && e->x->x->op == cot_helper &&
			std::string(e->x->x->helper) == "NtCurrentTeb" && e->type.get_size() == 4)
			return mark(e, "((" + raw(e->type) + ")gen::readfs32(" + hexUpper(e->m) + "u))");
		// a call's result is a temporary, which `&` does not take: GenSupport's lvalue names it for its full-expression
		std::string base = e->op == cot_memptr ? expr(e->x)
						   : e->x->op == cot_call ? "&gen::lvalue(" + expr(e->x) + ")"
												  : "&" + operand(e->x, cot_ref);
		tinfo_t owner = e->op == cot_memptr ? e->x->type.get_pointed_object() : e->x->type;
		uint64_t offset = owner.is_union() ? 0 : e->m;
		tinfo_t held = memberType(e);
		if (held.is_array())
			return mark(e, "(&" + at(held.get_array_element(), base, offset) + ")");
		return mark(e, at(e->type, base, offset));
	}

	// the text is one function-like macro argument: no comma outside parentheses (`X<A, B>` is two)
	static bool oneMacroArgument(const std::string& text)
	{
		int depth = 0;
		for (char c : text)
		{
			depth += c == '(';
			depth -= c == ')';
			if (c == ',' && depth == 0)
				return false;
		}
		return true;
	}

	// the object of IDA's type at base + offset (FNVD_AT, or its expansion where the type has a parenthesis or is an
	// array, whose pointer type the macro's `T*` cannot spell, or the type or base is more than one macro argument)
	std::string at(const tinfo_t& type, const std::string& base, uint64_t offset)
	{
		std::string spelled = raw(type);
		if (spelled.find_first_of(",([") == std::string::npos && oneMacroArgument(base))
			return "FNVD_AT(" + spelled + ", " + base + ", " + hexUpper(offset) + ")";
		return "(*(" + raw(type, "*") + ")((uint8_t*)(" + base + ") + " + hexUpper(offset) + "))";
	}

	std::string number(const cexpr_t* e)
	{
		uint64_t value = e->n->_value;
		if (const Enumerators* names = as<Enumerators>(e))
			return enumerators(*names, e);
		if (as<Missing>(e) && e->type.is_ptr())
			return mark(e, "reinterpret_cast<" + raw(e->type) + ">(" + hexUpper(value) + "u)");
		if (e->type.is_ptr())
			return "nullptr";
		if (e->type.is_bool())
			return value ? "true" : "false";
		// a number of IDA's enum type with no enumerator: its cast
		if (as<Missing>(e) && e->type.is_enum())
			return mark(e, "(" + raw(e->type) + ")" + literal(value, e->type));
		// a number Calls typed as a 4-byte integer parameter of an overloaded callee: that type's literal (`0u`)
		if (const std::string* key = as<std::string>(e); key && !e->type.is_enum())
		{
			std::string k = m_types.canonical(*key);
			if (k == "builtin:unsigned int" || k == "builtin:int")
				return mark(e, literal(value, tinfo_t(k == "builtin:int" ? BTF_INT32 : BTF_UINT32)));
		}
		return mark(e, literal(value, e->type));
	}

	// a src global's name, `::`-qualified where a local of the same name hides it (Hex-Rays names locals after the
	// globals they load), its header included
	std::string globalName(const SrcModel::Ref& ref)
	{
		if (ref.file.rfind("src/", 0) == 0)
			m_includes.insert(ref.file.substr(4));
		const std::string target = inTu(ref.target);
		bool hidden = target.find("::") == std::string::npos &&
					  std::any_of(m_function.locals.begin(), m_function.locals.end(),
								  [&](const Local& local) { return local.name == target; });
		return hidden ? "::" + target : target;
	}

	std::string global(const cexpr_t* e)
	{
		if (const SrcModel::Ref* const* ref = as<const SrcModel::Ref*>(e))
			return globalName(**ref);
		// read by its address (resolveGlobals' addressedGlobals): its element's object's address
		auto addressed = m_function.addressedGlobals.find(e);
		bool array = addressed != m_function.addressedGlobals.end();
		std::string element = array ? raw(addressed->second) : "";
		// a src global read as another type: the object of that type at its address
		if (const GlobalAs* other = as<GlobalAs>(e))
		{
			std::string name = globalName(*other->ref);
			std::string text = array ? "(&gen::as<" + element + ">(" + name + "))"
									 : "gen::as<" + raw(e->type) + ">(" + name + ")";
			return nested(e, text) + marker(e, other->what);
		}
		std::string name = "g_" + hex8(e->obj_ea);
		std::string type = array ? element : raw(e->type);
		std::string declaration;
		if (type.find_first_of(",()[") != std::string::npos)
			declaration = "using " + name + "_t = " + type + ";\nstatic FNVD_GLOBAL(" + name + "_t, " + name + ", " +
						  hexText(e->obj_ea) + ");";
		else
			declaration = "static FNVD_GLOBAL(" + type + ", " + name + ", " + hexText(e->obj_ea) + ");";
		// one declaration per address: a use as another type than the first reads the declared object as its type
		auto [declared, added] = m_globals.emplace(e->obj_ea, declaration);
		std::string object = added || declared->second == declaration ? name : "gen::as<" + type + ">(" + name + ")";
		return mark(e, array ? "(&" + object + ")" : object);
	}

	// a trivial accessor's call as its access on the receiver
	std::string accessed(const cexpr_t* call, const Access& access)
	{
		const cexpr_t* receiver = receiverOf(call);
		if (!access.field)
			return expr(receiver);
		// member access needs the receiver's class complete
		includeObjectClass(receiver->type.get_pointed_object());
		std::string path;
		for (const SrcModel::Field* member : access.through)
			path += member->name + ".";
		path += access.field->name;
		const std::string& outermost = access.through.empty() ? access.field->name : access.through.front()->name;
		std::string prefix = memberOf(receiver, outermost);
		// a qualified member of the implicit this keeps `this->`: `&X::f` alone is a pointer to member
		if (prefix.empty() && !access.qualifier.empty())
			prefix = "this->";
		std::string member = prefix + (access.qualifier.empty() ? "" : access.qualifier + "::") + path;
		if (access.value)
			return member + " = " + operand(access.value, cot_asg, true);
		return access.load ? member : "&" + member;
	}

	// the header of the src record an accessed object is (IDA's struct for it), which its member access needs complete
	void includeObjectClass(const tinfo_t& object)
	{
		if (const SrcModel::Record* record = m_types.recordOf(object))
			m_types.headers("record:" + record->name, m_includes);
	}

	// the object argument without Hex-Rays' casts
	static const cexpr_t* receiverOf(const cexpr_t* call)
	{
		const cexpr_t* e = &(*call->a)[0];
		while (e->op == cot_cast)
			e = e->x;
		return e;
	}

	// a record-returning arithmetic operator (`operator-` of NiPoint3) as C++ spells it (infixOperator): `*a - b`,
	// `-*a`; empty for another call. An operand that is a temporary's infix (foldHiddenResults) is parenthesised where
	// C++ would bind it otherwise.
	std::string arithmeticOperator(const cexpr_t* e, const Call& resolved, bool member, const std::string& name)
	{
		ctype_t op = infixOperator(e);
		if (op == cot_empty)
			return "";
		ctype_t binary = op == cot_neg ? cot_sub : op;
		int outer = op == cot_neg ? kUnary : precedenceOf(op);
		const carglist_t& args = *e->a;
		std::vector<std::string> operands;
		for (size_t k = 0; k < args.size(); ++k)
			if (static_cast<int>(k) != resolved.resultArgument)
			{
				bool right = operands.size() > 0;
				if (!((member && k == 0) || resolved.byReference[k]))
				{
					operands.push_back(operand(&args[k], binary, right));
					continue;
				}
				std::string object = deref(&args[k]);
				int inner = temporaryObject(&args[k]).value_or(kPostfix);
				operands.push_back(inner < outer || (inner == outer && right) ? "(" + object + ")" : object);
			}
		std::string symbol = name.substr(8);
		return op == cot_neg ? "-" + operands[0] : operands[0] + " " + symbol + " " + operands[1];
	}

	// the headers a call of the src function needs: its declaration's, its definition's where that is another (a
	// template instance's explicit specialization; its template's definition comes with the template), and those of the
	// types the call converts between (complete) and of a template instance's arguments (its members' bodies use them:
	// NiPointer<X>'s destructor needs X)
	void calleeHeaders(const SrcModel::Function* target)
	{
		if (target->file.rfind("src/", 0) == 0 && isHeader(target->file))
			m_includes.insert(target->file.substr(4));
		if (const SrcModel::Definition* definition = m_context.model.definition(target->mangled); definition &&
			!definition->instance && definition->file.rfind("src/", 0) == 0 && isHeader(definition->file))
			m_includes.insert(definition->file.substr(4));
		// an explicit specialization defined in another .cpp: its declaration at its home header
		if (std::string home = specializationHeader(m_context.model, target->mangled); !home.empty())
			m_includes.insert(home.substr(4));
		if (!target->cls.empty())
			m_types.headers("record:" + target->cls, m_includes);
		if (const SrcModel::Type* type = m_context.model.type(target->type))
		{
			m_types.headers(type->ret, m_includes);
			for (const std::string& p : type->params)
				m_types.headers(p, m_includes);
		}
	}

	// the headers of the class a pointer value points to (a derived class converts to its base only where it is
	// complete)
	void pointeeHeaders(const cexpr_t* e)
	{
		std::string key = srcType(m_function, m_context, e);
		if (!key.empty() && m_types.parts(m_types.canonical(key)).kind == "ptr")
			m_types.headers(key, m_includes);
	}

	// a resolved call's arguments from the first, as the callee takes them (the hidden result pointer left out, src's
	// apEdx a `nullptr`); each argument's class included (a derived one converts to the parameter's only where it is
	// complete)
	std::string arguments(const cexpr_t* e, const Call& resolved, size_t first)
	{
		const carglist_t& args = *e->a;
		std::string list;
		bool edx = resolved.edxParameter < 0;  // printed
		for (size_t k = first; k < args.size(); ++k)
		{
			if (static_cast<int>(k) == resolved.resultArgument)
				continue;
			int parameter = parameterIndex(resolved, k);
			if (!edx && parameter > resolved.edxParameter)
			{
				list += (list.empty() ? "" : ", ") + std::string("nullptr");
				edx = true;
			}
			if (!conversion(&args[k]))
				pointeeHeaders(&args[k]);
			const std::string& castTo = resolved.castTo[k];
			// an argument of a derived class's pointer converts to the parameter's base only where it is complete
			const SrcModel::Type* type = m_context.model.type(resolved.function->type);
			size_t param = static_cast<size_t>(parameter);
			if (std::string key = castTo.empty() ? srcType(m_function, m_context, &args[k]) : "";
				type && param < type->params.size() && m_types.parts(m_types.canonical(key)).kind == "ptr" &&
				m_types.canonical(key) != m_types.canonical(type->params[param]))
				m_types.headers(key, m_includes);
			std::string value = resolved.byReference[k] ? deref(&args[k])
								: castTo.empty()		? operand(&args[k], cot_asg, true)
														: "static_cast<" + declare(castTo, args[k].type) + ">(" +
															  expr(&args[k]) + ")";
			list += (list.empty() ? "" : ", ") + value;
		}
		if (!edx)
			list += (list.empty() ? "" : ", ") + std::string("nullptr");
		return list;
	}

	std::string call(const cexpr_t* e, bool lvalue = false)
	{
		const carglist_t& args = *e->a;
		if (const Call* resolved = as<Call>(e))
		{
			const SrcModel::Function* target = resolved->function;
			calleeHeaders(target);
			bool member = !target->cls.empty() && !target->isStatic;
			// the receiver's class (a derived one converts to the callee's only where it is complete)
			if (std::string key = member ? srcType(m_function, m_context, &args[0]) : ""; !key.empty())
				m_types.headers(key, m_includes);
			std::string name = lastComponent(target->name);
			// a receiver cast to the callee's class: name lookup from its own class finds another member
			std::string text = resolved->through ? operand(resolved->through, cot_call)
							   : !member		 ? (resolved->qualified ? "::" : "") + inTu(target->name)
							   : !resolved->castTo[0].empty()
								   ? "static_cast<" + declare(resolved->castTo[0], args[0].type) + ">(" + expr(&args[0]) +
										 ")->" + name
								   : memberOf(&args[0], name) + (resolved->qualified ? inTu(target->cls) + "::" : "") +
										 name;
			// a body clang could inline where the exe calls it: FNVD_NOINLINE, its own marker
			std::string called = text + "(" + arguments(e, *resolved, member ? 1 : 0) + ")";
			if (std::string infix = arithmeticOperator(e, *resolved, member, name); !infix.empty())
				called = infix;
			if (resolved->exeCalled)
				called = "FNVD_NOINLINE(" + called + ")";
			// the hidden result pointer folded back: the record stored to the object it points at (a local, the
			// body's own result, `*p`, a member or element; none where the call initialises the local its statement
			// declares), built in a whole object with placement new (placeDeclarations), or C++'s temporary
			if (resolved->resultArgument >= 0 && m_function.placements.count(e))
			{
				const carg_t& object = args[resolved->resultArgument];
				std::string address =
					object.op == cot_ref || referenceVar(&object) ? "&" + deref(&object) : expr(&object);
				const SrcModel::Type* type = m_context.model.type(resolved->function->type);
				m_includes.insert("<new>");
				called = "::new (" + address + ") " + declare(type ? type->ret : "", object.type.get_pointed_object()) +
						 "(" + called + ")";
			}
			else if (resolved->resultArgument >= 0 && !temporary(e) && !m_initialising.count(e))
				called = deref(&args[resolved->resultArgument]) + " = " + called;
			// an overload no cast pins (pinOverloads), printed as the call: its marker after it
			return resolved->unpinned.empty() ? called : called + marker(e, resolved->unpinned);
		}
		// each argument as the callee's printed type takes it (Hex-Rays' formal type where that has none); a number
		// passed to a Hex-Rays helper or compiler intrinsic as it is, its immediate (`_mm_shuffle_ps(a, b, 170)`, where
		// Hex-Rays' `char` parameter would make it -86); a number for a record formal (an empty functor the exe
		// pushes as a dword) as it is too: C++ casts no number to a record. A floating argument in an integer slot or
		// the reverse, which Hex-Rays passes uncast, is the bits the exe puts in the slot, never C++'s conversion of
		// the value: a floating argument past a variadic callee's formals in an 8-byte slot the double the exe stores
		// there (`fstp qword`), else the bits at the slot's size (gen::coerce; a float of an x87 value, as `fstp dword`)
		func_type_data_t params;
		tinfo_t function = e->x->type.is_ptr() ? e->x->type.get_pointed_object() : e->x->type;
		if (!function.get_func_details(&params))
			params.clear();
		std::vector<std::string> values;
		std::string list;
		for (size_t k = 0; k < args.size(); ++k)
		{
			const carg_t& arg = args[k];
			const tinfo_t& formal = k < params.size() ? params[k].type : arg.formal_type;
			std::string value = operand(&arg, cot_asg, true);
			bool immediate = (e->x->op == cot_helper || formal.is_udt()) && arg.op == cot_num;
			if (!formal.empty() && text(formal) != text(arg.type) && !conversion(&arg) && !immediate)
			{
				bool bits = formal.is_floating() != arg.type.is_floating() && formal.is_scalar() && arg.type.is_scalar();
				if (bits && arg.type.is_floating() && k >= params.size() && formal.get_size() == 8)
					value = "(double)" + operand(&arg, cot_cast);
				else if (bits && (formal.get_size() == arg.type.get_size() ||
								  (arg.type.is_floating() && formal.get_size() == 4)))
					value = "gen::coerce<" + raw(formal) + ">(" + expr(&arg) + ")";
				// Hex-Rays' cast to its 1-byte bool truncates, C++'s makes any nonzero value 1: the low byte
				else if (formal.is_bool() && arg.type.is_integral() && !arg.type.is_bool())
					value = "(uint8_t)" + operand(&arg, cot_cast);
				else
					value = "(" + raw(formal) + ")" + operand(&arg, cot_cast);
			}
			values.push_back(value);
			list += (list.empty() ? "" : ", ") + value;
		}
		const cexpr_t* callee = e->x;
		if (const Intrinsic* intrinsic = as<Intrinsic>(e))
		{
			if (!intrinsic->header.empty())
				m_includes.insert(intrinsic->header);
			return intrinsic->name + "(" + list + ")";
		}
		if (callee->op == cot_helper)
		{
			// CONTAINING_RECORD(p, T, f): the T whose member f p points at, by f's offset in IDA's T
			if (std::string_view(callee->helper) == "CONTAINING_RECORD")
				if (std::optional<uint64_t> offset = containingOffset(e))
					return mark(e, "((" + raw(e->type) + ")((uint8_t*)(" + expr(&args[0]) + ") - " +
									   hexUpper(*offset) + "))");
			// ADJ(p) of a shifted pointer (`B *__shifted(D, N)`): the D whose base subobject p points at, N bytes
			// before it
			ptr_type_data_t shifted;
			if (std::string_view(callee->helper) == "ADJ" && args.size() == 1 && e->type.is_ptr() &&
				args[0].type.get_ptr_details(&shifted) && (shifted.taptr_bits & TAPTR_SHIFTED))
				return mark(e, "((" + raw(e->type) + ")((uint8_t*)(" + expr(&args[0]) + ") - " +
								   hexUpper(static_cast<uint64_t>(shifted.delta)) + "))");
			// COERCE_<TYPE>(x): x's bits as the call's type, which Hex-Rays spells into the name (`COERCE_FLOAT`,
			// `COERCE_UINT32_T`, `COERCE_HKASKELETON_` for `hkaSkeleton *`)
			if (std::string_view(callee->helper).starts_with("COERCE_") && args.size() == 1)
				return mark(e, "gen::coerce<" + raw(e->type) + ">(" + list + ")");
			if (std::string text = genHelper(callee->helper, list, lvalue); !text.empty())
				return mark(e, text);
			return mark(e, std::string(callee->helper) + "(" + list + ")");
		}
		if (functionStart(callee))
		{
			tinfo_t type = callee->type.is_ptr() ? callee->type.get_pointed_object() : callee->type;
			std::string cc = conventionName(type.get_cc());
			// a variadic callee is cdecl: the caller cleans; a voidarg one takes nothing, which every convention
			// passes alike
			std::string function = cc == "cdecl" || cc == "vararg" || cc == "voidarg" ? "cdecl_" : cc;
			// a custom convention: the register arguments in fnvd::original::UserRegs (in its member order, which is
			// IDA's register numbering), the stack arguments after it
			if (cc == "usercall" || cc == "userpurge")
			{
				static const char* const registers[] = {"eax", "ecx", "edx", "ebx", nullptr, nullptr, "esi", "edi"};
				std::map<int, std::string> loaded;
				std::string stack;
				bool spelled = params.size() == values.size();
				for (size_t k = 0; spelled && k < values.size(); ++k)
				{
					const argloc_t& where = params[k].argloc;
					int reg = where.is_reg1() ? where.reg1() : -1;
					if (reg >= 0 && reg < 8 && registers[reg] && where.regoff() == 0)
						loaded[reg] = std::string(".") + registers[reg] + " = (uint32_t)(" + values[k] + ")";
					else if (where.is_stkoff())
						stack += ", " + values[k];
					else
						spelled = false;
				}
				if (spelled)
				{
					std::string regs;
					for (const auto& [reg, init] : loaded)
						regs += (regs.empty() ? "" : ", ") + init;
					return mark(e, "fnvd::original::usercall<" + raw(e->type) + ">(" + hexText(callee->obj_ea) +
									   ", fnvd::original::UserRegs{" + regs + "}" + stack + ")");
				}
			}
			return mark(e, "fnvd::original::" + function + "<" + raw(e->type) + ">(" + hexText(callee->obj_ea) +
							   (list.empty() ? "" : ", ") + list + ")");
		}
		tinfo_t pointer = callee->type.is_ptr() ? callee->type : tinfo_t();
		if (pointer.empty())
			pointer.create_ptr(callee->type);
		return mark(e, "((" + raw(pointer) + ")" + operand(callee, cot_cast) + ")(" + list + ")");
	}

	// an operand of Hex-Rays' integer + or - whose src type is a pointer to void (a field IDA types as an integer):
	// as a pointer to char, the exe's byte arithmetic (C++ has no arithmetic on void*)
	std::string byteOperand(const cexpr_t* e, ctype_t parent, bool right)
	{
		if (!e->type.is_ptr())
			if (std::string key = srcType(m_function, m_context, e); !key.empty())
			{
				std::string canonical = m_types.canonical(key);
				if (canonical == "ptr(builtin:void)")
					return "static_cast<char*>(" + expr(e) + ")";
				if (canonical == "ptr(const(builtin:void))")
					return "static_cast<const char*>(" + expr(e) + ")";
			}
		return operand(e, parent, right);
	}

	// the operand x of a write (an assignment, ++, --): a Hex-Rays helper's call as the bytes it names, in place
	// (`gen::lodword_ref(v) = 1`, `++gen::lodword_ref(v)`)
	std::string writtenOperand(const cexpr_t* x, ctype_t op)
	{
		if (x->op == cot_call && x->x->op == cot_helper)
			return call(x, true);
		return operand(x, op);
	}

	// a conversion recorded on e (Emit.hpp's Conversion), nullptr for none
	const Conversion* conversion(const cexpr_t* e) const
	{
		auto found = m_function.conversions.find(e);
		return found == m_function.conversions.end() ? nullptr : &found->second;
	}

	// e through its conversion: e itself, or the operand of the Hex-Rays cast it replaces; an enum's step as the
	// assignment of the next value; marked unless it is the static_cast src writes for a void*
	std::string convert(const cexpr_t* e, const Conversion& c)
	{
		std::string type = declare(c.type, c.ida);
		std::string out;
		if (c.kind == Conversion::Kind::enumStep)
		{
			bool up = e->op == cot_preinc || e->op == cot_postinc;
			out = operand(e->x, cot_asg) + " = static_cast<" + type + ">(" + operand(e->x, cot_add) +
				  (up ? " + 1)" : " - 1)");
		}
		else if (c.kind == Conversion::Kind::enumPostStep)
		{
			bool up = e->op == cot_postinc;
			m_includes.insert("<utility>");
			out = "std::exchange(" + operand(e->x, cot_comma, true) + ", static_cast<" + type + ">(" +
				  operand(e->x, cot_add) + (up ? " + 1))" : " - 1))");
		}
		else if (c.kind == Conversion::Kind::byteStep || c.kind == Conversion::Kind::bytePostStep)
		{
			bool up = e->op == cot_preinc || e->op == cot_postinc || e->op == cot_asgadd;
			std::string by =
				e->op == cot_asgadd || e->op == cot_asgsub ? operand(e->y, up ? cot_add : cot_sub, true) : "1";
			std::string next = "static_cast<" + type + ">(" + expr(e->x) + ")" + (up ? " + " : " - ") + by;
			if (c.kind == Conversion::Kind::byteStep)
				out = operand(e->x, cot_asg) + " = " + next;
			else
			{
				m_includes.insert("<utility>");
				out = "std::exchange(" + operand(e->x, cot_comma, true) + ", " + next + ")";
			}
		}
		else if (c.kind == Conversion::Kind::valueInit)
			out = type + "{}";
		else if (c.kind == Conversion::Kind::storeBits)
			out = "gen::store(" + writtenOperand(e->x, cot_comma) + ", " +
				  expr(c.replacesCast ? e->y->x : e->y) + ")";
		else if (c.kind == Conversion::Kind::memberAddress)
			out = "&" + memberOf(e, c);
		else if (c.kind == Conversion::Kind::implicit)
			out = c.replacesCast ? expr(e->x) : plain(e);
		else
		{
			std::string value = c.replacesCast ? expr(e->x) : plain(e);
			if (!c.unconst.empty())
				value = "const_cast<" + declare(c.unconst, tinfo_t()) + ">(" + value + ")";
			switch (c.kind)
			{
			case Conversion::Kind::constCast:
				out = "const_cast<" + type + ">(" + value + ")";
				break;
			case Conversion::Kind::pointerToInteger:
				out = "static_cast<" + type + ">(reinterpret_cast<uintptr_t>(" + value + "))";
				break;
			case Conversion::Kind::integerToPointer:
				out = "reinterpret_cast<" + type + ">(static_cast<uintptr_t>(" + value + "))";
				break;
			case Conversion::Kind::reinterpret:
				out = "reinterpret_cast<" + type + ">(" + value + ")";
				break;
			case Conversion::Kind::bits:
				out = "gen::bitcast<" + type + ">(" + value + ")";
				break;
			case Conversion::Kind::sameBits:
				m_includes.insert("<bit>");
				out = "std::bit_cast<" + type + ">(" + value + ")";
				break;
			case Conversion::Kind::lowLane:
				m_includes.insert(c.headers.begin(), c.headers.end());
				out = c.form;
				if (size_t at = out.find("$0"); at != std::string::npos)
					out.replace(at, 2, value);
				break;
			default:
				out = "static_cast<" + type + ">(" + value + ")";
				break;
			}
			if (!c.addConst.empty())
				out = "const_cast<" + declare(c.addConst, tinfo_t()) + ">(" + out + ")";
		}
		return c.what.empty() ? out : nested(e, out) + marker(e, c.what);
	}

	// the member at offset 0 a memberAddress conversion names: `v->path`, v the record's pointer
	std::string memberOf(const cexpr_t* e, const Conversion& c)
	{
		std::string pointer;
		if (c.replacesCast)
			pointer = operand(e->x, cot_memptr);
		else
		{
			pointer = plain(e);
			if (ownPrecedence(e) < kPostfix)
				pointer = "(" + pointer + ")";
		}
		return pointer + "->" + c.form;
	}

	std::string expr(const cexpr_t* e)
	{
		if (const Conversion* c = conversion(e))
			return convert(e, *c);
		return plain(e);
	}

	// e as its own meaning prints it
	std::string plain(const cexpr_t* e)
	{
		// an idiom's form; one item alone (`$0`) as that item prints anywhere, without the parentheses an operand
		// in a longer form takes; a temporary's address (`&*$0` where the pointer read is passed on as a pointer, a
		// raw call's argument), which C++ takes of no temporary, through gen::lvalue
		if (const Rewrite* rewrite = as<Rewrite>(e))
		{
			if (const cexpr_t* object = temporaryRead(*rewrite))
				return "&gen::lvalue(" + call(object) + ")";
			return loneItem(*rewrite) ? expr(loneItem(*rewrite)) : substituted(*rewrite, rewrite->form);
		}
		if (const char* op = binaryOperator(e->op))
		{
			if (written(e) && e->x->op == cot_call && e->x->x->op == cot_helper)
				return writtenOperand(e->x, e->op) + " " + op + " " + operand(e->y, e->op, true);
			if (e->op == cot_asg)
				return mark(e, operand(e->x, cot_asg) + " = " + converted(e, e->y, e->x->type));
			// a NaN takes the exe's branch: the complement, negated
			if (nanOutcomeDiffers(e))
			{
				ctype_t complement = complementOf(e->op);
				return mark(e, "!(" + operand(e->x, complement) + " " + binaryOperator(complement) + " " +
								   operand(e->y, complement, true) + ")");
			}
			if (e->op == cot_add || e->op == cot_sub)
				return mark(e, byteOperand(e->x, e->op, false) + " " + op + " " + byteOperand(e->y, e->op, true));
			// pointers compared: their classes complete (one converts to the other's base)
			if ((e->op == cot_eq || e->op == cot_ne) && e->x->type.is_ptr() && e->y->type.is_ptr())
			{
				pointeeHeaders(e->x);
				pointeeHeaders(e->y);
			}
			return mark(e, operand(e->x, e->op) + " " + op + " " + operand(e->y, e->op, true));
		}
		switch (e->op)
		{
		case cot_empty:
			return "";
		case cot_comma:
			return expr(e->x) + ", " + operand(e->y, cot_comma, true);
		case cot_tern:
			return operand(e->x, cot_tern) + " ? " + operand(e->y, cot_tern, true) + " : " +
				   operand(e->z, cot_tern, true);
		case cot_lnot:
			return "!" + operand(e->x, e->op);
		case cot_bnot:
			return "~" + operand(e->x, e->op);
		case cot_neg:
		case cot_fneg:
			return "-" + operand(e->x, e->op);
		case cot_postinc:
			return mark(e, writtenOperand(e->x, e->op) + "++");
		case cot_postdec:
			return mark(e, writtenOperand(e->x, e->op) + "--");
		case cot_preinc:
			return mark(e, "++" + writtenOperand(e->x, e->op));
		case cot_predec:
			return mark(e, "--" + writtenOperand(e->x, e->op));
		case cot_ptr:
			// the read of a `return this` fold's receiver's first field (the FoldedAccessors pass)
			if (const Member* first = as<Member>(e))
			{
				const cexpr_t* call = e->x;
				while (call->op == cot_cast)
					call = call->x;
				m_types.headers("record:" + first->owner, m_includes);
				includeObjectClass(receiverOf(call)->type.get_pointed_object());
				return memberOf(receiverOf(call), first->field->name) + first->field->name;
			}
			if (as<Missing>(e))
				return mark(e, at(e->type, expr(e->x), 0));
			return deref(e->x);
		case cot_ref:
			if (as<Implicit>(e))
				return implicitOperand(e);
			if (functionStart(e->x))
				return mark(e->x, "reinterpret_cast<" + raw(e->type) + ">(" + hexUpper(e->x->obj_ea) + "u)");
			if (referenceVar(e->x))
				return "&" + var(lvarIndex(e->x));
			// unresolved: the address as IDA's pointer type
			if (as<Missing>(e))
				return mark(e, "(" + raw(e->type) + ")&" + operand(e->x, e->op));
			// the address of a base subobject
			if (const std::string* key = as<std::string>(e->x); key && e->x->op == cot_memptr)
				return "static_cast<" + declare(*key, e->x->type) + "*>(" + expr(e->x->x) + ")";
			return "&" + operand(e->x, e->op);
		case cot_cast:
		{
			if (as<Implicit>(e))
				return implicitOperand(e);
			if (const Enumerators* names = as<Enumerators>(e))
				return enumerators(*names, e);
			if (const ConstCast* drop = as<ConstCast>(e))
				return "const_cast<" + declare(drop->type, e->type) + ">(" + expr(e->x) + ")";
			// the exe's bytes as another type; a pointer's as an integer, or an integer's as a pointer, through
			// uintptr_t (C++ casts no pointer to a narrower integer: the low bytes, on any layout)
			if (const ExeCast* bytes = as<ExeCast>(e))
			{
				std::string type = declare(bytes->type, e->type);
				if (e->x->type.is_ptr() && !e->type.is_ptr())
					return "static_cast<" + type + ">(reinterpret_cast<uintptr_t>(" + expr(e->x) + "))";
				if (!e->x->type.is_ptr() && e->type.is_ptr())
					return "reinterpret_cast<" + type + ">(static_cast<uintptr_t>(" + expr(e->x) + "))";
				return "reinterpret_cast<" + type + ">(" + expr(e->x) + ")";
			}
			if (const std::string* key = as<std::string>(e))
				return "static_cast<" + declare(*key, e->type) + ">(" + expr(e->x) + ")";
			// C casts a pointer to an enum or integer directly; C++ only through an integer (not where the operand
			// prints as src's integer, which IDA has as a pointer)
			std::string printed = srcType(m_function, m_context, e->x);
			std::string kind = printed.empty() ? "" : m_types.parts(m_types.canonical(printed)).kind;
			if (e->x->type.is_ptr() && !e->type.is_ptr() &&
				(kind.empty() || kind == "ptr" || kind == "array" || kind == "fn"))
				return mark(e, "(" + raw(e->type) + ")reinterpret_cast<uintptr_t>(" + expr(e->x) + ")");
			// Hex-Rays' cast between pointers keeps the bits; C++'s C cast between pointers to records is a
			// static_cast where one derives from the other, which moves the pointer by the base's offset
			// (`(PlayerCharacter*)pOwner` of an ActorValueOwner base at 0xA4): through uintptr_t, which keeps the
			// bits whatever const the operand's C++ type has (reinterpret_cast cannot drop it)
			bool records = e->x->type.is_ptr() && e->type.is_ptr() && e->type.get_pointed_object().is_udt();
			if (records && kind.empty())
				records = e->x->type.get_pointed_object().is_udt();
			else if (records)
			{
				SrcTypes::Parts pointer = m_types.parts(m_types.canonical(printed));
				records = kind == "ptr" && m_types.parts(m_types.canonical(pointer.of)).kind == "record";
			}
			if (records)
				return mark(e, "(" + raw(e->type) + ")reinterpret_cast<uintptr_t>(" + expr(e->x) + ")");
			// a float's bits read as a pointer (an untyped slot Hex-Rays typed float): C++ casts no float to a pointer,
			// so through the dword the bits are
			if (e->type.is_ptr() && e->x->type.is_floating() && e->x->type.get_size() == 4)
				return mark(e, "(" + raw(e->type) + ")gen::bitcast<uint32_t>(" + expr(e->x) + ")");
			return mark(e, "(" + raw(e->type) + ")" + operand(e->x, e->op));
		}
		case cot_idx:
		{
			// an index that prints as FNVD_NOINLINE, a lambda whose `[` would open `[[` (an attribute), parenthesised
			std::string index = expr(e->y);
			if (index.starts_with("FNVD_NOINLINE("))
				index = "(" + index + ")";
			return operand(e->x, e->op) + "[" + index + "]";
		}
		case cot_memptr:
		case cot_memref:
			return field(e);
		case cot_call:
			if (const Access* access = as<Access>(e))
				return accessed(e, *access);
			// a record returned by value: `&(v = F())`, or the new-expression building it in v (its value &v); C++'s
			// temporary (foldHiddenResults) passed on as a pointer, whose address C++ takes only through gen::lvalue
			if (byValueCall(e))
				return e->type.is_void() || placement(e) ? call(e)
					   : temporary(e)					? "&gen::lvalue(" + call(e) + ")"
														: "&(" + call(e) + ")";
			return referenceCall(e) ? "&" + call(e) : call(e);
		case cot_num:
			return number(e);
		case cot_fnum:
		{
			// a float literal where Floats typed the constant a float (the exe's operand, else its place), of the
			// exe's value where Hex-Rays rounded the exe's double to a float
			double value = 0;
			if (auto exe = m_function.floatValues.find(e); exe != m_function.floatValues.end())
				value = exe->second;
			else
				e->fpc->fnum.to_double(&value);
			const std::string* key = as<std::string>(e);
			bool single = key && *key == "builtin:float";
			// a NaN or an infinity has no literal: its bits
			if (!std::isfinite(value))
			{
				char bits[64];
				if (single)
					std::snprintf(bits, sizeof bits, "std::bit_cast<float>(0x%08Xu)",
								  std::bit_cast<uint32_t>(static_cast<float>(value)));
				else
					std::snprintf(bits, sizeof bits, "std::bit_cast<double>(0x%016llXull)",
								  static_cast<unsigned long long>(std::bit_cast<uint64_t>(value)));
				m_includes.insert("<bit>");
				return mark(e, bits);
			}
			return mark(e, floatLiteral(value, single));
		}
		case cot_obj:
			if (functionStart(e))
				return mark(e, "reinterpret_cast<uint8_t*>(" + hexUpper(e->obj_ea) + "u)");
			return global(e);
		case cot_var:
			if (referenceVar(e))
				return "&" + var(e->v.idx);
			{
				// a use of raw storage as the object it holds, or as its address (a pointer key: Hex-Rays' array
				// decaying to it); an array of an abstract class, which C++ cannot form, as the pointer to its first
				// element. A use a pass left unresolved (a by-value argument the exe builds in place:
				// OutgoingArguments) is still the object, marked.
				const Local& local = m_function.locals[e->v.idx];
				const std::string* key = as<std::string>(e);
				if (!key && local.storage && as<Missing>(e) && !local.holds.empty())
					key = &local.holds;
				if (key && local.storage)
				{
					SrcTypes::Parts parts = m_types.parts(*key);
					const SrcModel::Record* element = nullptr;
					if (parts.kind == "array")
						element = m_context.model.record(m_types.parts(m_types.canonical(parts.of)).name);
					bool abstract = element && element->abstract;
					std::string spelledKey = abstract ? "ptr(" + parts.of + ")" : *key;
					std::string declarator = parts.kind == "ptr" || abstract ? "" : "&";
					if (std::optional<std::string> type = m_types.spell(spelledKey, declarator))
					{
						m_types.headers(*key, m_includes);
						return mark(e, "reinterpret_cast<" + *type + ">(" + (abstract ? "&" : "") + var(e->v.idx) +
										   ")");
					}
				}
			}
			// a use of FNVD_CONSTRUCTED_LOCAL's local Hex-Rays types as bytes not as the object (no key): the bytes IDA
			// has there (an array decaying to their address)
			if (const Local& local = m_function.locals[e->v.idx]; local.inlined && local.inlined->construct &&
																   !as<std::string>(e) && !e->type.is_udt())
			{
				const tinfo_t& type = (*m_function.cfunc->get_lvars())[e->v.idx].type();
				return type.is_array()
						   ? "reinterpret_cast<" + raw(type.get_array_element()) + "*>(&" + var(e->v.idx) + ")"
						   : "reinterpret_cast<" + raw(type) + "&>(" + var(e->v.idx) + ")";
			}
			return var(e->v.idx);
		case cot_helper:
			return mark(e, e->helper);
		case cot_str:
		{
			// Hex-Rays holds the string in its user representation (`DATA\\`): its bytes back, then C++'s escapes
			qstring bytes;
			user2qstr(&bytes, qstring(e->string));
			return mark(e, cLiteral(std::string(bytes.c_str(), bytes.length())));
		}
		case cot_sizeof:
			return mark(e, "sizeof(" + expr(e->x) + ")");
		case cot_type:
			return mark(e, raw(e->type));
		default:
			return mark(e, "0");
		}
	}

	// --- the function's head and registration

	std::string srcName() const { return inTu(m_function.self->name); }

	bool member() const { return !m_function.self->cls.empty() && !m_function.self->isStatic; }

	// src's convention where it is not the default of its kind (thiscall for a member, cdecl otherwise): the macro
	// the head and a registration's pointer type spell
	std::string conventionMacro() const
	{
		const SrcModel::Abi* abi = m_context.model.abi(m_function.self->mangled);
		std::string cc = abi ? abi->cc : member() ? "thiscall" : "cdecl";
		if (cc == (member() ? "thiscall" : "cdecl"))
			return "";
		return cc == "thiscall"	  ? "FNVD_THISCALL"
			   : cc == "stdcall"  ? "FNVD_STDCALL"
			   : cc == "fastcall" ? "FNVD_FASTCALL"
								  : "FNVD_CDECL";
	}

	// the parameters: Hex-Rays' arguments past this (a member's) and the hidden result pointer (a record returned by
	// value: the local `R result;`); Construct's first is src's `void* apMemory`, its object the local pThis; src's
	// apEdx, which Hex-Rays has no argument for, as src declares it
	std::string srcParameters()
	{
		std::vector<std::string> out;
		const intvec_t& argidx = m_function.cfunc->argidx;
		const lvars_t& lvars = *m_function.cfunc->get_lvars();
		const SrcModel::Function* self = m_function.self;
		const SrcModel::Type* type = m_context.model.type(self->type);
		size_t k = member() ? 1 : 0;
		if (m_function.structor == "Construct")
		{
			std::string name = !self->params.empty() && !self->params[0].empty() ? self->params[0] : "apMemory";
			out.push_back(m_types.spell(type->params[0], name).value_or("void* " + name));
			++k;
		}
		for (; k < argidx.size(); ++k)
		{
			if (argidx[k] == m_function.resultArgument)
				continue;
			const Local& local = m_function.locals[argidx[k]];
			// a rebound parameter: its src declaration in the head, the lvar its copy (the prologue's)
			if (local.entryCopy)
				out.push_back(
					m_types.spell(local.entryCopy->type, local.entryCopy->name).value_or("? " + local.entryCopy->name));
			else
				out.push_back(declare(local.type, lvars[argidx[k]].type(), local.name));
		}
		if (int edx = m_function.edxParameter; edx >= 0 && static_cast<size_t>(edx) <= out.size())
		{
			std::string name = self->params[edx];
			out.insert(out.begin() + edx,
					   "[[maybe_unused]] " + m_types.spell(type->params[edx], name).value_or("void* " + name));
		}
		std::string list;
		for (const std::string& p : out)
			list += (list.empty() ? "" : ", ") + p;
		return list;
	}

	// what the body declares before its statements: Construct's object (`X* pThis = static_cast<X*>(apMemory);`), a
	// record returned by value (`R result;`), the copies of rebound parameters (`X* pThis = this;`, `T* pX = &arX;`),
	// the debug build's downcast locals (`const D& kX = static_cast<const D&>(arB);`)
	std::string prologue()
	{
		std::string out;
		const SrcModel::Function* self = m_function.self;
		const intvec_t& argidx = m_function.cfunc->argidx;
		if (m_function.structor == "Construct")
		{
			std::string object = "ptr(record:" + self->cls + ")";
			std::string memory = !self->params.empty() && !self->params[0].empty() ? self->params[0] : "apMemory";
			std::string type = m_types.spell(object).value_or(self->cls + "*");
			out += "\t" + type + " " + var(argidx[0]) + " = static_cast<" + type + ">(" + memory + ");\n";
		}
		for (int index : argidx)
			if (const Local& local = m_function.locals[index]; local.entryCopy)
			{
				std::string from = local.entryCopy->name == "this" ? "this"
								   : local.entryCopy->type.empty() ? local.entryCopy->name
																   : "&" + local.entryCopy->name;
				m_types.headers(local.type, m_includes);
				out += "\t" + m_types.spell(local.type, var(index)).value_or("? " + var(index)) + " = " + from + ";\n";
			}
		for (const Function::Downcast& downcast : m_function.downcasts)
		{
			bool toReference = m_types.parts(m_types.bare(downcast.type)).kind == "lref";
			bool fromReference = m_function.locals[downcast.parameter].reference;
			std::string from = (fromReference == toReference ? "" : toReference ? "*" : "&") + var(downcast.parameter);
			m_types.headers(downcast.type, m_includes);
			out += "\t" + m_types.spell(downcast.type, downcast.name).value_or("? " + downcast.name) + " = static_cast<" +
				   m_types.spell(downcast.type).value_or("?") + ">(" + from + ");\n";
		}
		// the hidden result's local: the record its reference type (Self's) refers to
		if (m_function.resultArgument >= 0)
		{
			std::string key = m_types.parts(m_function.locals[m_function.resultArgument].type).of;
			m_types.headers(key, m_includes);
			out += "\t" + m_types.spell(key, var(m_function.resultArgument)).value_or("?") + ";\n";
		}
		return out;
	}

	std::string srcHead()
	{
		const SrcModel::Type* type = m_context.model.type(m_function.self->type);
		const SrcModel::Record* record = m_context.model.record(m_function.self->cls);
		if (record && record->file.rfind("src/", 0) == 0)
			m_includes.insert(record->file.substr(4));
		// a free function's declaration
		if (const std::string& file = m_function.self->file;
			m_function.self->cls.empty() && file.rfind("src/", 0) == 0 && isHeader(file))
			m_includes.insert(file.substr(4));
		std::string cc = conventionMacro();
		std::string declarator = (cc.empty() ? "" : cc + " ") + srcName() + "(" + srcParameters() + ")" +
								 (type && type->isConst ? " const" : "");
		std::string prefix;
		for (int k = 0; k < m_function.templateHeads; ++k)
			prefix += "template <>\n";
		// a conversion function (`operator float`) declares no return type
		const std::string& qualified = m_function.self->name;
		size_t colons = qualified.rfind("::");
		std::string name = colons == std::string::npos ? qualified : qualified.substr(colons + 2);
		if (name.rfind("operator ", 0) == 0 && std::isalpha(static_cast<unsigned char>(name[9])) &&
			name.rfind("operator new", 0) != 0 && name.rfind("operator delete", 0) != 0)
			return prefix + declarator;
		tinfo_t ftype;
		m_function.cfunc->get_func_type(&ftype);
		return prefix + declare(m_function.returnType, ftype.get_rettype(), declarator);
	}

	// several src functions answer to the name (a registration names one by its pointer type)
	bool overloaded() const { return m_function.overloaded; }

	// A class name as a registration macro's argument: a name with a comma (a template instance's) through a `using`
	// alias (porting rules), its line added to aliases. The alias spelling is injective, so that two classes never
	// get one name (`BSSimpleArray<NavMesh *, 1024>` is `BSSimpleArray_lNavMesh_p_c1024`, `BSSimpleArray<NavMeshPtr,
	// 1024>` `BSSimpleArray_lNavMeshPtr_c1024`): alphanumerics stay, every other token is `_` and a letter (`_u` an
	// underscore, `_p` `*`, `_r` `&`, `_c` `,`, `_l` `<`, `_g` `>`, `_n` `::`, `_s` a space between two words: clang's
	// other spaces and the closing `>`s at the end follow from the rest), else `_x` and its two hex digits.
	std::string macroClass(const std::string& cls, std::string& aliases) const
	{
		if (cls.find(',') == std::string::npos)
			return cls;
		static constexpr std::string_view kEscapes = "_u*p&r,c<l>g";
		auto word = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; };
		std::string_view name = cls;
		while (name.ends_with('>'))
			name.remove_suffix(1);
		std::string alias;
		for (size_t k = 0; k < name.size(); ++k)
			if (char c = name[k]; std::isalnum(static_cast<unsigned char>(c)))
				alias += c;
			else if (size_t at = kEscapes.find(c); at != std::string_view::npos)
				alias += std::string{'_', kEscapes[at + 1]};
			else if (name.substr(k, 2) == "::")
				alias += "_n", ++k;
			else if (c == ' ' && k > 0 && k + 1 < name.size() && word(name[k - 1]) && word(name[k + 1]))
				alias += "_s";
			else if (c != ' ')
			{
				char hex[8];
				std::snprintf(hex, sizeof hex, "_x%02X", static_cast<unsigned char>(c));
				alias += hex;
			}
		aliases += "using " + alias + " = " + cls + ";\n";
		return alias;
	}

	// the definition's: FNVD_REPLACES (a member, static member or free function; FNVD_REPLACES_MEMBER for a protected
	// member), FNVD_REPLACES_VIRTUAL (_VIA where the exe enters it through a secondary vtable),
	// FNVD_REPLACES_CONSTRUCT, _DESTRUCT, _DELETING_DESTRUCT; _OVERLOAD forms for an overload, naming it by its pointer
	// type (FNVD_REPLACES_MEMBER takes it after the member); FNVD_REPLACES_USERCALL / _USERPURGE with Self's locations
	std::string registration()
	{
		const SrcModel::Function* self = m_function.self;
		const SrcModel::Type* type = m_context.model.type(self->type);
		std::string address = hexUpper(m_function.ea);
		std::string aliases;
		std::string cls = self->cls.empty() ? "" : macroClass(inTu(self->cls), aliases);
		std::string last = lastComponent(self->name);
		std::string name = cls.empty() ? inTu(self->name) : cls + "::" + last;
		// the overload's pointer type, its result spelled around the declarator (`char (&(C::*)(int32_t))[512]`)
		auto pointerType = [&] {
			std::string params;
			for (const std::string& p : type->params)
				params += (params.empty() ? "" : ", ") + m_types.spell(p).value_or("?");
			std::string cc = conventionMacro();
			std::string pointer = member() ? "(" + (cc.empty() ? "" : cc + " ") + inTu(self->cls) + "::*)"
										   : "(" + (cc.empty() ? "" : cc) + "*)";
			return m_types.spell(type->ret, pointer + "(" + params + ")" + (type->isConst ? " const" : ""))
				.value_or("?");
		};
		std::string text;
		// a name with a comma (a function template's arguments) in parentheses, as FNVD_REPLACES's below
		if (m_function.userLocations)
			text = std::string(m_function.userPurge ? "FNVD_REPLACES_USERPURGE(" : "FNVD_REPLACES_USERCALL(") +
				   address + ", " +
				   (name.find(',') != std::string::npos && (cls.empty() || self->isStatic) ? "(" + name + ")" : name) +
				   (m_function.userLocations->empty() ? "" : ", " + *m_function.userLocations) + ")";
		else if (m_function.structor == "Construct")
			text = overloaded()
					   ? "FNVD_REPLACES_CONSTRUCT_OVERLOAD(" + address + ", " + name + ", " + pointerType() + ")"
					   : "FNVD_REPLACES_CONSTRUCT(" + address + ", " + name + ")";
		else if (m_function.structor == "Destruct")
			text = "FNVD_REPLACES_DESTRUCT(" + address + ", " + name + ")";
		else if (m_function.structor == "DeletingDestruct")
			text = "FNVD_REPLACES_DELETING_DESTRUCT(" + address + ", " + name + ")";
		else if (self->isVirtual && !m_function.virtualBase.empty())
		{
			std::string base = macroClass(inTu(m_function.virtualBase), aliases);
			text = overloaded() ? "FNVD_REPLACES_VIRTUAL_VIA_OVERLOAD(" + address + ", " + cls + ", " + base + ", " +
									  last + ", " + pointerType() + ")"
								: "FNVD_REPLACES_VIRTUAL_VIA(" + address + ", " + cls + ", " + base + ", " + last + ")";
		}
		else if (self->isVirtual)
			text = overloaded() ? "FNVD_REPLACES_VIRTUAL_OVERLOAD(" + address + ", " + cls + ", " + last + ", " +
									  pointerType() + ")"
								: "FNVD_REPLACES_VIRTUAL(" + address + ", " + cls + ", " + last + ")";
		else if (m_function.protectedMember)
			text = "FNVD_REPLACES_MEMBER(" + address + ", " + cls + ", " + last +
				   (overloaded() ? ", " + pointerType() : "") + ")";
		else
		{
			// a name with a comma (a function template's arguments) is one macro argument in parentheses, which
			// `&(f<A, B>)` takes for a free or static function
			std::string function =
				name.find(',') != std::string::npos && (cls.empty() || self->isStatic) ? "(" + name + ")" : name;
			text = overloaded() ? "FNVD_REPLACES_OVERLOAD(" + address + ", " + function + ", " + pointerType() + ")"
								: "FNVD_REPLACES(" + address + ", " + function + ")";
		}
		return aliases + text + ";\n";
	}

	const Function& m_function;
	Context& m_context;
	SrcTypes& m_types;
	bool m_src;
	std::set<std::string> m_includes;
	std::map<ea_t, std::string> m_globals;
	std::set<const cexpr_t*> m_initialising;  // by-value calls printed as the initialiser of the local they fill
	std::unordered_map<const cinsn_t*, std::vector<int>> m_declared;  // the locals each statement or block declares
	std::set<const cinsn_t*> m_copies;								   // the aliases' copy statements (not printed)
	std::set<const cinsn_t*> m_dropped;								   // the statements idioms drop (not printed)
	std::string m_out;
	std::vector<Marker> m_markers;  // every marker printed, by its sentinel's index
};

}  // namespace

std::string print(const Function& function, Context& context, Emitted::Form form, std::vector<Emitted::Spot>& spots)
{
	return Printer(function, context, form).file(spots);
}

}  // namespace idakit
