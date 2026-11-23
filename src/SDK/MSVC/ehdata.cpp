#include "SDK/MSVC/ehdata.h"
#include "SDK/Windows/windef.h"

// The compiler's own record of a thrown type, which vcruntime's _CxxThrowException takes.
struct _s__ThrowInfo;

// The vcruntime's own _CxxThrowException (ehdata_forceinclude.h, which is not a public header), which
// rt::_CxxThrowException forwards to. Declared with C linkage here so that its name does not clash with rt's, and as
// the vcruntime declares it: Microsoft's cl declares the function itself.
namespace xfnv::imports {
    extern "C" [[noreturn]] void WINAPI
    _CxxThrowException(void* pExceptionObject, const _s__ThrowInfo* pThrowInfo) noexcept(false);
} // namespace xfnv::imports

void rt::_CxxThrowException(void* pExceptionObject, const rt::_s__ThrowInfo* pThrowInfo) {
    xfnv::imports::_CxxThrowException(pExceptionObject, reinterpret_cast<const ::_s__ThrowInfo*>(pThrowInfo));
}
