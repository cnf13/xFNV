// Intrinsics: a Hex-Rays helper call that is a compiler intrinsic of both builds (the hybrid's i386 and the Linux x64
// standalone build: clang's <xmmintrin.h>, <emmintrin.h>, <x86intrin.h> and its __builtin_bswap*) as that
// intrinsic, where every argument is of the intrinsic's own parameter type as IDA spells it, or a number where the
// intrinsic takes an immediate, and the call's result is of the intrinsic's result type: the call then means exactly
// the instruction Hex-Rays read. A helper call whose types differ stays unresolved (checkRules' marker), as does
// any helper this table does not hold (LOBYTE and the other part helpers, qmemcpy, ...). Hex-Rays' cast of a scalar
// to an SSE vector that means an intrinsic's value prints as it: `_mm_setzero_ps()`, `_mm_set_ss(f)`.

#include "EmitPasses.hpp"

#include "IdaSupport.hpp"

#include <array>
#include <cmath>
#include <cstring>
#include <string_view>

namespace idakit
{

namespace
{

// one intrinsic: Hex-Rays' helper name, src's name for it, its header ("" for a builtin), its result's and its
// parameters' IDA types (`imm` an immediate: a number)
struct Form
{
	std::string_view helper, name, header, result;
	std::array<std::string_view, 3> params;
	size_t count;
};

constexpr std::string_view kSse = "<xmmintrin.h>", kSse2 = "<emmintrin.h>";

// __m128 op(__m128, __m128)
#define FNVD_PS2(op) {#op, #op, kSse, "__m128", {"__m128", "__m128"}, 2}
// __m128 op(__m128)
#define FNVD_PS1(op) {#op, #op, kSse, "__m128", {"__m128"}, 1}
// __m128i op(__m128i, __m128i)
#define FNVD_EPI2(op) {#op, #op, kSse2, "__m128i", {"__m128i", "__m128i"}, 2}

constexpr Form kForms[] = {
	FNVD_PS2(_mm_add_ps),
	FNVD_PS2(_mm_sub_ps),
	FNVD_PS2(_mm_mul_ps),
	FNVD_PS2(_mm_div_ps),
	FNVD_PS2(_mm_min_ps),
	FNVD_PS2(_mm_max_ps),
	FNVD_PS2(_mm_and_ps),
	FNVD_PS2(_mm_andnot_ps),
	FNVD_PS2(_mm_or_ps),
	FNVD_PS2(_mm_xor_ps),
	FNVD_PS2(_mm_unpacklo_ps),
	FNVD_PS2(_mm_unpackhi_ps),
	FNVD_PS2(_mm_movelh_ps),
	FNVD_PS2(_mm_movehl_ps),
	FNVD_PS2(_mm_cmpeq_ps),
	FNVD_PS2(_mm_cmplt_ps),
	FNVD_PS2(_mm_cmple_ps),
	FNVD_PS2(_mm_cmpgt_ps),
	FNVD_PS2(_mm_cmpge_ps),
	FNVD_PS2(_mm_cmpneq_ps),
	FNVD_PS2(_mm_add_ss),
	FNVD_PS2(_mm_sub_ss),
	FNVD_PS2(_mm_mul_ss),
	FNVD_PS2(_mm_div_ss),
	FNVD_PS2(_mm_min_ss),
	FNVD_PS2(_mm_max_ss),
	FNVD_PS2(_mm_cmpeq_ss),
	FNVD_PS2(_mm_cmplt_ss),
	FNVD_PS2(_mm_cmple_ss),
	FNVD_PS2(_mm_move_ss),
	FNVD_PS1(_mm_rcp_ps),
	FNVD_PS1(_mm_rsqrt_ps),
	FNVD_PS1(_mm_sqrt_ps),
	FNVD_PS1(_mm_rcp_ss),
	FNVD_PS1(_mm_rsqrt_ss),
	FNVD_PS1(_mm_sqrt_ss),
	{"_mm_shuffle_ps", "_mm_shuffle_ps", kSse, "__m128", {"__m128", "__m128", "imm"}, 3},
	{"_mm_movemask_ps", "_mm_movemask_ps", kSse, "int", {"__m128"}, 1},
	// the hint an immediate, as clang's macro takes it
	{"_mm_prefetch", "_mm_prefetch", kSse, "void", {"const char *", "imm"}, 2},
	{"_mm_cvtepi32_ps", "_mm_cvtepi32_ps", kSse2, "__m128", {"__m128i"}, 1},
	{"_mm_load_si128", "_mm_load_si128", kSse2, "__m128i", {"const __m128i *"}, 1},
	FNVD_EPI2(_mm_unpacklo_epi16),
	FNVD_EPI2(_mm_unpackhi_epi16),
	{"_mm_slli_epi32", "_mm_slli_epi32", kSse2, "__m128i", {"__m128i", "imm"}, 2},
	{"_mm_srli_epi32", "_mm_srli_epi32", kSse2, "__m128i", {"__m128i", "imm"}, 2},
	{"_mm_cvtsi32_si128", "_mm_cvtsi32_si128", kSse2, "__m128i", {"int"}, 1},
	{"_mm_loadu_si128", "_mm_loadu_si128", kSse2, "__m128i", {"const __m128i *"}, 1},
	{"__rdtsc", "__rdtsc", "<x86intrin.h>", "unsigned __int64", {}, 0},
	// int 3: clang's builtin of both builds (MSVC's __debugbreak is clang-cl's alone)
	{"__debugbreak", "__builtin_debugtrap", "", "void", {}, 0},
	{"_byteswap_ushort", "__builtin_bswap16", "", "bits2", {"bits2"}, 1},
	{"_byteswap_ulong", "__builtin_bswap32", "", "bits4", {"bits4"}, 1},
	{"_byteswap_uint64", "__builtin_bswap64", "", "bits8", {"bits8"}, 1},
};

#undef FNVD_PS2
#undef FNVD_PS1
#undef FNVD_EPI2

// a type is the form's: its IDA spelling; `bitsN` an integer of N bytes that is not signed (unsigned, or IDA's `_DWORD`
// of unknown sign), which the printer converts to the unsigned parameter as it is (a byte swap takes bits)
bool fits(std::string_view spec, const tinfo_t& type)
{
	if (spec.starts_with("bits"))
		return type.is_integral() && !type.is_bool() && !type.is_enum() && !type.is_signed() &&
			   type.get_size() == static_cast<size_t>(spec[4] - '0');
	return text(type) == spec;
}

// the instruction at ea loads a float from memory into an SSE register's low lane, zeroing the others (`movss xmm,
// dword [m]`); value: the float it loads, where the operand's address is known
bool loadsFloat(ea_t ea, std::optional<float>* value = nullptr)
{
	insn_t insn;
	if (ea == BADADDR || decode_insn(&insn, ea) <= 0 || insn.itype != NN_movss || insn.ops[0].type != o_reg)
		return false;
	const op_t& source = insn.ops[1];
	if ((source.type != o_mem && source.type != o_displ && source.type != o_phrase) || source.dtype != dt_float)
		return false;
	if (value && source.type == o_mem)
	{
		uint32_t bits = get_dword(source.addr);
		float f;
		std::memcpy(&f, &bits, sizeof f);
		*value = f;
	}
	return true;
}

// a cast whose printed type (its src key, else IDA's) is SSE's four-float vector
bool toVector(const Function& function, Context& context, const cexpr_t* cast)
{
	auto found = function.meaning.find(cast);
	std::string why;
	std::optional<std::string> key;
	if (found != function.meaning.end() && std::holds_alternative<std::string>(found->second))
		key = std::get<std::string>(found->second);
	else if (found == function.meaning.end() || std::holds_alternative<Missing>(found->second))
		key = context.types.keyOf(cast->type, why);
	const SrcModel::Type* type = key ? context.model.type(context.types.canonical(*key)) : nullptr;
	return type && type->kind == "vector" && type->count == 4 && type->of == "builtin:float";
}

// the vector is read in its low lane alone: an operand of `_mm_shuffle_ps(a, b, imm)` whose two lane selectors in
// imm (a's: bits 0-3, b's: bits 4-7) both pick lane 0
bool lowLaneOnly(const cexpr_t* value, const std::unordered_map<const cexpr_t*, const citem_t*>& parents)
{
	auto parent = parents.find(value);
	if (parent == parents.end() || parent->second->op != cot_call)
		return false;
	const auto* call = static_cast<const cexpr_t*>(parent->second);
	if (call->x->op != cot_helper || std::string_view(call->x->helper) != "_mm_shuffle_ps" || call->a->size() != 3 ||
		(*call->a)[2].op != cot_num)
		return false;
	uint64_t imm = (*call->a)[2].numval();
	if (&(*call->a)[0] == value)
		return (imm & 0x0F) == 0;
	return &(*call->a)[1] == value && (imm & 0xF0) == 0;
}

// Hex-Rays' cast of a scalar to the vector, where it means what an SSE intrinsic writes: a zero of any width is
// `_mm_setzero_ps()` (every bit zero); a 4-byte number the cast's instruction loads as a float from memory, or
// LODWORD of a float so loaded or read in its low lane alone, is `_mm_set_ss(f)` (movss: the float in the low lane,
// the others zero). The LODWORD call is that intrinsic's own call, printed as the cast.
std::optional<Rewrite> vectorConstant(Function& function, Context& context, const cexpr_t* cast,
									  const std::unordered_map<const cexpr_t*, const citem_t*>& parents)
{
	const cexpr_t* x = cast->x;
	auto made = [](std::string form, std::vector<const citem_t*> items) {
		return Rewrite{1, std::move(form), std::move(items), {}, "typedef:__m128", std::string(kSse)};
	};
	if (!toVector(function, context, cast))
		return std::nullopt;
	if (x->op == cot_num && x->numval() == 0)
		return made("_mm_setzero_ps()", {});
	std::optional<float> loaded;
	bool loads = loadsFloat(cast->ea, &loaded) || loadsFloat(x->ea, &loaded);
	if (x->op == cot_num && x->type.get_size() == 4 && loads && loaded)
	{
		uint32_t bits = static_cast<uint32_t>(x->numval());
		float f;
		std::memcpy(&f, &bits, sizeof f);
		if (std::memcmp(&f, &*loaded, sizeof f) != 0 || !std::isfinite(f))
			return std::nullopt;
		return made("_mm_set_ss(" + floatLiteral(f, true) + ")", {});
	}
	if (x->op == cot_call && x->x->op == cot_helper && std::string_view(x->x->helper) == "LODWORD" &&
		x->a->size() == 1 && (*x->a)[0].type.is_float() && (loads || lowLaneOnly(cast, parents)))
	{
		function.meaning[x] = Intrinsic{"_mm_set_ss", std::string(kSse)};
		return made("_mm_set_ss($0)", {&(*x->a)[0]});
	}
	return std::nullopt;
}

// Hex-Rays' `fsqrt(x)` at an SSE site, x a float (a double Hex-Rays computes rounded to its float parameter, the
// float the exe's register holds): `sqrtss` is `_mm_sqrt_ss` of x in the low lane, its float result (on the call);
// `rsqrtss`, the approximate reciprocal Hex-Rays reads as `1.0 / fsqrt(x)`, is `_mm_rsqrt_ss` (on the division, the
// call taken as that intrinsic's). An rsqrtss without that division stays marked.
void scalarRoot(Function& function, const cexpr_t* call,
				const std::unordered_map<const cexpr_t*, const citem_t*>& parents)
{
	insn_t insn;
	if (std::string_view(call->x->helper) != "fsqrt" || call->a->size() != 1 || !(*call->a)[0].type.is_floating() ||
		decode_insn(&insn, call->ea) <= 0)
		return;
	const cexpr_t* x = &(*call->a)[0];
	std::string value = x->type.is_float() ? "$0" : "static_cast<float>($0)";
	auto made = [&](const char* op) {
		return Rewrite{1, std::string("_mm_cvtss_f32(") + op + "(_mm_set_ss(" + value + ")))", {x}, {},
					   "builtin:float", std::string(kSse)};
	};
	if (insn.itype == NN_sqrtss)
	{
		function.meaning[call] = made("_mm_sqrt_ss");
		return;
	}
	auto parent = parents.find(call);
	const cexpr_t* division = parent != parents.end() && parent->second->op == cot_fdiv
								  ? static_cast<const cexpr_t*>(parent->second)
								  : nullptr;
	double one = 0;
	if (insn.itype != NN_rsqrtss || !division || division->y != call || division->x->op != cot_fnum ||
		division->x->fpc->fnum.to_double(&one) != REAL_ERROR_OK || one != 1.0)
		return;
	function.meaning[division] = made("_mm_rsqrt_ss");
	function.meaning[call] = Intrinsic{"_mm_rsqrt_ss", std::string(kSse)};
}

}  // namespace

void resolveIntrinsics(Function& function, Context& context)
{
	std::unordered_map<const cexpr_t*, const citem_t*> parents = parentsOf(function);
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t* s) {
		if (function.omitted(s))
			return;
		if (e->op == cot_cast)
		{
			if (std::optional<Rewrite> constant = vectorConstant(function, context, e, parents))
				function.meaning[e] = *constant;
			return;
		}
		// a zero another pass gave the vector's type (a record's vector member assigned 0)
		if (e->op == cot_num && e->numval() == 0 && toVector(function, context, e))
		{
			function.meaning[e] = Rewrite{1, "_mm_setzero_ps()", {}, {}, "typedef:__m128", std::string(kSse)};
			return;
		}
		if (e->op != cot_call || e->x->op != cot_helper)
			return;
		scalarRoot(function, e, parents);
		const Form* form = nullptr;
		for (const Form& f : kForms)
			if (f.helper == e->x->helper)
				form = &f;
		if (!form || e->a->size() != form->count || !fits(form->result, e->type))
			return;
		for (size_t k = 0; k < form->count; ++k)
		{
			const carg_t& arg = (*e->a)[k];
			if (form->params[k] == "imm" ? arg.op != cot_num : !fits(form->params[k], arg.type))
				return;
		}
		function.meaning[e] = Intrinsic{std::string(form->name), std::string(form->header)};
	});
}

}  // namespace idakit
