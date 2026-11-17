#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiStepRotKey.h"

class NiStream;

// A rotation key slerped to the next one: the time and quaternion of NiRotKey, nothing more.
class NiLinRotKey : public NiRotKey {
public:
	// Allocates NiNew NiLinRotKey[uiNumKeys]; this is the ms_arrays entry for both NiLinRotKey and NiStepRotKey.
	static NiAnimationKey* CreateArray(uint32_t uiNumKeys);
	// pKey0's quaternion slerped towards pKey1's by fTime, into the NiQuaternion at pResult (ms_interps entry).
	static void Interpolate(float fTime, const NiAnimationKey* pKey0, const NiAnimationKey* pKey1, void* pResult);
	// A new array of uiNumKeys keys, each read by LoadBinary; this is also the loader for NiStepRotKey.
	static NiAnimationKey* CreateFromStream(NiStream& stream, uint32_t uiNumKeys);
	// Inserts a key at fTime with the interpolated rotation, in a new array one key longer, then refills the derived
	// values (ms_inserts entry).
	static bool Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys);
	// Once (the guard byte), fills the ROTKEY/LINKEY slot of each function table NiAnimationKey keeps and its key size;
	// returns 1 the first time, else 0.
	static int32_t RegisterLoader();
	// NiRotKey's; the step rotation keys' table holds it too.
	static void FillDerivedVals(NiAnimationKey* pkKeys, uint32_t uiNumKeys, uint8_t ucSize);
	// NiRotKey::Equal; the step rotation keys' table holds it too.
	static bool Equal(const NiAnimationKey& key0, const NiAnimationKey& key1);
	// One NiRotKey::SaveBinary per 20-byte key; the step rotation keys' table holds it too.
	static void SaveToStream(NiStream& kStream, NiAnimationKey* pkKeys, uint32_t uiNumKeys);
};

// NiRegisterAnimationStream(NiLinRotKey): a static of every file that includes this header, as the game does.
static int32_t NiLinRotKeyregisterloaderreturn = NiLinRotKey::RegisterLoader();
