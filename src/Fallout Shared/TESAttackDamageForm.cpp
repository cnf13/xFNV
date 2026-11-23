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
        uint16_t usDamage = pCopy->GetAttackDamage();
        SetDamage(usDamage);
    }
}

bool TESAttackDamageForm::CompareComponent(BaseFormComponent* apCompareFC) {
    TESAttackDamageForm* pCompare = dynamic_cast<TESAttackDamageForm*>(apCompareFC);
    if (!pCompare) {
        return true;
    }
    int32_t iDamage = GetAttackDamage();
    return static_cast<uint32_t>(iDamage) != static_cast<uint32_t>(pCompare->GetAttackDamage());
}

uint16_t TESAttackDamageForm::GetAttackDamage() const {
    return sAttackDamage;
}

void TESAttackDamageForm::SetDamage(uint16_t asDamage) {
    sAttackDamage = asDamage;
}

uint16_t TESAttackDamageForm::GetAttackDamage_0(TESForm* apForm) {
    TESAttackDamageForm* pAttackDamageForm = dynamic_cast<TESAttackDamageForm*>(apForm);
    if (pAttackDamageForm) {
        return pAttackDamageForm->GetAttackDamage();
    }

    return 0;
}
