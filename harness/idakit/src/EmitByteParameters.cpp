// Byte parameters: a bool or 1-byte integer parameter fills only the low byte of its 4-byte stack slot, whose upper
// bytes the caller may leave as garbage (`exe 0x1, src 0xffffff01`). Where the exe passes such a parameter on as a
// call's stack argument, it re-extends the byte first (`movzx ecx, byte ptr [ebp+8]; push ecx`); clang assumes the
// caller extended it (the ABI's extend) and pushes the whole incoming dword. So at each argument where the exe's push
// takes its register from the parameter's own byte, the parameter is read through a volatile byte pointer, src's
// idiom (`*reinterpret_cast<volatile const bool*>(&abFlag)`), which clang loads as the byte and extends.

#include "EmitPasses.hpp"

#include "idakit/AsmFacts.hpp"

namespace idakit
{

namespace
{

// the key without its top-level const and volatile
std::string unqualified(const SrcTypes& types, std::string key)
{
	for (SrcTypes::Parts p = types.parts(key); p.kind == "const" || p.kind == "volatile"; p = types.parts(key))
		key = p.of;
	return key;
}

// a bool, integer or enum of one byte
bool oneByte(const Context& context, const std::string& key)
{
	std::string k = context.types.canonical(key);
	SrcTypes::Parts p = context.types.parts(k);
	const SrcModel::Type* builtin = p.kind == "builtin" ? context.model.type(k) : nullptr;
	return (p.kind == "enum" || (builtin && !builtin->isFloat)) && context.model.sizeOf(k) == 1;
}

}  // namespace

void rereadByteParameters(Function& function, Context& context)
{
	func_t* pfn = get_func(function.ea);
	if (!pfn)
		return;
	const lvars_t& lvars = *function.cfunc->get_lvars();
	forEachExpression(function, [&](const cexpr_t* call, const cinsn_t*) {
		if (call->op != cot_call)
			return;
		tinfo_t called = call->x->type.is_ptr() ? call->x->type.get_pointed_object() : call->x->type;
		func_type_data_t fd;
		if (!called.get_func_details(&fd))
			return;
		for (size_t k = 0; k < call->a->size() && k < fd.size(); ++k)
		{
			// the parameter as the argument's value, through the casts Hex-Rays reads it with and a copy of it that
			// prints as it (Aliases)
			const cexpr_t* value = stripCasts(&(*call->a)[k]);
			if (value->op != cot_var || !fd[k].argloc.is_stkoff() || fd[k].argloc.stkoff() % 4 != 0 ||
				function.meaning.count(value))
				continue;
			int v = value->v.idx;
			while (function.locals[v].aliasOf >= 0)
				v = function.locals[v].aliasOf;
			const lvar_t& lvar = lvars[v];
			const Local& local = function.locals[v];
			if (!lvar.is_arg_var() || !lvar.is_stk_var() || local.parameter < 0 || local.entryCopy ||
				local.type.empty() || !oneByte(context, local.type))
				continue;
			// the exe's push of the argument's slot takes a register movzx set from the parameter's byte (a nested
			// call between them, which builds another argument or the receiver, leaves the parameter as it is)
			std::optional<PushedArgument> pushed = pushedArgument(call->ea, fd[k].argloc.stkoff());
			if (!pushed || pushed->source.itype != NN_movzx || get_dtype_size(pushed->source.ops[1].dtype) != 1 ||
				calc_stkvar_struc_offset(pfn, pushed->source, 1) !=
					static_cast<ea_t>(function.cfunc->mba->stkoff_vd2ida(lvar.get_stkoff())))
				continue;
			std::optional<std::string> type = context.types.spell(unqualified(context.types, local.type));
			if (!type)
				continue;
			std::string form = "*reinterpret_cast<volatile const " + *type + "*>(&" + local.name + ")";
			function.meaning[value] = Rewrite{1, form, {}, {}, local.type, ""};
		}
	});
}

}  // namespace idakit
