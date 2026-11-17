#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Reconstruction.hpp"

class NiColorKey : public NiAnimationKey {
public:
	NiColorKey();

	NiColorKey* GetKeyAt(uint32_t uiIndex, uint8_t ucSize) const {
		return static_cast<NiColorKey*>(NiAnimationKey::GetKeyAt(uiIndex, ucSize));
	}

	// Copies color into m_Color.
	void SetColor(const NiColorA& color);
	// Reads one key: the NiAnimationKey part, then the color (thiscall, ret 4, 0xA29640). The
	// exe folds NiRotKey::LoadBinary into it (the same code: a time, then four floats at +4); NiBezRotKey's and
	// NiTCBRotKey's loaders call it.
	void LoadBinary(NiStream& stream);

	static uint8_t GetKeySize(KeyType eType) { return NiAnimationKey::GetKeySize(COLORKEY, eType); }
	// Value at fTime over uiNumKeys keys; uiLastIdx caches the key the previous call ended on. The key size is read
	// as the low byte of iKeySize: GenInterpDefault passes its caller's whole dword on; uint8_t would be
	// zero-extended.
	static NiColorA GenInterp(float fTime, NiColorKey* pkKeys, KeyType eType, uint32_t uiNumKeys, uint32_t& uiLastIdx,
		int32_t ucSize);
	// The color at fTime over uiNumKeys keys without a cached index: ms_kDefault without keys, the first or last
	// key's color outside their times, else GenInterp from index 0 (cdecl, hidden result pointer). Protected in the
	// engine; public here so the replacement table can name it. The exe passes the caller's whole dword on; uint8_t
	// would be zero-extended.
	static NiColorA GenInterpDefault(float fTime, NiColorKey* pkKeys, KeyType eType, uint32_t uiNumKeys,
		int32_t ucSize);
	// Whether key0 and key1 have the same time and color.
	// cdecl, 0xA299C0.
	static bool Equal(const NiAnimationKey& key0, const NiAnimationKey& key1);
	// Writes one key: the NiAnimationKey part, then the color (cdecl, ret 0, 0xA29660). The
	// exe folds NiRotKey::SaveBinary into it as LoadBinary.
	static void SaveBinary(NiStream& stream, NiAnimationKey* pkKey);
	// Fills NiAnimationKey's tables for NiColorKey's NOINTERP slot once; 1 the first time, else 0.
	static int32_t RegisterLoader();

	// The PDB's NOINTERP table functions, which assert(false) in the source: empty, or returning 0.
	static NiAnimationKey* CreateFromStream(NiStream&, uint32_t) { return nullptr; }
	static void SaveToStream(NiStream&, NiAnimationKey*, uint32_t) {}
	static void Copy(NiAnimationKey*, const NiAnimationKey*) {}
	static NiAnimationKey* CreateArray(uint32_t) { return nullptr; }
	static void DeleteArray(NiAnimationKey*) {}
	// Not named in the PDB or IDA; name chosen here. The PDB's NiColorKey::Interpolate: black at pResult. The exe
	// leaves pResult in eax; the tables' callers ignore it.
	static void Interpolate(float fTime, const NiAnimationKey* pkKey0, const NiAnimationKey* pkKey1, void* pResult);

	NiColorA m_Color;

	static NiColorA ms_kDefault;
};

// NiRegisterAnimationStream(NiColorKey): a static of every file that includes this header, as in the original.
static int32_t NiColorKeyregisterloaderreturn = NiColorKey::RegisterLoader();
