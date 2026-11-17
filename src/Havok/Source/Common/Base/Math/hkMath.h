#pragma once

#include "Reconstruction.hpp"

// Havok's scalar math helpers. Their code stays with the exe.
namespace hkMath {
// Truncates to an int with integer bit operations. Defined in Math/Types/hkMathTypes.cpp.
int32_t hkFloatToInt(float r);

// fld/fistp: rounds in the FPU's current rounding mode, not by truncation. Inline in the SDK
// (Math/Types/Sse/hkSseMathTypes.inl); the exe has no out-of-line copy.
inline int32_t hkToIntFast(float r) {
	int32_t i;
	__asm__("flds %1\n\tfistpl %0" : "=m"(i) : "m"(r));
	return i;
}

// The smaller of x and y; out of line in the exe (cdecl, 0xC9E0F0).
float min2(float x, float y);

// The larger of x and y.
template <typename T> T max2(T x, T y);
// y unless y is below x (0xC98210 tests y < x; an unordered pair gives y).
template <typename T> T max2(T x, T y) {
	return (y < x) ? x : y;
}
// The arc cosine of r (cdecl, the result in ST(0), 0xC74A70).
float acos(float r);
// The arc sine of r (cdecl, 0xCE0C40); its callers read the result in ST(0) unrounded.
double asin(float r);
}
