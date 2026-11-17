#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"
#include "Reconstruction.hpp"

// A float key interpolated linearly to the next one: the time and value of NiFloatKey, nothing more.
class NiLinFloatKey : public NiFloatKey {
public:
	static void Interpolate(float fTime, const NiAnimationKey* pKey0, const NiAnimationKey* pKey1, void* pResult);
	// Inserts a key at fTime with the interpolated value, in a new array one key longer (cdecl, 0xA2A620;
	// ms_inserts entry).
	static bool Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys);
	// Once (the guard byte), fills the FLOATKEY/LINKEY slot of each function table NiAnimationKey keeps and its key
	// size, RegisterSupportedFunctions inlined; 1 the first time, else 0 (cdecl, 0xA2A720). PDB: returns bool; the
	// exe returns the whole eax (xor eax, eax / mov eax, 1).
	static int32_t RegisterLoader();
	// Writes one key (cdecl, 0xA295F0): the exe's code is NiBoolKey's, the time and the byte at +4 as 0 or 1.
	// Protected in the engine, like SaveToStream; public here so the replacement table can name them.
	static void SaveBinary(NiStream& stream, NiAnimationKey* pkKey);
	// Writes uiNumKeys keys to kStream, one SaveBinary each.
	static void SaveToStream(NiStream& stream, NiAnimationKey* pkKeys, uint32_t uiNumKeys);
	// The PDB's NiLinFloatKey::FillDerivedVals: nothing to derive.
	static void FillDerivedVals(NiAnimationKey*, uint32_t, uint8_t) {}
	// Not named in the PDB or IDA; name chosen here. The PDB's NiLinFloatKey::CreateArray (the table's ArrayFunction):
	// uiNumKeys 8-byte keys after a count cookie, from MemoryManager.
	static NiAnimationKey* CreateArray(uint32_t uiNumKeys);
	// NiFloatKey::Equal; the step float keys' table holds it too.
	static bool Equal(const NiAnimationKey& key0, const NiAnimationKey& key1);
	// The time and value; the step float keys' table holds it too.
	static void Copy(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey);
};

// NiRegisterAnimationStream(NiLinFloatKey): a static of every file that includes this header, as in the original.
static int32_t NiLinFloatKeyregisterloaderreturn = NiLinFloatKey::RegisterLoader();
