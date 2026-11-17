#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

// Havok's helpers for single triangles.
class hkpTriangleUtil {
public:
	// Whether the triangle a, b, c is degenerate: a sliver (an edge cross product of squared length under tolerance)
	// or a zero area. The exe returns the hkBool through a hidden pointer, so isDegenerate takes it as its first
	// argument.
	static hkBool* isDegenerate(hkBool* apResult, const hkVector4& a, const hkVector4& b, const hkVector4& c,
		float tolerance);
	static uint8_t* rayIntersect(uint8_t* a1, float* a2, float* a3, float* a4, __m128* a5, __m128* a6, __m128* a7,
		__m128* a8, __m128* a9, char a10);
};
