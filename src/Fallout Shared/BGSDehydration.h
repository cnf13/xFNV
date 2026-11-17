#pragma once

#include "Fallout Shared/TESForm.h"

class BGSDehydrationStage;
class SpellItem;

// The dehydration a stage starts at and the sickness it gives.
struct BGSDehydrationStageData {
    // Byte swap of the form's data.
    void Endian();
    // Zeroes both fields.
    void Clear();

    uint32_t iH20;
    SpellItem* pSickness;
};

// One stage of hardcore dehydration (DEHY).
class BGSDehydrationStage : public TESForm {
public:
    BGSDehydrationStage();
    ~BGSDehydrationStage() override;

    void InitializeData() override;
    void ClearData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;

    using TESForm::Save;

    SpellItem* GetSickness();
    BGSDehydrationStageData* GetData();

protected:
    // Removes apStage from the sorted stages array.
    static void RemoveStage(BGSDehydrationStage* apStage);

public:
    // The highest stage whose dehydration afRads reaches, or null.
    static BGSDehydrationStage* FindStageForRadLevel(float afRads);
    // Inserts apStage into the sorted stages array.
    static int32_t AddStage(BGSDehydrationStage* apNewStage);
    // The stage at auiIndex, or null.
    static BGSDehydrationStage* GetStage(uint32_t auiIndex);
    static BGSDehydrationStage** AllocateStages(uint32_t aiStagesCount);
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    BGSDehydrationStageData data;

    static inline int32_t iNumStages;
    static inline BGSDehydrationStage** pStagesArray;
};
