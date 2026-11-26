#pragma once

#include "BSCore/NewOverloads.h"
#include "Fallout Shared/TESForm.h"

class SpellItem;

// The rads a radiation stage starts at and the sickness it gives.
struct BGSRadiationStageData {
    // Swaps both dwords, the sickness then holding a form ID.
    void Endian();
    // Zeroes both dwords.
    void Clear();

    uint32_t iRads;
    SpellItem* pSickness;
};

// One stage of radiation sickness (RADS).
class BGSRadiationStage : public TESForm {
public:
    BGSRadiationStage();
    ~BGSRadiationStage() override;

    void InitializeData() override;
    void ClearData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;

    using TESForm::Save;

    // The stage's record data.
    BGSRadiationStageData* GetData();
    SpellItem* GetSickness();

protected:
    // Inserts apStage into the sorted stages array.
    static void AddStage(BGSRadiationStage* apNewStage);
    // Removes apStage from the sorted stages array.
    static void RemoveStage(BGSRadiationStage* apStage);

public:
    // The stage at aiIndex in the sorted stage array, or null past its end.
    static BGSRadiationStage* GetStage(uint32_t aiIndex);
    static BGSRadiationStage** AllocateStages(uint32_t aiStagesCount);
    // The highest stage whose rads afRads reaches, or null.
    static BGSRadiationStage* FindStageForRadLevel(float afRads);
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    BGSRadiationStageData data;

    static inline int32_t iNumStages;
    // The stages sorted by rads, iNumStages of them, allocated by AddStage and RemoveStage.
    static inline BGSRadiationStage** pStagesArray;
};
