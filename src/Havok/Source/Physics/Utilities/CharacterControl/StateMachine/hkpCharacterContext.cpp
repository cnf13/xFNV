#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpCharacterContext.h"

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpCharacterState.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpCharacterStateManager.h"

#include <bit>
#include <xmmintrin.h>

// A proxy character in state a2, with the default filter on; the context holds a reference to the state manager.
hkpCharacterContext::hkpCharacterContext(const hkpCharacterStateManager* a1, hkpCharacterStateType a2)
	: m_characterType(HK_CHARACTER_PROXY), m_stateManager(const_cast<hkpCharacterStateManager*>(a1)),
	  m_currentState(a2), m_previousState(a2), m_maxLinearAcceleration(625.0f), m_maxLinearVelocity(20.0f),
	  m_gain(1.0f) {
	m_filterEnable.m_bool = 1;
	m_stateManager->addReference();
}

// The context's reference to the state manager is dropped.
hkpCharacterContext::~hkpCharacterContext() {
	m_stateManager->removeReference();
}

hkpCharacterStateType hkpCharacterContext::getState() {
	return m_currentState;
}

void hkpCharacterContext::setFilterParameters(const float a1, const float a2, const float a3) {
	m_gain = a1;
	m_maxLinearVelocity = a2;
	m_maxLinearAcceleration = a3;
}

void hkpCharacterContext::setState(hkpCharacterStateType assertBuf, const hkpCharacterInput& input,
	hkpCharacterOutput& a3) {
	int32_t v5 = (int32_t)m_currentState;
	hkpCharacterState* state = m_stateManager->getState(static_cast<hkpCharacterStateType>(v5));
	state->leaveState(*this, assertBuf, input, a3);
	hkpCharacterStateManager* v7 = m_stateManager;
	m_currentState = static_cast<hkpCharacterStateType>((int32_t)assertBuf);
	hkpCharacterState* v8 = v7->getState(assertBuf);
	v8->enterState(*this, static_cast<hkpCharacterStateType>(v5), input, a3);
}

void hkpCharacterContext::update(const hkpCharacterInput& input, hkpCharacterOutput& output) {
	alignas(16) uint32_t vars0b;
	alignas(16) uint32_t vars0c;
	output.m_velocity = input.m_velocity;
	hkpCharacterState* State = m_stateManager->getState(m_currentState);
	State->change(*this, input, output);
	if (m_currentState == m_previousState) {
		hkpCharacterState* v5 = m_stateManager->getState(m_currentState);
		v5->update(*this, input, output);
	}
	bool v6 = m_filterEnable.m_bool == 0;
	m_previousState = m_currentState;
	if (!v6) {
		hkVector4 v10;
		double v14;
		__m128 m_quad = output.m_velocity.m_quad;
		__m128 v8 = _mm_mul_ps(m_quad, m_quad);
		float v9 = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(static_cast<float>((_mm_shuffle_ps(v8, v8, 170)[0] +
			static_cast<float>(_mm_shuffle_ps(v8, v8, 85)[0] + v8[0]))))));
		if (m_maxLinearVelocity < static_cast<double>(v9)) {
			*reinterpret_cast<float*>(&vars0b) = m_maxLinearVelocity / v9;
			output.m_velocity.m_quad = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(vars0b)),
													  _mm_set_ss(std::bit_cast<float>(vars0b)), 0),
				m_quad);
		}
		v10.m_quad = input.m_velocity.m_quad;
		__m128 v11 =
			_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(input.m_stepInfo
											  .m_invDeltaTime))),
						   _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(input.m_stepInfo.m_invDeltaTime))),
						   0),
				_mm_sub_ps(output.m_velocity.m_quad, v10.m_quad));
		__m128 v12 = _mm_mul_ps(v11, v11);
		double v13 = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(static_cast<float>((_mm_shuffle_ps(v12, v12, 170)[0] +
						 static_cast<float>(_mm_shuffle_ps(v12, v12, 85)[0] + v12[0])))))) /
			m_maxLinearAcceleration;
		output.m_velocity.m_quad = v10.m_quad;
		float vars0 = v13;
		if (!(vars0 > 1.0f) || m_currentState == HK_CHARACTER_JUMPING) {
			v14 = input.m_stepInfo.m_deltaTime * m_gain;
		} else {
			v14 = input.m_stepInfo.m_deltaTime * m_gain / vars0;
		}
		*reinterpret_cast<float*>(&vars0c) = v14;
		output.m_velocity.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(vars0c)),
															 _mm_set_ss(std::bit_cast<float>(vars0c)), 0),
												  v11),
			v10.m_quad);
	}
}

void hkpCharacterContext::setFilterEnable(const hkBool status) {
	m_filterEnable.m_bool = status.m_bool;
}

void hkpCharacterContext::setCharacterType(const hkpCharacterContext::CharacterType newType) {
	m_characterType = newType;
}
