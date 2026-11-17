#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Reconstruction.hpp"

class NiFloatKey : public NiAnimationKey {
public:
	NiFloatKey* GetKeyAt(uint32_t uiIndex, uint8_t ucKeySize) const {
		return static_cast<NiFloatKey*>(NiAnimationKey::GetKeyAt(uiIndex, ucKeySize));
	}

	// The exe calls an out-of-line copy (TESImageSpaceModifier::InitializeData 0x52BD40).
	void SetValue(float fValue);
	float GetValue() const { return m_fValue; }
	// The base's LoadBinary, then m_fValue's 4 bytes.
	void LoadBinary(NiStream& stream);

	static uint8_t GetKeySize(KeyType eType) { return NiAnimationKey::GetKeySize(FLOATKEY, eType); }
	static InterpFunction GetInterpFunction(KeyType eType);
	// Value at fTime over uiNumKeys keys; uiLastIdx caches the key the previous call ended on.
	static float GenInterp(float fTime, NiFloatKey* pkKeys, KeyType eType, uint32_t uiNumKeys, uint32_t& uiLastIdx,
		uint8_t ucSize);
	// The value at fTime over uiNumKeys keys without a cached index: ms_fDefault without keys, the first or last
	// key's value outside their times, else GenInterp from index 0. Protected in the engine; public here so the
	// replacement table can name it.
	static float GenInterpDefault(float fTime, NiFloatKey* pkKeys, KeyType eType, uint32_t uiNumKeys, uint8_t ucSize);
	// The PDB's NOINTERP table functions, which assert(false) in the source: empty, or returning 0.
	static NiAnimationKey* CreateFromStream(NiStream&, uint32_t) { return nullptr; }
	static void SaveToStream(NiStream&, NiAnimationKey*, uint32_t) {}
	static void Copy(NiAnimationKey*, const NiAnimationKey*) {}
	static NiAnimationKey* CreateArray(uint32_t) { return nullptr; }
	static void DeleteArray(NiAnimationKey*) {}
	static void Interpolate(float, const NiAnimationKey*, const NiAnimationKey*, void*) {}
	static void FillDerivedVals(NiAnimationKey*, uint32_t, uint8_t) {}
	// Named here after its address; it is the PDB's NiFloatKey::RegisterLoader, with RegisterSupportedFunctions
	// inlined: fills NiAnimationKey's tables for the FLOATKEY/NOINTERP slot once; 1 the first time, else 0.
	static int32_t RegisterLoader();
	// Named here after its address; it is the PDB's NiFloatKey::SaveBinary.
	// Inferred type: a1: NiStream* from 0xa2ca6d (parameter 0 of NiTCBFloatKey::SaveBinary), its members used at 0xa26f01
	// a2 is the key: the body is the PDB's NiFloatKey::SaveBinary(NiStream&, NiAnimationKey*) (NiAnimationKey::SaveBinary
	// 0xA3DA50 on a2, then the 4 bytes at a2 + 4, m_fValue).
	static uint32_t SaveBinary(NiStream* a1, NiAnimationKey* a2);
	// The times (NiAnimationKey::Equal), then the values. The false after the times is NiAnimationKey::Equal's
	// int32_t; the callers test its low byte.
	static bool Equal(const NiAnimationKey& key0, const NiAnimationKey& key1);

	float m_fValue;

	static inline float ms_fDefault;
};

// NiRegisterAnimationStream(NiFloatKey): a static of every file that includes this header, as in the original.
static int32_t NiFloatKeyregisterloaderreturn = NiFloatKey::RegisterLoader();
