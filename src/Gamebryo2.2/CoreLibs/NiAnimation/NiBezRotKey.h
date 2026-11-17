#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class NiStream;

// A rotation key with a second quaternion, m_IntQuat, after the key's own rotation. IDA has no struct for it; the
// layout is the base's 0x14 bytes and the PDB's single member, and the exe's Copy moves the 8 dwords after the time.
class NiBezRotKey : public NiRotKey {
public:
	// The PDB's NiBezRotKey::NiBezRotKey: zeroes m_quat through NiQuaternion::SetValues
	// (0xA6DB50) and returns apMemory (thiscall, ret 0, 0xA2BDB0); new[]'s vector constructor iterator runs it.
	NiBezRotKey();

	static void Copy(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey);
	// Puts a key at fTime into the array, which it reallocates one key larger; false when OkayToInsert refuses.
	static bool Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys);
	// Fills the derived values (m_IntQuat) of uiNumKeys keys of ucSize bytes each.
	static void FillDerivedVals(NiAnimationKey* pkKeys, uint32_t uiNumKeys, uint8_t ucSize);
	static int32_t RegisterLoader();
	static void Interpolate(float fTime, const NiAnimationKey* pKey0, const NiAnimationKey* pKey1, void* pResult);
	static bool Equal(const NiAnimationKey& key0, const NiAnimationKey& key1);
	static NiAnimationKey* CreateArray(uint32_t uiNumKeys);
	// Allocates uiNumKeys keys (count cookie first) and loads each through NiRotKey::LoadBinary.
	static NiAnimationKey* CreateFromStream(NiStream& stream, uint32_t uiNumKeys);
	// Saves each of the uiNumKeys keys (0x24 bytes apart) through 0xA28720 (cdecl, 0xA28ED0;
	// NiBezRotKey_SaveToStream below is its address as a value).
	static void SaveToStream(NiStream& stream, NiAnimationKey* pkKeys, uint32_t uiNumKeys);

	NiQuaternion m_IntQuat;
};

// NiRegisterAnimationStream(NiBezRotKey): a static of every file that includes this header, as in the original.
static int32_t NiBezRotKeyregisterloaderreturn = NiBezRotKey::RegisterLoader();
