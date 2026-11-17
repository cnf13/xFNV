#pragma once

#include "BSHavok/bhkCharacterState.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Reconstruction.hpp"

// The character state used while airborne.
class bhkCharacterStateInAir : public bhkCharacterState {
public:
	// The constructor, run on apMemory.
	bhkCharacterStateInAir();

	hkpCharacterStateType getType() const override;
	void Update(bhkCharacterController& arInput) override;

	// The largest velocity change the state applies; the constructor stores 2000.0f here.
	float fMaxVelocityDelta;
};
inline constexpr fnvd::Vtable bhkCharacterStateInAir_vftable{};
