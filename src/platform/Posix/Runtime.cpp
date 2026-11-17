// The MSVC runtime's exports src/SDK/MSVC forwards to, on POSIX, where there is no vcruntime: Windows links
// vcruntime's own, so this file has no Win32 side.

#include <cstdio>
#include <cstdlib>

// vcruntime's _CxxThrowException raises a C++ exception from MSVC's throw record (rt::_CxxThrowException's
// target, src/SDK/MSVC/ehdata.cpp). The Itanium C++ ABI cannot raise an MSVC throw record, so the throw is
// reported and the process ends, as an exception no handler catches would end it.
extern "C" [[noreturn]] void _CxxThrowException(void* pExceptionObject, const void* pThrowInfo) {
    std::fprintf(
        stderr, "_CxxThrowException(%p, %p): an MSVC throw record cannot be raised on POSIX\n", pExceptionObject,
        pThrowInfo
    );
    std::abort();
}
