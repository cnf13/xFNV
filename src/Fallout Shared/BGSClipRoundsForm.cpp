#include "Fallout Shared/BGSClipRoundsForm.h"

#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "SDK/MSVC/rtti.h"

BGSClipRoundsForm::BGSClipRoundsForm() {
    BGSClipRoundsForm::InitializeDataComponent();
}

BGSClipRoundsForm::~BGSClipRoundsForm() {}

void BGSClipRoundsForm::InitializeDataComponent() {
    cClipRounds = 12;
}

void BGSClipRoundsForm::CopyComponent(BaseFormComponent* apCopyFC) {
    BGSClipRoundsForm* pCopy = dynamic_cast<BGSClipRoundsForm*>(apCopyFC);
    if (pCopy) {
        cClipRounds = pCopy->GetFormClipRounds();
    }
}

bool BGSClipRoundsForm::CompareComponent(BaseFormComponent* apCompareFC) {
    BGSClipRoundsForm* pCompare = dynamic_cast<BGSClipRoundsForm*>(apCompareFC);
    if (!pCompare) {
        return true;
    }
    int32_t iClipRounds = cClipRounds;
    return iClipRounds != pCompare->cClipRounds;
}

int32_t BGSClipRoundsForm::GetFormClipRounds() {
    return cClipRounds;
}
