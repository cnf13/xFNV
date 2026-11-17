#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "Reconstruction.hpp"

class Actor;

// The near and far sound handles of one weapon's fire sound, with the actor that fired it
// and when.
class WeaponNearFarSound {
public:
	// IDA's WeaponNearFarSound::Init; not in the PDB. The exe's constructor: it returns `this` in eax.
	WeaponNearFarSound();

	BSSoundHandle soundNear;
	BSSoundHandle soundFar;
	Actor* uiIdentifier;
	uint32_t uiTimestamp;
	float fRelativeDistance;
};
