#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Reconstruction.hpp"

class NiRotKey : public NiAnimationKey {
public:
	NiRotKey* GetKeyAt(uint32_t uiIndex, uint8_t ucKeySize) const {
		return static_cast<NiRotKey*>(NiAnimationKey::GetKeyAt(uiIndex, ucKeySize));
	}

	// The time, then the four floats of m_quat; NiTCBRotKey's and NiBezRotKey's loaders call it as their base's.
	void LoadBinary(NiStream& stream);

	static uint8_t GetKeySize(KeyType eType) { return NiAnimationKey::GetKeySize(ROTKEY, eType); }
	// Fills the derived values of uiNumKeys keys of ucSize bytes each.
	static void FillDerivedVals(NiAnimationKey* pkKeys, uint32_t uiNumKeys, uint8_t ucSize);
	// Value at fTime over uiNumKeys keys; uiLastIdx caches the key the previous call ended on.
	static NiQuaternion GenInterp(float fTime, NiRotKey* pkKeys, KeyType eType, uint32_t uiNumKeys, uint32_t& uiLastIdx,
		uint8_t ucSize);
	// The value at fTime over uiNumKeys keys without a last-index cache (cdecl, 0xA289B0); the exe returns it through
	// a hidden result pointer.
	static NiQuaternion GenInterpDefault(float fTime, NiRotKey* pkKeys, KeyType eType, uint32_t uiNumKeys,
		uint8_t ucSize);
	// NiAnimationKey::SaveBinary, then pkKey's m_quat.
	static void SaveBinary(NiStream& stream, NiAnimationKey* pkKey);
	// NiAnimationKey::Equal (the times), then the rotations; NiBezRotKey's and NiTCBRotKey's
	// Equal call it as their base's.
	static bool Equal(const NiAnimationKey& key0, const NiAnimationKey& key1);
	// Fills NiAnimationKey's tables for NiRotKey's NOINTERP slot once; 1 the first time, else 0.
	static int32_t RegisterLoader();
	// The PDB's NOINTERP table functions, which assert(false) in the source: empty, or returning 0.
	static NiAnimationKey* CreateFromStream(NiStream&, uint32_t) { return nullptr; }
	static void SaveToStream(NiStream&, NiAnimationKey*, uint32_t) {}
	static void Copy(NiAnimationKey*, const NiAnimationKey*) {}
	static NiAnimationKey* CreateArray(uint32_t) { return nullptr; }
	static void DeleteArray(NiAnimationKey*) {}
	// Not named in the PDB or IDA; name chosen here. The PDB's NiRotKey::Interpolate: the identity quaternion at
	// pResult.
	static void Interpolate(float fTime, const NiAnimationKey* pkKey0, const NiAnimationKey* pkKey1, void* pResult);

	NiQuaternion m_quat;

	static NiQuaternion ms_kDefault;
};

// NiRegisterAnimationStream(NiRotKey): a static of every file that includes this header, as in the original.
static int32_t NiRotKeyregisterloaderreturn = NiRotKey::RegisterLoader();
