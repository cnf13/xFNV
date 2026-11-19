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

// iCurrentNodeIndex is deliberately left unset by the constructor, as the game does.
VirtualActorPathHandler::VirtualActorPathHandler(
    Actor* a1,
    ActorMover* a2,
    NiPointer<PathingSolution>& a3,
    PathingRequest* a4
)
  : ActorPathHandler(a1, a2, a3, a4), fDistanceTraveled(0.0f), fPathLength(0.0f) {}

VirtualActorPathHandler::~VirtualActorPathHandler() {
    VirtualActorPathHandler::ClearMovement();
}

ActorPathHandler::PATH_HANDLER_RESULT VirtualActorPathHandler::Init(float afPercentTraveled, float afDeltaT) {
    iCurrentNodeIndex = static_cast<int64_t>(afPercentTraveled);
    fDistanceTraveled = 0.0;
    PathingSolution* v3 = spPath;
    fPathLength = v3->ComputeVirtualPathLength();
    if (afPercentTraveled == 0.0) {
        return Update(afDeltaT);
    }

    return ActorPathHandler::PATH_HANDLER_RESULT_OK;
}

ActorPathHandler::PATH_HANDLER_RESULT VirtualActorPathHandler::Update(float afDeltaT) {
    double fscale;
    float fdelta = afDeltaT;
    BaseProcess* pprocess = pActor->pCurrentProcess;
    if (pprocess && pprocess->GetSitSleepState()) {
        return ActorPathHandler::PATH_HANDLER_RESULT_OK;
    }
    if (pPlayer->IsSleepingorResting() || pActor->IsFollowAccompanyPackageTarget(pPlayer)) {
        fdelta = 3600.0 / appCalendar.GetTimeScale();
    }
    else {
        if (!pActor->IsInCombat()) {
            fdelta = 900.0 / appCalendar.GetTimeScale();
            BaseProcess* pactorProcess = pActor->pCurrentProcess;
            if (pactorProcess) {
                double fseconds = (fGameHour - pactorProcess->fHourLastProcessed) * 3600.0;
                fdelta = fseconds / appCalendar.GetTimeScale();
            }
        }
    }
    if (iCurrentNodeIndex >= spPath.m_pObject->GetVirtualNodeCount()) {
        return ActorPathHandler::PATH_HANDLER_RESULT_DONE;
    }
    NiPoint3 currentLoc = pActor->GetLocationOnReference();
    NiPoint3 deltaToNode = NiPoint3_ZERO;
    float fdistCanTravel = pActor->GetCurrentSpeed() * fdelta;
    float fdistToNode = 0.0f;
    while (fdistCanTravel >= 0.0) {
        VirtualPathingNode* pNode = spPath.m_pObject->GetVirtualNode(iCurrentNodeIndex);
        NiPoint3 nextLoc = pNode->Location.GetLocation();
        deltaToNode = nextLoc - currentLoc;
        fdistToNode = deltaToNode.XYLength();
        if (!(fdistToNode <= fdistCanTravel)) {
            break;
        }
        fDistanceTraveled = fDistanceTraveled + fdistToNode;
        fdistCanTravel = fdistCanTravel - fdistToNode;
        currentLoc = nextLoc;
        float fangle = GetZAngleFromVector(deltaToNode);
        pActor->SetHeading(fangle);
        if (spPath.m_pObject->GetVirtualNode(iCurrentNodeIndex)->IsTransitionNode()) {
            int32_t iDoorResult = UseDoorAtNode(currentLoc);
            if (iDoorResult >= -2) {
                if (iDoorResult < 0) {
                    return ActorPathHandler::PATH_HANDLER_RESULT_CANT_USE_DOOR;
                }
                if (iDoorResult == 1) {
                    TESWorldSpace* pWorldSpace = pActor->GetWorldSpace();
                    TESObjectCELL* pParentCell = pActor->GetParentCell();
                    pActor->MoveActorAndFollowers(currentLoc, fangle, pParentCell, pWorldSpace, true, false);
                    return ActorPathHandler::PATH_HANDLER_RESULT_OK;
                }
            }
        }
        else {
            TESWorldSpace* pWorldSpace = pActor->GetWorldSpace();
            TESObjectCELL* pParentCell = pActor->GetParentCell();
            pActor->MoveActorAndFollowers(currentLoc, fangle, pParentCell, pWorldSpace, true, false);
            if (pActor->GetDesiredProcessLevel() == PROCESS_TYPE_HIGH && !pPlayer->IsSleepingorResting()) {
                return ActorPathHandler::PATH_HANDLER_RESULT_OK;
            }
            ++iCurrentNodeIndex;
        }
        if (iCurrentNodeIndex >= spPath.m_pObject->GetVirtualNodeCount()) {
            return ActorPathHandler::PATH_HANDLER_RESULT_DONE;
        }
    }
    if (fdistToNode == 0.0) {
        fscale = 0.0;
    }
    else {
        fscale = 1.0 - (fdistToNode - fdistCanTravel) / fdistToNode;
    }
    float fscale_2 = fscale;
    deltaToNode *= fscale_2;
    currentLoc += deltaToNode;
    fDistanceTraveled = deltaToNode.XYLength() + fDistanceTraveled;
    float fangle_2 = GetZAngleFromVector(deltaToNode);
    pActor->SetHeading(fangle_2);
    TESWorldSpace* pWorldSpace = pActor->GetWorldSpace();
    TESObjectCELL* pParentCell = pActor->GetParentCell();
    pActor->MoveActorAndFollowers(currentLoc, fangle_2, pParentCell, pWorldSpace, true, false);
    if (pActor->GetDesiredProcessLevel() == PROCESS_TYPE_HIGH && !pPlayer->IsSleepingorResting()) {
        return ActorPathHandler::PATH_HANDLER_RESULT_OK;
    }
    if (!spPath.m_pObject) {
        return ActorPathHandler::PATH_HANDLER_RESULT_OK;
    }
    uint32_t uiNextIndex = iCurrentNodeIndex + 1;
    if (uiNextIndex != spPath.m_pObject->GetVirtualNodeCount()) {
        return ActorPathHandler::PATH_HANDLER_RESULT_OK;
    }
    double fTargetRadius = spRequest.m_pObject->fTargetRadius;
    if (!(fdistToNode <= fTargetRadius + fdistCanTravel)) {
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
    PathingSolution* v2 = spPath;
    if ((iCurrentNodeIndex >= v2->GetVirtualNodeCount()) || (fPathLength <= 0.0)) {
        return 1.0;
    }
    return (fDistanceTraveled / fPathLength);
}

NiPoint3 VirtualActorPathHandler::GetNextNodeLocation() {
    if (spPath.m_pObject->GetNodeCount()) {
        VirtualPathingNode* pNode;
        uint32_t uiNextIndex = iCurrentNodeIndex + 1;
        if (uiNextIndex >= spPath.m_pObject->GetVirtualNodeCount()) {
            pNode = spPath.m_pObject->GetLastVirtualNode();
        }
        else {
            pNode = spPath.m_pObject->GetVirtualNode(iCurrentNodeIndex + 1);
        }
        return pNode->Location.GetLocation();
    }

    return pActor->GetLocationOnReference();
}

bool VirtualActorPathHandler::GetNodeLocation(uint32_t aiIndex, NiPoint3& arLocation) {
    NiPoint3 result;
    if (!spPath) {
        return false;
    }
    PathingSolution* v3 = spPath;
    if (aiIndex >= v3->GetVirtualNodeCount()) {
        return false;
    }
    PathingSolution* v4 = spPath;
    VirtualPathingNode* VirtualNode = v4->GetVirtualNode(aiIndex);
    PathingLocation* ActorPackageThatIsRunning = static_cast<PathingLocation*>(&VirtualNode->GetLocation());
    arLocation = (result = ActorPackageThatIsRunning->GetLocation());
    return true;
}

// The path length not yet travelled, at least 0.
float VirtualActorPathHandler::GetDistanceRemaining() {
    float fRemaining = fPathLength - fDistanceTraveled;
    fRemaining = BSMAX(fRemaining, 0.0f);
    return fRemaining;
}

NiPoint3 VirtualActorPathHandler::ComputeApproximateLocation(float afDeltaT) {
    NiPoint3 a2;
    a2 = pActor->GetLocationOnReference();
    return a2;
}

bool VirtualActorPathHandler::IsDetailedPathHandler() {
    return false;
}

uint32_t VirtualActorPathHandler::GetMoveMode() {
    return 0;
}

bool VirtualActorPathHandler::GetMoveDelta(NiPoint3& aDelta) {
    aDelta = NiPoint3_ZERO;
    return false;
}

void VirtualActorPathHandler::PrintCurrentStatus(
    IDebugText* apDebugText,
    DEBUGTEXT_LOCATION aLoc,
    DEBUGTEXT_JUSTIFY aJust
) {
    const NiPoint3* v4 = &pActor->GetLocationOnReference();
    apDebugText->PrintLine(aLoc, aJust, "VirtualPathHandler: Actor's Positon %1.f %1.f %1.f", v4->x, v4->y, v4->z);
    if (spPath.m_pObject) {
        apDebugText->PrintLine(
            aLoc, aJust, "Current path node %u of %u", iCurrentNodeIndex, spPath.m_pObject->GetVirtualNodeCount()
        );
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
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iCurrentNodeIndex), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fDistanceTraveled), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fPathLength), 4, 0);
}

void VirtualActorPathHandler::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iCurrentNodeIndex), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fDistanceTraveled), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fPathLength), 4);
}

int32_t VirtualActorPathHandler::UseDoorAtNode(NiPoint3& aNewLoc) {
    if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
        Actor* pActor = this->pActor;
        TaskQueueInterface* Instance = TaskQueueInterface::GetInstance();
        Instance->QueueActorSetPosition(pActor, aNewLoc);
        Actor* v9 = this->pActor;
        TaskQueueInterface* v4 = TaskQueueInterface::GetInstance();
        v4->QueueActorMoveCharController(v9);
    }
    else {
        this->pActor->SetLocationOnReference(aNewLoc);
    }
    TESObjectREFR* pdoor = spPath.m_pObject->GetVirtualNode(iCurrentNodeIndex)->pDoorRef;
    ++iCurrentNodeIndex;
    if (pdoor && !pdoor->Activate(this->pActor, false, nullptr, 1)) {
        return -1;
    }
    if (spPath.m_pObject && iCurrentNodeIndex < spPath.m_pObject->GetVirtualNodeCount()) {
        NiPoint3 movedLoc = this->pActor->GetLocationOnReference();
        VirtualPathingNode* pNode = spPath.m_pObject->GetVirtualNode(iCurrentNodeIndex);
        float fdiff = (movedLoc - pNode->Location.GetLocation()).SqrXYLength();
        if (fdiff > 10000.0) {
            if (aNewLoc != movedLoc) {
                const char* pActorName;
                const char* pCellName;
                if (pdoor->GetParentCell()) {
                    const char* pName;
                    if (pdoor->GetParentCell()->GetFormEditorIDLength()) {
                        pName = pdoor->GetParentCell()->GetFullName();
                    }
                    else {
                        TESObjectCELL* pParentCell = pdoor->GetParentCell();
                        pName = pParentCell->GetFormEditorID();
                    }
                    pCellName = pName;
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
                    pActorName, pdoor->GetFormEditorID(), pCellName
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
