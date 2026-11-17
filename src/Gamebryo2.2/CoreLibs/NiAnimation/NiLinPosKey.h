#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBezFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiStepPosKey.h"
#include "Reconstruction.hpp"

class NiStream;

// A position key interpolated linearly to the next one: the time and position of NiPosKey, nothing more (its
// LoadBinary is NiPosKey's: the exe calls NiPosKey::LoadBinary, 0xA27430).
class NiLinPosKey : public NiPosKey {
public:
	// Copies the time and position (cdecl, 0xA2A7B0; ms_copies entry).
	static void Copy(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey);
	// fTime * pKey1's position + (1 - fTime) * pKey0's, into the NiPoint3 at pResult (cdecl, 0xA2A7F0; ms_interps
	// entry).
	static void Interpolate(float fTime, const NiAnimationKey* pKey0, const NiAnimationKey* pKey1, void* pResult);
	// Loads uiNumKeys keys into a new array (cdecl, ret 0, 0xA2A8E0; ms_loaders' NiAnimationKey::CreateFunction).
	// Protected in the engine; public here so the replacement table can name it.
	static NiAnimationKey* CreateFromStream(NiStream& stream, uint32_t uiNumKeys);
	// Inserts a key at fTime with the interpolated position, in a new array one key longer (cdecl, 0xA2A940;
	// ms_inserts entry).
	static bool Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys);
	// Fills this key type's slot of the curvature, interpolation, equality, fill-derived and insert tables.
	static void RegisterSupportedFunctions(KeyContent eContent, KeyType eType);
	// Once (the guard byte), fills the POSKEY/LINKEY slot's stream and array functions and key size, then registers
	// its supported functions; 1 the first time, else 0 (cdecl, 0xA2AAB0). PDB: returns bool; the exe returns the
	// whole eax (xor eax, eax / mov eax, 1).
	static int32_t RegisterLoader();
	// Not named in the PDB or IDA; name chosen here. The PDB's NiLinPosKey::InterpolateD1: pkKey1's position minus
	// pkKey0's, at pResult. The exe leaves pResult in eax; the table's callers ignore it.
	static void InterpolateD1(float fTime, const NiAnimationKey* pkKey0, const NiAnimationKey* pkKey1, void* pResult);
	// NiPosKey's equality test (NiPosKey_Equal); the step position keys' table holds it too.
	static bool Equal(const NiAnimationKey& key0, const NiAnimationKey& key1);
	// One NiPosKey::SaveBinary per 16-byte key; the step position keys' table holds it too.
	static void SaveToStream(NiStream& kStream, NiAnimationKey* pkKeys, uint32_t uiNumKeys);
};

// NiRegisterAnimationStream(NiLinPosKey): a static of every file that includes this header, as in the original.
static int32_t NiLinPosKeyregisterloaderreturn = NiLinPosKey::RegisterLoader();
