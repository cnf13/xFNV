#include "BSHavok/bhkCharacterStateSwimming.h"

#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCharacterProxy.h"
#include "BSHavok/bhkConvert.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxy.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/Util/hkpCharacterMovementUtil.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpCharacterStateManager.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

// Not named in the PDB or IDA; name chosen here. The cosine of half a degree (pi/360), as a float; the exe never
// reads it. Placed here because its initialiser (0xFBE640) comes right before this file's fBS2HkScaleSC copy
// (0xFBE660) in the exe's initialiser table, where each file's initialisers run together.
[[maybe_unused]] static const float fCosHalfDegree = static_cast<float>(std::cos(0.008726646192371845));

namespace bhkConvert {} // namespace bhkConvert

bhkCharacterStateSwimming::bhkCharacterStateSwimming() {}

// The game's swimming state is Havok's first user state.
hkpCharacterStateType bhkCharacterStateSwimming::getType() const {
	return HK_CHARACTER_USER_STATE_0;
}

void bhkCharacterStateSwimming::Update(bhkCharacterController& arInput) {
	hkVector4 v3;
	double m_deltaTime;
	__m128 v10;
	float fGravity;
	uint32_t v15;
	hkVector4 v18;
	hkpCharacterMovementUtil::hkpMovementUtilInput muInput;
	muInput.m_gain = 1.0f;
	uint32_t iFlags = arInput.iFlags;
	v3.m_quad = arInput.UpVec.m_quad;
	muInput.m_forward = arInput.ForwardVec;
	muInput.m_currentVelocity = arInput.OutVelocity;
	uint32_t v11 = std::bit_cast<uint32_t>(_mm_shuffle_ps(arInput.Direction.m_quad, arInput.Direction.m_quad, 170)[0]);
	hkVector4* p_kOutVelocity = &arInput.OutVelocity;
	uint32_t v16 = std::bit_cast<uint32_t>(arInput.Direction.m_quad[0]);
	__m128 v5 = _mm_shuffle_ps(arInput.Direction.m_quad, arInput.Direction.m_quad, 85);
	muInput.m_maxVelocityDelta = 100.0f;
	muInput.m_up.m_quad = v3.m_quad;
	muInput.m_desiredVelocity.m_quad = _mm_unpacklo_ps(_mm_unpacklo_ps(v5, _mm_set_ss(std::bit_cast<float>(v11))),
		_mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(v16)),
			_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))));
	muInput.m_surfaceVelocity = arInput.m_surfaceVelocity;
	if ((iFlags & bhkCharacterController::SUPPORT) != 0) {
		muInput.m_surfaceNormal = arInput.hkSupportNorm;
	} else {
		muInput.m_surfaceNormal.m_quad = v3.m_quad;
	}
	hkpCharacterMovementUtil::calculateMovement(muInput, arInput.OutVelocity);
	hkpCharacterProxy* phkObject = static_cast<hkpCharacterProxy*>(arInput.phkObject);
	const hkVector4* v7 = &hkVector4Zero;
	if (phkObject) {
		arInput.bhkCharacterProxy::GethkWorldRw();
		const hkVector4* Position = &phkObject->getPosition();
		arInput.bhkCharacterProxy::GethkWorldRw();
		v7 = Position;
	}
	float v17 = _mm_shuffle_ps(v7->m_quad, v7->m_quad, 170)[0] - arInput.RotCenter.m_quad[2];
	if (arInput.fSwimFloatHeight >= 0.5f) {
		v17 = arInput.fActorHeight * 0.5 * arInput.fSwimFloatHeight + v17;
	}
	arInput.GethkWorld();
	v18.m_quad = arInput.GethkWorld()->m_gravity.m_quad;
	arInput.GethkWorld();
	arInput.iFlags |= bhkCharacterController::CANJUMP;
	if (arInput.fWaterHeight < static_cast<double>(v17)) {
		if (arInput.eWantState == HK_CHARACTER_JUMPING) {
		LABEL_10:;
			arInput.UpdateState();
			goto LABEL_24;
		}
		if (_mm_shuffle_ps(p_kOutVelocity->m_quad, p_kOutVelocity->m_quad, 170)[0] > 0.0f) {
			arInput.OutVelocity.m_quad[2] = 0.0f;
		}
		goto LABEL_22;
	}
	if (!(arInput.fSwimFloatHeight < 0.5f)) {
		if (1.0f != arInput.fGravity) {
			uint32_t v9;
			v9 = arInput.iFlags;
			if ((v9 & bhkCharacterController::SUPPORT) == 0 && (v9 & bhkCharacterController::SUPPORT2) == 0) {
			LABEL_22:;
				fGravity = arInput.fGravity;
				m_deltaTime = arInput.StepInfo.m_deltaTime;
				goto LABEL_23;
			}
		}
	} else {
		float v13;
		if (arInput.eWantState == HK_CHARACTER_JUMPING && (arInput.iFlags & bhkCharacterController::CANJUMP) != 0) {
			goto LABEL_10;
		}
		v13 = bhkConvert::fBS2HkScaleSC * 10.0;
		if (arInput.fWaterHeight - v13 > v17) {
			fGravity = arInput.fGravity;
			m_deltaTime = -arInput.StepInfo.m_deltaTime;
		LABEL_23:;
			v10 = _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(fGravity)));
			*reinterpret_cast<float*>(&v15) = m_deltaTime;
			p_kOutVelocity->m_quad = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(_mm_shuffle_ps(v10, v10, 0), v18.m_quad),
													_mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v15)),
														_mm_set_ss(std::bit_cast<float>(v15)), 0)),
				p_kOutVelocity->m_quad);
		}
	}
LABEL_24:;
	arInput.ApplyVelocityMod();
	if (arInput.eWantState == HK_CHARACTER_ON_GROUND) {
		arInput.UpdateState();
	}
}
