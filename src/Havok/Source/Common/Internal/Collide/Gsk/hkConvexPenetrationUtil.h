#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Internal/Collide/Gsk/hkGsk.h"
#include "Havok/Source/Common/Internal/Collide/Gsk/hkWingedEdge.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/Mesh/hkGskShape.h"

double getRandomReal01(int32_t seed);
// seed is passed on to getRandomReal01; vecOut receives the three output floats.
void randomVector(float min, float max, int32_t seed, float* vecOut);

// hkInternal's copy of the expanding polytope solver for the penetration depth (hkpConvexPenetrationUtil's twin on
// hkGskShape and plain vectors). It is the one the hkInternal GSK solver's penetration case runs. It writes an
// hkGskOut, with the distance in m_normalInA's w.
class hkConvexPenetrationUtil {
public:
    // The point of the Minkowski difference furthest along direction, with the support points of A (along direction)
    // and B (along the negated direction, in B's space) it came from, through each shape's getSupportingVertex.
    void getPointForSimplex(const hkVector4& direction, hkWingedEdge::Vertex& result);
    // builtTetrahedron calls addPointToSimplex.
    void addPointToSimplex(const hkWingedEdge::Vertex& p);
    // Through getPointForSimplex: the larger of the two distances goes in all four lanes of *apBestDistanceOut, which
    // is also returned.
    hkVector4 findBestProjection(const hkVector4& direction, const hkVector4& from, hkWingedEdge::Vertex& result);
    // Grows the simplex to a tetrahedron around the origin (HK_SUCCESS), or writes output itself (HK_FAILURE, gjkStatus
    // OK); gjkStatus 5 where it cannot.
    hkResult builtTetrahedron(hkGskOut& output, hkGskStatus& gjkStatus);
    // Works through builtTetrahedron and hkWingedEdge.
    hkGskStatus calculatePenetrationDepth(hkGskOut& output);

    hkTransform m_aTb;
    hkVector4 m_epsTol;
    hkPadSpu<float> m_epsTolSqrd;
    hkPadSpu<const hkGskShape*> m_shapeA;
    hkPadSpu<const hkGskShape*> m_shapeB;
    hkPadSpu<int32_t> m_backupTries;
    hkPadSpu<hkVector4*> m_pointsAinA;
    hkPadSpu<hkVector4*> m_pointsBinB;
    hkPadSpu<hkVector4*> m_simplex;
    hkPadSpu<int32_t> m_simplexSize;
};
