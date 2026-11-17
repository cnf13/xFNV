#include "BSHavok/bhkCharacterStateOnGround.h"

#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCharacterProxy.h"
#include "BSHavok/bhkConvert.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/Util/hkpCharacterMovementUtil.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpCharacterStateManager.h"

#include <xmmintrin.h>

namespace bhkConvert {} // namespace bhkConvert

bhkCharacterStateOnGround::bhkCharacterStateOnGround() : bKillVelocityOnLaunch(true) {}

hkpCharacterStateType bhkCharacterStateOnGround::getType() const {
    return HK_CHARACTER_ON_GROUND;
}

// Walks the character along the ground: without support (real or faked) it falls into the in-air state; a faked support
// ends once the character is more than 64 game units (scaled to Havok) from where it began, or when it wants another
// state. The desired velocity is kDirection's (y, x) with z only when chasing the biped.
void bhkCharacterStateOnGround::Update(bhkCharacterController& arInput) {
    if ((arInput.iFlags & bhkCharacterController::SUPPORT) == 0 && !arInput.bFakeSupport) {
        if (bKillVelocityOnLaunch) {
            arInput.OutVelocity.m_quad[2] = 0.0f;
        }
        alignas(16) NiPoint3 kNiPosition;
        arInput.GetPosition(kNiPosition);
        arInput.fFallStartHeight = kNiPosition.z;
        arInput.eWantState = HK_CHARACTER_IN_AIR;
        arInput.UpdateState();
        hkpCharacterState* pState = arInput.m_stateManager->getState(arInput.m_currentState);
        static_cast<bhkCharacterState*>(pState)->Update(arInput);
        return;
    }
    if (arInput.bFakeSupport) {
        hkVector4 kStart;
        kStart.m_quad = arInput.FakeSupportStart.m_quad;
        hkVector4 kPosition;
        arInput.GetPosition_hk(kPosition);
        float fDistance = kPosition.distanceTo3(kStart);
        float fLimit = bhkConvert::fBS2HkScaleSC * 64.0;
        if (fLimit < fDistance || arInput.eWantState != HK_CHARACTER_MAX_STATE_ID) {
            arInput.bFakeSupport = false;
        }
    }
    hkpCharacterMovementUtil::hkpMovementUtilInput muInput;
    muInput.m_gain = 1.0f;
    muInput.m_forward = arInput.ForwardVec;
    muInput.m_up = arInput.UpVec;
    muInput.m_surfaceNormal = arInput.hkSupportNorm;
    muInput.m_currentVelocity = arInput.OutVelocity;
    __m128 kDirection = arInput.Direction.m_quad;
    if ((arInput.iFlags & bhkCharacterController::CHASEBIP) != 0) {
        float fZ = arInput.Direction.m_quad[2];
        float fX = arInput.Direction.m_quad[0];
        muInput.m_desiredVelocity.m_quad = _mm_unpacklo_ps(
            _mm_unpacklo_ps(_mm_shuffle_ps(kDirection, kDirection, 0x55), _mm_set_ss(fZ)),
            _mm_unpacklo_ps(_mm_set_ss(fX), _mm_set_ss(zero))
        );
    }
    else {
        float fX = arInput.Direction.m_quad[0];
        muInput.m_desiredVelocity.m_quad = _mm_unpacklo_ps(
            _mm_unpacklo_ps(_mm_shuffle_ps(kDirection, kDirection, 0x55), _mm_set_ss(zero)),
            _mm_unpacklo_ps(_mm_set_ss(fX), _mm_set_ss(zero))
        );
    }
    muInput.m_maxVelocityDelta = 500.0f;
    muInput.m_surfaceVelocity = arInput.m_surfaceVelocity;
    hkpCharacterMovementUtil::calculateMovement(muInput, arInput.OutVelocity);
    if ((arInput.iFlags & bhkCharacterController::CHASEBIP) == 0) {
        arInput.OutVelocity.m_quad[2] = muInput.m_currentVelocity.m_quad[2];
        const hkpWorld* pWorld = arInput.GethkWorld();
        float fScale = arInput.fGravity * 0.5 * arInput.fSpeedPct;
        float fDeltaTime = arInput.StepInfo.m_deltaTime;
        arInput.OutVelocity.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_mul_ps(_mm_set1_ps(fScale), pWorld->m_gravity.m_quad), _mm_set1_ps(fDeltaTime)),
            arInput.OutVelocity.m_quad
        );
    }
    arInput.ApplyVelocityMod();
    hkpCharacterStateType eWantState = arInput.eWantState;
    if (eWantState != HK_CHARACTER_MAX_STATE_ID &&
        (eWantState != HK_CHARACTER_JUMPING || (arInput.iFlags & bhkCharacterController::CANJUMP) != 0)) {
        arInput.UpdateState();
    }
}
