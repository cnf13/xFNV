// Variadic arguments: Calls' variadic peel (EmitCalls.cpp's peelVariadic, the Calls row) decided once every layer
// that fixes an operand's src type has run: a nested call Calls resolves, a global Globals names, a local the structor
// or inlined locals retype, an argument foldOutgoingArguments replaces. The casts it peels from a resolved call's
// arguments past a C-variadic callee's declared parameters print as their operand (Implicit). Before carryConst, which
// reads such a cast as its operand (a variadic argument keeps a pointer's const), and which changes no operand's kind.

#include "EmitPasses.hpp"

namespace idakit
{

void peelVariadicArguments(Function& function, Context& context)
{
	for (const cexpr_t* cast : variadicCasts(function, context))
		function.meaning[cast] = Implicit{};
}

}  // namespace idakit
