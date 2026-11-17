#include "Fallout Shared/Region System/TESRegionDataLandscape.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/Region System/TESRegionData.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"

#include <cmath>

NiPointer<NiTexturingProperty> TESRegionDataLandscape::spDefaultFullMaskTextureProp;
NiPointer<NiTexturingProperty> TESRegionDataLandscape::spDefaultEmptyMaskTextureProp;

TESRegionDataLandscape::TESRegionDataLandscape() : TESRegionData(), pCanopyShadowTexture(new TESTexture()) {
	pCanopyShadowTexture->SetTextureName("Trees\\CanopyShadow.dds");
}

TESRegionDataLandscape::TESRegionDataLandscape(TESRegionDataLandscape* apSource)
	: TESRegionData(apSource), pCanopyShadowTexture(new TESTexture()) {
	pCanopyShadowTexture->SetTextureName(apSource->pCanopyShadowTexture->GetTextureName());
}

TESRegionDataLandscape::~TESRegionDataLandscape() {
	delete pCanopyShadowTexture;
}

void TESRegionDataLandscape::Save() {
	TESRegionData::Save();
	pCanopyShadowTexture->Save(static_cast<CHUNK_ID>(1313817417));
}

REGION_DATA_ID TESRegionDataLandscape::GetID() const {
	return REGION_DATA_LANDSCAPE_ID;
}

TESRegionData* TESRegionDataLandscape::Copy() {
	void* v4 = ourMalloc(sizeof(TESRegionDataLandscape));
	if (v4) {
		return ::new (v4) TESRegionDataLandscape(this);
	} else {
		return nullptr;
	}
}

// A copy of this data with apRegionData blended into it.
TESRegionData* TESRegionDataLandscape::Blend(const TESRegionData* apRegionData) {
	TESRegionData* pCopy = Copy();
	pCopy->BlendInto(apRegionData, 2u);
	return pCopy;
}

void TESRegionDataLandscape::BlendInto(const TESRegionData* a1, uint32_t a2) {
	BSStringT<char> strtemp;
	if (a1 && (a1->GetID() == REGION_DATA_LANDSCAPE_ID && a2)) {
		// The source's downcast: GetID says a1 is landscape data.
		const TESRegionDataLandscape* pSource = static_cast<const TESRegionDataLandscape*>(a1);
		if (bIgnore) {
			goto LABEL_6;
		}
		if (a1->bIgnore) {
			goto LABEL_17;
		}
		if (bOverride) {
			if (a1->bOverride) {
				int32_t iPriority = a1->cPriority;
				if (iPriority > cPriority) {
					SetOverride(a1->bOverride);
					uint8_t v5 = a1->cPriority;
					SetPriority(v5);
					pSource->GetCanopyShadowTexture(&strtemp);
					SetCanopyShadowTexture(&strtemp);
				}
			}
			goto LABEL_17;
		}
		if (a1->bOverride) {
		LABEL_6:;
			SetOverride(a1->bOverride);
			uint8_t v3 = a1->cPriority;
			SetPriority(v3);
			pSource->GetCanopyShadowTexture(&strtemp);
			SetCanopyShadowTexture(&strtemp);
			goto LABEL_17;
		}
		int32_t v6 = a1->cPriority;
		if (v6 > cPriority) {
			pSource->GetCanopyShadowTexture(&strtemp);
			SetCanopyShadowTexture(&strtemp);
		}
		int32_t v7 = cPriority;
		int32_t v8 = cPriority * v7;
		int32_t v9 = 100 - cPriority;
		float v15 = static_cast<float>(a1->cPriority * v9 + v8);
		int32_t v10 = 100 - a1->cPriority;
		int32_t v11 = cPriority * v10;
		int32_t v12 = a1->cPriority;
		float v16 = static_cast<double>(a1->cPriority * v12 + v11) + v15;
		float fnewPriority = v16 * 0.004999999888241291;
		int32_t v13 = static_cast<int32_t>(std::floor(fnewPriority));
		SetPriority(v13);
	}
LABEL_17:;
}

bool TESRegionDataLandscape::Validate() {
	return (bool)((uint8_t)(pCanopyShadowTexture && pCanopyShadowTexture->GetTextureNameLength()));
}

bool TESRegionDataLandscape::LoadRegionDataLandscape(TESFile* apFile) {
	if (!apFile) {
		return (bool)0;
	}
	if (apFile->GetTESChunk() == 1313817417) {
		TESTexture::LoadTextureChunk(pCanopyShadowTexture, apFile);
	}
	return (bool)1;
}

void TESRegionDataLandscape::GetCanopyShadowTexture(BSStringT<char>* a1) const {
	if (pCanopyShadowTexture) {
		a1->Set(pCanopyShadowTexture->GetTextureName(), 0u);
	} else {
		a1->Set("", 0u);
	}
}

bool TESRegionDataLandscape::SetCanopyShadowTexture(BSStringT<char>* a1) {
	if (!pCanopyShadowTexture ||
		(pCanopyShadowTexture->GetTextureNameLength() &&
			!BSstricmp(pCanopyShadowTexture->GetTextureName(), a1->pString))) {
		return 0;
	}
	pCanopyShadowTexture->SetTextureName(a1->pString);
	return 1;
}

NiTexturingProperty* TESRegionDataLandscape::GetDefaultFullMaskTextureProp() {
	NiTexturingProperty* v1;
	NiPixelData* ppixeldata;
	FNVD_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
	NiTexture::FormatPrefs myPrefs;
	char v4 = 0;
	if (TESRegionDataLandscape::spDefaultFullMaskTextureProp.m_pObject) {
		return TESRegionDataLandscape::spDefaultFullMaskTextureProp.m_pObject;
	}
	void* apMemory = NiMemObject::operator new(sizeof(NiPixelData));
	if (apMemory) {
		ppixeldata = ::new (apMemory) NiPixelData(64u, 64u, NiPixelFormat_RGBA32, 1u, 1u);
	} else {
		ppixeldata = 0;
	}
	uint8_t* ppixels = ppixeldata->GetPixels(0u, 0u);
	for (uint32_t iy = 0; iy < 64; ++iy) {
		for (uint32_t ix = 0; ix < 64; ++ix) {
			ppixels[256 * iy + 4 * ix] = static_cast<int32_t>(RandomFloat(0.0f, 255.0f));
			ppixels[256 * iy + 1 + 4 * ix] = static_cast<int32_t>(RandomFloat(0.0f, 255.0f));
			ppixels[256 * iy + 2 + 4 * ix] = static_cast<int32_t>(RandomFloat(0.0f, 255.0f));
			ppixels[256 * iy + 3 + 4 * ix] = static_cast<int32_t>(RandomFloat(0.0f, 255.0f));
		}
	}
	ppixeldata->MarkAsChanged();
	myPrefs.~FormatPrefs();
	::new (&myPrefs) NiTexture::FormatPrefs;
	NiTexturingProperty* v6 = static_cast<NiTexturingProperty*>(NiMemObject::operator new(sizeof(NiTexturingProperty)));
	if (v6) {
		NiFixedString* kName = ::new (static_cast<void*>(&apString)) NiFixedString("DefaultFullMaskTextureProp");
		v4 = 1;
		v1 = ::new (v6) NiTexturingProperty(ppixeldata, *kName, myPrefs);
	} else {
		v1 = 0;
	}
	NiRefObject* v7 = v1;
	TESRegionDataLandscape::spDefaultFullMaskTextureProp.operator=(v1);
	if ((v4 & 1) != 0) {
		apString.~NiFixedString();
	}
	TESRegionDataLandscape::spDefaultFullMaskTextureProp.m_pObject
		->SetBaseClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
	return TESRegionDataLandscape::spDefaultFullMaskTextureProp.m_pObject;
}

// MarkAsChanged advances the revision ID once.
void NiPixelData::MarkAsChanged() {
	++m_uiRevID;
}

NiTexture::FormatPrefs::FormatPrefs()
	: m_ePixelLayout(PIX_DEFAULT), m_eAlphaFmt(ALPHA_DEFAULT), m_eMipMapped(MIP_DEFAULT) {}

// Sets the base map's clamp mode, creating the base map when there is none.
void NiTexturingProperty::SetBaseClampMode(ClampMode eMode) {
	// SWEEP-INLINE 0x877A30
	Map* pkBaseMap = m_kMaps[0];
	if (!pkBaseMap) {
		void* pMemory = ourMalloc(sizeof(Map));
		pkBaseMap = pMemory ? ::new (pMemory) Map : nullptr;
		m_kMaps.SetAt(0, pkBaseMap);
	}
	pkBaseMap->SetClampMode(eMode);
}
