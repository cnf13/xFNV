#include "Fallout/AI/Combat/CombatProcedureAttackGrenade.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMenu/IDebugText.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSExplosion.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatDialogueManager.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatProcedure.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

CombatProcedureAttackGrenade::CombatProcedureAttackGrenade()
  : eState(ATTACK_STATE_IDLE), bTargetLocation(false), TargetLocation(NiPoint3::ZERO), TargetedPoint(NiPoint3::ZERO),
    bHighArc(false) {}

CombatProcedureAttackGrenade::CombatProcedureAttackGrenade(const NiPoint3& arTargetLocation, bool abHighArc)
  : eState(ATTACK_STATE_IDLE), bTargetLocation(true), TargetLocation(arTargetLocation), TargetedPoint(NiPoint3::ZERO),
    bHighArc(abHighArc) {}

CombatProcedureAttackGrenade::~CombatProcedureAttackGrenade() {}

void CombatProcedureAttackGrenade::Update() {
    Actor* pAttacker = GetAttacker();
    Actor* pTarget = GetTarget();
    CombatState* pCombatState = pCombatController->pCombatState;
    TESObjectWEAP* pWeapon = pCombatController->GetCurrentWeapon();
    Animation* pAnim = pAttacker->GetAnimation();
    if (pWeapon && pAnim && pWeapon->GetProjectile(pAttacker)) {
        if (pAttacker->IsAttacking()) {
            eState = CombatProcedureAttackGrenade::ATTACK_STATE_ATTACKING;
        }
        else if (pAttacker->GetAnimAction() == ANIM_ACTION_NONE) {
            eState = CombatProcedureAttackGrenade::ATTACK_STATE_IDLE;
        }
        if (!bTargetLocation && eState == CombatProcedureAttackGrenade::ATTACK_STATE_IDLE) {
            bHighArc = false;
        }
        if (pCombatState->GetHasBufferedDetection() && !pCombatState->bTargetSpaceDifferent) {
            pCombatDialogueManager->StartDialogue(pCombatController, DT_COMBAT, 0, false);
            TargetedPoint = CalculateTargetedPoint();
            float fMinRange = 0.0f;
            float fMaxRange = 0.0f;
            float fAbsoluteMaxRange = 0.0f;
            CombatUtilities::ComputeOptimalWeaponRanges(
                pCombatController,
                pWeapon,
                fMinRange,
                fMaxRange,
                fAbsoluteMaxRange
            );
            NiPoint3 targetVector = TargetedPoint - pAttacker->GetLocationOnReference();
            if (fAbsoluteMaxRange * fAbsoluteMaxRange < targetVector.SqrXYLength()) {
                ClearLookAtTarget();
                eState = CombatProcedureAttackGrenade::ATTACK_STATE_IDLE;
            }
            else {
                if (TargetedPoint != NiPoint3::ZERO) {
                    SetLookAtTarget(TargetedPoint, 4);
                }
                if (eState != CombatProcedureAttackGrenade::ATTACK_STATE_ATTACKING ||
                    (pAttacker->GetAnimAction() == ANIM_ACTION_ATTACK_FOLLOW_THROUGH &&
                     pAnim->GetGroupAction(AGS_WEAPON) <= AGA_SPECIAL_IDLE_LOOP_STOP)) {
                    if (pAttacker->pCurrentProcess->CanAttack() && !pCombatController->IsSwitchingWeapons()) {
                        const NiPoint3* pTargetLocation;
                        NiPoint3 hitLocation;
                        BGSExplosion* pExplosion = pWeapon->GetProjectile(pAttacker)->GetExplosion();
                        if (!pExplosion ||
                            (pTargetLocation = !bTargetLocation ? &pTarget->GetLocationOnReference() : &TargetLocation,
                             (hitLocation = *pTargetLocation, CheckExplosion(pExplosion, hitLocation, 0.0f)))) {
                            BGSProjectile* pProjectile = pWeapon->GetProjectile(pAttacker);
                            if (bTargetLocation ||
                                CombatUtilities::CalculateGrenadeThrow_(
                                    pAttacker,
                                    pTarget,
                                    pProjectile,
                                    nullptr,
                                    &bHighArc,
                                    &TargetedPoint,
                                    -1.0f
                                )) {
                                if (pExplosion) {
                                    pCombatState->pExplosiveProjectileBlocked = nullptr;
                                    ExplosiveProjectileBlockedTimer.Clear();
                                }
                                pAttacker->pCurrentProcess->SetShotsToFire(1);
                                pAttacker->QueueAttack(ANIM_GROUP_ATTACK_THROW);
                                eState = CombatProcedureAttackGrenade::ATTACK_STATE_ATTACKING;
                                if (pExplosion && CombatUtilities::IsDangerousExplosion(pExplosion)) {
                                    pCombatDialogueManager->StartDialogue(pCombatController, DT_COMBAT, 4, true);
                                }
                            }
                        }
                    }
                }
            }
        }
        else if (eState) {
            ClearLookAtTarget();
            eState = CombatProcedureAttackGrenade::ATTACK_STATE_IDLE;
        }
    }
    else {
        SetFailed("Missing either weapon, animation, or weapon projectile");
    }
}

void CombatProcedureAttackGrenade::Shutdown() {
    ClearLookAtTarget();
}

void CombatProcedureAttackGrenade::DisplayDebugText(
    IDebugText* apDebugText,
    DEBUGTEXT_LOCATION aeLocation,
    DEBUGTEXT_JUSTIFY aeJustify
) {
    CombatProcedureAttackGrenade::ATTACK_STATE eState = this->eState;
    if (eState) {
        if (eState == CombatProcedureAttackGrenade::ATTACK_STATE_ATTACKING) {
            apDebugText->PrintLine(aeLocation, aeJustify, "ATTACKING");
        }
    }
    else {
        apDebugText->PrintLine(aeLocation, aeJustify, "IDLE");
    }
    apDebugText->PrintLine(
        aeLocation,
        aeJustify,
        "Targeted Point: %.02f, %.02f, %.02f",
        TargetedPoint.x,
        TargetedPoint.y,
        TargetedPoint.z
    );
    if (bHighArc) {
        apDebugText->PrintLine(aeLocation, aeJustify, "HIGH ARC");
    }
    if (ExplosiveProjectileBlockedTimer.IsSet()) {
        // Reads the target time first, then the elapsed time, and passes each on as a double.
        double dTargetTime = ExplosiveProjectileBlockedTimer.GetTargetTime();
        double dElapsedTime = ExplosiveProjectileBlockedTimer.ElapsedTime();
        apDebugText->PrintLine(
            aeLocation,
            aeJustify,
            "Explosive Projectile Blocked Timer: %.02f/%.02f",
            dElapsedTime,
            dTargetTime
        );
    }
}

bool CombatProcedureAttackGrenade::GetTargetedPoint(NiPoint3& arTargetedPoint, NiPoint3& arReleasePoint) {
    Actor* pAttacker = GetAttacker();
    Actor* pTarget = GetTarget();
    TESObjectWEAP* pWeapon = pCombatController->GetCurrentWeapon();
    BGSProjectile* pProjectile = pWeapon->GetProjectile(pAttacker);
    BGSExplosion* pExplosion = pProjectile->GetExplosion();
    bool bArcValid = false;
    if (bTargetLocation) {
        if (CombatUtilities::CheckProjectileArc(
                pAttacker,
                pTarget,
                pProjectile,
                arReleasePoint,
                TargetLocation,
                NiPoint3::ZERO,
                bHighArc,
                TargetedPoint,
                -1.0f
            )) {
            arTargetedPoint = TargetedPoint;
            bArcValid = true;
        }
    }
    else {
        BGSProjectile* pThrownProjectile = pWeapon->GetProjectile(pAttacker);
        Actor* pThrowTarget = GetTarget();
        pAttacker = GetAttacker();
        if (CombatUtilities::CalculateGrenadeThrow_(
                pAttacker,
                pThrowTarget,
                pThrownProjectile,
                &arReleasePoint,
                &bHighArc,
                &TargetedPoint,
                -1.0f
            )) {
            arTargetedPoint = TargetedPoint;
            bArcValid = true;
        }
    }
    if (bArcValid && pExplosion) {
        NiPoint3 hitLocation;
        if (bTargetLocation) {
            hitLocation = TargetLocation;
        }
        else {
            hitLocation = TargetedPoint;
            hitLocation.z = pTarget->GetLocationOnReference().z;
        }
        float fTime = CombatUtilities::ComputeProjectileTime_(pProjectile, arReleasePoint, hitLocation, bHighArc, 0.0f);
        if (!CheckExplosion(pExplosion, hitLocation, fTime)) {
            bArcValid = false;
        }
    }

    if (!bArcValid && bTargetLocation) {
        pCombatController->pCombatState->SetTargetFlushed(true);
        SetFinished();
    }
    return bArcValid;
}

void CombatProcedureAttackGrenade::FiredProjectile(Projectile* apProjectile) {
    if (bHighArc) {
        apProjectile->SetSpeedMult(gs_fGrenadeHighArcSpeedPercentage.Float());
    }
    bHighArc = false;
    if (bTargetLocation) {
        pCombatController->pCombatState->SetTargetFlushed(true);
        SetFinished();
    }
}

CombatProcedure::COMBAT_PROCEDURES CombatProcedureAttackGrenade::GetType() {
    return ATTACK_GRENADE;
}

// The procedure's data, the state, the target location and its flag, the targeted point, the arc flag, then the
// blocked-projectile timer.
void CombatProcedureAttackGrenade::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    CombatProcedure::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eState), sizeof(eState), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bTargetLocation), sizeof(bTargetLocation), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&TargetLocation), sizeof(TargetLocation), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&TargetedPoint), sizeof(TargetedPoint), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bHighArc), sizeof(bHighArc), 0);
    ExplosiveProjectileBlockedTimer.SaveGame(apSaveGameBuffer);
}

void CombatProcedureAttackGrenade::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&eState), sizeof(eState));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bTargetLocation), sizeof(bTargetLocation));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&TargetLocation), sizeof(TargetLocation));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&TargetedPoint), sizeof(TargetedPoint));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bHighArc), sizeof(bHighArc));
    ExplosiveProjectileBlockedTimer.LoadGame(apLoadGameBuffer);
}

void CombatProcedureAttackGrenade::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::InitLoadGame(apLoadGameBuffer);
}

bool CombatProcedureAttackGrenade::CheckExplosion(BGSExplosion* apExplosion, const NiPoint3& arLocation, float afTime) {
    Actor* pAttacker = pCombatController->pAttacker;
    CombatState* pCombatState = pCombatController->pCombatState;
    if (pCombatManager->CheckExplosionAttack(pAttacker, apExplosion, arLocation, afTime)) {
        return true;
    }
    if (ExplosiveProjectileBlockedTimer.IsSet()) {
        if (ExplosiveProjectileBlockedTimer.Check()) {
            pCombatState->pExplosiveProjectileBlocked = apExplosion;
        }
    }
    else {
        ExplosiveProjectileBlockedTimer.Reset(fExplosiveProjectileBlockedWaitTime.Float());
    }
    if (bTargetLocation) {
        pCombatState->SetTargetFlushed(true);
        SetFinished();
    }
    return false;
}

NiPoint3 CombatProcedureAttackGrenade::CalculateTargetedPoint() {
    if (bTargetLocation || GetTarget()) {
        NiPoint3 kPoint;
        TESObjectWEAP* pWeapon = pCombatController->GetCurrentWeapon();
        Actor* pAttacker = GetAttacker();
        BGSProjectile* pProjectile = pWeapon->GetProjectile(pAttacker);
        float fSpeedMult = this->bHighArc ? gs_fGrenadeHighArcSpeedPercentage.Float() : 1.0f;
        float fProjectileSpeed = pProjectile->GetSpeed() * fSpeedMult;
        bool bHighArc = this->bHighArc;
        if (bTargetLocation) {
            pAttacker = GetAttacker();
            kPoint = CombatUtilities::GetProjectedPointForTarget(
                pAttacker,
                TargetLocation,
                nullptr,
                pProjectile,
                fProjectileSpeed,
                bHighArc
            );
        }
        else {
            Actor* pTarget = GetTarget();
            pAttacker = GetAttacker();
            kPoint = CombatUtilities::GetProjectedPointForTarget(
                pAttacker,
                pTarget,
                nullptr,
                pProjectile,
                fProjectileSpeed,
                bHighArc
            );
        }
        return kPoint;
    }

    return NiPoint3::ZERO;
}
