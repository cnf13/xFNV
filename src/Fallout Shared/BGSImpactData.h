#pragma once

#include "BSCore/NewOverloads.h"
#include "Fallout Shared/DecalData.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"

class BGSTextureSet;
class TESSound;

enum SOUND_LEVEL : int32_t;

// An impact data form (IPCT): the effect, decal and sounds of a hit on one material.
class BGSImpactData : public TESForm, public TESModel {
public:
    enum ORIENTATION : int32_t {
        ORIENT_SURFACE_NORMAL = 0,
        ORIENT_PROJ_VECTOR = 1,
        ORIENT_PROJ_REFLECT = 2,
        ORIENT_COUNT = 3,
    };

    // The impact's data as older files store it, the decal data included.
    struct OLD_IMPACT_DATA_DATA {
        // Swaps every multi-byte field read from a big-endian file.
        void Endian();

        float fDecalMinWidth;
        float fEffectDuration;
        float fParallaxScale;
        ORIENTATION eOrient;
        uint8_t cFlags;
        uint8_t cParallaxPasses;
        float fDecalMaxWidth;
        float fDepth;
        float fDecalMinHeight;
        float fDecalMaxHeight;
        float fShininess;
        float fAngleThreshold;
        float fPlacementRadius;
        uint32_t iColor;
        SOUND_LEVEL eSoundLevel;
    };

    // The impact's effect settings (DATA).
    struct IMPACT_DATA_DATA {
        enum IMPACT_FLAGS : uint8_t {
            FLAG_USE_TEXTURE_SET_DECAL_DATA = 0x1,
        };

        // Swaps the five fields read from a big-endian file, in field order.
        void Endian();

        float fEffectDuration;
        ORIENTATION eOrient;
        float fAngleThreshold;
        float fPlacementRadius;
        SOUND_LEVEL eSoundLevel;
        uint8_t cFlags;
        uint8_t cResultOverride;
    };

    BGSImpactData();
    ~BGSImpactData() override;

    void InitializeData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    using TESForm::Save;

    bool GetUseTextureSetDecalData();
    float GetDecalShininess();
    float GetDecalParallaxScale();
    uint32_t GetDecalColor();
    float GetDecalDepth();
    bool GetIsAlphaBlend();
    uint8_t GetParallaxPasses();
    // The game setting value of the impact's sound level; 0 for an unknown level.
    int32_t GetSoundLevelValue();
    // Copies the texture set's decal data (or the default one) into DData when the impact uses
    // the texture set's decals.
    void CopyDecalDataFromTextureSet();
    double GetMinWidth();
    float GetDecalMaxWidth();
    BGSTextureSet* GetDecalTextureSet();
    bool GetIsParallax();
    bool GetIsAlphaTest();
    // The decal placement radius.
    float GetPlacementRadius();
    float GetDecalAngleThreshold();
    float GetEffectDuration();
    // pSound1.
    TESSound* GetSound1();
    // pSound2.
    TESSound* GetSound2();

    void SetDecalTextureSet(BGSTextureSet* apSet);
    void SetSound1(TESSound* apSound);
    void SetSound2(TESSound* apSound);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    IMPACT_DATA_DATA Data;
    BGSTextureSet* pDecalTextureSet;
    TESSound* pSound1;
    TESSound* pSound2;
    DecalData DData;
};
