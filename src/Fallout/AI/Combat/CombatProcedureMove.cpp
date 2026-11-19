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

// Takes the destination, the target reference and the radius's sign (flag 2) from apCombatLocation when there is one;
// the look-at position is deliberately left unset, as the game does.
CombatProcedureMove::CombatProcedureMove(CombatLocation* apCombatLocation, float afTargetRadius, int32_t aiLocation)
  : eState(CombatProcedureMove::MOVE_STATE_INITIALIZING), fTargetRadius(afTargetRadius), pTargetRef(nullptr),
    iLocation(aiLocation), bReservedSecondaryLocation(false), spQueuedPathingRequest(nullptr),
    bHasQueuedPathingRequest(false), iCurrentNode(static_cast<uint32_t>(-1)) {
    if (apCombatLocation) {
        Destination = apCombatLocation->WorldLoc;
        CurrentDestination = Destination;
        pTargetRef = apCombatLocation->pRef;
        if (apCombatLocation->GetFlag(2u)) {
            fTargetRadius = -fTargetRadius;
        }
    }
}

CombatProcedureMove::~CombatProcedureMove() {}

void CombatProcedureMove::Update() {
    BGSWorldLocation attackerWorldLoc_2;
    NiPoint3 coverLocation_3;
    PathingCoverLocation coverLocation_2;
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
            CurrentDestination.Point.x = Destination.Point.x;
            CurrentDestination.Point.y = Destination.Point.y;
            CurrentDestination.Point.z = Destination.Point.z;
            CurrentDestination.pSpace = Destination.pSpace;
            eState = CombatProcedureMove::MOVE_STATE_MOVING;
            if (CreatePath() && iLocation == 15) {
                float* v1 = &fCombatSearchCheckInvestigateInitialTime.Float();
                CheckDestinationTimer.Reset(*v1);
            }
            break;
        }
        case CombatProcedureMove::MOVE_STATE_MOVING:
        case CombatProcedureMove::MOVE_STATE_MOVING_USING_COVER: {
            if (spQueuedPathingRequest.m_pObject && !pattacker->IsWaitingOnPath()) {
                PathingRequestCoveredMove* prequest =
                    static_cast<PathingRequestCoveredMove*>(spQueuedPathingRequest.m_pObject);
                if ((prequest->GetType() == 8) && (prequest->bFinished)) {
                    pcombatState->SetSecondaryCoverSearchInfo(prequest->pCombatCoverSearchInfo);
                    CombatCoverLocation* pcombatCoverLocation = prequest->pCombatCoverLocation;
                    pcombatState->SetSecondaryCoverLocation(pcombatCoverLocation);
                    prequest->pCombatCoverLocation = nullptr;
                    if (pcombatCoverLocation) {
                        TESForm* a2 = pattacker->GetSpace();
                        NiPoint3* ScriptList = &prequest->CoverLocation;
                        CurrentDestination.SetLocation(*ScriptList, a2);
                        if (!pcombatState->TryReserveSecondaryCoverLocation()) {
                            spQueuedPathingRequest = static_cast<PathingRequest*>(nullptr);
                            CurrentDestination.Point.x = Destination.Point.x;
                            CurrentDestination.Point.y = Destination.Point.y;
                            CurrentDestination.Point.z = Destination.Point.z;
                            CurrentDestination.pSpace = Destination.pSpace;
                            eState = CombatProcedureMove::MOVE_STATE_MOVING;
                            CreatePath();
                            return;
                        }
                        eState = CombatProcedureMove::MOVE_STATE_MOVING_USING_COVER;
                        bReservedSecondaryLocation = true;
                        CombatCoverLocation* SecondaryCoverLocation = pcombatState->GetSecondaryCoverLocation();
                        CombatUtilities::CalculateCoverPositions(
                            pCombatController, SecondaryCoverLocation, &coverLocation, &LookAtPosition
                        );
                        LookAtPosition.z =
                            CombatUtilities::GetTargetFiringZOffset(pattacker, ACTOR_SEGMENT_INVIEW_TOP, false) +
                            LookAtPosition.z;
                        CheckCoverTimer.Reset(0.5f);
                    }
                    else {
                        CurrentDestination.Point.x = Destination.Point.x;
                        CurrentDestination.Point.y = Destination.Point.y;
                        CurrentDestination.Point.z = Destination.Point.z;
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
                        if ((pcoverLocation) && (pcoverLocation->iLocation == iLocation)) {
                            pcombatState->AddUnreachableCoverLocation(pcoverLocation->iCoverLocationKey);
                            pcombatState->ClearCoverLocation(false);
                        }
                    }
                    pcombatState->AddUnreachableLocation(CurrentDestination);
                    pcombatState->ClearCombatLocation();
                    if (iLocation == 1 && eState != CombatProcedureMove::MOVE_STATE_MOVING_USING_COVER &&
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
                            float* v6 = &fTargetMovedRepathLengthLow.Float();
                            frepathDistanceSquared = *v6 * fTargetMovedRepathLengthLow.Float();
                        }
                        else {
                            double v26;
                            if (eState == CombatProcedureMove::MOVE_STATE_MOVING_USING_COVER) {
                                float* v4 = &fTargetMovedCoveredMoveRepathLength.Float();
                                v26 = *v4 * fTargetMovedCoveredMoveRepathLength.Float();
                            }
                            else {
                                float* v5 = &fTargetMovedRepathLength.Float();
                                v26 = *v5 * fTargetMovedRepathLength.Float();
                            }
                            frepathDistanceSquared = v26;
                        }
                        if (static_cast<double>(frepathDistanceSquared) <
                            targetWorldLocation.GetDistanceSquared(Destination)) {
                            Destination = targetWorldLocation;
                            bcreateNewPath = true;
                        }
                    }
                    if (eState == CombatProcedureMove::MOVE_STATE_MOVING_USING_COVER && CheckCoverTimer.Check()) {
                        if ((!bcreateNewPath) && (ptarget) && (pcombatState->GetHasBufferedDetection())) {
                            CombatCoverLocation* pcoverLocation_2 = pcombatState->GetSecondaryCoverLocation();
                            if (pcoverLocation_2) {
                                ptarget->GetWorldLocation(&targetWorldLoc);
                                pattacker->GetWorldLocation(&attackerWorldLoc);
                                if (attackerWorldLoc.GetDistanceSquared(targetWorldLoc) < 16777216.0) {
                                    NiPoint3* a2a = &pcoverLocation_2->CoverLocation.RightVertex;
                                    NiPoint3* afMinDistance = &pcoverLocation_2->CoverLocation.LeftVertex;
                                    const NiPoint3* v14 = &ptarget->GetLocationOnReference();
                                    if (NavMeshMath::Find2DIntersection3(
                                            pcoverLocation_2->CoverLocation.GetLocation(), *v14, *afMinDistance, *a2a
                                        )) {
                                        NiPoint3* Point = &(&CurrentDestination)->Point;
                                        if (!pcombatState->CheckCoverSafe(
                                                pcoverLocation_2->CoverLocation, *Point, ptarget, 0
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
                        CurrentDestination.Point.x = Destination.Point.x;
                        CurrentDestination.Point.y = Destination.Point.y;
                        CurrentDestination.Point.z = Destination.Point.z;
                        CurrentDestination.pSpace = Destination.pSpace;
                        eState = CombatProcedureMove::MOVE_STATE_MOVING;
                        CreatePath();
                    }
                    else {
                        if (!bUseBackgroundCoveredMovePathing && eState == CombatProcedureMove::MOVE_STATE_MOVING &&
                            !pattacker->IsWaitingOnPath() && eattackerProcessLevel == PROCESS_TYPE_HIGH &&
                            kCombatLocationTypeData[iLocation].bUseCoveredMovement && pcombatState->GetPermission(1u)) {
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
                                        float v25 = 1536.0 - fpathLength;
                                        vec *= v25;
                                        nextNodeLocation = previousNodeLocation + vec;
                                        bdone = true;
                                    }
                                    fpathLength = fpathLength + flength;
                                    previousNodeLocation = nextNodeLocation;
                                    if (!bcreatedAvoidNodeArray) {
                                        CreateAvoidNodeArray(spavoidNodeArray);
                                    }
                                    bcreatedAvoidNodeArray = true;
                                    coverLocation_2.~PathingCoverLocation();
                                    ::new (&coverLocation_2) PathingCoverLocation;
                                    PathingAvoidNodeArray* a2b = spavoidNodeArray.m_pObject;
                                    if (pcombatState->FindCoveredMoveLocation(
                                            &coverLocation_2, nextNodeLocation, iLocation, fminAdvanceDistance, a2b
                                        )) {
                                        bReservedSecondaryLocation = true;
                                        pcombatState->ReserveSecondaryCoverLocation();
                                        CombatCoverLocation* v8 = pcombatState->GetSecondaryCoverLocation();
                                        CombatUtilities::CalculateCoverPositions(
                                            pCombatController, v8, &coverLocation_3, &LookAtPosition
                                        );
                                        LookAtPosition.z = CombatUtilities::GetTargetFiringZOffset(
                                                               pattacker, ACTOR_SEGMENT_INVIEW_TOP, false
                                                           ) +
                                            LookAtPosition.z;
                                        eState = CombatProcedureMove::MOVE_STATE_MOVING_USING_COVER;
                                        TESForm* Space = pattacker->GetSpace();
                                        CurrentDestination.SetLocation(coverLocation_3, Space);
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
                            pattacker->GetWorldLocation(&attackerWorldLoc_2);
                            double DistanceSquared = attackerWorldLoc_2.GetDistanceSquared(Destination);
                            float* v10 = &fCombatSearchCheckInvestigateMaxDistance.Float();
                            if (*v10 * fCombatSearchCheckInvestigateMaxDistance.Float() > DistanceSquared) {
                                PathingLocation investigatePathingLocation(&Destination);
                                if (Pathing::CheckLineOfSight(
                                        investigatePathingLocation, attackerWorldLoc_2.Point, nullptr, nullptr
                                    )) {
                                    bforceComplete = true;
                                }
                            }
                            float* v11 = &fCombatSearchCheckInvestigateTime.Float();
                            CheckDestinationTimer.Reset(*v11);
                        }
                        if (bforceComplete) {
                            pattacker->StopMoving();
                        }
                        if (bforceComplete || pattacker->IsPathingComplete()) {
                            if (eState == CombatProcedureMove::MOVE_STATE_MOVING_USING_COVER) {
                                eState = CombatProcedureMove::MOVE_STATE_HIDING_BEHIND_COVER;
                                float a2c = GetHidingTime();
                                HidingTimer.Reset(a2c);
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
            CombatCoverLocation* pcoverLocation_3 = pcombatState->GetSecondaryCoverLocation();
            if (!HidingTimer.Check() && pcoverLocation_3) {
                if (ptarget) {
                    float fhidingTime = HidingTimer.fStartTime + 1.0;
                    if (static_cast<double>(fhidingTime) < CombatTimer::GetTimer()) {
                        if (pcombatState->bTargetLineOfSight) {
                            bstopHiding = true;
                        }
                        else {
                            double DecalMinWidth = pcombatState->GetLastDamagedByAttackerTimeStamp().fTimeStamp;
                            if (fhidingTime < DecalMinWidth) {
                                bstopHiding = true;
                            }
                        }
                    }
                    if (!bstopHiding && pcombatState->GetHasBufferedDetection() && CheckCoverTimer.Check()) {
                        NiPoint3* v16 = &(&CurrentDestination)->Point;
                        if (!pcombatState->CheckCoverSafe(pcoverLocation_3->CoverLocation, *v16, ptarget, 1)) {
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
    Actor* Attacker = pCombatController->GetAttacker();
    if (eState == 1 || eState == 2) {
        if (!GetFinished()) {
            Attacker->StopMoving();
        }
    }
    else {
        if (eState == 3) {
            ClearLookAtTarget();
        }
    }
    ClearMoveMode();
    if (bReservedSecondaryLocation) {
        CombatState* MusicType = pCombatController->pCombatState;
        MusicType->UnreserveSecondaryCoverLocation();
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
            apDebugText->PrintLine(
                aeLocation, aeJustify, "Check Cover Timer: %.02f/%.02f", dCheckElapsed, dCheckTarget
            );
            break;
        }
        case CombatProcedureMove::MOVE_STATE_HIDING_BEHIND_COVER: {
            apDebugText->PrintLine(aeLocation, aeJustify, "HIDING_BEHIND_COVER");
            double dHidingTarget = HidingTimer.fTargetTime;
            double dHidingElapsed = HidingTimer.ElapsedTime();
            apDebugText->PrintLine(aeLocation, aeJustify, "Hiding Timer: %.02f/%.02f", dHidingElapsed, dHidingTarget);
            double dCheckTarget = CheckCoverTimer.fTargetTime;
            double dCheckElapsed = CheckCoverTimer.ElapsedTime();
            apDebugText->PrintLine(
                aeLocation, aeJustify, "Check Cover Timer: %.02f/%.02f", dCheckElapsed, dCheckTarget
            );
            break;
        }
        default: {
            break;
        }
    }
    NiPoint3 v18 = Destination.Point;
    TESForm* pdestinationSpace = Destination.pSpace;
    if (!pdestinationSpace || pdestinationSpace == pCombatController->pAttacker->GetSpace()) {
        apDebugText->PrintLine(aeLocation, aeJustify, "Destination: (%.02f,%.02f,%.02f)", v18.x, v18.y, v18.z);
    }
    else {
        const char* v7 = pdestinationSpace->GetFormEditorID();
        apDebugText->PrintLine(aeLocation, aeJustify, "Destination: %s (%.02f,%.02f,%.02f)", v7, v18.x, v18.y, v18.z);
    }
    if (pTargetRef) {
        const char* FullName;
        NiPoint3 v16 = pTargetRef->GetLocationOnReference();
        if (pTargetRef->GetFormEditorIDLength()) {
            FullName = pTargetRef->GetFormEditorID();
        }
        else {
            FullName = pTargetRef->GetFullName();
        }
        int32_t CastingType = pTargetRef->iFormID;
        apDebugText->PrintLine(
            aeLocation, aeJustify, "Target: '%s' (%08X) - Position: (%.02f,%.02f,%.02f)", FullName, CastingType, v16.x,
            v16.y, v16.z
        );
    }
    if (CheckDestinationTimer.IsSet()) {
        double dDestTarget = CheckDestinationTimer.fTargetTime;
        double dDestElapsed = CheckDestinationTimer.ElapsedTime();
        apDebugText->PrintLine(
            aeLocation, aeJustify, "Check Destination Timer: %.02f/%.02f", dDestElapsed, dDestTarget
        );
    }
}

void CombatProcedureMove::StartedPlanning() {
    if (bReservedSecondaryLocation) {
        CombatState* MusicType = pCombatController->pCombatState;
        MusicType->UnreserveSecondaryCoverLocation();
        bReservedSecondaryLocation = false;
    }
}

void CombatProcedureMove::FinishedPlanning() {
    if (eState == 2 || eState == 3) {
        CombatState* MusicType = pCombatController->pCombatState;
        MusicType->ReserveSecondaryCoverLocation();
        bReservedSecondaryLocation = true;
    }
}

CombatProcedure::COMBAT_PROCEDURES CombatProcedureMove::GetType() {
    return MOVE_TO_LOCATION;
}

void CombatProcedureMove::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    CombatProcedure::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&eState)), 4u, 0);
    (&Destination)->SaveGame(apSaveGameBuffer);
    (&CurrentDestination)->SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fTargetRadius)), 4u, 0);
    apSaveGameBuffer->SaveFormID(pTargetRef, (uint32_t)0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&iLocation)), 4u, 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&iCurrentNode)), 4u, 0);
    (&CoveredMoveSearchTimer)->SaveGame(apSaveGameBuffer);
    (&HidingTimer)->SaveGame(apSaveGameBuffer);
    (&CheckCoverTimer)->SaveGame(apSaveGameBuffer);
    (&CheckDestinationTimer)->SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&LookAtPosition)), 0xCu, 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bReservedSecondaryLocation)), 1u, 0);
    bHasQueuedPathingRequest = ((&spQueuedPathingRequest)->m_pObject != nullptr);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bHasQueuedPathingRequest)), 1u, 0);
}

void CombatProcedureMove::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&eState, 4u);
    Destination.LoadGame(apLoadGameBuffer);
    CurrentDestination.LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&fTargetRadius, 4u);
    apLoadGameBuffer->LoadFormID(pTargetRef);
    apLoadGameBuffer->LoadData(&iLocation, 4u);
    apLoadGameBuffer->LoadData(&iCurrentNode, 4u);
    CoveredMoveSearchTimer.LoadGame(apLoadGameBuffer);
    HidingTimer.LoadGame(apLoadGameBuffer);
    CheckCoverTimer.LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetVersion() >= 8u) {
        CheckDestinationTimer.LoadGame(apLoadGameBuffer);
    }
    apLoadGameBuffer->LoadData(&LookAtPosition, 12u);
    apLoadGameBuffer->LoadData(&bReservedSecondaryLocation, 1u);
    if (apLoadGameBuffer->GetVersion() >= 12u) {
        apLoadGameBuffer->LoadData(&bHasQueuedPathingRequest, 1u);
    }
}

void CombatProcedureMove::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESObjectREFR* v3;
    Actor* pattacker = GetAttacker();
    if (pTargetRef) {
        TESForm* FormByNumericID =
            TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTargetRef)));
        v3 = dynamic_cast<TESObjectREFR*>(FormByNumericID);
    }
    else {
        v3 = nullptr;
    }
    pTargetRef = v3;
    if (bHasQueuedPathingRequest) {
        spQueuedPathingRequest = pattacker->GetCurrentPathingRequest();
    }
}

bool CombatProcedureMove::CreatePath(NiPointer<PathingAvoidNodeArray>& arspAvoidNodeArray) {
    NiPoint3 faceTargetLocation;
    Actor* pattacker = GetAttacker();
    Actor* ptarget = GetTarget();
    CombatState* pcombatState = pCombatController->pCombatState;
    COMBAT_MOVE_MODE ecombatMoveMode = COMBAT_MOVE_MODE_RUN;
    NiPoint3 center = NiPoint3_ZERO;
    float fcenterRadius = 0.0f;
    uint32_t iLocation = this->iLocation;
    if (iLocation) {
        if (iLocation == 3) {
            center = pattacker->GetLocationOnReference();
            fcenterRadius = 640.0f;
        }
    }
    else {
        if (pCombatController->CombatPlanContainsAction(24)) {
            ecombatMoveMode = COMBAT_MOVE_MODE_SLOW_RUN;
        }
    }
    CombatLocationTarget* plocationTarget = pcombatState->pLocationTarget;
    if ((plocationTarget) &&
        (plocationTarget->cType == 1 && !pcombatState->GetLocationUnreachable(256) && this->iLocation != 8)) {
        TESForm* Space = plocationTarget->WorldLoc.pSpace;
        if (Space == pattacker->GetSpace()) {
            center = plocationTarget->WorldLoc.Point;
            fcenterRadius = pcombatState->GetLocationTargetDistance();
            const NiPoint3* v3 = &pattacker->GetLocationOnReference();
            NiPoint3 vec = center - *v3;
            double v4 = vec.SqrLength();
            if (fcenterRadius * fcenterRadius < v4) {
                fcenterRadius = 0.0f;
            }
        }
    }

    float fradius = 0.0f;
    if (eState == CombatProcedureMove::MOVE_STATE_MOVING) {
        fradius = fTargetRadius;
    }
    if ((this->iLocation == 1 && eState == CombatProcedureMove::MOVE_STATE_MOVING) && (pTargetRef)) {
        NiPoint3* afRadius = &(&Destination)->Point;
        ::new (&faceTargetLocation)
            NiPoint3(CombatUtilities::ComputeFaceTargetLocation(pattacker, pTargetRef, *afRadius, -1.0f));
        PathingLocation pathingLocation(faceTargetLocation, pattacker);
        float a3 = pattacker->GetRadius();
        if (pathingLocation.IsValidLocation(a3)) {
            NiPoint3* Point = &(&Destination)->Point;
            if (CombatUtilities::CalculateLOS(pattacker, pTargetRef, faceTargetLocation, *Point, false, -1)) {
                CurrentDestination.SetPoint(faceTargetLocation);
                if (!arspAvoidNodeArray.m_pObject) {
                    PathingAvoidNodeArray* v17;
                    PathingAvoidNodeArray* v21 =
                        static_cast<PathingAvoidNodeArray*>(NiMemObject::operator new(sizeof(PathingAvoidNodeArray)));
                    if (v21) {
                        v17 = ::new (v21) PathingAvoidNodeArray;
                    }
                    else {
                        v17 = nullptr;
                    }
                    arspAvoidNodeArray = v17;
                }
                float a3a = fMovementNearTargetAvoidCost.Float();
                float afRadiusa = fMovementNearTargetAvoidRadius.Float();
                const NiPoint3* v5 = &pTargetRef->GetLocationOnReference();
                arspAvoidNodeArray.m_pObject->Add(PathingAvoidNode(*v5, afRadiusa, a3a));
            }
        }
    }

    PathingRequest* prequest = nullptr;
    PathingRequest request;
    PathingRequestCoveredMove coveredMoveRequest;
    if (bUseBackgroundCoveredMovePathing &&
        (kCombatLocationTypeData[this->iLocation].bUseCoveredMovement &&
         (pcombatState->GetPermission(1u) && pattacker->GetCurrentProcessType() == PROCESS_TYPE_HIGH))) {
        pcombatState->BuildCoveredMoveRequest(&coveredMoveRequest);
        coveredMoveRequest.iLocationIndex = this->iLocation;
        prequest = (PathingRequest*)&coveredMoveRequest;
    }
    else {
        prequest = &request;
    }
    bool bhardRadius = false;
    if (fradius < 0.0) {
        fradius = -fradius;
        bhardRadius = true;
    }
    PathingAvoidNodeArray* a3b = arspAvoidNodeArray.m_pObject;
    TESForm* v10 = CurrentDestination.pSpace;
    NiPoint3* v8 = &(&CurrentDestination)->Point;
    pattacker->BuildRequest_0(prequest, *v8, v10, fradius, a3b);
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
    const NiPoint3* v2 = &pattacker->GetLocationOnReference();
    PathingAvoidNodeArray* AvoidNodeArray = pCombatManager->pThreatMap->CreateAvoidNodeArray(*v2, pattacker, -1.0f);
    arspAvoidNodeArray = AvoidNodeArray;
    float a2 = CombatTimer::GetTimer();
    pcombatState->SetThreatsConsideredTimeStamp(a2);
    Actor* pignoreActor = nullptr;
    int32_t iLocation = this->iLocation;
    if (iLocation >= 0) {
        if (iLocation <= 1) {
            pignoreActor = GetTarget();
        }
        else {
            if (iLocation == 4) {
                PathingAvoidNodeArray* FlankingAvoidNodeArray =
                    pCombatController->pCombatGroup->CreateFlankingAvoidNodeArray(
                        pattacker, (&Destination)->Point, 0.0f, arspAvoidNodeArray.m_pObject
                    );
                arspAvoidNodeArray = FlankingAvoidNodeArray;
            }
        }
    }
    if (kCombatLocationTypeData[this->iLocation].bAvoidTargets) {
        PathingAvoidNodeArray* MovementAvoidNodeArray = pCombatController->pCombatGroup->CreateMovementAvoidNodeArray(
            pattacker, (&Destination)->Point, 0.0f, arspAvoidNodeArray.m_pObject, pignoreActor
        );
        arspAvoidNodeArray = MovementAvoidNodeArray;
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
    bool Path = CreatePath(aspArray);
    return Path;
}

void CombatState::ReserveSecondaryCoverLocation() {
    ReserveCoverLocation(pSecondaryCoverLocation);
}

void CombatState::SetThreatsConsideredTimeStamp(float afValue) {
    CombatTimeStamp TimeStamp;
    TimeStamp.~CombatTimeStamp();
    ::new (&TimeStamp) CombatTimeStamp(afValue);
    fThreatsConsideredTimeStamp = TimeStamp;
}

void CombatState::SetSecondaryCoverSearchInfo(CombatCoverSearchInfo* apCoverSearchInfo) {
    SecondaryCoverSearchInfo.Copy(apCoverSearchInfo);
}
