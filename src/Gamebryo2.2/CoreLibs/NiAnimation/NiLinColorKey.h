#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiColorKey.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

// A color key interpolated linearly to the next one: the time and color of NiColorKey, nothing more.
class NiLinColorKey : public NiColorKey {
public:
	NiLinColorKey();

	// The table's equality test for linear (and, folded, step) color keys: NiColorKey::Equal.
	static bool Equal(const NiAnimationKey& key0, const NiAnimationKey& key1);
	// Copies the time and the four color floats (cdecl, 0xA2AB10; ms_copies entry). The exe's NiLinRotKey,
	// NiStepRotKey and NiStepColorKey slots hold this address too (the same 5-dword copy).
	static void Copy(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey);
	// NiNew NiLinColorKey[uiNumKeys] (cdecl, 0xA2A1B0; ms_arrays entry).
	static NiAnimationKey* CreateArray(uint32_t uiNumKeys);
	// A new array of uiNumKeys keys, each read by LoadBinary (cdecl, 0xA2A320; ms_loaders entry). Protected in the
	// engine; public here so the replacement table can name it.
	static NiAnimationKey* CreateFromStream(NiStream& kStream, uint32_t uiNumKeys);
	// Inserts a key at fTime with the interpolated color, in a new array one key longer (cdecl, 0xA2A390;
	// ms_inserts entry).
	static bool Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys);
	// Once (the guard byte), fills the COLORKEY/LINKEY slot of each function table NiAnimationKey keeps and its key
	// size, RegisterSupportedFunctions inlined; 1 the first time, else 0 (cdecl, 0xA2A4D0). PDB: returns bool; the
	// exe returns the whole eax (xor eax, eax / mov eax, 1).
	static int32_t RegisterLoader();
	// Not named in the PDB or IDA; name chosen here. The PDB's NiLinColorKey::Interpolate: pkKey0's color times
	// (1 - fTime) plus pkKey1's times fTime, at pResult. The exe leaves pResult in eax; the table's callers ignore it.
	static void Interpolate(float fTime, const NiAnimationKey* pkKey0, const NiAnimationKey* pkKey1, void* pResult);
	// One NiColorKey::SaveBinary per 20-byte key; the step color keys' table holds it too.
	static void SaveToStream(NiStream& kStream, NiAnimationKey* pkKeys, uint32_t uiNumKeys);
};

// NiRegisterAnimationStream(NiLinColorKey): a static of every file that includes this header, as in the original.
static int32_t NiLinColorKeyregisterloaderreturn = NiLinColorKey::RegisterLoader();
