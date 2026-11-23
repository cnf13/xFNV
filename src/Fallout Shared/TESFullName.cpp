#include "Fallout Shared/TESFullName.h"

#include "BSCore/BSStringT.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

TESFullName::TESFullName() : BaseFormComponent(), cFullName() {}

void TESFullName::InitializeDataComponent() {
    cFullName.Set(nullptr, 0);
}

void TESFullName::ClearDataComponent() {}

void TESFullName::CopyComponent(BaseFormComponent* apCopyFC) {
    TESFullName* pCopy = dynamic_cast<TESFullName*>(apCopyFC);
    if (pCopy) {
        const char* pFullName = pCopy->TESFullName::GetFullName();
        cFullName.Set(pFullName, 0);
    }
}

bool TESFullName::CompareComponent(BaseFormComponent* apCompareFC) {
    TESFullName* pCompare = dynamic_cast<TESFullName*>(apCompareFC);
    if (!pCompare) {
        return true;
    }
    if (!GetFullNameLength() && !pCompare->GetFullNameLength()) {
        return false;
    }
    uint32_t uiFullNameLength = GetFullNameLength();
    if (uiFullNameLength != pCompare->GetFullNameLength()) {
        return true;
    }
    return cFullName.StrCmp(pCompare->GetFullName(), false) != 0;
}

void TESFullName::Save() {
    if (GetFullNameLength()) {
        TESForm::AddChunkArray(FULL_ID, GetFullName(), GetFullNameLength() + 1);
    }
}

uint32_t TESFullName::GetFullNameLength() {
    return cFullName.GetLength();
}

// Reads the FULL chunk of apFile into apFullName. An empty chunk clears the name.
void TESFullName::LoadFullNameChunk(TESFullName* apFullName, TESFile* apFile) {
    if (apFile && apFullName) {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == FULL_ID) {
            if (apFile->GetChunkSize()) {
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0);
                apFullName->SetFullName(pBuffer);
            }
            else {
                apFullName->SetFullName(nullptr);
            }
        }
    }
}
