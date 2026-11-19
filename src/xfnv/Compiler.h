#pragma once

// What the game's code takes from its compiler beyond standard C++, for clang (clang++, clang-cl) and Microsoft's cl.
// Each clang form is the builtin or asm the code was written with, so clang's output is what it was; cl builds the
// 64-bit Windows program only (cmake/x86_64-windows-msvc-wine.cmake). It includes the x86 intrinsics (clang's
// x86intrin.h, cl's intrin.h), which the users of __rdtsc take from here.

#include <cstdint>

// clang first: clang-cl defines _MSC_VER too. Every #else below is cl's.
#if defined(__clang__)
#include <x86intrin.h>
#elif defined(_MSC_VER)
#include <intrin.h>
#include <malloc.h>
static_assert(sizeof(void*) == 8, "cl builds the 64-bit program only (its _alloca aligns 16 there)");
#else
#error "xfnv/Compiler.h: clang (clang++, clang-cl) or Microsoft's cl only"
#endif

// The float in lane i of an __m128, as an lvalue: clang's vector subscript, cl's union member.
#if defined(__clang__)
#define XFNV_M128_F32(v, i) (v)[i]
#else
#define XFNV_M128_F32(v, i) (v).m128_f32[i]
#endif

// size bytes of the calling function's stack frame: XFNV_ALLOCA16's 16-byte aligned (clang's alignment is in bits).
#if defined(__clang__)
#define XFNV_ALLOCA(size) __builtin_alloca(size)
#define XFNV_ALLOCA16(size) __builtin_alloca_with_align(size, 128)
#else
#define XFNV_ALLOCA(size) _alloca(size)
#define XFNV_ALLOCA16(size) _alloca(size)
#endif

// The calling function's return address.
#if defined(__clang__)
#define XFNV_RETURN_ADDRESS() __builtin_return_address(0)
#else
#define XFNV_RETURN_ADDRESS() _ReturnAddress()
#endif

// A function whose loops clang must not turn into calls of the named C runtime function (the exe's inline loops).
#if defined(__clang__)
#define XFNV_NO_BUILTIN(name) __attribute__((no_builtin(name)))
#else
#define XFNV_NO_BUILTIN(name)
#endif

namespace xfnv {

    // int3: stops in an attached debugger.
    inline void DebugTrap() {
#if defined(__clang__)
        __builtin_debugtrap();
#else
        __debugbreak();
#endif
    }

    // Hides the pointer's value from the optimiser: clang knows a member's address is non-null and drops the code's
    // null tests of it (cl drops none).
    template <class T>
    inline void HidePointer(T*& p) {
#if defined(__clang__)
        __asm__("" : "+r"(p));
#else
        (void)p;
#endif
    }

    // Keeps a parameter that is never read live: clang's dead argument elimination would leave the callers' argument
    // unset.
    template <class T>
    inline void KeepLive(T value) {
#if defined(__clang__)
        __asm__ volatile("" : : "r"(value));
#else
        (void)value;
#endif
    }

    // result = value rounded to an int in the current rounding mode, not truncated: x87 fld/fistp on the caller's own
    // memory as the exe does; cl's x64 has no x87 asm and rounds by cvtss2si in SSE's mode (both round to nearest
    // unless the game changes them).
    inline void RoundToInt(int32_t& result, const float& value) {
#if defined(__clang__)
        __asm__("flds %1\n\tfistpl %0" : "=m"(result) : "m"(value));
#else
        result = _mm_cvtss_si32(_mm_set_ss(value));
#endif
    }

    // sqrtss: the float's square root without the C runtime's errno.
    inline float SqrtF(float value) {
#if defined(__clang__)
        return __builtin_elementwise_sqrt(value);
#else
        return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(value)));
#endif
    }

    // cpuid of the leaf in eax: its four registers. CpuidEdxIn also passes edx in, as one of the exe's probes does
    // (cpuid ignores it).
    inline void Cpuid(uint32_t& eax, uint32_t& ebx, uint32_t& ecx, uint32_t& edx) {
#if defined(__clang__)
        __asm__ volatile("cpuid" : "+a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx));
#else
        int32_t aiRegs[4];
        __cpuid(aiRegs, static_cast<int32_t>(eax));
        eax = static_cast<uint32_t>(aiRegs[0]);
        ebx = static_cast<uint32_t>(aiRegs[1]);
        ecx = static_cast<uint32_t>(aiRegs[2]);
        edx = static_cast<uint32_t>(aiRegs[3]);
#endif
    }

    inline void CpuidEdxIn(uint32_t& eax, uint32_t& ebx, uint32_t& ecx, uint32_t& edx) {
#if defined(__clang__)
        __asm__ volatile("cpuid" : "+a"(eax), "=b"(ebx), "=c"(ecx), "+d"(edx));
#else
        Cpuid(eax, ebx, ecx, edx);
#endif
    }

} // namespace xfnv
