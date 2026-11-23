#include "Fallout Shared/BGSVoiceType.h"

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "xfnv/Compiler.h"

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
            char* pEditorID = static_cast<char*>(XFNV_ALLOCA(apFile->GetChunkSize()));
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
    BGSVoiceType* pCopy = dynamic_cast<BGSVoiceType*>(apCopy);
    if (pCopy) {
        Data.cFlags = pCopy->Data.cFlags;
        TESForm::CopyAllComponents(apCopy);
    }
}

bool BGSVoiceType::Compare(TESForm* apCompare) {
    BGSVoiceType* pCompare = dynamic_cast<BGSVoiceType*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (Data.cFlags == pCompare->Data.cFlags) {
        return TESForm::CompareAllComponents(pCompare);
    }
    return true;
}

const char* BGSVoiceType::GetFormEditorID() {
    if (strEditorID.pString) {
        return strEditorID.pString;
    }

    return "";
}

bool BGSVoiceType::SetFormEditorID(const char* apEditorID) {
    strEditorID.Set(apEditorID, 0u);
    return true;
}
