#pragma once

#include <cstdint>
#include <xmmintrin.h>

class hkVector4;

// File-static helper. Computes the separating normal and closest point on A of the simplex of dimA and dimB vertices;
// callers in hk4dGskCollideCalcToi pass the hkpGsk's m_verticesA and m_verticesBinA.
void hkGskRecalcContactInternal(
    const hkVector4* verticesA,
    const hkVector4* verticesB,
    int32_t dimA,
    int32_t dimB,
    const hkVector4* hint,
    hkVector4* pointOut,
    hkVector4* normalOut
);
// hkGskRecalcContactInternal's point-triangle normal: triangle, normalOut, distanceOut (its x), point,
// hint; returns distanceOut.
hkVector4* calcTriangleNormalAndDistance(
    const hkVector4* triangle,
    hkVector4* normalOut,
    hkVector4* distanceOut,
    const hkVector4* point,
    const hkVector4* hint
);
// hkGskRecalcContactInternal's edge-edge normal: verticesA, the hint, pointOut, verticesB and normalOut.
void calcEdgeEdgeNormalAndDistance(
    const hkVector4* verticesA,
    const hkVector4* hint,
    hkVector4* pointOut,
    const hkVector4* verticesB,
    hkVector4* normalOut
);
// A file-static helper of hkpGsk.cpp (no symbol in hkpInternal.lib):
// the tetrahedron of the four vertices against the point, with fSign the side sign.
int32_t findClosestTriangle(float fSign, hkVector4* vertices, hkVector4* point);
// The segment simplex vertices against the point: writes the separating direction to searchDirOut, and decrements
// *numVertices when the simplex drops to a vertex. flipSign == 1 flips the sign.
int32_t calcEdgeSearchDirection(
    hkVector4* vertices,
    int32_t* numVertices,
    const hkVector4* point,
    hkVector4* searchDirOut,
    int32_t flipSign
);

// Guessed bound: the database gives none; the next data item starts 0x8 bytes on.
using BoundaryMaskFeatureTable = char[8];
inline BoundaryMaskFeatureTable boundaryMaskToFeature{-1, 0, 1, -6, 2, -7, -8, -1};
