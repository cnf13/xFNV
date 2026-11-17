#include "Fallout Shared/TESProduceForm.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/IngredientItem.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

TESProduceForm::TESProduceForm() {
	TESProduceForm::InitializeDataComponent();
}

void TESProduceForm::InitializeDataComponent() {
	pFormIngredient = nullptr;
	ourMemset(iProduceChance, 0, 4);
}

void TESProduceForm::CopyComponent(BaseFormComponent* apCopyFC) {
	TESProduceForm* pCopy = dynamic_cast<TESProduceForm*>(apCopyFC);
	if (pCopy) {
		pFormIngredient = pCopy->pFormIngredient;
		SetAllProductionChances(pCopy->iProduceChance);
	}
}

bool TESProduceForm::CompareComponent(BaseFormComponent* apCompareFC) {
	TESProduceForm* pcompare = dynamic_cast<TESProduceForm*>(apCompareFC);
	if (!pcompare) {
		return 1;
	}
	IngredientItem* FormHealth_0 = pFormIngredient;
	return FormHealth_0 != pcompare->pFormIngredient || std::memcmp(iProduceChance, pcompare->iProduceChance, 4u) != 0;
}

void TESProduceForm::Save() {
	if (pFormIngredient) {
		uint32_t uiFormID = pFormIngredient->GetFormID();
		TESForm::AddChunk(PFIG_ID, uiFormID);
	}
	TESForm::AddChunkArray(PFPC_ID, reinterpret_cast<const char*>(iProduceChance), 4);
}

void TESProduceForm::SetAllProductionChances(const uint8_t* apiChances) {
	if (apiChances) {
		ourMemcpy(iProduceChance, apiChances, 4);
	}
}

void TESProduceForm::SetFormIngredient(IngredientItem* apIngr) {
	pFormIngredient = apIngr;
}

void TESProduceForm::InitItem(TESForm* apOwner) {
	if (pFormIngredient) {
		uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pFormIngredient));
		TESFile* File = apOwner->GetFile(-1);
		TESForm::AddCompileIndex(itempint, File);
		TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempint);
		pFormIngredient = dynamic_cast<IngredientItem*>(FormByNumericID);
		if (!pFormIngredient) {
			if (apOwner) {
				if (apOwner->GetFormEditorIDLength()) {
					const char* v4 = apOwner->GetFormEditorID();
					Warning("MASTERFILE: Unable to find ingredient (%08X) on owner object \"%s\".", itempint, v4);
				} else {
					int32_t CastingType = apOwner->iFormID;
					Warning("MASTERFILE: Unable to find ingredient (%08X) on owner object (%08X).", itempint,
						CastingType);
				}
			} else {
				Warning("MASTERFILE: Unable to find ingredient (%08X) on unknown owner.", itempint);
			}
		}
	}
}
