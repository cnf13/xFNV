#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <cstdint>

// Havok's scalar math helpers.
namespace hkMath {
    // Truncates to an int with integer bit operations. Defined in Math/Types/hkMathTypes.cpp.
    int32_t hkFloatToInt(float r);

    // Rounds in the FPU's current rounding mode, not by truncation.
    inline int32_t hkToIntFast(float r) {
        int32_t i;
        xfnv::RoundToInt(i, r);
        return i;
    }

    // Whether r's exponent is not all ones: neither infinite nor NaN.
    inline hkBool isFinite(float r) {
        return (std::bit_cast<uint32_t>(r) & 0x7F800000u) != 0x7F800000u;
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
        return std::fabsf(r);
    }

    float acos(float r);
    // The arc sine of r; its callers read the result unrounded.
    double asin(float r);
} // namespace hkMath
