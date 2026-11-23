#include "Fallout/AI/Combat/CombatProcedureMove.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMenu/IDebugText.h"
#include "Fallout Shared/BGSWorldLocation.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/Pathfinding/NavMeshMath.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Pathfinding/PathingRequestCoveredMove.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatLocation.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatProcedure.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "SDK/MSVC/rtti.h"

// Takes the destination, the target reference and the radius's sign from apCombatLocation when there is one;
// the look-at position is deliberately left unset.
CombatProcedureMove::CombatProcedureMove(CombatLocation* apCombatLocation, float afTargetRadius, int32_t aiLocation)
  : eState(CombatProcedureMove::MOVE_STATE_INITIALIZING), fTargetRadius(afTargetRadius), pTargetRef(nullptr),
    iLocation(aiLocation), bReservedSecondaryLocation(false), spQueuedPathingRequest(nullptr),
    bHasQueuedPathingRequest(false), iCurrentNode(static_cast<uint32_t>(-1)) {
    if (apCombatLocation) {
        Destination = apCombatLocation->WorldLoc;
        CurrentDestination = Destination;
        pTargetRef = apCombatLocation->pRef;
        if (apCombatLocation->GetFlag(CombatLocation::FLAG_NEGATIVE_RADIUS)) {
            fTargetRadius = -fTargetRadius;
        }
    }
}

CombatProcedureMove::~CombatProcedureMove() {}

void CombatProcedureMove::Update() {
    BGSWorldLocation attackerInvestigateLoc;
    NiPoint3 coverPosition;
    NiPoint3 nextNodeLocation;
    BGSWorldLocation attackerWorldLoc;
    BGSWorldLocation targetWorldLoc;
    BGSWorldLocation targetWorldLocation;
    NiPoint3 coverLocation;
    Actor* pattacker = GetAttacker();
    PROCESS_TYPE eattackerProcessLevel = pattacker->GetCurrentProcessType();
    Actor* ptarget = GetTarget();
    CombatState* pcombatState = pCombatController->pCombatState;
    switch (eState) {
        case CombatProcedureMove::MOVE_STATE_INITIALIZING: {
            CurrentDestination.Point = Destination.Point;
            CurrentDestination.pSpace = Destination.pSpace;
            eState = CombatProcedureMove::MOVE_STATE_MOVING;
            if (CreatePath() && iLocation == 15) {
                CheckDestinationTimer.Reset(fCombatSearchCheckInvestigateInitialTime.Float());
            }
            break;
        }
        case CombatProcedureMove::MOVE_STATE_MOVING:
        case CombatProcedureMove::MOVE_STATE_MOVING_USING_COVER: {
            if (spQueuedPathingRequest.m_pObject && !pattacker->IsWaitingOnPath()) {
                PathingRequestCoveredMove* prequest = static_cast<PathingRequestCoveredMove*>(
                    spQueuedPathingRequest.m_pObject
                );
                if (prequest->GetType() == 8 && prequest->bFinished) {
                    pcombatState->SetSecondaryCoverSearchInfo(prequest->pCombatCoverSearchInfo);
                    CombatCoverLocation* pcombatCoverLocation = prequest->pCombatCoverLocation;
                    pcombatState->SetSecondaryCoverLocation(pcombatCoverLocation);
                    prequest->pCombatCoverLocation = nullptr;
                    if (pcombatCoverLocation) {
                        CurrentDestination.SetLocation(prequest->CoverLocation, pattacker->GetSpace());
                        if (!pcombatState->TryReserveSecondaryCoverLocation()) {
                            spQueuedPathingRequest = static_cast<PathingRequest*>(nullptr);
                            CurrentDestination.Point = Destination.Point;
                            CurrentDestination.pSpace = Destination.pSpace;
                            eState = CombatProcedureMove::MOVE_STATE_MOVING;
                            CreatePath();
                            return;
                        }
                        eState = CombatProcedureMove::MOVE_STATE_MOVING_USING_COVER;
                        bReservedSecondaryLocation = true;
                        CombatCoverLocation* psecondaryCoverLocation = pcombatState->GetSecondaryCoverLocation();
                        CombatUtilities::CalculateCoverPositions(
                            pCombatController,
                            psecondaryCoverLocation,
                            &coverLocation,
                            &LookAtPosition
                        );
                        LookAtPosition
                            .z = CombatUtilities::GetTargetFiringZOffset(pattacker, ACTOR_SEGMENT_INVIEW_TOP, false) +
                            LookAtPosition.z;
                        CheckCoverTimer.Reset(0.5f);
                    }
                    else {
                        CurrentDestination.Point = Destination.Point;
                        CurrentDestination.pSpace = Destination.pSpace;
                    }
                }

                spQueuedPathingRequest = static_cast<PathingRequest*>(nullptr);
            }
            if (pattacker->DidPathingFail()) {
                if (!pattacker->IsPathValid()) {
                    if (eState == CombatProcedureMove::MOVE_STATE_MOVING_USING_COVER) {
                        CombatCoverLocation* psecondaryCoverLocation = pcombatState->GetSecondaryCoverLocation();
                        if (psecondaryCoverLocation) {
                            pcombatState->AddUnreachableCoverLocation(psecondaryCoverLocation->iCoverLocationKey);
                            pcombatState->ClearSecondaryCoverLocation(false);
                        }
                    }
                    else {
                        switch (iLocation) {
                            case 0u:
                            case 1u:
                            case 7u:
                            case 8u: {
                                pcombatState->SetFlag(1 << iLocation);
                                break;
                            }
                            case 3u: {
                                pcombatState->ClearLastDodgeDirection();
                                break;
                            }
                            default: {
                                break;
                            }
                        }
                        CombatCoverLocation* pcoverLocation = pcombatState->GetCoverLocation();
                        if (pcoverLocation && pcoverLocation->iLocation == iLocation) {
                            pcombatState->AddUnreachableCoverLocation(pcoverLocation->iCoverLocationKey);
                            pcombatState->ClearCoverLocation(false);
                        }
                    }
                    pcombatState->AddUnreachableLocation(CurrentDestination);
                    pcombatState->ClearCombatLocation();
                    if (iLocation == 1 &&
                        eState != CombatProcedureMove::MOVE_STATE_MOVING_USING_COVER &&
                        CurrentDestination.Point != Destination.Point) {
                        pcombatState->AddUnreachableLocation(Destination);
                    }
                }
                if (pattacker->IsPathValid()) {
                    SetFailed("Pathing failed while moving to destination");
                }
                else {
                    SetFailed("Path Building failed to destination");
                }
            }
            else {
                if (pTargetRef && (pTargetRef->GetDisabled() || pTargetRef->GetDelete())) {
                    SetFailed("Target ref is now disabled or deleted");
                }
                else {
                    bool bcreateNewPath = false;
                    if (pTargetRef) {
                        PROCESS_TYPE etargetProcessLevel = PROCESS_TYPE_HIGH;
                        if (pTargetRef->IsActor()) {
                            Actor* ptargetRefActor = static_cast<Actor*>(pTargetRef);
                            etargetProcessLevel = ptargetRefActor->GetCurrentProcessType();
                        }
                        pTargetRef->GetWorldLocation(&targetWorldLocation);
                        float frepathDistanceSquared = 0.0f;
                        if (eattackerProcessLevel || etargetProcessLevel) {
                            frepathDistanceSquared = fTargetMovedRepathLengthLow.Float() *
                                fTargetMovedRepathLengthLow.Float();
                        }
                        else {
                            double dRepathLengthSquared;
                            if (eState == CombatProcedureMove::MOVE_STATE_MOVING_USING_COVER) {
                                dRepathLengthSquared = fTargetMovedCoveredMoveRepathLength.Float() *
                                    fTargetMovedCoveredMoveRepathLength.Float();
                            }
                            else {
                                dRepathLengthSquared = fTargetMovedRepathLength.Float() *
                                    fTargetMovedRepathLength.Float();
                            }
                            frepathDistanceSquared = dRepathLengthSquared;
                        }
                        if (static_cast<double>(frepathDistanceSquared) <
                            targetWorldLocation.GetDistanceSquared(Destination)) {
                            Destination = targetWorldLocation;
                            bcreateNewPath = true;
                        }
                    }
                    if (eState == CombatProcedureMove::MOVE_STATE_MOVING_USING_COVER && CheckCoverTimer.Check()) {
                        if (!bcreateNewPath && ptarget && pcombatState->GetHasBufferedDetection()) {
                            CombatCoverLocation* pcoverLocation = pcombatState->GetSecondaryCoverLocation();
                            if (pcoverLocation) {
                                ptarget->GetWorldLocation(&targetWorldLoc);
                                pattacker->GetWorldLocation(&attackerWorldLoc);
                                if (attackerWorldLoc.GetDistanceSquared(targetWorldLoc) < 16777216.0) {
                                    const NiPoint3* pTargetLocation = &ptarget->GetLocationOnReference();
                                    if (NavMeshMath::Find2DIntersection3(
                                            pcoverLocation->CoverLocation.GetLocation(),
                                            *pTargetLocation,
                                            pcoverLocation->CoverLocation.LeftVertex,
                                            pcoverLocation->CoverLocation.RightVertex
                                        )) {
                                        if (!pcombatState->CheckCoverSafe(
                                                pcoverLocation->CoverLocation,
                                                CurrentDestination.Point,
                                                ptarget,
                                                0
                                            )) {
                                            bcreateNewPath = true;
                                        }
                                    }
                                    else {
                                        bcreateNewPath = true;
                                    }
                                }
                            }
                            else {
                                bcreateNewPath = true;
                            }
                        }

                        CheckCoverTimer.Reset(0.5f);
                    }
                    if (bcreateNewPath) {
                        if (eState == CombatProcedureMove::MOVE_STATE_MOVING_USING_COVER) {
                            pcombatState->UnreserveSecondaryCoverLocation();
                            pcombatState->ClearSecondaryCoverLocation(false);
                            bReservedSecondaryLocation = false;
                        }
                        CurrentDestination.Point = Destination.Point;
                        CurrentDestination.pSpace = Destination.pSpace;
                        eState = CombatProcedureMove::MOVE_STATE_MOVING;
                        CreatePath();
                    }
                    else {
                        if (!bUseBackgroundCoveredMovePathing &&
                            eState == CombatProcedureMove::MOVE_STATE_MOVING &&
                            !pattacker->IsWaitingOnPath() &&
                            eattackerProcessLevel == PROCESS_TYPE_HIGH &&
                            kCombatLocationTypeData[iLocation].bUseCoveredMovement &&
                            pcombatState->GetPermission(1u)) {
                            float fminAdvanceDistance = 256.0f;
                            int32_t icurrentNode = static_cast<int32_t>(pattacker->GetCurrentNodeIndex());
                            if (iCurrentNode == -1) {
                                fminAdvanceDistance = 512.0f;
                            }
                            if (icurrentNode != iCurrentNode || CoveredMoveSearchTimer.Check()) {
                                iCurrentNode = icurrentNode;
                                CoveredMoveSearchTimer.Reset(2.0f);
                                NiPointer<PathingAvoidNodeArray> spavoidNodeArray(
                                    static_cast<PathingAvoidNodeArray*>(nullptr)
                                );
                                bool bcreatedAvoidNodeArray = false;
                                NiPoint3 previousNodeLocation = pattacker->GetLocationOnReference();
                                float fpathLength = 0.0f;
                                bool bdone = false;
                                while (!bdone && pattacker->GetPathingLocationAt(icurrentNode + 1, &nextNodeLocation)) {
                                    ++icurrentNode;
                                    NiPoint3 vec = nextNodeLocation - previousNodeLocation;
                                    float flength = vec.UnitizeGetLength();
                                    if (fpathLength + flength > 1536.0) {
                                        float fRemaining = 1536.0 - fpathLength;
                                        vec *= fRemaining;
                                        nextNodeLocation = previousNodeLocation + vec;
                                        bdone = true;
                                    }
                                    fpathLength = fpathLength + flength;
                                    previousNodeLocation = nextNodeLocation;
                                    if (!bcreatedAvoidNodeArray) {
                                        CreateAvoidNodeArray(spavoidNodeArray);
                                    }
                                    bcreatedAvoidNodeArray = true;
                                    PathingCoverLocation coveredMoveLocation;
                                    if (pcombatState->FindCoveredMoveLocation(
                                            &coveredMoveLocation,
                                            nextNodeLocation,
                                            iLocation,
                                            fminAdvanceDistance,
                                            spavoidNodeArray.m_pObject
                                        )) {
                                        bReservedSecondaryLocation = true;
                                        pcombatState->ReserveSecondaryCoverLocation();
                                        CombatCoverLocation*
                                            psecondaryCoverLocation = pcombatState->GetSecondaryCoverLocation();
                                        CombatUtilities::CalculateCoverPositions(
                                            pCombatController,
                                            psecondaryCoverLocation,
                                            &coverPosition,
                                            &LookAtPosition
                                        );
                                        LookAtPosition.z = CombatUtilities::GetTargetFiringZOffset(
                                                               pattacker,
                                                               ACTOR_SEGMENT_INVIEW_TOP,
                                                               false
                                                           ) +
                                            LookAtPosition.z;
                                        eState = CombatProcedureMove::MOVE_STATE_MOVING_USING_COVER;
                                        CurrentDestination.SetLocation(coverPosition, pattacker->GetSpace());
                                        if (!CreatePath()) {
                                            return;
                                        }
                                        CheckCoverTimer.Reset(0.5f);
                                        break;
                                    }
                                }
                            }
                        }
                        bool bforceComplete = false;
                        if (iLocation == 15 && CheckDestinationTimer.Check()) {
                            pattacker->GetWorldLocation(&attackerInvestigateLoc);
                            double dDistanceSquared = attackerInvestigateLoc.GetDistanceSquared(Destination);
                            if (fCombatSearchCheckInvestigateMaxDistance.Float() *
                                    fCombatSearchCheckInvestigateMaxDistance.Float() >
                                dDistanceSquared) {
                                PathingLocation investigatePathingLocation(&Destination);
                                if (Pathing::CheckLineOfSight(
                                        investigatePathingLocation,
                                        attackerInvestigateLoc.Point,
                                        nullptr,
                                        nullptr
                                    )) {
                                    bforceComplete = true;
                                }
                            }
                            CheckDestinationTimer.Reset(fCombatSearchCheckInvestigateTime.Float());
                        }
                        if (bforceComplete) {
                            pattacker->StopMoving();
                        }
                        if (bforceComplete || pattacker->IsPathingComplete()) {
                            if (eState == CombatProcedureMove::MOVE_STATE_MOVING_USING_COVER) {
                                eState = CombatProcedureMove::MOVE_STATE_HIDING_BEHIND_COVER;
                                HidingTimer.Reset(GetHidingTime());
                                CheckCoverTimer.Reset(0.25f);
                                SetLookAtTarget(LookAtPosition, 3);
                                SetMoveMode(2);
                            }
                            else {
                                pcombatState->ReachedDestination(Destination, static_cast<int32_t>(iLocation));
                                SetFinished();
                            }
                        }
                    }
                }
            }
            break;
        }
        case CombatProcedureMove::MOVE_STATE_HIDING_BEHIND_COVER: {
            bool bstopHiding = false;
            CombatCoverLocation* pcoverLocation = pcombatState->GetSecondaryCoverLocation();
            if (!HidingTimer.Check() && pcoverLocation) {
                if (ptarget) {
                    float fhidingTime = HidingTimer.fStartTime + 1.0;
                    if (static_cast<double>(fhidingTime) < CombatTimer::GetTimer()) {
                        if (pcombatState->bTargetLineOfSight) {
                            bstopHiding = true;
                        }
                        else {
                            double dDamagedTime = pcombatState->GetLastDamagedByAttackerTimeStamp().fTimeStamp;
                            if (fhidingTime < dDamagedTime) {
                                bstopHiding = true;
                            }
                        }
                    }
                    if (!bstopHiding && pcombatState->GetHasBufferedDetection() && CheckCoverTimer.Check()) {
                        if (!pcombatState->CheckCoverSafe(
                                pcoverLocation->CoverLocation,
                                CurrentDestination.Point,
                                ptarget,
                                1
                            )) {
                            bstopHiding = true;
                        }
                        CheckCoverTimer.Reset(0.25f);
                    }
                }
            }
            else {
                bstopHiding = true;
            }
            if (bstopHiding) {
                eState = CombatProcedureMove::MOVE_STATE_INITIALIZING;
                pcombatState->UnreserveSecondaryCoverLocation();
                bReservedSecondaryLocation = false;
                pcombatState->ClearSecondaryCoverLocation(false);
                ClearLookAtTarget();
            }
            break;
        }
        default: {
            return;
        }
    }
}

void CombatProcedureMove::Shutdown() {
    Actor* pattacker = pCombatController->GetAttacker();
    if (eState == 1 || eState == 2) {
        if (!GetFinished()) {
            pattacker->StopMoving();
        }
    }
    else if (eState == 3) {
        ClearLookAtTarget();
    }
    ClearMoveMode();
    if (bReservedSecondaryLocation) {
        pCombatController->pCombatState->UnreserveSecondaryCoverLocation();
    }
}

void CombatProcedureMove::ClearReferencePointers(TESObjectREFR* apRef) {
    if (pTargetRef == apRef) {
        pTargetRef = nullptr;
        SetFailed("Targeted reference pointer was cleared");
    }
}

// Each timer's values are passed as doubles: fTargetTime is widened from its float, and ElapsedTime's result is passed
// as a double.
void CombatProcedureMove::DisplayDebugText(
    IDebugText* apDebugText,
    DEBUGTEXT_LOCATION aeLocation,
    DEBUGTEXT_JUSTIFY aeJustify
) {
    switch (eState) {
        case CombatProcedureMove::MOVE_STATE_INITIALIZING: {
            apDebugText->PrintLine(aeLocation, aeJustify, "INITIALIZING");
            break;
        }
        case CombatProcedureMove::MOVE_STATE_MOVING: {
            apDebugText->PrintLine(aeLocation, aeJustify, "MOVING");
            break;
        }
        case CombatProcedureMove::MOVE_STATE_MOVING_USING_COVER: {
            apDebugText->PrintLine(aeLocation, aeJustify, "MOVING_USING_COVER");
            double dCheckTarget = CheckCoverTimer.fTargetTime;
            double dCheckElapsed = CheckCoverTimer.ElapsedTime();
            apDebugText
                ->PrintLine(aeLocation, aeJustify, "Check Cover Timer: %.02f/%.02f", dCheckElapsed, dCheckTarget);
            break;
        }
        case CombatProcedureMove::MOVE_STATE_HIDING_BEHIND_COVER: {
            apDebugText->PrintLine(aeLocation, aeJustify, "HIDING_BEHIND_COVER");
            double dHidingTarget = HidingTimer.fTargetTime;
            double dHidingElapsed = HidingTimer.ElapsedTime();
            apDebugText->PrintLine(aeLocation, aeJustify, "Hiding Timer: %.02f/%.02f", dHidingElapsed, dHidingTarget);
            double dCheckTarget = CheckCoverTimer.fTargetTime;
            double dCheckElapsed = CheckCoverTimer.ElapsedTime();
            apDebugText
                ->PrintLine(aeLocation, aeJustify, "Check Cover Timer: %.02f/%.02f", dCheckElapsed, dCheckTarget);
            break;
        }
        default: {
            break;
        }
    }
    NiPoint3 kDestination = Destination.Point;
    TESForm* pdestinationSpace = Destination.pSpace;
    if (!pdestinationSpace || pdestinationSpace == pCombatController->pAttacker->GetSpace()) {
        apDebugText->PrintLine(
            aeLocation,
            aeJustify,
            "Destination: (%.02f,%.02f,%.02f)",
            kDestination.x,
            kDestination.y,
            kDestination.z
        );
    }
    else {
        const char* pcEditorID = pdestinationSpace->GetFormEditorID();
        apDebugText->PrintLine(
            aeLocation,
            aeJustify,
            "Destination: %s (%.02f,%.02f,%.02f)",
            pcEditorID,
            kDestination.x,
            kDestination.y,
            kDestination.z
        );
    }
    if (pTargetRef) {
        const char* pcName;
        NiPoint3 kTargetLocation = pTargetRef->GetLocationOnReference();
        if (pTargetRef->GetFormEditorIDLength()) {
            pcName = pTargetRef->GetFormEditorID();
        }
        else {
            pcName = pTargetRef->GetFullName();
        }
        apDebugText->PrintLine(
            aeLocation,
            aeJustify,
            "Target: '%s' (%08X) - Position: (%.02f,%.02f,%.02f)",
            pcName,
            pTargetRef->iFormID,
            kTargetLocation.x,
            kTargetLocation.y,
            kTargetLocation.z
        );
    }
    if (CheckDestinationTimer.IsSet()) {
        double dDestTarget = CheckDestinationTimer.fTargetTime;
        double dDestElapsed = CheckDestinationTimer.ElapsedTime();
        apDebugText
            ->PrintLine(aeLocation, aeJustify, "Check Destination Timer: %.02f/%.02f", dDestElapsed, dDestTarget);
    }
}

void CombatProcedureMove::StartedPlanning() {
    if (bReservedSecondaryLocation) {
        pCombatController->pCombatState->UnreserveSecondaryCoverLocation();
        bReservedSecondaryLocation = false;
    }
}

void CombatProcedureMove::FinishedPlanning() {
    if (eState == 2 || eState == 3) {
        pCombatController->pCombatState->ReserveSecondaryCoverLocation();
        bReservedSecondaryLocation = true;
    }
}

CombatProcedure::COMBAT_PROCEDURES CombatProcedureMove::GetType() {
    return MOVE_TO_LOCATION;
}

void CombatProcedureMove::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    CombatProcedure::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&eState)), sizeof(eState), 0);
    Destination.SaveGame(apSaveGameBuffer);
    CurrentDestination.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fTargetRadius)), sizeof(fTargetRadius), 0);
    apSaveGameBuffer->SaveFormID(pTargetRef, 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&iLocation)), sizeof(iLocation), 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&iCurrentNode)), sizeof(iCurrentNode), 0);
    CoveredMoveSearchTimer.SaveGame(apSaveGameBuffer);
    HidingTimer.SaveGame(apSaveGameBuffer);
    CheckCoverTimer.SaveGame(apSaveGameBuffer);
    CheckDestinationTimer.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer
        ->SaveDataEndian(static_cast<char*>(static_cast<void*>(&LookAtPosition)), sizeof(LookAtPosition), 0);
    apSaveGameBuffer->SaveDataEndian(
        static_cast<char*>(static_cast<void*>(&bReservedSecondaryLocation)),
        sizeof(bReservedSecondaryLocation),
        0
    );
    bHasQueuedPathingRequest = (spQueuedPathingRequest.m_pObject != nullptr);
    apSaveGameBuffer->SaveDataEndian(
        static_cast<char*>(static_cast<void*>(&bHasQueuedPathingRequest)),
        sizeof(bHasQueuedPathingRequest),
        0
    );
}

void CombatProcedureMove::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&eState, sizeof(eState));
    Destination.LoadGame(apLoadGameBuffer);
    CurrentDestination.LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&fTargetRadius, sizeof(fTargetRadius));
    apLoadGameBuffer->LoadFormID(pTargetRef);
    apLoadGameBuffer->LoadData(&iLocation, sizeof(iLocation));
    apLoadGameBuffer->LoadData(&iCurrentNode, sizeof(iCurrentNode));
    CoveredMoveSearchTimer.LoadGame(apLoadGameBuffer);
    HidingTimer.LoadGame(apLoadGameBuffer);
    CheckCoverTimer.LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetVersion() >= 8u) {
        CheckDestinationTimer.LoadGame(apLoadGameBuffer);
    }
    apLoadGameBuffer->LoadData(&LookAtPosition, sizeof(LookAtPosition));
    apLoadGameBuffer->LoadData(&bReservedSecondaryLocation, sizeof(bReservedSecondaryLocation));
    if (apLoadGameBuffer->GetVersion() >= 12u) {
        apLoadGameBuffer->LoadData(&bHasQueuedPathingRequest, sizeof(bHasQueuedPathingRequest));
    }
}

void CombatProcedureMove::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESObjectREFR* pTarget;
    Actor* pattacker = GetAttacker();
    if (pTargetRef) {
        TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTargetRef)));
        pTarget = dynamic_cast<TESObjectREFR*>(pForm);
    }
    else {
        pTarget = nullptr;
    }
    pTargetRef = pTarget;
    if (bHasQueuedPathingRequest) {
        spQueuedPathingRequest = pattacker->GetCurrentPathingRequest();
    }
}

bool CombatProcedureMove::CreatePath(NiPointer<PathingAvoidNodeArray>& arspAvoidNodeArray) {
    NiPoint3 faceTargetLocation;
    Actor* pattacker = GetAttacker();
    CombatState* pcombatState = pCombatController->pCombatState;
    COMBAT_MOVE_MODE ecombatMoveMode = COMBAT_MOVE_MODE_RUN;
    NiPoint3 center = NiPoint3::ZERO;
    float fcenterRadius = 0.0f;
    uint32_t iLocation = this->iLocation;
    if (iLocation) {
        if (iLocation == 3) {
            center = pattacker->GetLocationOnReference();
            fcenterRadius = 640.0f;
        }
    }
    else if (pCombatController->CombatPlanContainsAction(24)) {
        ecombatMoveMode = COMBAT_MOVE_MODE_SLOW_RUN;
    }
    CombatLocationTarget* plocationTarget = pcombatState->pLocationTarget;
    if (plocationTarget &&
        plocationTarget->cType == 1 &&
        !pcombatState->GetLocationUnreachable(256) &&
        this->iLocation != 8) {
        TESForm* pSpace = plocationTarget->WorldLoc.pSpace;
        if (pSpace == pattacker->GetSpace()) {
            center = plocationTarget->WorldLoc.Point;
            fcenterRadius = pcombatState->GetLocationTargetDistance();
            NiPoint3 vec = center - pattacker->GetLocationOnReference();
            double dDistanceSquared = vec.SqrLength();
            if (fcenterRadius * fcenterRadius < dDistanceSquared) {
                fcenterRadius = 0.0f;
            }
        }
    }

    float fradius = 0.0f;
    if (eState == CombatProcedureMove::MOVE_STATE_MOVING) {
        fradius = fTargetRadius;
    }
    if (this->iLocation == 1 && eState == CombatProcedureMove::MOVE_STATE_MOVING && pTargetRef) {
        ::new (&faceTargetLocation)
            NiPoint3(CombatUtilities::ComputeFaceTargetLocation(pattacker, pTargetRef, Destination.Point, -1.0f));
        PathingLocation pathingLocation(faceTargetLocation, pattacker);
        if (pathingLocation.IsValidLocation(pattacker->GetRadius())) {
            if (CombatUtilities::CalculateLOS(
                    pattacker,
                    pTargetRef,
                    faceTargetLocation,
                    Destination.Point,
                    false,
                    -1
                )) {
                CurrentDestination.SetPoint(faceTargetLocation);
                if (!arspAvoidNodeArray.m_pObject) {
                    arspAvoidNodeArray = new PathingAvoidNodeArray;
                }
                float fAvoidCost = fMovementNearTargetAvoidCost.Float();
                float fAvoidRadius = fMovementNearTargetAvoidRadius.Float();
                const NiPoint3* pTargetLocation = &pTargetRef->GetLocationOnReference();
                arspAvoidNodeArray->Add(PathingAvoidNode(*pTargetLocation, fAvoidRadius, fAvoidCost));
            }
        }
    }

    PathingRequest* prequest = nullptr;
    PathingRequest request;
    PathingRequestCoveredMove coveredMoveRequest;
    if (bUseBackgroundCoveredMovePathing &&
        kCombatLocationTypeData[this->iLocation].bUseCoveredMovement &&
        pcombatState->GetPermission(1u) &&
        pattacker->GetCurrentProcessType() == PROCESS_TYPE_HIGH) {
        pcombatState->BuildCoveredMoveRequest(&coveredMoveRequest);
        coveredMoveRequest.iLocationIndex = this->iLocation;
        prequest = &coveredMoveRequest;
    }
    else {
        prequest = &request;
    }
    bool bhardRadius = false;
    if (fradius < 0.0) {
        fradius = -fradius;
        bhardRadius = true;
    }
    pattacker->BuildRequest_0(
        prequest,
        CurrentDestination.Point,
        CurrentDestination.pSpace,
        fradius,
        arspAvoidNodeArray.m_pObject
    );
    prequest->SetCenterRadius(center, fcenterRadius);
    if (bhardRadius) {
        prequest->SetSoftGoalRadius(false);
    }
    if (pattacker->SetPathfindingGoal(prequest)) {
        iCurrentNode = -1;
        spQueuedPathingRequest = pattacker->GetCurrentPathingRequest();
        SetMoveMode(ecombatMoveMode);
        return true;
    }

    SetFailed("Cannot setup path to destination");
    return false;
}

void CombatProcedureMove::CreateAvoidNodeArray(NiPointer<PathingAvoidNodeArray>& arspAvoidNodeArray) {
    Actor* pattacker = GetAttacker();
    CombatState* pcombatState = pCombatController->pCombatState;
    const NiPoint3* pLocation = &pattacker->GetLocationOnReference();
    arspAvoidNodeArray = pCombatManager->pThreatMap->CreateAvoidNodeArray(*pLocation, pattacker, -1.0f);
    float fTimeStamp = CombatTimer::GetTimer();
    pcombatState->SetThreatsConsideredTimeStamp(fTimeStamp);
    Actor* pignoreActor = nullptr;
    int32_t iLocation = this->iLocation;
    if (iLocation >= 0) {
        if (iLocation <= 1) {
            pignoreActor = GetTarget();
        }
        else if (iLocation == 4) {
            arspAvoidNodeArray = pCombatController->pCombatGroup->CreateFlankingAvoidNodeArray(
                pattacker,
                Destination.Point,
                0.0f,
                arspAvoidNodeArray.m_pObject
            );
        }
    }
    if (kCombatLocationTypeData[this->iLocation].bAvoidTargets) {
        arspAvoidNodeArray = pCombatController->pCombatGroup->CreateMovementAvoidNodeArray(
            pattacker,
            Destination.Point,
            0.0f,
            arspAvoidNodeArray.m_pObject,
            pignoreActor
        );
    }
}

// iLocation is compared signed.
float CombatProcedureMove::GetHidingTime() {
    int32_t iLoc = static_cast<int32_t>(iLocation);
    if (iLoc < 6) {
        return 1.0f;
    }
    if (iLoc <= 7) {
        return 2.0f;
    }
    if (iLoc == 8) {
        return 0.5f;
    }
    return 1.0f;
}

NiPoint3 CombatProcedureMove::GetDestinationPos() {
    return *Destination.GetPoint();
}

bool CombatProcedureMove::CreatePath() {
    NiPointer<PathingAvoidNodeArray> aspArray(static_cast<PathingAvoidNodeArray*>(nullptr));
    CreateAvoidNodeArray(aspArray);
    return CreatePath(aspArray);
}

void CombatState::ReserveSecondaryCoverLocation() {
    ReserveCoverLocation(pSecondaryCoverLocation);
}

void CombatState::SetThreatsConsideredTimeStamp(float afValue) {
    CombatTimeStamp kTimeStamp(afValue);
    fThreatsConsideredTimeStamp = kTimeStamp;
}

void CombatState::SetSecondaryCoverSearchInfo(CombatCoverSearchInfo* apCoverSearchInfo) {
    SecondaryCoverSearchInfo.Copy(apCoverSearchInfo);
}
