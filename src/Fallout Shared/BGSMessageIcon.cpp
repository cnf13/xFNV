#include "Fallout Shared/BGSMessageIcon.h"

#include "BSCore/BSString.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESTexture.h"

BGSMessageIcon::BGSMessageIcon() {
    BGSMessageIcon::InitializeDataComponent();
}

BGSMessageIcon::~BGSMessageIcon() {
    BGSMessageIcon::ClearDataComponent();
}

void BGSMessageIcon::InitializeDataComponent() {
    // Calling through the base keeps the call virtual and stops clang from devirtualising it.
    static_cast<BaseFormComponent&>(Icon).InitializeDataComponent();
}

void BGSMessageIcon::ClearDataComponent() {
    Icon.ClearDataComponent();
}

void BGSMessageIcon::CopyComponent(BaseFormComponent* apCopyFC) {
    BGSMessageIcon* pCopy = dynamic_cast<BGSMessageIcon*>(apCopyFC);
    if (pCopy) {
        SetMessageIconTextureName(pCopy->GetMessageIconTextureName());
    }
}

bool BGSMessageIcon::CompareComponent(BaseFormComponent* apCompareFC) {
    BGSMessageIcon* pCompare = dynamic_cast<BGSMessageIcon*>(apCompareFC);
    if (!pCompare) {
        return true;
    }
    const char* pCompareName = pCompare->GetMessageIconTextureName();
    const char* pName = GetMessageIconTextureName();
    return BSstricmp(pName, pCompareName) != 0;
}

const char* BGSMessageIcon::GetMessageIconTextureName() {
    return Icon.GetTextureName();
}

void BGSMessageIcon::Save(CHUNK_ID aeTextureID) {
    if (Icon.GetTextureNameLength()) {
        TESForm::AddChunkArray(aeTextureID, Icon.GetTextureName(), Icon.GetTextureNameLength() + 1);
    }
}

void BGSMessageIcon::SetMessageIconTextureName(const char* apTexture) {
    Icon.TESTexture::SetTextureName(apTexture);
}

void BGSMessageIcon::LoadMessageIcon(BGSMessageIcon* apIcon, TESFile* apFile) {
    TESTexture::LoadTextureChunk(&apIcon->Icon, apFile);
}
