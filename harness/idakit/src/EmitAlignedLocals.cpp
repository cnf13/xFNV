// AlignedLocals: a frame the exe realigns (`and esp, -N`) holds each local at a fixed address mod N, and a callee
// given a local's address may rely on an N-aligned one (movaps). A local whose address the body takes (`&v`, an array
// decaying to its first element) and that the exe holds N-aligned is declared `alignas(N)` where src's type aligns
// it less: clang then realigns src's frame and places it N-aligned too.

#include "EmitPasses.hpp"

#include "idakit/Asm.hpp"

#include <Facts.hpp>

#include <set>

namespace idakit
{

void alignFrameLocals(Function& function, Context& context)
{
	uint32_t align = 0;
	try
	{
		align = context.exe.frameAlign(static_cast<uint32_t>(function.ea));
	}
	catch (const exefacts::Raised&)
	{
	}
	func_t* pfn = get_func(function.ea);
	if (!align || pfn == nullptr)
		return;
	// the frame offset of esp right after the prologue's `and esp, -N` (exefacts' frameAlign reads the same first
	// eight instructions): an N-aligned address
	std::optional<sval_t> aligned;
	std::vector<ea_t> code = heads(pfn);
	for (size_t k = 0; k < code.size() && k < 8 && !aligned; ++k)
		if (Insn insn(code[k]); insn.mnem() == "and" && insn.opText(0) == "esp")
			aligned = get_spd(pfn, get_item_end(code[k])) + frame_off_retaddr(pfn);
	if (!aligned)
		return;
	// the stack locals among those whose address the body takes
	lvars_t& lvars = *function.cfunc->get_lvars();
	std::set<int> taken = addressTakenLocals(*function.cfunc);
	for (int v : taken)
	{
		const lvar_t& lvar = lvars[v];
		Local& local = function.locals[v];
		if (!lvar.is_stk_var() || lvar.is_arg_var())
			continue;
		sval_t offset = function.cfunc->mba->stkoff_vd2ida(lvar.get_stkoff()) - *aligned;
		if (offset % static_cast<sval_t>(align) != 0)
			continue;
		// src's alignment of the type (an array's its element's: the model holds no row for every array key)
		std::string key = local.storage ? local.holds : local.type;
		while (context.types.parts(key).kind == "array")
			key = context.types.parts(key).of;
		std::optional<int64_t> natural = context.model.alignOf(key);
		if (!natural || *natural < static_cast<int64_t>(align))
			local.align = align;
	}
}

}  // namespace idakit
