#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiRotKey.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

class NiEulerRotKey;
class NiStream;

// A rotation key made of three float key arrays, one per Euler angle (x, y, z).
class NiEulerRotKey : public NiRotKey {
public:
	// A zero quaternion, no keys in any angle.
	NiEulerRotKey();

	// Frees the three angle key arrays.
	void CleanUp();
	// Reads each angle's count, then its type and float keys when it has any.
	void LoadBinary(NiStream& stream);
	void Initialize(NiFloatKey* pkX, uint32_t uiNumX, NiAnimationKey::KeyType eTypeX, NiFloatKey* pkY, uint32_t uiNumY,
		NiAnimationKey::KeyType eTypeY, NiFloatKey* pkZ, uint32_t uiNumZ, NiAnimationKey::KeyType eTypeZ);

	// The rotation at fTime as an NiQuaternion in pkResult, from the three angles' float keys (pkKey1 is unused; the
	// Euler keys' ms_interps entry, 0xA29B10).
	static void Interpolate(float fTime, const NiAnimationKey* pkKey0, const NiAnimationKey* pkKey1, void* pkResult);
	// Writes uiNumKeys keys to kStream, one SaveBinary each (the Euler keys' ms_saves entry, 0xA2A0E0).
	static void SaveToStream(NiStream& stream, NiAnimationKey* pkKeys, uint32_t uiNumKeys);
	// Writes one key: each angle's count, then its type and float keys when it has any.
	static void SaveBinary(NiStream& stream, NiAnimationKey* pkKey);
	// Whether two keys have the same angle counts, types, key sizes and float keys (ms_equals entry, 0xA29C20).
	static bool Equal(const NiAnimationKey& key0, const NiAnimationKey& key1);
	// Copies the counts and types and a new copy of each angle's float keys (ms_copies entry, 0xA29CF0).
	static void Copy(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey);
	// NiNew NiEulerRotKey[uiNumKeys] (ms_arrays entry, 0xA29DB0).
	static NiAnimationKey* CreateArray(uint32_t uiNumKeys);
	// Inserts a float key at fTime into each angle that has keys; makes a one-key array when pkKeys is empty
	// (ms_inserts entry, 0xA29E40).
	static bool Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys);
	// A new array of uiNumKeys keys, each read by LoadBinary (ms_loaders entry, 0xA2A070).
	static NiAnimationKey* CreateFromStream(NiStream& stream, uint32_t uiNumKeys);
	// Once (the guard byte), fills the ROTKEY/EULERKEY slot of each function table NiAnimationKey keeps and its key
	// size, RegisterSupportedFunctions inlined; 1 the first time, else 0 (cdecl, 0xA2A110). PDB: returns bool; the
	// exe returns the whole eax (xor eax, eax / mov eax, 1).
	static int32_t RegisterLoader();
	// The key array's FLOATKEY fill function for each angle that has keys (registered in ms_fillderived). Protected
	// in the engine; public here so the replacement table can name it.
	static void FillDerivedVals(NiAnimationKey* pkKeys, uint32_t uiNumKeys, uint8_t ucSize);

	uint32_t m_uiNumKeys[3];
	NiAnimationKey::KeyType m_eType[3];
	uint8_t m_ucSizes[3];
	NiFloatKey* m_apkKeys[3];
	uint32_t m_uiLastIdx[3];
};

// NiRegisterAnimationStream(NiEulerRotKey): a static of every file that includes this header, as in the original.
static int32_t NiEulerRotKeyregisterloaderreturn = NiEulerRotKey::RegisterLoader();
