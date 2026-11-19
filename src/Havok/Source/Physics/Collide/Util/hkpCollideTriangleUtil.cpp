#include "Havok/Source/Physics/Collide/Util/hkpCollideTriangleUtil.h"

#include "Havok/Source/Common/Base/Algorithm/Collide/LineSegment/hkLineSegmentUtil.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/hkpGsk.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

// Takes the position and the three corners, and writes the three weights out to result.
void hkpCollideTriangleUtil::calcBarycentricCoordinates(
    const hkVector4& pos,
    const hkVector4& t0,
    const hkVector4& t1,
    const hkVector4& t2,
    hkPadSpu<float>* result
) {
    __m128 m_quad = t1.m_quad;
    __m128 v6 = t2.m_quad;
    __m128 v7 = _mm_sub_ps(t0.m_quad, t1.m_quad);
    __m128 v8 = _mm_mul_ps(v7, v7);
    float v37 = XFNV_M128_F32(_mm_shuffle_ps(v8, v8, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v8, v8, 85), 0) + XFNV_M128_F32(v8, 0));
    __m128 v9 = _mm_sub_ps(t2.m_quad, t1.m_quad);
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
        __m128 v27 = _mm_sub_ps(t0.m_quad, v6);
        __m128 v28 = _mm_mul_ps(v27, v27);
        float v36 = XFNV_M128_F32(_mm_shuffle_ps(v28, v28, 170), 0) +
            (XFNV_M128_F32(_mm_shuffle_ps(v28, v28, 85), 0) + XFNV_M128_F32(v28, 0));
        if (!(v13 >= v18)) {
            if (!(v36 >= v18)) {
                __m128 v33 = pos.m_quad;
                result->m_storage = 0.0f;
                __m128 v34 = _mm_mul_ps(_mm_sub_ps(v33, m_quad), v9);
                float v47 = (XFNV_M128_F32(_mm_shuffle_ps(v34, v34, 170), 0) +
                             (XFNV_M128_F32(_mm_shuffle_ps(v34, v34, 85), 0) + XFNV_M128_F32(v34, 0))) /
                    v18;
                result[2].m_storage = v47;
                result[1].m_storage = 1.0 - v47;
            }
            else {
                __m128 v32 = _mm_mul_ps(_mm_sub_ps(pos.m_quad, v6), v27);
                float v46 = (XFNV_M128_F32(_mm_shuffle_ps(v32, v32, 170), 0) +
                             (XFNV_M128_F32(_mm_shuffle_ps(v32, v32, 85), 0) + XFNV_M128_F32(v32, 0))) /
                    v36;
                result->m_storage = v46;
                result[1].m_storage = 0.0f;
                result[2].m_storage = 1.0 - v46;
            }
        }
        else {
            double v29 = v36;
            if (!(v36 >= static_cast<double>(v37))) {
                __m128 v31 = _mm_mul_ps(_mm_sub_ps(pos.m_quad, m_quad), v7);
                float v45 = (XFNV_M128_F32(_mm_shuffle_ps(v31, v31, 170), 0) +
                             (XFNV_M128_F32(_mm_shuffle_ps(v31, v31, 85), 0) + XFNV_M128_F32(v31, 0))) /
                    v37;
                result->m_storage = v45;
                result[1].m_storage = 1.0 - v45;
                result[2].m_storage = 0.0f;
            }
            else {
                result[1].m_storage = 0.0f;
                if (!(v29 > 0.0)) {
                    result->m_storage = 0.0f;
                    result[2].m_storage = 0.0f;
                }
                else {
                    __m128 v30 = _mm_mul_ps(_mm_sub_ps(pos.m_quad, t2.m_quad), v27);
                    float v44 = (XFNV_M128_F32(_mm_shuffle_ps(v30, v30, 170), 0) +
                                 (XFNV_M128_F32(_mm_shuffle_ps(v30, v30, 85), 0) + XFNV_M128_F32(v30, 0))) /
                        v29;
                    result->m_storage = v44;
                    result[2].m_storage = 1.0 - v44;
                }
            }
        }
    }
    else {
        double v19 = v42;
        __m128 v20 = _mm_sub_ps(m_quad, pos.m_quad);
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
        result->m_storage = v43 * v35;
        result[1].m_storage = (v19 - v43 - v38) * v35;
        result[2].m_storage = v35 * v38;
    }
}

void hkpCollideTriangleUtil::setupClosestPointTriangleCache(
    const hkVector4* tri,
    hkpCollideTriangleUtil::ClosestPointTriangleCache& cache
) {
    __m128 m_quad = tri[1].m_quad;
    __m128 v3 = _mm_sub_ps(tri->m_quad, m_quad);
    __m128 v4 = _mm_sub_ps(tri[2].m_quad, m_quad);
    __m128 v5 = _mm_mul_ps(v3, v3);
    float v13 = XFNV_M128_F32(_mm_shuffle_ps(v5, v5, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v5, v5, 85), 0) + XFNV_M128_F32(v5, 0));
    __m128 v6 = _mm_mul_ps(v4, v4);
    float v7 = XFNV_M128_F32(_mm_shuffle_ps(v6, v6, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v6, v6, 85), 0) + XFNV_M128_F32(v6, 0));
    __m128 v8 = _mm_mul_ps(v4, v3);
    float v12 = XFNV_M128_F32(_mm_shuffle_ps(v8, v8, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v8, v8, 85), 0) + XFNV_M128_F32(v8, 0));
    float v14 = v13 * v7;
    __m128 v9 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(v4, v4, 210), _mm_shuffle_ps(v3, v3, 201)),
        _mm_mul_ps(_mm_shuffle_ps(v4, v4, 201), _mm_shuffle_ps(v3, v3, 210))
    );
    __m128 v10 = _mm_mul_ps(v9, v9);
    double v11 = v14;
    float v15 = v12 * v12;
    float v16 = v11 - v15;
    float v17 = 1.0 / v16;
    cache.m_QQ = v13 * v17;
    cache.m_RR = v7 * v17;
    cache.m_QR = v12 * v17;
    cache.m_invTriNormal = 1.0 /
        _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(
            (XFNV_M128_F32(_mm_shuffle_ps(v10, v10, 170), 0) +
             (XFNV_M128_F32(_mm_shuffle_ps(v10, v10, 85), 0) + XFNV_M128_F32(v10, 0)))
        )));
}

hkpCollideTriangleUtil::ClosestPointTriangleStatus hkpCollideTriangleUtil::closestPointTriangle(
    const hkVector4& position,
    const hkVector4* tri,
    const hkpCollideTriangleUtil::ClosestPointTriangleCache& cache,
    hkpCollideTriangleUtil::ClosestPointTriangleResult& result,
    hkpFeatureOutput* featureOutput
) {
    hkLineSegmentUtil::ClosestPointLineSegResult a4;
    if (featureOutput) {
        featureOutput->numFeatures = 0;
    }
    __m128 m_quad = tri[1].m_quad;
    __m128 v6 = _mm_sub_ps(m_quad, position.m_quad);
    __m128 v7 = _mm_sub_ps(tri->m_quad, m_quad);
    __m128 v8 = _mm_mul_ps(v7, v6);
    float v9 = XFNV_M128_F32(_mm_shuffle_ps(v8, v8, 85), 0) + XFNV_M128_F32(v8, 0);
    float v10 = XFNV_M128_F32(_mm_shuffle_ps(v8, v8, 170), 0);
    __m128 v11 = _mm_sub_ps(tri[2].m_quad, m_quad);
    float v40 = v10 + v9;
    __m128 v12 = _mm_mul_ps(v11, v6);
    float v13 = XFNV_M128_F32(_mm_shuffle_ps(v12, v12, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v12, v12, 85), 0) + XFNV_M128_F32(v12, 0));
    float a3b = cache.m_QR * v13 - cache.m_RR * v40;
    float v41 = v40 * cache.m_QR - v13 * cache.m_QQ;
    XFNV_M128_F32(a4.m_pointOnEdge.m_quad, 0) = a3b + 0.0010000000474974513;
    XFNV_M128_F32(a4.m_pointOnEdge.m_quad, 1) = 1.0010000000474975 - a3b - v41;
    XFNV_M128_F32(a4.m_pointOnEdge.m_quad, 2) = v41 + 0.0010000000474974513;
    XFNV_M128_F32(a4.m_pointOnEdge.m_quad, 3) = 0.001f;
    int32_t n7 = _mm_movemask_ps(_mm_cmplt_ps(hkVector4Zero.m_quad, a4.m_pointOnEdge.m_quad)) & 7;
    if (n7 == 7) {
        hkVector4 v15;
        v15.m_quad = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(v11, v11, 210), _mm_shuffle_ps(v7, v7, 201)),
            _mm_mul_ps(_mm_shuffle_ps(v11, v11, 201), _mm_shuffle_ps(v7, v7, 210))
        );
        result.hitDirection.m_quad = v15.m_quad;
        __m128 m_quad_1 = _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(cache.m_invTriNormal), _mm_set_ss(cache.m_invTriNormal), 0), v15.m_quad
        );
        __m128 v17 = _mm_mul_ps(v6, m_quad_1);
        float distance = XFNV_M128_F32(_mm_shuffle_ps(v17, v17, 170), 0) +
            (XFNV_M128_F32(_mm_shuffle_ps(v17, v17, 85), 0) + XFNV_M128_F32(v17, 0));
        result.distance = distance;
        result.hitDirection.m_quad = m_quad_1;
        if (!(distance > 0.0f)) {
            result.distance = -distance;
        }
        else {
            result.hitDirection.m_quad = _mm_xor_ps(m_quad_1, hkQuadRealNegateMask);
        }
        if (featureOutput) {
            featureOutput->numFeatures = 3;
            featureOutput->featureIds[0u] = 0;
            featureOutput->featureIds[1] = 1;
            featureOutput->featureIds[2] = 2;
        }
        return hkpCollideTriangleUtil::HIT_TRIANGLE_FACE;
    }

    int32_t v20 = rdata_10CB438[n7];
    if (v20 >= 0) {
        hkLineSegmentUtil::ClosestPointLineSegResult v50;
        int32_t n8_1 = hkLineSegmentUtil::closestPointLineSeg(
            position, tri[v20], tri[hkpCollideTriangleUtil::vertexToEdgeLut[v20 + 2]], a4
        );
        int32_t n8 = hkLineSegmentUtil::closestPointLineSeg(
            position, tri[hkpCollideTriangleUtil::vertexToEdgeLut[v20]], tri[v20], v50
        );
        __m128 v34 = _mm_sub_ps(position.m_quad, v50.m_pointOnEdge.m_quad);
        __m128 v51 = _mm_sub_ps(position.m_quad, a4.m_pointOnEdge.m_quad);
        __m128 v35 = _mm_mul_ps(v51, v51);
        float v36 = XFNV_M128_F32(_mm_shuffle_ps(v35, v35, 85), 0) + XFNV_M128_F32(v35, 0);
        float v37 = XFNV_M128_F32(_mm_shuffle_ps(v35, v35, 170), 0);
        __m128 v38 = _mm_mul_ps(v34, v34);
        float v39 = v37 + v36;
        float a3 = XFNV_M128_F32(_mm_shuffle_ps(v38, v38, 170), 0) +
            (XFNV_M128_F32(_mm_shuffle_ps(v38, v38, 85), 0) + XFNV_M128_F32(v38, 0));
        if (!(a3 > static_cast<double>(v39))) {
            float v48 = std::sqrt((double)a3);
            float v42 = 1.0 / v48;
            if (featureOutput) {
                if (n8 == 4) {
                    featureOutput->numFeatures = 1;
                    featureOutput->featureIds[0u] = v20;
                }
                else {
                    if (n8 == 8) {
                        featureOutput->numFeatures = 1;
                        featureOutput->featureIds[0u] = hkpCollideTriangleUtil::vertexToEdgeLut[v20];
                    }
                    else {
                        featureOutput->numFeatures = 2;
                        featureOutput->featureIds[0u] = v20;
                        featureOutput->featureIds[1] = hkpCollideTriangleUtil::vertexToEdgeLut[v20];
                    }
                }
            }
            result.distance = a3 * v42;
            result.hitDirection.m_quad = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v42), _mm_set_ss(v42), 0), v34);
            return hkpCollideTriangleUtil::HIT_TRIANGLE_EDGE;
        }

        float a3c = std::sqrt((double)v39);
        float a3a = 1.0 / a3c;
        if (featureOutput) {
            if (n8_1 == 4) {
                featureOutput->numFeatures = 1;
                featureOutput->featureIds[0u] = v20;
            }
            else {
                if (n8_1 == 8) {
                    featureOutput->numFeatures = 1;
                    featureOutput->featureIds[0u] = hkpCollideTriangleUtil::vertexToEdgeLut[v20 + 2];
                }
                else {
                    featureOutput->numFeatures = 2;
                    featureOutput->featureIds[0u] = v20;
                    featureOutput->featureIds[1] = hkpCollideTriangleUtil::vertexToEdgeLut[v20 + 2];
                }
            }
        }
        result.distance = v39 * a3a;
        result.hitDirection.m_quad = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(a3a), _mm_set_ss(a3a), 0), v51);
        return hkpCollideTriangleUtil::HIT_TRIANGLE_EDGE;
    }

    __m128 v28 = _mm_undefined_ps();
    int32_t v21 = hkpCollideTriangleUtil::vertexToEdgeLut[v20 + 8];
    int32_t v22 = v20 + 8;
    int32_t n8_2 = hkLineSegmentUtil::closestPointLineSeg(
        position, tri[hkpCollideTriangleUtil::vertexToEdgeLut[v22 + 2]], tri[v21], a4
    );
    if (featureOutput) {
        if (n8_2 == 4) {
            featureOutput->featureIds[0u] = hkpCollideTriangleUtil::vertexToEdgeLut[v22];
            featureOutput->numFeatures = 1;
        }
        else {
            if (n8_2 == 8) {
                featureOutput->featureIds[0u] = hkpCollideTriangleUtil::vertexToEdgeLut[v22 + 2];
                featureOutput->numFeatures = 1;
            }
            else {
                featureOutput->featureIds[0u] = hkpCollideTriangleUtil::vertexToEdgeLut[v22];
                featureOutput->featureIds[1] = hkpCollideTriangleUtil::vertexToEdgeLut[v22 + 2];
                featureOutput->numFeatures = 2;
            }
        }
    }
    __m128 v24 = _mm_sub_ps(position.m_quad, a4.m_pointOnEdge.m_quad);
    __m128 v25 = _mm_mul_ps(v24, v24);
    __m128 v26 = _mm_shuffle_ps(v25, v25, 170);
    XFNV_M128_F32(v26, 0) =
        XFNV_M128_F32(v26, 0) + (XFNV_M128_F32(_mm_shuffle_ps(v25, v25, 85), 0) + XFNV_M128_F32(v25, 0));
    float v27 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(v26, 0))));
    XFNV_M128_F32(v28, 0) = std::bit_cast<float>(static_cast<int32_t>(0.0f));
    __m128 v29 = _mm_cmpeq_ss(v26, v28);
    XFNV_M128_F32(v28, 0) = 3.0f - ((XFNV_M128_F32(v26, 0) * v27) * v27);
    __m128 v30 = _mm_setzero_ps();
    XFNV_M128_F32(v30, 0) = (0.5f * v27) * XFNV_M128_F32(v28, 0);
    __m128 v31 = _mm_andnot_ps(v29, v30);
    __m128 v32 = _mm_shuffle_ps(v31, v31, 0);
    result.hitDirection.m_quad = _mm_mul_ps(v24, v32);
    result.distance = (XFNV_M128_F32(_mm_shuffle_ps(v25, v25, 170), 0) +
                       (XFNV_M128_F32(_mm_shuffle_ps(v25, v25, 85), 0) + XFNV_M128_F32(v25, 0))) *
        XFNV_M128_F32(v32, 0);
    return hkpCollideTriangleUtil::HIT_TRIANGLE_EDGE;
}

void hkpCollideTriangleUtil::setupPointTriangleDistanceCache(
    const hkVector4* tri,
    hkpCollideTriangleUtil::PointTriangleDistanceCache& cache
) {
    __m128 v16;
    __m128 m_quad = tri[1].m_quad;
    __m128 v3 = _mm_sub_ps(tri->m_quad, tri[2].m_quad);
    __m128 v4 = _mm_sub_ps(tri[2].m_quad, m_quad);
    __m128 v5 = _mm_sub_ps(m_quad, tri->m_quad);
    __m128 v6 = _mm_mul_ps(_mm_shuffle_ps(v3, v3, 201), _mm_shuffle_ps(v4, v4, 210));
    __m128 v7 = _mm_shuffle_ps(v3, v3, 210);
    __m128 v8 = _mm_shuffle_ps(v4, v4, 201);
    __m128 v9 = _mm_mul_ps(v4, v4);
    __m128 v10 = _mm_mul_ps(v3, v3);
    __m128 v11 = _mm_sub_ps(_mm_mul_ps(v7, v8), v6);
    __m128 v12 = _mm_mul_ps(v11, v11);
    __m128 v13 = _mm_mul_ps(v5, v5);
    XFNV_M128_F32(v16, 0) = XFNV_M128_F32(_mm_shuffle_ps(v9, v9, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v9, v9, 85), 0) + XFNV_M128_F32(v9, 0));
    XFNV_M128_F32(v16, 1) = XFNV_M128_F32(_mm_shuffle_ps(v10, v10, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v10, v10, 85), 0) + XFNV_M128_F32(v10, 0));
    XFNV_M128_F32(v16, 2) = XFNV_M128_F32(_mm_shuffle_ps(v13, v13, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v13, v13, 85), 0) + XFNV_M128_F32(v13, 0));
    float v15 = XFNV_M128_F32(_mm_shuffle_ps(v12, v12, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v12, v12, 85), 0) + XFNV_M128_F32(v12, 0));
    XFNV_M128_F32(v16, 3) = v15;
    __m128 v14 = _mm_rsqrt_ps(v16);
    __m128 v17 =
        _mm_mul_ps(_mm_sub_ps(hkQuadReal3333, _mm_mul_ps(_mm_mul_ps(v14, v16), v14)), _mm_mul_ps(hkQuadRealHalf, v14));
    // The store4 into cache.m_invEdgeLen writes four lanes; the fourth lane is m_invNormalLen.
    hkVector4 invLen;
    invLen.m_quad = v17;
    invLen.store4(&cache.m_invEdgeLen[0]);
    cache.m_normalLen = v15 * cache.m_invNormalLen;
}
