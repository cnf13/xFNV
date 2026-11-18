#include "Havok/Source/Physics/Collide/Util/Welding/hkpMeshWeldingUtility.h"

#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/Stream/hkStreamStatisticsCollector.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkVtableClassRegistry.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/hkpBvTreeShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Collide/Util/Welding/hkHistogram.h"
#include "Havok/Source/Physics/Collide/Util/Welding/hkpWeldingUtility.h"
#include "Havok/Source/Physics/Collide/Util/hkpTriangleUtil.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpBuildJacobianFromAtoms.h"
#include "Havok/Source/Physics/Utilities/Dynamics/Inertia/hkpInertiaTensorComputer.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

// Only x, y and z of the clipped ends reach the result, so only those lanes are computed, each product and sum rounded
// to a float. The dot products add y to x, then z. Each plane distance and final quotient is stored as a float.
hkBool* hkClipEdgeAgainstPlanes(
    hkBool* apResult,
    const hkVector4& edgeStart,
    const hkVector4& edgeEnd,
    int32_t numPlanes,
    const hkVector4* planes,
    float& tStart,
    float& tEnd
) {
    float fStartX = edgeStart.m_quad[0];
    float fStartY = edgeStart.m_quad[1];
    float fStartZ = edgeStart.m_quad[2];
    float fEndX = edgeEnd.m_quad[0];
    float fEndY = edgeEnd.m_quad[1];
    float fEndZ = edgeEnd.m_quad[2];

    for (int32_t i = 0; i < numPlanes; ++i) {
        const hkQuadReal& kPlane = planes[i].m_quad;

        // The signed distances of the two ends: the plane's w plus the dot product of its normal with the end.
        const float fStartDotX = kPlane[0] * fStartX;
        const float fStartDotY = kPlane[1] * fStartY;
        const float fStartDotZ = kPlane[2] * fStartZ;
        const float fStartDotXY = fStartDotY + fStartDotX;
        const float fStartDot = fStartDotZ + fStartDotXY;
        const float fDistStart = kPlane[3] + fStartDot;
        const float fEndDotX = fEndX * kPlane[0];
        const float fEndDotY = fEndY * kPlane[1];
        const float fEndDotZ = fEndZ * kPlane[2];
        const float fEndDotXY = fEndDotY + fEndDotX;
        const float fEndDot = fEndDotZ + fEndDotXY;
        const float fDistEnd = kPlane[3] + fEndDot;

        // Both ends at or above the plane (neither NaN) leave nothing of the edge.
        if (fDistStart >= 0.0f && fDistEnd >= 0.0f) {
            apResult->m_bool = 0;
            return apResult;
        }

        // The ends straddle the plane only when the product is below zero (not for NaN).
        if (fDistEnd * fDistStart < 0.0f) {
            const float fT = fDistStart / (fDistStart - fDistEnd);
            const float fOneMinusT = 1.0f - fT;
            const float fStartPartX = fOneMinusT * fStartX;
            const float fStartPartY = fOneMinusT * fStartY;
            const float fStartPartZ = fOneMinusT * fStartZ;
            const float fEndPartX = fT * fEndX;
            const float fEndPartY = fT * fEndY;
            const float fEndPartZ = fT * fEndZ;
            const float fClipX = fStartPartX + fEndPartX;
            const float fClipY = fStartPartY + fEndPartY;
            const float fClipZ = fStartPartZ + fEndPartZ;
            // The start moves when its distance is above zero; zero, below or NaN moves the end.
            if (fDistStart > 0.0f) {
                fStartX = fClipX;
                fStartY = fClipY;
                fStartZ = fClipZ;
            }
            else {
                fEndX = fClipX;
                fEndY = fClipY;
                fEndZ = fClipZ;
            }
        }
    }

    // The clipped ends projected onto the original edge, over its squared length.
    const hkQuadReal& kStart = edgeStart.m_quad;
    const hkQuadReal& kEnd = edgeEnd.m_quad;
    const float fEdgeX = kEnd[0] - kStart[0];
    const float fEdgeY = kEnd[1] - kStart[1];
    const float fEdgeZ = kEnd[2] - kStart[2];
    const float fLengthX = fEdgeX * fEdgeX;
    const float fLengthY = fEdgeY * fEdgeY;
    const float fLengthZ = fEdgeZ * fEdgeZ;
    const float fLengthXY = fLengthY + fLengthX;
    const float fLengthSq = fLengthZ + fLengthXY;

    const float fStartOffX = fStartX - kStart[0];
    const float fStartOffY = fStartY - kStart[1];
    const float fStartOffZ = fStartZ - kStart[2];
    const float fStartProjX = fStartOffX * fEdgeX;
    const float fStartProjY = fStartOffY * fEdgeY;
    const float fStartProjZ = fStartOffZ * fEdgeZ;
    const float fStartProjXY = fStartProjY + fStartProjX;
    const float fStartProj = fStartProjZ + fStartProjXY;

    const float fEndOffX = fEndX - kStart[0];
    const float fEndOffY = fEndY - kStart[1];
    const float fEndOffZ = fEndZ - kStart[2];
    const float fEndProjX = fEndOffX * fEdgeX;
    const float fEndProjY = fEndOffY * fEdgeY;
    const float fEndProjZ = fEndOffZ * fEdgeZ;
    const float fEndProjXY = fEndProjY + fEndProjX;
    const float fEndProj = fEndProjZ + fEndProjXY;

    apResult->m_bool = 1;
    tStart = fStartProj / fLengthSq;
    tEnd = fEndProj / fLengthSq;
    return apResult;
}

int32_t getSectorFromWeldingInfo(uint16_t a1, char a2) {
    return a1 >> 5 * a2 & 31;
}

// Whether two histogram entries' values differ by 0.001 or more (or either is NaN); the difference and its absolute
// value are rounded to floats.
bool* entriesDiffer(bool* a1, const hkEntry* a2, const hkEntry* a3) {
    const float fDiff = a2->m_value - a3->m_value;
    const float fAbs = std::fabs(static_cast<double>(fDiff));
    *a1 = !(fAbs < 0.001f);
    return a1;
}

void hkBuildClipPlanesForTriangle(hkVector4* out, const hkpTriangleShape* triangle, float extraRadius) {
    hkVector4 v3;
    hkVector4 v4;
    hkVector4 normal;
    hkpMeshWeldingUtility::calcAntiClockwiseTriangleNormal(
        triangle->m_vertexA, triangle->m_vertexB, triangle->m_vertexC, normal
    );
    v3.m_quad = triangle->m_vertexB.m_quad;
    float v36 = -extraRadius;
    v4.m_quad = triangle->m_vertexC.m_quad;
    __m128 v5 = _mm_sub_ps(v3.m_quad, triangle->m_vertexA.m_quad);
    __m128 v40 = _mm_sub_ps(triangle->m_vertexA.m_quad, v4.m_quad);
    __m128 m_quad = normal.m_quad;
    (*out).m_quad = normal.m_quad;
    __m128 v7 = _mm_sub_ps(v4.m_quad, v3.m_quad);
    __m128 v8 = _mm_mul_ps(triangle->m_vertexA.m_quad, m_quad);
    __m128 v9 = _mm_setzero_ps();
    out->m_quad[3] = v36 - (_mm_shuffle_ps(v8, v8, 170)[0] + (_mm_shuffle_ps(v8, v8, 85)[0] + v8[0]));
    __m128 v10 = _mm_xor_ps(rdata_10C4C00, m_quad);
    out[1].m_quad = v10;
    __m128 v11 = _mm_mul_ps(v10, triangle->m_vertexA.m_quad);
    __m128 v12 = _mm_shuffle_ps(m_quad, m_quad, 210);
    __m128 v38 = _mm_shuffle_ps(m_quad, m_quad, 201);
    out[1].m_quad[3] = v36 - (_mm_shuffle_ps(v11, v11, 170)[0] + (_mm_shuffle_ps(v11, v11, 85)[0] + v11[0]));
    out[2].m_quad =
        _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v5, v5, 201), v12), _mm_mul_ps(_mm_shuffle_ps(v5, v5, 210), v38));
    __m128 v13 = _mm_mul_ps(out[2].m_quad, out[2].m_quad);
    v11[0] = _mm_shuffle_ps(v13, v13, 85)[0] + v13[0];
    __m128 v14 = _mm_shuffle_ps(v13, v13, 170);
    v14[0] = v14[0] + v11[0];
    float v15 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v14[0])));
    v9[0] = 0.0f;
    normal.m_quad = v9;
    __m128 v16 = _mm_setzero_ps();
    v16[0] = 0.5f;
    __m128 v17 = _mm_cmpeq_ss(v14, v9);
    v9[0] = 3.0f - ((v14[0] * v15) * v15);
    v16[0] = (0.5f * v15) * v9[0];
    __m128 v19 = _mm_andnot_ps(v17, v16);
    out[2].m_quad = _mm_mul_ps(_mm_shuffle_ps(v19, v19, 0), out[2].m_quad);
    __m128 v20 = _mm_mul_ps(out[2].m_quad, triangle->m_vertexA.m_quad);
    out[2].m_quad[3] = v36 - (_mm_shuffle_ps(v20, v20, 170)[0] + (_mm_shuffle_ps(v20, v20, 85)[0] + v20[0]));
    out[3].m_quad =
        _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v7, v7, 201), v12), _mm_mul_ps(_mm_shuffle_ps(v7, v7, 210), v38));
    __m128 v21 = out[3].m_quad;
    __m128 v22 = _mm_mul_ps(v21, v21);
    v7[0] = _mm_shuffle_ps(v22, v22, 85)[0] + v22[0];
    __m128 v23 = _mm_shuffle_ps(v22, v22, 170);
    v23[0] = v23[0] + v7[0];
    float v24 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v23[0])));
    __m128 v25 = _mm_cmpeq_ss(v23, normal.m_quad);
    v9[0] = 3.0f - ((v23[0] * v24) * v24);
    v16[0] = (0.5f * v24) * v9[0];
    __m128 v27 = _mm_andnot_ps(v25, v16);
    out[3].m_quad = _mm_mul_ps(_mm_shuffle_ps(v27, v27, 0), v21);
    __m128 v28 = _mm_mul_ps(out[3].m_quad, triangle->m_vertexB.m_quad);
    float v37 = _mm_shuffle_ps(v28, v28, 170)[0] + (_mm_shuffle_ps(v28, v28, 85)[0] + v28[0]);
    __m128 v30 = _mm_mul_ps(_mm_shuffle_ps(v40, v40, 210), v38);
    out[3].m_quad[3] = v36 - v37;
    out[4].m_quad = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v40, v40, 201), v12), v30);
    __m128 v31 = out[4].m_quad;
    __m128 v32 = _mm_mul_ps(v31, v31);
    v30[0] = _mm_shuffle_ps(v32, v32, 85)[0] + v32[0];
    __m128 v33 = _mm_shuffle_ps(v32, v32, 170);
    v33[0] = v33[0] + v30[0];
    v30[0] = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v33[0])));
    v16[0] = (0.5f * v30[0]) * (3.0f - ((v33[0] * v30[0]) * v30[0]));
    __m128 v34 = _mm_andnot_ps(_mm_cmpeq_ss(v33, normal.m_quad), v16);
    out[4].m_quad = _mm_mul_ps(_mm_shuffle_ps(v34, v34, 0), v31);
    __m128 v35 = _mm_mul_ps(out[4].m_quad, triangle->m_vertexC.m_quad);
    out[4].m_quad[3] = v36 - (_mm_shuffle_ps(v35, v35, 170)[0] + (_mm_shuffle_ps(v35, v35, 85)[0] + v35[0]));
}

// Whether two triangles have the same three vertices, each within 0.001 in x, y and z.
uint8_t* isSameTriangle(uint8_t* a1, const hkpTriangleShape* a2, const hkpTriangleShape* a3) {
    __m128 v3 = _mm_shuffle_ps(_mm_set_ss(0.001f), _mm_set_ss(0.001f), 0);
    if ((_mm_movemask_ps(_mm_cmple_ps(
             _mm_and_ps(_mm_sub_ps(a2->m_vertexA.m_quad, a3->m_vertexA.m_quad), hkVector4_setAbs4_absMask), v3
         )) &
         7) == 7 &&
        ((_mm_movemask_ps(_mm_cmple_ps(
              _mm_and_ps(_mm_sub_ps(a2->m_vertexB.m_quad, a3->m_vertexB.m_quad), hkVector4_setAbs4_absMask), v3
          )) &
          7) == 7 &&
         (_mm_movemask_ps(_mm_cmple_ps(
              _mm_and_ps(_mm_sub_ps(a2->m_vertexC.m_quad, a3->m_vertexC.m_quad), hkVector4_setAbs4_absMask), v3
          )) &
          7) == 7)) {
        *a1 = 1;
        return a1;
    }

    *a1 = 0;
    return a1;
}

// Whether both points are vertices of the triangle, each within 0.001 in x, y and z.
uint8_t* triangleHasVertices(uint8_t* a1, const hkVector4* a2, const hkVector4* a3, const hkpTriangleShape* a4) {
    __m128 v6;
    __m128 v4 = a2->m_quad;
    __m128 v5 = _mm_shuffle_ps(_mm_set_ss(0.001f), _mm_set_ss(0.001f), 0);
    if (((_mm_movemask_ps(
              _mm_cmple_ps(_mm_and_ps(_mm_sub_ps(a4->m_vertexA.m_quad, a2->m_quad), hkVector4_setAbs4_absMask), v5)
          ) &
          7) == 7 ||
         ((_mm_movemask_ps(
               _mm_cmple_ps(_mm_and_ps(_mm_sub_ps(a4->m_vertexB.m_quad, v4), hkVector4_setAbs4_absMask), v5)
           ) &
           7) == 7 ||
          (_mm_movemask_ps(
               _mm_cmple_ps(_mm_and_ps(_mm_sub_ps(a4->m_vertexC.m_quad, v4), hkVector4_setAbs4_absMask), v5)
           ) &
           7) == 7)) &&
        ((v6 = a3->m_quad,
          (_mm_movemask_ps(
               _mm_cmple_ps(_mm_and_ps(_mm_sub_ps(a4->m_vertexA.m_quad, a3->m_quad), hkVector4_setAbs4_absMask), v5)
           ) &
           7) == 7) ||
         ((_mm_movemask_ps(
               _mm_cmple_ps(_mm_and_ps(_mm_sub_ps(a4->m_vertexB.m_quad, v6), hkVector4_setAbs4_absMask), v5)
           ) &
           7) == 7 ||
          (_mm_movemask_ps(
               _mm_cmple_ps(_mm_and_ps(_mm_sub_ps(a4->m_vertexC.m_quad, v6), hkVector4_setAbs4_absMask), v5)
           ) &
           7) == 7))) {
        *a1 = 1;
        return a1;
    }

    *a1 = 0;
    return a1;
}

// Whether two directions are almost perpendicular: |dot3| below 0.05.
bool* isPerpendicular(bool* a1, const hkVector4* a2, const hkVector4* a3) {
    __m128 v3 = _mm_mul_ps(a2->m_quad, a3->m_quad);
    float v5 = std::fabs((double)(_mm_shuffle_ps(v3, v3, 170)[0] + (_mm_shuffle_ps(v3, v3, 85)[0] + v3[0])));
    *a1 = v5 < 0.05000000074505806;
    return a1;
}

// Whether the triangle lies on the welding side of an edge: its vertices' distances from point along normal1 (each an
// SSE dot product, y + x then z, stored as a float) give a range; a range above 0.01 somewhere that does not straddle
// 0.2 decides by the sign of dir along normal2, weighted by whether the far end dominates (1 when |min| < |max|, else
// 0). Else the same along normal2, deciding by dir along normal1. The ranges and the comparisons are x87.
int32_t testWindingConsistency(
    const hkVector4* point,
    const hkVector4* normal1,
    const hkVector4* normal2,
    const hkpTriangleShape* triangle,
    const hkVector4* dir
) {
    const hkVector4* pVertices = &triangle->m_vertexA;
    const __m128 kPoint = point->m_quad;
    const __m128 kNormal1 = normal1->m_quad;
    float fMax = -3.40282e+38f;
    float fMin = 3.40282e+38f;
    for (int32_t i = 0; i < 3; ++i) {
        const __m128 kProduct = _mm_mul_ps(_mm_sub_ps(pVertices[i].m_quad, kPoint), kNormal1);
        const __m128 kXY = _mm_add_ss(_mm_shuffle_ps(kProduct, kProduct, 0x55), kProduct);
        const float fDist = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kProduct, kProduct, 0xAA), kXY));
        if (fDist > fMax) {
            fMax = fDist;
        }
        if (fDist < fMin) {
            fMin = fDist;
        }
    }
    if (fMax > 0.01f || fMin < 0.01f) {
        if (fMax > 0.20000000298023224 && fMin < 0.20000000298023224) {
            return 0;
        }
        const float fAbsMax = std::fabs(static_cast<double>(fMax));
        const float fAbsMin = std::fabs(static_cast<double>(fMin));
        const int32_t iWeight = (fAbsMin < fAbsMax) ? 1 : 0;
        const float fWeight = static_cast<float>(iWeight);
        const __m128 kProduct = _mm_mul_ps(normal2->m_quad, dir->m_quad);
        const __m128 kXY = _mm_add_ss(_mm_shuffle_ps(kProduct, kProduct, 0x55), kProduct);
        const __m128 kDot = _mm_add_ss(_mm_shuffle_ps(kProduct, kProduct, 0xAA), kXY);
        const float fSide = _mm_cvtss_f32(_mm_mul_ss(kDot, _mm_set_ss(fWeight)));
        if (0.0f > fSide) {
            return 1;
        }
    }
    const __m128 kNormal2 = normal2->m_quad;
    float fMax2 = -3.40282e+38f;
    float fMin2 = 3.40282e+38f;
    for (int32_t i = 0; i < 3; ++i) {
        const __m128 kProduct = _mm_mul_ps(_mm_sub_ps(pVertices[i].m_quad, kPoint), kNormal2);
        const __m128 kXY = _mm_add_ss(_mm_shuffle_ps(kProduct, kProduct, 0x55), kProduct);
        const float fDist = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kProduct, kProduct, 0xAA), kXY));
        if (fDist > fMax2) {
            fMax2 = fDist;
        }
        if (fDist < fMin2) {
            fMin2 = fDist;
        }
    }
    if (!(fMax2 > 0.01f) && !(0.01f > fMin2)) {
        return 0;
    }
    if (fMax2 > 0.20000000298023224 && 0.20000000298023224 > fMin2) {
        return 0;
    }
    const float fAbsMax2 = std::fabs(static_cast<double>(fMax2));
    const float fAbsMin2 = std::fabs(static_cast<double>(fMin2));
    const int32_t iWeight2 = (fAbsMin2 < fAbsMax2) ? 1 : 0;
    const float fWeight2 = static_cast<float>(iWeight2);
    const __m128 kProduct = _mm_mul_ps(kNormal1, dir->m_quad);
    const __m128 kXY = _mm_add_ss(_mm_shuffle_ps(kProduct, kProduct, 0x55), kProduct);
    const __m128 kDot = _mm_add_ss(_mm_shuffle_ps(kProduct, kProduct, 0xAA), kXY);
    const float fSide = _mm_cvtss_f32(_mm_mul_ss(kDot, _mm_set_ss(fWeight2)));
    return (0.0f < fSide) ? 1 : 0;
}

// The edge's two vertices of the triangle (from vertex edgeIndex on) and the opposite one into out[0..2], then each
// vertex of the other triangle that is neither of the edge's two: into out[3..] when it differs from both, else its
// index into sharedIndices at the edge vertex it equals. Returns the number of points in out. The squared distances
// are SSE sums (y + x, then z) compared with zero on the x87.
int32_t createSingularVertexArray(
    const hkVector4* vertices,
    const hkVector4* otherVertices,
    int32_t edgeIndex,
    hkVector4* out,
    int32_t* sharedIndices
) {
    out[0] = vertices[edgeIndex % 3];
    out[1] = vertices[(edgeIndex + 1) % 3];
    out[2] = vertices[(edgeIndex + 2) % 3];
    int32_t iNumPoints = 3;
    hkVector4* pNext = &out[3];
    for (int32_t i = 0; i < 3; ++i, ++otherVertices) {
        const __m128 kVertex = otherVertices->m_quad;
        int32_t j = 0;
        for (; j < 2; ++j) {
            __m128 diff = _mm_sub_ps(out[j].m_quad, kVertex);
            diff = _mm_mul_ps(diff, diff);
            const __m128 kXY = _mm_add_ss(_mm_shuffle_ps(diff, diff, 0x55), diff);
            const __m128 kXYZ = _mm_add_ss(_mm_shuffle_ps(diff, diff, 0xAA), kXY);
            if (0.0f == _mm_cvtss_f32(kXYZ)) {
                break;
            }
        }
        if (j < 2) {
            sharedIndices[j] = i;
        }
        else {
            ++iNumPoints;
            pNext->m_quad = kVertex;
            ++pNext;
        }
    }
    return iNumPoints;
}

// The transform and the shape; the padding is not copied.
hkpMeshWeldingUtility::ShapeInfo::ShapeInfo(const hkpMeshWeldingUtility::ShapeInfo& a1)
  : m_transform(a1.m_transform), m_shape(a1.m_shape) {}

uint16_t hkpMeshWeldingUtility::modifyCombinedEdgesBitcode(uint16_t a1, int32_t a2, int32_t a3) {
    return (uint16_t)((a3 << (5 * ((int8_t)a2))) | (((int32_t)a1) & (~(31 << (5 * ((int8_t)a2))))));
}

int32_t hkpMeshWeldingUtility::calcEdgeAngleBitcode(float cosAngle) {
    float v5;
    double v1 = (double)cosAngle;
    int32_t v2 = 0;
    int32_t v4 = 0;
    while (true) {
        v5 = (float)((((double)v4) * 0.20943951606750488) - 3.1415927410125732);
        if (v5 >= v1) {
            break;
        }
        v4 = ++v2;
        if (v2 >= 31) {
            return v2;
        }
    }
    if ((!(v1 > 0.0)) || (v5 == v1)) {
        return v2;
    }

    return v2 - 1;
}

// KEEP: an uninitialised read. createSingularVertexArray writes sharedIndices[j] only for the edge vertex one of the
// neighbour's vertices equals, so a neighbour two of whose vertices equal the same edge vertex returns 4 with
// sharedIndices[1] never written (on the first neighbour), and the winding test (0xDDE639) reads it. The neighbours of
// the edge from vertex edgeIndex: every triangle the tree finds within the tolerance of that vertex that shares the
// edge puts the angle between the two triangles into the edge's bitcode; with the winding test on, a neighbour that
// runs the edge the same way fails the triangle (warned once). The error text is written into the child shape buffer,
// as the exe does.
hkResult hkpMeshWeldingUtility::calcBitcodeForTriangleEdge(
    const hkpBvTreeShape* bvTreeShape,
    const hkpTriangleShape* triangleShape,
    uint32_t triangleShapeKey,
    int32_t edgeIndex,
    hkpMeshWeldingUtility::WindingConsistency testConsistency,
    uint16_t& combinedBitcodesOut
) {
    const float kTolerance = 0.0001f;
    const float fNegTolerance = -0.0f - kTolerance;
    const hkVector4* pVertices = &triangleShape->m_vertexA;
    const __m128 kVertex = pVertices[edgeIndex % 3].m_quad;
    alignas(16) hkAabb aabb;
    aabb.m_min.m_quad = _mm_add_ps(_mm_setr_ps(fNegTolerance, fNegTolerance, fNegTolerance, 0.0f), kVertex);
    aabb.m_max.m_quad = _mm_add_ps(_mm_setr_ps(kTolerance, kTolerance, kTolerance, 0.0f), kVertex);
    using KeyArray = hkInplaceArray<uint32_t, 128>;
    alignas(16) XFNV_INLINED_LOCAL(KeyArray, hits, std::nullopt, std::nullopt);
    // EXE-INLINED none: hkInplaceArray<hkpShapeKey, 128>::hkInplaceArray
    hits.m_data = hits.m_storage;
    hits.m_size = 0;
    hits.m_capacityAndFlags = 128 | hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG;
    bvTreeShape->queryAabb(aabb, hits);

    alignas(16) char buffer[512];
    alignas(16) hkVector4 points[6];
    int32_t sharedIndices[2];
    uint8_t bConsistent = 1;
    for (int32_t i = hits.m_size - 1; i >= 0; --i) {
        const uint32_t key = hits.m_data[i];
        if (key == triangleShapeKey) {
            continue;
        }
        const hkpShapeContainer* pContainer = bvTreeShape->getContainer();
        const hkpShape* pChild = pContainer->getChildShape(key, buffer);
        if (pChild->m_type.m_storage != HK_SHAPE_TRIANGLE) {
            continue;
        }
        const hkpTriangleShape* pNeighbour = static_cast<const hkpTriangleShape*>(pChild);
        if (createSingularVertexArray(pVertices, &pNeighbour->m_vertexA, edgeIndex, points, sharedIndices) != 4) {
            continue;
        }
        if (testConsistency != hkpMeshWeldingUtility::WINDING_IGNORE_CONSISTENCY &&
            sharedIndices[0] != (sharedIndices[1] + 1) % 3) {
            if (bConsistent) {
                alignas(16) hkErrStream errStream(buffer, 512);
                errStream << "Inconsistant triangle winding between at least triangle " << triangleShapeKey
                          << " and triangle " << key << ". One sided welding will not work.";
                hkError::s_instance->message(
                    hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA751E), buffer,
                    ".\\Util\\Welding\\hkpMeshWeldingUtility.cpp", 713
                );
            }
            bConsistent = 0;
        }
        const int32_t iAngleBitcode = hkpMeshWeldingUtility::calcEdgeAngleBitcode(points);
        combinedBitcodesOut =
            hkpMeshWeldingUtility::modifyCombinedEdgesBitcode(combinedBitcodesOut, edgeIndex, iAngleBitcode);
    }
    const hkResult eResult = static_cast<hkResult>(bConsistent == 0);
    {
        // EXE-INLINED none: hkInplaceArray<hkpShapeKey, 128>::~hkInplaceArray
        hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
        const int32_t iCapacityAndFlags = hits.m_capacityAndFlags;
        hits.m_size = 0;
        if (iCapacityAndFlags >= 0) {
            pHeap->bufFree(
                hits.m_data,
                (iCapacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK) * static_cast<int32_t>(sizeof(uint32_t))
            );
        }
    }
    return eResult;
}

float hkpMeshWeldingUtility::calcAngleForEdge(
    const hkVector4& edgeNormal,
    const hkVector4& edgeOrtho,
    const hkVector4& triangleNormal
) {
    __m128 v3 = _mm_mul_ps(edgeOrtho.m_quad, triangleNormal.m_quad);
    float v4 = _mm_shuffle_ps(v3, v3, 85)[0] + v3[0];
    float v5 = _mm_shuffle_ps(v3, v3, 170)[0];
    __m128 v6 = _mm_mul_ps(edgeNormal.m_quad, triangleNormal.m_quad);
    return hkMath_atan2fApproximation(
        v5 + v4, _mm_shuffle_ps(v6, v6, 170)[0] + (_mm_shuffle_ps(v6, v6, 85)[0] + v6[0])
    );
}

void hkpMeshWeldingUtility::calcAntiClockwiseTriangleNormal(
    const hkVector4& vertex0,
    const hkVector4& vertex1,
    const hkVector4& vertex2,
    hkVector4& normal
) {
    __m128 v10 = _mm_undefined_ps();
    __m128 v4 = _mm_sub_ps(vertex1.m_quad, vertex0.m_quad);
    __m128 v5 = _mm_sub_ps(vertex2.m_quad, vertex1.m_quad);
    __m128 v6 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(v4, v4, 201), _mm_shuffle_ps(v5, v5, 210)),
        _mm_mul_ps(_mm_shuffle_ps(v4, v4, 210), _mm_shuffle_ps(v5, v5, 201))
    );
    __m128 v7 = _mm_mul_ps(v6, v6);
    v4[0] = _mm_shuffle_ps(v7, v7, 85)[0] + v7[0];
    __m128 v8 = _mm_shuffle_ps(v7, v7, 170);
    v8[0] = v8[0] + v4[0];
    float v9 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v8[0])));
    v10[0] = std::bit_cast<float>(static_cast<int32_t>(0.0f));
    __m128 v11 = _mm_cmpeq_ss(v8, v10);
    v10[0] = 3.0f - ((v8[0] * v9) * v9);
    __m128 v12 = _mm_setzero_ps();
    v12[0] = (0.5f * v9) * v10[0];
    __m128 v13 = _mm_andnot_ps(v11, v12);
    normal.m_quad = _mm_mul_ps(_mm_shuffle_ps(v13, v13, 0), v6);
}

// The welding of a triangle collection against the shapes of allShapes: for each triangle and each of its edges, the
// triangles of every shape near the edge (moved into the mesh's space) that share the edge but are not the triangle
// itself give the angles between the two, over the part of the edge they cover; the histogram of those angles gives the
// edge's welding bitcode, and the three bitcodes go into the collection's welding info. A collection of another type is
// warned about.
void hkpMeshWeldingUtility::computeWeldingInfoMultiShape(
    const hkTransform& meshTransform,
    hkpShapeCollection* mesh,
    hkpWeldingUtility::WeldingType weldingType,
    hkArray<hkpMeshWeldingUtility::ShapeInfo, hkContainerHeapAllocator>& allShapes,
    bool weldOpenEdges
) {
    const uint8_t kCollectionType = mesh->m_collectionType.m_storage;
    if (kCollectionType != hkpShapeCollection::COLLECTION_EXTENDED_MESH &&
        kCollectionType != hkpShapeCollection::COLLECTION_TRISAMPLED_HEIGHTFIELD &&
        kCollectionType != hkpShapeCollection::COLLECTION_COMPRESSED_MESH &&
        kCollectionType != hkpShapeCollection::COLLECTION_SIMPLE_MESH &&
        kCollectionType != hkpShapeCollection::COLLECTION_MESH_SHAPE) {
        alignas(16) char buf[512];
        hkErrStream errStream(buf, 512);
        errStream << "hkpShapeCollection must be a triangle collection that supports welding.";
        hkError::s_instance->message(
            hkError::MESSAGE_WARNING, 0x3F508C6A, buf, ".\\Util\\Welding\\hkpMeshWeldingUtility.cpp", 271
        );
        return;
    }
    mesh->initWeldingInfo(weldingType);
    if (weldingType == hkpWeldingUtility::WELDING_TYPE_NONE) {
        return;
    }
    // Adjusts to the container base without a null test, as the game does.
    hkpShapeContainer* pContainer = &static_cast<hkpShapeContainer&>(*mesh);
    mesh->m_disableWelding.m_bool = 1;
    uint32_t key = pContainer->getFirstKey();
    if (key == 0xFFFFFFFF) {
        return;
    }
    alignas(16) char shapeBuffer[512];
    alignas(16) char childBuffer[512];
    const __m128 kZero = _mm_set_ss(0.0f);
    const __m128 kThree = _mm_set_ss(3.0f);
    const __m128 kHalf = _mm_set_ss(0.5f);
    do {
        const hkpShape* pShape = pContainer->getChildShape(key, shapeBuffer);
        if (pShape->m_type.m_storage == HK_SHAPE_TRIANGLE) {
            const hkpTriangleShape* pTriangle = static_cast<const hkpTriangleShape*>(pShape);
            hkVector4 normal;
            hkpMeshWeldingUtility::calcAntiClockwiseTriangleNormal(
                pTriangle->m_vertexA, pTriangle->m_vertexB, pTriangle->m_vertexC, normal
            );
            uint16_t combinedBitcode = 0;
            const hkVector4* pVertices = &pTriangle->m_vertexA;
            for (int32_t edge = 0; edge < 3; ++edge) {
                XFNV_INLINED_LOCAL(hkHistogram, histogram, std::nullopt, std::nullopt);
                // EXE-INLINED none: hkHistogram::hkHistogram
                histogram.m_freeAngle = weldOpenEdges ? 0.0f : 3.1415927f;
                histogram.m_entries.m_data = nullptr;
                histogram.m_entries.m_size = 0;
                histogram.m_entries.m_capacityAndFlags = hkArrayBase<hkEntry>::DONT_DEALLOCATE_FLAG;
                hkVector4 edgeStart;
                edgeStart.m_quad = pVertices[edge].m_quad;
                hkVector4 edgeEnd;
                edgeEnd.m_quad = pVertices[(edge + 1) % 3].m_quad;
                hkVector4 third;
                third.m_quad = pVertices[(edge + 2) % 3].m_quad;
                for (int32_t i = 0; i < allShapes.m_size; ++i) {
                    const hkpMeshWeldingUtility::ShapeInfo& kInfo = allShapes.m_data[i];
                    const hkpBvTreeShape* pBvTree = kInfo.m_shape;
                    // The triangle's normal and points in the shape's space.
                    hkTransform toShape;
                    toShape.setMulInverseMul(kInfo.m_transform, meshTransform);
                    const __m128 kCol0 = toShape.m_rotation.m_col0.m_quad;
                    const __m128 kCol1 = toShape.m_rotation.m_col1.m_quad;
                    const __m128 kCol2 = toShape.m_rotation.m_col2.m_quad;
                    const __m128 kTrans = toShape.m_translation.m_quad;
                    hkVector4 edgeNormal;
                    edgeNormal.m_quad = _mm_add_ps(
                        _mm_add_ps(
                            _mm_mul_ps(kCol0, _mm_shuffle_ps(normal.m_quad, normal.m_quad, 0x00)),
                            _mm_mul_ps(kCol1, _mm_shuffle_ps(normal.m_quad, normal.m_quad, 0x55))
                        ),
                        _mm_mul_ps(kCol2, _mm_shuffle_ps(normal.m_quad, normal.m_quad, 0xAA))
                    );
                    hkVector4 start;
                    start.m_quad = _mm_add_ps(
                        _mm_add_ps(
                            _mm_mul_ps(kCol0, _mm_shuffle_ps(edgeStart.m_quad, edgeStart.m_quad, 0x00)),
                            _mm_mul_ps(kCol1, _mm_shuffle_ps(edgeStart.m_quad, edgeStart.m_quad, 0x55))
                        ),
                        _mm_add_ps(_mm_mul_ps(kCol2, _mm_shuffle_ps(edgeStart.m_quad, edgeStart.m_quad, 0xAA)), kTrans)
                    );
                    hkVector4 end;
                    end.m_quad = _mm_add_ps(
                        _mm_add_ps(
                            _mm_mul_ps(kCol0, _mm_shuffle_ps(edgeEnd.m_quad, edgeEnd.m_quad, 0x00)),
                            _mm_mul_ps(kCol1, _mm_shuffle_ps(edgeEnd.m_quad, edgeEnd.m_quad, 0x55))
                        ),
                        _mm_add_ps(_mm_mul_ps(kCol2, _mm_shuffle_ps(edgeEnd.m_quad, edgeEnd.m_quad, 0xAA)), kTrans)
                    );
                    hkVector4 opposite;
                    opposite.m_quad = _mm_add_ps(
                        _mm_add_ps(
                            _mm_mul_ps(kCol0, _mm_shuffle_ps(third.m_quad, third.m_quad, 0x00)),
                            _mm_mul_ps(kCol1, _mm_shuffle_ps(third.m_quad, third.m_quad, 0x55))
                        ),
                        _mm_add_ps(_mm_mul_ps(kCol2, _mm_shuffle_ps(third.m_quad, third.m_quad, 0xAA)), kTrans)
                    );
                    // The edge's direction, normalised (hkVector4::normalize3, inline).
                    hkVector4 edgeDir;
                    {
                        const __m128 kDiff = _mm_sub_ps(end.m_quad, start.m_quad);
                        const __m128 kSq = _mm_mul_ps(kDiff, kDiff);
                        const __m128 kLen2 =
                            _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq));
                        const __m128 kRsqrt = _mm_rsqrt_ss(kLen2);
                        const __m128 kIsZero = _mm_cmpeq_ss(kLen2, kZero);
                        const __m128 kNewton = _mm_sub_ss(kThree, _mm_mul_ss(_mm_mul_ss(kLen2, kRsqrt), kRsqrt));
                        const __m128 kScale = _mm_andnot_ps(kIsZero, _mm_mul_ss(_mm_mul_ss(kHalf, kRsqrt), kNewton));
                        edgeDir.m_quad = _mm_mul_ps(_mm_shuffle_ps(kScale, kScale, 0), kDiff);
                    }
                    alignas(16) hkAabb aabb;
                    aabb.m_min.m_quad = _mm_min_ps(_mm_min_ps(_mm_set1_ps(3.40282e+38f), start.m_quad), end.m_quad);
                    aabb.m_max.m_quad = _mm_max_ps(_mm_max_ps(_mm_set1_ps(-3.40282e+38f), start.m_quad), end.m_quad);
                    using KeyArray = hkInplaceArray<uint32_t, 128>;
                    alignas(16) XFNV_INLINED_LOCAL(KeyArray, hits, std::nullopt, std::nullopt);
                    // EXE-INLINED none: hkInplaceArray<hkpShapeKey, 128>::hkInplaceArray
                    hits.m_data = hits.m_storage;
                    hits.m_size = 0;
                    hits.m_capacityAndFlags = 128 | hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG;
                    pBvTree->queryAabb(aabb, hits);
                    for (int32_t j = 0; j < hits.m_size; ++j) {
                        const uint32_t childKey = hits.m_data[j];
                        const hkpShapeContainer* pChildContainer = pBvTree->getContainer();
                        const hkpShape* pChild = pChildContainer->getChildShape(childKey, childBuffer);
                        if (pChild->m_type.m_storage != HK_SHAPE_TRIANGLE) {
                            continue;
                        }
                        const hkpTriangleShape* pOther = static_cast<const hkpTriangleShape*>(pChild);
                        uint8_t bSame;
                        if (*isSameTriangle(&bSame, pTriangle, pOther)) {
                            continue;
                        }
                        uint8_t bHasEndAndOpposite;
                        if (*triangleHasVertices(&bHasEndAndOpposite, &end, &opposite, pOther)) {
                            continue;
                        }
                        uint8_t bHasOppositeAndStart;
                        if (*triangleHasVertices(&bHasOppositeAndStart, &opposite, &start, pOther)) {
                            continue;
                        }
                        hkVector4 otherNormal;
                        hkpMeshWeldingUtility::calcAntiClockwiseTriangleNormal(
                            pOther->m_vertexA, pOther->m_vertexB, pOther->m_vertexC, otherNormal
                        );
                        bool bPerpendicular;
                        if (!*isPerpendicular(&bPerpendicular, &edgeDir, &otherNormal)) {
                            continue;
                        }
                        // The direction across the edge in the triangle's plane: the normal crossed with the edge,
                        // normalised.
                        hkVector4 edgeOrtho;
                        {
                            const __m128 kCross = _mm_sub_ps(
                                _mm_mul_ps(
                                    _mm_shuffle_ps(edgeNormal.m_quad, edgeNormal.m_quad, 0xD2),
                                    _mm_shuffle_ps(edgeDir.m_quad, edgeDir.m_quad, 0xC9)
                                ),
                                _mm_mul_ps(
                                    _mm_shuffle_ps(edgeNormal.m_quad, edgeNormal.m_quad, 0xC9),
                                    _mm_shuffle_ps(edgeDir.m_quad, edgeDir.m_quad, 0xD2)
                                )
                            );
                            const __m128 kSq = _mm_mul_ps(kCross, kCross);
                            const __m128 kLen2 = _mm_add_ss(
                                _mm_shuffle_ps(kSq, kSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq)
                            );
                            const __m128 kRsqrt = _mm_rsqrt_ss(kLen2);
                            const __m128 kIsZero = _mm_cmpeq_ss(kLen2, kZero);
                            const __m128 kNewton = _mm_sub_ss(kThree, _mm_mul_ss(_mm_mul_ss(kLen2, kRsqrt), kRsqrt));
                            const __m128 kScale =
                                _mm_andnot_ps(kIsZero, _mm_mul_ss(_mm_mul_ss(kHalf, kRsqrt), kNewton));
                            edgeOrtho.m_quad = _mm_mul_ps(_mm_shuffle_ps(kScale, kScale, 0), kCross);
                        }
                        const float fAngle =
                            hkpMeshWeldingUtility::calcAngleForEdge(edgeNormal, edgeOrtho, otherNormal);
                        hkVector4 planes[5];
                        hkBuildClipPlanesForTriangle(planes, pOther, 0.05f);
                        hkBool bClipped;
                        float tStart;
                        float tEnd;
                        if (hkClipEdgeAgainstPlanes(&bClipped, start, end, 5, planes, tStart, tEnd)->m_bool) {
                            histogram.addEntry(tStart, tEnd, fAngle);
                        }
                    }
                    {
                        // EXE-INLINED none: hkInplaceArray<hkpShapeKey, 128>::~hkInplaceArray
                        hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
                        const int32_t iCapacityAndFlags = hits.m_capacityAndFlags;
                        hits.m_size = 0;
                        if (iCapacityAndFlags >= 0) {
                            pHeap->bufFree(
                                hits.m_data,
                                (iCapacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK) *
                                    static_cast<int32_t>(sizeof(uint32_t))
                            );
                        }
                    }
                }
                const float fEdgeAngle = histogram.evaluate();
                const int32_t iAngleBitcode = hkpMeshWeldingUtility::calcEdgeAngleBitcode(fEdgeAngle);
                combinedBitcode =
                    hkpMeshWeldingUtility::modifyCombinedEdgesBitcode(combinedBitcode, edge, iAngleBitcode);
                {
                    // EXE-INLINED none: hkArray<hkEntry>::~hkArray
                    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
                    const int32_t iCapacityAndFlags = histogram.m_entries.m_capacityAndFlags;
                    histogram.m_entries.m_size = 0;
                    if (iCapacityAndFlags >= 0) {
                        pHeap->bufFree(
                            histogram.m_entries.m_data,
                            (iCapacityAndFlags & hkArrayBase<hkEntry>::CAPACITY_MASK) *
                                static_cast<int32_t>(sizeof(hkEntry))
                        );
                    }
                    histogram.m_entries.m_data = nullptr;
                    histogram.m_entries.m_capacityAndFlags = hkArrayBase<hkEntry>::DONT_DEALLOCATE_FLAG;
                }
            }
            mesh->setWeldingInfo(key, static_cast<int16_t>(combinedBitcode));
        }
        pContainer = &static_cast<hkpShapeContainer&>(*mesh);
        key = pContainer->getNextKey(key);
    }
    while (key != 0xFFFFFFFF);
}

// The single mesh's welding: one shape info (the tree at the identity) in a local array of capacity one, handed to
// computeWeldingInfoMultiShape with the identity transform.
void hkpMeshWeldingUtility::computeWeldingInfo(
    hkpShapeCollection* collection,
    const hkpBvTreeShape* bvTree,
    hkpWeldingUtility::WeldingType weldingType,
    bool weldOpenEdges
) {
    hkpMeshWeldingUtility::ShapeInfo info;
    info.m_transform = hkTransformIdentity;
    info.m_shape = bvTree;
    // The stack allocator's bytes are untyped memory; the local array keeps its shape infos there.
    XFNV_INLINED_LOCAL(hkLocalArray<hkpMeshWeldingUtility::ShapeInfo>, shapes, std::nullopt, std::nullopt);
    {
        // EXE-INLINED none: hkLocalArray<hkpMeshWeldingUtility::ShapeInfo>::hkLocalArray(1) (hkAllocateStack)
        shapes.m_data = nullptr;
        shapes.m_size = 0;
        shapes.m_capacityAndFlags = hkArrayBase<hkpMeshWeldingUtility::ShapeInfo>::DONT_DEALLOCATE_FLAG;
        hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
        char* pBlock = stack.m_next.m_storage;
        stack.m_next.m_storage =
            pBlock + ((static_cast<int32_t>(sizeof(hkpMeshWeldingUtility::ShapeInfo)) + 127) & ~127);
        shapes.m_data = reinterpret_cast<hkpMeshWeldingUtility::ShapeInfo*>(pBlock);
        shapes.m_capacityAndFlags = 1 | hkArrayBase<hkpMeshWeldingUtility::ShapeInfo>::DONT_DEALLOCATE_FLAG;
        shapes.m_localMemory = shapes.m_data;
    }
    {
        // EXE-INLINED none: hkArray<hkpMeshWeldingUtility::ShapeInfo>::pushBack
        hkMemoryRouter& router = hkMemoryRouter::getInstance();
        if (shapes.m_size ==
            (shapes.m_capacityAndFlags & hkArrayBase<hkpMeshWeldingUtility::ShapeInfo>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(
                *router.m_heap, &shapes, static_cast<int32_t>(sizeof(hkpMeshWeldingUtility::ShapeInfo))
            );
        }
        hkpMeshWeldingUtility::ShapeInfo* pInfo = shapes.m_data + shapes.m_size;
        if (pInfo) {
            ::new (pInfo) hkpMeshWeldingUtility::ShapeInfo(info);
        }
        ++shapes.m_size;
    }
    hkpMeshWeldingUtility::computeWeldingInfoMultiShape(
        hkTransformIdentity, collection, weldingType, shapes, weldOpenEdges
    );
    {
        // EXE-INLINED none: hkLocalArray<hkpMeshWeldingUtility::ShapeInfo>::~hkLocalArray
        hkpMeshWeldingUtility::ShapeInfo* pLocalMemory = shapes.m_localMemory;
        if (pLocalMemory == shapes.m_data) {
            shapes.m_size = 0;
        }
        // EXE-INLINED none: hkDeallocateStack<hkpMeshWeldingUtility::ShapeInfo>
        hkMemoryRouter::getInstance().m_stack.m_next.m_storage = static_cast<char*>(static_cast<void*>(pLocalMemory));
        hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
        const int32_t iCapacityAndFlags = shapes.m_capacityAndFlags;
        shapes.m_size = 0;
        if (iCapacityAndFlags >= 0) {
            pHeap->bufFree(
                shapes.m_data,
                (iCapacityAndFlags & hkArrayBase<hkpMeshWeldingUtility::ShapeInfo>::CAPACITY_MASK) *
                    static_cast<int32_t>(sizeof(hkpMeshWeldingUtility::ShapeInfo))
            );
        }
    }
}

float hkpMeshWeldingUtility::calcAngleFromVertices(const hkVector4* vertices, float& sinAngleOut, float& cosAngleOut) {
    __m128 v8 = _mm_undefined_ps();
    hkVector4 normal;
    hkVector4 v19;
    hkpMeshWeldingUtility::calcAntiClockwiseTriangleNormal(*vertices, *(vertices + 1), *(vertices + 2), normal);
    hkpMeshWeldingUtility::calcAntiClockwiseTriangleNormal(*(vertices + 1), *vertices, *(vertices + 3), v19);
    __m128 afX = _mm_sub_ps(vertices[1].m_quad, vertices->m_quad);
    __m128 v4 = _mm_mul_ps(afX, afX);
    float v5 = _mm_shuffle_ps(v4, v4, 85)[0] + v4[0];
    __m128 v6 = _mm_shuffle_ps(v4, v4, 170);
    v6[0] = v6[0] + v5;
    float v7 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v6[0])));
    v8[0] = std::bit_cast<float>(static_cast<int32_t>(0.0f));
    __m128 v9 = _mm_cmpeq_ss(v6, v8);
    v8[0] = 3.0f - ((v6[0] * v7) * v7);
    __m128 v10 = _mm_setzero_ps();
    v10[0] = (0.5f * v7) * v8[0];
    __m128 v11 = _mm_andnot_ps(v9, v10);
    __m128 v12 = _mm_mul_ps(_mm_shuffle_ps(v11, v11, 0), afX);
    __m128 v13 = _mm_mul_ps(
        _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(normal.m_quad, normal.m_quad, 210), _mm_shuffle_ps(v12, v12, 201)),
            _mm_mul_ps(_mm_shuffle_ps(normal.m_quad, normal.m_quad, 201), _mm_shuffle_ps(v12, v12, 210))
        ),
        v19.m_quad
    );
    float v14 = _mm_shuffle_ps(v13, v13, 85)[0] + v13[0];
    v8[0] = _mm_shuffle_ps(v13, v13, 170)[0];
    __m128 v15 = _mm_mul_ps(v19.m_quad, normal.m_quad);
    sinAngleOut = v8[0] + v14;
    afX[0] = _mm_shuffle_ps(v15, v15, 170)[0] + (_mm_shuffle_ps(v15, v15, 85)[0] + v15[0]);
    cosAngleOut = afX[0];
    return hkMath_atan2fApproximation(sinAngleOut, afX[0]);
}

// The welding bitcodes of the triangle's three edges; a degenerate triangle gets none and fails, as does one any edge
// of which fails.
hkResult hkpMeshWeldingUtility::calcWeldingInfoForTriangle(
    uint32_t shapeKey,
    const hkpBvTreeShape* bvTreeShape,
    hkpMeshWeldingUtility::WindingConsistency testConsistency,
    uint16_t& info
) {
    alignas(16) char buffer[512];
    const hkpShapeContainer* pContainer = bvTreeShape->getContainer();
    // The container's child shapes are triangles.
    const hkpTriangleShape* pTriangle =
        static_cast<const hkpTriangleShape*>(pContainer->getChildShape(shapeKey, buffer));
    hkBool bDegenerate;
    if (hkpTriangleUtil::isDegenerate(
            &bDegenerate, pTriangle->m_vertexA, pTriangle->m_vertexB, pTriangle->m_vertexC,
            hkDefaultTriangleDegeneracyTolerance
        )
            ->m_bool) {
        info = 0;
        return HK_FAILURE;
    }
    uint16_t sBitcode = 0;
    hkResult eResult = HK_SUCCESS;
    for (int32_t i = 0; i < 3; ++i) {
        if (hkpMeshWeldingUtility::calcBitcodeForTriangleEdge(
                bvTreeShape, pTriangle, shapeKey, i, testConsistency, sBitcode
            ) == HK_FAILURE) {
            eResult = HK_FAILURE;
        }
    }
    info = sBitcode;
    return eResult;
}

// Whether the triangle's winding agrees with its neighbours': each of its three edges' bitcodes computed with the
// winding test off, the warning hkpMeshWeldingUtility.cpp:713 raises (0xFE98751E) switched off for the time if it was
// on.
hkBool*
hkpMeshWeldingUtility::isTriangleWindingValid(hkBool* apResult, uint32_t shapeKey, const hkpBvTreeShape* bvTreeShape) {
    alignas(16) char buffer[512];
    const hkpShapeContainer* pContainer = bvTreeShape->getContainer();
    // The container's child shapes are triangles.
    const hkpTriangleShape* pTriangle =
        static_cast<const hkpTriangleShape*>(pContainer->getChildShape(shapeKey, buffer));
    hkBool bWasEnabled;
    hkError::s_instance->isEnabled(&bWasEnabled, static_cast<int32_t>(0xFE98751E));
    if (bWasEnabled.m_bool) {
        hkError::s_instance->setEnabled(static_cast<int32_t>(0xFE98751E), hkBool(false));
    }
    apResult->m_bool = 1;
    uint16_t sBitcode = 0;
    for (int32_t i = 0; i < 3; ++i) {
        if (hkpMeshWeldingUtility::calcBitcodeForTriangleEdge(
                bvTreeShape, pTriangle, shapeKey, i, hkpMeshWeldingUtility::WINDING_IGNORE_CONSISTENCY, sBitcode
            ) == HK_FAILURE) {
            apResult->m_bool = 0;
            break;
        }
    }
    if (bWasEnabled.m_bool) {
        hkError::s_instance->setEnabled(static_cast<int32_t>(0xFE98751E), hkBool(true));
    }
    return apResult;
}

int32_t hkpMeshWeldingUtility::calcEdgeAngleBitcode(hkVector4* a1) {
    int32_t a2;
    float a3;
    a3 = hkpMeshWeldingUtility::calcAngleFromVertices(a1, *reinterpret_cast<float*>(&a2), a3);
    double v1 = a3;
    int32_t v2 = 0;
    a2 = 0;
    while (true) {
        a3 = static_cast<double>(a2) * 0.20943951606750488 - 3.1415927410125732;
        if (a3 >= v1) {
            break;
        }
        a2 = ++v2;
        if (v2 >= 31) {
            return v2;
        }
    }
    if (!(v1 > 0.0) || a3 == v1) {
        return v2;
    }

    return v2 - 1;
}

void hkHistogram::addEntry(float lo, float hi, float value) {
    hkEntry t;
    int32_t m_size = m_entries.m_size;
    int32_t v6 = 0;
    if (m_size > 0) {
        hkEntry* m_data = m_entries.begin();
        while (!(m_data->m_lo < static_cast<double>(lo))) {
            ++v6;
            ++m_data;
            if (v6 >= m_size) {
                goto LABEL_7;
            }
        }
        m_size = v6 + 1;
    }
LABEL_7:;
    t.m_lo = lo;
    t.m_hi = hi;
    t.m_value = value;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    m_entries._insertAt<hkEntry>(*Instance->m_heap, m_size, t);
}

// The histogram's angle: the entries sorted by value, each run of values equal within 0.001 summed into one range
// (from zero, its width clamped to [0, 1], the run's first value), then the smallest value among the ranges at least
// 0.6 wide, zero when there is none. An empty histogram gives its free angle.
float hkHistogram::evaluate() {
    const int32_t iNumEntries = m_entries.m_size;
    if (iNumEntries == 0) {
        return m_freeAngle;
    }
    if (iNumEntries > 1) {
        hkAlgorithm::quickSortRecursive<hkEntry, hkAlgorithm::less<hkEntry>>(
            m_entries.m_data, 0, iNumEntries - 1, hkAlgorithm::less<hkEntry>()
        );
    }
    XFNV_INLINED_LOCAL(hkArray<hkEntry>, ranges, std::nullopt, std::nullopt);
    // EXE-INLINED none: hkArray<hkEntry>::hkArray
    ranges.m_data = nullptr;
    ranges.m_size = 0;
    ranges.m_capacityAndFlags = hkArrayBase<hkEntry>::DONT_DEALLOCATE_FLAG;
    int32_t iStart = 0;
    for (int32_t i = 1; i <= m_entries.m_size; ++i) {
        if (i < m_entries.m_size) {
            bool bDiffer;
            if (!*entriesDiffer(&bDiffer, &m_entries.m_data[iStart], &m_entries.m_data[i])) {
                continue;
            }
        }
        // The widths summed in order, the sum stored as a float after each.
        float fSum = 0.0f;
        const int32_t iCount = i - iStart;
        for (int32_t j = 0; j < iCount; ++j) {
            const hkEntry& kEntry = m_entries.m_data[iStart + j];
            fSum = (kEntry.m_hi - kEntry.m_lo) + fSum;
        }
        float fValue = m_entries.m_data[iStart].m_value;
        float fWidth;
        if (fSum < 0.0f) {
            fWidth = 0.0f;
        }
        else if (1.0f < fSum) {
            fWidth = 1.0f;
        }
        else {
            fWidth = fSum;
        }
        {
            // EXE-INLINED none: hkArray<hkEntry>::pushBack
            hkMemoryRouter& router = hkMemoryRouter::getInstance();
            if (ranges.m_size == (ranges.m_capacityAndFlags & hkArrayBase<hkEntry>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(*router.m_heap, &ranges, static_cast<int32_t>(sizeof(hkEntry)));
            }
            hkEntry* pRange = ranges.m_data + ranges.m_size;
            if (pRange) {
                pRange->m_lo = 0.0f;
                pRange->m_hi = fWidth;
                pRange->m_value = fValue;
            }
            ++ranges.m_size;
        }
        iStart = i;
    }
    float fBest = 3.1415927f;
    bool bFound = false;
    for (int32_t k = 0; k < ranges.m_size; ++k) {
        const hkEntry& kRange = ranges.m_data[k];
        if (!(kRange.m_hi - kRange.m_lo < 0.6000000238418579)) {
            float fValue = kRange.m_value;
            if (!(fValue > fBest)) {
                fBest = fValue;
            }
            bFound = true;
        }
    }
    const float fResult = bFound ? fBest : 0.0f;
    {
        // EXE-INLINED none: hkArray<hkEntry>::~hkArray
        hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
        const int32_t iCapacityAndFlags = ranges.m_capacityAndFlags;
        ranges.m_size = 0;
        if (iCapacityAndFlags >= 0) {
            pHeap->bufFree(
                ranges.m_data,
                (iCapacityAndFlags & hkArrayBase<hkEntry>::CAPACITY_MASK) * static_cast<int32_t>(sizeof(hkEntry))
            );
        }
    }
    return fResult;
}

hkStreamStatisticsCollector::hkStreamStatisticsCollector(hkVtableClassRegistry* vtblReg, int32_t memSize)
  : hkStatisticsCollector(vtblReg) {
    // A stream buffer of memSize bytes, empty.
    if (memSize) {
        int32_t iSize = memSize;
        m_stream.m_data = static_cast<char*>(hkMemoryRouter::getInstance().m_heap->bufAlloc(iSize));
        m_stream.m_capacityAndFlags = iSize;
    }
    m_stream.m_size = 0;
}

// The known-object map, the stream and the base clean up after themselves.
hkStreamStatisticsCollector::~hkStreamStatisticsCollector() {}

// Copies a command stream in and, when asked, byte-swaps each command's two 16-bit sizes.
void hkStreamStatisticsCollector::setDataByCopying(const char* buffer, int32_t length, hkBool endianSwap) {
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    int32_t iCapacity = m_stream.getCapacityAndFlags() & hkArrayBase<char>::CAPACITY_MASK;
    if (iCapacity < length) {
        int32_t iNewCapacity = 2 * iCapacity;
        if (length >= iNewCapacity) {
            iNewCapacity = length;
        }
        hkArrayUtil::_reserve(pHeap, &m_stream, iNewCapacity, sizeof(char));
    }
    m_stream.setSizeUnchecked(length);
    hkString::memCpy(m_stream.begin(), buffer, length);
    // The byte at +1 holds the command's length, which is read after swapping the two sizes that follow it.
    if (endianSwap.m_bool) {
        for (int32_t i = 0; i < m_stream.getSize();) {
            // The stream is untyped bytes walked as commands; the cast is over that raw memory.
            Command* pCommand = reinterpret_cast<Command*>(m_stream.begin() + i);
            uint16_t uiMemSize = pCommand->m_memSize;
            pCommand->m_memSize = static_cast<uint16_t>((uiMemSize << 8) | (uiMemSize >> 8));
            uint16_t uiAllocSize = pCommand->m_allocSize;
            pCommand->m_allocSize = static_cast<uint16_t>((uiAllocSize << 8) | (uiAllocSize >> 8));
            i += pCommand->m_slen + 6;
        }
    }
}
