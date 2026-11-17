#include "Fallout Shared/DecalData.h"

#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"

#include <cstring>

DecalData DecalData::DefaultDecalData;

// ucFlags is left as the exe leaves it: unset.
DecalData::DecalData()
	: fDecalMinWidth(0x1p+3f), fDecalMaxWidth(0x1p+5f), fDecalMinHeight(0x1p+3f), fDecalMaxHeight(0x1p+5f),
	  fDepth(0x1p+5f), fShininess(0x1p+2f), fParallaxScale(1.0f), ucParallaxPasses(4), uiColor(0xFFFFFFFFu) {}

void DecalData::Copy(DecalData* apCopy) {
	if (apCopy == nullptr) {
		return;
	}

	// Natively harmless: the members are plain floats and bytes (no vtable, nothing owned).
	memcpy(static_cast<void*>(this), apCopy, sizeof(DecalData));
}

float DecalData::GetDecalMinWidth() {
	return fDecalMinWidth;
}

uint16_t DecalData::GetDecalParallaxPasses() {
	return ucParallaxPasses;
}

bool DecalData::IsParallax() {
	return ((ucFlags & 1) != 0);
}

bool DecalData::GetAlphaBlending() {
	return ((ucFlags & 2) != 0);
}

bool DecalData::GetAlphaTesting() {
	return ((ucFlags & 4) != 0);
}

bool DecalData::Compare(DecalData* apCompare) {
	return ((!apCompare) || (std::memcmp(this, apCompare, sizeof(DecalData)) != 0));
}

void DecalData::Save(CHUNK_ID aeChunk) {
	if (TESForm::QEndianSwapOnSave()) {
		reinterpret_cast<DECAL_DATA_DATA*>(this)->Endian();
	}
	TESForm::__AddChunkData(aeChunk, this, 0x24u);
	if (TESForm::QEndianSwapOnSave()) {
		reinterpret_cast<DECAL_DATA_DATA*>(this)->Endian();
	}
}

void DecalData::Load(TESFile* apFile) {
	apFile->GetChunkData(this, 0x24u);
	if (apFile->QEndian()) {
		reinterpret_cast<DECAL_DATA_DATA*>(this)->Endian();
	}
}

DecalData* DecalData::GetDefaultDecalData() {
	return &DefaultDecalData;
}

void DecalData::operator delete(void* apMemory) {
	ourFree(apMemory);
}

void DecalData::Destruct(DecalData* apThis) {}

// The sizes, then the parallax scale before the depth and shininess (the exe's order), then the colour. The
// parallax pass and flag bytes need no swap.
void DECAL_DATA_DATA::Endian() {
	ENDIAN(fDecalMinWidth, false);
	ENDIAN(fDecalMaxWidth, false);
	ENDIAN(fDecalMinHeight, false);
	ENDIAN(fDecalMaxHeight, false);
	ENDIAN(fParallaxScale, false);
	ENDIAN(fDepth, false);
	ENDIAN(fShininess, false);
	ENDIAN(iColor, false);
}
