#include "Fallout Shared/TESHealthForm.h"

#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "SDK/MSVC/rtti.h"

TESHealthForm::TESHealthForm() : BaseFormComponent() {
    TESHealthForm::InitializeDataComponent();
}

TESHealthForm::~TESHealthForm() {}

void TESHealthForm::InitializeDataComponent() {
    iHealth = 100;
}

void TESHealthForm::CopyComponent(BaseFormComponent* apCopyFC) {
    TESHealthForm* pCopy = dynamic_cast<TESHealthForm*>(apCopyFC);
    if (pCopy) {
        iHealth = pCopy->iHealth;
    }
}

bool TESHealthForm::CompareComponent(BaseFormComponent* apCompareFC) {
    TESHealthForm* pCompare = dynamic_cast<TESHealthForm*>(apCompareFC);
    return !pCompare || iHealth != pCompare->iHealth;
}

void TESHealthForm::SetFormHealth(uint32_t aiHealth) {
    iHealth = aiHealth;
}

uint32_t TESHealthForm::GetFormHealth(TESForm* apForm) {
    TESHealthForm* pHealthForm = TESHealthForm::GetFormAsHealthForm(apForm);
    if (pHealthForm) {
        return pHealthForm->GetFormHealth();
    }

    return 0;
}

// The health component of apForm when its type has one (armor, weapon, NPC or creature), else null.
TESHealthForm* TESHealthForm::GetFormAsHealthForm(TESForm* apForm) {
    TESHealthForm* pRetVal = nullptr;
    if (apForm) {
        switch (apForm->GetFormType()) {
            case ARMO_ID: {
                pRetVal = static_cast<TESObjectARMO*>(apForm);
                break;
            }
            case WEAP_ID: {
                pRetVal = static_cast<TESObjectWEAP*>(apForm);
                break;
            }
            case NPC__ID:
            case CREA_ID: {
                pRetVal = static_cast<TESActorBase*>(apForm);
                break;
            }
            default: {
                break;
            }
        }
    }
    return pRetVal;
}
