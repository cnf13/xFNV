#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransformUtil.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"

#include <bit>
#include <xmmintrin.h>

void hkSweptTransformUtil::setCentreOfRotationLocal(const hkVector4& newCenterOfRotation, hkMotionState& motionState) {
    hkVector4 v2;
    hkVector4 v4;
    hkVector4 v5;
    v2.m_quad = newCenterOfRotation.m_quad;
    v4.m_quad = _mm_sub_ps(newCenterOfRotation.m_quad, motionState.m_sweptTransform.m_centerOfMassLocal.m_quad);
    motionState.m_sweptTransform.m_centerOfMassLocal.m_quad = v2.m_quad;
    v5.setRotatedDir(motionState.m_transform.m_rotation, v4);
    __m128 v3 = _mm_and_ps(hkVector4_zeroElement_cx[3], v5.m_quad);
    motionState.m_sweptTransform.m_centerOfMass0.m_quad =
        _mm_add_ps(motionState.m_sweptTransform.m_centerOfMass0.m_quad, v3);
    motionState.m_sweptTransform.m_centerOfMass1.m_quad =
        _mm_add_ps(motionState.m_sweptTransform.m_centerOfMass1.m_quad, v3);
}

void hkSweptTransformUtil::warpToPosition(const hkVector4& position, hkMotionState& ms) {
    hkVector4 v2;
    hkVector4 v4;
    hkQuaternion v5;
    ms.m_deltaAngle.m_quad = _mm_setzero_ps();
    ms.m_transform.m_translation.m_quad = position.m_quad;
    v2.m_quad = ms.m_sweptTransform.m_centerOfMassLocal.m_quad;
    double v3 = ms.m_sweptTransform.m_centerOfMass0.m_quad[3];
    v4.m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(v2.m_quad, v2.m_quad, 85), ms.m_transform.m_rotation.m_col1.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(v2.m_quad, v2.m_quad, 0), ms.m_transform.m_rotation.m_col0.m_quad)
            ),
            _mm_mul_ps(_mm_shuffle_ps(v2.m_quad, v2.m_quad, 170), ms.m_transform.m_rotation.m_col2.m_quad)
        ),
        position.m_quad
    );
    ms.m_sweptTransform.m_centerOfMass0.m_quad = v4.m_quad;
    ms.m_sweptTransform.m_centerOfMass0.m_quad[3] = v3;
    ms.m_sweptTransform.m_centerOfMass1.m_quad = v4.m_quad;
    v5.m_vec.m_quad = ms.m_sweptTransform.m_rotation1.m_vec.m_quad;
    ms.m_sweptTransform.m_centerOfMass1.m_quad[3] = 0.0f;
    ms.m_sweptTransform.m_rotation0.m_vec.m_quad = v5.m_vec.m_quad;
}

void hkSweptTransformUtil::warpToRotation(const hkQuaternion& rotation, hkMotionState& ms) {
    warpTo(ms.m_transform.m_translation, rotation, ms);
}

void hkSweptTransformUtil::setTimeInformation(float startTime, float invDeltaTime, hkMotionState& motionState) {
    motionState.m_sweptTransform.m_centerOfMass0.m_quad[3] = startTime;
    motionState.m_sweptTransform.m_centerOfMass1.m_quad[3] = invDeltaTime;
}

void hkSweptTransformUtil::warpTo(const hkVector4& position, const hkQuaternion& rotation, hkMotionState& ms) {
    hkVector4 v3;
    hkVector4 v5;
    ms.m_deltaAngle.m_quad = _mm_setzero_ps();
    ms.m_sweptTransform.m_rotation0.m_vec.m_quad = rotation.m_vec.m_quad;
    ms.m_sweptTransform.m_rotation1.m_vec.m_quad = rotation.m_vec.m_quad;
    ms.m_transform.m_rotation.set(rotation);
    ms.m_transform.m_translation.m_quad = position.m_quad;
    v3.m_quad = ms.m_sweptTransform.m_centerOfMassLocal.m_quad;
    double v4 = ms.m_sweptTransform.m_centerOfMass0.m_quad[3];
    v5.m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(v3.m_quad, v3.m_quad, 85), ms.m_transform.m_rotation.m_col1.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(v3.m_quad, v3.m_quad, 0), ms.m_transform.m_rotation.m_col0.m_quad)
            ),
            _mm_mul_ps(_mm_shuffle_ps(v3.m_quad, v3.m_quad, 170), ms.m_transform.m_rotation.m_col2.m_quad)
        ),
        position.m_quad
    );
    ms.m_sweptTransform.m_centerOfMass0.m_quad = v5.m_quad;
    ms.m_sweptTransform.m_centerOfMass1.m_quad = v5.m_quad;
    ms.m_sweptTransform.m_centerOfMass0.m_quad[3] = v4;
    ms.m_sweptTransform.m_centerOfMass1.m_quad[3] = 0.0f;
}

void hkSweptTransformUtil::warpTo(const hkTransform& transform, hkMotionState& ms) {
    hkVector4 v3;
    hkVector4 v5;
    hkQuaternion v6;
    ms.m_deltaAngle.m_quad = _mm_setzero_ps();
    v6.set(transform.m_rotation);
    ms.m_transform = transform;
    __m128 m_quad = v6.m_vec.m_quad;
    ms.m_sweptTransform.m_rotation0.m_vec.m_quad = v6.m_vec.m_quad;
    ms.m_sweptTransform.m_rotation1.m_vec.m_quad = m_quad;
    v3.m_quad = ms.m_sweptTransform.m_centerOfMassLocal.m_quad;
    double v4 = ms.m_sweptTransform.m_centerOfMass0.m_quad[3];
    v5.m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(v3.m_quad, v3.m_quad, 85), transform.m_rotation.m_col1.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(v3.m_quad, v3.m_quad, 0), transform.m_rotation.m_col0.m_quad)
            ),
            _mm_mul_ps(_mm_shuffle_ps(v3.m_quad, v3.m_quad, 170), transform.m_rotation.m_col2.m_quad)
        ),
        transform.m_translation.m_quad
    );
    ms.m_sweptTransform.m_centerOfMass0.m_quad = v5.m_quad;
    ms.m_sweptTransform.m_centerOfMass1.m_quad = v5.m_quad;
    ms.m_sweptTransform.m_centerOfMass0.m_quad[3] = v4;
    ms.m_sweptTransform.m_centerOfMass1.m_quad[3] = 0.0f;
}

void hkSweptTransformUtil::lerp2(const hkSweptTransform& sweptTrans, float t, hkTransform& transformOut) {
    hkQuaternion v3;
    hkQuaternion v4;
    __m128 v11;
    __m128 v12;
    __m128 v19;
    uint32_t v25;
    uint32_t v26;
    uint32_t v27;
    hkQuaternion arQuaternion;
    v3.m_vec.m_quad = sweptTrans.m_rotation1.m_vec.m_quad;
    v4.m_vec.m_quad = sweptTrans.m_rotation0.m_vec.m_quad;
    __m128 v5 = _mm_add_ps(v3.m_vec.m_quad, v4.m_vec.m_quad);
    __m128 v6 = _mm_mul_ps(v5, v5);
    float v28 = (t - sweptTrans.m_centerOfMass0.m_quad[3]) * sweptTrans.m_centerOfMass1.m_quad[3];
    __m128 v7 = _mm_add_ps(_mm_shuffle_ps(v6, v6, 78), v6);
    v6[0] = _mm_shuffle_ps(v7, v7, 177)[0] + v7[0];
    arQuaternion.m_vec.m_quad = v5;
    float v24 = 0.75 - 0.125 * v6[0];
    *reinterpret_cast<float*>(&v25) = (1.5 - v6[0] * 0.5 * v24 * v24) * v24;
    __m128 v8 =
        _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v25)), _mm_set_ss(std::bit_cast<float>(v25)), 0), v5);
    double v9 = v28 + v28;
    if (!(v28 < 0.5f)) {
        *reinterpret_cast<float*>(&v27) = v9 - 1.0;
        __m128 v13 = _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v27)), _mm_set_ss(std::bit_cast<float>(v27)), 0);
        v11 = _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v13), v8);
        v12 = _mm_mul_ps(v13, v3.m_vec.m_quad);
    }
    else {
        *reinterpret_cast<float*>(&v26) = v9;
        __m128 v10 = _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v26)), _mm_set_ss(std::bit_cast<float>(v26)), 0);
        v11 = _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v10), v4.m_vec.m_quad);
        v12 = _mm_mul_ps(v10, v8);
    }
    __m128 v14 = _mm_add_ps(v11, v12);
    __m128 v15 = _mm_mul_ps(v14, v14);
    __m128 v16 = _mm_add_ps(_mm_shuffle_ps(v15, v15, 78), v15);
    __m128 v17 = _mm_add_ps(_mm_shuffle_ps(v16, v16, 177), v16);
    float v18 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v17[0])));
    v19[0] = std::bit_cast<float>(static_cast<int32_t>(0.0f));
    __m128 v20 = _mm_cmpeq_ss(v17, v19);
    v19[0] = 3.0f - ((v17[0] * v18) * v18);
    __m128 v21 = _mm_setzero_ps();
    v21[0] = (0.5f * v18) * v19[0];
    __m128 v22 = _mm_andnot_ps(v20, v21);
    arQuaternion.m_vec.m_quad = _mm_mul_ps(_mm_shuffle_ps(v22, v22, 0), v14);
    transformOut.m_rotation.set(arQuaternion);
    __m128 v23 = _mm_shuffle_ps(_mm_set_ss(v28), _mm_set_ss(v28), 0);
    transformOut.m_translation.m_quad = _mm_add_ps(
        _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v23), sweptTrans.m_centerOfMass0.m_quad),
        _mm_mul_ps(sweptTrans.m_centerOfMass1.m_quad, v23)
    );
    transformOut.m_translation.m_quad = _mm_sub_ps(
        transformOut.m_translation.m_quad,
        _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(sweptTrans.m_centerOfMassLocal.m_quad, sweptTrans.m_centerOfMassLocal.m_quad, 85),
                    transformOut.m_rotation.m_col1.m_quad
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(sweptTrans.m_centerOfMassLocal.m_quad, sweptTrans.m_centerOfMassLocal.m_quad, 0),
                    transformOut.m_rotation.m_col0.m_quad
                )
            ),
            _mm_mul_ps(
                _mm_shuffle_ps(sweptTrans.m_centerOfMassLocal.m_quad, sweptTrans.m_centerOfMassLocal.m_quad, 170),
                transformOut.m_rotation.m_col2.m_quad
            )
        )
    );
}

void hkSweptTransformUtil::backStepMotionState(float time, hkMotionState& motionState) {
    hkQuaternion v3;
    hkQuaternion v4;
    hkQuaternion v6;
    hkQuaternion v14;
    __m128 v20;
    uint32_t v27;
    uint32_t v28;
    uint32_t v29;
    float v31 = (time - motionState.m_sweptTransform.m_centerOfMass0.m_quad[3]) *
        motionState.m_sweptTransform.m_centerOfMass1.m_quad[3];
    double v2 = 1.1920929e-07f;
    if (v31 > 1.1920929e-07f) {
        v2 = v31;
    }
    v3.m_vec.m_quad = motionState.m_sweptTransform.m_rotation1.m_vec.m_quad;
    float v32 = v2;
    v4.m_vec.m_quad = motionState.m_sweptTransform.m_rotation0.m_vec.m_quad;
    hkQuaternion* p_m_rotation1 = &motionState.m_sweptTransform.m_rotation1;
    v6.m_vec.m_quad = _mm_add_ps(v3.m_vec.m_quad, v4.m_vec.m_quad);
    __m128 v7 = _mm_mul_ps(v6.m_vec.m_quad, v6.m_vec.m_quad);
    __m128 v8 = _mm_add_ps(_mm_shuffle_ps(v7, v7, 78), v7);
    double v9 = (_mm_shuffle_ps(v8, v8, 177)[0] + v8[0]);
    motionState.m_sweptTransform.m_rotation1.m_vec.m_quad = v6.m_vec.m_quad;
    float v26 = 0.75 - 0.125 * v9;
    *reinterpret_cast<float*>(&v27) = (1.5 - v9 * 0.5 * v26 * v26) * v26;
    __m128 v10 = _mm_mul_ps(
        _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v27)), _mm_set_ss(std::bit_cast<float>(v27)), 0), v6.m_vec.m_quad
    );
    motionState.m_sweptTransform.m_rotation1.m_vec.m_quad = v10;
    double v11 = v32;
    double v12 = v11 + v11;
    if (!(v32 < 0.5f)) {
        *reinterpret_cast<float*>(&v29) = v12 - 1.0;
        __m128 v15 = _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v29)), _mm_set_ss(std::bit_cast<float>(v29)), 0);
        v14.m_vec.m_quad =
            _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v15), v10), _mm_mul_ps(v15, v3.m_vec.m_quad));
    }
    else {
        *reinterpret_cast<float*>(&v28) = v12;
        __m128 v13 = _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v28)), _mm_set_ss(std::bit_cast<float>(v28)), 0);
        v14.m_vec.m_quad =
            _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v13), v4.m_vec.m_quad), _mm_mul_ps(v10, v13));
    }
    (*p_m_rotation1).m_vec.m_quad = v14.m_vec.m_quad;
    __m128 v16 = _mm_mul_ps(p_m_rotation1->m_vec.m_quad, p_m_rotation1->m_vec.m_quad);
    __m128 v17 = _mm_add_ps(_mm_shuffle_ps(v16, v16, 78), v16);
    __m128 v18 = _mm_add_ps(_mm_shuffle_ps(v17, v17, 177), v17);
    float v19 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v18[0])));
    v20[0] = std::bit_cast<float>(static_cast<int32_t>(0.0f));
    __m128 v21 = _mm_cmpeq_ss(v18, v20);
    v20[0] = 3.0f - ((v18[0] * v19) * v19);
    __m128 v22 = _mm_setzero_ps();
    v22[0] = (0.5f * v19) * v20[0];
    __m128 v23 = _mm_andnot_ps(v21, v22);
    __m128 v24 = _mm_mul_ps(_mm_shuffle_ps(v23, v23, 0), p_m_rotation1->m_vec.m_quad);
    (*p_m_rotation1).m_vec.m_quad = v24;
    float v30 = motionState.m_sweptTransform.m_centerOfMass1.m_quad[3];
    v24[0] = v32;
    __m128 v25 = _mm_shuffle_ps(v24, v24, 0);
    motionState.m_sweptTransform.m_centerOfMass1.m_quad = _mm_add_ps(
        _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v25), motionState.m_sweptTransform.m_centerOfMass0.m_quad),
        _mm_mul_ps(motionState.m_sweptTransform.m_centerOfMass1.m_quad, v25)
    );
    motionState.m_sweptTransform.m_centerOfMass1.m_quad[3] = v30 / v11;
    motionState.m_deltaAngle.m_quad =
        _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v32), _mm_set_ss(v32), 0), motionState.m_deltaAngle.m_quad);
    motionState.m_transform.m_rotation.set(motionState.m_sweptTransform.m_rotation1);
    motionState.m_transform.m_translation.m_quad = _mm_sub_ps(
        motionState.m_sweptTransform.m_centerOfMass1.m_quad,
        _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(
                        motionState.m_sweptTransform.m_centerOfMassLocal.m_quad,
                        motionState.m_sweptTransform.m_centerOfMassLocal.m_quad, 85
                    ),
                    motionState.m_transform.m_rotation.m_col1.m_quad
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(
                        motionState.m_sweptTransform.m_centerOfMassLocal.m_quad,
                        motionState.m_sweptTransform.m_centerOfMassLocal.m_quad, 0
                    ),
                    motionState.m_transform.m_rotation.m_col0.m_quad
                )
            ),
            _mm_mul_ps(
                _mm_shuffle_ps(
                    motionState.m_sweptTransform.m_centerOfMassLocal.m_quad,
                    motionState.m_sweptTransform.m_centerOfMassLocal.m_quad, 170
                ),
                motionState.m_transform.m_rotation.m_col2.m_quad
            )
        )
    );
}

void hkSweptTransformUtil::freezeMotionState(float time, hkMotionState& motionState) {
    hkQuaternion v3;
    hkQuaternion v4;
    hkQuaternion v6;
    hkQuaternion v13;
    __m128 v19;
    hkVector4 v23;
    hkVector4 v25;
    uint32_t v27;
    uint32_t v28;
    uint32_t v29;
    float v30 = motionState.m_sweptTransform.m_centerOfMass0.m_quad[3];
    double v2 = time;
    if (!(v30 < static_cast<double>(time))) {
        v2 = v30;
    }
    v3.m_vec.m_quad = motionState.m_sweptTransform.m_rotation1.m_vec.m_quad;
    v4.m_vec.m_quad = motionState.m_sweptTransform.m_rotation0.m_vec.m_quad;
    hkQuaternion* p_m_rotation1 = &motionState.m_sweptTransform.m_rotation1;
    v6.m_vec.m_quad = _mm_add_ps(v3.m_vec.m_quad, v4.m_vec.m_quad);
    __m128 v7 = _mm_mul_ps(v6.m_vec.m_quad, v6.m_vec.m_quad);
    float v31 = (v2 - motionState.m_sweptTransform.m_centerOfMass0.m_quad[3]) *
        motionState.m_sweptTransform.m_centerOfMass1.m_quad[3];
    __m128 v8 = _mm_add_ps(_mm_shuffle_ps(v7, v7, 78), v7);
    double v9 = (_mm_shuffle_ps(v8, v8, 177)[0] + v8[0]);
    motionState.m_sweptTransform.m_rotation1.m_vec.m_quad = v6.m_vec.m_quad;
    float v26 = 0.75 - 0.125 * v9;
    *reinterpret_cast<float*>(&v27) = (1.5 - v9 * 0.5 * v26 * v26) * v26;
    __m128 v10 = _mm_mul_ps(
        _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v27)), _mm_set_ss(std::bit_cast<float>(v27)), 0), v6.m_vec.m_quad
    );
    motionState.m_sweptTransform.m_rotation1.m_vec.m_quad = v10;
    double v11 = v31 + v31;
    if (!(v31 < 0.5f)) {
        *reinterpret_cast<float*>(&v29) = v11 - 1.0;
        __m128 v14 = _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v29)), _mm_set_ss(std::bit_cast<float>(v29)), 0);
        v13.m_vec.m_quad =
            _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v14), v10), _mm_mul_ps(v14, v3.m_vec.m_quad));
    }
    else {
        *reinterpret_cast<float*>(&v28) = v11;
        __m128 v12 = _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v28)), _mm_set_ss(std::bit_cast<float>(v28)), 0);
        v13.m_vec.m_quad =
            _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v12), v4.m_vec.m_quad), _mm_mul_ps(v10, v12));
    }
    (*p_m_rotation1).m_vec.m_quad = v13.m_vec.m_quad;
    __m128 v15 = _mm_mul_ps(p_m_rotation1->m_vec.m_quad, p_m_rotation1->m_vec.m_quad);
    __m128 v16 = _mm_add_ps(_mm_shuffle_ps(v15, v15, 78), v15);
    __m128 v17 = _mm_add_ps(_mm_shuffle_ps(v16, v16, 177), v16);
    float v18 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v17[0])));
    v19[0] = std::bit_cast<float>(static_cast<int32_t>(0.0f));
    __m128 v20 = _mm_cmpeq_ss(v17, v19);
    v19[0] = 3.0f - ((v17[0] * v18) * v18);
    __m128 v21 = _mm_setzero_ps();
    v21[0] = (0.5f * v18) * v19[0];
    __m128 v22 = _mm_andnot_ps(v20, v21);
    p_m_rotation1->m_vec.m_quad = _mm_mul_ps(_mm_shuffle_ps(v22, v22, 0), p_m_rotation1->m_vec.m_quad);
    v23.m_quad = motionState.m_sweptTransform.m_centerOfMass0.m_quad;
    motionState.m_sweptTransform.m_rotation0.m_vec.m_quad = p_m_rotation1->m_vec.m_quad;
    __m128 v24 = _mm_shuffle_ps(_mm_set_ss(v31), _mm_set_ss(v31), 0);
    v25.m_quad = _mm_add_ps(
        _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v24), v23.m_quad),
        _mm_mul_ps(v24, motionState.m_sweptTransform.m_centerOfMass1.m_quad)
    );
    motionState.m_sweptTransform.m_centerOfMass0.m_quad = v25.m_quad;
    motionState.m_sweptTransform.m_centerOfMass0.m_quad[3] = v2;
    motionState.m_sweptTransform.m_centerOfMass1.m_quad = v25.m_quad;
    motionState.m_sweptTransform.m_centerOfMass1.m_quad[3] = 0.0f;
    motionState.m_transform.m_rotation.set(motionState.m_sweptTransform.m_rotation1);
    motionState.m_transform.m_translation.m_quad = _mm_sub_ps(
        motionState.m_sweptTransform.m_centerOfMass1.m_quad,
        _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(
                        motionState.m_sweptTransform.m_centerOfMassLocal.m_quad,
                        motionState.m_sweptTransform.m_centerOfMassLocal.m_quad, 85
                    ),
                    motionState.m_transform.m_rotation.m_col1.m_quad
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(
                        motionState.m_sweptTransform.m_centerOfMassLocal.m_quad,
                        motionState.m_sweptTransform.m_centerOfMassLocal.m_quad, 0
                    ),
                    motionState.m_transform.m_rotation.m_col0.m_quad
                )
            ),
            _mm_mul_ps(
                _mm_shuffle_ps(
                    motionState.m_sweptTransform.m_centerOfMassLocal.m_quad,
                    motionState.m_sweptTransform.m_centerOfMassLocal.m_quad, 170
                ),
                motionState.m_transform.m_rotation.m_col2.m_quad
            )
        )
    );
}

// The swept transform's end becomes its start, pos1 and rot1 its new end, with the step's start time and inverse
// duration in the centres' w; m_deltaAngle is the angle of the turn from the old rotation to rot1 times its axis (zero
// when the axis is not valid), the angle in w; the transform follows the new end.
void hkSweptTransformUtil::keyframeMotionState(
    const hkStepInfo& stepInfo,
    const hkVector4& pos1,
    const hkQuaternion& rot1,
    hkMotionState& motionState
) {
    hkSweptTransform& st = motionState.m_sweptTransform;
    st.m_centerOfMass0.m_quad = st.m_centerOfMass1.m_quad;
    st.m_rotation0.m_vec.m_quad = st.m_rotation1.m_vec.m_quad;
    st.m_centerOfMass1.m_quad = pos1.m_quad;
    st.m_rotation1.m_vec.m_quad = rot1.m_vec.m_quad;
    hkQuaternion diff;
    diff.setMulInverse(rot1, st.m_rotation0);
    const float angle = diff.getAngle();
    bool validAxis;
    hkVector4 axis;
    if (*diff.m_vec.hasValidAxis(&validAxis)) {
        diff.getAxis(axis);
    }
    else {
        axis.m_quad = _mm_setzero_ps();
    }
    motionState.m_deltaAngle.m_quad = _mm_mul_ps(_mm_set1_ps(angle), axis.m_quad);
    motionState.m_deltaAngle.m_quad[3] = angle;
    st.m_centerOfMass0.m_quad[3] = stepInfo.m_startTime;
    st.m_centerOfMass1.m_quad[3] = stepInfo.m_invDeltaTime;
    motionState.m_transform.m_rotation.set(st.m_rotation1);
    const hkQuadReal local = st.m_centerOfMassLocal.m_quad;
    const hkRotation& rotation = motionState.m_transform.m_rotation;
    motionState.m_transform.m_translation.m_quad = _mm_sub_ps(
        st.m_centerOfMass1.m_quad,
        _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(local, local, 0x55), rotation.m_col1.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(local, local, 0x00), rotation.m_col0.m_quad)
            ),
            _mm_mul_ps(_mm_shuffle_ps(local, local, 0xAA), rotation.m_col2.m_quad)
        )
    );
}

void hkSweptTransformUtil::lerp2Ha(
    const hkSweptTransform& sweptTrans,
    float t,
    float tAddOn,
    hkTransform& transformOut
) {
    hkQuaternion v4;
    hkQuaternion v5;
    __m128 v12;
    __m128 v13;
    __m128 v20;
    uint32_t v26;
    uint32_t v27;
    uint32_t v28;
    hkQuaternion arQuaternion;
    v4.m_vec.m_quad = sweptTrans.m_rotation1.m_vec.m_quad;
    v5.m_vec.m_quad = sweptTrans.m_rotation0.m_vec.m_quad;
    float v29 = t - sweptTrans.m_centerOfMass0.m_quad[3];
    __m128 v6 = _mm_add_ps(v4.m_vec.m_quad, v5.m_vec.m_quad);
    __m128 v7 = _mm_mul_ps(v6, v6);
    float v30 = v29 + tAddOn;
    __m128 v8 = _mm_add_ps(_mm_shuffle_ps(v7, v7, 78), v7);
    v7[0] = _mm_shuffle_ps(v8, v8, 177)[0] + v8[0];
    float v31 = v30 * sweptTrans.m_centerOfMass1.m_quad[3];
    arQuaternion.m_vec.m_quad = v6;
    float v25 = 0.75 - 0.125 * v7[0];
    *reinterpret_cast<float*>(&v26) = (1.5 - v7[0] * 0.5 * v25 * v25) * v25;
    __m128 v9 =
        _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v26)), _mm_set_ss(std::bit_cast<float>(v26)), 0), v6);
    double v10 = v31 + v31;
    if (!(v31 < 0.5f)) {
        *reinterpret_cast<float*>(&v28) = v10 - 1.0;
        __m128 v14 = _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v28)), _mm_set_ss(std::bit_cast<float>(v28)), 0);
        v12 = _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v14), v9);
        v13 = _mm_mul_ps(v14, v4.m_vec.m_quad);
    }
    else {
        *reinterpret_cast<float*>(&v27) = v10;
        __m128 v11 = _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v27)), _mm_set_ss(std::bit_cast<float>(v27)), 0);
        v12 = _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v11), v5.m_vec.m_quad);
        v13 = _mm_mul_ps(v11, v9);
    }
    __m128 v15 = _mm_add_ps(v12, v13);
    __m128 v16 = _mm_mul_ps(v15, v15);
    __m128 v17 = _mm_add_ps(_mm_shuffle_ps(v16, v16, 78), v16);
    __m128 v18 = _mm_add_ps(_mm_shuffle_ps(v17, v17, 177), v17);
    float v19 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v18[0])));
    v20[0] = std::bit_cast<float>(static_cast<int32_t>(0.0f));
    __m128 v21 = _mm_cmpeq_ss(v18, v20);
    v20[0] = 3.0f - ((v18[0] * v19) * v19);
    __m128 v22 = _mm_setzero_ps();
    v22[0] = (0.5f * v19) * v20[0];
    __m128 v23 = _mm_andnot_ps(v21, v22);
    arQuaternion.m_vec.m_quad = _mm_mul_ps(_mm_shuffle_ps(v23, v23, 0), v15);
    transformOut.m_rotation.set(arQuaternion);
    __m128 v24 = _mm_shuffle_ps(_mm_set_ss(v31), _mm_set_ss(v31), 0);
    transformOut.m_translation.m_quad = _mm_add_ps(
        _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v24), sweptTrans.m_centerOfMass0.m_quad),
        _mm_mul_ps(sweptTrans.m_centerOfMass1.m_quad, v24)
    );
    transformOut.m_translation.m_quad = _mm_sub_ps(
        transformOut.m_translation.m_quad,
        _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(sweptTrans.m_centerOfMassLocal.m_quad, sweptTrans.m_centerOfMassLocal.m_quad, 85),
                    transformOut.m_rotation.m_col1.m_quad
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(sweptTrans.m_centerOfMassLocal.m_quad, sweptTrans.m_centerOfMassLocal.m_quad, 0),
                    transformOut.m_rotation.m_col0.m_quad
                )
            ),
            _mm_mul_ps(
                _mm_shuffle_ps(sweptTrans.m_centerOfMassLocal.m_quad, sweptTrans.m_centerOfMassLocal.m_quad, 170),
                transformOut.m_rotation.m_col2.m_quad
            )
        )
    );
}

void hkSweptTransformUtil::lerp2Rel(const hkSweptTransform& sweptTrans, float r, hkTransform& transformOut) {
    hkQuaternion v3;
    hkQuaternion v4;
    __m128 v11;
    __m128 v12;
    __m128 v19;
    uint32_t v25;
    uint32_t v26;
    uint32_t v27;
    hkQuaternion arQuaternion;
    v3.m_vec.m_quad = sweptTrans.m_rotation1.m_vec.m_quad;
    v4.m_vec.m_quad = sweptTrans.m_rotation0.m_vec.m_quad;
    __m128 v5 = _mm_add_ps(v3.m_vec.m_quad, v4.m_vec.m_quad);
    __m128 v6 = _mm_mul_ps(v5, v5);
    __m128 v7 = _mm_add_ps(_mm_shuffle_ps(v6, v6, 78), v6);
    v6[0] = _mm_shuffle_ps(v7, v7, 177)[0] + v7[0];
    arQuaternion.m_vec.m_quad = v5;
    float v24 = 0.75 - 0.125 * v6[0];
    *reinterpret_cast<float*>(&v25) = (1.5 - v6[0] * 0.5 * v24 * v24) * v24;
    __m128 v8 =
        _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v25)), _mm_set_ss(std::bit_cast<float>(v25)), 0), v5);
    double v9 = r + r;
    if (!(r < 0.5f)) {
        *reinterpret_cast<float*>(&v27) = v9 - 1.0;
        __m128 v13 = _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v27)), _mm_set_ss(std::bit_cast<float>(v27)), 0);
        v11 = _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v13), v8);
        v12 = _mm_mul_ps(v13, v3.m_vec.m_quad);
    }
    else {
        *reinterpret_cast<float*>(&v26) = v9;
        __m128 v10 = _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v26)), _mm_set_ss(std::bit_cast<float>(v26)), 0);
        v11 = _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v10), v4.m_vec.m_quad);
        v12 = _mm_mul_ps(v10, v8);
    }
    __m128 v14 = _mm_add_ps(v11, v12);
    __m128 v15 = _mm_mul_ps(v14, v14);
    __m128 v16 = _mm_add_ps(_mm_shuffle_ps(v15, v15, 78), v15);
    __m128 v17 = _mm_add_ps(_mm_shuffle_ps(v16, v16, 177), v16);
    float v18 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v17[0])));
    v19[0] = std::bit_cast<float>(static_cast<int32_t>(0.0f));
    __m128 v20 = _mm_cmpeq_ss(v17, v19);
    v19[0] = 3.0f - ((v17[0] * v18) * v18);
    __m128 v21 = _mm_setzero_ps();
    v21[0] = (0.5f * v18) * v19[0];
    __m128 v22 = _mm_andnot_ps(v20, v21);
    arQuaternion.m_vec.m_quad = _mm_mul_ps(_mm_shuffle_ps(v22, v22, 0), v14);
    transformOut.m_rotation.set(arQuaternion);
    __m128 v23 = _mm_shuffle_ps(_mm_set_ss(r), _mm_set_ss(r), 0);
    transformOut.m_translation.m_quad = _mm_add_ps(
        _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v23), sweptTrans.m_centerOfMass0.m_quad),
        _mm_mul_ps(sweptTrans.m_centerOfMass1.m_quad, v23)
    );
    transformOut.m_translation.m_quad = _mm_sub_ps(
        transformOut.m_translation.m_quad,
        _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(sweptTrans.m_centerOfMassLocal.m_quad, sweptTrans.m_centerOfMassLocal.m_quad, 85),
                    transformOut.m_rotation.m_col1.m_quad
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(sweptTrans.m_centerOfMassLocal.m_quad, sweptTrans.m_centerOfMassLocal.m_quad, 0),
                    transformOut.m_rotation.m_col0.m_quad
                )
            ),
            _mm_mul_ps(
                _mm_shuffle_ps(sweptTrans.m_centerOfMassLocal.m_quad, sweptTrans.m_centerOfMassLocal.m_quad, 170),
                transformOut.m_rotation.m_col2.m_quad
            )
        )
    );
}
