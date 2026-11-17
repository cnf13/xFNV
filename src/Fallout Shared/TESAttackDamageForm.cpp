#include "Fallout Shared/TESAttackDamageForm.h"

#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/TESForm.h"
#include "SDK/MSVC/rtti.h"

TESAttackDamageForm::TESAttackDamageForm() {
    TESAttackDamageForm::InitializeDataComponent();
}

TESAttackDamageForm::~TESAttackDamageForm() {}

void TESAttackDamageForm::InitializeDataComponent() {
    sAttackDamage = 0;
}

void TESAttackDamageForm::CopyComponent(BaseFormComponent* apCopyFC) {
    TESAttackDamageForm* pCopy = dynamic_cast<TESAttackDamageForm*>(apCopyFC);
    if (pCopy) {
        uint16_t v2 = pCopy->GetAttackDamage();
        SetDamage(v2);
    }
}

bool TESAttackDamageForm::CompareComponent(BaseFormComponent* apCompareFC) {
    TESAttackDamageForm* v5 = dynamic_cast<TESAttackDamageForm*>(apCompareFC);
    if (!v5) {
        return (bool)1;
    }
    int32_t v3 = (int32_t)GetAttackDamage();
    return (bool)((uint8_t)(((uint32_t)v3) != ((uint32_t)v5->GetAttackDamage())));
}

uint16_t TESAttackDamageForm::GetAttackDamage() const {
    return sAttackDamage;
}

void TESAttackDamageForm::SetDamage(uint16_t asDamage) {
    sAttackDamage = asDamage;
}

uint16_t TESAttackDamageForm::GetAttackDamage_0(TESForm* apForm) {
    TESAttackDamageForm* v2 = dynamic_cast<TESAttackDamageForm*>(apForm);
    if (v2) {
        return v2->GetAttackDamage();
    }
    else {
        return 0;
    }
}
