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

    // An exe vtable: the vptr the exe's inlined constructor stores in an XFNV_INLINED_LOCAL. There are no exe vtables
    // here: each constant is 0, and only says that the local starts with its class's own vptr.
    struct Vtable {
        uintptr_t address;
    };

    // `type name` whose constructor and destructor the exe inlines down to their vptr stores (atStart, atEnd: Vtable
    // constants or std::nullopt), the body writing the rest of them by hand (its EXE-INLINED steps). It is raw typed
    // storage, so the program runs exactly the body's statements, as the exe does: no destructor runs at the scope's
    // end, and the only construction is the class's default constructor where atStart names a vtable (it gives the
    // object the class's own vptr; atEnd stores nothing). At parity the form becomes `type name;` with the hand-inlined
    // steps (the EXE-INLINED lines) removed.
    template <class T>
    class NativeInlinedLocal {
    public:
        template <class E>
        NativeInlinedLocal(std::nullopt_t, E) {}
        template <class E>
        NativeInlinedLocal(Vtable, E) {
            ::new (static_cast<void*>(m_bytes)) T;
        }
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
