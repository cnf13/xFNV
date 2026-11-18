#include "Havok/Source/Physics/Collide/Util/hkpTriangleUtil.h"

hkBool hkpTriangleUtil::isDegenerate(const hkVector4& a2, const hkVector4& a3, const hkVector4& a4, float a5) {
    __m128 v12;
    __m128 v5 = _mm_sub_ps(a2.m_quad, a4.m_quad);
    __m128 v6 = _mm_sub_ps(a2.m_quad, a3.m_quad);
    __m128 v7 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(v5, v5, 210), _mm_shuffle_ps(v6, v6, 201)),
        _mm_mul_ps(_mm_shuffle_ps(v5, v5, 201), _mm_shuffle_ps(v6, v6, 210))
    );
    __m128 v8 = _mm_sub_ps(a3.m_quad, a2.m_quad);
    __m128 v9 = _mm_sub_ps(a3.m_quad, a4.m_quad);
    __m128 v10 = _mm_mul_ps(v7, v7);
    __m128 v11 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(v9, v9, 210), _mm_shuffle_ps(v8, v8, 201)),
        _mm_mul_ps(_mm_shuffle_ps(v9, v9, 201), _mm_shuffle_ps(v8, v8, 210))
    );
    if (a5 > static_cast<double>((_mm_shuffle_ps(v10, v10, 170)[0] + (_mm_shuffle_ps(v10, v10, 85)[0] + v10[0]))) ||
        (v12 = _mm_mul_ps(v11, v11),
         (_mm_shuffle_ps(v12, v12, 170)[0] + (_mm_shuffle_ps(v12, v12, 85)[0] + v12[0])) < static_cast<double>(a5))) {
        return 1;
    }
    else {
        __m128 v13 = _mm_mul_ps(v6, v6);
        __m128 v14 = _mm_sub_ps(a4.m_quad, a3.m_quad);
        float v20 = _mm_shuffle_ps(v13, v13, 170)[0] + (_mm_shuffle_ps(v13, v13, 85)[0] + v13[0]);
        __m128 v15 = _mm_mul_ps(v14, v14);
        __m128 v16 = _mm_mul_ps(v14, v6);
        float v21 = v20 * (_mm_shuffle_ps(v15, v15, 170)[0] + (_mm_shuffle_ps(v15, v15, 85)[0] + v15[0]));
        double v17 = (_mm_shuffle_ps(v16, v16, 170)[0] + (_mm_shuffle_ps(v16, v16, 85)[0] + v16[0]));
        double v18 = v21;
        float v22 = v17 * v17;
        float v23 = v18 - v22;
        if (v23 != 0.0f) {
            return 0;
        }
    }
    return 1;
}

uint8_t* hkpTriangleUtil::rayIntersect(
    uint8_t* a1,
    float* a2,
    float* a3,
    float* a4,
    __m128* a5,
    __m128* a6,
    __m128* a7,
    __m128* a8,
    __m128* a9,
    char a10
) {
    __m128 v16;
    __m128 v17;
    float v18;
    float v23;
    __m128 v10 = _mm_sub_ps(*a9, *a7);
    __m128 v11 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(*a6, *a6, 201), _mm_shuffle_ps(v10, v10, 210)),
        _mm_mul_ps(_mm_shuffle_ps(*a6, *a6, 210), _mm_shuffle_ps(v10, v10, 201))
    );
    __m128 v12 = _mm_sub_ps(*a8, *a7);
    __m128 v13 = _mm_mul_ps(v11, v12);
    double v14 = (_mm_shuffle_ps(v13, v13, 170)[0] + (_mm_shuffle_ps(v13, v13, 85)[0] + v13[0]));
    if (a10) {
        if (v14 < 9.999999747378752e-05) {
            *a1 = 0;
            return a1;
        }
    }
    else {
        if (v14 > -9.999999747378752e-05 && v14 < 9.999999747378752e-05) {
            goto LABEL_12;
        }
    }
    v16 = _mm_sub_ps(*a5, *a7);
    v17 = _mm_mul_ps(v16, v11);
    v18 = _mm_shuffle_ps(v17, v17, 170)[0] + (_mm_shuffle_ps(v17, v17, 85)[0] + v17[0]);
    v23 = 1.0 / v14;
    *a3 = v18;
    if (v18 < 0.0f) {
        *a1 = 0;
        return a1;
    }
    if (!(v18 > v14)) {
        __m128 v19 = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(v16, v16, 201), _mm_shuffle_ps(v12, v12, 210)),
            _mm_mul_ps(_mm_shuffle_ps(v16, v16, 210), _mm_shuffle_ps(v12, v12, 201))
        );
        __m128 v20 = _mm_mul_ps(*a6, v19);
        float v22 = _mm_shuffle_ps(v20, v20, 170)[0] + (_mm_shuffle_ps(v20, v20, 85)[0] + v20[0]);
        *a4 = v22;
        if (!(v22 < 0.0f) && !(v22 + *a3 > v14)) {
            __m128 v21 = _mm_mul_ps(v19, v10);
            *a2 = (_mm_shuffle_ps(v21, v21, 170)[0] + (_mm_shuffle_ps(v21, v21, 85)[0] + v21[0])) * v23;
            *a1 = 1;
            *a3 = v23 * *a3;
            *a4 = v23 * *a4;
            return a1;
        }
    }
LABEL_12:;
    *a1 = 0;
    return a1;
}
