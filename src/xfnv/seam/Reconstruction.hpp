#pragma once

// What is left of the seam between the reconstructed code and the original executable. The hybrid build, a DLL loaded
// into the original FalloutNV.exe, is gone: each form below is the plain C++ it stands for, and goes as its uses are
// rewritten into that C++ (the "at parity" form each one names).

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>
// __m128: the 16-byte SSE vector IDA types the exe's packed-single code and its 16-byte pointer parameters with
// (hkVector4 and friends). The compiler's own type, spelled as IDA does.
#include <xmmintrin.h>

// A 64-bit target (the x64 program): for the few places where a pointer-sized value cannot keep the exe's 32-bit
// form (code only that build needs). The 32-bit builds never see its side.
#if INTPTR_MAX == INT64_MAX
#define XFNV_64BIT 1
#else
#define XFNV_64BIT 0
#endif

namespace xfnv {

    // `type name` as raw typed storage: no constructor or destructor runs, so the program runs exactly the body's
    // statements, as the exe does. At parity the form becomes a real local.
    template <class T>
    class NativeInlinedLocal {
    public:
        NativeInlinedLocal(std::nullopt_t, std::nullopt_t) {}
        NativeInlinedLocal(const NativeInlinedLocal&) = delete;
        NativeInlinedLocal& operator=(const NativeInlinedLocal&) = delete;

        T& object() {
            return *reinterpret_cast<T*>(m_bytes);
        }

    private:
        alignas(T) unsigned char m_bytes[sizeof(T)];
    };
#define XFNV_INLINED_LOCAL(type, name, atStart, atEnd)                                                                 \
    ::xfnv::NativeInlinedLocal<type> name##Storage_{atStart, atEnd};                                                   \
    type& name = name##Storage_.object()

} // namespace xfnv
