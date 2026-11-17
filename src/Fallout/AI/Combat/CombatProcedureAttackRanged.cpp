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

CombatProcedureAttackRanged::CombatProcedureAttackRanged() : eState(ATTACK_STATE_IDLE), TargetedPoint(NiPoint3_ZERO) {}

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
    ANIM_GROUP_ENUM eattack_2 = ANIM_GROUP_NONE;
    TESObjectWEAP* pweapon = pCombatController->GetCurrentWeapon();
    Animation* panim = pattacker->GetAnimation();
    if (!pweapon || (!panim || !pweapon->GetProjectile(pattacker))) {
        SetFailed("Missing either weapon, animation, or weapon projectile");
        return;
    }
    TESCombatStyle* CurrentCombatStyle = pCombatController->GetCurrentCombatStyle();
    float ftargetingFOV = CurrentCombatStyle->SimpleData.fTargetingFOV;
    if (ftargetingFOV > 0.0) {
        if (ptarget) {
            float v32 = ftargetingFOV * 0.01745329238474369;
            const NiPoint3* v2 = &ptarget->GetLocationOnReference();
            if (!CombatUtilities::IsTargetWithinFiringArc(pattacker, *v2, v32, 6.2831855f)) {
                SetFailed("Target outside targeting FOV");
                return;
            }
        }
    }
    BGSProjectile* pprojectile = pweapon->GetProjectile(pattacker);
    bool bautomaticWeapon = pweapon->IsAutomaticWeapon();
    bool v31 = bautomaticWeapon || pweapon->GetBurstShot();
    bool btooCloseToTarget = 0;
    NiPoint3 targetPoint = CalculateTargetedPoint(0);
    BaseProcess* CurrentAIProcess = pattacker->pCurrentProcess;
    bool bCanAttack = CurrentAIProcess->CanAttack() || pattacker->IsAttacking();
    bool bcanAttack = bCanAttack;
    if (pCombatController->IsSwitchingWeapons()) {
        bcanAttack = 0;
    }
    if (this->eState) {
        if (!bcanAttack) {
            StopFiring();
            this->eState = CombatProcedureAttackRanged::ATTACK_STATE_IDLE;
        }
    }
    if (bcanAttack) {
        float fminRange = 0.0f;
        float fmaxRange = 0.0f;
        float fabsoluteMaxRange = 0.0f;
        CombatUtilities::ComputeOptimalWeaponRanges(
            pCombatController, pweapon, fminRange, fmaxRange, fabsoluteMaxRange
        );
        NiPoint3 targetPointModified = targetPoint;
        if (ptarget) {
            targetPointModified.z = (&ptarget->GetLocationOnReference())->z;
        }
        const NiPoint3* v3 = &pattacker->GetLocationOnReference();
        NiPoint3 vec = targetPointModified - *v3;
        float targetDist_2 = vec.SqrLength();
        if (targetDist_2 > fabsoluteMaxRange * fabsoluteMaxRange) {
            bcanAttack = 0;
        }
        if (targetDist_2 < fminRange * fminRange) {
            btooCloseToTarget = 1;
        }
    }
    if (pcombatState->GetHasTarget360LineOfSight()) {
        TargetedPoint = targetPoint;
    }
    if (pcombatState->GetHasTarget360LineOfSight() && (pcombatState->GetHasBufferedDetection() && bcanAttack)) {
        CombatProcedureAttackRanged::ATTACK_STATE eState = this->eState;
        if (eState == CombatProcedureAttackRanged::ATTACK_STATE_FIRING) {
            if (v31) {
                if (!FiringTimer.IsSet()) {
                    if (-3.4028235e+38f != static_cast<float>(pcombatState->GetFiringStartedTimeStamp())) {
                        float ffiringTime = fAutomaticWeaponBurstFireTime.Float();
                        uint16_t eanimGroup = panim->GetGroup(AGS_WEAPON);
                        if (TESAnimGroup::GetType(eanimGroup) == ANIM_GROUP_ATTACK_SPIN ||
                            TESAnimGroup::GetType(eanimGroup) == ANIM_GROUP_ATTACK_SPIN_2) {
                            AnimSequenceBase* panimSeq;
                            uint16_t a3 = eanimGroup;
                            NiTPointerMap<uint16_t, AnimSequenceBase*>* MapAnimSequenceBase =
                                panim->GetMapAnimSequenceBase();
                            if (MapAnimSequenceBase->GetAt(a3, panimSeq)) {
                                BSAnimGroupSequence* v6 = panimSeq->GetSequenceToPlay(-1);
                                TESAnimGroup* panimGroup = v6->GetAnimGroup();
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
                    float* v7 = &fAutomaticWeaponBurstCooldownTime.Float();
                    CooldownTimer.Reset(*v7);
                    this->eState = CombatProcedureAttackRanged::ATTACK_STATE_COOLDOWN;
                }
            }
        }
        else {
            if (eState == CombatProcedureAttackRanged::ATTACK_STATE_COOLDOWN) {
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
    if (ptarget) {
        if (pcombatState->GetHasBufferedDetection()) {
            pCombatDialogueManager->StartDialogue(pCombatController, DT_COMBAT, DC_ATTACK, false);
        }
    }
    bool buseIronSights = 0;
    if (ptarget) {
        float* v8 = &fCombatIronSightsDistance.Float();
        float fironSightsDistance = pweapon->GetIronSightUseMult() * *v8;
        const NiPoint3* targetActorPos = &ptarget->GetLocationOnReference();
        const NiPoint3* v9 = &pattacker->GetLocationOnReference();
        NiPoint3 targetVec = *v9 - *targetActorPos;
        double v10 = targetVec.SqrLength();
        if (fironSightsDistance * fironSightsDistance < v10) {
            buseIronSights = 1;
        }
    }
    pCombatController->SetUseIronSights(buseIronSights);
    if (pweapon->GetAimArc() > 0.0) {
        float ffiringArc = pweapon->GetAimArc() * 0.01745329238474369;
        if (!pattacker->IsPathing()) {
            float* v11 = &fCombatFiringArcStationaryTurnMult.Float();
            ffiringArc = ffiringArc * *v11;
        }
        if (TargetedPoint.operator!=(NiPoint3_ZERO) &&
            CombatUtilities::IsTargetWithinFiringArc(pattacker, TargetedPoint, ffiringArc, 3.4028235e+38f)) {
            pCombatController->SetIgnoreLookAtTarget(4);
        }
        else {
            pCombatController->SetIgnoreLookAtTarget(0);
        }
    }
    if (this->eState == CombatProcedureAttackRanged::ATTACK_STATE_IDLE) {
        if (ptarget &&
            (!pcombatState->bTargetSpaceDifferent &&
             ((pcombatState->GetHasTarget360LineOfSight() ||
               (pcombatState->GetHasBuffered360LineOfSight() && pcombatState->GetHasBufferedDetection())) &&
              (TargetedPoint.operator!=(NiPoint3_ZERO) ||
               pcombatState->GetLastSeenTargetFiringLocation(TargetedPoint))))) {
            SetLookAtTarget(TargetedPoint, 2);
        }
        else {
            ClearLookAtTarget();
        }
        return;
    }
    if (TargetedPoint.operator==(NiPoint3_ZERO)) {
        TargetedPoint = targetPoint;
    }
    if (TargetedPoint.operator!=(NiPoint3_ZERO)) {
        SetLookAtTarget(TargetedPoint, 4);
    }
    if (this->eState != CombatProcedureAttackRanged::ATTACK_STATE_COOLDOWN && (AimAtTarget() || btooCloseToTarget) &&
        FireDelayTimer.Check()) {
        bool battack = 0;
        uint8_t cshotsToFire = 1;
        if (bautomaticWeapon) {
            bool bweaponReadyToFire = 1;
            BaseProcess* v27 = pattacker->pCurrentProcess;
            cshotsToFire = v27->GetAutomaticShotsToFire(pattacker);
            uint16_t Group = panim->GetGroup(AGS_WEAPON);
            if (TESAnimGroup::IsAttackSpinAction(Group)) {
                ANIM_GROUP_ACTION eaction = panim->GetGroupAction(AGS_WEAPON);
                if (eaction != AGA_STOP) {
                    bweaponReadyToFire = 0;
                }
            }
            if (bweaponReadyToFire) {
                if (cshotsToFire) {
                    battack = 1;
                }
            }
        }
        else {
            BaseProcess* v26 = pattacker->pCurrentProcess;
            ANIMATION_ACTION v25 = v26->GetAnimAction();
            if (v25 == ANIM_ACTION_NONE) {
                battack = 1;
            }
            else {
                if (v25 == ANIM_ACTION_ATTACK_FOLLOW_THROUGH &&
                    panim->GetGroupAction(AGS_WEAPON) == AGA_SPECIAL_IDLE_STOP) {
                    battack = 1;
                }
            }
        }
        if (battack) {
            phitRef = 0;
            Actor* apActor = 0;
            fdistanceFraction = 0.0f;
            CombatUtilities::GetBackProjectedAttackerFiringLocation(&firingLocation, pattacker);
            bool bprojectilePickHit = CombatUtilities::CalculateProjectileLOS(
                pattacker, pprojectile, 0.0f, firingLocation, targetPoint, &hitPoint, &phitRef, &fdistanceFraction
            );
            if (!bprojectilePickHit) {
            LABEL_141:;
                BGSExplosion* pexplosion_2 = pprojectile->GetExplosion();
                if (pexplosion_2) {
                    NiPoint3 projectileImpactPoint = TargetedPoint;
                    if (bprojectilePickHit) {
                        projectileImpactPoint = hitPoint;
                    }
                    float ftime = CombatUtilities::ComputeProjectileTime_(
                        pprojectile, firingLocation, projectileImpactPoint, 0, 0.0f
                    );
                    if (pCombatManager->CheckExplosionAttack(pattacker, pexplosion_2, projectileImpactPoint, ftime)) {
                        pCombatController->pCombatState->pExplosiveProjectileBlocked = 0;
                        ExplosiveProjectileBlockedTimer.Clear();
                    }
                    else {
                        battack = 0;
                        if (ExplosiveProjectileBlockedTimer.IsSet()) {
                            if (ExplosiveProjectileBlockedTimer.Check()) {
                                pCombatController->pCombatState->pExplosiveProjectileBlocked = pexplosion_2;
                            }
                        }
                        else {
                            float* v15 = &fExplosiveProjectileBlockedWaitTime.Float();
                            ExplosiveProjectileBlockedTimer.Reset(*v15);
                        }
                    }
                }
                if (battack) {
                    switch (panim->FindSwitchKey(0)) {
                        case 51: {
                            eattack_2 = ANIM_GROUP_ATTACK_3;
                            break;
                        }
                        case 52: {
                            eattack_2 = ANIM_GROUP_ATTACK_4;
                            break;
                        }
                        case 53: {
                            eattack_2 = ANIM_GROUP_ATTACK_5;
                            break;
                        }
                        case 54: {
                            eattack_2 = ANIM_GROUP_ATTACK_6;
                            break;
                        }
                        case 55: {
                            eattack_2 = ANIM_GROUP_ATTACK_7;
                            break;
                        }
                        case 56: {
                            eattack_2 = ANIM_GROUP_ATTACK_8;
                            break;
                        }
                        case 108: {
                            eattack_2 = ANIM_GROUP_ATTACK_LEFT;
                            break;
                        }
                        default: {
                            if (!pweapon || pweapon->GetAttackAnim() == ANIM_GROUP_NONE) {
                                eattack_2 = ANIM_GROUP_ATTACK_RIGHT;
                            }
                            else {
                                eattack_2 = pweapon->GetAttackAnim();
                            }
                            break;
                        }
                    }
                    BaseProcess* v23 = pattacker->pCurrentProcess;
                    v23->SetShotsToFire(cshotsToFire);
                    if (pattacker->QueueAttack(eattack_2) && !bautomaticWeapon) {
                        double SemiAutomaticFireDelayMin = pweapon->GetSemiAutomaticFireDelayMin();
                        TESCombatStyle* v16 = pCombatController->GetCurrentCombatStyle();
                        float fsemiAutoFireDelayMin = v16->GetSemiAutoFiringDelayMinMult() * SemiAutomaticFireDelayMin;
                        double SemiAutomaticFireDelayMax = pweapon->GetSemiAutomaticFireDelayMax();
                        TESCombatStyle* v17 = pCombatController->GetCurrentCombatStyle();
                        float fsemiAutoFireDelayMax =
                            v17->SimpleData.fSemiAutoFiringDelayMaxMult * SemiAutomaticFireDelayMax;
                        float targetActorPosa = RandomFloat(fsemiAutoFireDelayMin, fsemiAutoFireDelayMax);
                        FireDelayTimer.Reset(targetActorPosa);
                    }
                    if (pexplosion_2) {
                        if (CombatUtilities::IsDangerousExplosion(pexplosion_2)) {
                            pCombatDialogueManager->StartDialogue(
                                pCombatController, DT_COMBAT, DC_FIRE_EXPLOSIVE, true
                            );
                        }
                    }
                }
                return;
            }
            bool battackAnyway = 0;
            CombatProcedureAttackRanged::ATTACK_STATE v24 = this->eState;
            if (v24 == CombatProcedureAttackRanged::ATTACK_STATE_FIRING) {
                if (phitRef) {
                    if (phitRef->IsActor()) {
                        Actor* phitActor = static_cast<Actor*>(phitRef);
                        Actor* ptarget_2 = GetTarget();
                        if (phitActor == ptarget_2 || pcombatGroup->IsTarget(phitActor)) {
                            battackAnyway = 1;
                        }
                    }
                }
                if (!battackAnyway) {
                    BGSExplosion* pexplosion = pprojectile->GetExplosion();
                    if (pexplosion) {
                        NiPoint3 missVector = hitPoint - targetPoint;
                        float fmissDistance = missVector.Length();
                        double RadiusBSUnits = pexplosion->GetRadiusBSUnits();
                        if (fmissDistance <= RadiusBSUnits) {
                            battackAnyway = 1;
                        }
                    }
                }
            }
            else {
                if (v24 == CombatProcedureAttackRanged::ATTACK_STATE_SUPPRESSIVE_FIRE) {
                    if (phitRef && phitRef->IsActor()) {
                        if (phitRef) {
                            if (phitRef->IsActor() && !phitRef->IsDead(0) &&
                                pcombatGroup->IsTarget(static_cast<Actor*>(phitRef))) {
                                battackAnyway = 1;
                            }
                        }
                    }
                    else {
                        if (fdistanceFraction > 0.5) {
                            battackAnyway = 1;
                        }
                    }
                    if (!battackAnyway) {
                        this->eState = CombatProcedureAttackRanged::ATTACK_STATE_IDLE;
                        StopFiring();
                        SuppressiveFireTimer.Clear();
                        return;
                    }
                }
            }
            if (battackAnyway) {
                pCombatController->pCombatState->SetTargetBlocked(0);
                goto LABEL_141;
            }
            if (!pCombatController->pCombatState->bTargetBlocked) {
                pCombatController->pCombatState->SetTargetBlocked(1);
            }
        }
    }
}

// The attacker is fetched and not used, as the game does.
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
    NiPoint3 rot;
    Actor* pattacker = GetAttacker();
    switch (eState) {
        case CombatProcedureAttackRanged::ATTACK_STATE_IDLE: {
            apDebugText->PrintLine(aeLocation, aeJustify, "IDLE");
            break;
        }
        case CombatProcedureAttackRanged::ATTACK_STATE_FIRING: {
            apDebugText->PrintLine(aeLocation, aeJustify, "FIRING");
            if (FiringTimer.IsSet()) {
                double ftarget = FiringTimer.GetTargetTime();
                double felapsed = FiringTimer.ElapsedTime();
                apDebugText->PrintLine(aeLocation, aeJustify, "Firing Timer: %.02f/%.02f", felapsed, ftarget);
            }
            if (GetTarget()) {
                apDebugText->PrintLine(
                    aeLocation, aeJustify, "Target location: %.02f, %.02f, %.02f", TargetedPoint.x, TargetedPoint.y,
                    TargetedPoint.z
                );
            }
            if (!FireDelayTimer.Check()) {
                double ftarget = FireDelayTimer.GetTargetTime();
                double felapsed = FireDelayTimer.ElapsedTime();
                apDebugText->PrintLine(aeLocation, aeJustify, "Firing Delay Timer: %.02f/%.02f", felapsed, ftarget);
            }
            break;
        }
        case CombatProcedureAttackRanged::ATTACK_STATE_COOLDOWN: {
            apDebugText->PrintLine(aeLocation, aeJustify, "COOLDOWN");
            double ftarget = CooldownTimer.GetTargetTime();
            double felapsed = CooldownTimer.ElapsedTime();
            apDebugText->PrintLine(aeLocation, aeJustify, "Cooldown Timer: %.02f/%.02f", felapsed, ftarget);
            break;
        }
        case CombatProcedureAttackRanged::ATTACK_STATE_SUPPRESSIVE_FIRE: {
            apDebugText->PrintLine(aeLocation, aeJustify, "SUPPRESSIVE_FIRE");
            apDebugText->PrintLine(
                aeLocation, aeJustify, "Suppression location: %.02f, %.02f, %.02f", TargetedPoint.x, TargetedPoint.y,
                TargetedPoint.z
            );
            double ftarget = SuppressiveFireTimer.GetTargetTime();
            double felapsed = SuppressiveFireTimer.ElapsedTime();
            apDebugText->PrintLine(aeLocation, aeJustify, "Suppressive Fire Timer: %.02f/%.02f", felapsed, ftarget);
            if (!FireDelayTimer.Check()) {
                double fdelayTarget = FireDelayTimer.GetTargetTime();
                double fdelayElapsed = FireDelayTimer.ElapsedTime();
                apDebugText->PrintLine(
                    aeLocation, aeJustify, "Firing Delay Timer: %.02f/%.02f", fdelayElapsed, fdelayTarget
                );
            }
            break;
        }
        default: {
            break;
        }
    }
    if (ExplosiveProjectileBlockedTimer.IsSet()) {
        double ftarget = ExplosiveProjectileBlockedTimer.GetTargetTime();
        double felapsed = ExplosiveProjectileBlockedTimer.ElapsedTime();
        apDebugText->PrintLine(
            aeLocation, aeJustify, "Explosive Projectile Blocked Timer: %.02f/%.02f", felapsed, ftarget
        );
    }
    TESObjectWEAP* pweapon = pCombatController->GetCurrentWeapon();
    if (TargetedPoint.operator!=(NiPoint3_ZERO)) {
        if (pweapon) {
            if (pweapon->GetAimArc() > 0.0) {
                CombatUtilities::GetAngleToProjectedPoint(&rot, pattacker, TargetedPoint);
                float fheading = pattacker->GetActualHeading();
                rot.z = ClampAngle(rot.z);
                float fdir = 0.0f;
                float a1_4 = GetAngleDelta(fheading, rot.z, fdir);
                float fdelta = std::fabs(a1_4);
                float ffiringArc = pweapon->GetAimArc();
                if (pattacker->IsPathing()) {
                    apDebugText->PrintLine(
                        aeLocation, aeJustify, "Weapon firing arc: ( %.2f/%.2f ) degrees (Moving)",
                        (fdelta + fdelta) * 57.2957763671875, ffiringArc
                    );
                }
                else {
                    float* v10 = &fCombatFiringArcStationaryTurnMult.Float();
                    ffiringArc = ffiringArc * *v10;
                    apDebugText->PrintLine(
                        aeLocation, aeJustify, "Weapon firing arc: ( %.2f/%.2f ) degrees (Stationary)",
                        (fdelta + fdelta) * 57.2957763671875, ffiringArc
                    );
                }
            }
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
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eState), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&TargetedPoint), 0xC, 0);
    ExplosiveProjectileBlockedTimer.SaveGame(apSaveGameBuffer);
    CooldownTimer.SaveGame(apSaveGameBuffer);
    SuppressiveFireTimer.SaveGame(apSaveGameBuffer);
    FiringTimer.SaveGame(apSaveGameBuffer);
    FireDelayTimer.SaveGame(apSaveGameBuffer);
}

// Save form version 10 added the fire delay timer.
void CombatProcedureAttackRanged::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&eState), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&TargetedPoint), 0xC);
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
    if (TargetedPoint == NiPoint3_ZERO) {
        return false;
    }
    Actor* pAttacker = GetAttacker();
    TESObjectWEAP* pWeapon = pCombatController->GetCurrentWeapon();
    float fArc = float(BSMAX(pWeapon->GetAimArc(), 15.0f) * 0.01745329238474369);
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
    NiPoint3 result;
    Actor* apTarget = GetTarget();
    if (apTarget) {
        TESObjectWEAP* ConsoleSelection = pCombatController->GetCurrentWeapon();
        Actor* Attacker = GetAttacker();
        BGSProjectile* Projectile_ = ConsoleSelection->GetProjectile(Attacker);
        Actor* v5 = GetAttacker();
        NiPoint3 arResult = CombatUtilities::GetProjectedPointForTarget(
            v5, apTarget, apReleasePoint, Projectile_, static_cast<float>(0.0), static_cast<bool>(0)
        );
        result = arResult;
    }
    else {
        result = NiPoint3_ZERO;
    }
    return result;
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
    CombatTimeStamp TimeStamp;
    TimeStamp.~CombatTimeStamp();
    ::new (&TimeStamp) CombatTimeStamp(-3.4028234663852886e+38f);
    fFiringStartedTimeStamp = TimeStamp;
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
