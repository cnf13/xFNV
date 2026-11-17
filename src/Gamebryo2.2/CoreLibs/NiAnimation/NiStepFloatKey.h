#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"

// A float key held until the next one: the time and value of NiFloatKey, nothing more (keysize 8, ms_keysizes[5]).
class NiStepFloatKey : public NiFloatKey {
public:
	// The step value: pkKey0's value while fTime < 1, else pkKey1's, into the float at pvResult (ms_interps[FLOATKEY,
	// STEPKEY]).
	static void Interpolate(float fTime, const NiAnimationKey* pkKey0, const NiAnimationKey* pkKey1, void* pvResult);
	static bool Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys);
	static int32_t RegisterLoader();
};

// NiRegisterAnimationStream(NiStepFloatKey): a static of every file that includes this header, as the game does.
static int32_t NiStepFloatKeyregisterloaderreturn = NiStepFloatKey::RegisterLoader();
