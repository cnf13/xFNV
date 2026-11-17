#include "Fallout Shared/TESWeightForm.h"

#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/Magic/IngredientItem.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectBOOK.h"
#include "Fallout Shared/TESObjectCLOT.h"
#include "Fallout Shared/TESObjectCONT.h"
#include "Fallout Shared/TESObjectMISC.h"
#include "Fallout Shared/TESObjectWEAP.h"

TESWeightForm::TESWeightForm() {
	TESWeightForm::InitializeDataComponent();
}

void TESWeightForm::InitializeDataComponent() {
	fWeight = (float)0.0;
}

void TESWeightForm::CopyComponent(BaseFormComponent* apCopyFC) {
	TESWeightForm* pCopy = dynamic_cast<TESWeightForm*>(apCopyFC);
	if (pCopy) {
		SetFormWeight(pCopy->GetWeight());
	}
}

bool TESWeightForm::CompareComponent(BaseFormComponent* apCompareFC) {
	TESWeightForm* v5 = dynamic_cast<TESWeightForm*>(apCompareFC);
	if (!v5) {
		return (bool)1;
	}
	double flt004 = (double)GetWeight();
	return (bool)((uint8_t)(flt004 != v5->GetWeight()));
}

void TESWeightForm::SetFormWeight(float afWeight) {
	fWeight = afWeight;
}

float TESWeightForm::GetFormWeight(TESForm* apForm, bool abIsHardcore) {
	TESWeightForm* pweight = nullptr;
	float fWeightOverride = 0.0f;
	if (apForm) {
		switch (apForm->GetFormType()) {
		case ARMO_ID:
			pweight = static_cast<TESObjectARMO*>(apForm);
			break;
		case BOOK_ID:
			pweight = static_cast<TESObjectBOOK*>(apForm);
			break;
		case CLOT_ID:
			pweight = static_cast<TESObjectCLOT*>(apForm);
			break;
		case CONT_ID:
			pweight = static_cast<TESObjectCONT*>(apForm);
			break;
		case INGR_ID:
			pweight = static_cast<IngredientItem*>(apForm);
			break;
		case LIGH_ID:
			pweight = static_cast<TESObjectLIGH*>(apForm);
			break;
		// The exe takes the MISC weight offset for all four types (one jump table target).
		case MISC_ID:
		case KEYM_ID:
		case COBJ_ID:
		case IMOD_ID:
			pweight = static_cast<TESObjectMISC*>(apForm);
			break;
		case WEAP_ID:
			pweight = static_cast<TESObjectWEAP*>(apForm);
			break;
		case AMMO_ID:
			if (abIsHardcore) {
				// The exe calls the getter out of line (0x48EE50).
				fWeightOverride = static_cast<TESAmmo*>(apForm)->GetWeight();
			}
			break;
		case ALCH_ID:
			pweight = static_cast<AlchemyItem*>(apForm);
			break;
		default:
			break;
		}
	}
	// The exe's `test ah,0x44; jnp` jumps to the weight form's value only on an ordered equal compare with 0.0: a
	// NaN override is returned.
	if (fWeightOverride != 0.0) {
		return fWeightOverride;
	}
	if (pweight) {
		return pweight->GetWeight();
	}
	return -1.0f;
}
