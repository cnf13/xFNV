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
    pCanopyShadowTexture->Save(ICON_ID);
}

REGION_DATA_ID TESRegionDataLandscape::GetID() const {
    return REGION_DATA_LANDSCAPE_ID;
}

TESRegionData* TESRegionDataLandscape::Copy() {
    return new TESRegionDataLandscape(this);
}

// A copy of this data with apRegionData blended into it.
TESRegionData* TESRegionDataLandscape::Blend(const TESRegionData* apRegionData) {
    TESRegionData* pCopy = Copy();
    pCopy->BlendInto(apRegionData, 2u);
    return pCopy;
}

void TESRegionDataLandscape::BlendInto(const TESRegionData* apRegionData, uint32_t aiTotalBlending) {
    BSStringT<char> strtemp;
    if (apRegionData && apRegionData->GetID() == REGION_DATA_LANDSCAPE_ID && aiTotalBlending) {
        // The source's downcast: GetID says apRegionData is landscape data.
        const TESRegionDataLandscape* pSource = static_cast<const TESRegionDataLandscape*>(apRegionData);
        if (!bIgnore) {
            if (apRegionData->bIgnore) {
                return;
            }
            if (bOverride) {
                if (apRegionData->bOverride) {
                    int32_t iPriority = apRegionData->cPriority;
                    if (iPriority > cPriority) {
                        SetOverride(apRegionData->bOverride);
                        SetPriority(apRegionData->cPriority);
                        pSource->GetCanopyShadowTexture(&strtemp);
                        SetCanopyShadowTexture(&strtemp);
                    }
                }
                return;
            }
            if (!apRegionData->bOverride) {
                if (apRegionData->cPriority > cPriority) {
                    pSource->GetCanopyShadowTexture(&strtemp);
                    SetCanopyShadowTexture(&strtemp);
                }
                float fPartialSum = static_cast<float>(
                    apRegionData->cPriority * (100 - cPriority) + cPriority * cPriority
                );
                float fSum = static_cast<double>(
                                 apRegionData->cPriority * apRegionData->cPriority +
                                 cPriority * (100 - apRegionData->cPriority)
                             ) +
                    fPartialSum;
                float fnewPriority = fSum * 0.004999999888241291;
                SetPriority(static_cast<int32_t>(std::floor(fnewPriority)));
                return;
            }
        }
        SetOverride(apRegionData->bOverride);
        SetPriority(apRegionData->cPriority);
        pSource->GetCanopyShadowTexture(&strtemp);
        SetCanopyShadowTexture(&strtemp);
    }
}

bool TESRegionDataLandscape::Validate() {
    return pCanopyShadowTexture && pCanopyShadowTexture->GetTextureNameLength();
}

bool TESRegionDataLandscape::LoadRegionDataLandscape(TESFile* apFile) {
    if (!apFile) {
        return false;
    }
    if (apFile->GetTESChunk() == 1313817417) {
        TESTexture::LoadTextureChunk(pCanopyShadowTexture, apFile);
    }
    return true;
}

void TESRegionDataLandscape::GetCanopyShadowTexture(BSStringT<char>* apTextureName) const {
    if (pCanopyShadowTexture) {
        apTextureName->Set(pCanopyShadowTexture->GetTextureName(), 0u);
    }
    else {
        apTextureName->Set("", 0u);
    }
}

bool TESRegionDataLandscape::SetCanopyShadowTexture(BSStringT<char>* apTextureName) {
    if (!pCanopyShadowTexture ||
        (pCanopyShadowTexture->GetTextureNameLength() &&
         !BSstricmp(pCanopyShadowTexture->GetTextureName(), apTextureName->pString))) {
        return false;
    }
    pCanopyShadowTexture->SetTextureName(apTextureName->pString);
    return true;
}

NiTexturingProperty* TESRegionDataLandscape::GetDefaultFullMaskTextureProp() {
    NiPixelData* ppixeldata;
    if (TESRegionDataLandscape::spDefaultFullMaskTextureProp.m_pObject) {
        return TESRegionDataLandscape::spDefaultFullMaskTextureProp.m_pObject;
    }
    ppixeldata = new NiPixelData(64u, 64u, NiPixelFormat::RGBA32, 1u, 1u);
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
    NiTexture::FormatPrefs myPrefs;
    // The name is a temporary of the whole assignment, built only when the allocation succeeded.
    TESRegionDataLandscape::spDefaultFullMaskTextureProp = new NiTexturingProperty(
        ppixeldata,
        NiFixedString("DefaultFullMaskTextureProp"),
        myPrefs
    );
    TESRegionDataLandscape::spDefaultFullMaskTextureProp->SetBaseClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
    return TESRegionDataLandscape::spDefaultFullMaskTextureProp.m_pObject;
}

void TESRegionDataLandscape::ClearDefaultTextureProps() {
    TESRegionDataLandscape::spDefaultFullMaskTextureProp = static_cast<NiTexturingProperty*>(nullptr);
    TESRegionDataLandscape::spDefaultEmptyMaskTextureProp = static_cast<NiTexturingProperty*>(nullptr);
}

// MarkAsChanged advances the revision ID once.
void NiPixelData::MarkAsChanged() {
    ++m_uiRevID;
}

NiTexture::FormatPrefs::FormatPrefs()
  : m_ePixelLayout(PIX_DEFAULT), m_eAlphaFmt(ALPHA_DEFAULT), m_eMipMapped(MIP_DEFAULT) {}

// Sets the base map's clamp mode, creating the base map when there is none.
void NiTexturingProperty::SetBaseClampMode(ClampMode eMode) {
    Map* pkBaseMap = m_kMaps[0];
    if (!pkBaseMap) {
        pkBaseMap = ::new Map;
        m_kMaps.SetAt(0, pkBaseMap);
    }
    pkBaseMap->SetClampMode(eMode);
}
