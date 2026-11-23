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
        iAmountofEnchantment = pCopy->iAmountofEnchantment;
        eCastingType = pCopy->GetCastingType();
    }
}

bool TESEnchantableForm::CompareComponent(BaseFormComponent* apCompareFC) {
    TESEnchantableForm* pCompare = dynamic_cast<TESEnchantableForm*>(apCompareFC);
    if (!pCompare) {
        return true;
    }
    EnchantmentItem* pEnchanting = TESEnchantableForm::GetFormEnchanting();
    if (pEnchanting != pCompare->TESEnchantableForm::GetFormEnchanting()) {
        return true;
    }
    return iAmountofEnchantment != pCompare->iAmountofEnchantment;
}

void TESEnchantableForm::InitItem(TESForm* apOwner) {
    if (pFormEnchanting) {
        uint32_t tempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pFormEnchanting));
        if (apOwner) {
            TESFile* pFile = apOwner->GetFile(-1);
            TESForm::AddCompileIndex(tempint, pFile);
        }
        TESForm* pForm = TESForm::GetFormByNumericID(tempint);
        pFormEnchanting = dynamic_cast<EnchantmentItem*>(pForm);
        if (!pFormEnchanting) {
            if (apOwner) {
                if (apOwner->GetFormEditorIDLength()) {
                    const char* pEditorID = apOwner->GetFormEditorID();
                    Warning("MASTERFILE: Unable to find enchanting (%08X) on owner object \"%s\".", tempint, pEditorID);
                }
                else {
                    int32_t iFormID = apOwner->iFormID;
                    Warning("MASTERFILE: Unable to find enchanting (%08X) on owner object (%08X).", tempint, iFormID);
                }
            }
            else {
                Warning("MASTERFILE: Unable to find enchanting (%08X) on unknown owner.", tempint);
            }
        }
        if (pFormEnchanting && apOwner && !pFormEnchanting->IsValidEnchantForObject(apOwner)) {
            const char* pOwnerEditorID = apOwner->GetFormEditorID();
            int32_t iOwnerFormID = apOwner->iFormID;
            const char* pEnchantingEditorID = pFormEnchanting->GetFormEditorID();
            Warning(
                "MASTERFILE: Enchanting (%08X) '%s' has invalid effect trigger type or effect for object "
                "(%08X) '%s'.",
                pFormEnchanting->iFormID,
                pEnchantingEditorID,
                iOwnerFormID,
                pOwnerEditorID
            );
            pFormEnchanting = nullptr;
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

EnchantmentItem* TESEnchantableForm::GetFormEnchanting() {
    return pFormEnchanting;
}

void TESEnchantableForm::SetFormEnchanting(EnchantmentItem* apEnch) {
    pFormEnchanting = apEnch;
}
