#include "Fallout/AI/Combat/CombatProcedureAttackLow.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSMenu/IDebugText.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/CombatProcedure.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

// fWeaponCondition is deliberately left unset.
CombatProcedureAttackLow::CombatProcedureAttackLow()
  : eState(CombatProcedureAttackLow::ATTACK_STATE_INITIALIZING), fWeaponDPS(0.0f), pWeapon(nullptr) {}

CombatProcedureAttackLow::~CombatProcedureAttackLow() {}

void CombatProcedureAttackLow::Update() {
    COMBAT_WEAPON_TYPE eWeaponType;
    Actor* pAttacker = GetAttacker();
    Actor* pTarget = GetTarget();
    CombatState* pCombatState = pCombatController->pCombatState;
    bool bDifferentSpace = true;
    if (pTarget) {
        TESForm* pTargetSpace = pTarget->GetSpace();
        bDifferentSpace = pTargetSpace != pAttacker->GetSpace();
    }
    if (bDifferentSpace) {
        SetFailed("Target is in different space");
        return;
    }
    if (eState == CombatProcedureAttackLow::ATTACK_STATE_INITIALIZING) {
        pWeapon = pCombatState->GetBestCombatWeapon();
        eWeaponType = pWeapon ? pWeapon->GetCombatWeaponType() : COMBAT_WEAPON_TYPE_COUNT;
        fWeaponDPS = pCombatState->GetMaxDPSForWeaponType(eWeaponType);
        if (pWeapon) {
            ItemChange* pWeaponItem = pAttacker->GetInventoryItem(pWeapon, 0);
            if (!pWeaponItem) {
                pCombatState->SetInventoryDirty(true);
                return;
            }
            fWeaponCondition = pWeaponItem->GetItemHealth(true);
            if (pWeaponItem) {
                delete pWeaponItem;
            }
        }
        eState = CombatProcedureAttackLow::ATTACK_STATE_ATTACKING;
    }
    eWeaponType = pWeapon ? pWeapon->GetCombatWeaponType() : COMBAT_WEAPON_TYPE_COUNT;
    if (!pWeapon || pWeapon == pCombatState->GetWeaponByCombatType(eWeaponType)) {
        float fHitPercent = pTarget->IsFleeing(false) ? fCombatLowFleeingTargetHitPercent.Float()
                                                      : fCombatLowTargetHitPercent.Float();
        if (static_cast<double>(fHitPercent) > RandomFloat(0.0f, 1.0f)) {
            HitData* pHitData;
            pHitData = new HitData;
            NiPointer<HitData> spData(pHitData);
            HitData::InitializeHitData(spData.m_pObject, pAttacker, pTarget, fWeaponDPS);
            pTarget->HitMe(spData.m_pObject, 0);
        }
        if (pWeapon && pWeapon->GetCurrentAmmo(pAttacker) && pAttacker->ShouldWeaponUseAmmo(pWeapon)) {
            TESAmmo* pAmmo = pWeapon->GetCurrentAmmo(pAttacker);
            int32_t iAmmoCount = pAttacker->GetInventoryObjectCount(pAmmo);
            double dRateOfFire;
            if (pAttacker) {
                dRateOfFire = CombatFormulas::GetWeaponRateOfFire(pAttacker, pWeapon, nullptr);
            }
            else {
                dRateOfFire = CombatFormulas::GetWeaponRateOfFire(nullptr, pWeapon, nullptr);
            }
            int32_t iAmmoUsed = BSMAX(static_cast<int32_t>(dRateOfFire), 1);
            iAmmoUsed = BSMIN(iAmmoUsed, iAmmoCount);
            pAttacker->RemoveItem(pAmmo, nullptr, iAmmoUsed, false, false, nullptr, nullptr, nullptr, true, false);
            if (iAmmoUsed == iAmmoCount) {
                pCombatState->SetInventoryDirty(true);
                eState = CombatProcedureAttackLow::ATTACK_STATE_INITIALIZING;
            }
        }
    }
    else {
        pCombatState->SetInventoryDirty(true);
        eState = CombatProcedureAttackLow::ATTACK_STATE_INITIALIZING;
    }
}

void CombatProcedureAttackLow::DisplayDebugText(
    IDebugText* apDebugText,
    DEBUGTEXT_LOCATION aeLocation,
    DEBUGTEXT_JUSTIFY aeJustify
) {
    CombatProcedureAttackLow::ATTACK_STATE eState = this->eState;
    if (eState) {
        if (eState == CombatProcedureAttackLow::ATTACK_STATE_ATTACKING) {
            apDebugText->PrintLine(aeLocation, aeJustify, "ATTACKING");
            const char* pWeaponName = pWeapon ? pWeapon->GetFullName() : "Unarmed";
            apDebugText->PrintLine(aeLocation, aeJustify, "Weapon: %s  DPS: %.2f", pWeaponName, fWeaponDPS);
        }
    }
    else {
        apDebugText->PrintLine(aeLocation, aeJustify, "INITIALIZING");
    }
}

CombatProcedure::COMBAT_PROCEDURES CombatProcedureAttackLow::GetType() {
    return ATTACK_LOW;
}

// The procedure's data, the state, DPS and condition, then the weapon's form ID.
void CombatProcedureAttackLow::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    CombatProcedure::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eState), sizeof(eState), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fWeaponDPS), sizeof(fWeaponDPS), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fWeaponCondition), sizeof(fWeaponCondition), 0);
    apSaveGameBuffer->SaveFormID(pWeapon, 0);
}

void CombatProcedureAttackLow::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&eState), sizeof(eState));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fWeaponDPS), sizeof(fWeaponDPS));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fWeaponCondition), sizeof(fWeaponCondition));
    pWeapon = dynamic_cast<TESObjectWEAP*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
}
