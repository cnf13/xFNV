#include "Havok/Source/Common/Internal/Collide/Gsk/hkConvexPenetrationUtil.h"

#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Internal/Collide/Gsk/hkGsk.h"
#include "Havok/Source/Common/Internal/Collide/Gsk/hkWingedEdge.h"
#include "xfnv/Compiler.h"

#include <cmath>
#include <xmmintrin.h>

static void calcBarycentricCoordinates(__m128* a1, __m128* a2, __m128* a3, __m128* a4, float* a5);

// The SDK's file static (hkInternal.lib's hkConvexPenetrationUtil.obj names it; .rdata): the width of the band in
// which the polytope's triangles count as visible.
static const float HK_WINGED_EDGE_VISIBILITY_TOLERANCE_WIDTH = 1e-07f;

static void calcBarycentricCoordinates(__m128* a1, __m128* a2, __m128* a3, __m128* a4, float* a5) {
    __m128 v5 = *a3;
    __m128 v6 = *a4;
    __m128 v7 = _mm_sub_ps(*a2, *a3);
    __m128 v8 = _mm_mul_ps(v7, v7);
    float v37 = XFNV_M128_F32(_mm_shuffle_ps(v8, v8, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v8, v8, 85), 0) + XFNV_M128_F32(v8, 0));
    __m128 v9 = _mm_sub_ps(*a4, *a3);
    __m128 v10 = _mm_mul_ps(v9, v9);
    float v39 = XFNV_M128_F32(_mm_shuffle_ps(v10, v10, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v10, v10, 85), 0) + XFNV_M128_F32(v10, 0));
    __m128 v11 = _mm_mul_ps(v9, v7);
    double v12 =
        (XFNV_M128_F32(_mm_shuffle_ps(v11, v11, 170), 0) +
         (XFNV_M128_F32(_mm_shuffle_ps(v11, v11, 85), 0) + XFNV_M128_F32(v11, 0)));
    double v13 = v37;
    double v14 = v39;
    float v40 = v37 * v39;
    double v15 = v40;
    float v41 = v12 * v12;
    double v17 = v15 - v41;
    double v18 = v14;
    float v42 = v17;
    if (!(v42 > 0.0f)) {
        __m128 v27 = _mm_sub_ps(*a2, v6);
        __m128 v28 = _mm_mul_ps(v27, v27);
        float v36 = XFNV_M128_F32(_mm_shuffle_ps(v28, v28, 170), 0) +
            (XFNV_M128_F32(_mm_shuffle_ps(v28, v28, 85), 0) + XFNV_M128_F32(v28, 0));
        if (!(v13 >= v18)) {
            if (!(v36 >= v18)) {
                __m128 v33 = *a1;
                *a5 = 0.0f;
                __m128 v34 = _mm_mul_ps(_mm_sub_ps(v33, v5), v9);
                float v47 = (XFNV_M128_F32(_mm_shuffle_ps(v34, v34, 170), 0) +
                             (XFNV_M128_F32(_mm_shuffle_ps(v34, v34, 85), 0) + XFNV_M128_F32(v34, 0))) /
                    v18;
                a5[2] = v47;
                a5[1] = 1.0 - v47;
            }
            else {
                __m128 v32 = _mm_mul_ps(_mm_sub_ps(*a1, v6), v27);
                float v46 = (XFNV_M128_F32(_mm_shuffle_ps(v32, v32, 170), 0) +
                             (XFNV_M128_F32(_mm_shuffle_ps(v32, v32, 85), 0) + XFNV_M128_F32(v32, 0))) /
                    v36;
                *a5 = v46;
                a5[1] = 0.0f;
                a5[2] = 1.0 - v46;
            }
        }
        else {
            double v29 = v36;
            if (!(v36 >= static_cast<double>(v37))) {
                __m128 v31 = _mm_mul_ps(_mm_sub_ps(*a1, v5), v7);
                float v45 = (XFNV_M128_F32(_mm_shuffle_ps(v31, v31, 170), 0) +
                             (XFNV_M128_F32(_mm_shuffle_ps(v31, v31, 85), 0) + XFNV_M128_F32(v31, 0))) /
                    v37;
                *a5 = v45;
                a5[1] = 1.0 - v45;
                a5[2] = 0.0f;
            }
            else {
                a5[1] = 0.0f;
                if (!(v29 > 0.0)) {
                    *a5 = 0.0f;
                    a5[2] = 0.0f;
                }
                else {
                    __m128 v30 = _mm_mul_ps(_mm_sub_ps(*a1, *a4), v27);
                    float v44 = (XFNV_M128_F32(_mm_shuffle_ps(v30, v30, 170), 0) +
                                 (XFNV_M128_F32(_mm_shuffle_ps(v30, v30, 85), 0) + XFNV_M128_F32(v30, 0))) /
                        v29;
                    *a5 = v44;
                    a5[2] = 1.0 - v44;
                }
            }
        }
    }
    else {
        double v19 = v42;
        __m128 v20 = _mm_sub_ps(v5, *a1);
        __m128 v21 = _mm_mul_ps(v20, v7);
        float v22 = XFNV_M128_F32(_mm_shuffle_ps(v21, v21, 85), 0) + XFNV_M128_F32(v21, 0);
        float v23 = XFNV_M128_F32(_mm_shuffle_ps(v21, v21, 170), 0);
        __m128 v24 = _mm_mul_ps(v20, v9);
        float v25 = XFNV_M128_F32(_mm_shuffle_ps(v24, v24, 170), 0) +
            (XFNV_M128_F32(_mm_shuffle_ps(v24, v24, 85), 0) + XFNV_M128_F32(v24, 0));
        float v35 = 1.0 / v42;
        double v26 = (v23 + v22);
        float v43 = v25 * v12 - v18 * v26;
        float v38 = v26 * v12 - v13 * v25;
        *a5 = v43 * v35;
        a5[1] = (v19 - v43 - v38) * v35;
        a5[2] = v35 * v38;
    }
}

// KEEP: the exe body writes only eax (it saves and restores ecx), and randomVector keeps its output vector in edx
// across the call, which clang does not guarantee: it stores through edx at 0xE62B0B after the call at 0xE62AF0. The
// exe rounds the result to a float (fstp/fld dword) before returning it in ST0. The LCG step is computed unsigned: the
// exe's imul wraps.
double getRandomReal01(int32_t r_eax) {
    const int32_t bits = static_cast<int32_t>((0x3039u - 0x3E39B193u * static_cast<uint32_t>(r_eax)) & 0x7FFFFFFFu);
    return static_cast<float>(static_cast<double>(bits) * 4.656612873077393e-10);
}

// Adds a1 to getRandomReal01's random float for the seed r_eax, multiplies by (a2 - a1), rounds the result to a float
// once and stores it into the three floats at r_edx. r_eax is passed to getRandomReal01 unchanged.
void randomVector(float a1, float a2, int32_t r_eax, float* r_edx) {
    const float fValue = (getRandomReal01(r_eax) + a1) * (a2 - a1);
    r_edx[0] = fValue;
    r_edx[1] = fValue;
    r_edx[2] = fValue;
}

// B's support point is found along the negated direction rotated into B's space (the transpose of m_aTb's rotation),
// then transformed back by m_aTb and subtracted from A's.
void hkConvexPenetrationUtil::getPointForSimplex(const hkVector4& direction, hkWingedEdge::Vertex& result) {
    hkVector4 v5;
    hkVector4 v6;
    hkVector4 v7;
    hkVector4 pointBinB;
    hkVector4 directionInB;
    __m128 v3 = _mm_xor_ps(direction.m_quad, hkQuadRealNegateMask);
    v5.m_quad = m_aTb.m_rotation.m_col0.m_quad;
    v6.m_quad = m_aTb.m_rotation.m_col1.m_quad;
    v7.m_quad = m_aTb.m_rotation.m_col2.m_quad;
    __m128 v8 = _mm_unpacklo_ps(m_aTb.m_rotation.m_col0.m_quad, v6.m_quad);
    __m128 v9 = _mm_movelh_ps(v8, v7.m_quad);
    directionInB.m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(v3, v3, 85), _mm_shuffle_ps(_mm_movehl_ps(v9, v8), v7.m_quad, 212)),
            _mm_mul_ps(_mm_shuffle_ps(v3, v3, 0), v9)
        ),
        _mm_mul_ps(_mm_shuffle_ps(_mm_unpackhi_ps(v5.m_quad, v6.m_quad), v7.m_quad, 228), _mm_shuffle_ps(v3, v3, 170))
    );
    m_shapeA.m_storage->getSupportingVertex(direction, &result.m_pointAinA);
    m_shapeB.m_storage->getSupportingVertex(directionInB, &pointBinB);
    result.m_position.m_quad = _mm_sub_ps(
        result.m_pointAinA.m_quad,
        _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(pointBinB.m_quad, pointBinB.m_quad, 85), m_aTb.m_rotation.m_col1.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(pointBinB.m_quad, pointBinB.m_quad, 0), m_aTb.m_rotation.m_col0.m_quad)
            ),
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(pointBinB.m_quad, pointBinB.m_quad, 170), m_aTb.m_rotation.m_col2.m_quad),
                m_aTb.m_translation.m_quad
            )
        )
    );
    result.m_pointBinB.m_quad = pointBinB.m_quad;
}

void hkConvexPenetrationUtil::addPointToSimplex(const hkWingedEdge::Vertex& p) {
    int32_t m_storage = m_simplexSize.m_storage;
    m_simplex.m_storage[m_storage] = p.m_position;
    m_pointsAinA.m_storage[m_storage] = p.m_pointAinA;
    m_pointsBinB.m_storage[m_storage] = p.m_pointBinB;
    m_simplexSize.m_storage = m_storage + 1;
}

hkVector4 hkConvexPenetrationUtil::findBestProjection(
    const hkVector4& direction,
    const hkVector4& from,
    hkWingedEdge::Vertex& result
) {
    hkVector4 v6;
    hkVector4 v14;
    hkVector4 v17;
    hkWingedEdge::Vertex resulta;
    v17.m_quad = _mm_xor_ps(direction.m_quad, hkQuadRealNegateMask);
    getPointForSimplex(direction, resulta);
    getPointForSimplex(v17, result);
    v6.m_quad = resulta.m_position.m_quad;
    __m128 v8 = _mm_mul_ps(_mm_sub_ps(resulta.m_position.m_quad, from.m_quad), direction.m_quad);
    float v9 = XFNV_M128_F32(_mm_shuffle_ps(v8, v8, 85), 0) + XFNV_M128_F32(v8, 0);
    __m128 v10 = _mm_shuffle_ps(v8, v8, 170);
    __m128 v11 = _mm_mul_ps(_mm_sub_ps(result.m_position.m_quad, from.m_quad), v17.m_quad);
    float v12 = XFNV_M128_F32(_mm_shuffle_ps(v11, v11, 85), 0) + XFNV_M128_F32(v11, 0);
    __m128 v13 = _mm_shuffle_ps(v11, v11, 170);
    XFNV_M128_F32(v13, 0) = XFNV_M128_F32(v13, 0) + v12;
    XFNV_M128_F32(v10, 0) = XFNV_M128_F32(v10, 0) + v9;
    v14.m_quad = _mm_max_ps(v10, v13);
    XFNV_M128_F32(v13, 0) = XFNV_M128_F32(v13, 0) - XFNV_M128_F32(v10, 0);
    if ((_mm_movemask_ps(v13) & 1) != 0) {
        hkVector4 v16;
        hkWingedEdge::Edge* m_storage = resulta.m_referee.m_storage;
        result.m_pointAinA = resulta.m_pointAinA;
        v16.m_quad = resulta.m_pointBinB.m_quad;
        result.m_position.m_quad = v6.m_quad;
        result.m_pointBinB.m_quad = v16.m_quad;
        result.m_referee.m_storage = m_storage;
    }
    return v14;
}

// Grows the simplex from its m_simplexSize points to a tetrahedron around the origin, one support point per step; iStep
// is the last size grown from (-1: none yet), so that a step that falls back below it gives up. HK_SUCCESS (status OK):
// the tetrahedron is ready for the expansion. HK_FAILURE with status OK: the simplex reaches the origin within the
// tolerance and output already holds the answer. HK_FAILURE with HK_GSK_PD_UNSOLVABLE: no tetrahedron.
hkResult hkConvexPenetrationUtil::builtTetrahedron(hkGskOut& output, hkGskStatus& gjkStatus) {
    int32_t iStep = -1;
    hkVector4 triangleNormal;
    switch (m_simplexSize.m_storage) {
        case 0: goto empty;
        case 1: goto point;
        case 2: goto edge;
        case 3: goto triangle;
        case 4: goto tetrahedron;
        default: gjkStatus = hkGskStatus::HK_GSK_PD_UNSOLVABLE; return HK_FAILURE;
    }

tetrahedron:
    {
        // Done once the fourth point lies off the plane of the other three by more than the tolerance.
        hkVector4* pSimplex = m_simplex.m_storage;
        hkVector4 edge0;
        hkVector4 edge1;
        hkVector4 normal;
        hkVector4 toFourth;
        edge0.m_quad = _mm_sub_ps(pSimplex[0].m_quad, pSimplex[1].m_quad);
        edge1.m_quad = _mm_sub_ps(pSimplex[1].m_quad, pSimplex[2].m_quad);
        normal.setCross(edge0, edge1);
        const float fNormalLength2 = normal.dot3(normal);
        toFourth.m_quad = _mm_sub_ps(pSimplex[3].m_quad, pSimplex[0].m_quad);
        const float fDistance = toFourth.dot3(normal);
        // Rounds the limit to a float.
        const float fLimit = m_epsTolSqrd.m_storage * fNormalLength2;
        if (!(fLimit >= fDistance * fDistance)) {
            gjkStatus = hkGskStatus::HK_GSK_OK;
            return HK_SUCCESS;
        }
        if (iStep >= 3) {
            gjkStatus = hkGskStatus::HK_GSK_PD_UNSOLVABLE;
            return HK_FAILURE;
        }
        m_simplexSize.m_storage = 3;
    }

triangle:
    {
        hkVector4* pSimplex = m_simplex.m_storage;
        hkVector4 edge0;
        hkVector4 edge1;
        edge0.m_quad = _mm_sub_ps(pSimplex[0].m_quad, pSimplex[1].m_quad);
        edge1.m_quad = _mm_sub_ps(pSimplex[1].m_quad, pSimplex[2].m_quad);
        triangleNormal.setCross(edge0, edge1);
        const float fNormalLength2 = triangleNormal.dot3(triangleNormal);
        if (0.0f != fNormalLength2) {
            goto expandTriangle;
        }
        if (iStep >= 2) {
            // A degenerate triangle after an edge step: the answer is on the edge s0 s1, its normal any perpendicular.
            hkVector4 edge;
            edge.m_quad = _mm_sub_ps(pSimplex[0].m_quad, pSimplex[1].m_quad);
            hkVector4 perpendicular;
            hkVector4Util::calculatePerpendicularVector(edge, perpendicular);
            perpendicular.normalize3();
            output.m_normalInA.m_quad = _mm_and_ps(perpendicular.m_quad, rdata_10C4C10);
            // The point on A at the ratio sum|s0| / (2 sum|s0| + FLT_EPSILON) from A's first point to its second.
            const __m128 absS0 = _mm_and_ps(m_simplex.m_storage[0].m_quad, hkVector4_setAbs4_absMask);
            const __m128 sum =
                _mm_add_ss(_mm_shuffle_ps(absS0, absS0, 0xAA), _mm_add_ss(_mm_shuffle_ps(absS0, absS0, 0x55), absS0));
            __m128 t = _mm_div_ss(sum, _mm_add_ss(_mm_add_ss(sum, sum), _mm_set_ss(1.1920929e-07f)));
            t = _mm_shuffle_ps(t, t, 0);
            const hkVector4* pPointsAinA = m_pointsAinA.m_storage;
            output.m_pointAinA.m_quad = _mm_add_ps(
                _mm_mul_ps(_mm_sub_ps(_mm_set1_ps(1.0f), t), pPointsAinA[0].m_quad),
                _mm_mul_ps(pPointsAinA[1].m_quad, t)
            );
            gjkStatus = hkGskStatus::HK_GSK_OK;
            return HK_FAILURE;
        }
        m_simplexSize.m_storage = 2;
    }

edge:
    {
        hkVector4 edge;
        edge.m_quad = _mm_sub_ps(m_simplex.m_storage[0].m_quad, m_simplex.m_storage[1].m_quad);
        const float fEdgeLength2 = edge.dot3(edge);
        if (!(m_epsTolSqrd.m_storage >= fEdgeLength2)) {
            goto expandEdge;
        }
        if (iStep >= 1) {
            gjkStatus = hkGskStatus::HK_GSK_PD_UNSOLVABLE;
            return HK_FAILURE;
        }
        m_simplexSize.m_storage = 1;
    }

point:
    {
        const float fLength2 = m_simplex.m_storage[0].dot3(m_simplex.m_storage[0]);
        if (!(m_epsTolSqrd.m_storage >= fLength2)) {
            goto expandPoint;
        }
        if (iStep >= 0) {
            gjkStatus = hkGskStatus::HK_GSK_PD_UNSOLVABLE;
            return HK_FAILURE;
        }
        m_simplexSize.m_storage = 0;
    }

empty:
    {
        // The first point along the negated translation, or along y where A and B are about centred.
        hkVector4 direction;
        hkWingedEdge::Vertex support;
        const float fTranslationLength2 = m_aTb.m_translation.dot3(m_aTb.m_translation);
        direction.m_quad = _mm_xor_ps(m_aTb.m_translation.m_quad, hkQuadRealNegateMask);
        if (m_epsTolSqrd.m_storage >= fTranslationLength2) {
            direction.m_quad = hkTransformIdentity.m_rotation.m_col1.m_quad;
        }
        getPointForSimplex(direction, support);
        addPointToSimplex(support);
        const float fLength2 = m_simplex.m_storage[0].dot3(m_simplex.m_storage[0]);
        if (m_epsTolSqrd.m_storage >= fLength2) {
            // The shapes touch at that point: the normal is the direction, its w the distance.
            const float fDistance = m_simplex.m_storage[0].length3();
            output.m_pointAinA.m_quad = support.m_pointAinA.m_quad;
            direction.normalize3();
            const float fZ = direction(2);
            const float fY = direction(1);
            output.m_normalInA.m_quad = _mm_unpacklo_ps(
                _mm_unpacklo_ps(direction.m_quad, _mm_set_ss(fZ)),
                _mm_unpacklo_ps(_mm_set_ss(fY), _mm_set_ss(fDistance))
            );
            gjkStatus = hkGskStatus::HK_GSK_OK;
            return HK_FAILURE;
        }
        iStep = 0;
        goto point;
    }

expandPoint:
    {
        // The second point: the support along the first point or against it, whichever lies further from it.
        hkVector4* pSimplex = m_simplex.m_storage;
        hkWingedEdge::Vertex along;
        hkWingedEdge::Vertex against;
        hkVector4 direction;
        hkVector4 offset;
        iStep = 1;
        getPointForSimplex(*pSimplex, along);
        offset.m_quad = _mm_sub_ps(along.m_position.m_quad, m_simplex.m_storage[0].m_quad);
        const float fAlongDistance2 = offset.dot3(offset);
        direction.m_quad = _mm_xor_ps(pSimplex->m_quad, hkQuadRealNegateMask);
        getPointForSimplex(direction, against);
        offset.m_quad = _mm_sub_ps(against.m_position.m_quad, m_simplex.m_storage[0].m_quad);
        const float fAgainstDistance2 = offset.dot3(offset);
        if (!(fAgainstDistance2 <= fAlongDistance2)) {
            addPointToSimplex(against);
        }
        else {
            addPointToSimplex(along);
        }
        goto edge;
    }

expandEdge:
    {
        // The third point: the better of the projections along a perpendicular of the edge and along the edge crossed
        // with that perpendicular.
        hkVector4* pSimplex = m_simplex.m_storage;
        hkVector4 edge;
        edge.m_quad = _mm_sub_ps(pSimplex[0].m_quad, pSimplex[1].m_quad);
        iStep = 2;
        hkVector4 perpendicular;
        hkVector4Util::calculatePerpendicularVector(edge, perpendicular);
        hkVector4 cross;
        cross.setCross(edge, perpendicular);
        perpendicular.normalize3();
        cross.normalize3();
        hkWingedEdge::Vertex alongPerpendicular;
        hkWingedEdge::Vertex alongCross;
        hkVector4 perpendicularDistance = findBestProjection(perpendicular, *pSimplex, alongPerpendicular);
        hkVector4 crossDistance = findBestProjection(cross, *m_simplex.m_storage, alongCross);
        if ((_mm_movemask_ps(_mm_sub_ss(crossDistance.m_quad, perpendicularDistance.m_quad)) & 1) != 0) {
            addPointToSimplex(alongPerpendicular);
        }
        else {
            addPointToSimplex(alongCross);
        }
        goto triangle;
    }

expandTriangle:
    {
        // The fourth point, along the triangle's normal or against it; where neither gets further than m_epsTol the
        // origin's projection on the triangle is the answer.
        hkVector4* pSimplex = m_simplex.m_storage;
        hkWingedEdge::Vertex support;
        triangleNormal.normalize3();
        iStep = 3;
        hkVector4 distance = findBestProjection(triangleNormal, *pSimplex, support);
        if ((_mm_movemask_ps(_mm_sub_ss(distance.m_quad, m_epsTol.m_quad)) & 1) == 0) {
            addPointToSimplex(support);
            goto tetrahedron;
        }
        output.m_normalInA.m_quad = _mm_and_ps(triangleNormal.m_quad, rdata_10C4C10);
        hkVector4* pTriangle = m_simplex.m_storage;
        alignas(16) float afWeights[3];
        calcBarycentricCoordinates(
            &hkVector4Zero.m_quad, &pTriangle[0].m_quad, &pTriangle[1].m_quad, &pTriangle[2].m_quad, afWeights
        );
        const __m128 weight0 = _mm_set_ss(afWeights[0]);
        const __m128 weight1 = _mm_set_ss(afWeights[1]);
        const __m128 weight2 = _mm_set_ss(afWeights[2]);
        const hkVector4* pPointsAinA = m_pointsAinA.m_storage;
        output.m_pointAinA.m_quad = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(weight1, weight1, 0), pPointsAinA[1].m_quad),
                _mm_mul_ps(_mm_shuffle_ps(weight0, weight0, 0), pPointsAinA[0].m_quad)
            ),
            _mm_mul_ps(_mm_shuffle_ps(weight2, weight2, 0), pPointsAinA[2].m_quad)
        );
        gjkStatus = hkGskStatus::HK_GSK_OK;
        return HK_FAILURE;
    }
}

// KEEP: uninitialised read. The backup step (0xE6399A..0xE639D0) sets x, y and z of the perturbation vector at
// [esp+0x20] and adds all four lanes to m_aTb's translation; its w ([esp+0x2C]) is written on no path of the
// function, so the exe stores the caller's stack garbage plus the translation's w into m_aTb.m_translation.w.
// The exe keeps the status, the boundary edge count and the perturbation's float temporary in shared frame slots.
hkGskStatus hkConvexPenetrationUtil::calculatePenetrationDepth(hkGskOut& output) {
    hkWingedEdge polytope;
    hkWingedEdge::Triangle* pTriangle;
    hkWingedEdge::Edge* pBoundaryEdges;
    int32_t iNumBoundaryEdges;
    hkGskStatus status;
    while (true) {
        status = hkGskStatus::HK_GSK_OK;
        if (builtTetrahedron(output, status) != HK_FAILURE) {
            polytope.initializePolytopeExpansion(m_pointsAinA.m_storage, m_pointsBinB.m_storage, m_simplex.m_storage);
            while (true) {
                pTriangle = polytope.findTriangleOnNearestPlane();
                hkWingedEdge::Vertex* pNewVertex = &polytope.m_vertex[polytope.m_numVertices.m_storage++];
                pNewVertex->m_referee.m_storage = nullptr;
                getPointForSimplex(pTriangle->m_normal, *pNewVertex);
                // The new vertex seen from each corner of the triangle, against the triangle's normal: done once any
                // lies below m_epsTol.
                __m128 v7 = _mm_sub_ps(pNewVertex->m_position.m_quad, pTriangle->m_edge[0].m_vertex->m_position.m_quad);
                __m128 v8 = _mm_sub_ps(pNewVertex->m_position.m_quad, pTriangle->m_edge[1].m_vertex->m_position.m_quad);
                __m128 v9 = _mm_sub_ps(pNewVertex->m_position.m_quad, pTriangle->m_edge[2].m_vertex->m_position.m_quad);
                __m128 v10 = _mm_unpacklo_ps(v7, v8);
                __m128 v11 = _mm_movelh_ps(v10, v9);
                const __m128 normal = pTriangle->m_normal.m_quad;
                __m128 dots = _mm_add_ps(
                    _mm_add_ps(
                        _mm_mul_ps(
                            _mm_shuffle_ps(_mm_movehl_ps(v11, v10), v9, 212), _mm_shuffle_ps(normal, normal, 85)
                        ),
                        _mm_mul_ps(_mm_shuffle_ps(normal, normal, 0), v11)
                    ),
                    _mm_mul_ps(_mm_shuffle_ps(_mm_unpackhi_ps(v7, v8), v9, 228), _mm_shuffle_ps(normal, normal, 170))
                );
                if ((_mm_movemask_ps(_mm_cmplt_ps(dots, m_epsTol.m_quad)) & 7) != 0) {
                    status = hkGskStatus::HK_GSK_OK;
                    goto done;
                }
                polytope.m_toleranceBorder.m_storage = HK_WINGED_EDGE_VISIBILITY_TOLERANCE_WIDTH;
                polytope.m_previousMinimumDistance.m_storage = 3.40282e+38f;
                pBoundaryEdges = nullptr;
                iNumBoundaryEdges = 0;
                hkWingedEdge::Triangle::TriangleState state = pTriangle->calcStateAndUpdateTolerance(
                    pNewVertex->m_position, &polytope.m_toleranceBorder.m_storage,
                    &polytope.m_previousMinimumDistance.m_storage
                );
                pTriangle->m_state = state;
                if (state != hkWingedEdge::Triangle::HK_VISIBLE) {
                    goto backup;
                }
                if (polytope.findVisibleTriangles(
                        pTriangle, pNewVertex->m_position, iNumBoundaryEdges, pBoundaryEdges
                    ) != 0) {
                    goto backup;
                }
                if (polytope.m_numFreeTriangle.m_storage - polytope.m_numTriangles.m_storage +
                        hkWingedEdge::HK_WINGED_EDGE_TRIANGLE_LIMIT <
                    iNumBoundaryEdges) {
                    status = hkGskStatus::HK_GSK_PD_OUT_OF_MEMORY;
                    goto done;
                }
                if (polytope.patchInBoundaryToPointOfViewExpansion(iNumBoundaryEdges, pBoundaryEdges, pNewVertex) !=
                    0) {
                    goto backup;
                }
                if (polytope.m_numVertices.m_storage >= hkWingedEdge::HK_WINGED_EDGE_VERTICIES_LIMIT) {
                    status = hkGskStatus::HK_GSK_PD_OUT_OF_MEMORY;
                    goto done;
                }
            }
        }
        if (status != hkGskStatus::HK_GSK_PD_UNSOLVABLE) {
            return status;
        }
    backup:
        int32_t iTries = ++m_backupTries.m_storage;
        if (iTries == 1) {
            m_simplexSize.m_storage = 1;
        }
        else {
            if (iTries >= 20) {
                return hkGskStatus::HK_GSK_PD_UNSOLVABLE;
            }
            m_simplexSize.m_storage = 0;
            float fOffset = (getRandomReal01(iTries) - 9.999999747378752e-05) * 0.00019999999494757503;
            // x, y and z only: w is the uninitialised lane (KEEP, above).
            hkVector4 perturbation;
            perturbation(0) = fOffset;
            perturbation(1) = fOffset;
            perturbation(2) = fOffset;
            m_aTb.m_translation.m_quad = _mm_add_ps(perturbation.m_quad, m_aTb.m_translation.m_quad);
        }
    }
done:
    // The triangle whose closest point to the origin has no negative barycentric weight, trying at most ten.
    float afWeights[3];
    for (int32_t i = 0; i < 10; ++i) {
        m_simplex.m_storage[0] = pTriangle->m_edge[0].m_vertex->m_position;
        m_simplex.m_storage[1] = pTriangle->m_edge[1].m_vertex->m_position;
        m_simplex.m_storage[2] = pTriangle->m_edge[2].m_vertex->m_position;
        hkVector4* pSimplex = m_simplex.m_storage;
        calcBarycentricCoordinates(
            &hkVector4Zero.m_quad, &pSimplex[0].m_quad, &pSimplex[1].m_quad, &pSimplex[2].m_quad, afWeights
        );
        if (!(afWeights[0] < 0.0f) && !(afWeights[1] < 0.0f) && !(afWeights[2] < 0.0f)) {
            break;
        }
        pTriangle = polytope.findTriangleNearestOrigin(pTriangle);
    }
    hkWingedEdge::Vertex* pVertex0 = pTriangle->m_edge[0].m_vertex;
    m_pointsAinA.m_storage[0] = pVertex0->m_pointAinA;
    m_pointsBinB.m_storage[0] = pVertex0->m_pointBinB;
    hkWingedEdge::Vertex* pVertex1 = pTriangle->m_edge[1].m_vertex;
    m_pointsAinA.m_storage[1] = pVertex1->m_pointAinA;
    m_pointsBinB.m_storage[1] = pVertex1->m_pointBinB;
    hkWingedEdge::Vertex* pVertex2 = pTriangle->m_edge[2].m_vertex;
    m_pointsAinA.m_storage[2] = pVertex2->m_pointAinA;
    m_pointsBinB.m_storage[2] = pVertex2->m_pointBinB;
    __m128 weight0 = _mm_set_ss(afWeights[0]);
    __m128 weight1 = _mm_set_ss(afWeights[1]);
    __m128 weight2 = _mm_set_ss(afWeights[2]);
    m_simplexSize.m_storage = 3;
    hkVector4* pPointsAinA = m_pointsAinA.m_storage;
    output.m_pointAinA.m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(weight1, weight1, 0), pPointsAinA[1].m_quad),
            _mm_mul_ps(_mm_shuffle_ps(weight0, weight0, 0), pPointsAinA[0].m_quad)
        ),
        _mm_mul_ps(_mm_shuffle_ps(weight2, weight2, 0), pPointsAinA[2].m_quad)
    );
    output.m_normalInA.m_quad = _mm_xor_ps(pTriangle->m_normal.m_quad, hkQuadRealNegateMask);
    output.m_normalInA(3) = -pTriangle->m_distanceToOrigin;
    return status;
}

// The point's signed distance from the triangle's plane decides: above the tolerance border the triangle is visible
// and may lower the previous minimum distance; otherwise it is invisible and pushes the border out past the point.
// A triangle whose previous minimum is not above the border comes back unknown.
hkWingedEdge::Triangle::TriangleState hkWingedEdge::Triangle::calcStateAndUpdateTolerance(
    const hkVector4& pointOfView,
    float* toleranceBorder,
    float* previousMinimumDistance
) {
    __m128 vProduct =
        _mm_mul_ps(_mm_sub_ps(pointOfView.m_quad, m_edge[0].m_vertex->m_position.m_quad), m_normal.m_quad);
    // Sums the lanes in the order z + (y + x).
    float fDistance = _mm_cvtss_f32(_mm_add_ss(
        _mm_shuffle_ps(vProduct, vProduct, 0xAA), _mm_add_ss(_mm_shuffle_ps(vProduct, vProduct, 0x55), vProduct)
    ));
    TriangleState state = HK_VISIBLE;
    if (*toleranceBorder < fDistance) {
        if (*previousMinimumDistance > fDistance) {
            *previousMinimumDistance = fDistance;
        }
    }
    else {
        // Adds 1e-7f widened to double.
        double dBorder = fDistance + 1.0000000116860974e-07;
        if (*toleranceBorder < dBorder) {
            *toleranceBorder = static_cast<float>(dBorder);
        }
        state = HK_INVISIBLE;
    }
    // When the compare is unordered (NaN), the state is kept.
    if (*previousMinimumDistance <= *toleranceBorder) {
        return HK_UNKNOWN;
    }
    return state;
}

// The first triangle with the least distance to the origin.
hkWingedEdge::Triangle* hkWingedEdge::findTriangleOnNearestPlane() {
    Triangle* pNearest = m_triangles;
    float fNearestDistance = m_triangles[0].m_distanceToOrigin;
    for (int32_t i = 1; i < m_numTriangles.m_storage; ++i) {
        if (fNearestDistance > m_triangles[i].m_distanceToOrigin) {
            fNearestDistance = m_triangles[i].m_distanceToOrigin;
            pNearest = &m_triangles[i];
        }
    }
    return pNearest;
}
