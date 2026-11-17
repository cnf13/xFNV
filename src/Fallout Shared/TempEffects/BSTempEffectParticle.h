#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"

class BGSImpactData;
class NiAVObject;
class NiNode;

class BSTempEffectParticle : public BSTempEffect {
public:
    BSTempEffectParticle(TESObjectCELL* apCell, float afLifetime, const char* apFilename, NiAVObject* apParent);
    ~BSTempEffectParticle() override;

    const NiRTTI* GetRTTI() const override;
    void Detach() override;
    bool Update(float) override;
    TEMP_EFFECT_TYPE GetType() override;
    bool CheckShouldSave() override;
    uint16_t GetSaveSize() override;
    void SaveGame() override;
    bool LoadGame() override;
    using BSTempEffect::LoadGame;
    using BSTempEffect::SaveGame;

    // Loads the particle file and attaches it below apSceneParent at aRotation, aPosition and afScale; aiFlags' bits 1,
    // 2 and 4 are tested on the full 32-bit value.
    void Init(NiNode* apSceneParent, NiMatrix3 aRotation, NiPoint3 aPosition, float afScale, uint32_t aiFlags);
    // Plays the impact data's two sounds at the particle model.
    void StartSounds();
    // The lifetime of apAVObject's controller.
    float GetControllerLifetime(NiAVObject* apAVObject);
    // Plays the particle model's sequence apcSequenceName.
    void PlaySequence(const char* apcSequenceName);
    // The particle model (spParticleObject).
    NiAVObject* GetParticleObject();
    // Returns the followed object (spFollowObject).
    NiAVObject* GetFollowObject();
    void StopSounds();
    void SetDecalImpactData(BGSImpactData* apObject);
    void ResetGravityRecursive(NiAVObject* apObject, NiTPrimitiveArray<NiAVObject*>& iArray, uint32_t& aiCount);
    void ResetGravity(NiAVObject* apObject);

    // Creates the particle effect apParticleFilename in apCell at aPosition facing aVector, for afLifetime seconds at
    // afScale, following apFollowObject when given (the NiPoint3 overload).
    static BSTempEffectParticle* Spawn(
        TESObjectCELL* apCell,
        float afLifetime,
        const char* apParticleFilename,
        NiPoint3 aVector,
        NiPoint3 aPosition,
        float afScale,
        uint32_t aiFlags,
        NiAVObject* apFollowObject
    );
    // The same with the effect's rotation given as aRotation.
    static BSTempEffectParticle* Spawn(
        TESObjectCELL* apCell,
        float afLifetime,
        const char* apParticleFilename,
        NiMatrix3 aRotation,
        NiPoint3 aPosition,
        float afScale,
        uint32_t aiFlags,
        NiAVObject* apFollowObject
    );

    NiPointer<NiAVObject> spParticleObject;
    const char* pFilename;
    NiPointer<NiAVObject> spFollowObject;
    NiTransform kFollowOffset;
    BGSImpactData* pDecalImpactData;
    BSSoundHandle Sound1;
    BSSoundHandle Sound2;

    static NiRTTI ms_RTTI;
};
