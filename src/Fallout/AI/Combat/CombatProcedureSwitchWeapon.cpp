#include "Fallout/AI/Combat/CombatProcedureSwitchWeapon.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatProcedure.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"

CombatProcedureSwitchWeapon::CombatProcedureSwitchWeapon(TESObjectWEAP* apWeapon)
  : pWeapon(apWeapon), bEquipQueued(false) {}

CombatProcedureSwitchWeapon::~CombatProcedureSwitchWeapon() {}

void CombatProcedureSwitchWeapon::Update() {
    TESObjectWEAP* pcurrentWeapon;
    if (pCombatController->IsFleeing()) {
    LABEL_22:;
        SetFinished();
        return;
    }
    Actor* pattacker = GetAttacker();
    if (pattacker->GetAnimAction() != ANIM_ACTION_NONE) {
        return;
    }
    BaseProcess* CurrentAIProcess = pattacker->pCurrentProcess;
    ItemChange* pcurrentWeaponItem = CurrentAIProcess->GetCurrentWeapon();
    if (pcurrentWeaponItem) {
        pcurrentWeapon = static_cast<TESObjectWEAP*>(pcurrentWeaponItem->pContainerObj);
    }
    else {
        pcurrentWeapon = 0;
    }
    if (bEquipQueued || pcurrentWeapon == pWeapon) {
        if (pcurrentWeapon != pWeapon) {
            return;
        }
        BaseProcess* v2 = pattacker->pCurrentProcess;
        if (!v2->GetWeaponDrawn()) {
            pattacker->SetWantWeaponDrawn(1);
        }
        if (!pcurrentWeapon || pcurrentWeapon->IsMeleeWeapon()) {
            pCombatController->InitPowerAttackDistances();
        }
        goto LABEL_22;
    }
    BaseProcess* v3 = pattacker->pCurrentProcess;
    if (v3->GetWeaponDrawn()) {
        pattacker->SetWantWeaponDrawn(0);
    }
    else {
        if (pWeapon) {
            uint32_t icount = 1;
            if (pWeapon->IsThrownWeapon()) {
                icount = pattacker->GetInventoryObjectCount(pWeapon);
            }
            pattacker->QueueEquipObject(pWeapon, static_cast<int32_t>(icount), 0, 1, 0, 1);
        }
        else {
            uint32_t FormHealth_0 = pcurrentWeaponItem->iNumber;
            pattacker->QueueUnEquipObject(pcurrentWeapon, FormHealth_0, 0, 0, 0, 1);
        }
        bEquipQueued = 1;
    }
}

CombatProcedure::COMBAT_PROCEDURES CombatProcedureSwitchWeapon::GetType() {
    return SWITCH_WEAPON;
}

// The procedure's data, the weapon's form ID, then whether the equip is queued.
void CombatProcedureSwitchWeapon::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    CombatProcedure::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveFormID(pWeapon, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bEquipQueued), 1, 0);
}

void CombatProcedureSwitchWeapon::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::LoadGame(apLoadGameBuffer);
    pWeapon = dynamic_cast<TESObjectWEAP*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bEquipQueued), 1);
}

void CombatProcedureSwitchWeapon::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::InitLoadGame(apLoadGameBuffer);
}
