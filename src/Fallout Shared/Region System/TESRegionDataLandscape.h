#pragma once

#include "BSCore/BSStringT.h"
#include "Fallout Shared/Region System/TESRegionData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"

class TESFile;
class TESTexture;

// The landscape data of a region: the texture its canopy shadows are cast with.
class TESRegionDataLandscape : public TESRegionData {
public:
    TESRegionDataLandscape();
    TESRegionDataLandscape(TESRegionDataLandscape* apSource);
    ~TESRegionDataLandscape() override;

    void Save() override;
    REGION_DATA_ID GetID() const override;
    TESRegionData* Copy() override;
    TESRegionData* Blend(const TESRegionData*) override;
    void BlendInto(const TESRegionData*, uint32_t) override;
    bool Validate() override;
    virtual bool LoadRegionDataLandscape(TESFile*);
    // Writes the canopy shadow texture's name to the string (the empty string when there is no texture).
    virtual void GetCanopyShadowTexture(BSStringT<char>*) const;
    // Renames the canopy shadow texture; false when there is none or its name is already the string's.
    virtual bool SetCanopyShadowTexture(BSStringT<char>*);

    // The shared texturing property of a 64x64 texture of random RGBA texels, named "DefaultFullMaskTextureProp";
    // made on first use.
    static NiTexturingProperty* GetDefaultFullMaskTextureProp();

    TESTexture* pCanopyShadowTexture;

    static NiPointer<NiTexturingProperty> spDefaultFullMaskTextureProp;
    static NiPointer<NiTexturingProperty> spDefaultEmptyMaskTextureProp;
};
