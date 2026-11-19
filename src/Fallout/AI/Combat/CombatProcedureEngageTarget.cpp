#include "Fallout/AI/Combat/CombatProcedureEngageTarget.h"

#include "BSCore/MemoryManager.h"
#include "BSMenu/IDebugText.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSWorldLocation.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Pathfinding/PathingRequestOptimalLocation.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatLocation.h"
#include "Fallout/AI/Combat/CombatProcedure.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

CombatProcedureEngageTarget::CombatProcedureEngageTarget()
  : eState(ENGAGE_STATE_INITIALIZING), fMinOptimalRange(0.0f), fMaxOptimalRange(0.0f), fAbsoluteMaxRange(0.0f),
    bLeftRightDodgeHadLOS(false), bLeftRightDodgeCheckLOS(0), bLeftRightDodgeLostLOS(0), cCheckedOptimalPath(1),
    LeftRightDodgeStartLocation(NiPoint3_ZERO), fOptimalLocationPathingFailedTimeStamp(-3.4028235e+38f) {}

CombatProcedureEngageTarget::~CombatProcedureEngageTarget() {}

void CombatProcedureEngageTarget::Update() {
    NiPoint3 optimalLocation;
    NiPoint3 newPoint;
    BGSWorldLocation attackerWorldLoc;
    BGSWorldLocation targetWorldLoc;
    Actor* pattacker = GetAttacker();
    Actor* ptarget = GetTarget();
    CombatState* pcombatState = pCombatController->pCombatState;
    if (ptarget) {
        NiPoint3 targetLoc;
        if (pcombatState->GetLastSeenTargetFiringLocation(targetLoc)) {
            SetLookAtTarget(targetLoc, 1);
        }
        if (!pCombatController->bAttackerCanMove) {
            this->eState = CombatProcedureEngageTarget::ENGAGE_STATE_HOLDING;
            return;
        }
        float ftempMinOptimalRange = fMinOptimalRange;
        float ftempMaxOptimalRange = fMaxOptimalRange;
        CombatUtilities::ComputeOptimalWeaponRanges(
            pCombatController, pCombatController->GetCurrentWeapon(), fMinOptimalRange, fMaxOptimalRange,
            fAbsoluteMaxRange
        );
        bool brangeChanged = false;
        if (fMinOptimalRange != ftempMinOptimalRange || fMaxOptimalRange != ftempMaxOptimalRange) {
            brangeChanged = true;
        }
        ptarget->GetWorldLocation(&targetWorldLoc);
        pattacker->GetWorldLocation(&attackerWorldLoc);
        float fsquaredDistance = attackerWorldLoc.GetDistanceSquared(targetWorldLoc);
        bool btooFar = fsquaredDistance > fMaxOptimalRange * fMaxOptimalRange;
        bool btooClose = fsquaredDistance < fMinOptimalRange * fMinOptimalRange;
        bool boutsideAbsoluteRange = fsquaredDistance > fAbsoluteMaxRange * fAbsoluteMaxRange;
        switch (this->eState) {
            case CombatProcedureEngageTarget::ENGAGE_STATE_INITIALIZING: {
                float a2 = RandomFloat(0.1f, 0.5f);
                HoldingTimer.Reset(a2);
                this->eState = CombatProcedureEngageTarget::ENGAGE_STATE_HOLDING;
                return;
            }
            case CombatProcedureEngageTarget::ENGAGE_STATE_HOLDING:
            case CombatProcedureEngageTarget::ENGAGE_STATE_CROUCHING: {
                NiPoint3* Point;
                TESObjectCELL* ParentCell;
                TESForm* Space;
                TESWorldSpace* WorldSpace;
                float fcutoff;
                // Compares ElapsedTime's result with 0.5 directly.
                if (!(pcombatState->GetLastTarget360LineOfSightTimeStamp().ElapsedTime() > 0.5)) {
                    goto LABEL_17;
                }
                if (!(fsquaredDistance < 262144.0) || !pcombatState->CheckMovement(targetWorldLoc, true, false)) {
                    goto LABEL_17;
                }
                {
                    PathingRequest request;
                    Space = targetWorldLoc.pSpace;
                    Point = &(&targetWorldLoc)->Point;
                    pattacker->BuildRequest_0(&request, *Point, Space, -1.0f, nullptr);
                    if (pattacker->SetPathfindingGoal(&request)) {
                        Destination = targetWorldLoc;
                        SetMoveMode(1);
                        this->eState = CombatProcedureEngageTarget::ENGAGE_STATE_APPROACHING_TARGET;
                        return;
                    }
                }
            LABEL_17:;
                if (bLeftRightDodgeCheckLOS) {
                    if (!(HoldingTimer.ElapsedTime() < 0.5)) {
                        bLeftRightDodgeCheckLOS = 0;
                    }
                    else {
                        if (!pcombatState->GetHasTarget360LineOfSight()) {
                            bLeftRightDodgeLostLOS = 1;
                            bLeftRightDodgeCheckLOS = 0;
                        }
                    }
                }
                if (!bLeftRightDodgeLostLOS) {
                    goto LABEL_29;
                }
                if ((!HoldingTimer.Check()) && (!(HoldingTimer.ElapsedTime() > 1.0))) {
                    goto LABEL_29;
                }

                bLeftRightDodgeCheckLOS = 0;
                bLeftRightDodgeLostLOS = 0;
                bLeftRightDodgeHadLOS = pcombatState->GetHasTarget360LineOfSight();
                LeftRightDodgeLOSTimer.Clear();
                {
                    PathingRequest request_2;
                    WorldSpace = pattacker->GetWorldSpace();
                    ParentCell = pattacker->GetParentCell();
                    pattacker->BuildRequest(
                        &request_2, LeftRightDodgeStartLocation, ParentCell, WorldSpace, 0.0f, nullptr
                    );
                    if (pattacker->SetPathfindingGoal(&request_2)) {
                        SetMoveMode(1);
                        this->eState = CombatProcedureEngageTarget::ENGAGE_STATE_DODGING_LEFT_RIGHT;
                        return;
                    }
                }
            LABEL_29:;
                if (!btooFar && !btooClose) {
                    goto LABEL_58;
                }
                fcutoff = 2.0f;
                if (btooFar && boutsideAbsoluteRange) {
                    fcutoff = 0.1f;
                }
                if ((!HoldingTimer.Check()) && (!(fcutoff < HoldingTimer.ElapsedTime()))) {
                    goto LABEL_58;
                }

                if (CreateOptimalLocationPath(boutsideAbsoluteRange)) {
                    if (btooFar) {
                        this->eState = CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_OPTIMAL_CLOSE;
                    }
                    else {
                        this->eState = CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_OPTIMAL_FAR;
                    }
                    return;
                }
                if (!boutsideAbsoluteRange) {
                    CombatLocation dodgeLocation;
                    float fdistance = RandomFloat(128.0f, 256.0f);
                    if (pcombatState->FindBestDodgeLocation(&dodgeLocation, btooFar ? 4 : 8, 0, fdistance, 24.0f)) {
                        PathingRequest request_3;
                        TESWorldSpace* v13 = pattacker->GetWorldSpace();
                        TESObjectCELL* v8 = pattacker->GetParentCell();
                        const NiPoint3* Pos = &dodgeLocation.GetPoint();
                        pattacker->BuildRequest(&request_3, *Pos, v8, v13, 16.0f, nullptr);
                        NiPoint3 center = (&request_3.Origin)->GetLocation();
                        float v28 = fdistance + 256.0;
                        request_3.SetCenterRadius(center, v28);
                        if (pattacker->SetPathfindingGoal(&request_3)) {
                            SetMoveMode(1);
                            this->eState = CombatProcedureEngageTarget::ENGAGE_STATE_DODGING_FORWARD_BACKWARD;
                        }
                        else {
                            SetFailed("Cannot setup path to dodge location");
                        }
                        return;
                    }
                    goto LABEL_58;
                }
                if (pcombatState->CheckMovement(targetWorldLoc, true, false)) {
                    PathingRequest request_4;
                    TESForm* v14 = targetWorldLoc.pSpace;
                    NiPoint3* v9 = &(&targetWorldLoc)->Point;
                    pattacker->BuildRequest_0(&request_4, *v9, v14, 128.0f, nullptr);
                    if (pattacker->SetPathfindingGoal(&request_4)) {
                        Destination = targetWorldLoc;
                        SetMoveMode(3);
                        this->eState = CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_TO_TARGET;
                    }
                    else {
                        SetFailed("Cannot setup path to target location");
                    }
                }
                else {
                    TESForm* v15;
                    NiPoint3* a1;
                    TESObjectCELL* ptargetCell;
                    TESWorldSpace* ptargetWorld;
                    ptargetCell = nullptr;
                    ptargetWorld = nullptr;
                    targetWorldLoc.GetCellOrWorld(ptargetCell, ptargetWorld);
                    a1 = &(&targetWorldLoc)->Point;
                    if (!Pathing::FindClosestPointOnNavmesh(ptargetWorld, ptargetCell, *a1, &newPoint) ||
                        (v15 = targetWorldLoc.pSpace, !pcombatState->CheckMovement(newPoint, v15, true, false))) {
                    LABEL_58:;
                        if (HoldingTimer.Check()) {
                            BaseProcess* v26;
                            float fdodgeDistance;
                            bool bcanDodge = true;
                            BaseProcess* CurrentAIProcess = pattacker->pCurrentProcess;
                            if (!CurrentAIProcess->HasMovementAnim(ANIM_GROUP_MOVE_LEFT) ||
                                (v26 = pattacker->pCurrentProcess, !v26->HasMovementAnim(ANIM_GROUP_MOVE_RIGHT))) {
                                bcanDodge = false;
                            }
                            CombatLocation dodgeLocation_2;
                            if (bcanDodge &&
                                (!RandomBool(0.25f) &&
                                 ((fdodgeDistance = 0.0f,
                                   !(bLeftRightDodgeHadLOS = pcombatState->GetHasTarget360LineOfSight()))
                                      ? fdodgeDistance = RandomFloat(64.0f, 192.0f)
                                      : fdodgeDistance = RandomFloat(128.0f, 384.0f),
                                  (LeftRightDodgeLOSTimer.Clear(),
                                   pcombatState
                                       ->FindBestDodgeLocation(&dodgeLocation_2, 3u, 0, fdodgeDistance, 24.0f))))) {
                                PathingRequest request_6;
                                TESWorldSpace* v16 = pattacker->GetWorldSpace();
                                TESObjectCELL* v10 = pattacker->GetParentCell();
                                const NiPoint3* v4 = &dodgeLocation_2.GetPoint();
                                pattacker->BuildRequest(&request_6, *v4, v10, v16, 16.0f, nullptr);
                                NiPoint3 center_2 = (&request_6.Origin)->GetLocation();
                                float v25 = fdodgeDistance + 256.0;
                                request_6.SetCenterRadius(center_2, v25);
                                if (pattacker->SetPathfindingGoal(&request_6)) {
                                    bLeftRightDodgeCheckLOS = 1;
                                    LeftRightDodgeStartLocation = pattacker->GetLocationOnReference();
                                    SetMoveMode(1);
                                    this->eState = CombatProcedureEngageTarget::ENGAGE_STATE_DODGING_LEFT_RIGHT;
                                }
                                else {
                                    SetFailed("Cannot setup path to dodge location");
                                }
                            }
                            else {
                                if (CanAttackerSneak()) {
                                    if (pattacker->IsSneaking()) {
                                        if (RandomBool(0.5f)) {
                                            this->eState = CombatProcedureEngageTarget::ENGAGE_STATE_CROUCHING;
                                        }
                                        else {
                                            ClearMoveMode();
                                            this->eState = CombatProcedureEngageTarget::ENGAGE_STATE_HOLDING;
                                        }
                                    }
                                    else {
                                        SetMoveMode(2);
                                        this->eState = CombatProcedureEngageTarget::ENGAGE_STATE_CROUCHING;
                                    }
                                }
                                else {
                                    this->eState = CombatProcedureEngageTarget::ENGAGE_STATE_HOLDING;
                                }
                                float a2a = RandomFloat(2.0f, 5.0f);
                                HoldingTimer.Reset(a2a);
                            }
                        }
                        return;
                    }
                    PathingRequest request_5;
                    pattacker->BuildRequest(&request_5, newPoint, ptargetCell, ptargetWorld, 128.0f, nullptr);
                    if (pattacker->SetPathfindingGoal(&request_5)) {
                        Destination.SetPoint(newPoint);
                        Destination.pSpace = targetWorldLoc.pSpace;
                        SetMoveMode(3);
                        this->eState = CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_TO_TARGET;
                    }
                    else {
                        SetFailed("Cannot setup path to target location");
                    }
                }
                break;
            }
            case CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_OPTIMAL_CLOSE:
            case CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_OPTIMAL_FAR:
            case CombatProcedureEngageTarget::ENGAGE_STATE_APPROACHING_TARGET:
            case CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_TO_TARGET: {
                bLeftRightDodgeLostLOS = 0;
                bool bforceComplete = false;
                bool bforceStopMoving = false;
                if (!pattacker->DidPathingFail()) {
                    goto LABEL_122;
                }
                if (!pattacker->IsPathValid()) {
                    if (this->eState == CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_OPTIMAL_FAR ||
                        this->eState == CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_OPTIMAL_CLOSE) {
                        float a2c = CombatTimer::GetTimer();
                        CombatTimeStamp failedTimeStamp(a2c);
                        fOptimalLocationPathingFailedTimeStamp = failedTimeStamp;
                        bforceComplete = true;
                    }
                    else {
                        pcombatState->AddUnreachableLocation(Destination);
                        bforceComplete = this->eState == CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_TO_TARGET;
                    }
                }
                if (bforceComplete) {
                    float ftargetMovedDistanceSquared;
                LABEL_122:;
                    if (!bforceComplete) {
                        CombatProcedureEngageTarget::ENGAGE_STATE eState = this->eState;
                        if (eState == CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_OPTIMAL_CLOSE) {
                            bforceComplete = fsquaredDistance < fMinOptimalRange * fMinOptimalRange;
                        }
                        else {
                            if (eState == CombatProcedureEngageTarget::ENGAGE_STATE_APPROACHING_TARGET) {
                                bforceComplete = pcombatState->GetHasTarget360LineOfSight();
                            }
                            else {
                                if (eState == CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_TO_TARGET &&
                                    pcombatState->GetHasTarget360LineOfSight() && !boutsideAbsoluteRange) {
                                    bforceComplete = true;
                                }
                            }
                        }
                    }
                    if (this->eState != CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_OPTIMAL_CLOSE &&
                        this->eState != CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_OPTIMAL_FAR) {
                        goto LABEL_148;
                    }
                    if (!bforceComplete && !cCheckedOptimalPath && !pattacker->IsWaitingOnPath()) {
                        cCheckedOptimalPath = 1;
                        if (pattacker->GetCurrentPathfindingGoal_008B3840(&optimalLocation)) {
                            TESForm* v17 = attackerWorldLoc.pSpace;
                            if (pcombatState->CheckMovement(optimalLocation, v17, false, false)) {
                                const NiPoint3* v5 = &ptarget->GetLocationOnReference();
                                if (!CombatUtilities::CalculateLOS(
                                        pattacker, ptarget, optimalLocation, *v5, false, -1
                                    )) {
                                    cCheckedOptimalPath = -2;
                                }
                            }
                            else {
                                cCheckedOptimalPath = -1;
                            }
                            if (cCheckedOptimalPath < 0) {
                                bforceComplete = true;
                                bforceStopMoving = true;
                                float a2d = CombatTimer::GetTimer();
                                CombatTimeStamp failedTimeStamp(a2d);
                                fOptimalLocationPathingFailedTimeStamp = failedTimeStamp;
                            }
                        }
                    }
                    if (bforceComplete ||
                        (((ftargetMovedDistanceSquared = targetWorldLoc.GetDistanceSquared(TargetOptimalWorldLoc),
                           !brangeChanged) &&
                          !(ftargetMovedDistanceSquared > 65536.0)) ||
                         !CreateOptimalLocationPath(boutsideAbsoluteRange))) {
                    LABEL_148:;
                        if (bforceComplete) {
                            if (bforceStopMoving) {
                                pattacker->ForceStopMoving();
                            }
                            else {
                                pattacker->StopMoving();
                            }
                        }
                        if (pattacker->IsPathingComplete() || bforceComplete) {
                            if (CanAttackerSneak() && RandomBool(0.5f)) {
                                SetMoveMode(2);
                                this->eState = CombatProcedureEngageTarget::ENGAGE_STATE_CROUCHING;
                            }
                            else {
                                this->eState = CombatProcedureEngageTarget::ENGAGE_STATE_HOLDING;
                            }
                            float fholdTime = RandomFloat(2.0f, 5.0f);
                            if (brangeChanged) {
                                fholdTime = 0.01f;
                            }
                            HoldingTimer.Reset(fholdTime);
                        }
                    }
                }
                else {
                    switch (this->eState) {
                        case CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_OPTIMAL_CLOSE: {
                            if (pattacker->IsPathValid()) {
                                SetFailed("Pathing failed while moving to optimal far location");
                            }
                            else {
                                SetFailed("Path Building failed to move to optimal far location");
                            }
                            break;
                        }
                        case CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_OPTIMAL_FAR: {
                            if (pattacker->IsPathValid()) {
                                SetFailed("Pathing failed while moving to optimal close location");
                            }
                            else {
                                SetFailed("Path Building failed to move to optimal close location");
                            }
                            break;
                        }
                        case CombatProcedureEngageTarget::ENGAGE_STATE_APPROACHING_TARGET: {
                            if (pattacker->IsPathValid()) {
                                SetFailed("Pathing failed while approaching target");
                            }
                            else {
                                SetFailed("Path Building failed to approach target");
                            }
                            break;
                        }
                        case CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_TO_TARGET: {
                            if (pattacker->IsPathValid()) {
                                SetFailed("Pathing failed while moving to target");
                            }
                            else {
                                SetFailed("Path Building failed to move to target");
                            }
                            break;
                        }
                        default: {
                            return;
                        }
                    }
                }
                return;
            }
            case CombatProcedureEngageTarget::ENGAGE_STATE_DODGING_FORWARD_BACKWARD:
            case CombatProcedureEngageTarget::ENGAGE_STATE_DODGING_LEFT_RIGHT: {
                bLeftRightDodgeLostLOS = 0;
                if (pattacker->DidPathingFail()) {
                    if (!pattacker->IsPathValid()) {
                        pcombatState->ClearLastDodgeDirection();
                    }
                    if (pattacker->IsPathValid()) {
                        SetFailed("Pathing failed while dodging");
                    }
                    else {
                        SetFailed("Path Building failed to dodge");
                    }
                }
                else {
                    if (this->eState == CombatProcedureEngageTarget::ENGAGE_STATE_DODGING_LEFT_RIGHT) {
                        if (LeftRightDodgeLOSTimer.IsSet()) {
                            if (LeftRightDodgeLOSTimer.Check()) {
                                pattacker->StopMoving();
                                bLeftRightDodgeHadLOS = false;
                                LeftRightDodgeLOSTimer.Clear();
                            }
                        }
                        else {
                            if (bLeftRightDodgeHadLOS && !pcombatState->GetHasTarget360LineOfSight()) {
                                LeftRightDodgeLOSTimer.Reset(0.5f);
                            }
                        }
                    }
                    if (pattacker->IsPathingComplete()) {
                        if (CanAttackerSneak() && RandomBool(0.5f)) {
                            SetMoveMode(2);
                            this->eState = CombatProcedureEngageTarget::ENGAGE_STATE_CROUCHING;
                        }
                        else {
                            this->eState = CombatProcedureEngageTarget::ENGAGE_STATE_HOLDING;
                        }
                        float a2b = RandomFloat(2.0f, 5.0f);
                        HoldingTimer.Reset(a2b);
                    }
                }
                return;
            }
            default: {
                return;
            }
        }
    }
}

void CombatProcedureEngageTarget::Shutdown() {
    CombatProcedure::ClearMoveMode();
    Actor* attacker = CombatProcedure::GetAttacker();
    attacker->StopMoving();
    CombatProcedure::ClearLookAtTarget();
}

void CombatProcedureEngageTarget::DisplayDebugText(
    IDebugText* apDebugText,
    DEBUGTEXT_LOCATION aeLocation,
    DEBUGTEXT_JUSTIFY aeJustify
) {
    BGSWorldLocation targetWorldLoc_2;
    BGSWorldLocation targetWorldLoc;
    Actor* ptarget = GetTarget();
    switch (eState) {
        case CombatProcedureEngageTarget::ENGAGE_STATE_INITIALIZING: {
            apDebugText->PrintLine(aeLocation, aeJustify, "INITIALIZING");
            break;
        }
        case CombatProcedureEngageTarget::ENGAGE_STATE_HOLDING: {
            apDebugText->PrintLine(aeLocation, aeJustify, "HOLDING");
            // The target time is read before ElapsedTime is called.
            float ftargetTime = HoldingTimer.fTargetTime;
            apDebugText->PrintLine(
                aeLocation, aeJustify, "Holding Timer: %.02f/%.02f", HoldingTimer.ElapsedTime(), ftargetTime
            );
            break;
        }
        case CombatProcedureEngageTarget::ENGAGE_STATE_CROUCHING: {
            apDebugText->PrintLine(aeLocation, aeJustify, "CROUCHING");
            float ftargetTime = HoldingTimer.fTargetTime;
            apDebugText->PrintLine(
                aeLocation, aeJustify, "Crouching Timer: %.02f/%.02f", HoldingTimer.ElapsedTime(), ftargetTime
            );
            break;
        }
        case CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_OPTIMAL_CLOSE: {
            apDebugText->PrintLine(aeLocation, aeJustify, "MOVING_OPTIMAL_CLOSE");
            apDebugText->PrintLine(aeLocation, aeJustify, "Range: %.2f - %.2f", fMinOptimalRange, fMaxOptimalRange);
            if (ptarget) {
                ptarget->GetWorldLocation(&targetWorldLoc);
                apDebugText->PrintLine(
                    aeLocation, aeJustify, "Target movement distance: %.2f/%.2f",
                    targetWorldLoc.GetDistance(TargetOptimalWorldLoc), 256.0
                );
            }
            break;
        }
        case CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_OPTIMAL_FAR: {
            apDebugText->PrintLine(aeLocation, aeJustify, "MOVING_OPTIMAL_FAR");
            apDebugText->PrintLine(aeLocation, aeJustify, "Range: %.2f - %.2f", fMinOptimalRange, fMaxOptimalRange);
            if (ptarget) {
                ptarget->GetWorldLocation(&targetWorldLoc_2);
                apDebugText->PrintLine(
                    aeLocation, aeJustify, "Target movement distance: %.2f/%.2f",
                    targetWorldLoc_2.GetDistance(TargetOptimalWorldLoc), 256.0
                );
            }
            break;
        }
        case CombatProcedureEngageTarget::ENGAGE_STATE_APPROACHING_TARGET: {
            apDebugText->PrintLine(aeLocation, aeJustify, "APPROACHING_TARGET");
            break;
        }
        case CombatProcedureEngageTarget::ENGAGE_STATE_DODGING_FORWARD_BACKWARD: {
            apDebugText->PrintLine(aeLocation, aeJustify, "DODGING FORWARD/BACKWARD");
            break;
        }
        case CombatProcedureEngageTarget::ENGAGE_STATE_DODGING_LEFT_RIGHT: {
            apDebugText->PrintLine(aeLocation, aeJustify, "DODGING LEFT/RIGHT");
            if (bLeftRightDodgeHadLOS && LeftRightDodgeLOSTimer.IsSet()) {
                float ftargetTime = LeftRightDodgeLOSTimer.fTargetTime;
                apDebugText->PrintLine(
                    aeLocation, aeJustify, "Lost LOS.  Stop Moving Timer: %.02f/%.02f",
                    LeftRightDodgeLOSTimer.ElapsedTime(), ftargetTime
                );
            }
            break;
        }
        case CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_TO_TARGET: {
            apDebugText->PrintLine(aeLocation, aeJustify, "MOVING_TO_TARGET");
            break;
        }
        default: {
            break;
        }
    }
    if (fOptimalLocationPathingFailedTimeStamp.ElapsedTime() < 5.0) {
        if (cCheckedOptimalPath == -1) {
            double v5 = fOptimalLocationPathingFailedTimeStamp.DisplayElapsedTime();
            apDebugText->PrintLine(
                aeLocation, aeJustify, "Optimal location pathing CheckMovement failed: %.02f/%.02f", v5, 5.0
            );
        }
        else {
            if (cCheckedOptimalPath == -2) {
                double v6 = fOptimalLocationPathingFailedTimeStamp.DisplayElapsedTime();
                apDebugText->PrintLine(
                    aeLocation, aeJustify, "Optimal location pathing LOS check failed: %.02f/%.02f", v6, 5.0
                );
            }
            else {
                double v7 = fOptimalLocationPathingFailedTimeStamp.DisplayElapsedTime();
                apDebugText->PrintLine(
                    aeLocation, aeJustify, "Optimal location path building failed: %.02f/%.02f", v7, 5.0
                );
            }
        }
    }
}

CombatProcedure::COMBAT_PROCEDURES CombatProcedureEngageTarget::GetType() {
    return ENGAGE_TARGET;
}

void CombatProcedureEngageTarget::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    CombatProcedure::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&eState, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fMinOptimalRange, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fMaxOptimalRange, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fAbsoluteMaxRange, 4u, 0);
    HoldingTimer.SaveGame(apSaveGameBuffer);
    LeftRightDodgeLOSTimer.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&bLeftRightDodgeHadLOS, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bLeftRightDodgeCheckLOS, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bLeftRightDodgeLostLOS, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&LeftRightDodgeStartLocation, 12u, 0);
    Destination.SaveGame(apSaveGameBuffer);
    TargetOptimalWorldLoc.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&cCheckedOptimalPath, 1u, 0);
    fOptimalLocationPathingFailedTimeStamp.SaveGame(apSaveGameBuffer);
}

void CombatProcedureEngageTarget::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&eState, 4u);
    apLoadGameBuffer->LoadData(&fMinOptimalRange, 4u);
    apLoadGameBuffer->LoadData(&fMaxOptimalRange, 4u);
    if (apLoadGameBuffer->GetVersion() < 14u) {
        fAbsoluteMaxRange = 10000.0f;
    }
    else {
        apLoadGameBuffer->LoadData(&fAbsoluteMaxRange, 4u);
    }
    HoldingTimer.LoadGame(apLoadGameBuffer);
    LeftRightDodgeLOSTimer.LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&bLeftRightDodgeHadLOS, 1u);
    apLoadGameBuffer->LoadData(&bLeftRightDodgeCheckLOS, 1u);
    apLoadGameBuffer->LoadData(&bLeftRightDodgeLostLOS, 1u);
    apLoadGameBuffer->LoadData(&LeftRightDodgeStartLocation, 12u);
    Destination.LoadGame(apLoadGameBuffer);
    TargetOptimalWorldLoc.LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetVersion() >= 10u) {
        apLoadGameBuffer->LoadData(&cCheckedOptimalPath, 1u);
        fOptimalLocationPathingFailedTimeStamp.LoadGame(apLoadGameBuffer);
    }
}

void CombatProcedureEngageTarget::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::InitLoadGame(apLoadGameBuffer);
}

bool CombatProcedureEngageTarget::CanAttackerSneak() {
    NiPoint3 v5;
    if (!pCombatController->GetAttackerCanSneak()) {
        return false;
    }
    Actor* shader = pCombatController->GetTarget();
    NiPoint3 v6 = shader->GetLocationOnReference();
    Actor* packageOwner = pCombatController->GetAttacker();
    NiPoint3 v7 = packageOwner->GetLocationOnReference();
    (v5 = v6 - v7);
    return (bool)((uint8_t)(!((&v5)->SqrLength() < 65536.0)));
}

bool CombatProcedureEngageTarget::CreateOptimalLocationPath(bool abOutsideAbsoluteRange) {
    BGSWorldLocation v15;
    BGSWorldLocation apLocation;
    if (fOptimalLocationPathingFailedTimeStamp.ElapsedTime() < 5.0) {
        return false;
    }
    cCheckedOptimalPath = 1;
    if (bUseBackgroundOptimalLocationPathing) {
        Actor* pattacker = GetAttacker();
        Actor* ptarget = GetTarget();
        PathingRequestOptimalLocation request;
        TESWorldSpace* a5 = pattacker->GetWorldSpace();
        TESObjectCELL* a4 = pattacker->GetParentCell();
        const NiPoint3* v4 = &ptarget->GetLocationOnReference();
        pattacker->BuildRequest(&request, *v4, a4, a5, 16.0f, nullptr);
        request.SetMinDistance(fMinOptimalRange);
        request.SetMaxDistance(fMaxOptimalRange);
        float v13 = fMinOptimalRange * 0.5;
        request.SetActorHeight(v13);
        request.SetAbsoluteMaxDistance(fAbsoluteMaxRange);
        if (pattacker->SetPathfindingGoal(&request)) {
            TargetOptimalWorldLoc = *ptarget->GetWorldLocation(&apLocation);
            SetMoveMode(abOutsideAbsoluteRange ? 3 : 1);
            cCheckedOptimalPath = 0;
            return true;
        }

        return false;
    }

    Actor* pattacker_2 = GetAttacker();
    Actor* ptarget_2 = GetTarget();
    CombatLocation optimalLocation;
    CombatState* pcombatState = pCombatController->pCombatState;
    float v12 = fMinOptimalRange * 0.5;
    if (pcombatState->FindOptimalLocation(
            fMinOptimalRange, fMaxOptimalRange, v12, fAbsoluteMaxRange, &optimalLocation, true
        )) {
        if (pcombatState->CheckMovement(optimalLocation.WorldLoc, true, false)) {
            PathingRequest request_2;
            TESWorldSpace* a5b = pattacker_2->GetWorldSpace();
            TESObjectCELL* a4a = pattacker_2->GetParentCell();
            const NiPoint3* Pos = &optimalLocation.GetPoint();
            pattacker_2->BuildRequest(&request_2, *Pos, a4a, a5b, 16.0f, nullptr);
            if (pattacker_2->SetPathfindingGoal(&request_2)) {
                Destination = optimalLocation.WorldLoc;
                TargetOptimalWorldLoc = *ptarget_2->GetWorldLocation(&v15);
                SetMoveMode(abOutsideAbsoluteRange ? 3 : 1);
                return true;
            }

            return false;
        }

        return false;
    }

    return false;
}

void CombatState::ClearLastDodgeDirection() {
    cLastDodgeDirection = -1;
}
