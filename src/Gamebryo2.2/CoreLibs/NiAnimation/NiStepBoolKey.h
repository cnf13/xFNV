#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinFloatKey.h"
#include "Reconstruction.hpp"

class NiStream;

// A bool key that holds its value until the next key.
class NiStepBoolKey : public NiBoolKey {
public:
	// Copies a step bool key: the time and the flag byte.
	static void Copy(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey);

	// BOOLKEY/STEPKEY's ms_equals entry: NiBoolKey::Equal.
	static bool Equal(const NiAnimationKey& key0, const NiAnimationKey& key1);

	// The step bool value: pkKey0's flag while fTime < 1, else pkKey1's,
	// into the byte at pvResult.
	static void Interpolate(float fTime, const NiAnimationKey* pkKey0, const NiAnimationKey* pkKey1, void* pvResult);

	static bool Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys);

	// A new array of uiNumKeys step bool keys, each read by
	// NiBoolKey::LoadBinary.
	static NiAnimationKey* CreateFromStream(NiStream& kStream, uint32_t uiNumKeys);

	// Once (the guard byte), fills BOOLKEY/STEPKEY's slot of each function table NiAnimationKey keeps and its key
	// size, RegisterSupportedFunctions inlined; 1 the first time, else 0.
	static int32_t RegisterLoader();
};

// NiRegisterAnimationStream(NiStepBoolKey): a static of every file that includes this header, as in the original.
static int32_t NiStepBoolKeyregisterloaderreturn = NiStepBoolKey::RegisterLoader();
