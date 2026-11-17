#pragma once

#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpCharacterState.h"
#include "Reconstruction.hpp"

class bhkCharacterController;

// The game's base of the character states: the Havok update and change callbacks do nothing here (the exe shares
// one empty stub, ret 0xC), and the states do their work in Update, which is handed the game's controller.
class bhkCharacterState : public hkpCharacterState {
public:
	void update(hkpCharacterContext& arContext, const hkpCharacterInput& arInput,
		hkpCharacterOutput& arOutput) override {}

	void change(hkpCharacterContext& arContext, const hkpCharacterInput& arInput,
		hkpCharacterOutput& arOutput) override {}

	virtual void Update(bhkCharacterController& arController) = 0;
};
