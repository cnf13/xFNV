#pragma once

#include "BSHavok/bhkCharacterState.h"
#include "Reconstruction.hpp"

// The character state used while swimming.
class bhkCharacterStateSwimming : public bhkCharacterState {
public:
	// The constructor, run on apMemory.
	bhkCharacterStateSwimming();

	hkpCharacterStateType getType() const override;
	void Update(bhkCharacterController& arInput) override;
};
inline constexpr fnvd::Vtable bhkCharacterStateSwimming_vftable{};
