#include "Havok/Source/Physics/Collide/Util/hkpTriangleUtil.h"

#include "xfnv/Compiler.h"

hkBool hkpTriangleUtil::isDegenerate(const hkVector4& a, const hkVector4& b, const hkVector4& c, float tolerance) {
    __m128 v12;
    __m128 v5 = _mm_sub_ps(a.m_quad, c.m_quad);
    __m128 v6 = _mm_sub_ps(a.m_quad, b.m_quad);
    __m128 v7 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(v5, v5, 210), _mm_shuffle_ps(v6, v6, 201)),
        _mm_mul_ps(_mm_shuffle_ps(v5, v5, 201), _mm_shuffle_ps(v6, v6, 210))
    );
    __m128 v8 = _mm_sub_ps(b.m_quad, a.m_quad);
    __m128 v9 = _mm_sub_ps(b.m_quad, c.m_quad);
    __m128 v10 = _mm_mul_ps(v7, v7);
    __m128 v11 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(v9, v9, 210), _mm_shuffle_ps(v8, v8, 201)),
        _mm_mul_ps(_mm_shuffle_ps(v9, v9, 201), _mm_shuffle_ps(v8, v8, 210))
    );
    if (tolerance > static_cast<double>(
                        (XFNV_M128_F32(_mm_shuffle_ps(v10, v10, 170), 0) +
                         (XFNV_M128_F32(_mm_shuffle_ps(v10, v10, 85), 0) + XFNV_M128_F32(v10, 0)))
                    ) ||
        (v12 = _mm_mul_ps(v11, v11),
         (XFNV_M128_F32(_mm_shuffle_ps(v12, v12, 170), 0) +
          (XFNV_M128_F32(_mm_shuffle_ps(v12, v12, 85), 0) + XFNV_M128_F32(v12, 0))) < static_cast<double>(tolerance))) {
        return 1;
    }
    else {
        __m128 v13 = _mm_mul_ps(v6, v6);
        __m128 v14 = _mm_sub_ps(c.m_quad, b.m_quad);
        float v20 = XFNV_M128_F32(_mm_shuffle_ps(v13, v13, 170), 0) +
            (XFNV_M128_F32(_mm_shuffle_ps(v13, v13, 85), 0) + XFNV_M128_F32(v13, 0));
        __m128 v15 = _mm_mul_ps(v14, v14);
        __m128 v16 = _mm_mul_ps(v14, v6);
        float v21 = v20 *
            (XFNV_M128_F32(_mm_shuffle_ps(v15, v15, 170), 0) +
             (XFNV_M128_F32(_mm_shuffle_ps(v15, v15, 85), 0) + XFNV_M128_F32(v15, 0)));
        double v17 =
            (XFNV_M128_F32(_mm_shuffle_ps(v16, v16, 170), 0) +
             (XFNV_M128_F32(_mm_shuffle_ps(v16, v16, 85), 0) + XFNV_M128_F32(v16, 0)));
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
) {
    __m128 v16;
    __m128 v17;
    float v18;
    float v23;
    __m128 v10 = _mm_sub_ps(*c, *a);
    __m128 v11 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(*dir, *dir, 201), _mm_shuffle_ps(v10, v10, 210)),
        _mm_mul_ps(_mm_shuffle_ps(*dir, *dir, 210), _mm_shuffle_ps(v10, v10, 201))
    );
    __m128 v12 = _mm_sub_ps(*b, *a);
    __m128 v13 = _mm_mul_ps(v11, v12);
    double v14 =
        (XFNV_M128_F32(_mm_shuffle_ps(v13, v13, 170), 0) +
         (XFNV_M128_F32(_mm_shuffle_ps(v13, v13, 85), 0) + XFNV_M128_F32(v13, 0)));
    if (perform_culling) {
        if (v14 < 9.999999747378752e-05) {
            *result = 0;
            return result;
        }
    }
    else {
        if (v14 > -9.999999747378752e-05 && v14 < 9.999999747378752e-05) {
            *result = 0;
            return result;
        }
    }
    v16 = _mm_sub_ps(*pos, *a);
    v17 = _mm_mul_ps(v16, v11);
    v18 = XFNV_M128_F32(_mm_shuffle_ps(v17, v17, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v17, v17, 85), 0) + XFNV_M128_F32(v17, 0));
    v23 = 1.0 / v14;
    *u = v18;
    if (v18 < 0.0f) {
        *result = 0;
        return result;
    }
    if (!(v18 > v14)) {
        __m128 v19 = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(v16, v16, 201), _mm_shuffle_ps(v12, v12, 210)),
            _mm_mul_ps(_mm_shuffle_ps(v16, v16, 210), _mm_shuffle_ps(v12, v12, 201))
        );
        __m128 v20 = _mm_mul_ps(*dir, v19);
        float v22 = XFNV_M128_F32(_mm_shuffle_ps(v20, v20, 170), 0) +
            (XFNV_M128_F32(_mm_shuffle_ps(v20, v20, 85), 0) + XFNV_M128_F32(v20, 0));
        *v = v22;
        if (!(v22 < 0.0f) && !(v22 + *u > v14)) {
            __m128 v21 = _mm_mul_ps(v19, v10);
            *t = (XFNV_M128_F32(_mm_shuffle_ps(v21, v21, 170), 0) +
                  (XFNV_M128_F32(_mm_shuffle_ps(v21, v21, 85), 0) + XFNV_M128_F32(v21, 0))) *
                v23;
            *result = 1;
            *u = v23 * *u;
            *v = v23 * *v;
            return result;
        }
    }
    *result = 0;
    return result;
}
