#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpCharacterState.h"

#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpCharacterContext.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpCharacterStateManager.h"

// hkpCharacterState is abstract; these empty bodies are the ones every game character state uses for enterState and
// leaveState.

void hkpCharacterState::enterState(
    hkpCharacterContext& context,
    hkpCharacterStateType prevState,
    const hkpCharacterInput& input,
    hkpCharacterOutput& output
) {
    ;
}

void hkpCharacterState::leaveState(
    hkpCharacterContext& context,
    hkpCharacterStateType nextState,
    const hkpCharacterInput& input,
    hkpCharacterOutput& output
) {
    ;
}
