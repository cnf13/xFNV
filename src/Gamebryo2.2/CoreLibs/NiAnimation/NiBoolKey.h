#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"

class NiStream;

class NiBoolKey : public NiAnimationKey {
public:
	NiBoolKey* GetKeyAt(uint32_t uiIndex, uint8_t ucKeySize) const {
		return static_cast<NiBoolKey*>(NiAnimationKey::GetKeyAt(uiIndex, ucKeySize));
	}

	void SetBool(bool bBool) { m_bBool = bBool; }
	bool GetBool() const { return m_bBool; }
	void LoadBinary(NiStream& stream);

	static uint8_t GetKeySize(KeyType eType) { return NiAnimationKey::GetKeySize(BOOLKEY, eType); }
	// Value at fTime over uiNumKeys keys; uiLastIdx caches the key the previous call ended on.
	static uint8_t GenInterp(float fTime, NiBoolKey* pkKeys, KeyType eType, uint32_t uiNumKeys, uint32_t& uiLastIdx,
		uint8_t ucSize);
	// The value at fTime over uiNumKeys keys without a cached index: ms_bDefault without keys, the first or last
	// key's value outside their times, else GenInterp from index 0. Protected in the engine; public here so the
	// replacement table can name it.
	static uint8_t GenInterpDefault(float fTime, NiBoolKey* pkKeys, KeyType eType, uint32_t uiNumKeys, uint8_t ucSize);
	// NiAnimationKey::Equal (the times), then the flags.
	static bool Equal(const NiAnimationKey& key0, const NiAnimationKey& key1);
	// Stores NiBoolKey's interpolation functions in NiAnimationKey's tables at eContent's eType slot.
	static void RegisterSupportedFunctions(KeyContent eContent, KeyType eType);
	// Fills NiAnimationKey's tables for NiBoolKey's NOINTERP slot once; 1 the first time, else 0.
	static int32_t RegisterLoader();

	// NOINTERP table functions, which assert(false) in Gamebryo: empty, or returning 0.
	static NiAnimationKey* CreateFromStream(NiStream&, uint32_t) { return nullptr; }
	static void SaveToStream(NiStream&, NiAnimationKey*, uint32_t) {}
	static void Copy(NiAnimationKey*, const NiAnimationKey*) {}
	static NiAnimationKey* CreateArray(uint32_t) { return nullptr; }
	static void DeleteArray(NiAnimationKey*) {}
	static void FillDerivedVals(NiAnimationKey*, uint32_t, uint8_t) {}
	// The table's InterpFunction: stores ms_bDefault at pResult.
	static void Interpolate(float fTime, const NiAnimationKey* pkKey0, const NiAnimationKey* pkKey1, void* pResult);
	// The curvature of a key sequence without one: 0. The position keys' tables hold it too.
	static float ComputeMaxCurvature(NiAnimationKey* pkKeys, uint32_t uiNumKeys);

	bool m_bBool;

	static inline bool ms_bDefault;
};

// NiRegisterAnimationStream(NiBoolKey): a static of every file that includes this header, as the game does.
static int32_t NiBoolKeyregisterloaderreturn = NiBoolKey::RegisterLoader();
