#include "Havok/Source/Physics/Internal/Collide/Gjk/hkpGsk.h"

#include "Havok/Source/Common/Base/Algorithm/Collide/LineSegment/hkLineSegmentUtil.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Thread/Thread/hkThreadLocalData.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGsk.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Gjk/Penetration/hkpConvexPenetrationUtil.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpCdVertex.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Util/hkpCollideTriangleUtil.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/hkpGskCache.h"

#include <cmath>
#include <xmmintrin.h>

// The triangle's vertex whose edge plane the point lies outside of:
// vertices holds the triangle's three vertices and a fourth point, the edge distances are cleared to -1 where the
// point is on the inner side; -1 when no distance stays positive.
static int32_t findClosestTriangleEdge(const hkVector4* point, float* edgeDistances, const hkVector4* vertices);

// For each edge (vertexToEdgeLut's pair of edge distances, both not negative), sets one of the two distances to -1
// by the sign of the 2D cross term of the point and the fourth vector against the edge's vertices; then returns the
// first edge whose distance stays positive, -1 when none does. The 0 and -1 are kept on the x87 stack.
static int32_t findClosestTriangleEdge(const hkVector4* point, float* edgeDistances, const hkVector4* vertices) {
    const hkVector4* vertex = vertices;
    for (int32_t i = 0; i < 3; ++i, ++vertex) {
        const int32_t e2 = hkpCollideTriangleUtil::vertexToEdgeLut[i + 2];
        const int32_t e0 = hkpCollideTriangleUtil::vertexToEdgeLut[i];
        if (0.0f <= edgeDistances[e2] && 0.0f <= edgeDistances[e0]) {
            hkVector4 a;
            a.setSub4(vertices[e2], *vertex);
            hkVector4 c;
            c.setSub4(vertices[3], *vertex);
            hkVector4 b;
            b.setSub4(vertices[e0], *vertex);
            hkVector4 p;
            p.setSub4(*point, *vertex);
            const __m128 pb = p.dot3(b).getQuad();
            const __m128 pa = p.dot3(a).getQuad();
            const __m128 ca = c.dot3(a).getQuad();
            const __m128 cb = c.dot3(b).getQuad();
            const __m128 det = _mm_sub_ss(_mm_mul_ss(pa, cb), _mm_mul_ss(pb, ca));
            if ((_mm_movemask_ps(det) & 1) != 0) {
                edgeDistances[e0] = -1.0f;
            }
            else {
                edgeDistances[e2] = -1.0f;
            }
        }
    }
    if (0.0f < edgeDistances[0]) {
        return 0;
    }
    if (0.0f < edgeDistances[1]) {
        return 1;
    }
    if (0.0f < edgeDistances[2]) {
        return 2;
    }
    return -1;
}

// The separating normal (w the signed distance) and the closest point on A of a simplex of dimA vertices of A and dimB
// of B, both in one space; dimA * 8 | dimB picks the case: point-point, point-edge, edge-point, edge-edge,
// point-triangle and triangle-point. hint is the previous normal: a degenerate feature keeps it, and an edge-edge or
// triangle normal is turned to its side. Other dimensions write nothing. Each dot product is summed as z + (y + x).
void hkGskRecalcContactInternal(
    const hkVector4* verticesA,
    const hkVector4* verticesB,
    int32_t dimA,
    int32_t dimB,
    const hkVector4* hint,
    hkVector4* pointOut,
    hkVector4* normalOut
) {
    const int32_t iCase = dimA * 8 | dimB;
    switch (iCase) {
        case 9: {
            const __m128 kDiff = _mm_sub_ps(verticesA[0].m_quad, verticesB[0].m_quad);
            normalOut->m_quad = kDiff;
            const __m128 kHint = hint->m_quad;
            const __m128 kHintDiff = _mm_mul_ps(kHint, kDiff);
            const float fHintDot = _mm_cvtss_f32(_mm_add_ss(
                _mm_shuffle_ps(kHintDiff, kHintDiff, 0xAA),
                _mm_add_ss(_mm_shuffle_ps(kHintDiff, kHintDiff, 0x55), kHintDiff)
            ));
            const __m128 kSq = _mm_mul_ps(kDiff, kDiff);
            const __m128 kLen2 =
                _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq));
            __m128 kW;
            // Compares the squared length against the hint's dot product times 1000.0.
            if (!(static_cast<double>(_mm_cvtss_f32(kLen2)) > static_cast<double>(fHintDot) * 1000.0)) {
                const __m128 kApprox = _mm_rsqrt_ss(kLen2);
                const __m128 kIsZero = _mm_cmpeq_ss(kLen2, _mm_set_ss(0.0f));
                const __m128 kRefined = _mm_mul_ss(
                    _mm_mul_ss(_mm_set_ss(0.5f), kApprox),
                    _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(kLen2, kApprox), kApprox))
                );
                const __m128 kMasked = _mm_andnot_ps(kIsZero, kRefined);
                const __m128 kInv = _mm_shuffle_ps(kMasked, kMasked, 0);
                normalOut->m_quad = _mm_mul_ps(kInv, kDiff);
                kW = _mm_mul_ss(kLen2, kInv);
            }
            else {
                kW = _mm_add_ss(
                    _mm_shuffle_ps(kHintDiff, kHintDiff, 0xAA),
                    _mm_add_ss(_mm_shuffle_ps(kHintDiff, kHintDiff, 0x55), kHintDiff)
                );
                normalOut->m_quad = kHint;
            }
            normalOut->m_quad[3] = _mm_cvtss_f32(kW);
            pointOut->m_quad = verticesA[0].m_quad;
            break;
        }
        case 10: {
            const __m128 kDiff = _mm_sub_ps(verticesA[0].m_quad, verticesB[0].m_quad);
            const __m128 kHint = hint->m_quad;
            normalOut->m_quad = kHint;
            const __m128 kProd = _mm_mul_ps(kDiff, kHint);
            normalOut->m_quad[3] = _mm_cvtss_f32(
                _mm_add_ss(_mm_shuffle_ps(kProd, kProd, 0xAA), _mm_add_ss(_mm_shuffle_ps(kProd, kProd, 0x55), kProd))
            );
            pointOut->m_quad = verticesA[0].m_quad;
            break;
        }
        case 17: {
            const __m128 kDiff = _mm_sub_ps(verticesB[0].m_quad, verticesA[0].m_quad);
            const __m128 kHint = hint->m_quad;
            normalOut->m_quad = kHint;
            const __m128 kProd = _mm_mul_ps(kDiff, kHint);
            const __m128 kDist =
                _mm_add_ss(_mm_shuffle_ps(kProd, kProd, 0xAA), _mm_add_ss(_mm_shuffle_ps(kProd, kProd, 0x55), kProd));
            const __m128 kPointB = verticesB[0].m_quad;
            pointOut->m_quad = kPointB;
            pointOut->m_quad = _mm_sub_ps(kPointB, _mm_mul_ps(_mm_shuffle_ps(kDist, kDist, 0), normalOut->m_quad));
            normalOut->m_quad[3] = _mm_cvtss_f32(_mm_sub_ss(hkQuadReal0000, kDist));
            break;
        }
        case 18: {
            hkVector4 dA;
            hkVector4 dB;
            hkLineSegmentUtil::ClosestLineSegLineSegResult result;
            dA.m_quad = _mm_sub_ps(verticesA[1].m_quad, verticesA[0].m_quad);
            dB.m_quad = _mm_sub_ps(verticesB[1].m_quad, verticesB[0].m_quad);
            hkLineSegmentUtil::closestLineSegLineSeg(verticesA[0], dA, verticesB[0], dB, result);
            const __m128 kCross = _mm_sub_ps(
                _mm_mul_ps(_mm_shuffle_ps(dB.m_quad, dB.m_quad, 0xD2), _mm_shuffle_ps(dA.m_quad, dA.m_quad, 0xC9)),
                _mm_mul_ps(_mm_shuffle_ps(dB.m_quad, dB.m_quad, 0xC9), _mm_shuffle_ps(dA.m_quad, dA.m_quad, 0xD2))
            );
            const __m128 kSum = _mm_add_ps(_mm_mul_ps(dB.m_quad, dB.m_quad), _mm_mul_ps(dA.m_quad, dA.m_quad));
            const float fSum = _mm_cvtss_f32(
                _mm_add_ss(_mm_shuffle_ps(kSum, kSum, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSum, kSum, 0x55), kSum))
            );
            const __m128 kCrossSq = _mm_mul_ps(kCross, kCross);
            const float fCrossLen2 = _mm_cvtss_f32(_mm_add_ss(
                _mm_shuffle_ps(kCrossSq, kCrossSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kCrossSq, kCrossSq, 0x55), kCrossSq)
            ));
            normalOut->m_quad = kCross;
            __m128 kNormal;
            // Compares in double precision: the cross product's squared length times 0.001f (widened to double) against
            // the square of the edges' summed squared lengths.
            if (!(static_cast<double>(fCrossLen2) * 0.0010000000474974513 > static_cast<double>(fSum) * fSum)) {
                kNormal = hint->m_quad;
            }
            else {
                const __m128 kHintProd = _mm_mul_ps(hint->m_quad, kCross);
                const __m128 kHintDot = _mm_add_ss(
                    _mm_shuffle_ps(kHintProd, kHintProd, 0xAA),
                    _mm_add_ss(_mm_shuffle_ps(kHintProd, kHintProd, 0x55), kHintProd)
                );
                const __m128 kSided =
                    _mm_xor_ps(_mm_and_ps(_mm_shuffle_ps(kHintDot, kHintDot, 0), hkQuadRealNegateMask), kCross);
                const __m128 kSq = _mm_mul_ps(kSided, kSided);
                const __m128 kLen2 =
                    _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq));
                const __m128 kApprox = _mm_rsqrt_ss(kLen2);
                const __m128 kIsZero = _mm_cmpeq_ss(kLen2, _mm_set_ss(0.0f));
                const __m128 kRefined = _mm_mul_ss(
                    _mm_mul_ss(_mm_set_ss(0.5f), kApprox),
                    _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(kLen2, kApprox), kApprox))
                );
                const __m128 kMasked = _mm_andnot_ps(kIsZero, kRefined);
                kNormal = _mm_mul_ps(_mm_shuffle_ps(kMasked, kMasked, 0), kSided);
            }
            normalOut->m_quad = kNormal;
            const __m128 kProd = _mm_mul_ps(_mm_sub_ps(verticesA[0].m_quad, verticesB[0].m_quad), normalOut->m_quad);
            const __m128 kDist =
                _mm_add_ss(_mm_shuffle_ps(kProd, kProd, 0xAA), _mm_add_ss(_mm_shuffle_ps(kProd, kProd, 0x55), kProd));
            pointOut->m_quad = result.m_closestPointA.m_quad;
            const __m128 kN = normalOut->m_quad;
            normalOut->m_quad = _mm_shuffle_ps(kN, _mm_unpackhi_ps(kN, _mm_shuffle_ps(kDist, kDist, 0)), 0xC4);
            break;
        }
        case 11:
        case 25: {
            // The triangle is B's for point-triangle (11), A's for triangle-point (25).
            const hkVector4* triangle = iCase == 11 ? verticesB : verticesA;
            const hkVector4* point = iCase == 11 ? verticesA : verticesB;
            const __m128 kT2 = triangle[2].m_quad;
            const __m128 kE1 = _mm_sub_ps(kT2, triangle[1].m_quad);
            const __m128 kE2 = _mm_sub_ps(triangle[0].m_quad, kT2);
            const __m128 kCross = _mm_sub_ps(
                _mm_mul_ps(_mm_shuffle_ps(kE2, kE2, 0xD2), _mm_shuffle_ps(kE1, kE1, 0xC9)),
                _mm_mul_ps(_mm_shuffle_ps(kE2, kE2, 0xC9), _mm_shuffle_ps(kE1, kE1, 0xD2))
            );
            const __m128 kSq = _mm_mul_ps(kCross, kCross);
            const __m128 kLen2 =
                _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq));
            const __m128 kApprox = _mm_rsqrt_ss(kLen2);
            const __m128 kIsZero = _mm_cmpeq_ss(kLen2, _mm_set_ss(0.0f));
            normalOut->m_quad = kCross;
            const __m128 kRefined = _mm_mul_ss(
                _mm_mul_ss(_mm_set_ss(0.5f), kApprox),
                _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(kLen2, kApprox), kApprox))
            );
            const __m128 kMasked = _mm_andnot_ps(kIsZero, kRefined);
            const __m128 kHintProd = _mm_mul_ps(hint->m_quad, kCross);
            const __m128 kHintDot = _mm_add_ss(
                _mm_shuffle_ps(kHintProd, kHintProd, 0xAA),
                _mm_add_ss(_mm_shuffle_ps(kHintProd, kHintProd, 0x55), kHintProd)
            );
            const __m128 kNormal = _mm_mul_ps(
                _mm_xor_ps(_mm_and_ps(_mm_shuffle_ps(kHintDot, kHintDot, 0), hkQuadRealNegateMask), kCross),
                _mm_shuffle_ps(kMasked, kMasked, 0)
            );
            normalOut->m_quad = kNormal;
            const __m128 kPoint = point[0].m_quad;
            const __m128 kProd = _mm_mul_ps(_mm_sub_ps(kPoint, triangle[0].m_quad), kNormal);
            const __m128 kDist =
                _mm_add_ss(_mm_shuffle_ps(kProd, kProd, 0xAA), _mm_add_ss(_mm_shuffle_ps(kProd, kProd, 0x55), kProd));
            pointOut->m_quad = kPoint;
            if (iCase == 11) {
                normalOut->m_quad[3] = _mm_cvtss_f32(kDist);
            }
            else {
                pointOut->m_quad = _mm_sub_ps(kPoint, _mm_mul_ps(_mm_shuffle_ps(kDist, kDist, 0), normalOut->m_quad));
                normalOut->m_quad[3] = _mm_cvtss_f32(_mm_sub_ss(hkQuadReal0000, kDist));
            }
            break;
        }
        default: break;
    }
}

// The point-triangle normal of hkGskRecalcContactInternal's case 11: the triangle a1's normal, sided against the hint
// r_esi and normalized (reciprocal square root estimate and one Newton step, 0 for a zero length), to a2; the point
// a4's distance along it to a3's x.
hkVector4* calcTriangleNormalAndDistance(
    const hkVector4* a1,
    hkVector4* a2,
    hkVector4* a3,
    const hkVector4* a4,
    const hkVector4* r_esi
) {
    const __m128 kT2 = a1[2].m_quad;
    const __m128 kE1 = _mm_sub_ps(kT2, a1[1].m_quad);
    const __m128 kE2 = _mm_sub_ps(a1[0].m_quad, kT2);
    const __m128 kCross = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(kE2, kE2, 0xD2), _mm_shuffle_ps(kE1, kE1, 0xC9)),
        _mm_mul_ps(_mm_shuffle_ps(kE2, kE2, 0xC9), _mm_shuffle_ps(kE1, kE1, 0xD2))
    );
    const __m128 kSq = _mm_mul_ps(kCross, kCross);
    const __m128 kLen2 = _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq));
    const __m128 kApprox = _mm_rsqrt_ss(kLen2);
    const __m128 kIsZero = _mm_cmpeq_ss(kLen2, _mm_set_ss(0.0f));
    a2->m_quad = kCross;
    const __m128 kRefined = _mm_mul_ss(
        _mm_mul_ss(_mm_set_ss(0.5f), kApprox),
        _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(kLen2, kApprox), kApprox))
    );
    const __m128 kMasked = _mm_andnot_ps(kIsZero, kRefined);
    const __m128 kHintProd = _mm_mul_ps(kCross, r_esi->m_quad);
    const __m128 kHintDot = _mm_add_ss(
        _mm_shuffle_ps(kHintProd, kHintProd, 0xAA), _mm_add_ss(_mm_shuffle_ps(kHintProd, kHintProd, 0x55), kHintProd)
    );
    const __m128 kNormal = _mm_mul_ps(
        _mm_xor_ps(_mm_and_ps(_mm_shuffle_ps(kHintDot, kHintDot, 0), hkQuadRealNegateMask), kCross),
        _mm_shuffle_ps(kMasked, kMasked, 0)
    );
    a2->m_quad = kNormal;
    const __m128 kProd = _mm_mul_ps(_mm_sub_ps(a4->m_quad, a1[0].m_quad), kNormal);
    a3->m_quad = _mm_add_ss(_mm_shuffle_ps(kProd, kProd, 0xAA), _mm_add_ss(_mm_shuffle_ps(kProd, kProd, 0x55), kProd));
    return a3;
}

// The edge-edge normal of hkGskRecalcContactInternal's case 18: verticesA a1, verticesB r_edi, hint a2, pointOut a3,
// normalOut r_esi.
void calcEdgeEdgeNormalAndDistance(
    const hkVector4* a1,
    const hkVector4* a2,
    hkVector4* a3,
    const hkVector4* r_edi,
    hkVector4* r_esi
) {
    hkVector4 dA;
    hkVector4 dB;
    hkLineSegmentUtil::ClosestLineSegLineSegResult result;
    dA.m_quad = _mm_sub_ps(a1[1].m_quad, a1[0].m_quad);
    dB.m_quad = _mm_sub_ps(r_edi[1].m_quad, r_edi[0].m_quad);
    hkLineSegmentUtil::closestLineSegLineSeg(a1[0], dA, r_edi[0], dB, result);
    const __m128 kCross = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(dB.m_quad, dB.m_quad, 0xD2), _mm_shuffle_ps(dA.m_quad, dA.m_quad, 0xC9)),
        _mm_mul_ps(_mm_shuffle_ps(dB.m_quad, dB.m_quad, 0xC9), _mm_shuffle_ps(dA.m_quad, dA.m_quad, 0xD2))
    );
    const __m128 kSum = _mm_add_ps(_mm_mul_ps(dB.m_quad, dB.m_quad), _mm_mul_ps(dA.m_quad, dA.m_quad));
    const float fSum =
        _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kSum, kSum, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSum, kSum, 0x55), kSum)));
    const __m128 kCrossSq = _mm_mul_ps(kCross, kCross);
    const float fCrossLen2 = _mm_cvtss_f32(_mm_add_ss(
        _mm_shuffle_ps(kCrossSq, kCrossSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kCrossSq, kCrossSq, 0x55), kCrossSq)
    ));
    r_esi->m_quad = kCross;
    __m128 kNormal;
    // Compares in double precision: the cross product's squared length times 0.001f (widened to double) against the
    // square of the edges' summed squared lengths.
    if (!(static_cast<double>(fCrossLen2) * 0.0010000000474974513 > static_cast<double>(fSum) * fSum)) {
        kNormal = a2->m_quad;
    }
    else {
        const __m128 kHintProd = _mm_mul_ps(kCross, a2->m_quad);
        const __m128 kHintDot = _mm_add_ss(
            _mm_shuffle_ps(kHintProd, kHintProd, 0xAA),
            _mm_add_ss(_mm_shuffle_ps(kHintProd, kHintProd, 0x55), kHintProd)
        );
        const __m128 kSided =
            _mm_xor_ps(_mm_and_ps(_mm_shuffle_ps(kHintDot, kHintDot, 0), hkQuadRealNegateMask), kCross);
        const __m128 kSq = _mm_mul_ps(kSided, kSided);
        const __m128 kLen2 =
            _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq));
        const __m128 kApprox = _mm_rsqrt_ss(kLen2);
        const __m128 kIsZero = _mm_cmpeq_ss(kLen2, _mm_set_ss(0.0f));
        const __m128 kRefined = _mm_mul_ss(
            _mm_mul_ss(_mm_set_ss(0.5f), kApprox),
            _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(kLen2, kApprox), kApprox))
        );
        const __m128 kMasked = _mm_andnot_ps(kIsZero, kRefined);
        kNormal = _mm_mul_ps(_mm_shuffle_ps(kMasked, kMasked, 0), kSided);
    }
    r_esi->m_quad = kNormal;
    const __m128 kProd = _mm_mul_ps(_mm_sub_ps(a1[0].m_quad, r_edi[0].m_quad), r_esi->m_quad);
    const __m128 kDist =
        _mm_add_ss(_mm_shuffle_ps(kProd, kProd, 0xAA), _mm_add_ss(_mm_shuffle_ps(kProd, kProd, 0x55), kProd));
    a3->m_quad = result.m_closestPointA.m_quad;
    const __m128 kN = r_esi->m_quad;
    r_esi->m_quad = _mm_shuffle_ps(kN, _mm_unpackhi_ps(kN, _mm_shuffle_ps(kDist, kDist, 0)), 0xC4);
}

// The tetrahedron of vertices[0..3] (r_eax) against the point (r_esi): the normals of the three faces through
// vertices[3], the point's offset from vertices[3] scaled by fSign projected on them (D), and for each face the signed
// square of that projection over the normal's squared length (FLT_MAX for a zero normal). With the three ratios sorted
// (a partial sort, as the game does: the smallest is deliberately not carried down when the third swap moves it) the
// index of the largest goes back when it is clear of the next by 10%; -1 when even the largest is under 1e-6; otherwise
// findClosestTriangleEdge decides from a copy of D. The quotient is computed in double and stored as a float.
int32_t findClosestTriangle(float fSign, hkVector4* r_eax, hkVector4* r_esi) {
    const __m128 kV1 = r_eax[1].m_quad;
    const __m128 kV0 = r_eax[0].m_quad;
    const __m128 kV3 = r_eax[3].m_quad;
    const __m128 kV2 = r_eax[2].m_quad;
    const __m128 k02 = _mm_sub_ps(kV0, kV2);
    const __m128 k30 = _mm_sub_ps(kV3, kV0);
    const __m128 k21 = _mm_sub_ps(kV2, kV1);
    const __m128 k10 = _mm_sub_ps(kV1, kV0);
    const __m128 k32 = _mm_sub_ps(kV3, kV2);
    const __m128 k31 = _mm_sub_ps(kV3, kV1);
    __m128 akNormals[3];
    akNormals[0] = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(k31, k31, 0xD2), _mm_shuffle_ps(k21, k21, 0xC9)),
        _mm_mul_ps(_mm_shuffle_ps(k31, k31, 0xC9), _mm_shuffle_ps(k21, k21, 0xD2))
    );
    akNormals[1] = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(k32, k32, 0xD2), _mm_shuffle_ps(k02, k02, 0xC9)),
        _mm_mul_ps(_mm_shuffle_ps(k32, k32, 0xC9), _mm_shuffle_ps(k02, k02, 0xD2))
    );
    akNormals[2] = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(k30, k30, 0xD2), _mm_shuffle_ps(k10, k10, 0xC9)),
        _mm_mul_ps(_mm_shuffle_ps(k30, k30, 0xC9), _mm_shuffle_ps(k10, k10, 0xD2))
    );
    const __m128 kOffset = _mm_mul_ps(_mm_set1_ps(fSign), _mm_sub_ps(r_esi->m_quad, r_eax[3].m_quad));

    // The three projections, the normals transposed into columns.
    const __m128 kLo = _mm_unpacklo_ps(akNormals[0], akNormals[1]);
    const __m128 kHi = _mm_unpackhi_ps(akNormals[0], akNormals[1]);
    const __m128 kCol0 = _mm_movelh_ps(kLo, akNormals[2]);
    const __m128 kCol1 = _mm_shuffle_ps(_mm_movehl_ps(kCol0, kLo), akNormals[2], 0xD4);
    const __m128 kCol2 = _mm_shuffle_ps(kHi, akNormals[2], 0xE4);
    const __m128 kDots = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(kOffset, kOffset, 0x55), kCol1),
            _mm_mul_ps(_mm_shuffle_ps(kOffset, kOffset, 0x00), kCol0)
        ),
        _mm_mul_ps(kCol2, _mm_shuffle_ps(kOffset, kOffset, 0xAA))
    );
    const __m128 kSignedSq = _mm_mul_ps(_mm_and_ps(kDots, hkVector4_setAbs4_absMask), kDots);

    alignas(16) float afRatio[4];
    for (int32_t i = 0; i < 3; ++i) {
        const __m128 kSq = _mm_mul_ps(akNormals[i], akNormals[i]);
        const float fLen2 =
            _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq)));
        if (fLen2 == 0.0f) {
            afRatio[i] = 3.40282e+38f;
        }
        else {
            afRatio[i] = static_cast<float>(static_cast<double>(kSignedSq[i]) / static_cast<double>(fLen2));
        }
    }

    int32_t iResult = 1;
    if (afRatio[1] < afRatio[0]) {
        iResult = 0;
        const float fTmp = afRatio[0];
        afRatio[0] = afRatio[1];
        afRatio[1] = fTmp;
    }
    if (!(afRatio[2] < afRatio[1])) {
        iResult = 2;
    }
    else {
        const float fTmp = afRatio[1];
        afRatio[1] = afRatio[2];
        afRatio[2] = fTmp;
        if (afRatio[1] < afRatio[0]) {
            afRatio[1] = afRatio[0];
        }
    }
    if (afRatio[2] < 1.0000001e-06f) {
        return -1;
    }
    if (static_cast<double>(afRatio[1]) * 1.100000023841858 < static_cast<double>(afRatio[2])) {
        return iResult;
    }
    // findClosestTriangleEdge is given a copy of the projections in the ratios' slots.
    _mm_store_ps(afRatio, kDots);
    return findClosestTriangleEdge(r_esi, afRatio, r_eax);
}

// The segment a1[0]-a1[1] against the point a3: where both ends lie on one side along the segment the simplex drops to
// one vertex (a1[1] and a1[5] moved down when the point is past a1[1]; *a2 counts down) and r_edi gets the offset
// between that vertex and the point; else r_edi gets the segment's normal towards the point, from the unrefined
// reciprocal square root estimate of its length. r_esi == 1 flips the sign. Returns the sign bit of the far end's dot
// product.
int32_t calcEdgeSearchDirection(hkVector4* a1, int32_t* a2, const hkVector4* a3, hkVector4* r_edi, int32_t r_esi) {
    const __m128 kPoint = a3->m_quad;
    const __m128 kV1 = a1[1].m_quad;
    const __m128 kV0 = a1[0].m_quad;
    const __m128 kEdge = _mm_sub_ps(kV1, kV0);
    const __m128 kToV1 = _mm_sub_ps(kV1, kPoint);
    const __m128 kProd1 = _mm_mul_ps(kToV1, kEdge);
    const __m128 kDot1 =
        _mm_add_ss(_mm_shuffle_ps(kProd1, kProd1, 0xAA), _mm_add_ss(_mm_shuffle_ps(kProd1, kProd1, 0x55), kProd1));
    const __m128 kToV0 = _mm_sub_ps(kV0, kPoint);
    const __m128 kProd0 = _mm_mul_ps(kToV0, kEdge);
    const __m128 kDot0 =
        _mm_add_ss(_mm_shuffle_ps(kProd0, kProd0, 0xAA), _mm_add_ss(_mm_shuffle_ps(kProd0, kProd0, 0x55), kProd0));
    const int32_t iSign1 = _mm_movemask_ps(kDot1) & 1;
    const int32_t iSign0 = _mm_movemask_ps(kDot0) & 1;
    if (iSign1 == iSign0) {
        if (iSign1) {
            a1[0].m_quad = a1[1].m_quad;
            a1[4].m_quad = a1[5].m_quad;
        }
        --*a2;
        const __m128 kVertex = a1[0].m_quad;
        if (r_esi == 1) {
            r_edi->m_quad = _mm_sub_ps(kPoint, kVertex);
        }
        else {
            r_edi->m_quad = _mm_sub_ps(kVertex, kPoint);
        }
        return iSign1;
    }
    const __m128 kSq = _mm_mul_ps(kEdge, kEdge);
    const __m128 kLenInv =
        _mm_rsqrt_ss(_mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq)));
    const __m128 kDir = _mm_mul_ps(_mm_shuffle_ps(kLenInv, kLenInv, 0), kEdge);
    const __m128 kCross = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(kToV0, kToV0, 0xD2), _mm_shuffle_ps(kToV1, kToV1, 0xC9)),
        _mm_mul_ps(_mm_shuffle_ps(kToV0, kToV0, 0xC9), _mm_shuffle_ps(kToV1, kToV1, 0xD2))
    );
    const __m128 kA = _mm_mul_ps(_mm_shuffle_ps(kCross, kCross, 0xC9), _mm_shuffle_ps(kDir, kDir, 0xD2));
    const __m128 kB = _mm_mul_ps(_mm_shuffle_ps(kCross, kCross, 0xD2), _mm_shuffle_ps(kDir, kDir, 0xC9));
    if (r_esi == 1) {
        r_edi->m_quad = _mm_sub_ps(kB, kA);
    }
    else {
        r_edi->m_quad = _mm_sub_ps(kA, kB);
    }
    return iSign1;
}

// GSK's main loop. B's vertices are first taken into A's space; each step then dispatches on the simplex's dimensions
// (m_dimA * 8 | m_dimB): a case reduces the simplex towards the closest features and leaves the search direction in
// m_support (an A-B vector). A new direction asks the shapes for their supporting vertices - A's first on odd passes,
// then B's, then A's on even passes - and a vertex farther along it than the limit (the squared direction length times
// an epsilon that doubles every pass) enlarges the simplex and starts a new pass. With no such vertex m_support is
// normalised and written to separatingNormalOut, the distance in w of both. A degenerate case, or a direction too short
// or not separating, is penetration: handlePenetration unless m_doNotHandlePenetration, else the dimensions are clamped
// to a vertex-triangle pair and HK_GSK_PENETRATING returned. Each d * |d| of the vertex tests is rounded to float,
// while the penetration test's d * |d| and lenSq * eps keep extended precision (both exact in double); the limit is
// stored as a float.
hkpGskStatus hkpGsk::getClosestFeature(
    const hkpConvexShape* shapeA,
    const hkpConvexShape* shapeB,
    const hkTransform& aTb,
    hkVector4& separatingNormalOut
) {
    {
        const __m128 kCol0 = aTb.m_rotation.m_col0.m_quad;
        const __m128 kCol1 = aTb.m_rotation.m_col1.m_quad;
        const __m128 kCol2 = aTb.m_rotation.m_col2.m_quad;
        const __m128 kTrans = aTb.m_translation.m_quad;
        // The loop is a do-while, so with m_dimB 0 it still transforms index -1 (m_verticesBinA[3] into m_dummy[3]), as
        // the game does.
        int32_t i = m_dimB.m_storage - 1;
        do {
            const __m128 kV = m_verticesBinB[i].m_quad;
            m_verticesBinA[i].m_quad = _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(kV, kV, 0x55), kCol1), _mm_mul_ps(_mm_shuffle_ps(kV, kV, 0x00), kCol0)
                ),
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kV, kV, 0xAA), kCol2), kTrans)
            );
            i--;
        }
        while (i >= 0);
    }

    int32_t iPass = 0;
    float fEpsilon = 1e-10f;
    int32_t iDimSum;
    bool bDegenerate;
    float fLimit;
    hkpGsk::NextCase eNext;
    int32_t iRemoved;

newPass:
    iPass++;
    fEpsilon = fEpsilon + fEpsilon;
    iDimSum = m_dimB.m_storage + m_dimA.m_storage;
    m_supportState = hkpGsk::SUPPORT_CHANGED;
nextCase:
    switch (m_dimA.m_storage * 8 | m_dimB.m_storage) {
        case GSK_POINT_POINT: goto pointPoint;
        case GSK_POINT_EDGE: goto pointEdge;
        case GSK_POINT_TRIANGLE: goto pointTriangle;
        case GSK_POINT_TETRA:
            iRemoved = findClosestTriangle(1.0f, m_verticesBinA, m_verticesA);
            if (iRemoved < 0) {
                goto degenerate;
            }
            m_dimB.m_storage--;
            m_verticesBinA[iRemoved] = m_verticesBinA[3];
            m_verticesBinB[iRemoved] = m_verticesBinB[3];
            goto pointTriangle;
        case GSK_EDGE_POINT: goto edgePoint;
        case GSK_EDGE_EDGE: goto edgeEdge;
        case GSK_EDGE_TRIANGLE:
            eNext = processEdgeTriangle(
                m_verticesA, m_verticesBinA, m_dimA, m_dimB, hkBool(m_lastDimB.m_storage == 2),
                hkpGsk::BUILD_POS_SUPPORT
            );
            goto edgeTriangleDone;
        case GSK_TRIANGLE_POINT: goto trianglePoint;
        case GSK_TRIANGLE_EDGE:
            eNext = processEdgeTriangle(
                m_verticesBinA, m_verticesA, m_dimB, m_dimA, hkBool(m_lastDimB.m_storage == 2),
                hkpGsk::BUILD_NEG_SUPPORT
            );
            goto edgeTriangleDone;
        case GSK_TETRA_POINT:
            iRemoved = findClosestTriangle(-1.0f, m_verticesA, m_verticesBinA);
            if (iRemoved < 0) {
                goto degenerate;
            }
            m_dimA.m_storage--;
            m_verticesA[iRemoved] = m_verticesA[3];
            goto trianglePoint;
        default: goto degenerate;
    }

edgeTriangleDone:
    if (eNext == hkpGsk::NEXT_IS_PENETRATION) {
        goto degenerate;
    }
    if (eNext != hkpGsk::NEXT_IS_EDGE_EDGE) {
        goto keepSupport;
    }
edgeEdge:
    {
        alignas(16) hkVector4 dA;
        alignas(16) hkVector4 dB;
        dA.setSub4(m_verticesA[1], m_verticesA[0]);
        dB.setSub4(m_verticesBinA[1], m_verticesBinA[0]);
        const int32_t iResult = hkLineSegmentUtil::closestLineSegLineSeg(
            m_verticesA[0], dA, m_verticesBinA[0], dB, m_closestLineLineResult
        );
        if (iResult == 0) {
            // The closest points lie inside both edges: the edges' normal, turned to point from B to A.
            m_support.m_quad = _mm_sub_ps(
                _mm_mul_ps(_mm_shuffle_ps(dB.m_quad, dB.m_quad, 0xD2), _mm_shuffle_ps(dA.m_quad, dA.m_quad, 0xC9)),
                _mm_mul_ps(_mm_shuffle_ps(dB.m_quad, dB.m_quad, 0xC9), _mm_shuffle_ps(dA.m_quad, dA.m_quad, 0xD2))
            );
            hkVector4 ab;
            ab.setSub4(m_verticesA[0], m_verticesBinA[0]);
            const __m128 kSide = ab.dot3(m_support).getQuad();
            m_support.m_quad =
                _mm_xor_ps(_mm_and_ps(_mm_shuffle_ps(kSide, kSide, 0x00), hkQuadRealNegateMask), m_support.m_quad);
            goto keepSupport;
        }
        if (iResult & 1) {
            m_verticesA[0] = m_verticesA[1];
            m_dimA.m_storage = 1;
        }
        else if (iResult & 2) {
            m_dimA.m_storage = 1;
        }
        if (iResult & 4) {
            m_verticesBinA[0] = m_verticesBinA[1];
            m_verticesBinB[0] = m_verticesBinB[1];
            m_dimB.m_storage = 1;
        }
        else if (iResult & 8) {
            m_dimB.m_storage = 1;
        }
    }
    goto nextCase;

pointTriangle:
    {
        const int32_t iMask = checkTriangleBoundaries(m_verticesA[0], m_verticesBinA, hkpGsk::BUILD_POS_SUPPORT);
        if (iMask == 7) {
            goto keepSupport;
        }
        const int32_t iVertex = rdata_10CB438[iMask];
        if (iVertex < 0) {
            int32_t iEdge = iVertex + 8;
            if (iEdge > 3) {
                iEdge = 1;
            }
            m_dimB.m_storage--;
            m_verticesBinA[iEdge] = m_verticesBinA[2];
            m_verticesBinB[iEdge] = m_verticesBinB[2];
            goto pointEdge;
        }
        const int32_t iPrev = hkpCollideTriangleUtil::vertexToEdgeLut[iVertex];
        int32_t iNext = hkpCollideTriangleUtil::vertexToEdgeLut[iVertex + 2];
        hkVector4 toA;
        toA.setSub4(m_verticesA[0], m_verticesBinA[iVertex]);
        hkVector4 prevEdge;
        prevEdge.setSub4(m_verticesBinA[iPrev], m_verticesBinA[iVertex]);
        hkVector4 nextEdge;
        nextEdge.setSub4(m_verticesBinA[iNext], m_verticesBinA[iVertex]);
        const int32_t iPrevSign = _mm_movemask_ps(prevEdge.dot3(toA).getQuad()) & 1;
        m_dimB.m_storage = 2;
        if (iPrevSign) {
            m_verticesBinA[iPrev] = m_verticesBinA[2];
            m_verticesBinB[iPrev] = m_verticesBinB[2];
            if (_mm_movemask_ps(nextEdge.dot3(toA).getQuad()) & 1) {
                if (iNext == m_dimB.m_storage) {
                    iNext = iPrev;
                }
                m_dimB.m_storage--;
                m_verticesBinA[iNext] = m_verticesBinA[m_dimB.m_storage];
                m_verticesBinB[iNext] = m_verticesBinB[m_dimB.m_storage];
            }
        }
        else {
            m_verticesBinA[iNext] = m_verticesBinA[2];
            m_verticesBinB[iNext] = m_verticesBinB[2];
        }
    }
    goto nextCase;

trianglePoint:
    {
        const int32_t iMask = checkTriangleBoundaries(m_verticesBinA[0], m_verticesA, hkpGsk::BUILD_NEG_SUPPORT);
        if (iMask == 7) {
            goto keepSupport;
        }
        const int32_t iVertex = rdata_10CB438[iMask];
        if (iVertex < 0) {
            int32_t iEdge = iVertex + 8;
            if (iEdge > 3) {
                iEdge = 1;
            }
            m_dimA.m_storage--;
            m_verticesA[iEdge] = m_verticesA[2];
            goto edgePoint;
        }
        const int32_t iPrev = hkpCollideTriangleUtil::vertexToEdgeLut[iVertex];
        int32_t iNext = hkpCollideTriangleUtil::vertexToEdgeLut[iVertex + 2];
        hkVector4 toB;
        toB.setSub4(m_verticesBinA[0], m_verticesA[iVertex]);
        hkVector4 prevEdge;
        prevEdge.setSub4(m_verticesA[iPrev], m_verticesA[iVertex]);
        hkVector4 nextEdge;
        nextEdge.setSub4(m_verticesA[iNext], m_verticesA[iVertex]);
        const int32_t iPrevSign = _mm_movemask_ps(prevEdge.dot3(toB).getQuad()) & 1;
        m_dimA.m_storage = 2;
        if (iPrevSign) {
            m_verticesA[iPrev] = m_verticesA[2];
            if (_mm_movemask_ps(nextEdge.dot3(toB).getQuad()) & 1) {
                if (iNext == m_dimA.m_storage) {
                    iNext = iPrev;
                }
                m_dimA.m_storage--;
                m_verticesA[iNext] = m_verticesA[m_dimA.m_storage];
            }
        }
        else {
            m_verticesA[iNext] = m_verticesA[2];
        }
    }
    goto nextCase;

pointEdge:
    {
        hkVector4 edge;
        edge.setSub4(m_verticesBinA[1], m_verticesBinA[0]);
        hkVector4 toB0;
        toB0.setSub4(m_verticesBinA[0], m_verticesA[0]);
        hkVector4 toB1;
        toB1.setSub4(m_verticesBinA[1], m_verticesA[0]);
        const int32_t iSign1 = _mm_movemask_ps(toB1.dot3(edge).getQuad()) & 1;
        const int32_t iSign0 = _mm_movemask_ps(toB0.dot3(edge).getQuad()) & 1;
        if (iSign1 == iSign0) {
            // A's point lies beyond one end of the edge: keep that end.
            if (iSign1) {
                m_verticesBinA[0] = m_verticesBinA[1];
                m_verticesBinB[0] = m_verticesBinB[1];
            }
            m_dimB.m_storage--;
            goto pointPoint;
        }
        // The approximate reciprocal square root is used alone, with no Newton step, as the game does.
        const __m128 kLenInv = _mm_rsqrt_ss(edge.dot3(edge).getQuad());
        const __m128 kEdgeN = _mm_mul_ps(_mm_shuffle_ps(kLenInv, kLenInv, 0x00), edge.m_quad);
        const __m128 kNormal = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(toB1.m_quad, toB1.m_quad, 0xC9), _mm_shuffle_ps(toB0.m_quad, toB0.m_quad, 0xD2)),
            _mm_mul_ps(_mm_shuffle_ps(toB1.m_quad, toB1.m_quad, 0xD2), _mm_shuffle_ps(toB0.m_quad, toB0.m_quad, 0xC9))
        );
        m_support.m_quad = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(kEdgeN, kEdgeN, 0xC9), _mm_shuffle_ps(kNormal, kNormal, 0xD2)),
            _mm_mul_ps(_mm_shuffle_ps(kEdgeN, kEdgeN, 0xD2), _mm_shuffle_ps(kNormal, kNormal, 0xC9))
        );
    }
    goto keepSupport;

edgePoint:
    {
        hkVector4 edge;
        edge.setSub4(m_verticesA[1], m_verticesA[0]);
        hkVector4 fromB0;
        fromB0.setSub4(m_verticesA[0], m_verticesBinA[0]);
        hkVector4 fromB1;
        fromB1.setSub4(m_verticesA[1], m_verticesBinA[0]);
        const int32_t iSign1 = _mm_movemask_ps(fromB1.dot3(edge).getQuad()) & 1;
        const int32_t iSign0 = _mm_movemask_ps(fromB0.dot3(edge).getQuad()) & 1;
        if (iSign1 == iSign0) {
            // B's point lies beyond one end of the edge: keep that end. m_dummy's first two entries are swapped along
            // with it, as m_verticesBinB's are for B's edge.
            if (iSign1) {
                m_verticesA[0] = m_verticesA[1];
                m_dummy[0] = m_dummy[1];
            }
            m_dimA.m_storage--;
            goto pointPoint;
        }
        // The approximate reciprocal square root is used alone, with no Newton step, as the game does.
        const __m128 kLenInv = _mm_rsqrt_ss(edge.dot3(edge).getQuad());
        const __m128 kEdgeN = _mm_mul_ps(_mm_shuffle_ps(kLenInv, kLenInv, 0x00), edge.m_quad);
        const __m128 kNormal = _mm_sub_ps(
            _mm_mul_ps(
                _mm_shuffle_ps(fromB0.m_quad, fromB0.m_quad, 0xD2), _mm_shuffle_ps(fromB1.m_quad, fromB1.m_quad, 0xC9)
            ),
            _mm_mul_ps(
                _mm_shuffle_ps(fromB0.m_quad, fromB0.m_quad, 0xC9), _mm_shuffle_ps(fromB1.m_quad, fromB1.m_quad, 0xD2)
            )
        );
        m_support.m_quad = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(kNormal, kNormal, 0xC9), _mm_shuffle_ps(kEdgeN, kEdgeN, 0xD2)),
            _mm_mul_ps(_mm_shuffle_ps(kNormal, kNormal, 0xD2), _mm_shuffle_ps(kEdgeN, kEdgeN, 0xC9))
        );
    }
    goto keepSupport;

pointPoint:
    m_support.setSub4(m_verticesA[0], m_verticesBinA[0]);
keepSupport:
    m_lastDimB.m_storage = m_dimB.m_storage;
    bDegenerate = false;
    goto testSupport;
degenerate:
    bDegenerate = true;
testSupport:
    {
        const float fLenSq = _mm_cvtss_f32(m_support.dot3(m_support).getQuad());
        hkVector4 ab;
        ab.setSub4(m_verticesA[0], m_verticesBinA[0]);
        m_featureChange.m_storage |= m_dimA.m_storage - iDimSum + m_dimB.m_storage;
        const float fDist = _mm_cvtss_f32(ab.dot3(m_support).getQuad());
        if (bDegenerate) {
            goto penetrating;
        }
        const double dLimit = static_cast<double>(fLenSq) * fEpsilon;
        const float fAbsDist = std::fabs(fDist);
        if (static_cast<double>(fDist) * fAbsDist <= dLimit || fEpsilon > fLenSq) {
            goto penetrating;
        }
        fLimit = static_cast<float>(dLimit);
    }

    // A's supporting vertex first on odd passes.
    if ((iPass & 1) != 0 && m_dimA.m_storage != m_maxDimA.m_storage &&
        m_supportState != hkpGsk::DONT_CALL_GET_SUPPORTING_VERTEX_A) {
        alignas(16) hkVector4 direction;
        direction.m_quad = _mm_xor_ps(m_support.m_quad, hkQuadRealNegateMask);
        shapeA->getSupportingVertexImpl(direction, m_verticesA[m_dimA.m_storage]);
        const int32_t iDim = m_dimA.m_storage;
        hkVector4 diff;
        diff.setSub4(m_verticesA[iDim], m_verticesA[0]);
        const float fDot0 = _mm_cvtss_f32(diff.dot3(direction).getQuad());
        const float fProd0 = fDot0 * std::fabs(fDot0);
        if (fLimit < fProd0) {
            bool bGrow = true;
            if (iDim == 3) {
                diff.setSub4(m_verticesA[iDim], m_verticesA[2]);
                const float fDot2 = _mm_cvtss_f32(diff.dot3(direction).getQuad());
                const float fProd2 = fDot2 * std::fabs(fDot2);
                if (fProd2 < fLimit) {
                    bGrow = false;
                }
            }
            if (bGrow && (iDim == 2 || iDim == 3)) {
                diff.setSub4(m_verticesA[iDim], m_verticesA[1]);
                const float fDot1 = _mm_cvtss_f32(diff.dot3(direction).getQuad());
                const float fProd1 = fDot1 * std::fabs(fDot1);
                if (fProd1 < fLimit) {
                    bGrow = false;
                }
            }
            if (bGrow) {
                m_dimA.m_storage = iDim + 1;
                m_featureChange.m_storage = 1;
                goto newPass;
            }
        }
    }

    if (m_dimB.m_storage != m_maxDimB.m_storage && m_supportState != hkpGsk::DONT_CALL_GET_SUPPORTING_VERTEX_B) {
        // m_support turned into B's space: the rotation's transpose, built with shuffles.
        const __m128 kCol0 = aTb.m_rotation.m_col0.m_quad;
        const __m128 kCol1 = aTb.m_rotation.m_col1.m_quad;
        const __m128 kCol2 = aTb.m_rotation.m_col2.m_quad;
        const __m128 kLo = _mm_unpacklo_ps(kCol0, kCol1);
        const __m128 kRow0 = _mm_movelh_ps(kLo, kCol2);
        const __m128 kRow1 = _mm_shuffle_ps(_mm_movehl_ps(kRow0, kLo), kCol2, 0xD4);
        const __m128 kRow2 = _mm_shuffle_ps(_mm_unpackhi_ps(kCol0, kCol1), kCol2, 0xE4);
        const __m128 kS = m_support.m_quad;
        alignas(16) hkVector4 direction;
        direction.m_quad = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(kS, kS, 0x55), kRow1), _mm_mul_ps(_mm_shuffle_ps(kS, kS, 0x00), kRow0)
            ),
            _mm_mul_ps(kRow2, _mm_shuffle_ps(kS, kS, 0xAA))
        );
        shapeB->getSupportingVertexImpl(direction, m_verticesBinB[m_dimB.m_storage]);
        const int32_t iDim = m_dimB.m_storage;
        hkVector4 diff;
        diff.setSub4(m_verticesBinB[iDim], m_verticesBinB[0]);
        const float fDot0 = _mm_cvtss_f32(diff.dot3(direction).getQuad());
        const float fProd0 = fDot0 * std::fabs(fDot0);
        if (fLimit < fProd0) {
            bool bGrow = true;
            if (iDim == 3) {
                diff.setSub4(m_verticesBinB[iDim], m_verticesBinB[2]);
                const float fDot2 = _mm_cvtss_f32(diff.dot3(direction).getQuad());
                const float fProd2 = fDot2 * std::fabs(fDot2);
                if (fProd2 < fLimit) {
                    bGrow = false;
                }
            }
            if (bGrow && (iDim == 2 || iDim == 3)) {
                diff.setSub4(m_verticesBinB[iDim], m_verticesBinB[1]);
                const float fDot1 = _mm_cvtss_f32(diff.dot3(direction).getQuad());
                const float fProd1 = fDot1 * std::fabs(fDot1);
                if (fProd1 < fLimit) {
                    bGrow = false;
                }
            }
            if (bGrow) {
                // The new vertex into A's space.
                const __m128 kV = m_verticesBinB[iDim].m_quad;
                m_verticesBinA[iDim].m_quad = _mm_add_ps(
                    _mm_add_ps(
                        _mm_mul_ps(_mm_shuffle_ps(kV, kV, 0x55), aTb.m_rotation.m_col1.m_quad),
                        _mm_mul_ps(_mm_shuffle_ps(kV, kV, 0x00), aTb.m_rotation.m_col0.m_quad)
                    ),
                    _mm_add_ps(
                        _mm_mul_ps(_mm_shuffle_ps(kV, kV, 0xAA), aTb.m_rotation.m_col2.m_quad), aTb.m_translation.m_quad
                    )
                );
                m_dimB.m_storage++;
                m_featureChange.m_storage = 1;
                goto newPass;
            }
        }
    }

    // A's supporting vertex last on even passes.
    if ((iPass & 1) == 0 && m_dimA.m_storage != m_maxDimA.m_storage &&
        m_supportState != hkpGsk::DONT_CALL_GET_SUPPORTING_VERTEX_A) {
        alignas(16) hkVector4 direction;
        direction.m_quad = _mm_xor_ps(m_support.m_quad, hkQuadRealNegateMask);
        shapeA->getSupportingVertexImpl(direction, m_verticesA[m_dimA.m_storage]);
        const int32_t iDim = m_dimA.m_storage;
        hkVector4 diff;
        diff.setSub4(m_verticesA[iDim], m_verticesA[0]);
        const float fDot0 = _mm_cvtss_f32(diff.dot3(direction).getQuad());
        const float fProd0 = fDot0 * std::fabs(fDot0);
        if (fLimit < fProd0) {
            bool bGrow = true;
            if (iDim == 3) {
                diff.setSub4(m_verticesA[iDim], m_verticesA[2]);
                const float fDot2 = _mm_cvtss_f32(diff.dot3(direction).getQuad());
                const float fProd2 = fDot2 * std::fabs(fDot2);
                if (fProd2 < fLimit) {
                    bGrow = false;
                }
            }
            if (bGrow && (iDim == 2 || iDim == 3)) {
                diff.setSub4(m_verticesA[iDim], m_verticesA[1]);
                const float fDot1 = _mm_cvtss_f32(diff.dot3(direction).getQuad());
                const float fProd1 = fDot1 * std::fabs(fDot1);
                if (fProd1 < fLimit) {
                    bGrow = false;
                }
            }
            if (bGrow) {
                m_dimA.m_storage = iDim + 1;
                m_featureChange.m_storage = 1;
                goto newPass;
            }
        }
    }
    goto separated;

penetrating:
    if (m_doNotHandlePenetration.m_storage == 0) {
        handlePenetration(shapeA, shapeB, aTb);
        goto separated;
    }
    if (m_dimA.m_storage + m_dimB.m_storage > 4) {
        if (m_dimA.m_storage > m_dimB.m_storage) {
            m_dimA.m_storage = 3;
            m_dimB.m_storage = 1;
            return HK_GSK_PENETRATING;
        }
        m_dimA.m_storage = 1;
        m_dimB.m_storage = 3;
    }
    return HK_GSK_PENETRATING;

separated:
    {
        hkVector4 ab;
        ab.setSub4(m_verticesA[0], m_verticesBinA[0]);
        m_support.normalize3();
        separatingNormalOut = m_support;
        const float fDistance = _mm_cvtss_f32(m_support.dot3(ab).getQuad());
        separatingNormalOut.m_quad[3] = fDistance;
        m_support.m_quad[3] = fDistance;
    }
    return HK_GSK_OK;
}

// Each side's vertex ids go to the cache in descending order of the first two, the third placed by the swaps a sort
// network of two compares makes.
void hkpGsk::exitAndExportCacheImpl(hkpGskCache& cache) const {
    cache.m_dimA = m_dimA.m_storage;
    cache.m_dimB = m_dimB.m_storage;
    const int32_t iDimA = m_dimA.m_storage;
    uint16_t usA0 = m_verticesA[0].getId();
    if (iDimA >= 2) {
        uint16_t usA1 = m_verticesA[1].getId();
        int32_t iSwaps = 0;
        if (usA1 > usA0) {
            usA1 = m_verticesA[0].getId();
            usA0 = m_verticesA[1].getId();
            iSwaps = 1;
        }
        if (iDimA > 2) {
            uint16_t usA2 = m_verticesA[2].getId();
            if (usA2 > usA0) {
                usA2 = usA0;
                usA0 = m_verticesA[2].getId();
                ++iSwaps;
            }
            if (iSwaps == 1) {
                const uint16_t usTmp = usA2;
                usA2 = usA1;
                usA1 = usTmp;
            }
            cache.m_vertices[2] = usA2;
        }
        cache.m_vertices[1] = usA1;
    }
    cache.m_vertices[0] = usA0;
    const int32_t iDimB = m_dimB.m_storage;
    uint16_t usB0 = m_verticesBinB[0].getId();
    if (iDimB < 2) {
        cache.m_vertices[m_dimA.m_storage] = usB0;
    }
    else {
        uint16_t usB1 = m_verticesBinB[1].getId();
        int32_t iSwaps = 0;
        if (usB1 > usB0) {
            usB1 = m_verticesBinB[0].getId();
            usB0 = m_verticesBinB[1].getId();
            iSwaps = 1;
        }
        if (iDimB > 2) {
            uint16_t usB2 = m_verticesBinB[2].getId();
            if (usB2 > usB0) {
                usB2 = usB0;
                usB0 = m_verticesBinB[2].getId();
                ++iSwaps;
            }
            if (iSwaps == 1) {
                const uint16_t usTmp = usB2;
                usB2 = usB1;
                usB1 = usTmp;
            }
            cache.m_vertices[m_dimA.m_storage + 2] = usB2;
        }
        cache.m_vertices[m_dimA.m_storage + 1] = usB1;
        cache.m_vertices[m_dimA.m_storage] = usB0;
    }
}

// The ray from input.m_from to input.m_to against shapeA, or against the difference of shapeA and shapeB (B turned into
// A's space by aRb) when shapeB is given, both grown by their radii: conservative advancement. The simplex's A side
// holds the difference's supporting vertices, its B side the ray's current point. Each pass takes the difference's
// supporting vertex along the search direction; while the ray's point lies outside the plane through it (moved out by
// the radius) the ray advances to that plane, and fails when it does not approach it or the fraction reaches one. GSK's
// case step then gives the next direction. The loop ends when the closest vertex distance falls within the radius (plus
// 0.001) or the direction stops changing on three passes; a fraction below output's is a hit, written with the plane's
// normal. The fraction is stored as a float.
hkBool hkpGsk::linearCast(
    const hkpConvexShape* shapeA,
    const hkpConvexShape* shapeB,
    const hkRotation& aRb,
    const hkpShapeRayCastInput& input,
    hkpShapeRayCastOutput& output
) {
    hkVector4 rayDir;
    rayDir.setSub4(input.m_to, input.m_from);
    float fRadius;
    if (shapeB) {
        const float fRadiusA = shapeA->m_radius;
        fRadius = static_cast<float>(static_cast<double>(shapeB->m_radius) + fRadiusA);
    }
    else {
        fRadius = shapeA->m_radius;
    }
    hkVector4 position = input.m_from;
    const double dRadiusEps = static_cast<double>(fRadius) + 0.0010000000474974513;
    const float fRadiusSq = static_cast<float>(dRadiusEps * dRadiusEps);
    float fLambda = 0.0f;
    hkVector4 normal;
    normal.m_quad = _mm_setzero_ps();
    uint16_t aVertexIds[2] = {0, 0};
    alignas(16) hkpCdVertex vertex;
    shapeA->convertVertexIdsToVerticesImpl(aVertexIds, 1, &vertex);
    float fMinDistance = 1e7f;
    m_support.setSub4(position, vertex);
    hkVector4 dir = m_support;
    dir.normalize3();
    m_dimA.m_storage = 0;
    m_dimB.m_storage = 1;
    alignas(16) hkpCdVertex point;
    point.m_quad = normal.m_quad;
    alignas(16) hkpCdVertex pointB;
    pointB.m_quad = normal.m_quad;
    int32_t iStalls = 0;
    const double dRadiusSq = fRadiusSq;
    hkVector4 lastSupport;
    hkpGsk::NextCase eNext;
    int32_t iRemoved;
    if (!(dRadiusSq < 100000000000000.0)) {
        goto done;
    }

nextPass:
    shapeA->getSupportingVertexImpl(m_support, point);
    if (shapeB) {
        // -m_support turned into B's space with the rotation's transpose.
        const __m128 kCol0 = aRb.m_col0.m_quad;
        const __m128 kCol1 = aRb.m_col1.m_quad;
        const __m128 kCol2 = aRb.m_col2.m_quad;
        const __m128 kS = _mm_xor_ps(hkQuadRealNegateMask, m_support.m_quad);
        const __m128 kLo = _mm_unpacklo_ps(kCol0, kCol1);
        const __m128 kRow0 = _mm_movelh_ps(kLo, kCol2);
        const __m128 kRow1 = _mm_shuffle_ps(_mm_movehl_ps(kRow0, kLo), kCol2, 0xD4);
        const __m128 kRow2 = _mm_shuffle_ps(_mm_unpackhi_ps(kCol0, kCol1), kCol2, 0xE4);
        alignas(16) hkVector4 direction;
        direction.m_quad = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(kS, kS, 0x55), kRow1), _mm_mul_ps(_mm_shuffle_ps(kS, kS, 0x00), kRow0)
            ),
            _mm_mul_ps(_mm_shuffle_ps(kS, kS, 0xAA), kRow2)
        );
        shapeB->getSupportingVertexImpl(direction, pointB);
        alignas(16) hkVector4 pointBinA;
        pointBinA.setRotatedDir(aRb, pointB);
        point.m_quad = _mm_sub_ps(point.m_quad, pointBinA.m_quad);
    }
    {
        // The difference's vertex joins the simplex unless it holds it already.
        const int32_t iDim = m_dimA.m_storage;
        for (int32_t i = 0; i < m_dimA.m_storage; i++) {
            if (point.m_quad[0] == m_verticesA[i].m_quad[0] && point.m_quad[1] == m_verticesA[i].m_quad[1] &&
                point.m_quad[2] == m_verticesA[i].m_quad[2]) {
                goto known;
            }
        }
        m_verticesA[iDim].m_quad = point.m_quad;
        m_dimA.m_storage++;
    }
known:
    {
        hkVector4 surface;
        surface.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(fRadius), dir.m_quad), point.m_quad);
        hkVector4 toPosition;
        toPosition.setSub4(position, surface);
        const float fDist = _mm_cvtss_f32(m_support.dot3(toPosition).getQuad());
        if (fDist > 0.0f) {
            const float fApproach = _mm_cvtss_f32(m_support.dot3(rayDir).getQuad());
            if (fApproach >= 0.0f) {
                return 0;
            }
            fLambda = static_cast<float>(fLambda - static_cast<double>(fDist) / fApproach);
            if (fLambda >= 1.0f) {
                return 0;
            }
            position.m_quad = _mm_add_ps(input.m_from.m_quad, _mm_mul_ps(_mm_set1_ps(fLambda), rayDir.m_quad));
            normal = dir;
        }
    }
    lastSupport = m_support;
    m_verticesBinB[0].m_quad = position.m_quad;
    m_verticesBinA[0].m_quad = m_verticesBinB[0].m_quad;
    m_supportState = hkpGsk::SUPPORT_CHANGED;

nextCase:
    switch (m_dimA.m_storage * 8 | m_dimB.m_storage) {
        case GSK_POINT_POINT: goto pointPoint;
        case GSK_POINT_EDGE: goto pointEdge;
        case GSK_POINT_TRIANGLE: goto pointTriangle;
        case GSK_POINT_TETRA:
            iRemoved = findClosestTriangle(1.0f, m_verticesBinA, m_verticesA);
            if (iRemoved < 0) {
                goto done;
            }
            m_dimB.m_storage--;
            m_verticesBinA[iRemoved] = m_verticesBinA[3];
            m_verticesBinB[iRemoved] = m_verticesBinB[3];
            goto pointTriangle;
        case GSK_EDGE_POINT: goto edgePoint;
        case GSK_EDGE_EDGE: goto edgeEdge;
        case GSK_EDGE_TRIANGLE:
            eNext = processEdgeTriangle(
                m_verticesA, m_verticesBinA, m_dimA, m_dimB, hkBool(m_lastDimB.m_storage == 2),
                hkpGsk::BUILD_POS_SUPPORT
            );
            goto edgeTriangleDone;
        case GSK_TRIANGLE_POINT: goto trianglePoint;
        case GSK_TRIANGLE_EDGE:
            eNext = processEdgeTriangle(
                m_verticesBinA, m_verticesA, m_dimB, m_dimA, hkBool(m_lastDimB.m_storage == 2),
                hkpGsk::BUILD_NEG_SUPPORT
            );
            goto edgeTriangleDone;
        case GSK_TETRA_POINT:
            iRemoved = findClosestTriangle(-1.0f, m_verticesA, m_verticesBinA);
            if (iRemoved < 0) {
                goto done;
            }
            m_dimA.m_storage--;
            m_verticesA[iRemoved] = m_verticesA[3];
            goto trianglePoint;
        default: goto done;
    }

edgeTriangleDone:
    if (eNext == hkpGsk::NEXT_IS_PENETRATION) {
        goto done;
    }
    if (eNext != hkpGsk::NEXT_IS_EDGE_EDGE) {
        goto keepSupport;
    }
edgeEdge:
    {
        alignas(16) hkVector4 dA;
        alignas(16) hkVector4 dB;
        dA.setSub4(m_verticesA[1], m_verticesA[0]);
        dB.setSub4(m_verticesBinA[1], m_verticesBinA[0]);
        const int32_t iResult = hkLineSegmentUtil::closestLineSegLineSeg(
            m_verticesA[0], dA, m_verticesBinA[0], dB, m_closestLineLineResult
        );
        if (iResult == 0) {
            // The closest points lie inside both edges: the edges' normal, turned to point from B to A.
            m_support.m_quad = _mm_sub_ps(
                _mm_mul_ps(_mm_shuffle_ps(dB.m_quad, dB.m_quad, 0xD2), _mm_shuffle_ps(dA.m_quad, dA.m_quad, 0xC9)),
                _mm_mul_ps(_mm_shuffle_ps(dB.m_quad, dB.m_quad, 0xC9), _mm_shuffle_ps(dA.m_quad, dA.m_quad, 0xD2))
            );
            hkVector4 ab;
            ab.setSub4(m_verticesA[0], m_verticesBinA[0]);
            const __m128 kSide = ab.dot3(m_support).getQuad();
            m_support.m_quad =
                _mm_xor_ps(_mm_and_ps(_mm_shuffle_ps(kSide, kSide, 0x00), hkQuadRealNegateMask), m_support.m_quad);
            goto keepSupport;
        }
        if (iResult & 1) {
            m_verticesA[0] = m_verticesA[1];
            m_dimA.m_storage = 1;
        }
        else if (iResult & 2) {
            m_dimA.m_storage = 1;
        }
        if (iResult & 4) {
            m_verticesBinA[0] = m_verticesBinA[1];
            m_verticesBinB[0] = m_verticesBinB[1];
            m_dimB.m_storage = 1;
        }
        else if (iResult & 8) {
            m_dimB.m_storage = 1;
        }
    }
    goto nextCase;

pointTriangle:
    {
        const int32_t iMask = checkTriangleBoundaries(m_verticesA[0], m_verticesBinA, hkpGsk::BUILD_POS_SUPPORT);
        if (iMask == 7) {
            goto keepSupport;
        }
        const int32_t iVertex = rdata_10CB438[iMask];
        if (iVertex < 0) {
            int32_t iEdge = iVertex + 8;
            if (iEdge > 3) {
                iEdge = 1;
            }
            m_dimB.m_storage--;
            m_verticesBinA[iEdge] = m_verticesBinA[2];
            m_verticesBinB[iEdge] = m_verticesBinB[2];
            goto pointEdge;
        }
        const int32_t iPrev = hkpCollideTriangleUtil::vertexToEdgeLut[iVertex];
        int32_t iNext = hkpCollideTriangleUtil::vertexToEdgeLut[iVertex + 2];
        hkVector4 toA;
        toA.setSub4(m_verticesA[0], m_verticesBinA[iVertex]);
        hkVector4 prevEdge;
        prevEdge.setSub4(m_verticesBinA[iPrev], m_verticesBinA[iVertex]);
        hkVector4 nextEdge;
        nextEdge.setSub4(m_verticesBinA[iNext], m_verticesBinA[iVertex]);
        const int32_t iPrevSign = _mm_movemask_ps(prevEdge.dot3(toA).getQuad()) & 1;
        m_dimB.m_storage = 2;
        if (iPrevSign) {
            m_verticesBinA[iPrev] = m_verticesBinA[2];
            m_verticesBinB[iPrev] = m_verticesBinB[2];
            if (_mm_movemask_ps(nextEdge.dot3(toA).getQuad()) & 1) {
                if (iNext == m_dimB.m_storage) {
                    iNext = iPrev;
                }
                m_dimB.m_storage--;
                m_verticesBinA[iNext] = m_verticesBinA[m_dimB.m_storage];
                m_verticesBinB[iNext] = m_verticesBinB[m_dimB.m_storage];
            }
        }
        else {
            m_verticesBinA[iNext] = m_verticesBinA[2];
            m_verticesBinB[iNext] = m_verticesBinB[2];
        }
    }
    goto nextCase;

trianglePoint:
    {
        const int32_t iMask = checkTriangleBoundaries(m_verticesBinA[0], m_verticesA, hkpGsk::BUILD_NEG_SUPPORT);
        if (iMask == 7) {
            goto keepSupport;
        }
        const int32_t iVertex = rdata_10CB438[iMask];
        if (iVertex < 0) {
            int32_t iEdge = iVertex + 8;
            if (iEdge > 3) {
                iEdge = 1;
            }
            m_dimA.m_storage--;
            m_verticesA[iEdge] = m_verticesA[2];
            goto edgePoint;
        }
        const int32_t iPrev = hkpCollideTriangleUtil::vertexToEdgeLut[iVertex];
        int32_t iNext = hkpCollideTriangleUtil::vertexToEdgeLut[iVertex + 2];
        hkVector4 toB;
        toB.setSub4(m_verticesBinA[0], m_verticesA[iVertex]);
        hkVector4 prevEdge;
        prevEdge.setSub4(m_verticesA[iPrev], m_verticesA[iVertex]);
        hkVector4 nextEdge;
        nextEdge.setSub4(m_verticesA[iNext], m_verticesA[iVertex]);
        const int32_t iPrevSign = _mm_movemask_ps(prevEdge.dot3(toB).getQuad()) & 1;
        m_dimA.m_storage = 2;
        if (iPrevSign) {
            m_verticesA[iPrev] = m_verticesA[2];
            if (_mm_movemask_ps(nextEdge.dot3(toB).getQuad()) & 1) {
                if (iNext == m_dimA.m_storage) {
                    iNext = iPrev;
                }
                m_dimA.m_storage--;
                m_verticesA[iNext] = m_verticesA[m_dimA.m_storage];
            }
        }
        else {
            m_verticesA[iNext] = m_verticesA[2];
        }
    }
    goto nextCase;

pointEdge:
    {
        hkVector4 edge;
        edge.setSub4(m_verticesBinA[1], m_verticesBinA[0]);
        hkVector4 toB0;
        toB0.setSub4(m_verticesBinA[0], m_verticesA[0]);
        hkVector4 toB1;
        toB1.setSub4(m_verticesBinA[1], m_verticesA[0]);
        const int32_t iSign1 = _mm_movemask_ps(toB1.dot3(edge).getQuad()) & 1;
        const int32_t iSign0 = _mm_movemask_ps(toB0.dot3(edge).getQuad()) & 1;
        if (iSign1 == iSign0) {
            // A's point lies beyond one end of the edge: keep that end.
            if (iSign1) {
                m_verticesBinA[0] = m_verticesBinA[1];
                m_verticesBinB[0] = m_verticesBinB[1];
            }
            m_dimB.m_storage--;
            goto pointPoint;
        }
        // The approximate reciprocal square root is used alone, with no Newton step, as the game does.
        const __m128 kLenInv = _mm_rsqrt_ss(edge.dot3(edge).getQuad());
        const __m128 kEdgeN = _mm_mul_ps(_mm_shuffle_ps(kLenInv, kLenInv, 0x00), edge.m_quad);
        const __m128 kNormal = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(toB1.m_quad, toB1.m_quad, 0xC9), _mm_shuffle_ps(toB0.m_quad, toB0.m_quad, 0xD2)),
            _mm_mul_ps(_mm_shuffle_ps(toB1.m_quad, toB1.m_quad, 0xD2), _mm_shuffle_ps(toB0.m_quad, toB0.m_quad, 0xC9))
        );
        m_support.m_quad = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(kEdgeN, kEdgeN, 0xC9), _mm_shuffle_ps(kNormal, kNormal, 0xD2)),
            _mm_mul_ps(_mm_shuffle_ps(kEdgeN, kEdgeN, 0xD2), _mm_shuffle_ps(kNormal, kNormal, 0xC9))
        );
    }
    goto keepSupport;

edgePoint:
    {
        hkVector4 edge;
        edge.setSub4(m_verticesA[1], m_verticesA[0]);
        hkVector4 fromB0;
        fromB0.setSub4(m_verticesA[0], m_verticesBinA[0]);
        hkVector4 fromB1;
        fromB1.setSub4(m_verticesA[1], m_verticesBinA[0]);
        const int32_t iSign1 = _mm_movemask_ps(fromB1.dot3(edge).getQuad()) & 1;
        const int32_t iSign0 = _mm_movemask_ps(fromB0.dot3(edge).getQuad()) & 1;
        if (iSign1 == iSign0) {
            // B's point lies beyond one end of the edge: keep that end. m_dummy's first two entries are swapped along
            // with it, as m_verticesBinB's are for B's edge.
            if (iSign1) {
                m_verticesA[0] = m_verticesA[1];
                m_dummy[0] = m_dummy[1];
            }
            m_dimA.m_storage--;
            goto pointPoint;
        }
        // The approximate reciprocal square root is used alone, with no Newton step, as the game does.
        const __m128 kLenInv = _mm_rsqrt_ss(edge.dot3(edge).getQuad());
        const __m128 kEdgeN = _mm_mul_ps(_mm_shuffle_ps(kLenInv, kLenInv, 0x00), edge.m_quad);
        const __m128 kNormal = _mm_sub_ps(
            _mm_mul_ps(
                _mm_shuffle_ps(fromB0.m_quad, fromB0.m_quad, 0xD2), _mm_shuffle_ps(fromB1.m_quad, fromB1.m_quad, 0xC9)
            ),
            _mm_mul_ps(
                _mm_shuffle_ps(fromB0.m_quad, fromB0.m_quad, 0xC9), _mm_shuffle_ps(fromB1.m_quad, fromB1.m_quad, 0xD2)
            )
        );
        m_support.m_quad = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(kNormal, kNormal, 0xC9), _mm_shuffle_ps(kEdgeN, kEdgeN, 0xD2)),
            _mm_mul_ps(_mm_shuffle_ps(kNormal, kNormal, 0xD2), _mm_shuffle_ps(kEdgeN, kEdgeN, 0xC9))
        );
    }
    goto keepSupport;

pointPoint:
    m_support.setSub4(m_verticesA[0], m_verticesBinA[0]);
keepSupport:
    {
        const bool bHasA = m_dimA.m_storage > 0;
        m_lastDimB.m_storage = m_dimB.m_storage;
        hkVector4 negSupport;
        negSupport.m_quad = _mm_mul_ps(_mm_set1_ps(-1.0f), m_support.m_quad);
        dir = negSupport;
        dir.normalize3();
        m_support = negSupport;
        if (bHasA) {
            // Repeats the same test once per vertex of A.
            hkVector4 ab;
            ab.setSub4(m_verticesBinA[0], m_verticesA[0]);
            const float fDist = _mm_cvtss_f32(ab.dot3(dir).getQuad());
            for (int32_t i = m_dimA.m_storage; i != 0; i--) {
                const float fAbsDist = std::fabs(fDist);
                if (fMinDistance > fAbsDist) {
                    fMinDistance = fAbsDist;
                }
            }
        }
        hkVector4 change;
        change.setSub4(negSupport, lastSupport);
        const float fChangeSq = _mm_cvtss_f32(change.dot3(change).getQuad());
        if (static_cast<double>(fChangeSq) < 1.000000013351432e-10) {
            iStalls++;
            if (iStalls >= 3) {
                goto done;
            }
        }
        if (static_cast<double>(fMinDistance) * fMinDistance > dRadiusSq) {
            goto nextPass;
        }
    }

done:
    if (output.m_hitFraction > fLambda) {
        output.m_hitFraction = fLambda;
        output.m_normal = normal;
        return 1;
    }
    return 0;
}

// The edge tests of the triangle of B's vertices (in A's space) against the point dots: three triple products in SSE,
// single precision, the w lanes as the shuffles leave them, stored in m_checkTriangleDots - lane 0 for the edge through
// dots-V1, lane 1 through dots-V2, lane 2 through dots-V0. The returned mask has a bit set for each lane below zero, so
// 7 means the point is inside the triangle. With buildSupport set the triangle's normal also goes to m_support, and for
// an inside point whose A side lies behind the triangle (the negative dot of m_verticesA[0] - m_verticesBinA[0] with
// it) the normal is negated, the triangle's winding reversed - the first two vertices of both arrays swap (the second
// pair is m_verticesBinB's: the array continues in the object) - and m_featureChange flagged. The only scalar float
// steps are the two swapped lanes of m_checkTriangleDots.
int32_t hkpGsk::checkTriangleBoundaries(const hkVector4& a, hkVector4* b, hkpGsk::SupportTypes checkSupport) {
    const __m128 kP = _mm_load_ps(&reinterpret_cast<const float*>(&a.m_quad)[0]);
    const __m128 kV0 = _mm_load_ps(&reinterpret_cast<float*>(&b[0].m_quad)[0]);
    const __m128 kV1 = _mm_load_ps(&reinterpret_cast<float*>(&b[1].m_quad)[0]);
    const __m128 kV2 = _mm_load_ps(&reinterpret_cast<float*>(&b[2].m_quad)[0]);

    const __m128 kA = _mm_sub_ps(kV2, kV1);
    const __m128 kARot2 = _mm_shuffle_ps(kA, kA, 0xD2);
    const __m128 kARot1 = _mm_shuffle_ps(kA, kA, 0xC9);
    const __m128 kB = _mm_sub_ps(kV0, kV2);
    const __m128 kBRot1 = _mm_shuffle_ps(kB, kB, 0xC9);
    const __m128 kBRot2 = _mm_shuffle_ps(kB, kB, 0xD2);
    const __m128 kC = _mm_sub_ps(kV1, kV0);
    const __m128 kCRot1 = _mm_shuffle_ps(kC, kC, 0xC9);
    const __m128 kCRot2 = _mm_shuffle_ps(kC, kC, 0xD2);

    const __m128 kNormal = _mm_sub_ps(_mm_mul_ps(kARot1, kBRot2), _mm_mul_ps(kARot2, kBRot1));

    const __m128 kP1 = _mm_sub_ps(kP, kV1);
    const __m128 kP2 = _mm_sub_ps(kP, kV2);
    const __m128 kP0 = _mm_sub_ps(kP, kV0);
    const __m128 kD1 = _mm_mul_ps(
        _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(kP1, kP1, 0xC9), kARot2), _mm_mul_ps(_mm_shuffle_ps(kP1, kP1, 0xD2), kARot1)
        ),
        kNormal
    );
    const __m128 kD2 = _mm_mul_ps(
        _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(kP2, kP2, 0xC9), kBRot2), _mm_mul_ps(_mm_shuffle_ps(kP2, kP2, 0xD2), kBRot1)
        ),
        kNormal
    );
    const __m128 kD0 = _mm_mul_ps(
        _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(kP0, kP0, 0xC9), kCRot2), _mm_mul_ps(_mm_shuffle_ps(kP0, kP0, 0xD2), kCRot1)
        ),
        kNormal
    );

    // Transposes the three products into one vector of lane sums.
    const __m128 kLo = _mm_unpacklo_ps(kD1, kD2);
    const __m128 kHi = _mm_unpackhi_ps(kD1, kD2);
    const __m128 kMix = _mm_movelh_ps(kLo, kD0);
    const __m128 kDots = _mm_add_ps(
        _mm_shuffle_ps(kHi, kD0, 0xE4), _mm_add_ps(_mm_shuffle_ps(_mm_movehl_ps(kMix, kLo), kD0, 0xD4), kMix)
    );
    _mm_store_ps(&reinterpret_cast<float*>(&m_checkTriangleDots.m_quad)[0], kDots);

    const int32_t iMask = _mm_movemask_ps(_mm_cmplt_ps(kDots, _mm_setzero_ps())) & 7;
    if (checkSupport == hkpGsk::BUILD_NO_SUPPORT) {
        return iMask;
    }
    _mm_store_ps(&reinterpret_cast<float*>(&m_support.m_quad)[0], kNormal);
    if (iMask != 7) {
        return iMask;
    }

    const __m128 kSide = _mm_mul_ps(
        _mm_sub_ps(
            _mm_load_ps(&reinterpret_cast<float*>(&m_verticesA[0].m_quad)[0]),
            _mm_load_ps(&reinterpret_cast<float*>(&m_verticesBinA[0].m_quad)[0])
        ),
        kNormal
    );
    const __m128 kSum =
        _mm_add_ss(_mm_shuffle_ps(kSide, kSide, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSide, kSide, 0x55), kSide));
    if (_mm_movemask_ps(kSum) & 1) {
        _mm_store_ps(
            &reinterpret_cast<float*>(&m_support.m_quad)[0],
            _mm_xor_ps(kNormal, _mm_load_ps(&reinterpret_cast<float*>(&hkQuadRealNegateMask)[0]))
        );
        const __m128 kV = _mm_load_ps(&reinterpret_cast<float*>(&b[0].m_quad)[0]);
        _mm_store_ps(
            &reinterpret_cast<float*>(&b[0].m_quad)[0], _mm_load_ps(&reinterpret_cast<float*>(&b[1].m_quad)[0])
        );
        _mm_store_ps(&reinterpret_cast<float*>(&b[1].m_quad)[0], kV);
        const __m128 kVB = _mm_load_ps(&reinterpret_cast<float*>(&b[4].m_quad)[0]);
        _mm_store_ps(
            &reinterpret_cast<float*>(&b[4].m_quad)[0], _mm_load_ps(&reinterpret_cast<float*>(&b[5].m_quad)[0])
        );
        _mm_store_ps(&reinterpret_cast<float*>(&b[5].m_quad)[0], kVB);
        const float fTmp = m_checkTriangleDots.m_quad[0];
        m_checkTriangleDots.m_quad[0] = m_checkTriangleDots.m_quad[1];
        m_checkTriangleDots.m_quad[1] = fTmp;
        m_featureChange.m_storage = 1;
    }
    return iMask;
}

// Drops the simplex to the vertices that matter: an A vertex inside the B triangle's Voronoi region ends it with m_dimA
// = 1 and that vertex first (likewise a B vertex against the A triangle), otherwise every vertex pair is measured with
// the closest-points-of-two-segments helper and the pair at the smallest squared distance decides which vertex of each
// side is dropped - the last vertex takes its slot through vertexToEdgeLut, and for a triangle the side's dimension
// drops to 2.
void hkpGsk::reduceDimension() {
    if (m_dimB.m_storage == 3 && m_dimA.m_storage > 0) {
        for (int32_t i = 0; i < m_dimA.m_storage; i++) {
            if (checkTriangleBoundaries(m_verticesA[i], &m_verticesBinA[0], hkpGsk::BUILD_NO_SUPPORT) == 7) {
                _mm_store_ps(
                    &reinterpret_cast<float*>(&m_verticesA[0].m_quad)[0],
                    _mm_load_ps(&reinterpret_cast<float*>(&m_verticesA[i].m_quad)[0])
                );
                m_dimA.m_storage = 1;
                return;
            }
        }
    }
    if (m_dimA.m_storage == 3 && m_dimB.m_storage > 0) {
        for (int32_t i = 0; i < m_dimB.m_storage; i++) {
            if (checkTriangleBoundaries(m_verticesBinA[i], &m_verticesA[0], hkpGsk::BUILD_NO_SUPPORT) == 7) {
                _mm_store_ps(
                    &reinterpret_cast<float*>(&m_verticesBinB[0].m_quad)[0],
                    _mm_load_ps(&reinterpret_cast<float*>(&m_verticesBinB[i].m_quad)[0])
                );
                _mm_store_ps(
                    &reinterpret_cast<float*>(&m_verticesBinA[0].m_quad)[0],
                    _mm_load_ps(&reinterpret_cast<float*>(&m_verticesBinA[i].m_quad)[0])
                );
                m_dimB.m_storage = 1;
                return;
            }
        }
    }

    const int32_t iNumA = 2 * (m_dimA.m_storage == 3) + 1;
    const int32_t iNumB = 2 * (m_dimB.m_storage == 3) + 1;
    float fBest = 3.40282e38f;
    int32_t iBest = 0;
    int32_t jBest = 0;
    for (int32_t i = 0; i < iNumA; i++) {
        for (int32_t j = 0; j < iNumB; j++) {
            const hkVector4& rA = m_verticesA[hkpCollideTriangleUtil::vertexToEdgeLut[i + 2]];
            hkVector4 kDirA;
            kDirA.m_quad[0] = rA.m_quad[0] - m_verticesA[i].m_quad[0];
            kDirA.m_quad[1] = rA.m_quad[1] - m_verticesA[i].m_quad[1];
            kDirA.m_quad[2] = rA.m_quad[2] - m_verticesA[i].m_quad[2];
            kDirA.m_quad[3] = rA.m_quad[3] - m_verticesA[i].m_quad[3];
            const hkVector4& rB = m_verticesBinA[hkpCollideTriangleUtil::vertexToEdgeLut[j + 2]];
            hkVector4 kDirB;
            kDirB.m_quad[0] = rB.m_quad[0] - m_verticesBinA[j].m_quad[0];
            kDirB.m_quad[1] = rB.m_quad[1] - m_verticesBinA[j].m_quad[1];
            kDirB.m_quad[2] = rB.m_quad[2] - m_verticesBinA[j].m_quad[2];
            kDirB.m_quad[3] = rB.m_quad[3] - m_verticesBinA[j].m_quad[3];
            hkLineSegmentUtil::ClosestLineSegLineSegResult kResult;
            hkLineSegmentUtil::closestLineSegLineSeg(m_verticesA[i], kDirA, m_verticesBinA[j], kDirB, kResult);
            if (fBest > kResult.m_distanceSquared.m_storage) {
                fBest = kResult.m_distanceSquared.m_storage;
                iBest = i;
                jBest = j;
            }
        }
    }

    if (iNumA == 3) {
        --m_dimA.m_storage;
        _mm_store_ps(
            &reinterpret_cast<float*>(&m_verticesA[hkpCollideTriangleUtil::vertexToEdgeLut[iBest]].m_quad)[0],
            _mm_load_ps(&reinterpret_cast<float*>(&m_verticesA[m_dimA.m_storage].m_quad)[0])
        );
    }
    if (iNumB == 3) {
        --m_dimB.m_storage;
        _mm_store_ps(
            &reinterpret_cast<float*>(&m_verticesBinB[hkpCollideTriangleUtil::vertexToEdgeLut[jBest]].m_quad)[0],
            _mm_load_ps(&reinterpret_cast<float*>(&m_verticesBinB[m_dimB.m_storage].m_quad)[0])
        );
        _mm_store_ps(
            &reinterpret_cast<float*>(&m_verticesBinA[hkpCollideTriangleUtil::vertexToEdgeLut[jBest]].m_quad)[0],
            _mm_load_ps(&reinterpret_cast<float*>(&m_verticesBinA[m_dimB.m_storage].m_quad)[0])
        );
    }
}

// The triangle/edge case of the GSK reduction: with pointOnSurface clear, the two dots of the edge's first and second
// vertices against the support vector place the edge across the triangle's plane, and the point between them (or
// nothing) is tested against the triangle; the second vertex's edge test then decides which vertex is dropped - the
// switch's cases 1, 2 and 4 compare the stored dot lanes against the point for the two vertex orders. With
// pointOnSurface set the two vertices are tested against the triangle's normal instead, the order swapped when the
// second is the nearer one, and the edge against the triangle's edge is measured with the
// closest-points-of-two-segments helper, whose smaller squared distance picks the vertex that takes the third vertex's
// place. Both results are the counts decremented in result0 and result1, and the return value is the caller's next
// state.
hkpGsk::NextCase hkpGsk::processEdgeTriangle(
    hkVector4* vertexA,
    hkVector4* vertexB,
    hkPadSpu<int32_t>& dimA,
    hkPadSpu<int32_t>& dimB,
    hkBool comingFromEdgeEdge,
    hkpGsk::SupportTypes supportType
) {
    hkVector4 kDots;
    _mm_store_ps(
        &reinterpret_cast<float*>(&kDots.m_quad)[0],
        _mm_load_ps(&reinterpret_cast<float*>(&m_checkTriangleDots.m_quad)[0])
    );
    if (!comingFromEdgeEdge.m_bool) {
        const __m128 kSupport = _mm_load_ps(&reinterpret_cast<float*>(&m_support.m_quad)[0]);
        const __m128 kFirst = _mm_load_ps(&reinterpret_cast<float*>(&vertexA[0].m_quad)[0]);
        const __m128 kSecond = _mm_load_ps(&reinterpret_cast<float*>(&vertexA[1].m_quad)[0]);
        const __m128 kBase = _mm_load_ps(&reinterpret_cast<float*>(&vertexB[0].m_quad)[0]);
        const __m128 kSecondProduct = _mm_mul_ps(_mm_sub_ps(kSecond, kBase), kSupport);
        const __m128 kSecondDots = _mm_add_ps(
            _mm_shuffle_ps(kSecondProduct, kSecondProduct, 0xAA),
            _mm_add_ps(_mm_shuffle_ps(kSecondProduct, kSecondProduct, 0x55), kSecondProduct)
        );
        const __m128 kFirstProduct = _mm_mul_ps(_mm_sub_ps(kFirst, kBase), kSupport);
        const __m128 kFirstDots = _mm_add_ps(
            _mm_shuffle_ps(kFirstProduct, kFirstProduct, 0xAA),
            _mm_add_ps(_mm_shuffle_ps(kFirstProduct, kFirstProduct, 0x55), kFirstProduct)
        );
        if (_mm_movemask_ps(_mm_mul_ps(kFirstDots, kSecondDots)) & 1) {
            // The fraction is computed in single precision and splatted to all four lanes.
            const __m128 kQuotient = _mm_div_ss(kFirstDots, _mm_sub_ss(kFirstDots, kSecondDots));
            const __m128 kFraction = _mm_shuffle_ps(kQuotient, kQuotient, 0);
            hkVector4 kPoint;
            _mm_store_ps(
                &reinterpret_cast<float*>(&kPoint.m_quad)[0],
                _mm_add_ps(
                    _mm_mul_ps(
                        _mm_sub_ps(_mm_load_ps(&reinterpret_cast<float*>(&hkQuadReal1111_0)[0]), kFraction), kFirst
                    ),
                    _mm_mul_ps(kSecond, kFraction)
                )
            );
            if (checkTriangleBoundaries(kPoint, vertexB, hkpGsk::BUILD_NO_SUPPORT) == 7) {
                return hkpGsk::NEXT_IS_PENETRATION;
            }
        }
        const int32_t iMask = checkTriangleBoundaries(vertexA[1], vertexB, hkpGsk::BUILD_NO_SUPPORT);
        if (iMask == 7) {
            const __m128 kLast = _mm_load_ps(&reinterpret_cast<float*>(&vertexA[1].m_quad)[0]);
            --dimA.m_storage;
            _mm_store_ps(&reinterpret_cast<float*>(&vertexA[0].m_quad)[0], kLast);
            _mm_store_ps(
                &reinterpret_cast<float*>(&vertexA[4].m_quad)[0],
                _mm_load_ps(&reinterpret_cast<float*>(&vertexA[5].m_quad)[0])
            );
            m_supportState = static_cast<SupportState>(supportType);
            return hkpGsk::NEXT_IS_BREAK;
        }
        switch (iMask) {
            case 5:
                --dimB.m_storage;
                _mm_store_ps(
                    &reinterpret_cast<float*>(&vertexB[1].m_quad)[0],
                    _mm_load_ps(&reinterpret_cast<float*>(&vertexB[2].m_quad)[0])
                );
                _mm_store_ps(
                    &reinterpret_cast<float*>(&vertexB[5].m_quad)[0],
                    _mm_load_ps(&reinterpret_cast<float*>(&vertexB[6].m_quad)[0])
                );
                return hkpGsk::NEXT_IS_EDGE_EDGE;
            case 3: --dimB.m_storage; return hkpGsk::NEXT_IS_EDGE_EDGE;
            // Cases 1, 2 and 4: each difference of the dots is stored as a float before multiplying, as the game does.
            case 1: {
                const float fA = (double)m_checkTriangleDots.m_quad[1] - (double)kDots.m_quad[1];
                const float fC = (double)m_checkTriangleDots.m_quad[2] - (double)kDots.m_quad[2];
                if ((double)fA * kDots.m_quad[2] > (double)kDots.m_quad[1] * fC) {
                    --dimB.m_storage;
                    return hkpGsk::NEXT_IS_EDGE_EDGE;
                }
                --dimB.m_storage;
                vertexB[1].m_quad = vertexB[2].m_quad;
                vertexB[5].m_quad = vertexB[6].m_quad;
                return hkpGsk::NEXT_IS_EDGE_EDGE;
            }
            case 2: {
                const float fA = (double)m_checkTriangleDots.m_quad[2] - (double)kDots.m_quad[2];
                const float fC = (double)m_checkTriangleDots.m_quad[0] - (double)kDots.m_quad[0];
                if (!((double)fA * kDots.m_quad[0] > (double)kDots.m_quad[2] * fC)) {
                    --dimB.m_storage;
                    return hkpGsk::NEXT_IS_EDGE_EDGE;
                }
                break;
            }
            case 4: {
                const float fA = (double)m_checkTriangleDots.m_quad[0] - (double)kDots.m_quad[0];
                const float fC = (double)m_checkTriangleDots.m_quad[1] - (double)kDots.m_quad[1];
                if ((double)fA * kDots.m_quad[1] > (double)kDots.m_quad[0] * fC) {
                    --dimB.m_storage;
                    vertexB[1].m_quad = vertexB[2].m_quad;
                    vertexB[5].m_quad = vertexB[6].m_quad;
                    return hkpGsk::NEXT_IS_EDGE_EDGE;
                }
                break;
            }
            default: break;
        }
        _mm_store_ps(
            &reinterpret_cast<float*>(&vertexB[0].m_quad)[0],
            _mm_load_ps(&reinterpret_cast<float*>(&vertexB[2].m_quad)[0])
        );
        _mm_store_ps(
            &reinterpret_cast<float*>(&vertexB[4].m_quad)[0],
            _mm_load_ps(&reinterpret_cast<float*>(&vertexB[6].m_quad)[0])
        );
        --dimB.m_storage;
        return hkpGsk::NEXT_IS_EDGE_EDGE;
    }

    const __m128 kVertex0 = _mm_load_ps(&reinterpret_cast<float*>(&vertexB[0].m_quad)[0]);
    const __m128 kVertex2 = _mm_load_ps(&reinterpret_cast<float*>(&vertexB[2].m_quad)[0]);
    const __m128 kEdge2 = _mm_sub_ps(_mm_load_ps(&reinterpret_cast<float*>(&vertexB[1].m_quad)[0]), kVertex2);
    const __m128 kEdge1 = _mm_sub_ps(kVertex0, kVertex2);
    const __m128 kNormal = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(kEdge1, kEdge1, 0xD2), _mm_shuffle_ps(kEdge2, kEdge2, 0xC9)),
        _mm_mul_ps(_mm_shuffle_ps(kEdge1, kEdge1, 0xC9), _mm_shuffle_ps(kEdge2, kEdge2, 0xD2))
    );
    const __m128 kFirstProduct =
        _mm_mul_ps(_mm_sub_ps(_mm_load_ps(&reinterpret_cast<float*>(&vertexA[0].m_quad)[0]), kVertex0), kNormal);
    const __m128 kFirstDots = _mm_add_ps(
        _mm_shuffle_ps(kFirstProduct, kFirstProduct, 0xAA),
        _mm_add_ps(_mm_shuffle_ps(kFirstProduct, kFirstProduct, 0x55), kFirstProduct)
    );
    const __m128 kSecondProduct =
        _mm_mul_ps(_mm_sub_ps(_mm_load_ps(&reinterpret_cast<float*>(&vertexA[1].m_quad)[0]), kVertex0), kNormal);
    const __m128 kSecondDots = _mm_add_ps(
        _mm_shuffle_ps(kSecondProduct, kSecondProduct, 0xAA),
        _mm_add_ps(_mm_shuffle_ps(kSecondProduct, kSecondProduct, 0x55), kSecondProduct)
    );
    if ((_mm_movemask_ps(kFirstDots) ^ _mm_movemask_ps(kSecondDots)) & 1) {
        // The fraction is computed in single precision and splatted to all four lanes.
        const __m128 kQuotient = _mm_div_ss(kFirstDots, _mm_sub_ss(kFirstDots, kSecondDots));
        const __m128 kFraction = _mm_shuffle_ps(kQuotient, kQuotient, 0);
        hkVector4 kPoint;
        _mm_store_ps(
            &reinterpret_cast<float*>(&kPoint.m_quad)[0],
            _mm_add_ps(
                _mm_mul_ps(
                    _mm_sub_ps(_mm_load_ps(&reinterpret_cast<float*>(&hkQuadReal1111_0)[0]), kFraction),
                    _mm_load_ps(&reinterpret_cast<float*>(&vertexA[0].m_quad)[0])
                ),
                _mm_mul_ps(_mm_load_ps(&reinterpret_cast<float*>(&vertexA[1].m_quad)[0]), kFraction)
            )
        );
        if (checkTriangleBoundaries(kPoint, vertexB, hkpGsk::BUILD_NO_SUPPORT) == 7) {
            return hkpGsk::NEXT_IS_PENETRATION;
        }
    }
    else {
        if (__builtin_fabsf(_mm_cvtss_f32(kSecondDots)) < __builtin_fabsf(_mm_cvtss_f32(kFirstDots))) {
            const __m128 kSwap = _mm_load_ps(&reinterpret_cast<float*>(&vertexA[0].m_quad)[0]);
            _mm_store_ps(
                &reinterpret_cast<float*>(&vertexA[0].m_quad)[0],
                _mm_load_ps(&reinterpret_cast<float*>(&vertexA[1].m_quad)[0])
            );
            _mm_store_ps(&reinterpret_cast<float*>(&vertexA[1].m_quad)[0], kSwap);
            const __m128 kSwap4 = _mm_load_ps(&reinterpret_cast<float*>(&vertexA[4].m_quad)[0]);
            _mm_store_ps(
                &reinterpret_cast<float*>(&vertexA[4].m_quad)[0],
                _mm_load_ps(&reinterpret_cast<float*>(&vertexA[5].m_quad)[0])
            );
            _mm_store_ps(&reinterpret_cast<float*>(&vertexA[5].m_quad)[0], kSwap4);
        }
        if (checkTriangleBoundaries(vertexA[0], vertexB, hkpGsk::BUILD_POS_SUPPORT) == 7) {
            --dimA.m_storage;
            return hkpGsk::NEXT_IS_BREAK;
        }
    }
    hkVector4 kEdge;
    _mm_store_ps(
        &reinterpret_cast<float*>(&kEdge.m_quad)[0],
        _mm_sub_ps(
            _mm_load_ps(&reinterpret_cast<float*>(&vertexA[1].m_quad)[0]),
            _mm_load_ps(&reinterpret_cast<float*>(&vertexA[0].m_quad)[0])
        )
    );
    hkVector4 kDirectionB1;
    _mm_store_ps(&reinterpret_cast<float*>(&kDirectionB1.m_quad)[0], kEdge2);
    hkVector4 kDirectionB0;
    _mm_store_ps(&reinterpret_cast<float*>(&kDirectionB0.m_quad)[0], kEdge1);
    hkLineSegmentUtil::ClosestLineSegLineSegResult kResult0;
    hkLineSegmentUtil::ClosestLineSegLineSegResult kResult1;
    hkLineSegmentUtil::closestLineSegLineSeg(vertexA[0], kEdge, vertexB[2], kDirectionB1, kResult0);
    hkLineSegmentUtil::closestLineSegLineSeg(vertexA[0], kEdge, vertexB[2], kDirectionB0, kResult1);
    // Overwrites the second vertex (index 1) when the second segment's distance is the smaller one.
    const int32_t iIndex = kResult1.m_distanceSquared.m_storage < kResult0.m_distanceSquared.m_storage ? 1 : 0;
    vertexB[iIndex].m_quad = vertexB[2].m_quad;
    vertexB[iIndex + 4].m_quad = vertexB[6].m_quad;
    --dimB.m_storage;
    return hkpGsk::NEXT_IS_EDGE_EDGE;
}

// Havok SDK: the normal and its w (the distance) go out as they are; the closest point on A is A's vertex when A's
// simplex is a point, B's point moved by the distance along the normal when B's is, else the closest point of the
// two segments.
void hkpGsk::convertFeatureToClosestDistance(const hkVector4& separatingNormal, hkpGskOut& out) {
    out.m_distance.m_storage = separatingNormal(3);
    out.m_normalInA.m_quad = separatingNormal.m_quad;
    if (m_dimA.m_storage == 1) {
        out.m_pointAinA.m_quad = m_verticesA[0].m_quad;
    }
    else if (m_dimB.m_storage == 1) {
        __m128 distance = _mm_shuffle_ps(separatingNormal.m_quad, separatingNormal.m_quad, _MM_SHUFFLE(3, 3, 3, 3));
        out.m_pointAinA.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(distance, distance, 0), out.m_normalInA.m_quad), m_verticesBinA[0].m_quad
        );
    }
    else {
        out.m_pointAinA.m_quad = m_closestLineLineResult.m_closestPointA.m_quad;
    }
}

// The simplex is padded to the tetrahedron the penetration solver (hkCalculatePenetrationDepth) starts from, with
// copies of its vertices (a point pair alone is passed as one point), and the solver's status decides: the heuristic
// result leaves a point pair; otherwise the triangle pair it left drops the vertices whose ids (getInt24W) repeat, B's
// vertices are moved into A's space by aTb, and the simplex is reduced and its edge pair measured. Both ways the
// solver's normal becomes m_support.
void hkpGsk::handlePenetration(const hkpConvexShape* shapeA, const hkpConvexShape* shapeB, const hkTransform& aTb) {
    int32_t simplexSize = 4;
    if (m_dimB.m_storage >= 3) {
        if (m_dimB.m_storage > 3) {
            m_verticesA[3] = m_verticesA[0];
        }
        else if (m_dimA.m_storage > 1) {
            m_verticesA[3] = m_verticesA[1];
            m_verticesBinB[3] = m_verticesBinB[0];
            m_verticesBinA[3] = m_verticesBinA[0];
        }
        else {
            simplexSize = 3;
        }
        m_verticesA[1] = m_verticesA[0];
        m_verticesA[2] = m_verticesA[0];
    }
    else if (m_dimA.m_storage >= 3) {
        if (m_dimA.m_storage > 3) {
            m_verticesBinB[3] = m_verticesBinB[0];
            m_verticesBinA[3] = m_verticesBinA[0];
        }
        else if (m_dimB.m_storage > 1) {
            m_verticesA[3] = m_verticesA[0];
            m_verticesBinB[3] = m_verticesBinB[1];
            m_verticesBinA[3] = m_verticesBinA[1];
        }
        else {
            simplexSize = 3;
        }
        m_verticesBinB[1] = m_verticesBinB[0];
        m_verticesBinA[1] = m_verticesBinA[0];
        m_verticesBinB[2] = m_verticesBinB[0];
        m_verticesBinA[2] = m_verticesBinA[0];
    }
    else {
        const hkpCdVertex vertexA = m_verticesA[0];
        m_verticesA[m_dimA.m_storage] = vertexA;
        m_verticesA[m_dimA.m_storage + 1] = vertexA;
        const hkpCdVertex vertexB = m_verticesBinB[0];
        m_verticesBinB[m_dimB.m_storage] = vertexB;
        m_verticesBinB[m_dimB.m_storage + 1] = vertexB;
        const hkVector4 vertexBinA = m_verticesBinA[0];
        m_verticesBinA[m_dimB.m_storage] = vertexBinA;
        m_verticesBinA[m_dimB.m_storage + 1] = vertexBinA;
        simplexSize = 1;
    }

    hkVector4 simplex[4];
    for (int32_t i = 0; i < simplexSize; ++i) {
        simplex[i].m_quad = _mm_sub_ps(m_verticesA[i].m_quad, m_verticesBinA[i].m_quad);
    }
    hkpGskOut output;
    if (hkCalculatePenetrationDepth(
            shapeA, shapeB, &aTb, 0.0001f, m_verticesA, m_verticesBinB, simplex, simplexSize, &output
        ) == HK_GSK_PD_HEURISTIC_SAMPLING) {
        m_dimA.m_storage = 1;
        m_dimB.m_storage = 1;
        m_featureChange.m_storage = 1;
        m_support = output.m_normalInA;
        return;
    }

    const int32_t idA2 = m_verticesA[2].getInt24W();
    const int32_t idA1 = m_verticesA[1].getInt24W();
    m_dimA.m_storage = 3;
    m_dimB.m_storage = 3;
    if (idA1 == idA2 || m_verticesA[0].getInt24W() == idA2) {
        m_dimA.m_storage = 2;
    }
    if (m_verticesA[0].getInt24W() == idA1) {
        --m_dimA.m_storage;
        m_verticesA[0] = m_verticesA[m_dimA.m_storage];
    }
    const int32_t idB2 = m_verticesBinB[2].getInt24W();
    const int32_t idB1 = m_verticesBinB[1].getInt24W();
    if (idB1 == idB2 || m_verticesBinB[0].getInt24W() == idB2) {
        --m_dimB.m_storage;
    }
    if (m_dimB.m_storage >= 2 && m_verticesBinB[0].getInt24W() == idB1) {
        --m_dimB.m_storage;
        m_verticesBinB[0] = m_verticesBinB[m_dimB.m_storage];
    }

    // B's vertices into A's space, the last first.
    const __m128 kCol0 = aTb.m_rotation.m_col0.m_quad;
    const __m128 kCol1 = aTb.m_rotation.m_col1.m_quad;
    const __m128 kCol2 = aTb.m_rotation.m_col2.m_quad;
    const __m128 kTranslation = aTb.m_translation.m_quad;
    int32_t i = m_dimB.m_storage - 1;
    do {
        const __m128 kB = m_verticesBinB[i].m_quad;
        m_verticesBinA[i].m_quad = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(kB, kB, 0x55), kCol1), _mm_mul_ps(_mm_shuffle_ps(kB, kB, 0x00), kCol0)
            ),
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kB, kB, 0xAA), kCol2), kTranslation)
        );
        --i;
    }
    while (i >= 0);

    if (m_dimA.m_storage + m_dimB.m_storage > 4) {
        reduceDimension();
    }
    if (m_dimA.m_storage == 2 && m_dimB.m_storage == 2) {
        hkVector4 dA;
        hkVector4 dB;
        dA.m_quad = _mm_sub_ps(m_verticesA[1].m_quad, m_verticesA[0].m_quad);
        dB.m_quad = _mm_sub_ps(m_verticesBinA[1].m_quad, m_verticesBinA[0].m_quad);
        hkLineSegmentUtil::closestLineSegLineSeg(m_verticesA[0], dA, m_verticesBinA[0], dB, m_closestLineLineResult);
    }
    m_featureChange.m_storage = 1;
    m_support = output.m_normalInA;
}

// Copies the output whole into a local, casts against an identity rotation and copies the local back only on a hit at a
// nonzero fraction.
hkBool hkpGsk::castRay(const hkpConvexShape* shape, const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& output) {
    HK_TIMER_BEGIN("GJK Ray Cast", nullptr);
    XFNV_INLINED_LOCAL(hkpShapeRayCastOutput, localOutput, std::nullopt, std::nullopt);
    // EXE-INLINED 0x696C60
    // Plain vectors and integers (no vtable, nothing owned): natively harmless.
    __builtin_memcpy(static_cast<void*>(&localOutput), &output, sizeof(hkpShapeRayCastOutput));
    // EXE-INLINED none: hkMatrix3::setIdentity
    hkRotation identity;
    identity.m_col0.m_quad = _mm_setzero_ps();
    identity.m_col0.m_quad[0] = 1.0f;
    identity.m_col1.m_quad = _mm_setzero_ps();
    identity.m_col1.m_quad[1] = 1.0f;
    identity.m_col2.m_quad = _mm_setzero_ps();
    identity.m_col2.m_quad[2] = 1.0f;
    hkBool hit = linearCast(shape, nullptr, identity, input, localOutput);
    HK_TIMER_END();
    if (hit.m_bool && !(0.0f == localOutput.m_hitFraction)) {
        output = localOutput;
        return 1;
    }
    return 0;
}

void hkpGsk::print() {}

// Havok SDK: runs GSK from the cache's simplex (exporting the cache back when the simplex changed) and gives the
// separating normal rotated into world space by A's transform, its w the distance less both radii. Below the
// tolerance it also gives the closest point on B in world space: A's point, B's point in A's space or the closest
// point of the two segments, moved along the normal by B's radius less the distance (by B's radius alone from B's
// point), and HK_SUCCESS.
hkResult hkpGsk::getClosestPoint(
    const hkpGsk::GetClosesetPointInput& input,
    hkpGskCache& gskCache,
    hkVector4& separatingNormalOut,
    hkVector4& pointOnBOut
) {
    const hkpConvexShape* shapeA = input.m_shapeA.m_storage;
    const hkpConvexShape* shapeB = input.m_shapeB.m_storage;
    hkpGsk gsk;
    gsk.m_dimA.m_storage = gskCache.m_dimA;
    gsk.m_dimB.m_storage = gskCache.m_dimB;
    gsk.m_maxDimA.m_storage = gskCache.m_maxDimA;
    gsk.m_maxDimB.m_storage = gskCache.m_maxDimB;
    gsk.m_doNotHandlePenetration.m_storage = 0;
    gsk.m_featureChange.m_storage = 0;
    shapeA->convertVertexIdsToVerticesImpl(&gskCache.m_vertices[0], gsk.m_dimA.m_storage, &gsk.m_verticesA[0]);
    shapeB->convertVertexIdsToVerticesImpl(
        &gskCache.m_vertices[gsk.m_dimA.m_storage], gsk.m_dimB.m_storage, &gsk.m_verticesBinB[0]
    );

    hkVector4 separatingNormal;
    gsk.getClosestFeature(input.m_shapeA.m_storage, input.m_shapeB.m_storage, *input.m_aTb.m_storage, separatingNormal);
    if (gsk.m_featureChange.m_storage != 0) {
        gsk.exitAndExportCacheImpl(gskCache);
    }

    const double distance = separatingNormal.m_quad[3];
    const hkQuadReal n = separatingNormal.m_quad;
    const hkRotation& rotationA = input.m_transformA.m_storage->m_rotation;
    const float distanceOut = distance - input.m_shapeA.m_storage->m_radius - input.m_shapeB.m_storage->m_radius;
    separatingNormalOut.m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(n, n, 85), rotationA.m_col1.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(n, n, 0), rotationA.m_col0.m_quad)
        ),
        _mm_mul_ps(_mm_shuffle_ps(n, n, 170), rotationA.m_col2.m_quad)
    );
    separatingNormalOut.m_quad[3] = distanceOut;
    if (!(input.m_collisionTolerance.m_storage > distanceOut)) {
        return HK_FAILURE;
    }

    float radius;
    hkQuadReal pointInA;
    if (gsk.m_dimA.m_storage == 1) {
        radius = input.m_shapeB.m_storage->m_radius - distance;
        pointInA = gsk.m_verticesA[0].m_quad;
    }
    else if (gsk.m_dimB.m_storage == 1) {
        radius = input.m_shapeB.m_storage->m_radius;
        pointInA = gsk.m_verticesBinA[0].m_quad;
    }
    else {
        radius = input.m_shapeB.m_storage->m_radius - distance;
        pointInA = gsk.m_closestLineLineResult.m_closestPointA.m_quad;
    }
    const hkQuadReal p = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(radius), n), pointInA);
    const hkTransform& transformA = *input.m_transformA.m_storage;
    pointOnBOut.m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(p, p, 85), transformA.m_rotation.m_col1.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(p, p, 0), transformA.m_rotation.m_col0.m_quad)
        ),
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(p, p, 170), transformA.m_rotation.m_col2.m_quad), transformA.m_translation.m_quad
        )
    );
    return HK_SUCCESS;
}

void hkpGsk::codeBegin() {}

void hkpGsk::codeEnd() {}
