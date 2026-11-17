#pragma once

#include "BSHavok/bhkCharacterState.h"
#include "Reconstruction.hpp"

// The character state used while supported by the ground.
class bhkCharacterStateOnGround : public bhkCharacterState {
public:
	// The constructor, run on apMemory.
	bhkCharacterStateOnGround();

	hkpCharacterStateType getType() const override;
	void Update(bhkCharacterController& arInput) override;

	// Whether a launch drops the character's velocity; the constructor stores 1 here as a byte.
	bool bKillVelocityOnLaunch;
	uint8_t padding09[3];
};
inline constexpr fnvd::Vtable bhkCharacterStateOnGround_vftable{};
