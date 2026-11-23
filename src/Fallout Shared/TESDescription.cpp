#include "Fallout Shared/TESDescription.h"

#include "BSCore/BSStringT.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

BSStringT<char> TESDescription::strCurrentDescription;

TESDescription::TESDescription() : BaseFormComponent() {
    TESDescription::InitializeDataComponent();
}

void TESDescription::InitializeDataComponent() {
    TESDescription::strCurrentDescription.Set(nullptr, 0);
    TESDescription::pCurrentDescription = nullptr;
    lFileOffset = 0;
}

void TESDescription::CopyComponent(BaseFormComponent* apCopyFC) {
    TESDescription* pCopy = dynamic_cast<TESDescription*>(apCopyFC);
    if (pCopy) {
        TESDescription::strCurrentDescription.Set(pCopy->GetDescription(nullptr, DESC_ID), 0u);
        lFileOffset = pCopy->lFileOffset;
    }
}

bool TESDescription::CompareComponent(BaseFormComponent* apCompareFC) {
    TESDescription* pCompare = dynamic_cast<TESDescription*>(apCompareFC);
    if (!pCompare) {
        return true;
    }
    if (!GetDescriptionLength() && !pCompare->GetDescriptionLength()) {
        return false;
    }
    uint32_t uiLength = GetDescriptionLength();
    if (uiLength != pCompare->GetDescriptionLength()) {
        return true;
    }
    const char* pDescription = pCompare->GetDescription(nullptr, DESC_ID);
    return TESDescription::strCurrentDescription.StrCmp(pDescription, false) != 0;
}

const char* TESDescription::GetDescription(TESForm* apForm, CHUNK_ID aeChunk) {
    if (!pDataHandler->GetLoadingFiles()) {
        TESForm* pForm = apForm;
        if (!apForm) {
            pForm = dynamic_cast<TESForm*>(this);
        }
        LoadDescription(pForm, aeChunk);
    }
    if (TESDescription::strCurrentDescription.pString) {
        return TESDescription::strCurrentDescription.pString;
    }

    return "";
}

// Walks the file the form came from for the aeChunk record and stores its text as the current description. An empty
// chunk leaves the buffer unwritten (malformed data), so one extra byte is reserved and terminated.
void TESDescription::LoadDescription(TESForm* apForm, CHUNK_ID aeChunk) {
    if (!apForm) {
        return;
    }
    // One description string is shared by the class: reuse it while it belongs to this component and holds text.
    if (TESDescription::pCurrentDescription == this && TESDescription::strCurrentDescription.GetLength()) {
        return;
    }
    TESDescription::pCurrentDescription = this;
    TESDescription::strCurrentDescription.Set(nullptr, 0);
    TESFile* pFile = apForm->GetFile(-1);
    if (!pFile || !lFileOffset) {
        return;
    }
    pFile = pFile->GetThreadSafeFile();
    if (!pFile->SetOffset(lFileOffset)) {
        return;
    }
    // The record the offset points at has to be the form's own type.
    if (static_cast<uint8_t>(pFile->GetTESForm()) != formEnumString[apForm->GetFormType()].cFormID) {
        return;
    }
    bool bFound = false;
    do {
        uint32_t uiChunk = pFile->GetTESChunk();
        if (uiChunk == aeChunk) {
            // A 16-byte aligned stack buffer of the chunk's size, read whole.
            uint32_t uiChunkSize = pFile->GetChunkSize();
            char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
            // An empty chunk leaves the buffer unwritten, so it is terminated first.
            pBuffer[0] = '\0';
            pFile->GetChunkData(pBuffer, 0);
            SetLastDescriptionString(pBuffer);
            bFound = true;
        }
    }
    while (!bFound && pFile->NextChunk());
}

bool TESDescription::SetLastDescriptionString(char* apText) {
    return TESDescription::strCurrentDescription.Set(apText, 0);
}

void TESDescription::SetDescriptionFileOffset(uint32_t alOffset) {
    lFileOffset = alOffset;
}

void TESDescription::Save() {
    TESDescription::SaveDescriptionData(this, DESC_ID);
}

uint32_t TESDescription::GetDescriptionLength() {
    return TESDescription::strCurrentDescription.GetLength();
}

// Writes the description as chunk aiChunk: its text and its length plus the terminator, or a single zero byte when the
// description is empty. The length is read twice.
void TESDescription::SaveDescriptionData(TESDescription* apDesc, CHUNK_ID aiChunk) {
    if (apDesc) {
        if (apDesc->GetDescriptionLength()) {
            uint32_t uiLength = apDesc->GetDescriptionLength() + 1;
            const char* pDescription = apDesc->GetDescription(nullptr, DESC_ID);
            TESForm::AddChunkArray(aiChunk, const_cast<char*>(pDescription), uiLength);
        }
        else {
            uint8_t ucZero = 0;
            TESForm::AddChunkArray(aiChunk, reinterpret_cast<char*>(&ucZero), 1);
        }
    }
}

void TESDescription::LoadDescriptionData(TESDescription* apDesc, TESFile* apFile) {
    if (apDesc && apFile) {
        apDesc->SetDescriptionFileOffset(apFile->GetOffset());
    }
}
