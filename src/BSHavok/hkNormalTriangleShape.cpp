#include "BSHavok/hkNormalTriangleShape.h"

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"

#include <bit>
#include <xmmintrin.h>

hkBool hkNormalTriangleShape::castRay(const hkpShapeRayCastInput& arInput, hkpShapeRayCastOutput& arResults) const {
    hkBool result;
    __m128 v9 = _mm_undefined_ps();
    __m128 v4 = _mm_sub_ps(arInput.m_to.m_quad, arInput.m_from.m_quad);
    __m128 v5 = _mm_mul_ps(v4, v4);
    float v6 = _mm_shuffle_ps(v5, v5, 85)[0] + v5[0];
    __m128 v7 = _mm_shuffle_ps(v5, v5, 170);
    v7[0] = v7[0] + v6;
    float v8 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v7[0])));
    v9[0] = std::bit_cast<float>(static_cast<int32_t>(0.0f));
    __m128 v10 = _mm_cmpeq_ss(v7, v9);
    v9[0] = 3.0f - static_cast<float>(static_cast<float>(v7[0] * v8) * v8);
    __m128 v11 = _mm_setzero_ps();
    v11[0] = static_cast<float>(0.5f * v8) * v9[0];
    __m128 v12 = _mm_andnot_ps(v10, v11);
    __m128 v13 = _mm_mul_ps(_mm_mul_ps(_mm_shuffle_ps(v12, v12, 0), v4), Normal.m_quad);
    float v16 = _mm_shuffle_ps(v13, v13, 170)[0] + static_cast<float>(_mm_shuffle_ps(v13, v13, 85)[0] + v13[0]);
    if (v16 < 0.0f) {
        hkBool v15 = castRayImpl(arInput, arResults);
        result.m_bool = v15.m_bool != 0;
    }
    else {
        result.m_bool = 0;
    }
    return result;
}
