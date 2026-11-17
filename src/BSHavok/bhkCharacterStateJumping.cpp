#include "BSHavok/bhkCharacterStateJumping.h"

#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCharacterProxy.h"
#include "Fallout Shared/TES.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxy.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpCharacterStateManager.h"

#include <cmath>
#include <xmmintrin.h>

bhkCharacterStateJumping::bhkCharacterStateJumping() {}

hkpCharacterStateType bhkCharacterStateJumping::getType() const {
	return HK_CHARACTER_JUMPING;
}

// Starts a jump: the launch speed sqrt(2 g h) along the scaled world gravity, added to the proxy's velocity with its
// z dropped, then the in-air state takes over. The exe's x87 copies (fld/fstp dword) of fGravity (0xCD4300) and of
// the position's z (0xCD4468) are plain float copies here.
void bhkCharacterStateJumping::Update(bhkCharacterController& arInput) {
	hkpCharacterStateType eWantState = arInput.eWantState;
	if (eWantState != HK_CHARACTER_MAX_STATE_ID &&
		(eWantState == HK_CHARACTER_ON_GROUND || eWantState == HK_CHARACTER_CLIMBING)) {
		arInput.UpdateState();
		return;
	}
	arInput.iFlags |= bhkCharacterController::JUMPING;
	arInput.GethkWorld();
	hkVector4 kGravity;
	kGravity.m_quad = arInput.GethkWorld()->m_gravity.m_quad;
	arInput.GethkWorld();
	float fGravity = arInput.fGravity;
	float fJumpHeight = std::fabs(arInput.fJumpHeight);
	kGravity.m_quad = _mm_mul_ps(_mm_set1_ps(fGravity), kGravity.m_quad);
	hkVector4 kDirection = kGravity;
	float fLength = kDirection.normalizeWithLength3();
	float fSquare = fJumpHeight * (fLength + fLength);
	float fRoot = std::sqrt(fSquare);
	float fSpeed = -fRoot;
	hkVector4 kJump;
	// EXE-INLINED 0x630AE0
	kJump.m_quad = _mm_mul_ps(_mm_set1_ps(fSpeed), kDirection.m_quad);
	hkpCharacterProxy* phkObject = static_cast<hkpCharacterProxy*>(arInput.phkObject);
	const hkVector4* pLinearVelocity = &hkVector4Zero;
	if (phkObject) {
		arInput.bhkCharacterProxy::GethkWorldRw();
		pLinearVelocity = &phkObject->getLinearVelocity();
		arInput.bhkCharacterProxy::GethkWorldRw();
	}
	hkVector4 kVelocity;
	kVelocity.m_quad = pLinearVelocity->m_quad;
	kVelocity.m_quad[2] = 0.0f;
	arInput.OutVelocity.m_quad = _mm_add_ps(kVelocity.m_quad, kJump.m_quad);
	hkVector4 kPosition;
	arInput.GetPosition_hk(kPosition);
	alignas(16) NiPoint3 kNiPosition;
	HK2NI(kNiPosition, kPosition);
	arInput.fFallStartHeight = kNiPosition.z;
	arInput.eWantState = HK_CHARACTER_IN_AIR;
	arInput.UpdateState();
	arInput.iFlags &= ~static_cast<bhkCharacterController::CHARACTER_FLAGS>(bhkCharacterController::SUPPORT |
		bhkCharacterController::ON_ELEVATOR);
	hkpCharacterState* pState = arInput.m_stateManager->getState(arInput.m_currentState);
	static_cast<bhkCharacterState*>(pState)->Update(arInput);
}
