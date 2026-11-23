#include "BSHavok/hkOrientHingedBodyAction.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkCharacterController.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpUnaryAction.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <xmmintrin.h>

hkOrientHingedBodyAction::hkOrientHingedBodyAction(
    hkpRigidBody* entityA,
    const hkVector4& arHingeAxisLS,
    const hkVector4& arForwardLS,
    float afStrength,
    float afDamping,
    uint32_t userData
)
  : hkpUnaryAction(entityA, userData), HingeAxisLS(arHingeAxisLS), ForwardLS(arForwardLS), fStrength(afStrength),
    fDamping(afDamping), bEnabled(false) {
    TargetDirWS.m_quad = _mm_setzero_ps();
}

void hkOrientHingedBodyAction::applyAction(const hkStepInfo& stepInfo) {
    uint32_t v38;
    uint32_t v41;
    uint32_t v42;
    if (bEnabled) {
        hkVector4 v3;
        hkQuaternion v5;
        hkVector4 v11;
        hkQuaternion v13;
        hkVector4 v19;
        int32_t v33;
        int32_t v36;
        hkVector4 v44;
        v3.m_quad = ForwardLS.m_quad;
        hkpRigidBody* pbody = static_cast<hkpRigidBody*>(m_entity);
        v5.m_vec.m_quad = pbody->m_motion.m_motionState.m_sweptTransform.m_rotation1.m_vec.m_quad;
        __m128 v6 = _mm_mul_ps(v3.m_quad, v5.m_vec.m_quad);
        __m128 v7 = _mm_shuffle_ps(v6, v6, 170);
        XFNV_M128_F32(v7, 0) = XFNV_M128_F32(v7, 0) +
            (XFNV_M128_F32(_mm_shuffle_ps(v6, v6, 85), 0) + XFNV_M128_F32(v6, 0));
        __m128 v8 = _mm_shuffle_ps(v5.m_vec.m_quad, v5.m_vec.m_quad, 255);
        __m128 v9 = _mm_add_ps(
            _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v8, v8), hkQuadRealMinusHalf), v3.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(v7, v7, 0), v5.m_vec.m_quad)
        );
        __m128 v10 = _mm_mul_ps(
            _mm_sub_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(v3.m_quad, v3.m_quad, 210),
                    _mm_shuffle_ps(v5.m_vec.m_quad, v5.m_vec.m_quad, 201)
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(v3.m_quad, v3.m_quad, 201),
                    _mm_shuffle_ps(v5.m_vec.m_quad, v5.m_vec.m_quad, 210)
                )
            ),
            v8
        );
        v11.m_quad = HingeAxisLS.m_quad;
        __m128 v12 = _mm_add_ps(v10, v9);
        v44.m_quad = _mm_add_ps(v12, v12);
        v13.m_vec.m_quad = pbody->m_motion.m_motionState.m_sweptTransform.m_rotation1.m_vec.m_quad;
        __m128 v14 = _mm_mul_ps(v11.m_quad, v13.m_vec.m_quad);
        __m128 v15 = _mm_shuffle_ps(v14, v14, 170);
        XFNV_M128_F32(v15, 0) = XFNV_M128_F32(v15, 0) +
            (XFNV_M128_F32(_mm_shuffle_ps(v14, v14, 85), 0) + XFNV_M128_F32(v14, 0));
        __m128 v16 = _mm_sub_ps(
            _mm_mul_ps(
                _mm_shuffle_ps(v11.m_quad, v11.m_quad, 210),
                _mm_shuffle_ps(v13.m_vec.m_quad, v13.m_vec.m_quad, 201)
            ),
            _mm_mul_ps(
                _mm_shuffle_ps(v11.m_quad, v11.m_quad, 201),
                _mm_shuffle_ps(v13.m_vec.m_quad, v13.m_vec.m_quad, 210)
            )
        );
        __m128 v17 = _mm_shuffle_ps(v13.m_vec.m_quad, v13.m_vec.m_quad, 255);
        __m128 v18 = _mm_add_ps(
            _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v17, v17), hkQuadRealMinusHalf), v11.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(v15, v15, 0), v13.m_vec.m_quad)
        );
        v19.m_quad = TargetDirWS.m_quad;
        __m128 v20 = _mm_add_ps(_mm_mul_ps(v16, v17), v18);
        __m128 v21 = _mm_add_ps(v20, v20);
        __m128 v22 = _mm_mul_ps(v21, v19.m_quad);
        *reinterpret_cast<float*>(&v38) = -(
            XFNV_M128_F32(_mm_shuffle_ps(v22, v22, 170), 0) +
            (XFNV_M128_F32(_mm_shuffle_ps(v22, v22, 85), 0) + XFNV_M128_F32(v22, 0))
        );
        __m128 v23 = _mm_add_ps(
            _mm_mul_ps(
                _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v38)), _mm_set_ss(std::bit_cast<float>(v38)), 0),
                v21
            ),
            v19.m_quad
        );
        __m128 v24 = _mm_mul_ps(v23, v23);
        XFNV_M128_F32(v19.m_quad, 0) = XFNV_M128_F32(_mm_shuffle_ps(v24, v24, 85), 0) + XFNV_M128_F32(v24, 0);
        __m128 v25 = _mm_shuffle_ps(v24, v24, 170);
        XFNV_M128_F32(v25, 0) = XFNV_M128_F32(v25, 0) + XFNV_M128_F32(v19.m_quad, 0);
        XFNV_M128_F32(v17, 0) = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(v25, 0))));
        XFNV_M128_F32(v15, 0) = std::bit_cast<float>(static_cast<int32_t>(0.0f));
        __m128 v26 = _mm_cmpeq_ss(v25, v15);
        XFNV_M128_F32(v15, 0) = 3.0f - ((XFNV_M128_F32(v25, 0) * XFNV_M128_F32(v17, 0)) * XFNV_M128_F32(v17, 0));
        __m128 v27 = _mm_setzero_ps();
        XFNV_M128_F32(v27, 0) = (0.5f * XFNV_M128_F32(v17, 0)) * XFNV_M128_F32(v15, 0);
        __m128 v28 = _mm_andnot_ps(v26, v27);
        __m128 v43 = _mm_mul_ps(_mm_shuffle_ps(v28, v28, 0), v23);
        __m128 v29 = _mm_mul_ps(v43, v44.m_quad);
        float v39 = hkMath::acos(
            XFNV_M128_F32(_mm_shuffle_ps(v29, v29, 170), 0) +
            (XFNV_M128_F32(_mm_shuffle_ps(v29, v29, 85), 0) + XFNV_M128_F32(v29, 0))
        );
        __m128 m_quad = v44.m_quad;
        double v31 = v39;
        fAngleDiff = v39;
        v44.m_quad = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(v43, v43, 210), _mm_shuffle_ps(m_quad, m_quad, 201)),
            _mm_mul_ps(_mm_shuffle_ps(v43, v43, 201), _mm_shuffle_ps(m_quad, m_quad, 210))
        );
        __m128 v32 = _mm_and_ps(v44.m_quad, hkVector4::absMask);
        if (!(XFNV_M128_F32(v32, 1) > static_cast<double>(XFNV_M128_F32(v32, 0)))) {
            if (!(XFNV_M128_F32(v32, 2) > static_cast<double>(XFNV_M128_F32(v32, 0)))) {
                v33 = 0;
            }
            else {
                v33 = 2;
            }
        }
        else {
            if (!(XFNV_M128_F32(v32, 2) > static_cast<double>(XFNV_M128_F32(v32, 1)))) {
                v33 = 1;
            }
            else {
                v33 = 2;
            }
        }
        float v40 = XFNV_M128_F32(v44.m_quad, v33);
        __m128 v35 = _mm_and_ps(v21, hkVector4::absMask);
        if (!(XFNV_M128_F32(v35, 1) > static_cast<double>(XFNV_M128_F32(v35, 0)))) {
            if (!(XFNV_M128_F32(v35, 2) > static_cast<double>(XFNV_M128_F32(v35, 0)))) {
                v36 = 0;
            }
            else {
                v36 = 2;
            }
        }
        else {
            if (!(XFNV_M128_F32(v35, 2) > static_cast<double>(XFNV_M128_F32(v35, 1)))) {
                v36 = 1;
            }
            else {
                v36 = 2;
            }
        }
        if (v40 < 0.0f != XFNV_M128_F32(v21, v36) < 0.0f) {
            fAngleDiff = -v31;
        }
        *reinterpret_cast<float*>(&v41) = fDamping * stepInfo.m_invDeltaTime;
        __m128 v37 = _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v41)), _mm_set_ss(std::bit_cast<float>(v41)), 0),
            pbody->m_motion.m_angularVelocity.m_quad
        );
        *reinterpret_cast<float*>(&v42) = fStrength * fAngleDiff * stepInfo.m_invDeltaTime;
        v44.m_quad = _mm_sub_ps(
            _mm_mul_ps(
                _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v42)), _mm_set_ss(std::bit_cast<float>(v42)), 0),
                v21
            ),
            v37
        );
        pbody->activate();
        pbody->m_motion.applyAngularImpulse(v44);
    }
}

hkpAction* hkOrientHingedBodyAction::clone(
    const hkArray<hkpEntity*, hkContainerHeapAllocator>& newEntities,
    const hkArray<hkpPhantom*, hkContainerHeapAllocator>& newPhantoms
) const {
    hkOrientHingedBodyAction* pclonedAction;
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (newEntities.m_size != 2 || newPhantoms.m_size) {
        pclonedAction = nullptr;
    }
    else {
        pclonedAction = new hkOrientHingedBodyAction(
            static_cast<hkpRigidBody*>(*newEntities.m_data),
            HingeAxisLS,
            ForwardLS,
            fStrength,
            fDamping,
            m_userData
        );
        pclonedAction->TargetDirWS = TargetDirWS;
    }
    return pclonedAction;
}

void hkOrientHingedBodyAction::SetEnabled(bool abEnable) {
    bEnabled = abEnable;
}

void hkOrientHingedBodyAction::setTargetDir(const hkVector4& arDir) {
    TargetDirWS = arDir;
}
