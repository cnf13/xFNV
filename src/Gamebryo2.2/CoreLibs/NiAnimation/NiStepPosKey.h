#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Reconstruction.hpp"

// A position key held until the next one: the time and position of NiPosKey, nothing more. The class is only the
// set of static functions NiAnimationKey's tables hold for this key type.
class NiStepPosKey : public NiPosKey {
public:
	// Frees a key array the exe allocated with a 4-byte header in front of it (cdecl, 0xA2A600).
	static void DeleteArray(NiAnimationKey* pkKeyArray);
	// Not named in the PDB or IDA; name chosen here. The PDB's NiStepPosKey::Interpolate: pkKey0's position, pkKey1's
	// from fTime 1 on, copied as dwords to pResult. The exe leaves the last dword in eax; the table's callers ignore it.
	static void Interpolate(float fTime, const NiAnimationKey* pkKey0, const NiAnimationKey* pkKey1, void* pResult);
	// Not named in the PDB or IDA; name chosen here. The PDB's NiStepPosKey::InterpolateD1: pkKey0's position minus
	// pkKey1's, at pResult. The exe leaves pResult in eax; the table's callers ignore it.
	static void InterpolateD1(float fTime, const NiAnimationKey* pkKey0, const NiAnimationKey* pkKey1, void* pResult);
	static bool Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys);
	static void RegisterSupportedFunctions(NiAnimationKey::KeyContent eContent, NiAnimationKey::KeyType eType);
	static int32_t RegisterLoader();
};

// NiRegisterAnimationStream(NiStepPosKey): a static of every file that includes this header, as in the original.
static int32_t NiStepPosKeyregisterloaderreturn = NiStepPosKey::RegisterLoader();
