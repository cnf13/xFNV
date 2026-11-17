#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpCharacterStateManager.h"

class hkpCharacterContext;
struct hkpCharacterInput;
struct hkpCharacterOutput;

// The handler of one character state (on ground, jumping, in air ...): it is entered, updated every step, asked
// to change state and left. enterState and leaveState do nothing here; the others are supplied by the state.
class hkpCharacterState : public hkReferencedObject {
public:
    // The state type this handler is registered for.
    virtual hkpCharacterStateType getType() const = 0;
    virtual void enterState(
        hkpCharacterContext& context,
        hkpCharacterStateType prevState,
        const hkpCharacterInput& input,
        hkpCharacterOutput& output
    );
    virtual void leaveState(
        hkpCharacterContext& context,
        hkpCharacterStateType nextState,
        const hkpCharacterInput& input,
        hkpCharacterOutput& output
    );
    virtual void
    update(hkpCharacterContext& arContext, const hkpCharacterInput& arInput, hkpCharacterOutput& arOutput) = 0;
    virtual void
    change(hkpCharacterContext& arContext, const hkpCharacterInput& arInput, hkpCharacterOutput& arOutput) = 0;
};
