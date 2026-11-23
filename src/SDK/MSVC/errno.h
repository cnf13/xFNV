#pragma once

#include <cerrno>
#include <cstddef>
#include <cstdint>

// The VC9 CRT's error numbers that the bodies of src/SDK/MSVC store and return. The C libraries src builds with have
// the same values for ENOENT, EACCES, EINVAL and ERANGE; EILSEQ is 42 in VC9 (glibc: 84), EOVERFLOW 132 (glibc: 75),
// and STRUNCATE is VC9's own.
namespace rt {

    enum : int32_t {
        kENOENT = 2,
        kEACCES = 13,
        kEINVAL = 22,
        kERANGE = 34,
        kEILSEQ = 42,
        kSTRUNCATE = 80,
        kEOVERFLOW = 132,
    };

    // VC9's _TRUNCATE: the count that asks strncpy_s and wcstombs_s to truncate instead of failing.
    constexpr size_t kTRUNCATE = static_cast<size_t>(-1);

} // namespace rt
