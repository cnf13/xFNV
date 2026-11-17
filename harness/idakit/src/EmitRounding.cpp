// Rounding: Hex-Rays prints the exe's every float-to-integer conversion as a cast, `(int)x`, which C++ truncates. The
// exe truncates where it calls _ftol2 or converts under a truncating control word (`fnstcw; or eax, 0C00h; fldcw;
// fistp; fldcw`), and those stay casts; a bare `fistp`/`fist` (no `fldcw` before it in its block, or one that loads
// back the word an `fnstcw` saved) rounds in the FPU's current mode, as the source's inline helpers do (Havok's
// hkMath::hkToIntFast, Gamebryo's NiFastFloatToInt, the game's FloatToInt: `fld dword; fistp dword`). The instruction
// is the first conversion at or after the operand's own instruction in its block (Hex-Rays keeps no address on the
// cast). Such a cast of a float the exe loads with `fld dword` right before, to a 4-byte integer with `fistp dword`,
// prints as src's NiFastFloatToInt(x) (NiMath.h's inline fld/fistp, which the checker runs natively), outside Havok's
// code; in Havok's (the body's src declaration under src/Havok/) it is marked: src does not declare hkToIntFast. Any
// other rounding conversion (of an x87 value not rounded to a float, into a word or a qword) is marked, as is a
// cast whose conversion is not found in a function that has a bare fistp.

#include "EmitPasses.hpp"

#include <ua.hpp>

namespace idakit
{

namespace
{

// what converts a float to an integer at a site
enum class Conversion
{
	none,		// not found in the block
	truncates,	// a call (_ftol2), an SSE convert, fisttp, or fistp under a loaded control word
	rounds,		// a bare fistp/fist
};

class Rounding
{
public:
	Rounding(Function& function, Context& context) : m_function(function), m_context(context) {}

	void run()
	{
		std::vector<const cexpr_t*> casts;
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			if (e->op == cot_cast && e->x->type.is_floating() && (e->type.is_integral() || e->type.is_enum()) &&
				!e->type.is_bool())
				casts.push_back(e);
		});
		if (casts.empty())
			return;
		bool bare = anyBare();
		for (const cexpr_t* e : casts)
		{
			insn_t fistp;
			Conversion c = conversionOf(e, fistp);
			if (c == Conversion::truncates || (c == Conversion::none && !bare))
				continue;
			if (c == Conversion::none)
			{
				m_function.meaning[e] =
					Missing{"converts a floating value where the function rounds with a bare fistp elsewhere and the "
							"instruction converting this one is not found"};
				continue;
			}
			mark(e, fistp);
		}
	}

private:
	void mark(const cexpr_t* e, const insn_t& fistp)
	{
		const std::string rounds = "rounds in the FPU's mode (a bare fistp at " + hex(fistp.ea) + ")";
		size_t stored = storedBytes(fistp.ea);
		if (stored != 4 || e->type.get_size() != 4)
		{
			m_function.meaning[e] = Missing{rounds + " into " + std::to_string(stored) + " bytes, cast to " +
											std::to_string(e->type.get_size()) +
											": src has no rounding helper of that width"};
			return;
		}
		if (e->x->type.get_size() != 4 || !loadsFloat(fistp.ea))
		{
			m_function.meaning[e] =
				Missing{rounds + " of an x87 value not rounded to a float: src has no rounding helper of it"};
			return;
		}
		const SrcModel::Function* self = m_function.self;
		if (self && self->file.rfind("src/Havok/", 0) == 0)
		{
			m_function.meaning[e] =
				Missing{rounds + ": Havok's inlined hkMath::hkToIntFast, which src does not declare"};
			return;
		}
		const std::vector<const SrcModel::Function*>& helpers = m_context.model.functionsNamed("NiFastFloatToInt");
		const SrcModel::Type* type = helpers.size() == 1 ? m_context.model.type(helpers[0]->type) : nullptr;
		if (!type)
		{
			m_function.meaning[e] = Missing{rounds + ": src declares no NiFastFloatToInt"};
			return;
		}
		const std::string& file = helpers[0]->file;
		m_function.meaning[e] = Rewrite{1, "NiFastFloatToInt($0)", {e->x}, {}, type->ret,
										file.substr(file.rfind("src/", 0) == 0 ? 4 : 0)};
	}

	// the integer bytes an fist/fistp stores, from its opcode (IDA's operand dtype does not say): DB is a dword, DF
	// a word (/2, /3) or a qword (/7)
	static size_t storedBytes(ea_t ea)
	{
		uchar opcode = get_byte(ea);
		if (opcode == 0xDB)
			return 4;
		if (opcode == 0xDF)
			return ((get_byte(ea + 1) >> 3) & 7) == 7 ? 8 : 2;
		return 0;
	}

	static std::string hex(ea_t ea)
	{
		char text[32];
		std::snprintf(text, sizeof text, "0x%llX", static_cast<unsigned long long>(ea));
		return text;
	}

	// the function has a bare fistp/fist anywhere
	bool anyBare() const
	{
		func_t* pfn = get_func(m_function.ea);
		func_item_iterator_t items;
		for (bool ok = pfn && items.set(pfn); ok; ok = items.next_code())
		{
			insn_t insn;
			if (decode_insn(&insn, items.current()) > 0 && (insn.itype == NN_fistp || insn.itype == NN_fist) &&
				!underControlWord(insn.ea))
				return true;
		}
		return false;
	}

	// the conversion of the cast's operand: the first converting instruction from the operand's instruction on, in
	// its block
	Conversion conversionOf(const cexpr_t* e, insn_t& found) const
	{
		ea_t ea = e->x->ea != BADADDR ? e->x->ea : e->ea;
		func_t* pfn = get_func(m_function.ea);
		if (ea == BADADDR || !pfn || !func_contains(pfn, ea))
			return Conversion::none;
		for (int k = 0; k < 24 && ea != BADADDR; ++k)
		{
			insn_t insn;
			if (decode_insn(&insn, ea) <= 0)
				return Conversion::none;
			switch (insn.itype)
			{
			case NN_fistp:
			case NN_fist:
				found = insn;
				return underControlWord(insn.ea) ? Conversion::truncates : Conversion::rounds;
			case NN_fisttp:
			case NN_cvttss2si:
			case NN_cvttsd2si:
			case NN_cvtss2si:
			case NN_cvtsd2si:
			case NN_call:
				return Conversion::truncates;
			default:
				break;
			}
			if (is_basic_block_end(insn, false))
				return Conversion::none;
			ea = next_head(ea, pfn->end_ea);
			if (ea == BADADDR || !is_flow(get_flags(ea)))
				return Conversion::none;
		}
		return Conversion::none;
	}

	// the x87 instruction before the conversion at ea (in its block) is `fld dword [m]`: st(0) holds a float's value
	static bool loadsFloat(ea_t ea)
	{
		for (int k = 0; k < 8 && is_flow(get_flags(ea)); ++k)
		{
			ea = prev_head(ea, 0);
			insn_t insn;
			if (ea == BADADDR || decode_insn(&insn, ea) <= 0)
				return false;
			// `fld m32fp`: D9 /0 with a memory operand (IDA's operand dtype does not say)
			if (insn.itype == NN_fld)
				return get_byte(ea) == 0xD9 && ((get_byte(ea + 1) >> 3) & 7) == 0 && (get_byte(ea + 1) >> 6) != 3;
			// the control and status word instructions leave st(0); any other x87 instruction computes, moves or pops it
			if (insn.itype == NN_fldcw || insn.itype == NN_fstcw || insn.itype == NN_fnstcw || insn.itype == NN_fstsw ||
				insn.itype == NN_fnstsw || insn.itype == NN_fclex || insn.itype == NN_fnclex)
				continue;
			if ((insn.itype >= NN_fld && insn.itype <= NN_fndisi) ||
				(insn.itype >= NN_fprem1 && insn.itype <= NN_fucompp) ||
				(insn.itype >= NN_fcmovb && insn.itype <= NN_fucomip) || insn.itype == NN_ffreep)
				return false;
		}
		return false;
	}

	// The conversion at ea runs under a control word an `fldcw` before it in its block loaded, other than the one an
	// `fnstcw` saved (the restore that ends a truncation): MSVC's truncation sequence.
	static bool underControlWord(ea_t ea)
	{
		for (int k = 0; k < 64 && is_flow(get_flags(ea)); ++k)
		{
			ea = prev_head(ea, 0);
			insn_t insn;
			if (ea == BADADDR || decode_insn(&insn, ea) <= 0)
				return false;
			if (insn.itype != NN_fldcw)
				continue;
			// the store into the word it loads, earlier in the block: a restore where it is the fnstcw's
			const op_t& word = insn.ops[0];
			for (int j = 0; j < 64 && is_flow(get_flags(ea)); ++j)
			{
				ea = prev_head(ea, 0);
				insn_t store;
				if (ea == BADADDR || decode_insn(&store, ea) <= 0)
					return true;
				const op_t& to = store.ops[0];
				if (to.type == word.type && to.reg == word.reg && to.addr == word.addr &&
					(to.type == o_mem || to.type == o_displ || to.type == o_phrase))
					return !(store.itype == NN_fnstcw || store.itype == NN_fstcw);
			}
			return true;
		}
		return false;
	}

	Function& m_function;
	Context& m_context;
};

}  // namespace

void markRoundingConversions(Function& function, Context& context)
{
	Rounding(function, context).run();
}

}  // namespace idakit
