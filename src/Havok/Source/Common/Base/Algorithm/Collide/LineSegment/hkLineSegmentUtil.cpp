#include "Havok/Source/Common/Base/Algorithm/Collide/LineSegment/hkLineSegmentUtil.h"

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

int32_t hkLineSegmentUtil::closestLineSegLineSeg(
    const hkVector4& A,
    const hkVector4& dA,
    const hkVector4& B,
    const hkVector4& dB,
    hkLineSegmentUtil::ClosestLineSegLineSegResult& result
) {
    int32_t v18;
    double v19;
    double v21;
    double v23;
    hkVector4 v24;
    float v28;
    float v29;
    float v32;
    __m128 m_quad = dA.m_quad;
    __m128 v6 = _mm_sub_ps(B.m_quad, A.m_quad);
    __m128 v7 = _mm_mul_ps(dB.m_quad, dA.m_quad);
    float v34 = _mm_shuffle_ps(v7, v7, 170)[0] + (_mm_shuffle_ps(v7, v7, 85)[0] + v7[0]);
    __m128 v8 = _mm_mul_ps(dA.m_quad, v6);
    float v9 = _mm_shuffle_ps(v8, v8, 170)[0] + (_mm_shuffle_ps(v8, v8, 85)[0] + v8[0]);
    __m128 v10 = _mm_mul_ps(dB.m_quad, v6);
    float v30 = _mm_shuffle_ps(v10, v10, 170)[0] + (_mm_shuffle_ps(v10, v10, 85)[0] + v10[0]);
    __m128 v11 = _mm_mul_ps(m_quad, m_quad);
    __m128 v12 = _mm_mul_ps(dB.m_quad, dB.m_quad);
    float v33 = _mm_shuffle_ps(v11, v11, 170)[0] + (_mm_shuffle_ps(v11, v11, 85)[0] + v11[0]);
    double v13 = (_mm_shuffle_ps(v12, v12, 170)[0] + (_mm_shuffle_ps(v12, v12, 85)[0] + v12[0]));
    double v14 = v33 * v13;
    double v15 = v34;
    double v16 = v34 * v34;
    float v35 = v14 - v16;
    float v36 = std::fabs((double)v35);
    float v27 = v9 * v13 - v30 * v15;
    double v17 = v36;
    if (!(v27 * v36 >= v36 * v36)) {
        if (!(v27 <= 0.0f)) {
            double v22 = v14 + v16;
            v19 = 0.0;
            float v37 = v22 * 9.5367431640625e-07;
            if (!(v37 < v17)) {
                v18 = 1;
                v28 = 1.0f;
            }
            else {
                v18 = 0;
                v28 = v27 / v17;
            }
        }
        else {
            v18 = 2;
            v28 = 0.0f;
            v19 = 0.0;
        }
    }
    else {
        v18 = 1;
        v19 = 0.0;
        v28 = 1.0f;
    }
    float v31 = v15 * v28 - v30;
    double v20 = v31;
    if (!(v31 >= v13)) {
        if (!(v20 <= v19)) {
            v32 = v20 / v13;
            goto LABEL_18;
        }
        v18 = 8;
        v32 = v19;
        v21 = v15;
    }
    else {
        v18 = 4;
        v21 = v15;
        v32 = 1.0f;
    }
    v29 = v21 * v32 + v9;
    v23 = v29;
    if (!(v29 <= v19)) {
        if (!(v33 <= v23)) {
            v28 = v23 / v33;
        }
        else {
            v18 |= 1u;
            v28 = 1.0f;
        }
    }
    else {
        v18 |= 2u;
        v28 = v19;
    }
LABEL_18:;
    v24.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v28), _mm_set_ss(v28), 0), m_quad), A.m_quad);
    result.m_closestPointA.m_quad = v24.m_quad;
    result.m_closestAminusClosestB.m_quad = _mm_sub_ps(
        v24.m_quad, _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v32), _mm_set_ss(v32), 0), dB.m_quad), B.m_quad)
    );
    __m128 v25 = _mm_mul_ps(result.m_closestAminusClosestB.m_quad, result.m_closestAminusClosestB.m_quad);
    result.m_distanceSquared.m_storage = _mm_shuffle_ps(v25, v25, 170)[0] + (_mm_shuffle_ps(v25, v25, 85)[0] + v25[0]);
    return v18;
}

int32_t hkLineSegmentUtil::closestPointLineSeg(
    const hkVector4& A,
    const hkVector4& a2,
    const hkVector4& a3,
    hkLineSegmentUtil::ClosestPointLineSegResult& a4
) {
    uint32_t v11;
    __m128 m_quad = a2.m_quad;
    __m128 v5 = _mm_sub_ps(a3.m_quad, a2.m_quad);
    __m128 v6 = _mm_mul_ps(_mm_sub_ps(A.m_quad, a2.m_quad), v5);
    double v7 = (_mm_shuffle_ps(v6, v6, 170)[0] + (_mm_shuffle_ps(v6, v6, 85)[0] + v6[0]));
    __m128 v8 = _mm_mul_ps(v5, v5);
    float v10 = _mm_shuffle_ps(v8, v8, 170)[0] + (_mm_shuffle_ps(v8, v8, 85)[0] + v8[0]);
    if (!(v7 <= 0.0)) {
        if (!(v10 <= v7)) {
            *reinterpret_cast<float*>(&v11) = v7 / v10;
            a4.m_pointOnEdge.m_quad = _mm_add_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v11)), _mm_set_ss(std::bit_cast<float>(v11)), 0), v5
                ),
                m_quad
            );
            return 0;
        }

        a4.m_pointOnEdge.m_quad = a3.m_quad;
        return 4;
    }

    a4.m_pointOnEdge.m_quad = m_quad;
    return 8;
}
