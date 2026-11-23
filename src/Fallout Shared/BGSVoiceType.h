#pragma once

#include "BSCore/BSStringT.h"
#include "Fallout Shared/TESForm.h"

// A voice type's DNAM data: its flags (the sex bit among them).
struct VOICE_TYPE_DATA {
    uint8_t cFlags;
};

// A voice type form (VTYP).
class BGSVoiceType : public TESForm {
public:
    BGSVoiceType();

    ~BGSVoiceType() override;

    bool Load(TESFile* apFile) override;
    void Save() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    const char* GetFormEditorID() override;
    bool SetFormEditorID(const char* apEditorID) override;

    using TESForm::Save;

    VOICE_TYPE_DATA Data;
    BSStringT<char> strEditorID;
};
