#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"

class NiColorKey : public NiAnimationKey {
public:
	NiColorKey();

	NiColorKey* GetKeyAt(uint32_t uiIndex, uint8_t ucSize) const {
		return static_cast<NiColorKey*>(NiAnimationKey::GetKeyAt(uiIndex, ucSize));
	}

	// Copies color into m_Color.
	void SetColor(const NiColorA& color);
	// Reads one key: the NiAnimationKey part, then the color.
	void LoadBinary(NiStream& stream);

	static uint8_t GetKeySize(KeyType eType) { return NiAnimationKey::GetKeySize(COLORKEY, eType); }
	// Value at fTime over uiNumKeys keys; uiLastIdx caches the key the previous call ended on. The key size is read as
	// the low byte of the int32_t size argument: GenInterpDefault passes its caller's whole 32-bit value on; uint8_t
	// would be zero-extended.
	static NiColorA GenInterp(float fTime, NiColorKey* pkKeys, KeyType eType, uint32_t uiNumKeys, uint32_t& uiLastIdx,
		int32_t ucSize);
	// The color at fTime over uiNumKeys keys without a cached index: ms_kDefault without keys, the first or last key's
	// color outside their times, else GenInterp from index 0. ucSize is int32_t because the caller's whole 32-bit value
	// is passed on; uint8_t would be zero-extended.
	static NiColorA GenInterpDefault(float fTime, NiColorKey* pkKeys, KeyType eType, uint32_t uiNumKeys,
		int32_t ucSize);
	// Whether key0 and key1 have the same time and color.
	static bool Equal(const NiAnimationKey& key0, const NiAnimationKey& key1);
	// Writes one key: the NiAnimationKey part, then the color.
	static void SaveBinary(NiStream& stream, NiAnimationKey* pkKey);
	// Fills NiAnimationKey's tables for NiColorKey's NOINTERP slot once; 1 the first time, else 0.
	static int32_t RegisterLoader();

	// The NOINTERP table functions, which assert(false) in Gamebryo: empty, or returning 0.
	static NiAnimationKey* CreateFromStream(NiStream&, uint32_t) { return nullptr; }
	static void SaveToStream(NiStream&, NiAnimationKey*, uint32_t) {}
	static void Copy(NiAnimationKey*, const NiAnimationKey*) {}
	static NiAnimationKey* CreateArray(uint32_t) { return nullptr; }
	static void DeleteArray(NiAnimationKey*) {}
	// Writes black to pResult.
	static void Interpolate(float fTime, const NiAnimationKey* pkKey0, const NiAnimationKey* pkKey1, void* pResult);

	NiColorA m_Color;

	static NiColorA ms_kDefault;
};

// NiRegisterAnimationStream(NiColorKey): a static of every file that includes this header, as the game does.
static int32_t NiColorKeyregisterloaderreturn = NiColorKey::RegisterLoader();
