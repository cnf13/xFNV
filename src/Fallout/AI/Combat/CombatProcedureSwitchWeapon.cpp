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
    TESObjectWEAP* pCurrentWeapon;
    if (!pCombatController->IsFleeing()) {
        Actor* pAttacker = GetAttacker();
        if (pAttacker->GetAnimAction() != ANIM_ACTION_NONE) {
            return;
        }
        ItemChange* pCurrentWeaponItem = pAttacker->pCurrentProcess->GetCurrentWeapon();
        if (pCurrentWeaponItem) {
            pCurrentWeapon = static_cast<TESObjectWEAP*>(pCurrentWeaponItem->pContainerObj);
        }
        else {
            pCurrentWeapon = nullptr;
        }
        if (!bEquipQueued && pCurrentWeapon != pWeapon) {
            if (pAttacker->pCurrentProcess->GetWeaponDrawn()) {
                pAttacker->SetWantWeaponDrawn(false);
            }
            else {
                if (pWeapon) {
                    uint32_t uiCount = 1;
                    if (pWeapon->IsThrownWeapon()) {
                        uiCount = pAttacker->GetInventoryObjectCount(pWeapon);
                    }
                    pAttacker->QueueEquipObject(pWeapon, static_cast<int32_t>(uiCount), nullptr, true, false, true);
                }
                else {
                    pAttacker
                        ->QueueUnEquipObject(pCurrentWeapon, pCurrentWeaponItem->iNumber, nullptr, false, false, true);
                }
                bEquipQueued = true;
            }
            return;
        }
        if (pCurrentWeapon != pWeapon) {
            return;
        }
        if (!pAttacker->pCurrentProcess->GetWeaponDrawn()) {
            pAttacker->SetWantWeaponDrawn(true);
        }
        if (!pCurrentWeapon || pCurrentWeapon->IsMeleeWeapon()) {
            pCombatController->InitPowerAttackDistances();
        }
    }
    SetFinished();
}

CombatProcedure::COMBAT_PROCEDURES CombatProcedureSwitchWeapon::GetType() {
    return SWITCH_WEAPON;
}

// The procedure's data, the weapon's form ID, then whether the equip is queued.
void CombatProcedureSwitchWeapon::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    CombatProcedure::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveFormID(pWeapon, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bEquipQueued), sizeof(bEquipQueued), 0);
}

void CombatProcedureSwitchWeapon::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::LoadGame(apLoadGameBuffer);
    pWeapon = dynamic_cast<TESObjectWEAP*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bEquipQueued), sizeof(bEquipQueued));
}

void CombatProcedureSwitchWeapon::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::InitLoadGame(apLoadGameBuffer);
}
