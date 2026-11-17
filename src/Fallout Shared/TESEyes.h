#pragma once

#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESTexture.h"

class TESNPC;

// An eyes form (EYES).
class TESEyes : public TESForm, public TESFullName, public TESTexture {
public:
    TESEyes();

    ~TESEyes() override;

    void InitializeData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    using TESForm::Save;

    // Whether the player can pick these eyes.
    bool IsPlayable();
    // Whether apNPC's race and sex allow these eyes.
    bool CanBeEyesForNPC(TESNPC* apNPC);
    void SetPlayable(bool abVal);
    bool CanBeMale();
    void SetCanBeMale(bool abVal);
    bool CanBeFemale();
    void SetCanBeFemale(bool abVal);

    uint8_t cFlags;
    uint8_t padding31[3];
};
