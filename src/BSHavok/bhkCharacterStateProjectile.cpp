#include "BSHavok/bhkCharacterStateProjectile.h"

#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCharacterProxy.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/Util/hkpCharacterMovementUtil.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpCharacterStateManager.h"

#include <bit>
#include <xmmintrin.h>

bhkCharacterStateProjectile::bhkCharacterStateProjectile() {}

hkpCharacterStateType bhkCharacterStateProjectile::getType() const {
    return HK_CHARACTER_USER_STATE_1;
}

void bhkCharacterStateProjectile::Update(bhkCharacterController& arInput) {
    hkpCharacterMovementUtil::hkpMovementUtilInput muInput;
    hkVector4* p_kOutVelocity = &arInput.OutVelocity;
    float v17 = arInput.OutVelocity.m_quad[0];
    muInput.m_forward = arInput.ForwardVec;
    muInput.m_up = arInput.UpVec;
    muInput.m_surfaceNormal = muInput.m_up;
    hkVector4* v16 = &arInput.OutVelocity;
    if (!(v17 < -1e+08f)) {
        hkVector4 v6;
        hkVector4 position;
        double fAcrobatics = arInput.fAcrobatics;
        muInput.m_currentVelocity.m_quad = p_kOutVelocity->m_quad;
        muInput.m_gain = fAcrobatics;
        __m128 v5 = _mm_set_ss(
            std::bit_cast<float>(
                std::bit_cast<uint32_t>(_mm_shuffle_ps(arInput.Direction.m_quad, arInput.Direction.m_quad, 170)[0])
            )
        );
        v6.m_quad = arInput.Direction.m_quad;
        uint32_t v21 = std::bit_cast<uint32_t>(arInput.Direction.m_quad[0]);
        muInput.m_maxVelocityDelta = 100000.0f;
        muInput.m_desiredVelocity.m_quad = _mm_unpacklo_ps(
            _mm_unpacklo_ps(_mm_shuffle_ps(v6.m_quad, v6.m_quad, 85), v5),
            _mm_unpacklo_ps(
                _mm_set_ss(std::bit_cast<float>(v21)), _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
            )
        );
        muInput.m_surfaceVelocity = arInput.m_surfaceVelocity;
        hkpCharacterMovementUtil::calculateMovement(muInput, *v16);
        float fGravity = arInput.fGravity;
        if (fGravity > 0.0f) {
            __m128 v7 = _mm_mul_ps(arInput.UpVec.m_quad, p_kOutVelocity->m_quad);
            float v8 = _mm_shuffle_ps(v7, v7, 85)[0] + v7[0];
            float v9 = _mm_shuffle_ps(v7, v7, 170)[0];
            __m128 m_quad = hkQuadReal0000;
            m_quad[0] = 0.0f - static_cast<float>(v9 + v8);
            p_kOutVelocity->m_quad =
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(m_quad, m_quad, 0), arInput.UpVec.m_quad), p_kOutVelocity->m_quad);
            __m128 v11 = _mm_mul_ps(arInput.UpVec.m_quad, muInput.m_currentVelocity.m_quad);
            float v12 = _mm_shuffle_ps(v11, v11, 85)[0] + v11[0];
            __m128 v13 = _mm_shuffle_ps(v11, v11, 170);
            v13[0] = v13[0] + v12;
            p_kOutVelocity->m_quad =
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v13, v13, 0), arInput.UpVec.m_quad), p_kOutVelocity->m_quad);
        }
        arInput.GetPosition_hk(position);
        if (arInput.fWaterHeight < static_cast<double>(_mm_shuffle_ps(position.m_quad, position.m_quad, 170)[0])) {
            arInput.GethkWorld();
            const hkpWorld* v14 = arInput.GethkWorld();
            __m128 v24 = v14->m_gravity.m_quad;
            arInput.GethkWorld();
            p_kOutVelocity->m_quad = _mm_add_ps(
                _mm_mul_ps(
                    _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(fGravity), _mm_set_ss(fGravity), 0), v24),
                    _mm_shuffle_ps(
                        _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(arInput.StepInfo.m_deltaTime))),
                        _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(arInput.StepInfo.m_deltaTime))), 0
                    )
                ),
                p_kOutVelocity->m_quad
            );
        }
    }
    else {
        hkVector4 v3;
        p_kOutVelocity->m_quad[0] = 0.0f;
        muInput.m_currentVelocity.m_quad = p_kOutVelocity->m_quad;
        muInput.m_gain = 1.0f;
        uint32_t v20 =
            std::bit_cast<uint32_t>(_mm_shuffle_ps(arInput.Direction.m_quad, arInput.Direction.m_quad, 170)[0]);
        v3.m_quad = arInput.Direction.m_quad;
        uint32_t v18 = std::bit_cast<uint32_t>(arInput.Direction.m_quad[0]);
        muInput.m_maxVelocityDelta = 100000.0f;
        muInput.m_desiredVelocity.m_quad = _mm_unpacklo_ps(
            _mm_unpacklo_ps(_mm_shuffle_ps(v3.m_quad, v3.m_quad, 85), _mm_set_ss(std::bit_cast<float>(v20))),
            _mm_unpacklo_ps(
                _mm_set_ss(std::bit_cast<float>(v18)), _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
            )
        );
        muInput.m_surfaceVelocity = arInput.m_surfaceVelocity;
        hkpCharacterMovementUtil::calculateMovement(muInput, *v16);
    }
    if (arInput.eWantState != HK_CHARACTER_MAX_STATE_ID) {
        arInput.UpdateState();
    }
}
