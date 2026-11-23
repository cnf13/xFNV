#pragma once

#include "BSHavok/bhkCharacterState.h"

// The character state used while swimming.
class bhkCharacterStateSwimming : public bhkCharacterState {
public:
    // The constructor, run on apMemory.
    bhkCharacterStateSwimming();

    hkpCharacterStateType getType() const override;
    void Update(bhkCharacterController& arInput) override;
};
