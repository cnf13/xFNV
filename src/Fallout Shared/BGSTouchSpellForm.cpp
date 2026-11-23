#include "Fallout Shared/BGSTouchSpellForm.h"

#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/EnchantmentItem.h"
#include "Fallout Shared/Magic/MagicItemForm.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "SDK/MSVC/rtti.h"

BGSTouchSpellForm::BGSTouchSpellForm() {
    BGSTouchSpellForm::InitializeDataComponent();
}

void BGSTouchSpellForm::InitializeDataComponent() {
    pFormTouchSpell = nullptr;
    iTouchSpellAnimation = 255;
}

void BGSTouchSpellForm::CopyComponent(BaseFormComponent* apCopyFC) {
    BGSTouchSpellForm* pCopy = dynamic_cast<BGSTouchSpellForm*>(apCopyFC);
    if (pCopy) {
        pFormTouchSpell = pCopy->pFormTouchSpell;
        SetFormTouchSpellAnimation(pCopy->iTouchSpellAnimation);
    }
}

bool BGSTouchSpellForm::CompareComponent(BaseFormComponent* apCompareFC) {
    BGSTouchSpellForm* pCompare = dynamic_cast<BGSTouchSpellForm*>(apCompareFC);
    if (!pCompare) {
        return true;
    }
    if (pFormTouchSpell != pCompare->pFormTouchSpell) {
        return true;
    }
    return iTouchSpellAnimation != pCompare->iTouchSpellAnimation;
}

void BGSTouchSpellForm::InitItem(TESForm* apOwner) {
    if (pFormTouchSpell) {
        uint32_t uiTempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pFormTouchSpell));
        if (apOwner) {
            TESFile* pFile = apOwner->GetFile(-1);
            TESForm::AddCompileIndex(uiTempInt, pFile);
        }
        TESForm* pForm = TESForm::GetFormByNumericID(uiTempInt);
        pFormTouchSpell = dynamic_cast<MagicItemForm*>(pForm);
        if (!pFormTouchSpell) {
            if (apOwner) {
                if (apOwner->GetFormEditorIDLength()) {
                    const char* pOwnerEditorID = apOwner->GetFormEditorID();
                    Warning(
                        "MASTERFILE: Unable to find Actor Effect (%08X) on owner object \"%s\".",
                        uiTempInt,
                        pOwnerEditorID
                    );
                }
                else {
                    int32_t iOwnerFormID = apOwner->iFormID;
                    Warning(
                        "MASTERFILE: Unable to find Actor Effect (%08X) on owner object (%08X).",
                        uiTempInt,
                        iOwnerFormID
                    );
                }
            }
            else {
                Warning("MASTERFILE: Unable to find Actor Effect (%08X) on unknown owner.", uiTempInt);
            }
        }
        EnchantmentItem* pEnchantment = nullptr;
        if (pFormTouchSpell && pFormTouchSpell->GetSpellType() == MagicSystem::ENCHANTMENT) {
            pEnchantment = static_cast<EnchantmentItem*>(pFormTouchSpell);
        }
        if (pEnchantment && apOwner && !pEnchantment->IsValidEnchantForObject(apOwner)) {
            const char* pOwnerEditorID = apOwner->GetFormEditorID();
            int32_t iOwnerFormID = apOwner->iFormID;
            const char* pSpellEditorID = pFormTouchSpell->GetFormEditorID();
            Warning(
                "MASTERFILE: Enchanting (%08X) '%s' has invalid effect trigger type or effect for object "
                "(%08X) '%s'.",
                pFormTouchSpell->iFormID,
                pSpellEditorID,
                iOwnerFormID,
                pOwnerEditorID
            );
            pFormTouchSpell = nullptr;
        }
    }
}

void BGSTouchSpellForm::SaveSubRecord() {
    if (pFormTouchSpell) {
        uint32_t uiFormID = pFormTouchSpell->TESForm::GetFormID();
        TESForm::AddChunk(EITM_ID, uiFormID);
    }
    if (iTouchSpellAnimation) {
        TESForm::AddChunk(EAMT_ID, iTouchSpellAnimation);
    }
}

void BGSTouchSpellForm::SetFormTouchSpell(MagicItemForm* apEnch) {
    pFormTouchSpell = apEnch;
}

void BGSTouchSpellForm::SetFormTouchSpellAnimation(uint16_t aiAnimGroup) {
    iTouchSpellAnimation = aiAnimGroup;
}

MagicItemForm* BGSTouchSpellForm::GetFormTouchSpell() {
    return pFormTouchSpell;
}

uint16_t BGSTouchSpellForm::GetFormTouchSpellAnimation() {
    return iTouchSpellAnimation;
}
