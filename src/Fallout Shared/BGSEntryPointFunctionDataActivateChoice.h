#pragma once

#include "Fallout Shared/BGSEntryPointFunctionData.h"
#include "Fallout Shared/Script.h"

// Entry point function data for an activate choice: the label, the script it runs and its flags.
class BGSEntryPointFunctionDataActivateChoice : public BGSEntryPointFunctionData {
public:
    enum ACTIVATE_CHOICE_FLAGS : uint16_t {
        FLAG_RUN_IMMEDIATELY = 0x1,
    };

    BGSEntryPointFunctionDataActivateChoice();
    ~BGSEntryPointFunctionDataActivateChoice() override;

    ENTRY_POINT_FUNCTION_DATA GetType() override;
    BGSEntryPointFunctionData* Duplicate() override;
    bool Compare(BGSEntryPointFunctionData* apCompare) override;
    void Save() override;
    bool Load(TESFile* apFile) override;
    void InitItem(TESForm* apOwner) override;

    void InitializeData();
    // Frees the label and stores a copy of apLabel, or null.
    void SetLabel(const char* apLabel);
    // Returns &ActivateChoiceScript.
    Script* GetScript();
    // Whether the script runs at once instead of offering the choice.
    uint32_t GetRunImmediately();

    char* pActivateChoiceLabel;
    Script ActivateChoiceScript;
    uint16_t cFlags;
};
