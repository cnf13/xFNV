#include "Fallout Shared/TESEnchantableForm.h"

#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/EnchantmentItem.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "SDK/MSVC/rtti.h"

TESEnchantableForm::TESEnchantableForm() : BaseFormComponent() {
	TESEnchantableForm::InitializeDataComponent();
}

void TESEnchantableForm::InitializeDataComponent() {
	pFormEnchanting = nullptr;
	iAmountofEnchantment = 0;
	eCastingType = MagicSystem::CASTING_COUNT;
}

void TESEnchantableForm::CopyComponent(BaseFormComponent* apCopyFC) {
	TESEnchantableForm* pCopy = dynamic_cast<TESEnchantableForm*>(apCopyFC);
	if (pCopy) {
		pFormEnchanting = pCopy->pFormEnchanting;
		uint16_t Fatigue = pCopy->iAmountofEnchantment;
		iAmountofEnchantment = Fatigue;
		eCastingType = pCopy->GetCastingType();
	}
}

bool TESEnchantableForm::CompareComponent(BaseFormComponent* apCompareFC) {
	TESEnchantableForm* v6 = dynamic_cast<TESEnchantableForm*>(apCompareFC);
	if (!v6) {
		return (bool)1;
	}
	EnchantmentItem* v3 = TESEnchantableForm::GetFormEnchanting();
	if (v3 != v6->TESEnchantableForm::GetFormEnchanting()) {
		return (bool)1;
	}
	int32_t fatigue = (int32_t)iAmountofEnchantment;
	return (bool)((uint8_t)(((uint32_t)fatigue) != ((uint32_t)v6->iAmountofEnchantment)));
}

void TESEnchantableForm::InitItem(TESForm* apOwner) {
	if (pFormEnchanting) {
		uint32_t tempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pFormEnchanting));
		if (apOwner) {
			TESFile* File = apOwner->GetFile(-1);
			TESForm::AddCompileIndex(tempint, File);
		}
		TESForm* FormByNumericID = TESForm::GetFormByNumericID(tempint);
		pFormEnchanting = dynamic_cast<EnchantmentItem*>(FormByNumericID);
		if (!pFormEnchanting) {
			if (apOwner) {
				if (apOwner->GetFormEditorIDLength()) {
					const char* v6 = apOwner->GetFormEditorID();
					Warning("MASTERFILE: Unable to find enchanting (%08X) on owner object \"%s\".", tempint, v6);
				} else {
					int32_t CastingType = apOwner->iFormID;
					Warning("MASTERFILE: Unable to find enchanting (%08X) on owner object (%08X).", tempint,
						CastingType);
				}
			} else {
				Warning("MASTERFILE: Unable to find enchanting (%08X) on unknown owner.", tempint);
			}
		}
		if (pFormEnchanting) {
			if (apOwner) {
				if (!pFormEnchanting->IsValidEnchantForObject(apOwner)) {
					const char* v8 = apOwner->GetFormEditorID();
					int32_t v5 = apOwner->iFormID;
					const char* v4 = pFormEnchanting->GetFormEditorID();
					Warning("MASTERFILE: Enchanting (%08X) '%s' has invalid effect trigger type or effect for object "
							"(%08X) '%s'.",
						pFormEnchanting->iFormID, v4, v5, v8);
					pFormEnchanting = 0;
				}
			}
		}
	}
}

void TESEnchantableForm::SaveSubRecord() {
	if (pFormEnchanting) {
		TESForm::AddChunk(EITM_ID, pFormEnchanting->GetFormID());
	}
	if (iAmountofEnchantment) {
		TESForm::AddChunk(EAMT_ID, iAmountofEnchantment);
	}
}

void TESEnchantableForm::SetFormEnchantmentAmount(uint16_t aiAmount) {
	iAmountofEnchantment = aiAmount;
}

// The exe's shared one-field getter.
EnchantmentItem* TESEnchantableForm::GetFormEnchanting() {
	return pFormEnchanting;
}

void TESEnchantableForm::SetFormEnchanting(EnchantmentItem* apEnch) {
	pFormEnchanting = apEnch;
}
