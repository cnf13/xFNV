#pragma once

#include "BSHavok/bhkCharacterState.h"
#include "Reconstruction.hpp"

// The character state used for jumping.
class bhkCharacterStateJumping : public bhkCharacterState {
public:
	// The constructor, run on apMemory.
	bhkCharacterStateJumping();

	hkpCharacterStateType getType() const override;
	void Update(bhkCharacterController& arInput) override;
};
inline constexpr fnvd::Vtable bhkCharacterStateJumping_vftable{};
