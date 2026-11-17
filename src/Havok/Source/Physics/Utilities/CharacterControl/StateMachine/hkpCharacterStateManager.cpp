#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpCharacterStateManager.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpCharacterState.h"

// No state registered for any type.
hkpCharacterStateManager::hkpCharacterStateManager() : m_registeredState{} {}

hkpCharacterStateManager::~hkpCharacterStateManager() {
	for (int32_t i = 0; i < HK_CHARACTER_MAX_STATE_ID; i++) {
		if (m_registeredState[i]) {
			m_registeredState[i]->removeReference();
			m_registeredState[i] = nullptr;
		}
	}
}

// The handler registered for a character state type, null while the game has registered none.
hkpCharacterState* hkpCharacterStateManager::getState(hkpCharacterStateType stateType) {
	return m_registeredState[stateType];
}

// The new state is referenced before the state it replaces is released.
void hkpCharacterStateManager::registerState(hkpCharacterState* apState, hkpCharacterStateType aeState) {
	apState->addReference();
	hkpCharacterState* pOldState = m_registeredState[aeState];
	if (pOldState != nullptr) {
		pOldState->removeReference();
	}
	m_registeredState[aeState] = apState;
}
