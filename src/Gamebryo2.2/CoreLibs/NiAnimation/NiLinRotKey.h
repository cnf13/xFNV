#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiStepRotKey.h"
#include "Reconstruction.hpp"

class NiStream;

// A rotation key slerped to the next one: the time and quaternion of NiRotKey, nothing more. Its Copy is
// NiLinColorKey::Copy (0xA2AB10, the same 5-dword copy), registered there.
class NiLinRotKey : public NiRotKey {
public:
	// NiNew NiLinRotKey[uiNumKeys] (cdecl, 0xA2AB40; ms_arrays entry, NiStepRotKey's slot too).
	static NiAnimationKey* CreateArray(uint32_t uiNumKeys);
	// pKey0's quaternion slerped towards pKey1's by fTime, into the NiQuaternion at pResult (cdecl, 0xA2AB90;
	// ms_interps entry).
	static void Interpolate(float fTime, const NiAnimationKey* pKey0, const NiAnimationKey* pKey1, void* pResult);
	// A new array of uiNumKeys keys, each read by LoadBinary (cdecl, 0xA2ABC0; ms_loaders entry, NiStepRotKey's slot
	// too). Protected in the engine; public here so the replacement table can name it.
	static NiAnimationKey* CreateFromStream(NiStream& stream, uint32_t uiNumKeys);
	// Inserts a key at fTime with the interpolated rotation, in a new array one key longer, then refills the derived
	// values (cdecl, 0xA2AC30; ms_inserts entry).
	static bool Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys);
	// Once (the guard byte), fills the ROTKEY/LINKEY slot of each function table NiAnimationKey keeps and its key
	// size, RegisterSupportedFunctions inlined; 1 the first time, else 0 (cdecl, 0xA2AD80). PDB: returns bool; the
	// exe returns the whole eax (xor eax, eax / mov eax, 1).
	static int32_t RegisterLoader();
	// NiRotKey's; the step rotation keys' table holds it too.
	static void FillDerivedVals(NiAnimationKey* pkKeys, uint32_t uiNumKeys, uint8_t ucSize);
	// NiRotKey::Equal; the step rotation keys' table holds it too.
	static bool Equal(const NiAnimationKey& key0, const NiAnimationKey& key1);
	// One NiRotKey::SaveBinary per 20-byte key; the step rotation keys' table holds it too.
	static void SaveToStream(NiStream& kStream, NiAnimationKey* pkKeys, uint32_t uiNumKeys);
};

// NiRegisterAnimationStream(NiLinRotKey): a static of every file that includes this header, as in the original.
static int32_t NiLinRotKeyregisterloaderreturn = NiLinRotKey::RegisterLoader();
