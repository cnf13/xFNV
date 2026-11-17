#include "SDK/MSVC/ehdata.h"
#include "SDK/Windows/windef.h"

// The vcruntime's own _CxxThrowException (ehdata_forceinclude.h, which is not a public header), which
// rt::_CxxThrowException forwards to. Declared with C linkage here so that its name does not clash with rt's; the
// ThrowInfo is the compiler's own record of the thrown type.
namespace fnvd::imports {
    extern "C" [[noreturn]] void WINAPI _CxxThrowException(void* pExceptionObject, const void* pThrowInfo);
} // namespace fnvd::imports

void rt::_CxxThrowException(void* pExceptionObject, const rt::_s__ThrowInfo* pThrowInfo) {
    fnvd::imports::_CxxThrowException(pExceptionObject, pThrowInfo);
}
