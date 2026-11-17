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

// fWeaponCondition is deliberately left unset, as the game does.
CombatProcedureAttackLow::CombatProcedureAttackLow()
  : eState(CombatProcedureAttackLow::ATTACK_STATE_INITIALIZING), fWeaponDPS(0.0f), pWeapon(nullptr) {}

CombatProcedureAttackLow::~CombatProcedureAttackLow() {}

void CombatProcedureAttackLow::Update() {
    TESForm* Space;
    COMBAT_WEAPON_TYPE eweaponType_2;
    Actor* pattacker = GetAttacker();
    Actor* ptarget = GetTarget();
    CombatState* pcombatState = pCombatController->pCombatState;
    if (!ptarget || (Space = ptarget->GetSpace(), Space != pattacker->GetSpace())) {
        SetFailed("Target is in different space");
        return;
    }
    if (eState == CombatProcedureAttackLow::ATTACK_STATE_INITIALIZING) {
        COMBAT_WEAPON_TYPE eweaponType;
        pWeapon = pcombatState->GetBestCombatWeapon();
        if (pWeapon) {
            eweaponType = pWeapon->GetCombatWeaponType();
        }
        else {
            eweaponType = COMBAT_WEAPON_TYPE_COUNT;
        }
        fWeaponDPS = pcombatState->GetMaxDPSForWeaponType(eweaponType);
        if (pWeapon) {
            ItemChange* pweaponItem = pattacker->GetInventoryItem(pWeapon, 0);
            if (!pweaponItem) {
                pcombatState->SetInventoryDirty(true);
                return;
            }
            fWeaponCondition = pweaponItem->GetItemHealth(true);
            if (pweaponItem) {
                delete pweaponItem;
            }
        }
        eState = CombatProcedureAttackLow::ATTACK_STATE_ATTACKING;
    }
    if (pWeapon) {
        eweaponType_2 = pWeapon->GetCombatWeaponType();
    }
    else {
        eweaponType_2 = COMBAT_WEAPON_TYPE_COUNT;
    }
    int32_t aeType = eweaponType_2;
    if (!pWeapon || pWeapon == pcombatState->GetWeaponByCombatType(static_cast<COMBAT_WEAPON_TYPE>(aeType))) {
        float fhitPercent;
        if (ptarget->IsFleeing(false)) {
            fhitPercent = fCombatLowFleeingTargetHitPercent.Float();
        }
        else {
            fhitPercent = fCombatLowTargetHitPercent.Float();
        }
        if (static_cast<double>(fhitPercent) > RandomFloat(0.0f, 1.0f)) {
            HitData* v5;
            HitData* apMemory = static_cast<HitData*>(ourMalloc(sizeof(HitData)));
            if (apMemory) {
                v5 = ::new (apMemory) HitData;
            }
            else {
                v5 = nullptr;
            }
            NiPointer<HitData> spdata(v5);
            HitData::InitializeHitData(spdata.m_pObject, pattacker, ptarget, fWeaponDPS);
            HitData* a1 = spdata.m_pObject;
            ptarget->HitMe(a1, 0);
        }
        if ((pWeapon) && (pWeapon->GetCurrentAmmo(pattacker)) && (pattacker->ShouldWeaponUseAmmo(pWeapon))) {
            double WeaponRateOfFire;
            TESAmmo* pammo = pWeapon->GetCurrentAmmo(pattacker);
            int32_t iammoCount = pattacker->GetInventoryObjectCount(pammo);
            if (pattacker) {
                WeaponRateOfFire = CombatFormulas::GetWeaponRateOfFire(pattacker, pWeapon, nullptr);
            }
            else {
                WeaponRateOfFire = CombatFormulas::GetWeaponRateOfFire(nullptr, pWeapon, nullptr);
            }
            int32_t iammoUsed = BSMAX(static_cast<int32_t>(WeaponRateOfFire), 1);
            int32_t iammoUsed_2 = BSMIN(iammoUsed, iammoCount);
            pattacker->RemoveItem(pammo, nullptr, iammoUsed_2, false, false, nullptr, nullptr, nullptr, true, false);
            if (iammoUsed_2 == iammoCount) {
                pcombatState->SetInventoryDirty(true);
                eState = CombatProcedureAttackLow::ATTACK_STATE_INITIALIZING;
            }
        }
    }
    else {
        pcombatState->SetInventoryDirty(true);
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
            const char* FullName;
            apDebugText->PrintLine(aeLocation, aeJustify, "ATTACKING");
            if (pWeapon) {
                FullName = pWeapon->GetFullName();
            }
            else {
                FullName = "Unarmed";
            }
            apDebugText->PrintLine(aeLocation, aeJustify, "Weapon: %s  DPS: %.2f", FullName, fWeaponDPS);
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
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eState), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fWeaponDPS), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fWeaponCondition), 4, 0);
    apSaveGameBuffer->SaveFormID(pWeapon, 0);
}

void CombatProcedureAttackLow::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&eState), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fWeaponDPS), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fWeaponCondition), 4);
    pWeapon = dynamic_cast<TESObjectWEAP*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
}
