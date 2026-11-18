#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <cmath>
#include <xmmintrin.h>

hkRotation::hkRotation() {}

// Rebuilds the matrix from its own normalized quaternion.
void hkRotation::renormalize() {
    hkQuaternion kQuat;
    kQuat.setAndNormalize(*this);
    set(kQuat);
}

void hkRotation::set(const hkQuaternion& qi) {
    hkVector4 v8;
    hkVector4 v11;
    __m128 m_quad = qi.m_vec.m_quad;
    __m128 v3 = _mm_add_ps(m_quad, m_quad);
    float v4 = qi.m_vec.m_quad[0] * v3[0];
    __m128 v5 = _mm_xor_ps(_mm_mul_ps(_mm_shuffle_ps(m_quad, m_quad, 254), _mm_shuffle_ps(v3, v3, 26)), rdata_10C8770);
    v5[0] = v5[0] + 1.0f;
    __m128 v6 = _mm_mul_ps(_mm_shuffle_ps(v3, v3, 165), _mm_shuffle_ps(qi.m_vec.m_quad, qi.m_vec.m_quad, 65));
    __m128 v7 = _mm_xor_ps(v6, rdata_10C8780);
    v8.m_quad = _mm_sub_ps(v5, v7);
    v7[0] = v4;
    __m128 v9 = hkQuadReal1000;
    m_col0.m_quad = v8.m_quad;
    __m128 v10 = _mm_sub_ps(_mm_xor_ps(v5, rdata_10C8780), v7);
    v11.m_quad = _mm_shuffle_ps(v10, v10, 177);
    v9[0] = (1.0f - v4) - v6[0];
    m_col1.m_quad = v11.m_quad;
    m_col2.m_quad = _mm_shuffle_ps(_mm_movehl_ps(v8.m_quad, v11.m_quad), v9, 141);
}

// Builds the quaternion of the axis and angle, then writes its rotation.
void hkRotation::setAxisAngle(const hkVector4& axis, float angle) {
    hkQuaternion kQuat;
    kQuat.setAxisAngle(axis, angle);
    set(kQuat);
}

// Whether the matrix is valid: the base check's answer.
hkBool hkRotation::isOk() const {
    return hkMatrix3::isOk();
}

hkBool hkRotation::isOrthonormal(float a3) const {
    hkVector4 v3;
    __m128 v6;
    __m128 v7;
    float v10;
    float v11;
    float v12;
    float v13;
    v3.m_quad = m_col0.m_quad;
    __m128 v4 = _mm_mul_ps(v3.m_quad, v3.m_quad);
    float v8 = (_mm_shuffle_ps(v4, v4, 170)[0] + (_mm_shuffle_ps(v4, v4, 85)[0] + v4[0])) - 1.0;
    float v9 = std::fabs((double)v8);
    if (a3 < static_cast<double>(v9) ||
        ((v6 = _mm_mul_ps(m_col1.m_quad, m_col1.m_quad),
          (v10 = (_mm_shuffle_ps(v6, v6, 170)[0] + (_mm_shuffle_ps(v6, v6, 85)[0] + v6[0])) - 1.0,
           (v11 = std::fabs((double)v10), v11 > static_cast<double>(a3)))) ||
         (v7 = _mm_mul_ps(m_col2.m_quad, m_col2.m_quad),
          (v12 = (_mm_shuffle_ps(v7, v7, 170)[0] + (_mm_shuffle_ps(v7, v7, 85)[0] + v7[0])) - 1.0,
           (v13 = std::fabs((double)v12), v13 > static_cast<double>(a3)))))) {
        return false;
    }

    return (_mm_movemask_ps(_mm_cmple_ps(
                _mm_and_ps(
                    _mm_sub_ps(
                        _mm_sub_ps(
                            _mm_mul_ps(
                                _mm_shuffle_ps(m_col1.m_quad, m_col1.m_quad, 210),
                                _mm_shuffle_ps(v3.m_quad, v3.m_quad, 201)
                            ),
                            _mm_mul_ps(
                                _mm_shuffle_ps(m_col1.m_quad, m_col1.m_quad, 201),
                                _mm_shuffle_ps(v3.m_quad, v3.m_quad, 210)
                            )
                        ),
                        m_col2.m_quad
                    ),
                    hkVector4_setAbs4_absMask
                ),
                _mm_shuffle_ps(_mm_set_ss(a3), _mm_set_ss(a3), 0)
            )) &
            7) == 7;
}
