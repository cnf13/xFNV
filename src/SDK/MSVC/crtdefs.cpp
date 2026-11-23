#include "SDK/MSVC/crtdefs.h"

#include <cstdlib>

// The home of the exe's out-of-line VC9 <crtdefs.h> bodies.

namespace rt {

    // __invoke_watson ends the process with STATUS_INVALID_CRT_PARAMETER after the debugger hook, without running the
    // atexit list; abort is the standard way to end it abnormally (no atexit list either).
    int32_t invalid_parameter(
        const char16_t* pExpression,
        const char16_t* pFunctionName,
        const char16_t* pFileName,
        uint32_t uiLineNo,
        uintptr_t uiReserved
    ) {
        static_cast<void>(pExpression);
        static_cast<void>(pFunctionName);
        static_cast<void>(pFileName);
        static_cast<void>(uiLineNo);
        static_cast<void>(uiReserved);
        std::abort();
    }
} // namespace rt
