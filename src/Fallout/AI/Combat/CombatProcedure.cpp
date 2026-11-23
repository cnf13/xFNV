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

CombatProcedure::CombatProcedure()
  : pCombatController(nullptr), eStatus(CombatProcedure::ACTIVE), pFailureReason(nullptr) {}

void CombatProcedure::Initialize(CombatController* apCombatController) {
    pCombatController = apCombatController;
}

bool CombatProcedure::GetTargetedPoint(NiPoint3& arTargetedPoint, NiPoint3& arReleasePoint) {
    return false;
}

void CombatProcedure::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eStatus), sizeof(eStatus), 0);
}

void CombatProcedure::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&eStatus), sizeof(eStatus));
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
    return eStatus == FAILED;
}

bool CombatProcedure::GetFinished() {
    return eStatus == FINISHED;
}

void CombatProcedure::SetMoveMode(int32_t aiCombatMoveMode) {
    pCombatController->SetMoveMode(static_cast<COMBAT_MOVE_MODE>(aiCombatMoveMode));
}

bool CombatProcedure::WithinAttackRange(float afDistance, float afReach, int32_t aiAttackWithinReach) {
    GetAttacker();
    TESCombatStyle* pCombatStyle = pCombatController->GetCurrentCombatStyle();
    pCombatStyle->QPowerAttackChance();
    return afDistance <= afReach + 0.0f;
}

void CombatProcedure::SetLookAtTarget(const NiPoint3& arLookAtPoint, uint8_t acPriority) {
    uint8_t lookAtTargetIndex = static_cast<uint8_t>(pCombatController->GetLookAtTargetIndex(this));
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
    NiPoint3 movedelta;
    ANIM_GROUP_ENUM einRangeAttacks[5];
    uint8_t aucInRange[8];
    bool bcanDo = false;
    ANIM_GROUP_ENUM echosenAttack = ANIM_GROUP_NONE;
    int32_t iinRangeAttackCount = 0;
    ourMemset(aucInRange, 0, 5u);
    Actor* pattacker = this->GetAttacker();
    TESCombatStyle* pcombatStyle = this->pCombatController->GetCurrentCombatStyle();
    if (this->pCombatController->IsSwitchingWeapons()) {
        return ANIM_GROUP_NONE;
    }
    if (!this->pCombatController->ShouldInitPowerAttackDistances()) {
        this->pCombatController->InitPowerAttackDistances();
    }
    bool brushing = this->pCombatController->bHasForwardAttacks && !this->WithinAttackRange(afDistance, afReach, 0);
    float fdistanceMult = brushing ? pcombatStyle->Data.fRushingAttackDistanceMult : 1.0f;
    Animation* panim = pattacker->GetAnimation();
    float fattackSpeed = panim->m_fAttackSpeed;
    if (fattackSpeed <= 0.0) {
        fattackSpeed = 1.0f;
    }
    float ftargetVelocity = static_cast<CombatProcedureAttackMelee*>(this)
                                ->CombatProcedureAttackMelee::GetMovingTargetVelocity();
    float fmodifiedReach = afReach * fdistanceMult;
    bool bcanDoMovementPowerAttacks = pattacker->GetActorFloatValue(ActorValue::LEFT_MOBILITY_CONDITION) > 0.0 &&
        pattacker->GetActorFloatValue(ActorValue::RIGHT_MOBILITY_CONDITION) > 0.0;
    for (int32_t i = 0; i < 5; ++i) {
        float fpowerAttackRange = this->pCombatController->GetPowerAttackRange(i);
        if (!(fpowerAttackRange > 1.0) || bcanDoMovementPowerAttacks) {
            float fscaledRange = fpowerAttackRange * fdistanceMult;
            float fattackRange = BSMAX(fscaledRange, fmodifiedReach);
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
                        aucInRange[i] = 1;
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
    else if (iinRangeAttackCount > 1 && brushing && !pcombatStyle->IsFlagSet(TESCombatStyle::USE_PA_PERCENT_CHANCE)) {
        int32_t iwhich = RandomInt(0, iinRangeAttackCount);
        echosenAttack = einRangeAttacks[iwhich];
    }
    if (echosenAttack == ANIM_GROUP_NONE) {
        if (!pcombatStyle->IsFlagSet(TESCombatStyle::USE_PA_PERCENT_CHANCE)) {
            int32_t ipreferredDirection = 5;
            if (aucInRange[2] && fCombatMinEngageDistance.Float() > static_cast<double>(afDistance)) {
                ipreferredDirection = 2;
            }
            else if (aucInRange[1] && afReach * 1.25 < afDistance) {
                ipreferredDirection = 1;
            }
            else {
                uint16_t smoveMode = pattacker->GetMoveMode();
                if (aucInRange[3] && (smoveMode & 4) != 0) {
                    ipreferredDirection = 3;
                }
                else if (aucInRange[4] && (smoveMode & 8) != 0) {
                    ipreferredDirection = 4;
                }
                else if (aucInRange[0]) {
                    ipreferredDirection = 0;
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
                uint32_t icumulativeChance = 0;
                for (uint32_t n = 0; n < iinRangeAttackCount; ++n) {
                    icumulativeChance += aucChances[einRangeAttacks[n] - ANIM_GROUP_ATTACK_NORMAL_POWER];
                    if (idieRoll <= icumulativeChance) {
                        echosenAttack = einRangeAttacks[n];
                        break;
                    }
                }
            }
        }
    }
    if (!this->pCombatController->GetPowerAttackDelta(echosenAttack, movedelta)) {
        int32_t iformID = pattacker->iFormID;
        const char* pcFullName = pattacker->GetFullName();
        Warning("Melee Attacker %s (%0X) is using %d attack for power attack\n", pcFullName, iformID, echosenAttack);
    }
    bcanDo = true;
    if (CombatController::GetShowVerboseMessages() && bcanDo) {
        const char* pcAttackName;
        if (echosenAttack == ANIM_GROUP_ATTACK_NORMAL_POWER) {
            pcAttackName = "NORMAL";
        }
        else if (echosenAttack == ANIM_GROUP_ATTACK_FORWARD_POWER) {
            pcAttackName = "FORWARD";
        }
        else if (echosenAttack == ANIM_GROUP_ATTACK_BACK_POWER) {
            pcAttackName = "BACKWARD";
        }
        else if (echosenAttack == ANIM_GROUP_ATTACK_LEFT_POWER) {
            pcAttackName = "LEFT";
        }
        else if (echosenAttack == ANIM_GROUP_ATTACK_RIGHT_POWER) {
            pcAttackName = "RIGHT";
        }
        else {
            pcAttackName = "NO";
        }
        Interface::PrintLine("%.20s selects %s power attack!", pattacker->GetFullName(), pcAttackName);
    }
    return echosenAttack;
}

CombatProcedure* CombatProcedure::CreateProcedure(uint32_t aiType) {
    CombatProcedure* pnewProcedure;
    switch (aiType) {
        case 0u: {
            CombatProcedureAttackRanged* pProcedure = new CombatProcedureAttackRanged;
            pnewProcedure = pProcedure;
            break;
        }
        case 1u: {
            CombatProcedureAttackMelee* pProcedure = new CombatProcedureAttackMelee;
            pnewProcedure = pProcedure;
            break;
        }
        case 2u: {
            CombatProcedureAttackGrenade* pProcedure = new CombatProcedureAttackGrenade;
            pnewProcedure = pProcedure;
            break;
        }
        case 3u: {
            CombatProcedureAttackLow* pProcedure = new CombatProcedureAttackLow;
            pnewProcedure = pProcedure;
            break;
        }
        case 4u: {
            CombatProcedureEvade* pProcedure;
            pProcedure = new CombatProcedureEvade(0);
            pnewProcedure = pProcedure;
            break;
        }
        case 5u: {
            CombatProcedureSwitchWeapon* pProcedure = new CombatProcedureSwitchWeapon(nullptr);
            pnewProcedure = pProcedure;
            break;
        }
        case 6u: {
            CombatProcedureMove* pProcedure;
            pProcedure = new CombatProcedureMove(nullptr, 0.0f, 0);
            pnewProcedure = pProcedure;
            break;
        }
        case 7u: {
            CombatProcedureBeInCover* pProcedure = new CombatProcedureBeInCover(nullptr);
            pnewProcedure = pProcedure;
            break;
        }
        case 8u: {
            CombatProcedureActivateObject* pProcedure = new CombatProcedureActivateObject(nullptr, nullptr);
            pnewProcedure = pProcedure;
            break;
        }
        case 9u: {
            CombatProcedureHideFromTarget* pProcedure = new CombatProcedureHideFromTarget(nullptr, 0.0f, false, false);
            pnewProcedure = pProcedure;
            break;
        }
        case 10u: {
            CombatProcedureSearch* pProcedure = new CombatProcedureSearch;
            pnewProcedure = pProcedure;
            break;
        }
        case 11u: {
            CombatProcedureUseCombatItem* pProcedure = new CombatProcedureUseCombatItem(nullptr);
            pnewProcedure = pProcedure;
            break;
        }
        case 12u: {
            CombatProcedureEngageTarget* pProcedure = new CombatProcedureEngageTarget;
            pnewProcedure = pProcedure;
            break;
        }
        default: {
            Warning(WARN_COMBAT, "COMBAT: Could not create combat procedure of type %i", aiType);
            pnewProcedure = nullptr;
            break;
        }
    }
    return pnewProcedure;
}
