#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpCharacterState.h"

#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpCharacterContext.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpCharacterStateManager.h"

// The exe has no hkpCharacterState vtable (the class is abstract): these entries are the ones every game character
// state's table holds in slots 4 and 5.

void hkpCharacterState::enterState(hkpCharacterContext& context, hkpCharacterStateType prevState,
	const hkpCharacterInput& input, hkpCharacterOutput& output) {
	;
}

void hkpCharacterState::leaveState(hkpCharacterContext& context, hkpCharacterStateType nextState,
	const hkpCharacterInput& input, hkpCharacterOutput& output) {
	;
}
