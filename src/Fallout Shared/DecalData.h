#pragma once

#include "Fallout Shared/TESFile.h"
#include "Reconstruction.hpp"

// The per-decal appearance settings; the nine values are the decal's texture ranges.
class DecalData {
public:
	DecalData();
	~DecalData() {}

	// Copies another decal's settings; a null source changes nothing.
	void Copy(DecalData* apCopy);

	// The getters below are shared one-field getters in the exe (thiscall, ret 0); the
	// float ones return in ST0.
	float GetDecalMinWidth();
	float GetDecalMaxWidth() { return fDecalMaxWidth; }
	float GetDecalMinHeight() { return fDecalMinHeight; }
	float GetDecalMaxHeight() { return fDecalMaxHeight; }
	float GetDecalDepth() { return fDepth; }
	float GetDecalShininess() { return fShininess; }
	float GetDecalParallaxScale() { return fParallaxScale; }
	uint32_t GetDecalColor() { return uiColor; }
	// The parallax passes and the flag bits 1 (parallax), 2 (alpha blending) and
	// 4 (alpha testing) of ucFlags.
	uint16_t GetDecalParallaxPasses();
	bool IsParallax();
	bool GetAlphaBlending();
	bool GetAlphaTesting();
	bool Compare(DecalData* apCompare);
	void Save(CHUNK_ID aeChunk);
	void Load(TESFile* apFile);

	// The engine's only DecalData instance; GetDefaultDecalData returns it.
	static DecalData* GetDefaultDecalData();
	static void operator delete(void* apMemory);
	// The exe's destructor is the shared empty function (thiscall, ret 0): a DecalData holds nothing to release.
	static void Destruct(DecalData* apThis);

	float fDecalMinWidth;          // 0x00
	float fDecalMaxWidth;          // 0x04
	float fDecalMinHeight;         // 0x08
	float fDecalMaxHeight;         // 0x0C
	float fDepth;                  // 0x10
	float fShininess;              // 0x14
	float fParallaxScale;          // 0x18
	uint8_t ucParallaxPasses;      // 0x1C
	uint8_t ucFlags;               // 0x1D
	uint8_t padding1E[2];          // 0x1E
	uint32_t uiColor;              // 0x20

	// The engine's default decal settings (GetDefaultDecalData returns it).
	static DecalData DefaultDecalData;
};

// A decal's settings as a record's DODT chunk stores them.
struct DECAL_DATA_DATA {
	// Swaps every multi-byte field read from a big-endian file.
	void Endian();

	float fDecalMinWidth;
	float fDecalMaxWidth;
	float fDecalMinHeight;
	float fDecalMaxHeight;
	float fDepth;
	float fShininess;
	float fParallaxScale;
	uint8_t cParallaxPasses;
	uint8_t cFlags;
	uint8_t padding1E[2];
	uint32_t iColor;
};
