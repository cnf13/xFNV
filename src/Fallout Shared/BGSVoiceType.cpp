#include "Fallout Shared/BGSVoiceType.h"

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"

BGSVoiceType::BGSVoiceType() : Data{0} {
    SetFormType(0x5Du);
}

BGSVoiceType::~BGSVoiceType() {
    TESForm::InitializeDataComponent();
}

// Reads the record: the editor ID into a buffer of the chunk's own size, the bounds chunk through the vtable, and the
// flags.
bool BGSVoiceType::Load(TESFile* apFile) {
    if (apFile->GetTESForm() != VTYP_ID) {
        return false;
    }
    this->LoadForm(apFile);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        if (uiChunk == EDID_ID) {
            char* pEditorID = static_cast<char*>(__builtin_alloca(apFile->GetChunkSize()));
            apFile->GetChunkData(pEditorID, 0x200u);
            this->SetFormEditorID(pEditorID);
        }
        else if (uiChunk == OBND_ID) {
            this->LoadObjectBound(apFile);
        }
        else if (uiChunk == DNAM_ID) {
            apFile->GetChunkData(&Data, 1u);
        }
    }
    while (apFile->NextChunk());
    return true;
}

void BGSVoiceType::Save() {
    StartForm();
    if (TESForm::QEndianSwapOnSave()) {}
    TESForm::__AddChunkData(DNAM_ID, &Data, 1u);
    if (TESForm::QEndianSwapOnSave()) {}
    CloseForm();
}

void BGSVoiceType::Copy(TESForm* apCopy) {
    BGSVoiceType* v3 = dynamic_cast<BGSVoiceType*>(apCopy);
    if (v3) {
        Data.cFlags = (uint8_t)((int8_t)v3->Data.cFlags);
        TESForm::CopyAllComponents(apCopy);
    }
}

bool BGSVoiceType::Compare(TESForm* apCompare) {
    BGSVoiceType* v4 = dynamic_cast<BGSVoiceType*>(apCompare);
    if (!v4) {
        return true;
    }
    if (((int8_t)Data.cFlags) == ((int8_t)v4->Data.cFlags)) {
        return (((int8_t)TESForm::CompareAllComponents(v4)) != 0);
    }
    return true;
}

const char* BGSVoiceType::GetFormEditorID() {
    if (strEditorID.pString) {
        return strEditorID.pString;
    }
    else {
        return "";
    }
}

bool BGSVoiceType::SetFormEditorID(const char* apEditorID) {
    strEditorID.Set(apEditorID, 0u);
    return true;
}
