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
    iTouchSpellAnimation = (uint16_t)255;
}

void BGSTouchSpellForm::CopyComponent(BaseFormComponent* apCopyFC) {
    BGSTouchSpellForm* pCopy = dynamic_cast<BGSTouchSpellForm*>(apCopyFC);
    if (pCopy) {
        pFormTouchSpell = pCopy->pFormTouchSpell;
        uint16_t Fatigue = pCopy->iTouchSpellAnimation;
        SetFormTouchSpellAnimation(Fatigue);
    }
}

bool BGSTouchSpellForm::CompareComponent(BaseFormComponent* apCompareFC) {
    BGSTouchSpellForm* pCompare = dynamic_cast<BGSTouchSpellForm*>(apCompareFC);
    if (!pCompare) {
        return 1;
    }
    MagicItemForm* FormTouchSpell = pFormTouchSpell;
    if (FormTouchSpell != pCompare->pFormTouchSpell) {
        return 1;
    }
    int32_t Fatigue = iTouchSpellAnimation;
    return Fatigue != pCompare->iTouchSpellAnimation;
}

void BGSTouchSpellForm::InitItem(TESForm* apOwner) {
    if (pFormTouchSpell) {
        EnchantmentItem* penchantment_2;
        uint32_t uitempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pFormTouchSpell));
        if (apOwner) {
            TESFile* File = apOwner->GetFile(-1);
            TESForm::AddCompileIndex(uitempint, File);
        }
        TESForm* FormByNumericID = TESForm::GetFormByNumericID(uitempint);
        pFormTouchSpell = dynamic_cast<MagicItemForm*>(FormByNumericID);
        if (!pFormTouchSpell) {
            if (apOwner) {
                if (apOwner->GetFormEditorIDLength()) {
                    const char* v6 = apOwner->GetFormEditorID();
                    Warning("MASTERFILE: Unable to find Actor Effect (%08X) on owner object \"%s\".", uitempint, v6);
                }
                else {
                    int32_t CastingType = apOwner->iFormID;
                    Warning(
                        "MASTERFILE: Unable to find Actor Effect (%08X) on owner object (%08X).", uitempint, CastingType
                    );
                }
            }
            else {
                Warning("MASTERFILE: Unable to find Actor Effect (%08X) on unknown owner.", uitempint);
            }
        }
        if (pFormTouchSpell) {
            MagicItemForm* penchantment;
            if (pFormTouchSpell->GetSpellType() == MagicSystem::ENCHANTMENT) {
                penchantment = pFormTouchSpell;
            }
            else {
                penchantment = 0;
            }
            penchantment_2 = static_cast<EnchantmentItem*>(penchantment);
        }
        else {
            penchantment_2 = 0;
        }
        if (penchantment_2) {
            if (apOwner) {
                if (!penchantment_2->IsValidEnchantForObject(apOwner)) {
                    const char* v8 = apOwner->GetFormEditorID();
                    int32_t v5 = apOwner->iFormID;
                    const char* v4 = pFormTouchSpell->GetFormEditorID();
                    Warning(
                        "MASTERFILE: Enchanting (%08X) '%s' has invalid effect trigger type or effect for object "
                        "(%08X) '%s'.",
                        pFormTouchSpell->iFormID, v4, v5, v8
                    );
                    pFormTouchSpell = 0;
                }
            }
        }
    }
}

void BGSTouchSpellForm::SaveSubRecord() {
    uint32_t v2;
    if (pFormTouchSpell) {
        v2 = pFormTouchSpell->TESForm::GetFormID();
        TESForm::AddChunk(static_cast<CHUNK_ID>(1297369413), v2);
    }
    if (iTouchSpellAnimation) {
        TESForm::AddChunk(static_cast<CHUNK_ID>(1414349125), iTouchSpellAnimation);
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
