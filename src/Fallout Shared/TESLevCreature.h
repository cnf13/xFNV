#pragma once

#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"

// The leveled creature form (LVLC): its leveled list picks the creature it stands for.
class TESLevCreature : public TESBoundObject, public TESLeveledList, public TESModelTextureSwap {
public:
    TESLevCreature();
    ~TESLevCreature() override;

    bool Load(TESFile* apFile) override;
    void Save() override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    void UnClone3D(TESObjectREFR*) override {}
    NiAVObject* Clone3D(TESObjectREFR*) override {
        return nullptr;
    }
    // TESLeveledList
    uint8_t GetChanceNone() override {
        return 0;
    }
    bool GetMultCalc() override {
        return false;
    }
    int32_t GetMaxLevelDifference() override;

    using TESBoundObject::Clone3D;
    using TESBoundObject::LoadGame;
    using TESBoundObject::Revert;
    using TESBoundObject::Save;
    using TESBoundObject::SaveGame;

    uint32_t pTemplate;
    static SettingT<GameSettingCollection> iLevCreaLevelDifferenceMax;
};
