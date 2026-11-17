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
    uint8_t formType;
    BGSClipRoundsForm* v4 = dynamic_cast<BGSClipRoundsForm*>(apCopyFC);
    if (v4) {
        formType = v4->GetFormClipRounds();
        cClipRounds = formType;
    }
}

bool BGSClipRoundsForm::CompareComponent(BaseFormComponent* apCompareFC) {
    BGSClipRoundsForm* pCompare = dynamic_cast<BGSClipRoundsForm*>(apCompareFC);
    if (!pCompare) {
        return true;
    }
    int32_t FormType = cClipRounds;
    return FormType != pCompare->cClipRounds;
}

int32_t BGSClipRoundsForm::GetFormClipRounds() {
    return cClipRounds;
}
