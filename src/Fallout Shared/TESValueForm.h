#pragma once

#include "Fallout Shared/BaseFormComponent.h"

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class TESForm;

// The gold value of a form.
class TESValueForm : public BaseFormComponent {
public:
    TESValueForm();

    void InitializeDataComponent() override;
    void ClearDataComponent() override {}
    void CopyComponent(BaseFormComponent* apCopyFC) override;
    bool CompareComponent(BaseFormComponent* apCompareFC) override;

    // The size of the component's save game data for the change flags aiFlags.
    virtual uint16_t GetSaveSize(uint32_t aiFlags);
    virtual void SaveGame(uint32_t aiFlags);
    virtual void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags);

    // Saves the value when the save changed it (change flag 2).
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
    // Loads the value when the save changed it (change flag 2).
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
    // Stores aiValue and marks the owning form's value changed (change flag 2).
    void SetFormValue(int32_t aiValue);
    int32_t GetFormValue() {
        return iValue;
    }

    // The gold value of apForm: its own value, a magic item's or an alchemy item's cost, else -1.
    static int32_t GetFormValue(TESForm* apForm);

    uint32_t iValue;
};
