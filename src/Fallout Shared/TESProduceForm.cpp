#include "Fallout Shared/TESProduceForm.h"

#include "BSCore/NewOverloads.h"
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
    TESProduceForm* pCompare = dynamic_cast<TESProduceForm*>(apCompareFC);
    if (!pCompare) {
        return true;
    }
    return pFormIngredient != pCompare->pFormIngredient ||
        std::memcmp(iProduceChance, pCompare->iProduceChance, sizeof(iProduceChance)) != 0;
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
        uint32_t uiTempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pFormIngredient));
        TESFile* pFile = apOwner->GetFile(-1);
        TESForm::AddCompileIndex(uiTempInt, pFile);
        TESForm* pForm = TESForm::GetFormByNumericID(uiTempInt);
        pFormIngredient = dynamic_cast<IngredientItem*>(pForm);
        if (!pFormIngredient) {
            if (apOwner) {
                if (apOwner->GetFormEditorIDLength()) {
                    const char* pEditorID = apOwner->GetFormEditorID();
                    Warning(
                        "MASTERFILE: Unable to find ingredient (%08X) on owner object \"%s\".",
                        uiTempInt,
                        pEditorID
                    );
                }
                else {
                    int32_t iOwnerFormID = apOwner->iFormID;
                    Warning(
                        "MASTERFILE: Unable to find ingredient (%08X) on owner object (%08X).",
                        uiTempInt,
                        iOwnerFormID
                    );
                }
            }
            else {
                Warning("MASTERFILE: Unable to find ingredient (%08X) on unknown owner.", uiTempInt);
            }
        }
    }
}
