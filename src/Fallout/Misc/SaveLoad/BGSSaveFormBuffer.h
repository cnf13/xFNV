#pragma once

#include "Fallout Shared/TESForm.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFormHeader.h"

// The buffer one form is saved into: the form's header and the form itself.
class BGSSaveFormBuffer : public BGSSaveGameBuffer {
public:
    BGSSaveFormBuffer();
    ~BGSSaveFormBuffer();

    TESForm* GetForm() override {
        return pForm;
    }
    TESObjectREFR* GetReference() override;
    Actor* GetActor() override;

    // The form's type, from Header.FormInfo.
    ENUM_FORM_ID GetFormType();

    // The form's change flags (Header.iChangeFlags).
    BGSChangeFlags GetChangeFlags();
    void SetHeader(uint32_t aiFormID, BGSChangeFlags aiChangeFlags, ENUM_FORM_ID aeFormType, uint8_t acVersion);
    void RemoveChange(int32_t aiFlags);
    uint32_t GetOverhead();
    // Sets Header.iChangeFlags = aiChangeFlags.
    void SetChangeFlags(BGSChangeFlags aiChangeFlags);
    void SetForm(TESForm* apForm);
    // Writes the header and the buffered data to apFile.
    void Save(BGSSaveLoadFile* apFile);
    void SaveMatrixAsQuaternion(NiMatrix3* apMatrix, int32_t aiSize, int32_t aiUnusuedMetaDataArg);

    BGSSaveLoadFormHeader Header;
    TESForm* pForm;
};
