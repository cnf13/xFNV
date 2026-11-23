#include "Fallout/AI/Combat/CombatProcedureAttackRanged.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMenu/IDebugText.h"
#include "Fallout Shared/AnimSequenceBase.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSExplosion.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESCombatStyle.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatDialogueManager.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatProcedure.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"

#include <cmath>
#include <limits>

CombatProcedureAttackRanged::CombatProcedureAttackRanged() : eState(ATTACK_STATE_IDLE), TargetedPoint(NiPoint3::ZERO) {}

CombatProcedureAttackRanged::~CombatProcedureAttackRanged() {}

void CombatProcedureAttackRanged::Update() {
    float fdistanceFraction;
    NiPoint3 firingLocation;
    TESObjectREFR* phitRef;
    NiPoint3 hitPoint;
    Actor* pattacker = GetAttacker();
    Actor* ptarget = GetTarget();
    CombatState* pcombatState = pCombatController->pCombatState;
    CombatGroup* pcombatGroup = pCombatController->pCombatGroup;
    ANIM_GROUP_ENUM eattack = ANIM_GROUP_NONE;
    TESObjectWEAP* pweapon = pCombatController->GetCurrentWeapon();
    Animation* panim = pattacker->GetAnimation();
    if (!pweapon || !panim || !pweapon->GetProjectile(pattacker)) {
        SetFailed("Missing either weapon, animation, or weapon projectile");
        return;
    }
    float ftargetingFOV = pCombatController->GetCurrentCombatStyle()->SimpleData.fTargetingFOV;
    if (ftargetingFOV > 0.0 && ptarget) {
        float ftargetingArc = ftargetingFOV * 0.01745329238474369;
        if (!CombatUtilities::IsTargetWithinFiringArc(
                pattacker,
                ptarget->GetLocationOnReference(),
                ftargetingArc,
                6.2831855f
            )) {
            SetFailed("Target outside targeting FOV");
            return;
        }
    }

    BGSProjectile* pprojectile = pweapon->GetProjectile(pattacker);
    bool bautomaticWeapon = pweapon->IsAutomaticWeapon();
    bool bfiresBursts = bautomaticWeapon || pweapon->GetBurstShot();
    bool btooCloseToTarget = false;
    NiPoint3 targetPoint = CalculateTargetedPoint(nullptr);
    bool bcanAttack = pattacker->pCurrentProcess->CanAttack() || pattacker->IsAttacking();
    if (pCombatController->IsSwitchingWeapons()) {
        bcanAttack = false;
    }
    if (this->eState && !bcanAttack) {
        StopFiring();
        this->eState = CombatProcedureAttackRanged::ATTACK_STATE_IDLE;
    }

    if (bcanAttack) {
        float fminRange = 0.0f;
        float fmaxRange = 0.0f;
        float fabsoluteMaxRange = 0.0f;
        CombatUtilities::ComputeOptimalWeaponRanges(
            pCombatController,
            pweapon,
            fminRange,
            fmaxRange,
            fabsoluteMaxRange
        );
        NiPoint3 targetPointModified = targetPoint;
        if (ptarget) {
            targetPointModified.z = ptarget->GetLocationOnReference().z;
        }
        NiPoint3 vec = targetPointModified - pattacker->GetLocationOnReference();
        float ftargetDistSquared = vec.SqrLength();
        if (ftargetDistSquared > fabsoluteMaxRange * fabsoluteMaxRange) {
            bcanAttack = false;
        }
        if (ftargetDistSquared < fminRange * fminRange) {
            btooCloseToTarget = true;
        }
    }
    if (pcombatState->GetHasTarget360LineOfSight()) {
        TargetedPoint = targetPoint;
    }
    if (pcombatState->GetHasTarget360LineOfSight() && pcombatState->GetHasBufferedDetection() && bcanAttack) {
        CombatProcedureAttackRanged::ATTACK_STATE ecurrentState = this->eState;
        if (ecurrentState == CombatProcedureAttackRanged::ATTACK_STATE_FIRING) {
            if (bfiresBursts) {
                if (!FiringTimer.IsSet()) {
                    if (-std::numeric_limits<float>::max() !=
                        static_cast<float>(pcombatState->GetFiringStartedTimeStamp())) {
                        float ffiringTime = fAutomaticWeaponBurstFireTime.Float();
                        uint16_t eanimGroup = panim->GetGroup(AGS_WEAPON);
                        if (TESAnimGroup::GetType(eanimGroup) == ANIM_GROUP_ATTACK_SPIN ||
                            TESAnimGroup::GetType(eanimGroup) == ANIM_GROUP_ATTACK_SPIN_2) {
                            AnimSequenceBase* panimSeq;
                            if (panim->GetMapAnimSequenceBase()->GetAt(eanimGroup, panimSeq)) {
                                TESAnimGroup* panimGroup = panimSeq->GetSequenceToPlay(-1)->GetAnimGroup();
                                float fstartTime = panimGroup->GetTime(AGA_START);
                                float ffireTime = panimGroup->GetTime(AGA_STOP);
                                ffiringTime = ffireTime - fstartTime + ffiringTime;
                            }
                        }
                        FiringTimer.Reset(ffiringTime);
                    }
                }
                if (FiringTimer.IsSet() && FiringTimer.Check()) {
                    StopFiring();
                    CooldownTimer.Reset(fAutomaticWeaponBurstCooldownTime.Float());
                    this->eState = CombatProcedureAttackRanged::ATTACK_STATE_COOLDOWN;
                }
            }
        }
        else if (ecurrentState == CombatProcedureAttackRanged::ATTACK_STATE_COOLDOWN) {
            if (CooldownTimer.Check()) {
                pcombatState->ClearFiringStartedTimeStamp();
                StartFiring(bautomaticWeapon);
                this->eState = CombatProcedureAttackRanged::ATTACK_STATE_FIRING;
            }
        }
        else {
            SuppressiveFireTimer.Clear();
            StartFiring(bautomaticWeapon);
            this->eState = CombatProcedureAttackRanged::ATTACK_STATE_FIRING;
        }
    }
    else {
        switch (this->eState) {
            case CombatProcedureAttackRanged::ATTACK_STATE_IDLE: {
                if (bcanAttack && !SuppressiveFireTimer.Check()) {
                    StartFiring(bautomaticWeapon);
                    this->eState = CombatProcedureAttackRanged::ATTACK_STATE_SUPPRESSIVE_FIRE;
                }
                break;
            }
            case CombatProcedureAttackRanged::ATTACK_STATE_FIRING: {
                StartFiring(bautomaticWeapon);
                this->eState = CombatProcedureAttackRanged::ATTACK_STATE_SUPPRESSIVE_FIRE;
                SuppressiveFireTimer.Reset(2.0f);
                break;
            }
            case CombatProcedureAttackRanged::ATTACK_STATE_COOLDOWN: {
                if (CooldownTimer.Check()) {
                    StartFiring(bautomaticWeapon);
                    this->eState = CombatProcedureAttackRanged::ATTACK_STATE_SUPPRESSIVE_FIRE;
                    SuppressiveFireTimer.Reset(2.0f);
                }
                break;
            }
            case CombatProcedureAttackRanged::ATTACK_STATE_SUPPRESSIVE_FIRE: {
                if (SuppressiveFireTimer.Check()) {
                    StopFiring();
                    this->eState = CombatProcedureAttackRanged::ATTACK_STATE_IDLE;
                }
                break;
            }
            default: {
                break;
            }
        }
    }
    if (ptarget && pcombatState->GetHasBufferedDetection()) {
        pCombatDialogueManager->StartDialogue(pCombatController, DT_COMBAT, DC_ATTACK, false);
    }

    bool buseIronSights = false;
    if (ptarget) {
        float fironSightsDistance = pweapon->GetIronSightUseMult() * fCombatIronSightsDistance.Float();
        const NiPoint3* targetActorPos = &ptarget->GetLocationOnReference();
        NiPoint3 targetVec = pattacker->GetLocationOnReference() - *targetActorPos;
        double dtargetDistSquared = targetVec.SqrLength();
        if (fironSightsDistance * fironSightsDistance < dtargetDistSquared) {
            buseIronSights = true;
        }
    }
    pCombatController->SetUseIronSights(buseIronSights);
    if (pweapon->GetAimArc() > 0.0) {
        float ffiringArc = pweapon->GetAimArc() * 0.01745329238474369;
        if (!pattacker->IsPathing()) {
            ffiringArc = ffiringArc * fCombatFiringArcStationaryTurnMult.Float();
        }
        if (TargetedPoint != NiPoint3::ZERO &&
            CombatUtilities::IsTargetWithinFiringArc(
                pattacker,
                TargetedPoint,
                ffiringArc,
                std::numeric_limits<float>::max()
            )) {
            pCombatController->SetIgnoreLookAtTarget(4);
        }
        else {
            pCombatController->SetIgnoreLookAtTarget(0);
        }
    }
    if (this->eState == CombatProcedureAttackRanged::ATTACK_STATE_IDLE) {
        if (ptarget &&
            !pcombatState->bTargetSpaceDifferent &&
            (pcombatState->GetHasTarget360LineOfSight() ||
             (pcombatState->GetHasBuffered360LineOfSight() && pcombatState->GetHasBufferedDetection())) &&
            (TargetedPoint != NiPoint3::ZERO || pcombatState->GetLastSeenTargetFiringLocation(TargetedPoint))) {
            SetLookAtTarget(TargetedPoint, 2);
        }
        else {
            ClearLookAtTarget();
        }
        return;
    }
    if (TargetedPoint == NiPoint3::ZERO) {
        TargetedPoint = targetPoint;
    }
    if (TargetedPoint != NiPoint3::ZERO) {
        SetLookAtTarget(TargetedPoint, 4);
    }
    if (this->eState != CombatProcedureAttackRanged::ATTACK_STATE_COOLDOWN &&
        (AimAtTarget() || btooCloseToTarget) &&
        FireDelayTimer.Check()) {
        bool battack = false;
        uint8_t cshotsToFire = 1;
        if (bautomaticWeapon) {
            bool bweaponReadyToFire = true;
            cshotsToFire = pattacker->pCurrentProcess->GetAutomaticShotsToFire(pattacker);
            uint16_t eanimGroup = panim->GetGroup(AGS_WEAPON);
            if (TESAnimGroup::IsAttackSpinAction(eanimGroup)) {
                ANIM_GROUP_ACTION eaction = panim->GetGroupAction(AGS_WEAPON);
                if (eaction != AGA_STOP) {
                    bweaponReadyToFire = false;
                }
            }
            if (bweaponReadyToFire && cshotsToFire) {
                battack = true;
            }
        }
        else {
            ANIMATION_ACTION eanimAction = pattacker->pCurrentProcess->GetAnimAction();
            if (eanimAction == ANIM_ACTION_NONE) {
                battack = true;
            }
            else if (
                eanimAction == ANIM_ACTION_ATTACK_FOLLOW_THROUGH &&
                panim->GetGroupAction(AGS_WEAPON) == AGA_SPECIAL_IDLE_STOP
            ) {
                battack = true;
            }
        }
        if (battack) {
            phitRef = nullptr;
            fdistanceFraction = 0.0f;
            firingLocation = CombatUtilities::GetBackProjectedAttackerFiringLocation(pattacker);
            bool bprojectilePickHit = CombatUtilities::CalculateProjectileLOS(
                pattacker,
                pprojectile,
                0.0f,
                firingLocation,
                targetPoint,
                &hitPoint,
                &phitRef,
                &fdistanceFraction
            );
            if (bprojectilePickHit) {
                bool battackAnyway = false;
                CombatProcedureAttackRanged::ATTACK_STATE ecurrentState = this->eState;
                if (ecurrentState == CombatProcedureAttackRanged::ATTACK_STATE_FIRING) {
                    if (phitRef && phitRef->IsActor()) {
                        Actor* phitActor = static_cast<Actor*>(phitRef);
                        if (phitActor == GetTarget() || pcombatGroup->IsTarget(phitActor)) {
                            battackAnyway = true;
                        }
                    }

                    if (!battackAnyway) {
                        BGSExplosion* pexplosion = pprojectile->GetExplosion();
                        if (pexplosion) {
                            NiPoint3 missVector = hitPoint - targetPoint;
                            float fmissDistance = missVector.Length();
                            double dradius = pexplosion->GetRadiusBSUnits();
                            if (fmissDistance <= dradius) {
                                battackAnyway = true;
                            }
                        }
                    }
                }
                else if (ecurrentState == CombatProcedureAttackRanged::ATTACK_STATE_SUPPRESSIVE_FIRE) {
                    if (phitRef && phitRef->IsActor()) {
                        if (phitRef &&
                            phitRef->IsActor() &&
                            !phitRef->IsDead(false) &&
                            pcombatGroup->IsTarget(static_cast<Actor*>(phitRef))) {
                            battackAnyway = true;
                        }
                    }
                    else if (fdistanceFraction > 0.5) {
                        battackAnyway = true;
                    }
                    if (!battackAnyway) {
                        this->eState = CombatProcedureAttackRanged::ATTACK_STATE_IDLE;
                        StopFiring();
                        SuppressiveFireTimer.Clear();
                        return;
                    }
                }
                if (!battackAnyway) {
                    if (!pCombatController->pCombatState->bTargetBlocked) {
                        pCombatController->pCombatState->SetTargetBlocked(true);
                    }
                    return;
                }
                pCombatController->pCombatState->SetTargetBlocked(false);
            }
            BGSExplosion* pexplosion = pprojectile->GetExplosion();
            if (pexplosion) {
                NiPoint3 projectileImpactPoint = TargetedPoint;
                if (bprojectilePickHit) {
                    projectileImpactPoint = hitPoint;
                }
                float ftime = CombatUtilities::ComputeProjectileTime_(
                    pprojectile,
                    firingLocation,
                    projectileImpactPoint,
                    false,
                    0.0f
                );
                if (pCombatManager->CheckExplosionAttack(pattacker, pexplosion, projectileImpactPoint, ftime)) {
                    pCombatController->pCombatState->pExplosiveProjectileBlocked = nullptr;
                    ExplosiveProjectileBlockedTimer.Clear();
                }
                else {
                    battack = false;
                    if (ExplosiveProjectileBlockedTimer.IsSet()) {
                        if (ExplosiveProjectileBlockedTimer.Check()) {
                            pCombatController->pCombatState->pExplosiveProjectileBlocked = pexplosion;
                        }
                    }
                    else {
                        ExplosiveProjectileBlockedTimer.Reset(fExplosiveProjectileBlockedWaitTime.Float());
                    }
                }
            }
            if (battack) {
                switch (panim->FindSwitchKey(nullptr)) {
                    case 51: {
                        eattack = ANIM_GROUP_ATTACK_3;
                        break;
                    }
                    case 52: {
                        eattack = ANIM_GROUP_ATTACK_4;
                        break;
                    }
                    case 53: {
                        eattack = ANIM_GROUP_ATTACK_5;
                        break;
                    }
                    case 54: {
                        eattack = ANIM_GROUP_ATTACK_6;
                        break;
                    }
                    case 55: {
                        eattack = ANIM_GROUP_ATTACK_7;
                        break;
                    }
                    case 56: {
                        eattack = ANIM_GROUP_ATTACK_8;
                        break;
                    }
                    case 108: {
                        eattack = ANIM_GROUP_ATTACK_LEFT;
                        break;
                    }
                    default: {
                        if (!pweapon || pweapon->GetAttackAnim() == ANIM_GROUP_NONE) {
                            eattack = ANIM_GROUP_ATTACK_RIGHT;
                        }
                        else {
                            eattack = pweapon->GetAttackAnim();
                        }
                        break;
                    }
                }
                pattacker->pCurrentProcess->SetShotsToFire(cshotsToFire);
                if (pattacker->QueueAttack(eattack) && !bautomaticWeapon) {
                    double dweaponFireDelayMin = pweapon->GetSemiAutomaticFireDelayMin();
                    float fsemiAutoFireDelayMin = pCombatController->GetCurrentCombatStyle()
                                                      ->GetSemiAutoFiringDelayMinMult() *
                        dweaponFireDelayMin;
                    double dweaponFireDelayMax = pweapon->GetSemiAutomaticFireDelayMax();
                    float fsemiAutoFireDelayMax = pCombatController->GetCurrentCombatStyle()
                                                      ->SimpleData.fSemiAutoFiringDelayMaxMult *
                        dweaponFireDelayMax;
                    float ffireDelay = RandomFloat(fsemiAutoFireDelayMin, fsemiAutoFireDelayMax);
                    FireDelayTimer.Reset(ffireDelay);
                }
                if (pexplosion && CombatUtilities::IsDangerousExplosion(pexplosion)) {
                    pCombatDialogueManager->StartDialogue(pCombatController, DT_COMBAT, DC_FIRE_EXPLOSIVE, true);
                }
            }
        }
    }
}

// The attacker is fetched and not used.
void CombatProcedureAttackRanged::Shutdown() {
    GetAttacker();
    ClearLookAtTarget();
    pCombatController->SetIgnoreLookAtTarget(0);
    StopFiring();
    pCombatController->SetUseIronSights(-1);
}

// Each timer line passes the target time (read first) and the elapsed time as doubles.
void CombatProcedureAttackRanged::DisplayDebugText(
    IDebugText* apDebugText,
    DEBUGTEXT_LOCATION aeLocation,
    DEBUGTEXT_JUSTIFY aeJustify
) {
    Actor* pattacker = GetAttacker();
    switch (eState) {
        case CombatProcedureAttackRanged::ATTACK_STATE_IDLE: {
            apDebugText->PrintLine(aeLocation, aeJustify, "IDLE");
            break;
        }
        case CombatProcedureAttackRanged::ATTACK_STATE_FIRING: {
            apDebugText->PrintLine(aeLocation, aeJustify, "FIRING");
            if (FiringTimer.IsSet()) {
                double dtarget = FiringTimer.GetTargetTime();
                double delapsed = FiringTimer.ElapsedTime();
                apDebugText->PrintLine(aeLocation, aeJustify, "Firing Timer: %.02f/%.02f", delapsed, dtarget);
            }
            if (GetTarget()) {
                apDebugText->PrintLine(
                    aeLocation,
                    aeJustify,
                    "Target location: %.02f, %.02f, %.02f",
                    TargetedPoint.x,
                    TargetedPoint.y,
                    TargetedPoint.z
                );
            }
            if (!FireDelayTimer.Check()) {
                double dtarget = FireDelayTimer.GetTargetTime();
                double delapsed = FireDelayTimer.ElapsedTime();
                apDebugText->PrintLine(aeLocation, aeJustify, "Firing Delay Timer: %.02f/%.02f", delapsed, dtarget);
            }
            break;
        }
        case CombatProcedureAttackRanged::ATTACK_STATE_COOLDOWN: {
            apDebugText->PrintLine(aeLocation, aeJustify, "COOLDOWN");
            double dtarget = CooldownTimer.GetTargetTime();
            double delapsed = CooldownTimer.ElapsedTime();
            apDebugText->PrintLine(aeLocation, aeJustify, "Cooldown Timer: %.02f/%.02f", delapsed, dtarget);
            break;
        }
        case CombatProcedureAttackRanged::ATTACK_STATE_SUPPRESSIVE_FIRE: {
            apDebugText->PrintLine(aeLocation, aeJustify, "SUPPRESSIVE_FIRE");
            apDebugText->PrintLine(
                aeLocation,
                aeJustify,
                "Suppression location: %.02f, %.02f, %.02f",
                TargetedPoint.x,
                TargetedPoint.y,
                TargetedPoint.z
            );
            double dtarget = SuppressiveFireTimer.GetTargetTime();
            double delapsed = SuppressiveFireTimer.ElapsedTime();
            apDebugText->PrintLine(aeLocation, aeJustify, "Suppressive Fire Timer: %.02f/%.02f", delapsed, dtarget);
            if (!FireDelayTimer.Check()) {
                double ddelayTarget = FireDelayTimer.GetTargetTime();
                double ddelayElapsed = FireDelayTimer.ElapsedTime();
                apDebugText
                    ->PrintLine(aeLocation, aeJustify, "Firing Delay Timer: %.02f/%.02f", ddelayElapsed, ddelayTarget);
            }
            break;
        }
        default: {
            break;
        }
    }
    if (ExplosiveProjectileBlockedTimer.IsSet()) {
        double dtarget = ExplosiveProjectileBlockedTimer.GetTargetTime();
        double delapsed = ExplosiveProjectileBlockedTimer.ElapsedTime();
        apDebugText
            ->PrintLine(aeLocation, aeJustify, "Explosive Projectile Blocked Timer: %.02f/%.02f", delapsed, dtarget);
    }
    TESObjectWEAP* pweapon = pCombatController->GetCurrentWeapon();
    if (TargetedPoint != NiPoint3::ZERO && pweapon && pweapon->GetAimArc() > 0.0) {
        NiPoint3 rot = CombatUtilities::GetAngleToProjectedPoint(pattacker, TargetedPoint);
        float fheading = pattacker->GetActualHeading();
        rot.z = ClampAngle(rot.z);
        float fdir = 0.0f;
        float fangleDelta = GetAngleDelta(fheading, rot.z, fdir);
        float fdelta = std::fabs(fangleDelta);
        float ffiringArc = pweapon->GetAimArc();
        if (pattacker->IsPathing()) {
            apDebugText->PrintLine(
                aeLocation,
                aeJustify,
                "Weapon firing arc: ( %.2f/%.2f ) degrees (Moving)",
                (fdelta + fdelta) * 57.2957763671875,
                ffiringArc
            );
        }
        else {
            ffiringArc = ffiringArc * fCombatFiringArcStationaryTurnMult.Float();
            apDebugText->PrintLine(
                aeLocation,
                aeJustify,
                "Weapon firing arc: ( %.2f/%.2f ) degrees (Stationary)",
                (fdelta + fdelta) * 57.2957763671875,
                ffiringArc
            );
        }
    }
}

// The state is compared signed.
bool CombatProcedureAttackRanged::GetTargetedPoint(NiPoint3& arTargetedPoint, NiPoint3& arReleasePoint) {
    int32_t eCurrentState = static_cast<int32_t>(eState);
    if (eCurrentState <= 0) {
        return false;
    }
    if (eCurrentState <= 2) {
        TargetedPoint = CalculateTargetedPoint(&arReleasePoint);
        arTargetedPoint = TargetedPoint;
        return true;
    }
    if (eCurrentState == 3) {
        arTargetedPoint = TargetedPoint;
        return true;
    }
    return false;
}

void CombatProcedureAttackRanged::Disarmed(Actor* apOpponent) {
    SetFailed("Disarmed");
}

CombatProcedure::COMBAT_PROCEDURES CombatProcedureAttackRanged::GetType() {
    return ATTACK_RANGED;
}

// Saves the procedure's data, the state and targeted point, then the timers, in the game's save order.
void CombatProcedureAttackRanged::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    CombatProcedure::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eState), sizeof(eState), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&TargetedPoint), sizeof(TargetedPoint), 0);
    ExplosiveProjectileBlockedTimer.SaveGame(apSaveGameBuffer);
    CooldownTimer.SaveGame(apSaveGameBuffer);
    SuppressiveFireTimer.SaveGame(apSaveGameBuffer);
    FiringTimer.SaveGame(apSaveGameBuffer);
    FireDelayTimer.SaveGame(apSaveGameBuffer);
}

// Save form version 10 added the fire delay timer.
void CombatProcedureAttackRanged::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&eState), sizeof(eState));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&TargetedPoint), sizeof(TargetedPoint));
    ExplosiveProjectileBlockedTimer.LoadGame(apLoadGameBuffer);
    CooldownTimer.LoadGame(apLoadGameBuffer);
    SuppressiveFireTimer.LoadGame(apLoadGameBuffer);
    FiringTimer.LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetVersion() >= 10) {
        FireDelayTimer.LoadGame(apLoadGameBuffer);
    }
}

void CombatProcedureAttackRanged::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::InitLoadGame(apLoadGameBuffer);
}

// The arc is the weapon's aim arc, at least 15 degrees, in radians; it is both the firing and the looking arc.
bool CombatProcedureAttackRanged::AimAtTarget() {
    if (TargetedPoint == NiPoint3::ZERO) {
        return false;
    }
    Actor* pAttacker = GetAttacker();
    TESObjectWEAP* pWeapon = pCombatController->GetCurrentWeapon();
    float fArc = static_cast<float>(BSMAX(pWeapon->GetAimArc(), 15.0f) * 0.01745329238474369);
    return CombatUtilities::IsTargetWithinFiringArc(pAttacker, TargetedPoint, fArc, fArc);
}

// Ends the attacker's firing: sustained fire off, the automatic shot count cleared.
void CombatProcedureAttackRanged::StopFiring() {
    Actor* pattacker = pCombatController->GetAttacker();
    pattacker->GetCurrentAIProcess()->SetSustainedFire(false);
    pattacker->GetCurrentAIProcess()->ClearAutomaticShotsToFire(false);
    FiringTimer.Clear();
}

NiPoint3 CombatProcedureAttackRanged::CalculateTargetedPoint(NiPoint3* apReleasePoint) {
    NiPoint3 kTargetedPoint;
    Actor* ptarget = GetTarget();
    if (ptarget) {
        TESObjectWEAP* pweapon = pCombatController->GetCurrentWeapon();
        Actor* pattacker = GetAttacker();
        BGSProjectile* pprojectile = pweapon->GetProjectile(pattacker);
        NiPoint3 kProjectedPoint = CombatUtilities::GetProjectedPointForTarget(
            GetAttacker(),
            ptarget,
            apReleasePoint,
            pprojectile,
            static_cast<float>(0.0),
            false
        );
        kTargetedPoint = kProjectedPoint;
    }
    else {
        kTargetedPoint = NiPoint3::ZERO;
    }
    return kTargetedPoint;
}

// Starts the attacker's firing: sustained fire when abAutomatic, the automatic shot count cleared.
void CombatProcedureAttackRanged::StartFiring(bool abAutomatic) {
    Actor* pattacker = pCombatController->GetAttacker();
    pattacker->GetCurrentAIProcess()->SetSustainedFire(abAutomatic);
    pattacker->GetCurrentAIProcess()->ClearAutomaticShotsToFire(true);
    FiringTimer.Clear();
}

// Returns this after storing the constructed time stamp.
CombatState* CombatState::ClearFiringStartedTimeStamp() {
    CombatTimeStamp kTimeStamp(-std::numeric_limits<float>::max());
    fFiringStartedTimeStamp = kTimeStamp;
    return this;
}

CombatTimeStamp CombatState::GetFiringStartedTimeStamp() {
    return fFiringStartedTimeStamp;
}

void CombatController::SetIgnoreLookAtTarget(uint8_t acPriority) {
    ucIgnoreLookAtTarget = acPriority;
}

void CombatController::SetUseIronSights(char acValue) {
    cUseIronSights = acValue;
}
