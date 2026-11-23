#include "Fallout/AI/Movement/VirtualActorPathHandler.h"

#include "BSCore/MemoryManager.h"
#include "BSMenu/IDebugText.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Pathfinding/VirtualPathingNode.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/Movement/ActorPathHandler.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

// iCurrentNodeIndex is deliberately left unset by the constructor.
VirtualActorPathHandler::VirtualActorPathHandler(
    Actor* apActor,
    ActorMover* apActorMover,
    NiPointer<PathingSolution>& aspPath,
    PathingRequest* apRequest
)
  : ActorPathHandler(apActor, apActorMover, aspPath, apRequest), fDistanceTraveled(0.0f), fPathLength(0.0f) {}

VirtualActorPathHandler::~VirtualActorPathHandler() {
    VirtualActorPathHandler::ClearMovement();
}

ActorPathHandler::PATH_HANDLER_RESULT VirtualActorPathHandler::Init(float afPercentTraveled, float afDeltaT) {
    iCurrentNodeIndex = static_cast<int64_t>(afPercentTraveled);
    fDistanceTraveled = 0.0;
    fPathLength = spPath->ComputeVirtualPathLength();
    if (afPercentTraveled == 0.0) {
        return Update(afDeltaT);
    }

    return ActorPathHandler::PATH_HANDLER_RESULT_OK;
}

ActorPathHandler::PATH_HANDLER_RESULT VirtualActorPathHandler::Update(float afDeltaT) {
    float fDelta = afDeltaT;
    BaseProcess* pProcess = pActor->pCurrentProcess;
    if (pProcess && pProcess->GetSitSleepState()) {
        return ActorPathHandler::PATH_HANDLER_RESULT_OK;
    }
    if (pPlayer->IsSleepingorResting() || pActor->IsFollowAccompanyPackageTarget(pPlayer)) {
        fDelta = 3600.0 / appCalendar.GetTimeScale();
    }
    else if (!pActor->IsInCombat()) {
        fDelta = 900.0 / appCalendar.GetTimeScale();
        BaseProcess* pActorProcess = pActor->pCurrentProcess;
        if (pActorProcess) {
            double dSeconds = (fGameHour - pActorProcess->fHourLastProcessed) * 3600.0;
            fDelta = dSeconds / appCalendar.GetTimeScale();
        }
    }
    if (iCurrentNodeIndex >= spPath->GetVirtualNodeCount()) {
        return ActorPathHandler::PATH_HANDLER_RESULT_DONE;
    }
    NiPoint3 currentLoc = pActor->GetLocationOnReference();
    NiPoint3 deltaToNode = NiPoint3::ZERO;
    float fDistCanTravel = pActor->GetCurrentSpeed() * fDelta;
    float fDistToNode = 0.0f;
    while (fDistCanTravel >= 0.0) {
        VirtualPathingNode* pNode = spPath->GetVirtualNode(iCurrentNodeIndex);
        NiPoint3 nextLoc = pNode->Location.GetLocation();
        deltaToNode = nextLoc - currentLoc;
        fDistToNode = deltaToNode.XYLength();
        if (!(fDistToNode <= fDistCanTravel)) {
            break;
        }
        fDistanceTraveled = fDistanceTraveled + fDistToNode;
        fDistCanTravel = fDistCanTravel - fDistToNode;
        currentLoc = nextLoc;
        float fAngle = GetZAngleFromVector(deltaToNode);
        pActor->SetHeading(fAngle);
        if (spPath->GetVirtualNode(iCurrentNodeIndex)->IsTransitionNode()) {
            int32_t iDoorResult = UseDoorAtNode(currentLoc);
            if (iDoorResult >= -2) {
                if (iDoorResult < 0) {
                    return ActorPathHandler::PATH_HANDLER_RESULT_CANT_USE_DOOR;
                }
                if (iDoorResult == 1) {
                    TESWorldSpace* pWorldSpace = pActor->GetWorldSpace();
                    TESObjectCELL* pParentCell = pActor->GetParentCell();
                    pActor->MoveActorAndFollowers(currentLoc, fAngle, pParentCell, pWorldSpace, true, false);
                    return ActorPathHandler::PATH_HANDLER_RESULT_OK;
                }
            }
        }
        else {
            TESWorldSpace* pWorldSpace = pActor->GetWorldSpace();
            TESObjectCELL* pParentCell = pActor->GetParentCell();
            pActor->MoveActorAndFollowers(currentLoc, fAngle, pParentCell, pWorldSpace, true, false);
            if (pActor->GetDesiredProcessLevel() == PROCESS_TYPE_HIGH && !pPlayer->IsSleepingorResting()) {
                return ActorPathHandler::PATH_HANDLER_RESULT_OK;
            }
            ++iCurrentNodeIndex;
        }
        if (iCurrentNodeIndex >= spPath->GetVirtualNodeCount()) {
            return ActorPathHandler::PATH_HANDLER_RESULT_DONE;
        }
    }
    double dFraction = fDistToNode == 0.0 ? 0.0 : 1.0 - (fDistToNode - fDistCanTravel) / fDistToNode;
    float fScale = dFraction;
    deltaToNode *= fScale;
    currentLoc += deltaToNode;
    fDistanceTraveled = deltaToNode.XYLength() + fDistanceTraveled;
    float fAngle = GetZAngleFromVector(deltaToNode);
    pActor->SetHeading(fAngle);
    TESWorldSpace* pWorldSpace = pActor->GetWorldSpace();
    TESObjectCELL* pParentCell = pActor->GetParentCell();
    pActor->MoveActorAndFollowers(currentLoc, fAngle, pParentCell, pWorldSpace, true, false);
    if (pActor->GetDesiredProcessLevel() == PROCESS_TYPE_HIGH && !pPlayer->IsSleepingorResting()) {
        return ActorPathHandler::PATH_HANDLER_RESULT_OK;
    }
    if (!spPath.m_pObject) {
        return ActorPathHandler::PATH_HANDLER_RESULT_OK;
    }
    uint32_t uiNextIndex = iCurrentNodeIndex + 1;
    if (uiNextIndex != spPath->GetVirtualNodeCount()) {
        return ActorPathHandler::PATH_HANDLER_RESULT_OK;
    }
    double dTargetRadius = spRequest->fTargetRadius;
    if (!(fDistToNode <= dTargetRadius + fDistCanTravel)) {
        return ActorPathHandler::PATH_HANDLER_RESULT_OK;
    }
    ++iCurrentNodeIndex;
    return ActorPathHandler::PATH_HANDLER_RESULT_DONE;
}

ActorPathHandler::PATH_HANDLER_RESULT VirtualActorPathHandler::UpdateWhilePlayerWaits(float afDeltaT) {
    return Update(afDeltaT);
}

float VirtualActorPathHandler::GetPercentTraveled() {
    if (!spPath) {
        return 0.0;
    }
    if (iCurrentNodeIndex >= spPath->GetVirtualNodeCount() || fPathLength <= 0.0) {
        return 1.0;
    }
    return fDistanceTraveled / fPathLength;
}

NiPoint3 VirtualActorPathHandler::GetNextNodeLocation() {
    if (spPath->GetNodeCount()) {
        VirtualPathingNode* pNode;
        uint32_t uiNextIndex = iCurrentNodeIndex + 1;
        if (uiNextIndex >= spPath->GetVirtualNodeCount()) {
            pNode = spPath->GetLastVirtualNode();
        }
        else {
            pNode = spPath->GetVirtualNode(iCurrentNodeIndex + 1);
        }
        return pNode->Location.GetLocation();
    }

    return pActor->GetLocationOnReference();
}

bool VirtualActorPathHandler::GetNodeLocation(uint32_t aiIndex, NiPoint3& arLocation) {
    if (!spPath) {
        return false;
    }
    if (aiIndex >= spPath->GetVirtualNodeCount()) {
        return false;
    }
    VirtualPathingNode* pNode = spPath->GetVirtualNode(aiIndex);
    PathingLocation* pLocation = static_cast<PathingLocation*>(&pNode->GetLocation());
    arLocation = pLocation->GetLocation();
    return true;
}

// The path length not yet travelled, at least 0.
float VirtualActorPathHandler::GetDistanceRemaining() {
    float fRemaining = fPathLength - fDistanceTraveled;
    fRemaining = BSMAX(fRemaining, 0.0f);
    return fRemaining;
}

NiPoint3 VirtualActorPathHandler::ComputeApproximateLocation(float afDeltaT) {
    return pActor->GetLocationOnReference();
}

bool VirtualActorPathHandler::IsDetailedPathHandler() {
    return false;
}

uint32_t VirtualActorPathHandler::GetMoveMode() {
    return 0;
}

bool VirtualActorPathHandler::GetMoveDelta(NiPoint3& aDelta) {
    aDelta = NiPoint3::ZERO;
    return false;
}

void VirtualActorPathHandler::PrintCurrentStatus(
    IDebugText* apDebugText,
    DEBUGTEXT_LOCATION aLoc,
    DEBUGTEXT_JUSTIFY aJust
) {
    const NiPoint3& kLocation = pActor->GetLocationOnReference();
    apDebugText->PrintLine(
        aLoc,
        aJust,
        "VirtualPathHandler: Actor's Positon %1.f %1.f %1.f",
        kLocation.x,
        kLocation.y,
        kLocation.z
    );
    if (spPath.m_pObject) {
        apDebugText
            ->PrintLine(aLoc, aJust, "Current path node %u of %u", iCurrentNodeIndex, spPath->GetVirtualNodeCount());
    }
    else {
        apDebugText->PrintLine(aLoc, aJust, "No path");
    }
}

void VirtualActorPathHandler::ClearMovement() {
    spPath = nullptr;
    spRequest = nullptr;
    iCurrentNodeIndex = 0;
}

void VirtualActorPathHandler::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iCurrentNodeIndex), sizeof(iCurrentNodeIndex), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fDistanceTraveled), sizeof(fDistanceTraveled), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fPathLength), sizeof(fPathLength), 0);
}

void VirtualActorPathHandler::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iCurrentNodeIndex), sizeof(iCurrentNodeIndex));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fDistanceTraveled), sizeof(fDistanceTraveled));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fPathLength), sizeof(fPathLength));
}

int32_t VirtualActorPathHandler::UseDoorAtNode(NiPoint3& aNewLoc) {
    if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
        Actor* pActor = this->pActor;
        TaskQueueInterface* pTaskQueue = TaskQueueInterface::GetInstance();
        pTaskQueue->QueueActorSetPosition(pActor, aNewLoc);
        pActor = this->pActor;
        pTaskQueue = TaskQueueInterface::GetInstance();
        pTaskQueue->QueueActorMoveCharController(pActor);
    }
    else {
        this->pActor->SetLocationOnReference(aNewLoc);
    }
    TESObjectREFR* pDoor = spPath->GetVirtualNode(iCurrentNodeIndex)->pDoorRef;
    ++iCurrentNodeIndex;
    if (pDoor && !pDoor->Activate(this->pActor, false, nullptr, 1)) {
        return -1;
    }
    if (spPath.m_pObject && iCurrentNodeIndex < spPath->GetVirtualNodeCount()) {
        NiPoint3 movedLoc = this->pActor->GetLocationOnReference();
        VirtualPathingNode* pNode = spPath->GetVirtualNode(iCurrentNodeIndex);
        float fDiff = (movedLoc - pNode->Location.GetLocation()).SqrXYLength();
        if (fDiff > 10000.0) {
            if (aNewLoc != movedLoc) {
                const char* pActorName;
                const char* pCellName;
                if (pDoor->GetParentCell()) {
                    if (pDoor->GetParentCell()->GetFormEditorIDLength()) {
                        pCellName = pDoor->GetParentCell()->GetFullName();
                    }
                    else {
                        TESObjectCELL* pParentCell = pDoor->GetParentCell();
                        pCellName = pParentCell->GetFormEditorID();
                    }
                }
                else {
                    pCellName = "";
                }
                if (this->pActor->GetFormEditorIDLength()) {
                    pActorName = this->pActor->GetFormEditorID();
                }
                else {
                    pActorName = this->pActor->GetFullName();
                }
                Warning(
                    "PATHFINDING: Actor %s used teleport door %s in cell %s and was not moved to the expected "
                    "pathing location, check teleport marker",
                    pActorName,
                    pDoor->GetFormEditorID(),
                    pCellName
                );
            }
            return -2;
        }
    }
    aNewLoc = this->pActor->GetLocationOnReference();
    if (this->pActor->GetDesiredProcessLevel() || pPlayer->IsSleepingorResting()) {
        return 0;
    }

    ++iCurrentNodeIndex;
    fDistanceTraveled = 0.0f;
    return 1;
}
