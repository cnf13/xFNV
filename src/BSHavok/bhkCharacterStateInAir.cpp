#include "BSHavok/bhkCharacterStateInAir.h"

#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCharacterProxy.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/Util/hkpCharacterMovementUtil.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpCharacterStateManager.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <xmmintrin.h>

bhkCharacterStateInAir::bhkCharacterStateInAir() : fMaxVelocityDelta(2000.0f) {}

hkpCharacterStateType bhkCharacterStateInAir::getType() const {
    return HK_CHARACTER_IN_AIR;
}

void bhkCharacterStateInAir::Update(bhkCharacterController& arInput) {
    hkVector4 startFakePos;
    if ((arInput.iFlags & bhkCharacterController::SUPPORT) != 0) {
        arInput.eWantState = HK_CHARACTER_ON_GROUND;
        arInput.iFlags |= bhkCharacterController::HITDAMAGE;
    }
    if (arInput.eWantState == HK_CHARACTER_MAX_STATE_ID &&
        arInput.fFallTime > 1.0f &&
        arInput.GetFallDistance() < 0.10000000149011612) {
        arInput.eWantState = HK_CHARACTER_ON_GROUND;
        arInput.bFakeSupport = true;
        arInput.GetPosition_hk(startFakePos);
        arInput.FakeSupportStart.m_quad = startFakePos.m_quad;
    }
    hkpCharacterStateType eWantState = arInput.eWantState;
    if (eWantState != HK_CHARACTER_MAX_STATE_ID) {
        if (eWantState != HK_CHARACTER_JUMPING) {
            arInput.fFallTime = 0.0f;
            arInput.UpdateState();
            return;
        }
        arInput.eWantState = HK_CHARACTER_MAX_STATE_ID;
    }
    if (arInput.fAcrobatics > 0.0f) {
        __m128 v2 = _mm_undefined_ps();
        __m128 v3 = _mm_undefined_ps();
        double v6;
        hkVector4 v7;
        hkVector4 v9;
        hkpCharacterMovementUtil::hkpMovementUtilInput muInput;
        if ((arInput.iFlags & (bhkCharacterController::CHASEBIP | bhkCharacterController::FOLLOWRAGDOLL)) != 0) {
            v6 = 1.0;
        }
        else {
            v6 = arInput.fAcrobatics * bhkCharacterController::fJumpMoveMult + bhkCharacterController::fJumpMoveBase;
        }
        v7.m_quad = arInput.ForwardVec.m_quad;
        muInput.m_gain = v6;
        double fMaxVelocityDelta = this->fMaxVelocityDelta;
        muInput.m_forward.m_quad = v7.m_quad;
        v9.m_quad = arInput.UpVec.m_quad;
        muInput.m_maxVelocityDelta = fMaxVelocityDelta;
        muInput.m_up.m_quad = v9.m_quad;
        muInput.m_surfaceNormal.m_quad = v9.m_quad;
        muInput.m_surfaceVelocity = arInput.m_surfaceVelocity;
        muInput.m_currentVelocity = arInput.OutVelocity;
        XFNV_M128_F32(v2, 0) = XFNV_M128_F32(_mm_shuffle_ps(arInput.Direction.m_quad, arInput.Direction.m_quad, 85), 0);
        XFNV_M128_F32(v3, 0) = 0.0f;
        muInput.m_desiredVelocity.m_quad = _mm_unpacklo_ps(
            _mm_unpacklo_ps(v2, v3),
            _mm_unpacklo_ps(
                _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(arInput.Direction.m_quad, 0)))),
                _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
            )
        );
        hkpCharacterMovementUtil::calculateMovement(muInput, arInput.OutVelocity);
        __m128 v10 = _mm_mul_ps(arInput.UpVec.m_quad, arInput.OutVelocity.m_quad);
        float v11 = XFNV_M128_F32(_mm_shuffle_ps(v10, v10, 85), 0) + XFNV_M128_F32(v10, 0);
        float v12 = XFNV_M128_F32(_mm_shuffle_ps(v10, v10, 170), 0);
        __m128 m_quad = hkQuadReal0000;
        XFNV_M128_F32(m_quad, 0) = 0.0f - (v12 + v11);
        arInput.OutVelocity.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(m_quad, m_quad, 0), arInput.UpVec.m_quad),
            arInput.OutVelocity.m_quad
        );
        __m128 v14 = _mm_mul_ps(arInput.UpVec.m_quad, muInput.m_currentVelocity.m_quad);
        float v15 = XFNV_M128_F32(_mm_shuffle_ps(v14, v14, 85), 0) + XFNV_M128_F32(v14, 0);
        __m128 v16 = _mm_shuffle_ps(v14, v14, 170);
        XFNV_M128_F32(v16, 0) = XFNV_M128_F32(v16, 0) + v15;
        arInput.OutVelocity.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(v16, v16, 0), arInput.UpVec.m_quad),
            arInput.OutVelocity.m_quad
        );
    }
    arInput.GethkWorld();
    const hkpWorld* v17 = arInput.GethkWorld();
    startFakePos.m_quad = v17->m_gravity.m_quad;
    arInput.GethkWorld();
    arInput.OutVelocity.m_quad = _mm_add_ps(
        _mm_mul_ps(
            _mm_mul_ps(
                _mm_shuffle_ps(
                    _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(arInput.fGravity))),
                    _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(arInput.fGravity))),
                    0
                ),
                startFakePos.m_quad
            ),
            _mm_shuffle_ps(
                _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(arInput.StepInfo.m_deltaTime))),
                _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(arInput.StepInfo.m_deltaTime))),
                0
            )
        ),
        arInput.OutVelocity.m_quad
    );
    arInput.UpdateFalling();
}
