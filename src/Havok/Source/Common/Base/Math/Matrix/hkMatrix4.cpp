#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix4.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

hkMatrix4 hkMatrix4Identity = {
    hkVector4(1.0f, 0.0f, 0.0f, 0.0f),
    hkVector4(0.0f, 1.0f, 0.0f, 0.0f),
    hkVector4(0.0f, 0.0f, 1.0f, 0.0f),
    hkVector4(0.0f, 0.0f, 0.0f, 1.0f)
};

hkResult hkMatrix4::invert(float epsilon) {
    uint32_t v22;
    hkSimdReal scale;
    hkMatrix4 a;
    float v17 = XFNV_M128_F32(m_col0.m_quad, 0) * XFNV_M128_F32(m_col1.m_quad, 1) -
        XFNV_M128_F32(m_col0.m_quad, 1) * XFNV_M128_F32(m_col1.m_quad, 0u);
    float v19 = XFNV_M128_F32(m_col2.m_quad, 1) * XFNV_M128_F32(m_col0.m_quad, 0) -
        XFNV_M128_F32(m_col0.m_quad, 1) * XFNV_M128_F32(m_col2.m_quad, 0u);
    float v15 = XFNV_M128_F32(m_col0.m_quad, 0) * XFNV_M128_F32(m_col3.m_quad, 1) -
        XFNV_M128_F32(m_col0.m_quad, 1) * XFNV_M128_F32(m_col3.m_quad, 0u);
    float v14 = XFNV_M128_F32(m_col2.m_quad, 1) * XFNV_M128_F32(m_col1.m_quad, 0u) -
        XFNV_M128_F32(m_col2.m_quad, 0u) * XFNV_M128_F32(m_col1.m_quad, 1);
    float v13 = XFNV_M128_F32(m_col3.m_quad, 1) * XFNV_M128_F32(m_col1.m_quad, 0u) -
        XFNV_M128_F32(m_col3.m_quad, 0u) * XFNV_M128_F32(m_col1.m_quad, 1);
    float v12 = XFNV_M128_F32(m_col2.m_quad, 0u) * XFNV_M128_F32(m_col3.m_quad, 1) -
        XFNV_M128_F32(m_col3.m_quad, 0u) * XFNV_M128_F32(m_col2.m_quad, 1);
    float v11 = XFNV_M128_F32(m_col1.m_quad, 3) * XFNV_M128_F32(m_col0.m_quad, 2) -
        XFNV_M128_F32(m_col1.m_quad, 2) * XFNV_M128_F32(m_col0.m_quad, 3);
    float v9 = XFNV_M128_F32(m_col2.m_quad, 3) * XFNV_M128_F32(m_col0.m_quad, 2) -
        XFNV_M128_F32(m_col0.m_quad, 3) * XFNV_M128_F32(m_col2.m_quad, 2);
    float v10 = XFNV_M128_F32(m_col3.m_quad, 3) * XFNV_M128_F32(m_col0.m_quad, 2) -
        XFNV_M128_F32(m_col3.m_quad, 2) * XFNV_M128_F32(m_col0.m_quad, 3);
    float v20 = XFNV_M128_F32(m_col2.m_quad, 3) * XFNV_M128_F32(m_col1.m_quad, 2) -
        XFNV_M128_F32(m_col1.m_quad, 3) * XFNV_M128_F32(m_col2.m_quad, 2);
    float v18 = XFNV_M128_F32(m_col3.m_quad, 3) * XFNV_M128_F32(m_col1.m_quad, 2) -
        XFNV_M128_F32(m_col3.m_quad, 2) * XFNV_M128_F32(m_col1.m_quad, 3);
    float v16 = XFNV_M128_F32(m_col3.m_quad, 3) * XFNV_M128_F32(m_col2.m_quad, 2) -
        XFNV_M128_F32(m_col3.m_quad, 2) * XFNV_M128_F32(m_col2.m_quad, 3);
    double v2 = v16;
    double v3 = v17;
    double v4 = v18;
    double v5 = v19;
    double v6 = v20;
    float v21 = v16 * v17 - v18 * v19 + v20 * v15 + v10 * v14 - v9 * v13 + v11 * v12;
    double v7 = v21;
    *reinterpret_cast<float*>(&v22) = 1.0 / v21;
    scale.m_real = _mm_set_ss(std::bit_cast<float>(v22));
    float v23 = std::fabs(v7);
    if (epsilon >= static_cast<double>(v23)) {
        return HK_FAILURE;
    }
    XFNV_M128_F32(a.m_col0.m_quad, 0) = v2 * XFNV_M128_F32(m_col1.m_quad, 1) - XFNV_M128_F32(m_col2.m_quad, 1) * v4 +
        v6 * XFNV_M128_F32(m_col3.m_quad, 1);
    XFNV_M128_F32(a.m_col0.m_quad, 1) = XFNV_M128_F32(m_col2.m_quad, 1) * v10 - XFNV_M128_F32(m_col0.m_quad, 1) * v2 -
        v9 * XFNV_M128_F32(m_col3.m_quad, 1);
    XFNV_M128_F32(a.m_col0.m_quad, 2) = XFNV_M128_F32(m_col0.m_quad, 1) * v4 - v10 * XFNV_M128_F32(m_col1.m_quad, 1) +
        v11 * XFNV_M128_F32(m_col3.m_quad, 1);
    XFNV_M128_F32(a.m_col0.m_quad, 3) = v9 * XFNV_M128_F32(m_col1.m_quad, 1) - XFNV_M128_F32(m_col0.m_quad, 1) * v6 -
        XFNV_M128_F32(m_col2.m_quad, 1) * v11;
    XFNV_M128_F32(a.m_col1.m_quad, 0) = XFNV_M128_F32(m_col2.m_quad, 0u) * v4 - v2 * XFNV_M128_F32(m_col1.m_quad, 0u) -
        XFNV_M128_F32(m_col3.m_quad, 0u) * v6;
    XFNV_M128_F32(a.m_col1.m_quad, 1) = v2 * XFNV_M128_F32(m_col0.m_quad, 0) - XFNV_M128_F32(m_col2.m_quad, 0u) * v10 +
        XFNV_M128_F32(m_col3.m_quad, 0u) * v9;
    XFNV_M128_F32(a.m_col1.m_quad, 2) = v10 * XFNV_M128_F32(m_col1.m_quad, 0u) - v4 * XFNV_M128_F32(m_col0.m_quad, 0) -
        XFNV_M128_F32(m_col3.m_quad, 0u) * v11;
    XFNV_M128_F32(a.m_col1.m_quad, 3) = v6 * XFNV_M128_F32(m_col0.m_quad, 0) - v9 * XFNV_M128_F32(m_col1.m_quad, 0u) +
        v11 * XFNV_M128_F32(m_col2.m_quad, 0u);
    XFNV_M128_F32(a.m_col2.m_quad, 0) = XFNV_M128_F32(m_col3.m_quad, 3) * v14 +
        (XFNV_M128_F32(m_col1.m_quad, 3) * v12 - XFNV_M128_F32(m_col2.m_quad, 3) * v13);
    XFNV_M128_F32(a.m_col2.m_quad, 1) = XFNV_M128_F32(m_col2.m_quad, 3) * v15 - v12 * XFNV_M128_F32(m_col0.m_quad, 3) -
        XFNV_M128_F32(m_col3.m_quad, 3) * v5;
    XFNV_M128_F32(a.m_col2.m_quad, 2) = XFNV_M128_F32(m_col0.m_quad, 3) * v13 - XFNV_M128_F32(m_col1.m_quad, 3) * v15 +
        XFNV_M128_F32(m_col3.m_quad, 3) * v3;
    XFNV_M128_F32(a.m_col2.m_quad, 3) = XFNV_M128_F32(m_col1.m_quad, 3) * v5 - XFNV_M128_F32(m_col0.m_quad, 3) * v14 -
        XFNV_M128_F32(m_col2.m_quad, 3) * v3;
    XFNV_M128_F32(a.m_col3.m_quad, 0) = v13 * XFNV_M128_F32(m_col2.m_quad, 2) - XFNV_M128_F32(m_col1.m_quad, 2) * v12 -
        XFNV_M128_F32(m_col3.m_quad, 2) * v14;
    XFNV_M128_F32(a.m_col3.m_quad, 1) = XFNV_M128_F32(m_col0.m_quad, 2) * v12 - v15 * XFNV_M128_F32(m_col2.m_quad, 2) +
        XFNV_M128_F32(m_col3.m_quad, 2) * v5;
    XFNV_M128_F32(a.m_col3.m_quad, 2) = v15 * XFNV_M128_F32(m_col1.m_quad, 2) - v13 * XFNV_M128_F32(m_col0.m_quad, 2) -
        XFNV_M128_F32(m_col3.m_quad, 2) * v3;
    XFNV_M128_F32(a.m_col3.m_quad, 3) = v17 * XFNV_M128_F32(m_col2.m_quad, 2) +
        (v14 * XFNV_M128_F32(m_col0.m_quad, 2) - v5 * XFNV_M128_F32(m_col1.m_quad, 2));
    setMul(scale, a);
    return HK_SUCCESS;
}

hkResult hkMatrix4::setInverse(const hkMatrix4& m, float epsilon) {
    *this = m;
    return invert(epsilon);
}

void hkMatrix4::transpose() {
    float x = XFNV_M128_F32(m_col1.m_quad, 0);
    XFNV_M128_F32(m_col1.m_quad, 0) = XFNV_M128_F32(m_col0.m_quad, 1);
    XFNV_M128_F32(m_col0.m_quad, 1) = x;
    float v2 = XFNV_M128_F32(m_col2.m_quad, 0);
    XFNV_M128_F32(m_col2.m_quad, 0) = XFNV_M128_F32(m_col0.m_quad, 2);
    XFNV_M128_F32(m_col0.m_quad, 2) = v2;
    float v3 = XFNV_M128_F32(m_col3.m_quad, 0);
    XFNV_M128_F32(m_col3.m_quad, 0) = XFNV_M128_F32(m_col0.m_quad, 3);
    XFNV_M128_F32(m_col0.m_quad, 3) = v3;
    float y = XFNV_M128_F32(m_col2.m_quad, 1);
    XFNV_M128_F32(m_col2.m_quad, 1) = XFNV_M128_F32(m_col1.m_quad, 2);
    XFNV_M128_F32(m_col1.m_quad, 2) = y;
    float v5 = XFNV_M128_F32(m_col3.m_quad, 1);
    XFNV_M128_F32(m_col3.m_quad, 1) = XFNV_M128_F32(m_col1.m_quad, 3);
    XFNV_M128_F32(m_col1.m_quad, 3) = v5;
    float z = XFNV_M128_F32(m_col3.m_quad, 2);
    XFNV_M128_F32(m_col3.m_quad, 2) = XFNV_M128_F32(m_col2.m_quad, 3);
    XFNV_M128_F32(m_col2.m_quad, 3) = z;
}

// Each element is the dot product of a row of a (a column of its transposed copy) with a column of b, added pairwise
// across the lanes. b's column is reloaded for every element, so b may be this.
void hkMatrix4::setMul(const hkMatrix4& a, const hkMatrix4& b) {
    alignas(16) hkMatrix4 aTrans;
    _mm_store_ps(
        &reinterpret_cast<float*>(&aTrans.m_col0.m_quad)[0],
        _mm_load_ps(&reinterpret_cast<const float*>(&a.m_col0.m_quad)[0])
    );
    _mm_store_ps(
        &reinterpret_cast<float*>(&aTrans.m_col1.m_quad)[0],
        _mm_load_ps(&reinterpret_cast<const float*>(&a.m_col1.m_quad)[0])
    );
    _mm_store_ps(
        &reinterpret_cast<float*>(&aTrans.m_col2.m_quad)[0],
        _mm_load_ps(&reinterpret_cast<const float*>(&a.m_col2.m_quad)[0])
    );
    _mm_store_ps(
        &reinterpret_cast<float*>(&aTrans.m_col3.m_quad)[0],
        _mm_load_ps(&reinterpret_cast<const float*>(&a.m_col3.m_quad)[0])
    );
    aTrans.transpose();
    const hkVector4* const apkACols[4] = {&aTrans.m_col0, &aTrans.m_col1, &aTrans.m_col2, &aTrans.m_col3};
    const hkVector4* const apkBCols[4] = {&b.m_col0, &b.m_col1, &b.m_col2, &b.m_col3};
    float* apfOut[4] = {
        &reinterpret_cast<float*>(&m_col0.m_quad)[0],
        &reinterpret_cast<float*>(&m_col1.m_quad)[0],
        &reinterpret_cast<float*>(&m_col2.m_quad)[0],
        &reinterpret_cast<float*>(&m_col3.m_quad)[0]
    };
    for (int32_t i = 0; i < 4; i++) {
        float* pfOut = apfOut[i];
        for (int32_t j = 0; j < 4; j++) {
            pfOut[j] = apkBCols[i]->dot4(*apkACols[j]);
        }
    }
}

// The inversion's result is ignored.
void hkMatrix4::setMulInverse(const hkMatrix4& a, const hkMatrix4& b) {
    alignas(16) hkMatrix4 bInverse;
    _mm_store_ps(
        &reinterpret_cast<float*>(&bInverse.m_col0.m_quad)[0],
        _mm_load_ps(&reinterpret_cast<const float*>(&b.m_col0.m_quad)[0])
    );
    _mm_store_ps(
        &reinterpret_cast<float*>(&bInverse.m_col1.m_quad)[0],
        _mm_load_ps(&reinterpret_cast<const float*>(&b.m_col1.m_quad)[0])
    );
    _mm_store_ps(
        &reinterpret_cast<float*>(&bInverse.m_col2.m_quad)[0],
        _mm_load_ps(&reinterpret_cast<const float*>(&b.m_col2.m_quad)[0])
    );
    _mm_store_ps(
        &reinterpret_cast<float*>(&bInverse.m_col3.m_quad)[0],
        _mm_load_ps(&reinterpret_cast<const float*>(&b.m_col3.m_quad)[0])
    );
    // HK_REAL_EPSILON, a float constant.
    bInverse.invert(HK_REAL_EPSILON);
    setMul(a, bInverse);
}

void hkMatrix4::setMul(const hkSimdReal& scale, const hkMatrix4& a) {
    m_col0.m_quad = _mm_mul_ps(_mm_shuffle_ps(scale.m_real, scale.m_real, 0), a.m_col0.m_quad);
    m_col1.m_quad = _mm_mul_ps(_mm_shuffle_ps(scale.m_real, scale.m_real, 0), a.m_col1.m_quad);
    m_col2.m_quad = _mm_mul_ps(_mm_shuffle_ps(scale.m_real, scale.m_real, 0), a.m_col2.m_quad);
    m_col3.m_quad = _mm_mul_ps(_mm_shuffle_ps(scale.m_real, scale.m_real, 0), a.m_col3.m_quad);
}

void hkMatrix4::mul(const hkSimdReal& scale) {
    m_col0.m_quad = _mm_mul_ps(_mm_shuffle_ps(scale.m_real, scale.m_real, 0), m_col0.m_quad);
    m_col1.m_quad = _mm_mul_ps(_mm_shuffle_ps(scale.m_real, scale.m_real, 0), m_col1.m_quad);
    m_col2.m_quad = _mm_mul_ps(_mm_shuffle_ps(scale.m_real, scale.m_real, 0), m_col2.m_quad);
    m_col3.m_quad = _mm_mul_ps(_mm_shuffle_ps(scale.m_real, scale.m_real, 0), m_col3.m_quad);
}

hkBool hkMatrix4::isOk() {
    int32_t v2 = 0;
    while ((std::bit_cast<uint32_t>(XFNV_M128_F32(m_col0.m_quad, v2)) & 0x7F800000) != 0x7F800000) {
        if (++v2 >= 16) {
            return 1;
        }
    }
    return 0;
}

void hkMatrix4::setTranspose(const hkMatrix4& s) {
    XFNV_M128_F32(m_col0.m_quad, 0) = XFNV_M128_F32(s.m_col0.m_quad, 0);
    XFNV_M128_F32(m_col1.m_quad, 1) = XFNV_M128_F32(s.m_col1.m_quad, 1);
    XFNV_M128_F32(m_col2.m_quad, 2) = XFNV_M128_F32(s.m_col2.m_quad, 2);
    XFNV_M128_F32(m_col3.m_quad, 3) = XFNV_M128_F32(s.m_col3.m_quad, 3);
    XFNV_M128_F32(m_col0.m_quad, 1) = XFNV_M128_F32(s.m_col1.m_quad, 0);
    XFNV_M128_F32(m_col1.m_quad, 0) = XFNV_M128_F32(s.m_col0.m_quad, 1);
    XFNV_M128_F32(m_col0.m_quad, 2) = XFNV_M128_F32(s.m_col2.m_quad, 0);
    XFNV_M128_F32(m_col2.m_quad, 0) = XFNV_M128_F32(s.m_col0.m_quad, 2);
    XFNV_M128_F32(m_col0.m_quad, 3) = XFNV_M128_F32(s.m_col3.m_quad, 0);
    XFNV_M128_F32(m_col3.m_quad, 0) = XFNV_M128_F32(s.m_col0.m_quad, 3);
    XFNV_M128_F32(m_col1.m_quad, 2) = XFNV_M128_F32(s.m_col2.m_quad, 1);
    XFNV_M128_F32(m_col2.m_quad, 1) = XFNV_M128_F32(s.m_col1.m_quad, 2);
    XFNV_M128_F32(m_col1.m_quad, 3) = XFNV_M128_F32(s.m_col3.m_quad, 1);
    XFNV_M128_F32(m_col3.m_quad, 1) = XFNV_M128_F32(s.m_col1.m_quad, 3);
    XFNV_M128_F32(m_col2.m_quad, 3) = XFNV_M128_F32(s.m_col3.m_quad, 2);
    XFNV_M128_F32(m_col3.m_quad, 2) = XFNV_M128_F32(s.m_col2.m_quad, 3);
}

void hkMatrix4::add(const hkMatrix4& a) {
    m_col0.m_quad = _mm_add_ps(a.m_col0.m_quad, m_col0.m_quad);
    m_col1.m_quad = _mm_add_ps(a.m_col1.m_quad, m_col1.m_quad);
    m_col2.m_quad = _mm_add_ps(a.m_col2.m_quad, m_col2.m_quad);
    m_col3.m_quad = _mm_add_ps(a.m_col3.m_quad, m_col3.m_quad);
}

void hkMatrix4::sub(const hkMatrix4& a) {
    m_col0.m_quad = _mm_sub_ps(m_col0.m_quad, a.m_col0.m_quad);
    m_col1.m_quad = _mm_sub_ps(m_col1.m_quad, a.m_col1.m_quad);
    m_col2.m_quad = _mm_sub_ps(m_col2.m_quad, a.m_col2.m_quad);
    m_col3.m_quad = _mm_sub_ps(m_col3.m_quad, a.m_col3.m_quad);
}

void hkMatrix4::get4x4RowMajor(float* d) {
    d[0] = XFNV_M128_F32(m_col0.m_quad, 0);
    d[4] = XFNV_M128_F32(m_col0.m_quad, 1);
    d[8] = XFNV_M128_F32(m_col0.m_quad, 2);
    d[12] = XFNV_M128_F32(m_col0.m_quad, 3);
    d[1] = XFNV_M128_F32(m_col1.m_quad, 0);
    d[5] = XFNV_M128_F32(m_col1.m_quad, 1);
    d[9] = XFNV_M128_F32(m_col1.m_quad, 2);
    d[13] = XFNV_M128_F32(m_col1.m_quad, 3);
    d[2] = XFNV_M128_F32(m_col2.m_quad, 0);
    d[6] = XFNV_M128_F32(m_col2.m_quad, 1);
    d[10] = XFNV_M128_F32(m_col2.m_quad, 2);
    d[14] = XFNV_M128_F32(m_col2.m_quad, 3);
    d[3] = XFNV_M128_F32(m_col3.m_quad, 0);
    d[7] = XFNV_M128_F32(m_col3.m_quad, 1);
    d[11] = XFNV_M128_F32(m_col3.m_quad, 2);
    d[15] = XFNV_M128_F32(m_col3.m_quad, 3);
}

void hkMatrix4::set4x4RowMajor(const float* p) {
    XFNV_M128_F32(m_col0.m_quad, 0) = p[0];
    XFNV_M128_F32(m_col0.m_quad, 1) = p[4];
    XFNV_M128_F32(m_col0.m_quad, 2) = p[8];
    XFNV_M128_F32(m_col0.m_quad, 3) = p[12];
    XFNV_M128_F32(m_col1.m_quad, 0) = p[1];
    XFNV_M128_F32(m_col1.m_quad, 1) = p[5];
    XFNV_M128_F32(m_col1.m_quad, 2) = p[9];
    XFNV_M128_F32(m_col1.m_quad, 3) = p[13];
    XFNV_M128_F32(m_col2.m_quad, 0) = p[2];
    XFNV_M128_F32(m_col2.m_quad, 1) = p[6];
    XFNV_M128_F32(m_col2.m_quad, 2) = p[10];
    XFNV_M128_F32(m_col2.m_quad, 3) = p[14];
    XFNV_M128_F32(m_col3.m_quad, 0) = p[3];
    XFNV_M128_F32(m_col3.m_quad, 1) = p[7];
    XFNV_M128_F32(m_col3.m_quad, 2) = p[11];
    XFNV_M128_F32(m_col3.m_quad, 3) = p[15];
}

// Copies the sixteen floats one by one, column after column.
void hkMatrix4::get4x4ColumnMajor(float* d) const {
    d[0] = XFNV_M128_F32(m_col0.m_quad, 0);
    d[1] = XFNV_M128_F32(m_col0.m_quad, 1);
    d[2] = XFNV_M128_F32(m_col0.m_quad, 2);
    d[3] = XFNV_M128_F32(m_col0.m_quad, 3);
    d[4] = XFNV_M128_F32(m_col1.m_quad, 0);
    d[5] = XFNV_M128_F32(m_col1.m_quad, 1);
    d[6] = XFNV_M128_F32(m_col1.m_quad, 2);
    d[7] = XFNV_M128_F32(m_col1.m_quad, 3);
    d[8] = XFNV_M128_F32(m_col2.m_quad, 0);
    d[9] = XFNV_M128_F32(m_col2.m_quad, 1);
    d[10] = XFNV_M128_F32(m_col2.m_quad, 2);
    d[11] = XFNV_M128_F32(m_col2.m_quad, 3);
    d[12] = XFNV_M128_F32(m_col3.m_quad, 0);
    d[13] = XFNV_M128_F32(m_col3.m_quad, 1);
    d[14] = XFNV_M128_F32(m_col3.m_quad, 2);
    d[15] = XFNV_M128_F32(m_col3.m_quad, 3);
}

void hkMatrix4::set4x4ColumnMajor(const float* p) {
    XFNV_M128_F32(m_col0.m_quad, 0) = p[0];
    XFNV_M128_F32(m_col0.m_quad, 1) = p[1];
    XFNV_M128_F32(m_col0.m_quad, 2) = p[2];
    XFNV_M128_F32(m_col0.m_quad, 3) = p[3];
    XFNV_M128_F32(m_col1.m_quad, 0) = p[4];
    XFNV_M128_F32(m_col1.m_quad, 1) = p[5];
    XFNV_M128_F32(m_col1.m_quad, 2) = p[6];
    XFNV_M128_F32(m_col1.m_quad, 3) = p[7];
    XFNV_M128_F32(m_col2.m_quad, 0) = p[8];
    XFNV_M128_F32(m_col2.m_quad, 1) = p[9];
    XFNV_M128_F32(m_col2.m_quad, 2) = p[10];
    XFNV_M128_F32(m_col2.m_quad, 3) = p[11];
    XFNV_M128_F32(m_col3.m_quad, 0) = p[12];
    XFNV_M128_F32(m_col3.m_quad, 1) = p[13];
    XFNV_M128_F32(m_col3.m_quad, 2) = p[14];
    XFNV_M128_F32(m_col3.m_quad, 3) = p[15];
}

// Each column's w becomes 0, the last one's 1.
void hkMatrix4::resetFourthRow() {
    XFNV_M128_F32(m_col2.m_quad, 3) = 0.0f;
    XFNV_M128_F32(m_col1.m_quad, 3) = 0.0f;
    XFNV_M128_F32(m_col0.m_quad, 3) = 0.0f;
    XFNV_M128_F32(m_col3.m_quad, 3) = 1.0f;
}

// The SDK copies the rotation's columns and the translation, then resets the fourth row.
void hkMatrix4::set(const hkTransform& t) {
    m_col0.m_quad = t.m_rotation.m_col0.m_quad;
    m_col1.m_quad = t.m_rotation.m_col1.m_quad;
    m_col2.m_quad = t.m_rotation.m_col2.m_quad;
    m_col3.m_quad = t.m_translation.m_quad;
    XFNV_M128_F32(m_col0.m_quad, 3) = 0.0f;
    XFNV_M128_F32(m_col1.m_quad, 3) = 0.0f;
    XFNV_M128_F32(m_col2.m_quad, 3) = 0.0f;
    XFNV_M128_F32(m_col3.m_quad, 3) = 1.0f;
}

void hkMatrix4::get(hkTransform& t) {
    t.set4x4ColumnMajorQuads(&m_col0.m_quad);
}

void hkMatrix4::setMulInverseAffine(const hkMatrix4& a, const hkMatrix4& b) {
    hkVector4 v4;
    hkVector4 v6;
    hkVector4 v10;
    hkMatrix4 v11 = b;
    v11.invert(HK_REAL_EPSILON);
    v4.m_quad = a.m_col1.m_quad;
    v6.m_quad = a.m_col2.m_quad;
    __m128 v5 = _mm_add_ps(
        _mm_mul_ps(_mm_shuffle_ps(v11.m_col1.m_quad, v11.m_col1.m_quad, 0), a.m_col0.m_quad),
        _mm_mul_ps(_mm_shuffle_ps(v11.m_col1.m_quad, v11.m_col1.m_quad, 85), v4.m_quad)
    );
    __m128 v7 = _mm_add_ps(
        _mm_mul_ps(_mm_shuffle_ps(v11.m_col2.m_quad, v11.m_col2.m_quad, 0), a.m_col0.m_quad),
        _mm_mul_ps(_mm_shuffle_ps(v11.m_col2.m_quad, v11.m_col2.m_quad, 85), v4.m_quad)
    );
    __m128 v8 = _mm_mul_ps(_mm_shuffle_ps(v11.m_col1.m_quad, v11.m_col1.m_quad, 170), v6.m_quad);
    __m128 v9 = _mm_mul_ps(_mm_shuffle_ps(v11.m_col2.m_quad, v11.m_col2.m_quad, 170), v6.m_quad);
    v10.m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(v11.m_col3.m_quad, v11.m_col3.m_quad, 0), a.m_col0.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(v11.m_col3.m_quad, v11.m_col3.m_quad, 85), v4.m_quad)
            ),
            _mm_mul_ps(_mm_shuffle_ps(v11.m_col3.m_quad, v11.m_col3.m_quad, 170), v6.m_quad)
        ),
        a.m_col3.m_quad
    );
    m_col0.m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(v11.m_col0.m_quad, v11.m_col0.m_quad, 0), a.m_col0.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(v11.m_col0.m_quad, v11.m_col0.m_quad, 85), v4.m_quad)
        ),
        _mm_mul_ps(_mm_shuffle_ps(v11.m_col0.m_quad, v11.m_col0.m_quad, 170), a.m_col2.m_quad)
    );
    m_col1.m_quad = _mm_add_ps(v8, v5);
    m_col2.m_quad = _mm_add_ps(v9, v7);
    m_col3.m_quad = v10.m_quad;
}

void hkMatrix4::mul(hkMatrix4* b) {
    hkMatrix4 v3;
    v3.setMul(*this, *b);
    *this = v3;
}

int32_t hkMatrix4::isAffineTransformation(uint32_t* matrix) {
    __m128 v1 = _mm_undefined_ps();
    __m128 v2 = _mm_set_ss(std::bit_cast<float>(matrix[11]));
    __m128 v3 = _mm_set_ss(std::bit_cast<float>(matrix[15]));
    __m128 v4 = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(matrix[3])), v2),
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(matrix[7])), v3)
    );
    XFNV_M128_F32(v2, 0) = 0.0f;
    XFNV_M128_F32(v3, 0) = 0.0f;
    XFNV_M128_F32(v1, 0) = 0.0f;
    return (_mm_movemask_ps(_mm_cmple_ps(
                _mm_and_ps(
                    _mm_sub_ps(
                        v4,
                        _mm_unpacklo_ps(
                            _mm_unpacklo_ps(v2, v1),
                            _mm_unpacklo_ps(v3, _mm_set_ss(std::bit_cast<float>(0x3F800000u)))
                        )
                    ),
                    hkVector4::absMask
                ),
                _mm_shuffle_ps(_mm_set_ss(zero), _mm_set_ss(zero), 0)
            )) &
            7) == 7;
}
