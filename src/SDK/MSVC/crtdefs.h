#pragma once

#include "Reconstruction.hpp"
#include <cstddef>
#include <cstdint>

// rt: runtime support the game relies on. Here the VC9 CRT's parameter-validation failure (<crtdefs.h>), which the
// game's inlined container checks call by name.
namespace rt {

// __invalid_parameter, 0xEC7C30 (cdecl): reports a parameter-validation failure. The exe never installs a handler
// (it has no _set_invalid_parameter_handler), so its path is always __invoke_watson, which ends the process; the
// body (crtdefs.cpp) ends it too. The CRT functions that call it go on to return their error code, the path they
// take when a handler returns. Defined out of line, so that a test program can link its own, returning one.
int32_t invalid_parameter(const char16_t* pExpression, const char16_t* pFunctionName, const char16_t* pFileName,
	uint32_t uiLineNo, uintptr_t uiReserved);

// _invalid_parameter_noinfo, 0xEC7C56: __invalid_parameter with no information.
inline void _invalid_parameter_noinfo()
{
	invalid_parameter(nullptr, nullptr, nullptr, 0, 0);
}

} // namespace rt
