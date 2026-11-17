#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/Util/hkpCharacterMovementUtil.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

void hkpCharacterMovementUtil::calculateMovement(
    const hkpCharacterMovementUtil::hkpMovementUtilInput& input,
    hkVector4& velocityOut
) {
    uint32_t v38;
    __m128 v2 = _mm_sub_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(input.m_up.m_quad, input.m_up.m_quad, 210),
            _mm_shuffle_ps(input.m_forward.m_quad, input.m_forward.m_quad, 201)
        ),
        _mm_mul_ps(
            _mm_shuffle_ps(input.m_up.m_quad, input.m_up.m_quad, 201),
            _mm_shuffle_ps(input.m_forward.m_quad, input.m_forward.m_quad, 210)
        )
    );
    __m128 v3 = _mm_mul_ps(v2, v2);
    if (!(static_cast<float>(
              _mm_shuffle_ps(v3, v3, 170)[0] + static_cast<float>(_mm_shuffle_ps(v3, v3, 85)[0] + v3[0])
          ) < 1.1920929e-07f)) {
        __m128 v7 = _mm_undefined_ps();
        hkVector4 v22;
        hkVector4 v29;
        __m128 v34;
        hkVector4 a3;
        hkRotation a2;
        float v4 = _mm_shuffle_ps(v3, v3, 85)[0] + v3[0];
        __m128 v5 = _mm_shuffle_ps(v3, v3, 170);
        v5[0] = v5[0] + v4;
        float v6 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v5[0])));
        v7[0] = 0.0f;
        __m128 v8 = _mm_setzero_ps();
        v8[0] = 0.5f;
        __m128 v9 = _mm_cmpeq_ss(v5, v7);
        float v10 = 3.0f - static_cast<float>(static_cast<float>(v5[0] * v6) * v6);
        v8[0] = static_cast<float>(0.5f * v6) * v10;
        __m128 v12 = _mm_andnot_ps(v9, v8);
        __m128 v13 = _mm_mul_ps(_mm_shuffle_ps(v12, v12, 0), v2);
        __m128 v14 = _mm_sub_ps(
            _mm_mul_ps(
                _mm_shuffle_ps(v13, v13, 201),
                _mm_shuffle_ps(input.m_surfaceNormal.m_quad, input.m_surfaceNormal.m_quad, 210)
            ),
            _mm_mul_ps(
                _mm_shuffle_ps(v13, v13, 210),
                _mm_shuffle_ps(input.m_surfaceNormal.m_quad, input.m_surfaceNormal.m_quad, 201)
            )
        );
        __m128 v15 = _mm_mul_ps(v14, v14);
        v12[0] = _mm_shuffle_ps(v15, v15, 85)[0] + v15[0];
        __m128 v16 = _mm_shuffle_ps(v15, v15, 170);
        v16[0] = v16[0] + v12[0];
        float v17 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v16[0])));
        float v18 = 3.0f - static_cast<float>(static_cast<float>(v16[0] * v17) * v17);
        __m128 v19 = _mm_cmpeq_ss(v16, v7);
        v8[0] = static_cast<float>(0.5f * v17) * v18;
        __m128 v21 = _mm_andnot_ps(v19, v8);
        v22.m_quad = _mm_mul_ps(_mm_shuffle_ps(v21, v21, 0), v14);
        __m128 v23 = _mm_sub_ps(
            _mm_mul_ps(
                _mm_shuffle_ps(v22.m_quad, v22.m_quad, 201),
                _mm_shuffle_ps(input.m_surfaceNormal.m_quad, input.m_surfaceNormal.m_quad, 210)
            ),
            _mm_mul_ps(
                _mm_shuffle_ps(v22.m_quad, v22.m_quad, 210),
                _mm_shuffle_ps(input.m_surfaceNormal.m_quad, input.m_surfaceNormal.m_quad, 201)
            )
        );
        __m128 v24 = _mm_mul_ps(v23, v23);
        float v25 = _mm_shuffle_ps(v24, v24, 85)[0] + v24[0];
        __m128 v26 = _mm_shuffle_ps(v24, v24, 170);
        v26[0] = v26[0] + v25;
        float v27 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v26[0])));
        v8[0] = static_cast<float>(0.5f * v27) *
            static_cast<float>(3.0f - static_cast<float>(static_cast<float>(v26[0] * v27) * v27));
        __m128 v28 = _mm_andnot_ps(_mm_cmpeq_ss(v26, v7), v8);
        a2.m_col1.m_quad = _mm_mul_ps(_mm_shuffle_ps(v28, v28, 0), v23);
        a2.m_col2 = input.m_surfaceNormal;
        v29.m_quad = input.m_currentVelocity.m_quad;
        a2.m_col0.m_quad = v22.m_quad;
        a3.m_quad = _mm_sub_ps(v29.m_quad, input.m_surfaceVelocity.m_quad);
        a3.setRotatedInverseDir(a2, a3);
        double m_maxVelocityDelta = input.m_maxVelocityDelta;
        double m_gain = input.m_gain;
        __m128 v32 = _mm_sub_ps(input.m_desiredVelocity.m_quad, a3.m_quad);
        __m128 v33 = _mm_mul_ps(v32, v32);
        float v35 = _mm_shuffle_ps(v33, v33, 170)[0] + static_cast<float>(_mm_shuffle_ps(v33, v33, 85)[0] + v33[0]);
        if (!(m_gain * v35 * input.m_gain > m_maxVelocityDelta * m_maxVelocityDelta)) {
            v34 = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(input.m_gain), _mm_set_ss(input.m_gain), 0), v32);
        }
        else {
            float v36 = std::sqrt((double)v35);
            float v37 = 1.0 / v36;
            *reinterpret_cast<float*>(&v38) = v37 * input.m_maxVelocityDelta;
            v34 = _mm_mul_ps(
                _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v38)), _mm_set_ss(std::bit_cast<float>(v38)), 0), v32
            );
        }
        a3.m_quad = _mm_add_ps(v34, a3.m_quad);
        velocityOut.setRotatedDir(a2, a3);
        velocityOut.m_quad = _mm_add_ps(input.m_surfaceVelocity.m_quad, velocityOut.m_quad);
    }
}
