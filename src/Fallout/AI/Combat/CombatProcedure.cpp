#include "Fallout/AI/Combat/CombatProcedure.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESCombatStyle.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/CombatLocation.h"
#include "Fallout/AI/Combat/CombatProcedureActivateObject.h"
#include "Fallout/AI/Combat/CombatProcedureAttackGrenade.h"
#include "Fallout/AI/Combat/CombatProcedureAttackLow.h"
#include "Fallout/AI/Combat/CombatProcedureAttackMelee.h"
#include "Fallout/AI/Combat/CombatProcedureAttackRanged.h"
#include "Fallout/AI/Combat/CombatProcedureBeInCover.h"
#include "Fallout/AI/Combat/CombatProcedureEngageTarget.h"
#include "Fallout/AI/Combat/CombatProcedureEvade.h"
#include "Fallout/AI/Combat/CombatProcedureHideFromTarget.h"
#include "Fallout/AI/Combat/CombatProcedureMove.h"
#include "Fallout/AI/Combat/CombatProcedureSearch.h"
#include "Fallout/AI/Combat/CombatProcedureSwitchWeapon.h"
#include "Fallout/AI/Combat/CombatProcedureUseCombatItem.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

// No code reads or writes these four bytes or the value after them.
using CombatProcedureUnusedByteArray = uint8_t[4];
static CombatProcedureUnusedByteArray data_11A5ADC{1, 2, 3, 4};
static uint32_t data_11A5AE0 = 4;

CombatProcedure::CombatProcedure()
  : pCombatController(nullptr), eStatus(CombatProcedure::ACTIVE), pFailureReason(nullptr) {}

void CombatProcedure::Initialize(CombatController* apCombatController) {
    pCombatController = apCombatController;
}

bool CombatProcedure::GetTargetedPoint(NiPoint3& arTargetedPoint, NiPoint3& arReleasePoint) {
    return false;
}

void CombatProcedure::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eStatus), 4, 0);
}

void CombatProcedure::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&eStatus), 4);
}

Actor* CombatProcedure::GetAttacker() {
    return pCombatController->GetAttacker();
}

void CombatProcedure::ClearLookAtTarget() {
    pCombatController->ClearLookAtTarget(pCombatController->GetLookAtTargetIndex(this));
}

void CombatProcedure::ClearMoveMode() {
    pCombatController->ClearMoveMode();
}

// The reason is a const string; the field is not.
void CombatProcedure::SetFailed(const char* apReason) {
    pFailureReason = const_cast<char*>(apReason);
    eStatus = FAILED;
}

Actor* CombatProcedure::GetTarget() {
    return pCombatController->GetTarget();
}

bool CombatProcedure::GetFailed() {
    return (bool)((uint8_t)(((uint32_t)eStatus) == 1));
}

bool CombatProcedure::GetFinished() {
    return (bool)((uint8_t)(((uint32_t)eStatus) == 2));
}

void CombatProcedure::SetMoveMode(int32_t aiCombatMoveMode) {
    pCombatController->SetMoveMode(static_cast<COMBAT_MOVE_MODE>(aiCombatMoveMode));
}

bool CombatProcedure::WithinAttackRange(float a2, float a3, int32_t a4) {
    GetAttacker();
    TESCombatStyle* combatStyle = pCombatController->GetCurrentCombatStyle();
    combatStyle->QPowerAttackChance();
    return (bool)((uint8_t)(a2 <= (a3 + ((float)0.0))));
}

void CombatProcedure::SetLookAtTarget(const NiPoint3& arLookAtPoint, uint8_t acPriority) {
    uint8_t lookAtTargetIndex = (uint8_t)pCombatController->GetLookAtTargetIndex(this);
    pCombatController->SetLookAtTarget(lookAtTargetIndex, arLookAtPoint, acPriority);
}

void CombatProcedure::SetFinished() {
    eStatus = CombatProcedure::FINISHED;
}

ANIM_GROUP_ENUM CombatProcedure::SelectPowerAttack(
    float afDistance,
    float afReach,
    bool abTargetAttacking,
    bool abTargetPostAttack,
    bool abForwardOnly
) {
    // Each power attack's chance, indexed by the attack's group minus ANIM_GROUP_ATTACK_NORMAL_POWER.
    uint8_t aucChances[5];
    const char* FullName;
    int32_t a1;
    ANIM_GROUP_ENUM a2;
    float MinDistance;
    NiPoint3 movedelta;
    ANIM_GROUP_ENUM einRangeAttacks[5];
    uint8_t apDest[8];
    bool bcanDo = 0;
    bool buseInRangeAttacks = 0;
    int32_t result = 255;
    ANIM_GROUP_ENUM echosenAttack = ANIM_GROUP_NONE;
    int32_t iinRangeAttackCount = 0;
    ourMemset(apDest, 0, 5u);
    Actor* pattacker = this->GetAttacker();
    TESCombatStyle* pcombatStyle = this->pCombatController->GetCurrentCombatStyle();
    if (this->pCombatController->IsSwitchingWeapons()) {
        return ANIM_GROUP_NONE;
    }
    if (!this->pCombatController->ShouldInitPowerAttackDistances()) {
        this->pCombatController->InitPowerAttackDistances();
    }
    int32_t v19 = this->pCombatController->bHasForwardAttacks && !this->WithinAttackRange(afDistance, afReach, 0);
    bool brushing = v19;
    if (v19) {
        MinDistance = pcombatStyle->Data.fRushingAttackDistanceMult;
    }
    else {
        MinDistance = 1.0f;
    }
    Animation* panim = pattacker->GetAnimation();
    float fattackSpeed = panim->m_fAttackSpeed;
    if (fattackSpeed <= 0.0) {
        fattackSpeed = 1.0f;
    }
    float ftargetVelocity =
        static_cast<CombatProcedureAttackMelee*>(this)->CombatProcedureAttackMelee::GetMovingTargetVelocity();
    float fmodifiedReach = afReach * MinDistance;
    int32_t v17 = pattacker->GetActorFloatValue(ActorValue::LEFT_MOBILITY_CONDITION) > 0.0 &&
        pattacker->GetActorFloatValue(ActorValue::RIGHT_MOBILITY_CONDITION) > 0.0;
    bool bcanDoMovementPowerAttacks = v17;
    for (int32_t i = 0; i < 5; ++i) {
        float fpowerAttackRange = this->pCombatController->GetPowerAttackRange(i);
        if (!(fpowerAttackRange > 1.0) || bcanDoMovementPowerAttacks) {
            float v16 = fpowerAttackRange * MinDistance;
            float fattackRange = BSMAX(v16, fmodifiedReach);
            float fpowerAttackHitTime = this->pCombatController->GetPowerAttackHitTime(i);
            if (!(fpowerAttackHitTime < 0.0)) {
                ANIM_GROUP_ENUM epowerAttack = static_cast<ANIM_GROUP_ENUM>(i + 92);
                if (!pcombatStyle->IsFlagSet(TESCombatStyle::USE_PA_PERCENT_CHANCE) ||
                    pcombatStyle->GetPowerAttackChance(epowerAttack)) {
                    float fmodifiedDistance = fpowerAttackHitTime * ftargetVelocity / fattackSpeed + afDistance;
                    if (fattackRange >= static_cast<double>(fmodifiedDistance) &&
                        (fmodifiedReach >= static_cast<double>(fpowerAttackRange) ||
                         fmodifiedDistance > fattackRange * 0.6600000262260437)) {
                        einRangeAttacks[iinRangeAttackCount++] = epowerAttack;
                        apDest[i] = 1;
                    }
                }
            }
        }
    }
    if (!iinRangeAttackCount) {
        return ANIM_GROUP_NONE;
    }
    for (int32_t j = iinRangeAttackCount; j < 5; ++j) {
        einRangeAttacks[j] = ANIM_GROUP_NONE;
    }
    if (iinRangeAttackCount == 1) {
        echosenAttack = einRangeAttacks[0];
    }
    else {
        if (iinRangeAttackCount > 1 && brushing && !pcombatStyle->IsFlagSet(TESCombatStyle::USE_PA_PERCENT_CHANCE)) {
            int32_t iwhich = RandomInt(0, iinRangeAttackCount);
            echosenAttack = einRangeAttacks[iwhich];
        }
    }
    if (echosenAttack == ANIM_GROUP_NONE) {
        if (!pcombatStyle->IsFlagSet(TESCombatStyle::USE_PA_PERCENT_CHANCE)) {
            int32_t ipreferredDirection = 5;
            if (apDest[2] && fCombatMinEngageDistance.Float() > static_cast<double>(afDistance)) {
                ipreferredDirection = 2;
            }
            else {
                if (apDest[1] && afReach * 1.25 < afDistance) {
                    ipreferredDirection = 1;
                }
                else {
                    uint16_t smoveMode = pattacker->GetMoveMode();
                    if (apDest[3] && (smoveMode & 4) != 0) {
                        ipreferredDirection = 3;
                    }
                    else {
                        if (apDest[4] && (smoveMode & 8) != 0) {
                            ipreferredDirection = 4;
                        }
                        else {
                            if (apDest[0]) {
                                ipreferredDirection = 0;
                            }
                        }
                    }
                }
            }
            if (ipreferredDirection != 5) {
                echosenAttack = static_cast<ANIM_GROUP_ENUM>(ePowerAttacks[ipreferredDirection]);
            }
        }
        if (echosenAttack == ANIM_GROUP_NONE) {
            for (uint32_t k = 0; k < 5; ++k) {
                aucChances[k] = pcombatStyle->GetPowerAttackChance(
                    static_cast<ANIM_GROUP_ENUM>(k + ANIM_GROUP_ATTACK_NORMAL_POWER)
                );
            }
            uint32_t itotalChance = 0;
            for (uint32_t m = 0; m < iinRangeAttackCount; ++m) {
                itotalChance += aucChances[einRangeAttacks[m] - ANIM_GROUP_ATTACK_NORMAL_POWER];
            }
            if (itotalChance) {
                uint32_t idieRoll = RandomInt(0, itotalChance);
                uint32_t v22 = 0;
                for (uint32_t n = 0; n < iinRangeAttackCount; ++n) {
                    v22 += aucChances[einRangeAttacks[n] - ANIM_GROUP_ATTACK_NORMAL_POWER];
                    if (idieRoll <= v22) {
                        echosenAttack = einRangeAttacks[n];
                        break;
                    }
                }
            }
        }
    }
    if (!this->pCombatController->GetPowerAttackDelta(echosenAttack, movedelta)) {
        a2 = echosenAttack;
        a1 = pattacker->iFormID;
        FullName = pattacker->GetFullName();
        Warning("Melee Attacker %s (%0X) is using %d attack for power attack\n", FullName, a1, a2);
    }
    bcanDo = 1;
    if (CombatController::GetShowVerboseMessages() && bcanDo) {
        const char* v15;
        if (echosenAttack == ANIM_GROUP_ATTACK_NORMAL_POWER) {
            v15 = "NORMAL";
        }
        else {
            const char* v14;
            if (echosenAttack == ANIM_GROUP_ATTACK_FORWARD_POWER) {
                v14 = "FORWARD";
            }
            else {
                const char* v13;
                if (echosenAttack == ANIM_GROUP_ATTACK_BACK_POWER) {
                    v13 = "BACKWARD";
                }
                else {
                    const char* v12;
                    if (echosenAttack == ANIM_GROUP_ATTACK_LEFT_POWER) {
                        v12 = "LEFT";
                    }
                    else {
                        const char* v11;
                        if (echosenAttack == ANIM_GROUP_ATTACK_RIGHT_POWER) {
                            v11 = "RIGHT";
                        }
                        else {
                            v11 = "NO";
                        }
                        v12 = v11;
                    }
                    v13 = v12;
                }
                v14 = v13;
            }
            v15 = v14;
        }
        Interface::PrintLine("%.20s selects %s power attack!", pattacker->GetFullName(), v15);
    }
    return echosenAttack;
}

CombatProcedure* CombatProcedure::CreateProcedure(uint32_t aiType) {
    CombatProcedure* result;
    switch (aiType) {
        case 0u: {
            CombatProcedureAttackRanged* v14;
            CombatProcedureAttackRanged* v27 =
                static_cast<CombatProcedureAttackRanged*>(ourMalloc(sizeof(CombatProcedureAttackRanged)));
            if (v27) {
                v14 = ::new (v27) CombatProcedureAttackRanged;
            }
            else {
                v14 = 0;
            }
            result = (CombatProcedure*)v14;
            break;
        }
        case 1u: {
            CombatProcedureAttackMelee* v13;
            CombatProcedureAttackMelee* v26 =
                static_cast<CombatProcedureAttackMelee*>(ourMalloc(sizeof(CombatProcedureAttackMelee)));
            if (v26) {
                v13 = ::new (v26) CombatProcedureAttackMelee;
            }
            else {
                v13 = 0;
            }
            result = (CombatProcedure*)v13;
            break;
        }
        case 2u: {
            CombatProcedureAttackGrenade* v12;
            CombatProcedureAttackGrenade* v25 =
                static_cast<CombatProcedureAttackGrenade*>(ourMalloc(sizeof(CombatProcedureAttackGrenade)));
            if (v25) {
                v12 = ::new (v25) CombatProcedureAttackGrenade;
            }
            else {
                v12 = 0;
            }
            result = (CombatProcedure*)v12;
            break;
        }
        case 3u: {
            CombatProcedureAttackLow* v11;
            CombatProcedureAttackLow* apMemory =
                static_cast<CombatProcedureAttackLow*>(ourMalloc(sizeof(CombatProcedureAttackLow)));
            if (apMemory) {
                v11 = ::new (apMemory) CombatProcedureAttackLow;
            }
            else {
                v11 = 0;
            }
            result = (CombatProcedure*)v11;
            break;
        }
        case 4u: {
            CombatProcedureEvade* v10;
            CombatProcedureEvade* v23 = static_cast<CombatProcedureEvade*>(ourMalloc(sizeof(CombatProcedureEvade)));
            if (v23) {
                v10 = ::new (v23) CombatProcedureEvade(0);
            }
            else {
                v10 = 0;
            }
            result = (CombatProcedure*)v10;
            break;
        }
        case 5u: {
            CombatProcedureSwitchWeapon* v9;
            CombatProcedureSwitchWeapon* v22 =
                static_cast<CombatProcedureSwitchWeapon*>(ourMalloc(sizeof(CombatProcedureSwitchWeapon)));
            if (v22) {
                v9 = ::new (v22) CombatProcedureSwitchWeapon(nullptr);
            }
            else {
                v9 = 0;
            }
            result = (CombatProcedure*)v9;
            break;
        }
        case 6u: {
            CombatProcedureMove* v8;
            CombatProcedureMove* v21 = static_cast<CombatProcedureMove*>(ourMalloc(sizeof(CombatProcedureMove)));
            if (v21) {
                v8 = ::new (v21) CombatProcedureMove(nullptr, 0.0f, 0);
            }
            else {
                v8 = 0;
            }
            result = (CombatProcedure*)v8;
            break;
        }
        case 7u: {
            CombatProcedureBeInCover* v7;
            CombatProcedureBeInCover* v20 =
                static_cast<CombatProcedureBeInCover*>(ourMalloc(sizeof(CombatProcedureBeInCover)));
            if (v20) {
                v7 = ::new (v20) CombatProcedureBeInCover(nullptr);
            }
            else {
                v7 = 0;
            }
            result = (CombatProcedure*)v7;
            break;
        }
        case 8u: {
            CombatProcedureActivateObject* v6;
            CombatProcedureActivateObject* v19 =
                static_cast<CombatProcedureActivateObject*>(ourMalloc(sizeof(CombatProcedureActivateObject)));
            if (v19) {
                v6 = ::new (v19) CombatProcedureActivateObject(nullptr, nullptr);
            }
            else {
                v6 = 0;
            }
            result = (CombatProcedure*)v6;
            break;
        }
        case 9u: {
            CombatProcedureHideFromTarget* v5;
            CombatProcedureHideFromTarget* v18 =
                static_cast<CombatProcedureHideFromTarget*>(ourMalloc(sizeof(CombatProcedureHideFromTarget)));
            if (v18) {
                v5 = ::new (v18) CombatProcedureHideFromTarget(nullptr, 0.0f, false, false);
            }
            else {
                v5 = 0;
            }
            result = (CombatProcedure*)v5;
            break;
        }
        case 10u: {
            CombatProcedureSearch* v4;
            CombatProcedureSearch* v17 = static_cast<CombatProcedureSearch*>(ourMalloc(sizeof(CombatProcedureSearch)));
            if (v17) {
                v4 = ::new (v17) CombatProcedureSearch;
            }
            else {
                v4 = 0;
            }
            result = (CombatProcedure*)v4;
            break;
        }
        case 11u: {
            CombatProcedureUseCombatItem* v3;
            CombatProcedureUseCombatItem* v16 =
                static_cast<CombatProcedureUseCombatItem*>(ourMalloc(sizeof(CombatProcedureUseCombatItem)));
            if (v16) {
                v3 = ::new (v16) CombatProcedureUseCombatItem(nullptr);
            }
            else {
                v3 = 0;
            }
            result = (CombatProcedure*)v3;
            break;
        }
        case 12u: {
            CombatProcedureEngageTarget* v2;
            CombatProcedureEngageTarget* v15 =
                static_cast<CombatProcedureEngageTarget*>(ourMalloc(sizeof(CombatProcedureEngageTarget)));
            if (v15) {
                v2 = ::new (v15) CombatProcedureEngageTarget;
            }
            else {
                v2 = 0;
            }
            result = (CombatProcedure*)v2;
            break;
        }
        default: {
            Warning(WARN_COMBAT, "COMBAT: Could not create combat procedure of type %i", aiType);
            result = 0;
            break;
        }
    }
    return result;
}
