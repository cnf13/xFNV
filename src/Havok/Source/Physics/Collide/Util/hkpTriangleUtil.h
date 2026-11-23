#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// The tolerance the mesh shapes and the welding utility pass to hkpTriangleUtil::isDegenerate.
inline float hkDefaultTriangleDegeneracyTolerance = 1e-07f;

// Havok's helpers for single triangles.
class hkpTriangleUtil {
public:
    // Whether the triangle a, b, c is degenerate: a sliver (an edge cross product of squared length under tolerance) or
    // a zero area.
    static hkBool isDegenerate(const hkVector4& a, const hkVector4& b, const hkVector4& c, float tolerance);
    static uint8_t* rayIntersect(
        uint8_t* result,
        float* t,
        float* u,
        float* v,
        __m128* pos,
        __m128* dir,
        __m128* a,
        __m128* b,
        __m128* c,
        char perform_culling
    );
};
