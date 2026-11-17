#include "Fallout Shared/BGSPickupPutdownSounds.h"

#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESSound.h"
#include "SDK/MSVC/rtti.h"

BGSPickupPutdownSounds::BGSPickupPutdownSounds() {
	BGSPickupPutdownSounds::InitializeDataComponent();
}

void BGSPickupPutdownSounds::InitializeDataComponent() {
	pPickupSound = nullptr;
	pPutdownSound = nullptr;
}

void BGSPickupPutdownSounds::CopyComponent(BaseFormComponent* apCopyFC) {
	BGSPickupPutdownSounds* pcopy = dynamic_cast<BGSPickupPutdownSounds*>(apCopyFC);
	if (pcopy) {
		pPickupSound = pcopy->pPickupSound;
		pPutdownSound = pcopy->pPutdownSound;
	}
}

bool BGSPickupPutdownSounds::CompareComponent(BaseFormComponent* apCompareFC) {
	void* v6 = FNVD_DYNAMIC_CAST_VOID(BGSPickupPutdownSounds)(apCompareFC, RTTI_BaseFormComponent,
		RTTI_BGSPickupPutdownSounds);
	if (!v6) {
		return (bool)1;
	}
	BGSPickupPutdownSounds* pCompare = static_cast<BGSPickupPutdownSounds*>(v6);
	TESSound* pPickup = GetPickupSound();
	uint8_t result = 1;
	bool vCondition1;
	vCondition1 = pPickup == pCompare->GetPickupSound();
	if (vCondition1) {
		TESSound* pPutdown = GetPutdownSound();
		if (pPutdown == pCompare->GetPutdownSound()) {
			return (bool)0;
		}
	}
	return (bool)result;
}

void BGSPickupPutdownSounds::InitItem(TESForm* apOwner) {
	TESFile* pownerFile;
	uint32_t itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pPickupSound));
	if (apOwner) {
		pownerFile = apOwner->GetFile(-1);
	} else {
		pownerFile = 0;
	}
	if (itempInt) {
		TESForm::AddCompileIndex(itempInt, pownerFile);
		TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempInt);
		pPickupSound = dynamic_cast<TESSound*>(FormByNumericID);
		if (!pPickupSound) {
			if (apOwner) {
				if (apOwner->GetFormEditorIDLength()) {
					const char* v4 = apOwner->GetFormEditorID();
					Warning("MASTERFILE: Unable to find pickup sound (%08X) for owner object \"%s\".", itempInt, v4);
				} else {
					int32_t CastingType = apOwner->iFormID;
					Warning("MASTERFILE: Unable to find pickup sound  (%08X) for owner object (%08X).", itempInt,
						CastingType);
				}
			} else {
				Warning("MASTERFILE: Unable to find pickup sound  (%08X) for unknown owner.", itempInt);
			}
		}
	}
	itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pPutdownSound));
	if (itempInt) {
		TESForm::AddCompileIndex(itempInt, pownerFile);
		TESForm* v3 = TESForm::GetFormByNumericID(itempInt);
		pPutdownSound = dynamic_cast<TESSound*>(v3);
		if (!pPutdownSound) {
			if (apOwner) {
				if (apOwner->GetFormEditorIDLength()) {
					const char* v6 = apOwner->GetFormEditorID();
					Warning("MASTERFILE: Unable to find putdown sound (%08X) for owner object \"%s\".", itempInt, v6);
				} else {
					int32_t v7 = apOwner->iFormID;
					Warning("MASTERFILE: Unable to find putdown sound  (%08X) for owner object (%08X).", itempInt, v7);
				}
			} else {
				Warning("MASTERFILE: Unable to find putdown sound  (%08X) for unknown owner.", itempInt);
			}
		}
	}
}

void BGSPickupPutdownSounds::Save() {
	if (pPickupSound) {
		TESForm::AddChunk(YNAM_ID, pPickupSound->iFormID);
	}
	if (pPutdownSound) {
		TESForm::AddChunk(ZNAM_ID, pPutdownSound->iFormID);
	}
}

TESSound* BGSPickupPutdownSounds::GetPutdownSound() {
	return pPutdownSound;
}

TESSound* BGSPickupPutdownSounds::GetPickupSound() {
	return pPickupSound;
}
