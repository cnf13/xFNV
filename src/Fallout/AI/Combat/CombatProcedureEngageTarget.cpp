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

#include <limits>

CombatProcedureEngageTarget::CombatProcedureEngageTarget()
  : eState(ENGAGE_STATE_INITIALIZING), fMinOptimalRange(0.0f), fMaxOptimalRange(0.0f), fAbsoluteMaxRange(0.0f),
    bLeftRightDodgeHadLOS(false), bLeftRightDodgeCheckLOS(0), bLeftRightDodgeLostLOS(0), cCheckedOptimalPath(1),
    LeftRightDodgeStartLocation(NiPoint3::ZERO),
    fOptimalLocationPathingFailedTimeStamp(-std::numeric_limits<float>::max()) {}

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
            pCombatController,
            pCombatController->GetCurrentWeapon(),
            fMinOptimalRange,
            fMaxOptimalRange,
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
                float fHoldTime = RandomFloat(0.1f, 0.5f);
                HoldingTimer.Reset(fHoldTime);
                this->eState = CombatProcedureEngageTarget::ENGAGE_STATE_HOLDING;
                return;
            }
            case CombatProcedureEngageTarget::ENGAGE_STATE_HOLDING:
            case CombatProcedureEngageTarget::ENGAGE_STATE_CROUCHING: {
                float fcutoff;
                // Compares ElapsedTime's result with 0.5 directly.
                if (pcombatState->GetLastTarget360LineOfSightTimeStamp().ElapsedTime() > 0.5 &&
                    fsquaredDistance < 262144.0 &&
                    pcombatState->CheckMovement(targetWorldLoc, true, false)) {
                    PathingRequest request;
                    pattacker->BuildRequest_0(&request, targetWorldLoc.Point, targetWorldLoc.pSpace, -1.0f, nullptr);
                    if (pattacker->SetPathfindingGoal(&request)) {
                        Destination = targetWorldLoc;
                        SetMoveMode(1);
                        this->eState = CombatProcedureEngageTarget::ENGAGE_STATE_APPROACHING_TARGET;
                        return;
                    }
                }
                if (bLeftRightDodgeCheckLOS) {
                    if (!(HoldingTimer.ElapsedTime() < 0.5)) {
                        bLeftRightDodgeCheckLOS = 0;
                    }
                    else if (!pcombatState->GetHasTarget360LineOfSight()) {
                        bLeftRightDodgeLostLOS = 1;
                        bLeftRightDodgeCheckLOS = 0;
                    }
                }
                if (bLeftRightDodgeLostLOS && (HoldingTimer.Check() || HoldingTimer.ElapsedTime() > 1.0)) {
                    bLeftRightDodgeCheckLOS = 0;
                    bLeftRightDodgeLostLOS = 0;
                    bLeftRightDodgeHadLOS = pcombatState->GetHasTarget360LineOfSight();
                    LeftRightDodgeLOSTimer.Clear();
                    PathingRequest request;
                    TESWorldSpace* pWorldSpace = pattacker->GetWorldSpace();
                    TESObjectCELL* pParentCell = pattacker->GetParentCell();
                    pattacker
                        ->BuildRequest(&request, LeftRightDodgeStartLocation, pParentCell, pWorldSpace, 0.0f, nullptr);
                    if (pattacker->SetPathfindingGoal(&request)) {
                        SetMoveMode(1);
                        this->eState = CombatProcedureEngageTarget::ENGAGE_STATE_DODGING_LEFT_RIGHT;
                        return;
                    }
                }
                if (btooFar || btooClose) {
                    fcutoff = 2.0f;
                    if (btooFar && boutsideAbsoluteRange) {
                        fcutoff = 0.1f;
                    }
                    if (HoldingTimer.Check() || fcutoff < HoldingTimer.ElapsedTime()) {
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
                            if (pcombatState
                                    ->FindBestDodgeLocation(&dodgeLocation, btooFar ? 4 : 8, 0, fdistance, 24.0f)) {
                                PathingRequest request;
                                TESWorldSpace* pWorldSpace = pattacker->GetWorldSpace();
                                TESObjectCELL* pParentCell = pattacker->GetParentCell();
                                pattacker->BuildRequest(
                                    &request,
                                    dodgeLocation.GetPoint(),
                                    pParentCell,
                                    pWorldSpace,
                                    16.0f,
                                    nullptr
                                );
                                NiPoint3 kCenter = request.Origin.GetLocation();
                                float fRadius = fdistance + 256.0;
                                request.SetCenterRadius(kCenter, fRadius);
                                if (pattacker->SetPathfindingGoal(&request)) {
                                    SetMoveMode(1);
                                    this->eState = CombatProcedureEngageTarget::ENGAGE_STATE_DODGING_FORWARD_BACKWARD;
                                }
                                else {
                                    SetFailed("Cannot setup path to dodge location");
                                }
                                return;
                            }
                        }
                        else if (pcombatState->CheckMovement(targetWorldLoc, true, false)) {
                            PathingRequest request;
                            pattacker->BuildRequest_0(
                                &request,
                                targetWorldLoc.Point,
                                targetWorldLoc.pSpace,
                                128.0f,
                                nullptr
                            );
                            if (pattacker->SetPathfindingGoal(&request)) {
                                Destination = targetWorldLoc;
                                SetMoveMode(3);
                                this->eState = CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_TO_TARGET;
                            }
                            else {
                                SetFailed("Cannot setup path to target location");
                            }
                            break;
                        }
                        else {
                            TESObjectCELL* ptargetCell;
                            TESWorldSpace* ptargetWorld;
                            ptargetCell = nullptr;
                            ptargetWorld = nullptr;
                            targetWorldLoc.GetCellOrWorld(ptargetCell, ptargetWorld);
                            if (Pathing::FindClosestPointOnNavmesh(
                                    ptargetWorld,
                                    ptargetCell,
                                    targetWorldLoc.Point,
                                    &newPoint
                                ) &&
                                pcombatState->CheckMovement(newPoint, targetWorldLoc.pSpace, true, false)) {
                                PathingRequest request;
                                pattacker->BuildRequest(&request, newPoint, ptargetCell, ptargetWorld, 128.0f, nullptr);
                                if (pattacker->SetPathfindingGoal(&request)) {
                                    Destination.SetPoint(newPoint);
                                    Destination.pSpace = targetWorldLoc.pSpace;
                                    SetMoveMode(3);
                                    this->eState = CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_TO_TARGET;
                                }
                                else {
                                    SetFailed("Cannot setup path to target location");
                                }
                                break;
                            }
                        }
                    }
                }
                if (HoldingTimer.Check()) {
                    float fdodgeDistance;
                    bool bcanDodge = true;
                    if (!pattacker->pCurrentProcess->HasMovementAnim(ANIM_GROUP_MOVE_LEFT) ||
                        !pattacker->pCurrentProcess->HasMovementAnim(ANIM_GROUP_MOVE_RIGHT)) {
                        bcanDodge = false;
                    }
                    CombatLocation dodgeLocation;
                    if (bcanDodge && !RandomBool(0.25f)) {
                        bLeftRightDodgeHadLOS = pcombatState->GetHasTarget360LineOfSight();
                        if (!bLeftRightDodgeHadLOS) {
                            fdodgeDistance = RandomFloat(64.0f, 192.0f);
                        }
                        else {
                            fdodgeDistance = RandomFloat(128.0f, 384.0f);
                        }
                        LeftRightDodgeLOSTimer.Clear();
                        if (pcombatState->FindBestDodgeLocation(&dodgeLocation, 3u, 0, fdodgeDistance, 24.0f)) {
                            PathingRequest request;
                            TESWorldSpace* pWorldSpace = pattacker->GetWorldSpace();
                            TESObjectCELL* pParentCell = pattacker->GetParentCell();
                            pattacker->BuildRequest(
                                &request,
                                dodgeLocation.GetPoint(),
                                pParentCell,
                                pWorldSpace,
                                16.0f,
                                nullptr
                            );
                            NiPoint3 kCenter = request.Origin.GetLocation();
                            float fRadius = fdodgeDistance + 256.0;
                            request.SetCenterRadius(kCenter, fRadius);
                            if (pattacker->SetPathfindingGoal(&request)) {
                                bLeftRightDodgeCheckLOS = 1;
                                LeftRightDodgeStartLocation = pattacker->GetLocationOnReference();
                                SetMoveMode(1);
                                this->eState = CombatProcedureEngageTarget::ENGAGE_STATE_DODGING_LEFT_RIGHT;
                            }
                            else {
                                SetFailed("Cannot setup path to dodge location");
                            }
                            return;
                        }
                    }
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
                    float fHoldTime = RandomFloat(2.0f, 5.0f);
                    HoldingTimer.Reset(fHoldTime);
                }
                return;
            }
            case CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_OPTIMAL_CLOSE:
            case CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_OPTIMAL_FAR:
            case CombatProcedureEngageTarget::ENGAGE_STATE_APPROACHING_TARGET:
            case CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_TO_TARGET: {
                bLeftRightDodgeLostLOS = 0;
                bool bforceComplete = false;
                bool bforceStopMoving = false;
                if (pattacker->DidPathingFail()) {
                    if (!pattacker->IsPathValid()) {
                        if (this->eState == CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_OPTIMAL_FAR ||
                            this->eState == CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_OPTIMAL_CLOSE) {
                            float fCurrentTime = CombatTimer::GetTimer();
                            CombatTimeStamp failedTimeStamp(fCurrentTime);
                            fOptimalLocationPathingFailedTimeStamp = failedTimeStamp;
                            bforceComplete = true;
                        }
                        else {
                            pcombatState->AddUnreachableLocation(Destination);
                            bforceComplete = this->eState == CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_TO_TARGET;
                        }
                    }
                    if (!bforceComplete) {
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
                        return;
                    }
                }
                if (!bforceComplete) {
                    CombatProcedureEngageTarget::ENGAGE_STATE eState = this->eState;
                    if (eState == CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_OPTIMAL_CLOSE) {
                        bforceComplete = fsquaredDistance < fMinOptimalRange * fMinOptimalRange;
                    }
                    else if (eState == CombatProcedureEngageTarget::ENGAGE_STATE_APPROACHING_TARGET) {
                        bforceComplete = pcombatState->GetHasTarget360LineOfSight();
                    }
                    else if (
                        eState == CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_TO_TARGET &&
                        pcombatState->GetHasTarget360LineOfSight() &&
                        !boutsideAbsoluteRange
                    ) {
                        bforceComplete = true;
                    }
                }
                if (this->eState == CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_OPTIMAL_CLOSE ||
                    this->eState == CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_OPTIMAL_FAR) {
                    if (!bforceComplete && !cCheckedOptimalPath && !pattacker->IsWaitingOnPath()) {
                        cCheckedOptimalPath = 1;
                        if (pattacker->GetCurrentPathfindingGoal(&optimalLocation)) {
                            if (pcombatState->CheckMovement(optimalLocation, attackerWorldLoc.pSpace, false, false)) {
                                if (!CombatUtilities::CalculateLOS(
                                        pattacker,
                                        ptarget,
                                        optimalLocation,
                                        ptarget->GetLocationOnReference(),
                                        false,
                                        -1
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
                                float fCurrentTime = CombatTimer::GetTimer();
                                CombatTimeStamp failedTimeStamp(fCurrentTime);
                                fOptimalLocationPathingFailedTimeStamp = failedTimeStamp;
                            }
                        }
                    }
                    if (!bforceComplete) {
                        float ftargetMovedDistanceSquared = targetWorldLoc.GetDistanceSquared(TargetOptimalWorldLoc);
                        if ((brangeChanged || ftargetMovedDistanceSquared > 65536.0) &&
                            CreateOptimalLocationPath(boutsideAbsoluteRange)) {
                            return;
                        }
                    }
                }
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
                        else if (bLeftRightDodgeHadLOS && !pcombatState->GetHasTarget360LineOfSight()) {
                            LeftRightDodgeLOSTimer.Reset(0.5f);
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
                        float fHoldTime = RandomFloat(2.0f, 5.0f);
                        HoldingTimer.Reset(fHoldTime);
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
    Actor* pAttacker = CombatProcedure::GetAttacker();
    pAttacker->StopMoving();
    CombatProcedure::ClearLookAtTarget();
}

void CombatProcedureEngageTarget::DisplayDebugText(
    IDebugText* apDebugText,
    DEBUGTEXT_LOCATION aeLocation,
    DEBUGTEXT_JUSTIFY aeJustify
) {
    BGSWorldLocation targetWorldLocFar;
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
                aeLocation,
                aeJustify,
                "Holding Timer: %.02f/%.02f",
                HoldingTimer.ElapsedTime(),
                ftargetTime
            );
            break;
        }
        case CombatProcedureEngageTarget::ENGAGE_STATE_CROUCHING: {
            apDebugText->PrintLine(aeLocation, aeJustify, "CROUCHING");
            float ftargetTime = HoldingTimer.fTargetTime;
            apDebugText->PrintLine(
                aeLocation,
                aeJustify,
                "Crouching Timer: %.02f/%.02f",
                HoldingTimer.ElapsedTime(),
                ftargetTime
            );
            break;
        }
        case CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_OPTIMAL_CLOSE: {
            apDebugText->PrintLine(aeLocation, aeJustify, "MOVING_OPTIMAL_CLOSE");
            apDebugText->PrintLine(aeLocation, aeJustify, "Range: %.2f - %.2f", fMinOptimalRange, fMaxOptimalRange);
            if (ptarget) {
                ptarget->GetWorldLocation(&targetWorldLoc);
                apDebugText->PrintLine(
                    aeLocation,
                    aeJustify,
                    "Target movement distance: %.2f/%.2f",
                    targetWorldLoc.GetDistance(TargetOptimalWorldLoc),
                    256.0
                );
            }
            break;
        }
        case CombatProcedureEngageTarget::ENGAGE_STATE_MOVING_OPTIMAL_FAR: {
            apDebugText->PrintLine(aeLocation, aeJustify, "MOVING_OPTIMAL_FAR");
            apDebugText->PrintLine(aeLocation, aeJustify, "Range: %.2f - %.2f", fMinOptimalRange, fMaxOptimalRange);
            if (ptarget) {
                ptarget->GetWorldLocation(&targetWorldLocFar);
                apDebugText->PrintLine(
                    aeLocation,
                    aeJustify,
                    "Target movement distance: %.2f/%.2f",
                    targetWorldLocFar.GetDistance(TargetOptimalWorldLoc),
                    256.0
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
                    aeLocation,
                    aeJustify,
                    "Lost LOS.  Stop Moving Timer: %.02f/%.02f",
                    LeftRightDodgeLOSTimer.ElapsedTime(),
                    ftargetTime
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
            double dElapsed = fOptimalLocationPathingFailedTimeStamp.DisplayElapsedTime();
            apDebugText->PrintLine(
                aeLocation,
                aeJustify,
                "Optimal location pathing CheckMovement failed: %.02f/%.02f",
                dElapsed,
                5.0
            );
        }
        else if (cCheckedOptimalPath == -2) {
            double dElapsed = fOptimalLocationPathingFailedTimeStamp.DisplayElapsedTime();
            apDebugText->PrintLine(
                aeLocation,
                aeJustify,
                "Optimal location pathing LOS check failed: %.02f/%.02f",
                dElapsed,
                5.0
            );
        }
        else {
            double dElapsed = fOptimalLocationPathingFailedTimeStamp.DisplayElapsedTime();
            apDebugText
                ->PrintLine(aeLocation, aeJustify, "Optimal location path building failed: %.02f/%.02f", dElapsed, 5.0);
        }
    }
}

CombatProcedure::COMBAT_PROCEDURES CombatProcedureEngageTarget::GetType() {
    return ENGAGE_TARGET;
}

void CombatProcedureEngageTarget::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    CombatProcedure::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&eState, sizeof(eState), 0);
    apSaveGameBuffer->SaveDataEndian(&fMinOptimalRange, sizeof(fMinOptimalRange), 0);
    apSaveGameBuffer->SaveDataEndian(&fMaxOptimalRange, sizeof(fMaxOptimalRange), 0);
    apSaveGameBuffer->SaveDataEndian(&fAbsoluteMaxRange, sizeof(fAbsoluteMaxRange), 0);
    HoldingTimer.SaveGame(apSaveGameBuffer);
    LeftRightDodgeLOSTimer.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&bLeftRightDodgeHadLOS, sizeof(bLeftRightDodgeHadLOS), 0);
    apSaveGameBuffer->SaveDataEndian(&bLeftRightDodgeCheckLOS, sizeof(bLeftRightDodgeCheckLOS), 0);
    apSaveGameBuffer->SaveDataEndian(&bLeftRightDodgeLostLOS, sizeof(bLeftRightDodgeLostLOS), 0);
    apSaveGameBuffer->SaveDataEndian(&LeftRightDodgeStartLocation, sizeof(LeftRightDodgeStartLocation), 0);
    Destination.SaveGame(apSaveGameBuffer);
    TargetOptimalWorldLoc.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&cCheckedOptimalPath, sizeof(cCheckedOptimalPath), 0);
    fOptimalLocationPathingFailedTimeStamp.SaveGame(apSaveGameBuffer);
}

void CombatProcedureEngageTarget::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&eState, sizeof(eState));
    apLoadGameBuffer->LoadData(&fMinOptimalRange, sizeof(fMinOptimalRange));
    apLoadGameBuffer->LoadData(&fMaxOptimalRange, sizeof(fMaxOptimalRange));
    if (apLoadGameBuffer->GetVersion() < 14u) {
        fAbsoluteMaxRange = 10000.0f;
    }
    else {
        apLoadGameBuffer->LoadData(&fAbsoluteMaxRange, sizeof(fAbsoluteMaxRange));
    }
    HoldingTimer.LoadGame(apLoadGameBuffer);
    LeftRightDodgeLOSTimer.LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&bLeftRightDodgeHadLOS, sizeof(bLeftRightDodgeHadLOS));
    apLoadGameBuffer->LoadData(&bLeftRightDodgeCheckLOS, sizeof(bLeftRightDodgeCheckLOS));
    apLoadGameBuffer->LoadData(&bLeftRightDodgeLostLOS, sizeof(bLeftRightDodgeLostLOS));
    apLoadGameBuffer->LoadData(&LeftRightDodgeStartLocation, sizeof(LeftRightDodgeStartLocation));
    Destination.LoadGame(apLoadGameBuffer);
    TargetOptimalWorldLoc.LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetVersion() >= 10u) {
        apLoadGameBuffer->LoadData(&cCheckedOptimalPath, sizeof(cCheckedOptimalPath));
        fOptimalLocationPathingFailedTimeStamp.LoadGame(apLoadGameBuffer);
    }
}

void CombatProcedureEngageTarget::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::InitLoadGame(apLoadGameBuffer);
}

bool CombatProcedureEngageTarget::CanAttackerSneak() {
    NiPoint3 kDelta;
    if (!pCombatController->GetAttackerCanSneak()) {
        return false;
    }
    Actor* pTarget = pCombatController->GetTarget();
    NiPoint3 kTargetPos = pTarget->GetLocationOnReference();
    Actor* pAttacker = pCombatController->GetAttacker();
    NiPoint3 kAttackerPos = pAttacker->GetLocationOnReference();
    kDelta = kTargetPos - kAttackerPos;
    return !(kDelta.SqrLength() < 65536.0);
}

bool CombatProcedureEngageTarget::CreateOptimalLocationPath(bool abOutsideAbsoluteRange) {
    BGSWorldLocation kOptimalTargetLoc;
    BGSWorldLocation kTargetLoc;
    if (fOptimalLocationPathingFailedTimeStamp.ElapsedTime() < 5.0) {
        return false;
    }
    cCheckedOptimalPath = 1;
    if (bUseBackgroundOptimalLocationPathing) {
        Actor* pattacker = GetAttacker();
        Actor* ptarget = GetTarget();
        PathingRequestOptimalLocation request;
        TESWorldSpace* pWorldSpace = pattacker->GetWorldSpace();
        TESObjectCELL* pParentCell = pattacker->GetParentCell();
        pattacker->BuildRequest(&request, ptarget->GetLocationOnReference(), pParentCell, pWorldSpace, 16.0f, nullptr);
        request.SetMinDistance(fMinOptimalRange);
        request.SetMaxDistance(fMaxOptimalRange);
        float fActorHeight = fMinOptimalRange * 0.5;
        request.SetActorHeight(fActorHeight);
        request.SetAbsoluteMaxDistance(fAbsoluteMaxRange);
        if (pattacker->SetPathfindingGoal(&request)) {
            TargetOptimalWorldLoc = *ptarget->GetWorldLocation(&kTargetLoc);
            SetMoveMode(abOutsideAbsoluteRange ? 3 : 1);
            cCheckedOptimalPath = 0;
            return true;
        }

        return false;
    }

    Actor* pattacker = GetAttacker();
    Actor* ptarget = GetTarget();
    CombatLocation optimalLocation;
    CombatState* pcombatState = pCombatController->pCombatState;
    float fActorHeight = fMinOptimalRange * 0.5;
    if (pcombatState->FindOptimalLocation(
            fMinOptimalRange,
            fMaxOptimalRange,
            fActorHeight,
            fAbsoluteMaxRange,
            &optimalLocation,
            true
        )) {
        if (pcombatState->CheckMovement(optimalLocation.WorldLoc, true, false)) {
            PathingRequest request;
            TESWorldSpace* pWorldSpace = pattacker->GetWorldSpace();
            TESObjectCELL* pParentCell = pattacker->GetParentCell();
            pattacker->BuildRequest(&request, optimalLocation.GetPoint(), pParentCell, pWorldSpace, 16.0f, nullptr);
            if (pattacker->SetPathfindingGoal(&request)) {
                Destination = optimalLocation.WorldLoc;
                TargetOptimalWorldLoc = *ptarget->GetWorldLocation(&kOptimalTargetLoc);
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
