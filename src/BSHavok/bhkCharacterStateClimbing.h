#pragma once

#include "BSHavok/bhkCharacterState.h"
#include "Reconstruction.hpp"

class bhkCharacterController;

// The character state for climbing (getType answers the climbing state type).
class bhkCharacterStateClimbing : public bhkCharacterState {
public:
	hkpCharacterStateType getType() const override;
	void Update(bhkCharacterController& arInput) override;
};
inline constexpr fnvd::Vtable bhkCharacterStateClimbing_vftable{};
