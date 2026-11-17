#pragma once

#include "BSHavok/bhkCharacterState.h"

// The character state for flying. Only its vtable is declared: bhkCharacterControllerCinfo's constructor builds it
// inline.
class bhkCharacterStateFlying : public bhkCharacterState {
public:
    hkpCharacterStateType getType() const override {
        return HK_CHARACTER_FLYING;
    }
    // Flying does nothing per update.
    void Update(bhkCharacterController& arController) override {}
};
