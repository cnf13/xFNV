#pragma once

#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESObject.h"

// The leveled item form: a bound object whose leveled list picks the items it stands for. The leveled list base
// sits at +0x30, right after the bound object.
class TESLevItem : public TESBoundObject, public TESLeveledList {
public:
    TESLevItem();
    ~TESLevItem() override;

    bool Load(TESFile* apFile) override;
    void Save() override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;

    int32_t GetMaxLevelDifference() override;
    using TESForm::LoadGame;
    using TESForm::Revert;
    using TESForm::Save;
    using TESForm::SaveGame;
};
