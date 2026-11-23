#include "Fallout/AI/Combat/CombatProcedureUseCombatItem.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMenu/IDebugText.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatProcedure.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/LowProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"

CombatProcedureUseCombatItem::CombatProcedureUseCombatItem(TESBoundObject* apItem) : pItem(apItem), bAnimating(false) {}

CombatProcedureUseCombatItem::~CombatProcedureUseCombatItem() {}

void CombatProcedureUseCombatItem::Update() {
    Actor* pActor = GetAttacker();
    CombatState* pCombatState = pCombatController->pCombatState;
    LowProcess* pProcess = static_cast<LowProcess*>(pActor->GetCurrentAIProcess());
    if (!pProcess->GetProcessLevel()) {
        Animation* pAnimation = pActor->GetAnimation();
        if (pAnimation && (!pAnimation->SpecialIdleDonePlaying() || pAnimation->IsIdleNotLoaded(nullptr))) {
            return;
        }
        if (!bAnimating) {
            pProcess = static_cast<LowProcess*>(pActor->GetCurrentAIProcess());
            static_cast<HighProcess*>(pProcess)->HighProcess::FindSpecialIdletoPlay(pActor, pItem, nullptr);
            bAnimating = true;
            return;
        }
    }
    pCombatState->UsedCombatItem(pItem);
    if (pItem->GetFormType() == 47) {
        pActor->DrinkPotion(static_cast<AlchemyItem*>(pItem), nullptr, true);
        pCombatState->SetInventoryDirty(true);
    }
    else {
        Warning("COMBAT: Unhandled form type %s in CombatProcedureUseCombatItem::Update()", pItem->GetFormTypeString());
    }
    SetFinished();
}

void CombatProcedureUseCombatItem::DisplayDebugText(
    IDebugText* apDebugText,
    DEBUGTEXT_LOCATION aeLocation,
    DEBUGTEXT_JUSTIFY aeJustify
) {
    if (bAnimating) {
        apDebugText->PrintLine(aeLocation, aeJustify, "ANIMATING");
    }
    if (pItem) {
        const char* pcName;
        if (pItem->GetFormEditorIDLength()) {
            pcName = pItem->GetFormEditorID();
        }
        else {
            pcName = TESFullName::GetFullName(pItem);
        }
        apDebugText->PrintLine(aeLocation, aeJustify, "Item: '%s' (%08X)", pcName, pItem->iFormID);
    }
}

CombatProcedure::COMBAT_PROCEDURES CombatProcedureUseCombatItem::GetType() {
    return CombatProcedure::USE_COMBAT_ITEM;
}

void CombatProcedureUseCombatItem::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    CombatProcedure::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveFormID(pItem, 0);
    apSaveGameBuffer->SaveDataEndian(&bAnimating, sizeof(bAnimating), 0);
}

void CombatProcedureUseCombatItem::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::LoadGame(apLoadGameBuffer);
    pItem = dynamic_cast<TESBoundObject*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bAnimating), sizeof(bAnimating));
}

void CombatProcedureUseCombatItem::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::InitLoadGame(apLoadGameBuffer);
}
