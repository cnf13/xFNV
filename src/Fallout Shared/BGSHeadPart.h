#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESModelTextureSwap.h"

// A head part form (HDPT).
class BGSHeadPart : public TESForm, public TESFullName, public TESModelTextureSwap {
public:
    BGSHeadPart();
    ~BGSHeadPart() override;

    void InitializeData() override;
    void ClearData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    using TESForm::Save;

    BSSimpleList<BGSHeadPart*>* GetExtraPartList();
    // Adds a part to the extra part list.
    void AddExtraPart(BGSHeadPart* apPart);
    bool IsPlayable();

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    uint8_t cFlags;
    uint8_t padding45[3];
    // The parts that come with this one.
    BSSimpleList<BGSHeadPart*> ExtraParts;
};
