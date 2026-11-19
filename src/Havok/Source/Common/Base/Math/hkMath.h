#pragma once

#include <cstdint>

// Havok's scalar math helpers.
namespace hkMath {
    // Truncates to an int with integer bit operations. Defined in Math/Types/hkMathTypes.cpp.
    int32_t hkFloatToInt(float r);

    // Rounds in the FPU's current rounding mode, not by truncation.
    inline int32_t hkToIntFast(float r) {
        int32_t i;
        __asm__("flds %1\n\tfistpl %0" : "=m"(i) : "m"(r));
        return i;
    }

    float min2(float x, float y);
    // The SDK's inline for other types: x when it is below y, else y.
    template <typename T>
    inline T min2(T x, T y) {
        return x < y ? x : y;
    }

    // The larger of x and y.
    template <typename T>
    T max2(T x, T y);
    // Returns y unless y is below x (tests y < x; an unordered pair gives y).
    template <typename T>
    T max2(T x, T y) {
        return (y < x) ? x : y;
    }
    // The absolute value of r.
    inline float fabs(float r) {
        return __builtin_fabsf(r);
    }

    float acos(float r);
    // The arc sine of r; its callers read the result unrounded.
    double asin(float r);
} // namespace hkMath
