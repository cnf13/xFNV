#pragma once

#include "Fallout/Magic/MagicHitEffect.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameSubBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class ActiveEffect;
class NiNode;
class TESObjectREFR;

class MagicModelHitEffect : public MagicHitEffect {
public:
    MagicModelHitEffect();
    MagicModelHitEffect(TESObjectREFR* apTarget, ActiveEffect* apEffect);
    MagicModelHitEffect(TESObjectREFR* apTarget, const char* apFilename, float afTime);
    ~MagicModelHitEffect() override;

    const NiRTTI* GetRTTI() const override;
    bool Update(float) override;
    TEMP_EFFECT_TYPE GetType() override;
    bool CheckShouldSave() override;
    uint16_t GetSaveSize() override;
    uint16_t GetSaveSize(ActiveEffect*, TESObjectREFR*) override;
    void SaveGame() override;
    void SaveGame(BGSSaveGameBuffer*) override;
    void SaveGame(ActiveEffect*, TESObjectREFR*) override;
    bool LoadGame() override;
    void LoadGame(BGSLoadGameBuffer*) override;
    void LoadGame(ActiveEffect*, TESObjectREFR*) override;
    void InitLoadGame(BGSLoadGameBuffer*) override;
    void InitLoadGame(ActiveEffect*, TESObjectREFR*) override;
    void FinishInitLoadGame(ActiveEffect*, TESObjectREFR*, void*) override;
    void FinishLoadGame(BGSLoadGameBuffer*) override;
    bool Init() override;
    void UpdatePosition() override;

    bool bOwnPath;
    bool b3rdPerson;
    uint8_t padding2A[2];
    const char* pFilename;
    NiPointer<NiNode> spObject;
    NiPointer<NiRefObject> spTarget;
    // Constructed with BGSLoadGameSubBuffer's default constructor; the destructor and FinishLoadGame call DeleteBuffer
    // and Create on it.
    BGSLoadGameSubBuffer LoadedDataSubBuffer;

    static NiRTTI ms_RTTI;
};
