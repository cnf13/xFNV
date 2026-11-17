#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

// arA's columns are loaded before any store; each column of arB is read just before this's column of the same index
// is written, from the last to the first, so this may be arA or arB.
void hkMatrix3::setMul(const hkMatrix3& arA, const hkMatrix3& arB) {
    const __m128 kCol0 = arA.m_col0.m_quad;
    const __m128 kCol1 = arA.m_col1.m_quad;
    const __m128 kCol2 = arA.m_col2.m_quad;
    for (int32_t i = 2; i >= 0; --i) {
        const __m128 kB = arB.getColumn(i).m_quad;
        (&m_col0)[i].m_quad = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kB, kB, 0), kCol0), _mm_mul_ps(_mm_shuffle_ps(kB, kB, 85), kCol1)),
            _mm_mul_ps(_mm_shuffle_ps(kB, kB, 170), kCol2)
        );
    }
}

void hkMatrix3::mul(const hkMatrix3& temp) {
    hkMatrix3 v3;
    v3.setMul(*this, temp);
    *this = v3;
}

void hkMatrix3::mul(const hkSimdReal& s) {
    m_col0.m_quad = _mm_mul_ps(_mm_shuffle_ps((&s)->m_real, (&s)->m_real, 0), m_col0.m_quad);
    m_col1.m_quad = _mm_mul_ps(_mm_shuffle_ps((&s)->m_real, (&s)->m_real, 0), m_col1.m_quad);
    m_col2.m_quad = _mm_mul_ps(_mm_shuffle_ps((&s)->m_real, (&s)->m_real, 0), m_col2.m_quad);
}

// this = a times the transpose of b: b's transpose goes into a local through setTranspose, a's columns are loaded,
// then this's columns are written from the last to the first.
void hkMatrix3::setMulInverse(const hkMatrix3& aTb, const hkMatrix3& cTb) {
    hkMatrix3 kBt;
    kBt.setTranspose(cTb);
    const __m128 kCol0 = aTb.m_col0.m_quad;
    const __m128 kCol1 = aTb.m_col1.m_quad;
    const __m128 kCol2 = aTb.m_col2.m_quad;
    for (int32_t i = 2; i >= 0; --i) {
        const __m128 kT = kBt.getColumn(i).m_quad;
        (&m_col0)[i].m_quad = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kT, kT, 0), kCol0), _mm_mul_ps(_mm_shuffle_ps(kT, kT, 85), kCol1)),
            _mm_mul_ps(_mm_shuffle_ps(kT, kT, 170), kCol2)
        );
    }
}

hkBool hkMatrix3::isOk() const {
    hkBool result;
    hkBool v4;
    hkBool v5;
    hkBool v6;
    if ((&(v4 = m_col0.isOk3()))->m_bool && ((&(v5 = m_col1.isOk3()))->m_bool && (&(v6 = m_col2.isOk3()))->m_bool)) {
        result.m_bool = 1;
        return result;
    }
    else {
        result.m_bool = 0;
        return result;
    }
}

hkBool hkMatrix3::isApproximatelyEqual(const hkMatrix3& m, float zero) const {
    hkBool result;
    __m128 v4 = _mm_shuffle_ps(_mm_set_ss(zero), _mm_set_ss(zero), 0);
    if ((_mm_movemask_ps(
             _mm_cmple_ps(_mm_and_ps(_mm_sub_ps(m_col0.m_quad, m.m_col0.m_quad), hkVector4_setAbs4_absMask), v4)
         ) &
         7) == 7 &&
        ((_mm_movemask_ps(
              _mm_cmple_ps(_mm_and_ps(_mm_sub_ps(m_col1.m_quad, m.m_col1.m_quad), hkVector4_setAbs4_absMask), v4)
          ) &
          7) == 7 &&
         (_mm_movemask_ps(
              _mm_cmple_ps(_mm_and_ps(_mm_sub_ps(m_col2.m_quad, m.m_col2.m_quad), hkVector4_setAbs4_absMask), v4)
          ) &
          7) == 7)) {
        result.m_bool = 1;
        return result;
    }
    else {
        result.m_bool = 0;
        return result;
    }
}

// Builds the transpose with the w components zeroed, then compares through isApproximatelyEqual.
bool hkMatrix3::isSymmetric(float epsilon) const {
    hkMatrix3 kTransposed;
    kTransposed.m_col0.m_quad[0] = m_col0.m_quad[0];
    kTransposed.m_col0.m_quad[1] = m_col1.m_quad[0];
    kTransposed.m_col0.m_quad[2] = m_col2.m_quad[0];
    kTransposed.m_col0.m_quad[3] = 0.0f;
    kTransposed.m_col1.m_quad[0] = m_col0.m_quad[1];
    kTransposed.m_col1.m_quad[1] = m_col1.m_quad[1];
    kTransposed.m_col1.m_quad[2] = m_col2.m_quad[1];
    kTransposed.m_col1.m_quad[3] = 0.0f;
    kTransposed.m_col2.m_quad[0] = m_col0.m_quad[2];
    kTransposed.m_col2.m_quad[1] = m_col1.m_quad[2];
    kTransposed.m_col2.m_quad[2] = m_col2.m_quad[2];
    kTransposed.m_col2.m_quad[3] = 0.0f;
    return isApproximatelyEqual(kTransposed, epsilon).m_bool != 0;
}

hkSimdReal hkMatrix3::getDeterminant() const {
    hkSimdReal result;
    __m128 v2 = _mm_undefined_ps();
    __m128 v3 = _mm_undefined_ps();
    hkSimdReal v8;
    v2[0] = 0.0f;
    v3[0] = 0.0f;
    __m128 v5 = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_col0.m_quad[1]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_col2.m_quad[1])))
        ),
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_col1.m_quad[1]))), v2)
    );
    __m128 v6 = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_col0.m_quad[2]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_col2.m_quad[2])))
        ),
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_col1.m_quad[2]))), v3)
    );
    __m128 v7 = _mm_mul_ps(
        _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(v6, v6, 210), _mm_shuffle_ps(v5, v5, 201)),
            _mm_mul_ps(_mm_shuffle_ps(v6, v6, 201), _mm_shuffle_ps(v5, v5, 210))
        ),
        _mm_unpacklo_ps(
            _mm_unpacklo_ps(
                _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_col0.m_quad[0]))),
                _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_col2.m_quad[0u])))
            ),
            _mm_unpacklo_ps(
                _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_col1.m_quad[0u]))),
                _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
            )
        )
    );
    v8.m_real = _mm_shuffle_ps(v7, v7, 170);
    v8.m_real[0] = v8.m_real[0] + static_cast<float>(_mm_shuffle_ps(v7, v7, 85)[0] + v7[0]);
    result.m_real = v8.m_real;
    return result;
}

void hkMatrix3::add(const hkMatrix3& a) {
    m_col0.m_quad = _mm_add_ps(a.m_col0.m_quad, m_col0.m_quad);
    m_col1.m_quad = _mm_add_ps(a.m_col1.m_quad, m_col1.m_quad);
    m_col2.m_quad = _mm_add_ps(a.m_col2.m_quad, m_col2.m_quad);
}

void hkMatrix3::changeBasis(const hkRotation& r) {
    hkMatrix3 arB;
    arB.setMulInverse(*this, r);
    setMul(r, arB);
}

void hkMatrix3::transpose() {
    float v1 = m_col1.m_quad[0];
    m_col1.m_quad[0] = m_col0.m_quad[1];
    m_col0.m_quad[1] = v1;
    float v2 = m_col2.m_quad[0];
    m_col2.m_quad[0] = m_col0.m_quad[2];
    m_col0.m_quad[2] = v2;
    float v3 = m_col2.m_quad[1];
    m_col2.m_quad[1] = m_col1.m_quad[2];
    m_col1.m_quad[2] = v3;
}

// KEEP: the exe's setMulInverse (0xCD5610) keeps edx live across its call of this function (`mov edx, ecx` before, `add
// edx, 0x20` after), which this body leaves alone and clang does not guarantee.
void hkMatrix3::setTranspose(const hkMatrix3& s) {
    m_col0(0) = s.m_col0(0);
    m_col1(1) = s.m_col1(1);
    m_col2(2) = s.m_col2(2);
    m_col0(3) = 0.0f;
    m_col1(3) = 0.0f;
    m_col2(3) = 0.0f;
    m_col0(1) = s.m_col1(0);
    m_col1(0) = s.m_col0(1);
    m_col0(2) = s.m_col2(0);
    m_col2(0) = s.m_col0(2);
    m_col1(2) = s.m_col2(1);
    m_col2(1) = s.m_col1(2);
}

void hkMatrix3::setCrossSkewSymmetric(const hkVector4& r) {
    __m128 v2 = _mm_undefined_ps();
    __m128 v3 = _mm_undefined_ps();
    __m128 v4 = _mm_undefined_ps();
    hkVector4 v7;
    __m128 v5 = _mm_set_ss(std::bit_cast<float>(0x80000000u));
    v3[0] = -0.0f - r.m_quad[1];
    v4[0] = 0.0f;
    v2[0] = 0.0f;
    __m128 v6 = _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(r.m_quad[2]))), v4);
    v7.m_quad = _mm_unpacklo_ps(_mm_unpacklo_ps(v2, v3), v6);
    m_col0.m_quad = v7.m_quad;
    v7.m_quad[0] = -0.0f - r.m_quad[2];
    v6[0] = 0.0f;
    __m128 v8 = _mm_unpacklo_ps(v6, v4);
    m_col1.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(v7.m_quad, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(r.m_quad[0])))), v8
    );
    v5[0] = -0.0f - r.m_quad[0];
    v8[0] = 0.0f;
    m_col2.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(r.m_quad[1]))), v8),
        _mm_unpacklo_ps(v5, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero))))
    );
}

void hkMatrix3::sub(const hkMatrix3& a) {
    m_col0.m_quad = _mm_sub_ps(m_col0.m_quad, a.m_col0.m_quad);
    m_col1.m_quad = _mm_sub_ps(m_col1.m_quad, a.m_col1.m_quad);
    m_col2.m_quad = _mm_sub_ps(m_col2.m_quad, a.m_col2.m_quad);
}

hkResult hkMatrix3::invert(float epsilon) {
    hkVector4 v2;
    hkVector4 v3;
    hkVector4 v7;
    hkVector4 v10;
    hkVector4 v13;
    v2.m_quad = m_col1.m_quad;
    v3.m_quad = m_col2.m_quad;
    __m128 v4 = _mm_shuffle_ps(v2.m_quad, v2.m_quad, 210);
    __m128 v5 = _mm_shuffle_ps(v3.m_quad, v3.m_quad, 201);
    __m128 v6 = _mm_mul_ps(_mm_shuffle_ps(v3.m_quad, v3.m_quad, 210), _mm_shuffle_ps(v2.m_quad, v2.m_quad, 201));
    v7.m_quad = m_col0.m_quad;
    __m128 v8 = _mm_shuffle_ps(v7.m_quad, v7.m_quad, 210);
    __m128 v9 = _mm_shuffle_ps(v7.m_quad, v7.m_quad, 201);
    v10.m_quad = m_col1.m_quad;
    __m128 v11 = _mm_shuffle_ps(v10.m_quad, v10.m_quad, 210);
    __m128 v25 = _mm_shuffle_ps(v10.m_quad, v10.m_quad, 201);
    __m128 v12 = _mm_sub_ps(v6, _mm_mul_ps(v5, v4));
    v13.m_quad = m_col2.m_quad;
    __m128 v14 = _mm_mul_ps(m_col0.m_quad, v12);
    float v15 = _mm_shuffle_ps(v14, v14, 170)[0] + static_cast<float>(_mm_shuffle_ps(v14, v14, 85)[0] + v14[0]);
    float v21 = std::fabs((double)v15);
    __m128 v16 = _mm_shuffle_ps(v13.m_quad, v13.m_quad, 201);
    __m128 v17 = _mm_shuffle_ps(v13.m_quad, v13.m_quad, 210);
    if (!(v21 > epsilon * (epsilon * epsilon))) {
        return HK_FAILURE;
    }
    __m128 v18 = _mm_set_ss(std::bit_cast<float>(0x3F800000u));
    v18[0] = 1.0f / v15;
    __m128 v19 = _mm_shuffle_ps(v18, v18, 0);
    m_col0.m_quad = _mm_mul_ps(v19, v12);
    m_col1.m_quad = _mm_mul_ps(_mm_sub_ps(_mm_mul_ps(v16, v8), _mm_mul_ps(v17, v9)), v19);
    m_col2.m_quad = _mm_mul_ps(_mm_sub_ps(_mm_mul_ps(v9, v11), _mm_mul_ps(v8, v25)), v19);
    float v22 = m_col1.m_quad[0u];
    m_col1.m_quad[0u] = m_col0.m_quad[1];
    m_col0.m_quad[1] = v22;
    float v23 = m_col2.m_quad[0u];
    m_col2.m_quad[0u] = m_col0.m_quad[2];
    m_col0.m_quad[2] = v23;
    float v24 = m_col2.m_quad[1];
    m_col2.m_quad[1] = m_col1.m_quad[2];
    m_col1.m_quad[2] = v24;
    return HK_SUCCESS;
}

// The threshold is kept as a float and each off-diagonal norm, rounded to float, is compared with it; the loop goes on
// while the norm is not below the threshold (NaN included). The eigenvalues are written on both exits.
hkResult
hkMatrix3::diagonalizeSymmetric(hkMatrix3& eigenVec, hkVector4& eigenVal, int32_t maxIter, float epsilon) const {
    if (!isSymmetric(epsilon)) {
        return HK_FAILURE;
    }
    hkMatrix3 kM;
    kM.m_col0.m_quad = m_col0.m_quad;
    kM.m_col1.m_quad = m_col1.m_quad;
    kM.m_col2.m_quad = m_col2.m_quad;
    // EXE-INLINED none: hkRotation::setIdentity
    eigenVec.m_col0.m_quad = hkQuadReal{0.0f, 0.0f, 0.0f, 0.0f};
    eigenVec.m_col1.m_quad = hkQuadReal{0.0f, 0.0f, 0.0f, 0.0f};
    eigenVec.m_col2.m_quad = hkQuadReal{0.0f, 0.0f, 0.0f, 0.0f};
    eigenVec.m_col0.m_quad[0] = 1.0f;
    eigenVec.m_col1.m_quad[1] = 1.0f;
    eigenVec.m_col2.m_quad[2] = 1.0f;
    const float fThreshold = static_cast<float>(_frobeniusNorm(this) * epsilon);
    int32_t iIter = 0;
    float fOffDiag = static_cast<float>(kM._offDiagNorm());
    while (!(fOffDiag < fThreshold) && iIter < maxIter) {
        int32_t iP = 0;
        int32_t iQ = 0;
        _findLargestOffDiagEntry(&kM, &iP, &iQ);
        SchurMatrix kRotation;
        _constructSchurMatrix(&kM, iP, iQ, &kRotation);
        hkMatrix3 kR;
        setJacobiRotation(&kRotation, &kR);
        hkMatrix3 kRt;
        kRt.m_col0.m_quad = kR.m_col0.m_quad;
        kRt.m_col1.m_quad = kR.m_col1.m_quad;
        kRt.m_col2.m_quad = kR.m_col2.m_quad;
        kRt.transpose();
        kM.mul(kR);
        kM.setMul(kRt, kM);
        eigenVec.mul(kR);
        ++iIter;
        fOffDiag = static_cast<float>(kM._offDiagNorm());
    }
    eigenVal.m_quad = hkQuadReal{kM.m_col0.m_quad[0], kM.m_col1.m_quad[1], kM.m_col2.m_quad[2], 0.0f};
    return fThreshold < fOffDiag ? HK_FAILURE : HK_SUCCESS;
}

// The sum of squares of the off-diagonal elements, row by row, kept as a float; its root is rounded to float.
double hkMatrix3::_offDiagNorm() const {
    float fValue = 0.0f;
    for (int32_t i = 0; i < 3; ++i) {
        if (i != 0) {
            fValue = m_col0.m_quad[i] * m_col0.m_quad[i] + fValue;
        }
        if (i != 1) {
            fValue = m_col1.m_quad[i] * m_col1.m_quad[i] + fValue;
        }
        if (i != 2) {
            fValue = m_col2.m_quad[i] * m_col2.m_quad[i] + fValue;
        }
    }
    return static_cast<float>(std::sqrt((double)fValue));
}

// this = the transpose of a times b, column by column from the first; a's transpose is built in registers before
// any store and each column of b is read just before this's column of the same index is written.
void hkMatrix3::setMulInverseMul(const hkRotation& bTa, const hkMatrix3& bTc) {
    const __m128 kCol1 = bTa.m_col1.m_quad;
    const __m128 kCol2 = bTa.m_col2.m_quad;
    const __m128 kLo = _mm_unpacklo_ps(bTa.m_col0.m_quad, kCol1);
    const __m128 kHi = _mm_unpackhi_ps(bTa.m_col0.m_quad, kCol1);
    const __m128 kRow0 = _mm_movelh_ps(kLo, kCol2);
    const __m128 kRow1 = _mm_shuffle_ps(_mm_movehl_ps(kRow0, kLo), kCol2, 212);
    const __m128 kRow2 = _mm_shuffle_ps(kHi, kCol2, 228);
    for (int32_t i = 0; i < 3; ++i) {
        const __m128 kB = bTc.getColumn(i).m_quad;
        (&m_col0)[i].m_quad = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kB, kB, 85), kRow1), _mm_mul_ps(_mm_shuffle_ps(kB, kB, 0), kRow0)),
            _mm_mul_ps(_mm_shuffle_ps(kB, kB, 170), kRow2)
        );
    }
}

__m128* hkMatrix3::setMulScalar(__m128* a1, __m128* a2) {
    m_col0.m_quad = _mm_mul_ps(_mm_shuffle_ps(*a1, *a1, 0), *a2);
    m_col1.m_quad = _mm_mul_ps(_mm_shuffle_ps(*a1, *a1, 0), a2[1]);
    m_col2.m_quad = _mm_mul_ps(_mm_shuffle_ps(*a1, *a1, 0), a2[2]);
    return a1;
}

__m128* hkMatrix3::addMul(__m128* a1, __m128* a2) {
    m_col0.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(*a1, *a1, 0), *a2), m_col0.m_quad);
    m_col1.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(*a1, *a1, 0), a2[1]), m_col1.m_quad);
    m_col2.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(*a1, *a1, 0), a2[2]), m_col2.m_quad);
    return a1;
}

void hkMatrix3::invertSymmetric() {
    hkVector4 v1;
    hkVector4 v2;
    hkVector4 v5;
    uint32_t v14;
    v1.m_quad = m_col1.m_quad;
    v2.m_quad = m_col2.m_quad;
    __m128 v3 = _mm_mul_ps(_mm_shuffle_ps(v1.m_quad, v1.m_quad, 201), _mm_shuffle_ps(v2.m_quad, v2.m_quad, 210));
    __m128 v4 = _mm_mul_ps(_mm_shuffle_ps(v1.m_quad, v1.m_quad, 210), _mm_shuffle_ps(v2.m_quad, v2.m_quad, 201));
    v5.m_quad = m_col0.m_quad;
    __m128 v6 = _mm_sub_ps(v3, v4);
    __m128 v7 = _mm_shuffle_ps(v5.m_quad, v5.m_quad, 210);
    __m128 v8 = _mm_shuffle_ps(v5.m_quad, v5.m_quad, 201);
    __m128 v9 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(m_col2.m_quad, m_col2.m_quad, 201), v7),
        _mm_mul_ps(_mm_shuffle_ps(m_col2.m_quad, m_col2.m_quad, 210), v8)
    );
    __m128 v10 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(m_col1.m_quad, m_col1.m_quad, 210), v8),
        _mm_mul_ps(_mm_shuffle_ps(m_col1.m_quad, m_col1.m_quad, 201), v7)
    );
    __m128 v11 = _mm_mul_ps(m_col0.m_quad, v6);
    float v13 = _mm_shuffle_ps(v11, v11, 170)[0] + static_cast<float>(_mm_shuffle_ps(v11, v11, 85)[0] + v11[0]);
    if (v13 < 1.6940658945086007e-21) {
        v13 = 1.6940659e-21f;
    }
    *reinterpret_cast<float*>(&v14) = 1.0 / v13;
    __m128 v12 = _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v14)), _mm_set_ss(std::bit_cast<float>(v14)), 0);
    m_col0.m_quad = _mm_mul_ps(v12, v6);
    m_col1.m_quad = _mm_mul_ps(v12, v9);
    m_col2.m_quad = _mm_mul_ps(v12, v10);
}

// The engine keeps one identity for the whole program: the rotation of the identity transform.
const hkMatrix3& hkMatrix3::getIdentity() {
    return hkTransformIdentity.getRotation();
}

// The sum of squares of all nine elements, row by row, kept as a float; its root is rounded to float.
double hkMatrix3::_frobeniusNorm(const hkMatrix3* apM) {
    float fValue = 0.0f;
    for (int32_t i = 0; i < 3; ++i) {
        fValue = apM->m_col0.m_quad[i] * apM->m_col0.m_quad[i] + fValue;
        fValue = apM->m_col1.m_quad[i] * apM->m_col1.m_quad[i] + fValue;
        fValue = apM->m_col2.m_quad[i] * apM->m_col2.m_quad[i] + fValue;
    }
    return static_cast<float>(std::sqrt((double)fValue));
}

void hkMatrix3::_findLargestOffDiagEntry(const hkMatrix3* apM, int32_t* apP, int32_t* apQ) {
    float v5 = 0.0f;
    int32_t v3 = 0;
    do {
        if (v3 < 0) {
            float v6 = std::fabs((double)apM->m_col0.m_quad[v3]);
            if (v5 <= static_cast<double>(v6)) {
                *apP = v3;
                v5 = v6;
                *apQ = 0;
            }
        }
        if (v3 < 1) {
            float v7 = std::fabs((double)apM->m_col1.m_quad[v3]);
            if (v5 <= static_cast<double>(v7)) {
                *apP = v3;
                v5 = v7;
                *apQ = 1;
            }
        }
        if (v3 < 2) {
            float v8 = std::fabs((double)apM->m_col2.m_quad[v3]);
            if (v5 <= static_cast<double>(v8)) {
                *apP = v3;
                v5 = v8;
                *apQ = 2;
            }
        }
        ++v3;
    }
    while (v3 < 3);
}

// Theta, its square plus one, the root and t are kept as floats. The 1.0 added to theta squared is a double constant.
void hkMatrix3::_constructSchurMatrix(const hkMatrix3* apM, int32_t aiP, int32_t aiQ, SchurMatrix* apRotOut) {
    const float& fApq = apM->getColumn(aiQ)(aiP);
    apRotOut->p = aiP;
    apRotOut->q = aiQ;
    if (0.0f == fApq) {
        apRotOut->c = 1.0f;
        apRotOut->s = 0.0f;
    }
    else {
        double v7;
        float fTheta = (apM->getColumn(aiQ)(aiQ) - apM->getColumn(aiP)(aiP)) / (fApq + fApq);
        float fSquare = fTheta * fTheta + 1.0;
        float fRoot = std::sqrt((double)fSquare);
        double v6 = fTheta;
        if (!(fTheta >= 0.0f)) {
            v7 = v6 - fRoot;
        }
        else {
            v7 = v6 + fRoot;
        }
        float fT = 1.0 / v7;
        float v9 = fT * fT + 1.0;
        float v10 = std::sqrt((double)v9);
        float c = 1.0 / v10;
        apRotOut->c = c;
        apRotOut->s = c * fT;
    }
}

// Zeroes the three columns, stores 1.0 on the diagonal, then writes c at (p, p) and (q, q), s at row p of column q and
// -s at row q of column p, indexing the matrix as twelve floats with the rotation's p and q.
void hkMatrix3::setJacobiRotation(const SchurMatrix* apRot, hkMatrix3* apOut) {
    apOut->m_col0.m_quad = hkQuadReal{0.0f, 0.0f, 0.0f, 0.0f};
    apOut->m_col1.m_quad = hkQuadReal{0.0f, 0.0f, 0.0f, 0.0f};
    apOut->m_col2.m_quad = hkQuadReal{0.0f, 0.0f, 0.0f, 0.0f};
    apOut->m_col0(0) = 1.0f;
    apOut->m_col1(1) = 1.0f;
    apOut->m_col2(2) = 1.0f;
    (*apOut)(apRot->p, apRot->p) = apRot->c;
    (*apOut)(apRot->p, apRot->q) = apRot->s;
    (*apOut)(apRot->q, apRot->p) = -apRot->s;
    (*apOut)(apRot->q, apRot->q) = apRot->c;
}

// The SDK's hkMatrix3_setMulMat3Mat3: each column of apResult is apA times that column of apB, last column first.
void hkMatrix3_setMulMat3Mat3(hkMatrix3* apResult, const hkMatrix3* apA, const hkMatrix3* apB) {
    const __m128 col0 = apA->m_col0.m_quad;
    const __m128 col1 = apA->m_col1.m_quad;
    const __m128 col2 = apA->m_col2.m_quad;
    for (int32_t i = 2; i >= 0; --i) {
        const __m128 b = apB->getColumn(i).m_quad;
        (&apResult->m_col0)[i].m_quad = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(b, b, 0), col0), _mm_mul_ps(_mm_shuffle_ps(b, b, 85), col1)),
            _mm_mul_ps(_mm_shuffle_ps(b, b, 170), col2)
        );
    }
}

void hkMatrix3_invertSymmetric(hkMatrix3* apM) {
    apM->invertSymmetric();
}
