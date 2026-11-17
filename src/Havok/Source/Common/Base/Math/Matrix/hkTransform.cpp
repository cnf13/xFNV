#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <xmmintrin.h>

// The identity rotation and the translation (0, 0, 0, 1), set column by column (hkTransform has constructors, so it
// takes no brace list).
hkTransform hkTransformIdentity = [] {
    hkTransform kIdentity;
    kIdentity.m_rotation.m_col0 = hkVector4(1.0f, 0.0f, 0.0f, 0.0f);
    kIdentity.m_rotation.m_col1 = hkVector4(0.0f, 1.0f, 0.0f, 0.0f);
    kIdentity.m_rotation.m_col2 = hkVector4(0.0f, 0.0f, 1.0f, 0.0f);
    kIdentity.m_translation = hkVector4(0.0f, 0.0f, 0.0f, 1.0f);
    return kIdentity;
}();

// A quad real is four floats in a row; the setMul loop indexes them by lane.
static inline const float* QuadLanes(const hkQuadReal& akQuad) {
    return &reinterpret_cast<const float*>(&akQuad)[0];
}

static inline float* QuadLanes(hkQuadReal& akQuad) {
    return &reinterpret_cast<float*>(&akQuad)[0];
}

hkTransform::hkTransform() {}

hkTransform::hkTransform(const hkTransform& arTransform)
  : m_rotation(arTransform.m_rotation), m_translation(arTransform.m_translation) {}

hkTransform::hkTransform(const hkQuaternion& arRotation, const hkVector4& arTranslation)
  : m_translation(arTranslation) {
    m_rotation.set(arRotation);
}

// The translation is checked first.
hkBool hkTransform::isOk() const {
    hkBool kResult;
    if (m_translation.isOk3().m_bool && m_rotation.isOk().m_bool) {
        kResult.m_bool = 1;
        return kResult;
    }
    kResult.m_bool = 0;
    return kResult;
}

// Places arB into this through arA: each destination row is arB's row scaled by arA's columns, and the translation row
// then gains arA's translation, component by component. arA's columns are loaded once before the loop and each arB row
// before its row is stored (so this may alias either), and each step rounds to float in the order ((x * col0 + y *
// col1) + z * col2).
void hkTransform::setMul(const hkTransform& aTb, const hkTransform& bTc) {
    const hkQuadReal kA0 = aTb.m_rotation.m_col0.m_quad;
    const hkQuadReal kA1 = aTb.m_rotation.m_col1.m_quad;
    const hkQuadReal kA2 = aTb.m_rotation.m_col2.m_quad;
    const float* pfA0 = QuadLanes(kA0);
    const float* pfA1 = QuadLanes(kA1);
    const float* pfA2 = QuadLanes(kA2);
    hkQuadReal* apkRows[4] = {
        &m_rotation.m_col0.m_quad,
        &m_rotation.m_col1.m_quad,
        &m_rotation.m_col2.m_quad,
        &m_translation.m_quad,
    };
    const hkQuadReal* apkBRows[4] = {
        &bTc.m_rotation.m_col0.m_quad,
        &bTc.m_rotation.m_col1.m_quad,
        &bTc.m_rotation.m_col2.m_quad,
        &bTc.m_translation.m_quad,
    };
    for (int32_t i = 3; i >= 0; --i) {
        const hkQuadReal kB = *apkBRows[i];
        const float* pfB = QuadLanes(kB);
        float* pfRow = QuadLanes(*apkRows[i]);
        for (uint32_t c = 0; c < 4; ++c) {
            const float fX = pfB[0] * pfA0[c];
            const float fY = pfB[1] * pfA1[c];
            const float fZ = pfB[2] * pfA2[c];
            const float fXY = fX + fY;
            pfRow[c] = fXY + fZ;
        }
    }
    float* pfTranslation = QuadLanes(m_translation.m_quad);
    const float* pfATranslation = QuadLanes(aTb.m_translation.m_quad);
    for (uint32_t c = 0; c < 4; ++c) {
        pfTranslation[c] = pfTranslation[c] + pfATranslation[c];
    }
}

// this = the inverse of arA times arB: each of arB's rotation columns turned by the transpose of arA's rotation, then
// the difference of the translations turned the same way. Transposes arA's rotation before the column loop and again
// after it (reloading arA, which this may alias), reads each arB column just before its own column is stored, and sums
// (y * row1 + x * row0) + z * row2.
void hkTransform::setMulInverseMul(const hkTransform& bTa, const hkTransform& bTc) {
    hkQuadReal kCol1 = bTa.m_rotation.m_col1.m_quad;
    hkQuadReal kCol2 = bTa.m_rotation.m_col2.m_quad;
    __m128 v5 = _mm_unpacklo_ps(bTa.m_rotation.m_col0.m_quad, kCol1);
    __m128 v6 = _mm_unpackhi_ps(bTa.m_rotation.m_col0.m_quad, kCol1);
    __m128 kRow0 = _mm_movelh_ps(v5, kCol2);
    __m128 kRow1 = _mm_shuffle_ps(_mm_movehl_ps(kRow0, v5), kCol2, 212);
    __m128 kRow2 = _mm_shuffle_ps(v6, kCol2, 228);
    for (int32_t i = 0; i < 3; ++i) {
        const hkQuadReal kB = bTc.getColumn(i).m_quad;
        getColumn(i).m_quad = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kB, kB, 85), kRow1), _mm_mul_ps(_mm_shuffle_ps(kB, kB, 0), kRow0)),
            _mm_mul_ps(_mm_shuffle_ps(kB, kB, 170), kRow2)
        );
    }
    hkQuadReal v13 = bTa.m_rotation.m_col1.m_quad;
    hkQuadReal v14 = bTa.m_rotation.m_col2.m_quad;
    __m128 v15 = _mm_sub_ps(bTc.m_translation.m_quad, bTa.m_translation.m_quad);
    __m128 v16 = _mm_unpacklo_ps(bTa.m_rotation.m_col0.m_quad, v13);
    __m128 v17 = _mm_movelh_ps(v16, v14);
    __m128 kTRow1 = _mm_shuffle_ps(_mm_movehl_ps(v17, v16), v14, 212);
    __m128 kTRow2 = _mm_shuffle_ps(_mm_unpackhi_ps(bTa.m_rotation.m_col0.m_quad, v13), v14, 228);
    m_translation.m_quad = _mm_add_ps(
        _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v15, v15, 85), kTRow1), _mm_mul_ps(_mm_shuffle_ps(v15, v15, 0), v17)),
        _mm_mul_ps(kTRow2, _mm_shuffle_ps(v15, v15, 170))
    );
}

hkTransform& hkTransform::Assign(const hkTransform& arTransform) {
    *this = arTransform;
    return *this;
}

void hkTransform::setInverse(const hkTransform& t) {
    m_rotation.setTranspose(t.m_rotation);
    __m128 v3 = _mm_xor_ps(t.m_translation.m_quad, hkQuadRealNegateMask);
    m_translation.m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(v3, v3, 85), m_rotation.m_col1.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(v3, v3, 0), m_rotation.m_col0.m_quad)
        ),
        _mm_mul_ps(_mm_shuffle_ps(v3, v3, 170), m_rotation.m_col2.m_quad)
    );
}

// this = this * b, through a copy of this (b may be this).
void hkTransform::setMulEq(const hkTransform& b) {
    hkTransform temp = *this;
    setMul(temp, b);
}

void hkTransform::setMulMulInverse(const hkTransform& wTa, const hkTransform& wTb) {
    hkTransform arBa;
    arBa.setInverse(wTb);
    setMul(wTa, arBa);
}

// The rotation is compared first; the translation's x, y and z must then each be within afEpsilon of arTransform's. A
// NaN difference fails the compare, and the w component is not tested.
hkBool hkTransform::isApproximatelyEqual(const hkTransform& t, float zero) const {
    hkBool kResult;
    if (m_rotation.isApproximatelyEqual(t.m_rotation, zero).m_bool) {
        const float* pfThis = QuadLanes(m_translation.m_quad);
        const float* pfOther = QuadLanes(t.m_translation.m_quad);
        bool bEqual = true;
        for (int32_t i = 0; i < 3; ++i) {
            float fDifference = pfThis[i] - pfOther[i];
            if (fDifference < 0.0f) {
                fDifference = -fDifference;
            }
            if (!(fDifference <= zero)) {
                bEqual = false;
            }
        }
        kResult.m_bool = bEqual ? 1 : 0;
    }
    else {
        kResult.m_bool = 0;
    }
    return kResult;
}

hkBool* hkTransform::isOk(hkBool* apResult) const {
    hkBool v4;
    hkBool v5;
    if ((&(v4 = m_translation.isOk3()))->m_bool && (&(v5 = m_rotation.isOk()))->m_bool) {
        apResult->m_bool = 1;
        return apResult;
    }
    else {
        apResult->m_bool = 0;
        return apResult;
    }
}

__m128* hkTransform::get4x4ColumnMajorQuads(__m128* a1) {
    *a1 = _mm_and_ps(m_rotation.m_col0.m_quad, rdata_10C4C10);
    a1[1] = _mm_and_ps(m_rotation.m_col1.m_quad, rdata_10C4C10);
    a1[2] = _mm_and_ps(m_rotation.m_col2.m_quad, rdata_10C4C10);
    a1[3] = _mm_shuffle_ps(m_translation.m_quad, _mm_unpackhi_ps(m_translation.m_quad, hkQuadReal1111), 196);
    return a1;
}

const __m128* hkTransform::set4x4ColumnMajorQuads(const __m128* a1) {
    m_rotation.m_col0.m_quad = _mm_and_ps(*a1, rdata_10C4C10);
    m_rotation.m_col1.m_quad = _mm_and_ps(a1[1], rdata_10C4C10);
    m_rotation.m_col2.m_quad = _mm_and_ps(a1[2], rdata_10C4C10);
    m_translation.m_quad = _mm_shuffle_ps(a1[3], _mm_unpackhi_ps(a1[3], hkQuadReal1111), 196);
    return a1;
}

void hkTransform::setMul_0(const hkQsTransform& aTb, const hkTransform& bTc) {
    hkTransform arA;
    aTb.copyToTransform(arA);
    setMul(arA, bTc);
}
