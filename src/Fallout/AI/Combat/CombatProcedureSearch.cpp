#include "Fallout/AI/Combat/CombatProcedureSearch.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMenu/IDebugText.h"
#include "Fallout Shared/BGSWorldLocation.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingLOSGridMap.h"
#include "Fallout Shared/Pathfinding/PathingRequestLOS.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/CombatAction.h"
#include "Fallout/AI/Combat/CombatGoal.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatGroupStrategy.h"
#include "Fallout/AI/Combat/CombatPlan.h"
#include "Fallout/AI/Combat/CombatPlanner.h"
#include "Fallout/AI/Combat/CombatProcedure.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Fallout/AI/Combat/CombatWorldState.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "SDK/MSVC/rtti.h"

CombatProcedureSearch::CombatProcedureSearch() : eState(SEARCH_STATE_INITIALIZING), pSearchDoor(nullptr) {}

CombatProcedureSearch::~CombatProcedureSearch() {}

void CombatProcedureSearch::Update() {
    NiPoint3 kDestination;
    BGSWorldLocation kDoorLocation;
    BGSWorldLocation currentWorldLoc;
    using ignoreLocations_t = BSSimpleArray<NiPoint3, 1024>;
    BGSWorldLocation attackerWorldLoc;
    BGSWorldLocation searchCenter;
    Actor* pattacker = GetAttacker();
    PROCESS_TYPE eprocessLevel = pattacker->GetCurrentProcessType();
    CombatGroup* pcombatGroup = pCombatController->pCombatGroup;
    PathingLOSGridMap* psearchLOSGridMap = pcombatGroup->GetSearchLOSGridMap();
    CombatState* pcombatState = pCombatController->pCombatState;
    switch (eState) {
        case CombatProcedureSearch::SEARCH_STATE_INITIALIZING: {
            if (eprocessLevel == PROCESS_TYPE_HIGH && pCombatController->eLowState == COMBAT_LOW_STATE_SEARCHING) {
                eState = CombatProcedureSearch::SEARCH_STATE_LOOKING_AROUND_NEW_SPACE;
                LookingTimer.Reset(2.0f);
                return;
            }
            pSearchDoor = nullptr;
            if (!pcombatGroup->IsSearching()) {
                return;
            }
            CombatSearchLocation* psearchLocation = pcombatGroup->FindSearchLocation(pattacker, 0, nullptr);
            if (psearchLocation && psearchLocation->GetScore() > 30.0) {
                PathingRequest request;
                pattacker->BuildRequest_0(
                    &request,
                    psearchLocation->WorldLoc.Point,
                    psearchLocation->WorldLoc.pSpace,
                    128.0f,
                    nullptr
                );
                request.SetSoftGoalRadius(false);
                if (!pattacker->SetPathfindingGoal(&request)) {
                    SetFailed("Cannot setup path to investigate point");
                    return;
                }
                eState = CombatProcedureSearch::SEARCH_STATE_INVESTIGATING;
                CheckDestinationTimer.Reset(fCombatSearchCheckInvestigateInitialTime.Float());
                InvestigateLocation = psearchLocation->WorldLoc;
            }
            else {
                float fdoorDistance = eprocessLevel ? fCombatSearchCheckDoorLowDistance.Float()
                                                    : fCombatSearchCheckDoorDistance.Float();
                TESObjectREFR* pdoor = pcombatGroup->FindSearchDoor(pattacker, fdoorDistance, true);
                if (pdoor) {
                    PathingRequest request;
                    TESForm* pdoorSpace = pdoor->GetSpace();
                    const NiPoint3* pteleportPosition = &pdoor->GetLinkedDoorTeleportPosition();
                    pattacker->BuildRequest_0(&request, *pteleportPosition, pdoorSpace, 0.0f, nullptr);
                    NiPoint3 teleportRotation = pdoor->GetLinkedDoorTeleportRotation();
                    float freversedAngle = teleportRotation.z - 3.1415927410125732;
                    float ffaceAngle = ClampAngle(freversedAngle);
                    request.SetGoalAngleToFace(ffaceAngle);
                    if (!pattacker->SetPathfindingGoal(&request)) {
                        SetFailed("Cannot setup path to search door");
                        return;
                    }
                    eState = CombatProcedureSearch::SEARCH_STATE_SEARCHING_DOOR;
                    pSearchDoor = pdoor;
                    pcombatGroup->ReserveSearchDoor(pdoor);
                }
                else {
                    ::new (&searchCenter) BGSWorldLocation(pcombatGroup->GetSearchCenter());
                    if (!psearchLOSGridMap || searchCenter.pSpace != pattacker->GetSpace()) {
                        pattacker->GetWorldLocation(&attackerWorldLoc);
                        if (attackerWorldLoc.GetDistanceSquared(searchCenter) > 16384.0) {
                            PathingRequest request;
                            if (pcombatState->CheckMovement(searchCenter, true, false)) {
                                pattacker->BuildRequest_0(
                                    &request,
                                    searchCenter.Point,
                                    searchCenter.pSpace,
                                    128.0f,
                                    nullptr
                                );
                                if (!pattacker->SetPathfindingGoal(&request)) {
                                    SetFailed("Cannot setup path to search center");
                                    return;
                                }
                                InvestigateLocation = searchCenter;
                                eState = CombatProcedureSearch::SEARCH_STATE_MOVING_TO_SEARCH_CENTER;
                            }
                        }
                        else {
                            eState = CombatProcedureSearch::SEARCH_STATE_LOOKING;
                            LookingTimer.Reset(1.0f);
                        }
                    }
                    else {
                        Actor* psearchingMember = pcombatGroup->GetSearchingMember();
                        if (psearchingMember && psearchingMember != pattacker) {
                            return;
                        }
                        PathingRequestLOS losRequest;
                        losRequest.Initialize(pattacker);
                        if (pattacker->IsCreature()) {
                            TESActorBaseData* pactorData = static_cast<TESActorBaseData*>(pattacker->GetActorBase());
                            losRequest.SetCantOpenDoors(pactorData->GetCantOpenDoors());
                        }
                        losRequest.SetNoDestination();
                        losRequest.SetLOSGridMap(NiPointer<PathingLOSGridMap>(psearchLOSGridMap));
                        CombatLocationTarget* plocationTarget = pcombatState->pLocationTarget;
                        if (plocationTarget &&
                            (plocationTarget->cType == 1 ||
                             plocationTarget->cType == 2 ||
                             plocationTarget->cType == 3)) {
                            float ftargetDistance = pcombatState->GetLocationTargetDistance();
                            losRequest.SetSearchCenter(plocationTarget->WorldLoc.Point, ftargetDistance);
                        }

                        if (psearchLocation) {
                            TESForm* plocationSpace = psearchLocation->WorldLoc.pSpace;
                            if (plocationSpace == pattacker->GetSpace()) {
                                PathingLocation destination(psearchLocation->WorldLoc.Point, pattacker);
                                losRequest.SetDestination(destination);
                            }
                        }
                        losRequest.SetIgnoreLocationRadius(512.0f);
                        losRequest.SetAllowIncompletePath(false);
                        ignoreLocations_t ignoreLocations;
                        if (pcombatGroup->CalculateSearchIgnoreLocations(pattacker, &ignoreLocations)) {
                            losRequest.SetIgnoreLocations(ignoreLocations);
                        }
                        if (!pattacker->SetPathfindingHide(&losRequest)) {
                            SetFailed("Cannot setup LOS search path");
                            return;
                        }
                        pcombatGroup->SetSearchingMember(pattacker);
                        CheckDestinationTimer.Reset(fCombatSearchCheckDestinationTime.Float());
                        eState = CombatProcedureSearch::SEARCH_STATE_SEARCHING;
                        CheckSearchDoorTimer.Reset(fCombatSearchCheckDoorMovingTime.Float());
                    }
                }
            }
            SetMoveMode(1);
            return;
        }
        case CombatProcedureSearch::SEARCH_STATE_INVESTIGATING:
        case CombatProcedureSearch::SEARCH_STATE_MOVING_TO_SEARCH_CENTER: {
            if (pattacker->DidPathingFail()) {
                if (!pattacker->IsPathValid()) {
                    pcombatState->AddUnreachableLocation(InvestigateLocation);
                }
                if (pattacker->IsPathValid()) {
                    SetFailed("Pathing failed while moving to destination");
                }
                else {
                    SetFailed("Path Building failed to destination");
                }
            }
            else {
                bool bforceComplete = false;
                if (eState == CombatProcedureSearch::SEARCH_STATE_INVESTIGATING && CheckDestinationTimer.Check()) {
                    pattacker->GetWorldLocation(&currentWorldLoc);
                    double dDistanceSquared = currentWorldLoc.GetDistanceSquared(InvestigateLocation);
                    if (fCombatSearchCheckInvestigateMaxDistance.Float() *
                            fCombatSearchCheckInvestigateMaxDistance.Float() >
                        dDistanceSquared) {
                        PathingLocation investigatePathingLocation(&InvestigateLocation);
                        if (Pathing::CheckLineOfSight(
                                investigatePathingLocation,
                                currentWorldLoc.Point,
                                nullptr,
                                nullptr
                            )) {
                            bforceComplete = true;
                        }
                    }
                    CheckDestinationTimer.Reset(fCombatSearchCheckInvestigateTime.Float());
                }
                if (bforceComplete || pattacker->IsPathingComplete()) {
                    if (bforceComplete) {
                        pattacker->StopMoving();
                    }
                    pcombatGroup->InvestigatedSearchLocation(InvestigateLocation);
                    eState = CombatProcedureSearch::SEARCH_STATE_LOOKING;
                    LookingTimer.Reset(1.0f);
                }
            }
            return;
        }
        case CombatProcedureSearch::SEARCH_STATE_SEARCHING_DOOR: {
            if (pattacker->DidPathingFail()) {
                if (!pattacker->IsPathValid() && pSearchDoor) {
                    BGSWorldLocation* pdoorLocation = pSearchDoor->GetWorldLocation(&kDoorLocation);
                    pcombatState->AddUnreachableLocation(*pdoorLocation);
                    pcombatGroup->FailedToInvestigateSearchDoor(pSearchDoor);
                }

                if (pattacker->IsPathValid()) {
                    SetFailed("Pathing failed while moving to search door");
                }
                else {
                    SetFailed("Path Building failed to search door");
                }
            }
            else {
                if (pattacker->IsPathingComplete()) {
                    if (pSearchDoor) {
                        pcombatGroup->InvestigatedSearchDoor(pSearchDoor);
                        if (eprocessLevel) {
                            PathingRequest request;
                            DoorTeleportData* pteleportData = pSearchDoor->GetTeleport();
                            if (!pteleportData) {
                                return;
                            }
                            TESForm* pteleportSpace = pteleportData->GetTeleportSpace();
                            pattacker->BuildRequest_0(&request, pteleportData->position, pteleportSpace, 0.0f, nullptr);
                            if (!pattacker->SetPathfindingGoal(&request)) {
                                SetFailed("Cannot setup path through search door");
                                return;
                            }
                            eState = CombatProcedureSearch::SEARCH_STATE_SEARCHING_DOOR;
                        }
                        else {
                            pattacker->FadeOut();
                            eState = CombatProcedureSearch::SEARCH_STATE_LOOKING_DOOR_FADE;
                            LookingTimer.Reset(3.0f);
                        }
                        pSearchDoor = nullptr;
                    }
                    else {
                        eState = CombatProcedureSearch::SEARCH_STATE_LOOKING;
                        LookingTimer.Reset(1.0f);
                    }
                }
            }
            return;
        }
        case CombatProcedureSearch::SEARCH_STATE_SEARCHING: {
            if (pattacker->DidPathingFail()) {
                if (!pattacker->IsPathValid()) {
                    PathingLocation* pfailedPathDestination = pattacker->GetFailedPathDestination();
                    if (pfailedPathDestination && psearchLOSGridMap) {
                        pfailedPathDestination->ResolveNavMeshInfo(nullptr);
                        psearchLOSGridMap->SetLocationSeen(*pfailedPathDestination, true);
                        pcombatGroup->bUpdateSearchDebugGeometry = true;
                    }
                }
                if (pattacker->IsPathValid()) {
                    SetFailed("Pathing failed while searching");
                }
                else {
                    SetFailed("Path Building failed to search");
                }
            }
            else {
                if (CheckSearchDoorTimer.Check() && eprocessLevel == PROCESS_TYPE_HIGH) {
                    CheckSearchDoorTimer.Reset(fCombatSearchCheckDoorMovingTime.Float());
                    TESObjectREFR* pdoor = pcombatGroup->FindSearchDoor(
                        pattacker,
                        fCombatSearchCheckDoorMovingDistance.Float(),
                        true
                    );
                    if (pdoor) {
                        PathingRequest request;
                        TESForm* pdoorSpace = pdoor->GetSpace();
                        const NiPoint3* pteleportPosition = &pdoor->GetLinkedDoorTeleportPosition();
                        pattacker->BuildRequest_0(&request, *pteleportPosition, pdoorSpace, 0.0f, nullptr);
                        if (pattacker->SetPathfindingGoal(&request)) {
                            eState = CombatProcedureSearch::SEARCH_STATE_SEARCHING_DOOR;
                            pSearchDoor = pdoor;
                            pcombatGroup->ReserveSearchDoor(pdoor);
                            return;
                        }
                    }
                }
                bool bforceComplete = false;
                if (psearchLOSGridMap && CheckDestinationTimer.Check()) {
                    PathingLocation searchDestination;
                    if (pattacker->GetCurrentPathfindingGoal(&searchDestination)) {
                        NiPoint3* pdestination = ::new (&kDestination) NiPoint3(searchDestination.GetLocation());
                        const NiPoint3* pattackerLocation = &pattacker->GetLocationOnReference();
                        NiPoint3 vec = *pattackerLocation - *pdestination;
                        float fsquaredDistance = vec.SqrLength();
                        if (fsquaredDistance < fCombatSearchCheckDestinationMaxDistance.Float() *
                                fCombatSearchCheckDestinationMaxDistance.Float()) {
                            searchDestination.ResolveNavMeshInfo(nullptr);
                            if (searchDestination.HasNavMeshData() &&
                                psearchLOSGridMap->IsLocationSeen(searchDestination)) {
                                bforceComplete = true;
                                NavMeshPtr spmesh;
                                if (searchDestination.GetNavMesh(spmesh)) {
                                    uint16_t usTriangle = searchDestination.GetTriangle();
                                    char cneighbors = spmesh->GetNeighborCount(usTriangle);
                                    if (cneighbors == 1) {
                                        float* pminDistance1 = &fCombatSearchCheckDestinationMinDistance.Float();
                                        float* pminDistance2 = &fCombatSearchCheckDestinationMinDistance.Float();
                                        if (fsquaredDistance > *pminDistance1 * *pminDistance2) {
                                            bforceComplete = false;
                                        }
                                    }
                                    else if (cneighbors == 2) {
                                        float* pmidDistance1 = &fCombatSearchCheckDestinationMidDistance.Float();
                                        float* pmidDistance2 = &fCombatSearchCheckDestinationMidDistance.Float();
                                        if (fsquaredDistance > *pmidDistance1 * *pmidDistance2) {
                                            bforceComplete = false;
                                        }
                                    }
                                }
                                if (bforceComplete) {
                                    pattacker->StopMoving();
                                }
                            }
                        }
                    }
                    CheckDestinationTimer.Reset(fCombatSearchCheckDestinationTime.Float());
                }

                if (bforceComplete || pattacker->IsPathingComplete()) {
                    eState = CombatProcedureSearch::SEARCH_STATE_LOOKING;
                    LookingTimer.Reset(1.0f);
                }
            }
            return;
        }
        case CombatProcedureSearch::SEARCH_STATE_LOOKING_DOOR_FADE: {
            if (pattacker->GetFadePercent() <= 0.0) {
                if (pSearchDoor) {
                    const NiPoint3* pteleportRotation = &pSearchDoor->GetLinkedDoorTeleportRotation();
                    pattacker->SetAngleOnReferenceZ(ClampAngle(pteleportRotation->z));
                }
                eState = CombatProcedureSearch::SEARCH_STATE_LOOKING_DOOR;
            }
            return;
        }
        case CombatProcedureSearch::SEARCH_STATE_LOOKING_DOOR: {
            if (LookingTimer.Check()) {
                if (pattacker->GetFadePercent() < 1.0) {
                    pattacker->FadeIn();
                }
                eState = CombatProcedureSearch::SEARCH_STATE_INITIALIZING;
            }
            return;
        }
        case CombatProcedureSearch::SEARCH_STATE_LOOKING: {
            if (LookingTimer.Check()) {
                eState = CombatProcedureSearch::SEARCH_STATE_INITIALIZING;
            }
            return;
        }
        case CombatProcedureSearch::SEARCH_STATE_LOOKING_AROUND_NEW_SPACE: {
            BGSWorldLocation kSearchCenter;
            double dGroupStartTime = pcombatGroup->SearchAreaUpdateTimer.fStartTime;
            if (LookingTimer.fStartTime > dGroupStartTime) {
                ::new (&kSearchCenter) BGSWorldLocation(pcombatGroup->GetSearchCenter());
                TESForm* pcenterSpace = kSearchCenter.pSpace;
                if (pcenterSpace != pattacker->GetSpace()) {
                    return;
                }
            }
            if (LookingTimer.Check()) {
                eState = CombatProcedureSearch::SEARCH_STATE_INITIALIZING;
            }
            return;
        }
        default: {
            return;
        }
    }
}

// Gives up the group's searching slot when this actor holds it, stops the actor, fades it back in when it hid in
// the last two states, and releases the door it reserved while searching a door.
void CombatProcedureSearch::Shutdown() {
    Actor* pattacker = GetAttacker();
    CombatGroup* pgroup = pCombatController->GetCombatGroup();
    if (pgroup && pgroup->GetSearchingMember() == pattacker) {
        pgroup->SetSearchingMember(nullptr);
    }
    pattacker->StopMoving();
    ClearMoveMode();
    if (eState == 5 || eState == 4) {
        pattacker->FadeIn();
    }
    if (eState == 2 && pgroup && pSearchDoor) {
        pgroup->UnreserveSearchDoor(pSearchDoor);
    }
}

// Each timer's values are passed as doubles: fTargetTime widened from its float, and ElapsedTime's result as a double.
void CombatProcedureSearch::DisplayDebugText(
    IDebugText* apDebugText,
    DEBUGTEXT_LOCATION aeLocation,
    DEBUGTEXT_JUSTIFY aeJustify
) {
    Actor* pattacker = GetAttacker();
    CombatGroup* pgroup = pCombatController->pCombatGroup;
    switch (eState) {
        case CombatProcedureSearch::SEARCH_STATE_INITIALIZING: {
            apDebugText->PrintLine(aeLocation, aeJustify, "INITIALIZING");
            break;
        }
        case CombatProcedureSearch::SEARCH_STATE_INVESTIGATING: {
            apDebugText->PrintLine(aeLocation, aeJustify, "INVESTIGATING");
            apDebugText->PrintLine(
                aeLocation,
                aeJustify,
                "Point: (%.02f,%.02f,%.02f)",
                InvestigateLocation.Point.x,
                InvestigateLocation.Point.y,
                InvestigateLocation.Point.z
            );
            apDebugText->PrintLine(aeLocation, aeJustify, "Space: %s", InvestigateLocation.pSpace->GetFormEditorID());
            break;
        }
        case CombatProcedureSearch::SEARCH_STATE_SEARCHING_DOOR: {
            apDebugText->PrintLine(aeLocation, aeJustify, "SEARCHING DOOR");
            if (pSearchDoor) {
                double dZ = pSearchDoor->GetLocationOnReference().z;
                double dY = pSearchDoor->GetLocationOnReference().y;
                double dX = pSearchDoor->GetLocationOnReference().x;
                const char* pEditorID = pSearchDoor->GetFormEditorID();
                apDebugText->PrintLine(aeLocation, aeJustify, "Door: %s (%.02f,%.02f,%.02f)", pEditorID, dX, dY, dZ);
            }
            break;
        }
        case CombatProcedureSearch::SEARCH_STATE_SEARCHING: {
            apDebugText->PrintLine(aeLocation, aeJustify, "SEARCHING");
            break;
        }
        case CombatProcedureSearch::SEARCH_STATE_LOOKING_DOOR_FADE: {
            apDebugText->PrintLine(aeLocation, aeJustify, "LOOKING DOOR FADE");
            double dTargetTime = LookingTimer.fTargetTime;
            double dElapsedTime = LookingTimer.ElapsedTime();
            apDebugText->PrintLine(aeLocation, aeJustify, "Looking Timer: %.02f/%.02f", dElapsedTime, dTargetTime);
            break;
        }
        case CombatProcedureSearch::SEARCH_STATE_LOOKING_DOOR: {
            apDebugText->PrintLine(aeLocation, aeJustify, "LOOKING DOOR");
            double dTargetTime = LookingTimer.fTargetTime;
            double dElapsedTime = LookingTimer.ElapsedTime();
            apDebugText->PrintLine(aeLocation, aeJustify, "Looking Timer: %.02f/%.02f", dElapsedTime, dTargetTime);
            break;
        }
        case CombatProcedureSearch::SEARCH_STATE_LOOKING: {
            apDebugText->PrintLine(aeLocation, aeJustify, "LOOKING");
            double dTargetTime = LookingTimer.fTargetTime;
            double dElapsedTime = LookingTimer.ElapsedTime();
            apDebugText->PrintLine(aeLocation, aeJustify, "Looking Timer: %.02f/%.02f", dElapsedTime, dTargetTime);
            break;
        }
        case CombatProcedureSearch::SEARCH_STATE_LOOKING_AROUND_NEW_SPACE: {
            apDebugText->PrintLine(aeLocation, aeJustify, "SEARCH_STATE_LOOKING_AROUND_NEW_SPACE");
            double dTargetTime = LookingTimer.fTargetTime;
            double dElapsedTime = LookingTimer.ElapsedTime();
            apDebugText->PrintLine(aeLocation, aeJustify, "Looking Timer: %.02f/%.02f", dElapsedTime, dTargetTime);
            // The group timer's start is kept as a double and the looking timer's start is compared against it; an
            // unordered compare skips the print.
            double dGroupStartTime = pgroup->SearchAreaUpdateTimer.fStartTime;
            if (LookingTimer.fStartTime > dGroupStartTime) {
                BGSWorldLocation searchCenter = pgroup->GetSearchCenter();
                if (searchCenter.pSpace != pattacker->GetSpace()) {
                    double dUpdateTarget = pgroup->SearchAreaUpdateTimer.fTargetTime;
                    double dUpdateElapsed = pgroup->SearchAreaUpdateTimer.ElapsedTime();
                    apDebugText->PrintLine(
                        aeLocation,
                        aeJustify,
                        "Search Area Update Timer: %.02f/%.02f",
                        dUpdateElapsed,
                        dUpdateTarget
                    );
                }
            }
            break;
        }
        case CombatProcedureSearch::SEARCH_STATE_MOVING_TO_SEARCH_CENTER: {
            apDebugText->PrintLine(aeLocation, aeJustify, "SEARCH_STATE_MOVING_TO_SEARCH_CENTER");
            apDebugText->PrintLine(
                aeLocation,
                aeJustify,
                "Point: (%.02f,%.02f,%.02f)",
                InvestigateLocation.Point.x,
                InvestigateLocation.Point.y,
                InvestigateLocation.Point.z
            );
            apDebugText->PrintLine(aeLocation, aeJustify, "Space: %s", InvestigateLocation.pSpace->GetFormEditorID());
            break;
        }
        default: {
            break;
        }
    }
    // The percentage is rounded to float before it is used.
    float fsearchPercent = pgroup->GetSearchPercent();
    if (fsearchPercent >= 0.0) {
        CombatTimeStamp searchStarted = pgroup->GetSearchStartedTimeStamp();
        double dStartedElapsed = searchStarted.DisplayElapsedTime();
        double dPercent = fsearchPercent * 100.0;
        double dRadius = pgroup->fSearchRadius;
        apDebugText->PrintLine(
            aeLocation,
            aeJustify,
            "Search: %i %.2f %.1f%% %.2fs",
            pgroup->iSearchCount,
            dRadius,
            dPercent,
            dStartedElapsed
        );
    }
}

void CombatProcedureSearch::ActivatedReference(
    TESObjectREFR* apRef,
    TESBoundObject* apObject,
    int32_t aiCount,
    bool abActivatedScriptOnly
) {
    if (pSearchDoor == apRef) {
        pSearchDoor = nullptr;
    }
}

CombatProcedure::COMBAT_PROCEDURES CombatProcedureSearch::GetType() {
    return SEARCH;
}

// The investigate location goes before the timers, and the timers in the order looking, check destination,
// check search door (not their member order).
void CombatProcedureSearch::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    CombatProcedure::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eState), sizeof(eState), 0);
    // The same buffer pointer is passed to every member; the cast is there because BGSWorldLocation::SaveGame is typed
    // on the form buffer.
    InvestigateLocation.SaveGame(apSaveGameBuffer);
    LookingTimer.SaveGame(apSaveGameBuffer);
    CheckDestinationTimer.SaveGame(apSaveGameBuffer);
    CheckSearchDoorTimer.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveFormID(pSearchDoor, 0);
}

// Reads what SaveGame writes; the search door's form ID goes into pSearchDoor, resolved later.
void CombatProcedureSearch::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&eState), sizeof(eState));
    InvestigateLocation.LoadGame(apLoadGameBuffer);
    LookingTimer.LoadGame(apLoadGameBuffer);
    CheckDestinationTimer.LoadGame(apLoadGameBuffer);
    CheckSearchDoorTimer.LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadFormID(pSearchDoor);
}

void CombatProcedureSearch::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESObjectREFR* pdoor = nullptr;
    if (pSearchDoor) {
        TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSearchDoor)));
        pdoor = dynamic_cast<TESObjectREFR*>(pForm);
    }
    pSearchDoor = pdoor;
}

void PathingRequestLOS::SetLOSGridMap(const NiPointer<PathingLOSGridMap>& aspLOSGridMap) {
    spLOSGridMap = aspLOSGridMap;
}

void PathingRequestLOS::SetIgnoreLocations(const BSSimpleArray<NiPoint3>& aLocations) {
    aLocations.CopyTo(&IgnoreLocations);
}

BGSWorldLocation CombatGroup::GetSearchCenter() {
    return SearchCenter;
}

float CombatGroup::GetSearchPercent() {
    float fPercent;
    if (spPathingLOSGridMap) {
        fPercent = spPathingLOSGridMap->GetSeenPercent();
    }
    else {
        fPercent = -1.0f;
    }
    return fPercent;
}

CombatTimeStamp CombatGroup::GetSearchStartedTimeStamp() {
    return fSearchStartedTimeStamp;
}
