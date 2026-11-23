#include "Fallout/AI/Movement/ActorMover.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSTStaticFreeList.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSSystem/BSTMessageQueue.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Pathfinding/ActorPathingMessage.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshInfoMap.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/Pathfinding/PathingLOSGridMap.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingNode.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Pathfinding/PathingRequestCoveredMove.h"
#include "Fallout Shared/Pathfinding/PathingRequestFlee.h"
#include "Fallout Shared/Pathfinding/PathingRequestHide.h"
#include "Fallout Shared/Pathfinding/PathingRequestLOS.h"
#include "Fallout Shared/Pathfinding/PathingRequestOptimalLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequestSafeStraightLine.h"
#include "Fallout Shared/Pathfinding/VirtualPathingNode.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/ActorPackage.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/Movement/ActorPathHandler.h"
#include "Fallout/AI/Movement/DetailedActorPathHandler.h"
#include "Fallout/AI/Movement/VirtualActorPathHandler.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

#include <new>

// The loaded virtual node range is deliberately left unset, as the game's constructor does.
PathingSolution::PathingSolution()
  : NiRefObject(), VirtualPathingNodes(), CurrentPathingNodes(), DoorIDs(), bIncompletePath(0) {}

PathingSolution::~PathingSolution() {
    Clear();
}

VirtualPathingNode* PathingSolution::GetVirtualNode(uint32_t aiIndex) {
    return &VirtualPathingNodes.GetAt(aiIndex);
}

PathingNode* PathingSolution::GetNode(uint32_t aiIndex) {
    return &CurrentPathingNodes.GetAt(aiIndex);
}

// Valid when it has virtual nodes and, with a loaded virtual range, detailed nodes.
bool PathingSolution::IsValid() {
    if (VirtualPathingNodes.IsEmpty()) {
        return false;
    }
    if (iFirstLoadedVirtualNodeIndex != -1 && iLastLoadedVirtualNodeIndex != -1 && CurrentPathingNodes.IsEmpty()) {
        return false;
    }
    IsIncomplete();
    return true;
}

uint32_t PathingSolution::GetNodeCount() {
    return CurrentPathingNodes.GetSize();
}

float PathingSolution::ComputeSplineLength(int32_t aiNodeIndex) {
    if (aiNodeIndex < static_cast<int32_t>(GetNodeCount() - 1)) {
        PathingNode* pNextNode = GetNode(aiNodeIndex + 1);
        NiPoint3 kNextLocation = pNextNode->PathingNode::GetLocation().GetLocation();
        PathingNode* pNode = GetNode(aiNodeIndex);
        NiPoint3 kLocation = pNode->PathingNode::GetLocation().GetLocation();
        return (kLocation - kNextLocation).Length();
    }

    return 0.0;
}

void PathingSolution::SetIncomplete(bool abIncomplete) {
    bIncompletePath = abIncomplete;
}

VirtualPathingNode* PathingSolution::GetLastVirtualNode() {
    return &VirtualPathingNodes.GetLast();
}

int32_t PathingSolution::AddNode_Door(const NavMeshInfo* apNavMeshInfo, NiPoint3* apLocation, TESObjectREFR* apDoor) {
    uint32_t uiIndex = VirtualPathingNodes.AddUninitialized();
    VirtualPathingNode* pWhere = &VirtualPathingNodes.GetAt(uiIndex);
    new (pWhere) VirtualPathingNode(apNavMeshInfo, apLocation, apDoor);
    return uiIndex;
}

void PathingSolution::Save(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(&bIncompletePath, sizeof(bIncompletePath), 0);
    apSaveGameBuffer->SaveDataEndian(&iFirstLoadedVirtualNodeIndex, sizeof(iFirstLoadedVirtualNodeIndex), 0);
    apSaveGameBuffer->SaveDataEndian(&iLastLoadedVirtualNodeIndex, sizeof(iLastLoadedVirtualNodeIndex), 0);
    uint32_t uiVirtualNodeCount = VirtualPathingNodes.iSize;
    apSaveGameBuffer->SaveVariableSizedValue(uiVirtualNodeCount);
    for (uint32_t uiIndex = 0; uiIndex < uiVirtualNodeCount; ++uiIndex) {
        VirtualPathingNodes[uiIndex].Save(apSaveGameBuffer);
    }
    // The address &CurrentPathingNodes is never null, so it is not tested.
    uint32_t uiNodeCount = CurrentPathingNodes.iSize;
    apSaveGameBuffer->SaveVariableSizedValue(uiNodeCount);
    for (uint32_t i = 0; i < uiNodeCount; ++i) {
        CurrentPathingNodes[i].SaveGame(apSaveGameBuffer);
    }
    PathingAvoidNodeArray kAvoidNodes;
    uint32_t uiAvoidNodeCount = kAvoidNodes.iSize;
    apSaveGameBuffer->SaveVariableSizedValue(uiAvoidNodeCount);
    for (uint32_t uiIndex = 0; uiIndex < uiAvoidNodeCount; ++uiIndex) {
        kAvoidNodes.GetAt(uiIndex).SaveGame(apSaveGameBuffer);
    }
    uint32_t uiDoorCount = DoorIDs.iSize;
    apSaveGameBuffer->SaveVariableSizedValue(uiDoorCount);
    for (uint32_t j = 0; j < uiDoorCount; ++j) {
        apSaveGameBuffer->SaveFormID(DoorIDs[j], 0);
    }
}

ActorPathingMessageQueue::ActorPathingMessageQueue()
  : BSTCommonLLMessageQueue<ActorPathingMessage>(&MessagePool), NiRefObject() {}

ActorPathingMessageQueue::~ActorPathingMessageQueue() {}

// SetPathingFailed(false) only clears bPathingFailed: it is an initialiser here.
ActorMover::ActorMover(Actor* apActor)
  : LookAtPoint(NiPoint3::ZERO), CachedMoveDelta(NiPoint3::ZERO), spCurrentRequest(nullptr), spCurrentPath(nullptr),
    pCurrentPathHandler(nullptr), pActor(apActor), DetectionDoorID(0), spMessageQueue(nullptr),
    uiPreferredMoveMode(257), uiForcedMoveMode(0), iCachedLastValidMoveMode(0), iPreviousNode(0xFFFF),
    iNextNode(0xFFFF), FailedPathDestination(), eState(ACTOR_MOVER_STATE_INVALID), bPathingFailed(0), bForceMoveMode(0),
    bStopMovementNextUpdate(false), bFaceTargetPoint(false), bWaitingForPath(0), bActorChangedProcessLevel(false),
    bInDialog(false), bHaveValidDeltaThisFrame(0), bStopCurrentMoveAnim(false), iLastFailureTimeStamp(0),
    iFirstFailureTimeStamp(0), iContinuedFailureCount(0) {}

ActorMover::~ActorMover() {
    ClearCurrentData();
}

void ActorMover::SetMoveModePreference(uint32_t auiPreferredMoveMode) {
    if ((auiPreferredMoveMode & 0xFF00) != 0) {
        if (!pCurrentPathHandler) {
            DetailedActorPathHandler* pPathHandler = new DetailedActorPathHandler(pActor, this);
            pCurrentPathHandler = pPathHandler;
        }
        uiPreferredMoveMode = (auiPreferredMoveMode & 0xFF00) | (uiPreferredMoveMode & 0xF);
    }
    if ((auiPreferredMoveMode & 0xF) != 0) {
        uiPreferredMoveMode = (uiPreferredMoveMode & 0xFF00) | (auiPreferredMoveMode & 0xF);
    }
}

void ActorMover::ClearMoveModePreference(uint32_t auiPreferredMoveMode) {
    uiPreferredMoveMode &= ~auiPreferredMoveMode;
}

void ActorMover::ForceMoveMode(uint32_t aiMoveMode) {
    bForceMoveMode = true;
    uiForcedMoveMode = aiMoveMode;
}

void ActorMover::ClearForcedMoveMode() {
    bForceMoveMode = false;
    uiForcedMoveMode = 0;
}

void ActorMover::Update(float fdeltaT) {
    BaseProcess* pProcess;
    int32_t iProcessLevel;
    if (bStopMovementNextUpdate) {
        bStopCurrentMoveAnim = true;
        FinishMoving(true);
        bStopMovementNextUpdate = false;
    }
    if (pActor->IsDead(false)) {
        if (pCurrentPathHandler) {
            pCurrentPathHandler->ClearMovement();
        }
        goto RETURN_POINT;
    }
    pProcess = pActor->pCurrentProcess;
    if (pProcess) {
        if (pProcess->GetFadeState() != 4) {
            iProcessLevel = pProcess->eLevel;
            if (iProcessLevel == pActor->GetDesiredProcessLevel() &&
                pProcess->GetSitSleepState() == SIT_SLEEP_NORMAL &&
                !pActor->IsKnockedOut() &&
                pProcess->GetKnockState() == KNOCK_STATE_NORMAL &&
                !pProcess->GetCurrentlyReanimating()) {
                if (bActorChangedProcessLevel) {
                    bActorChangedProcessLevel = false;
                    HandleProcessLevelChange();
                }
                if (!bPathingFailed) {
                    if (!bWaitingForPath) {
                        if (spCurrentPath.m_pObject && spCurrentPath->IsValid()) {
                            eState = ACTOR_MOVER_STATE_MOVING;
                            UpdateMovement(fdeltaT);
                            goto RETURN_POINT;
                        }
                    }
                    else {
                        ActorPathingMessage msg;
                        if (spMessageQueue->TryPop(&msg)) {
                            if (msg.msgType) {
                                if (msg.msgType == PMT_ERROR) {
                                    bWaitingForPath = 0;
                                    spCurrentPath = msg.spSolution;
                                    if (spCurrentPath.m_pObject &&
                                        spCurrentPath->IsValid() &&
                                        spCurrentRequest->bAllowIncompletePath) {
                                        eState = ACTOR_MOVER_STATE_MOVING;
                                        DoStartMoving(fdeltaT);
                                    }
                                    else {
                                        eState = ACTOR_MOVER_STATE_FAILED;
                                        SetPathingFailed(true);
                                        if (spCurrentPath.m_pObject) {
                                            uint32_t uiLastLoadedVirtualNodeIndex = spCurrentPath
                                                                                        ->iLastLoadedVirtualNodeIndex;
                                            if (uiLastLoadedVirtualNodeIndex != -1 &&
                                                spCurrentPath->GetVirtualNodeCount()) {
                                                VirtualPathingNode*
                                                    pLastLoadedVirtualNode = spCurrentPath->GetVirtualNode(
                                                        uiLastLoadedVirtualNodeIndex
                                                    );
                                                if (pLastLoadedVirtualNode) {
                                                    FailedPathDestination = pLastLoadedVirtualNode->Location;
                                                    FailedPathDestination.ClearTriangleData();
                                                }
                                            }
                                        }
                                        ClearCurrentData();
                                    }
                                }
                            }
                            else {
                                eState = ACTOR_MOVER_STATE_MOVING;
                                bWaitingForPath = 0;
                                spCurrentPath = msg.spSolution;
                                DoStartMoving(fdeltaT);
                            }
                            goto RETURN_POINT;
                        }
                    }
                }
                if ((!spCurrentPath.m_pObject || !spCurrentPath->IsValid() || bPathingFailed) &&
                    pCurrentPathHandler &&
                    pCurrentPathHandler->NoPathUpdate(fdeltaT) == 5) {
                    eState = ACTOR_MOVER_STATE_FINISHED;
                }
            }
        }
    }

RETURN_POINT:
    SetCurrentFrameMoveModeAndDelta();
    DebugOnEvent();
}

uint32_t ActorMover::GetMoveMode() {
    uint32_t uiMoveMode = bPathingFailed ? GetPreferredMoveModeType() : iCachedLastValidMoveMode;
    return uiMoveMode;
}

// Copies and consumes the cached delta only when this frame has one.
bool ActorMover::GetMoveDelta(NiPoint3& aDelta) {
    bool bResult = false;
    if (bHaveValidDeltaThisFrame) {
        bHaveValidDeltaThisFrame = false;
        aDelta = CachedMoveDelta;
        bResult = true;
    }
    return bResult;
}

void ActorMover::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(&iPreviousNode, sizeof(iPreviousNode), 0);
    apSaveGameBuffer->SaveDataEndian(&iNextNode, sizeof(iNextNode), 0);
    apSaveGameBuffer->SaveDataEndian(&bPathingFailed, sizeof(bPathingFailed), 0);
    apSaveGameBuffer->SaveDataEndian(&uiPreferredMoveMode, sizeof(uiPreferredMoveMode), 0);
    apSaveGameBuffer->SaveDataEndian(&bForceMoveMode, sizeof(bForceMoveMode), 0);
    apSaveGameBuffer->SaveDataEndian(&uiForcedMoveMode, sizeof(uiForcedMoveMode), 0);
    apSaveGameBuffer->SaveDataEndian(&bStopMovementNextUpdate, sizeof(bStopMovementNextUpdate), 0);
    apSaveGameBuffer->SaveDataEndian(&bFaceTargetPoint, sizeof(bFaceTargetPoint), 0);
    apSaveGameBuffer->SaveDataEndian(&LookAtPoint, sizeof(LookAtPoint), 0);
    apSaveGameBuffer->SaveDataEndian(&CachedMoveDelta, sizeof(CachedMoveDelta), 0);
    apSaveGameBuffer->SaveDataEndian(&iCachedLastValidMoveMode, sizeof(iCachedLastValidMoveMode), 0);
    apSaveGameBuffer->SaveDataEndian(&bWaitingForPath, sizeof(bWaitingForPath), 0);
    apSaveGameBuffer->SaveDataEndian(&bActorChangedProcessLevel, sizeof(bActorChangedProcessLevel), 0);
    apSaveGameBuffer->SaveDataEndian(&bHaveValidDeltaThisFrame, sizeof(bHaveValidDeltaThisFrame), 0);
    apSaveGameBuffer->SaveDataEndian(&bInDialog, sizeof(bInDialog), 0);
    apSaveGameBuffer->SaveDataEndian(&bStopCurrentMoveAnim, sizeof(bStopCurrentMoveAnim), 0);
    apSaveGameBuffer->SaveDataEndian(&eState, sizeof(eState), 0);
    uint32_t uiTimeSinceLastFailure = appTimer.iLastTime - iLastFailureTimeStamp;
    apSaveGameBuffer->SaveDataEndian(&uiTimeSinceLastFailure, sizeof(uiTimeSinceLastFailure), 0);
    apSaveGameBuffer->SaveDataEndian(&iContinuedFailureCount, sizeof(iContinuedFailureCount), 0);
    FailedPathDestination.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveFormID(DetectionDoorID, 0);
    char cFlags = 0;
    if (spCurrentRequest.m_pObject) {
        cFlags |= SAVE_HAS_REQUEST;
    }
    if (spCurrentPath.m_pObject) {
        cFlags |= SAVE_HAS_PATH;
    }
    if (pCurrentPathHandler) {
        cFlags |= pCurrentPathHandler->IsDetailedPathHandler() ? SAVE_DETAILED_PATH_HANDLER : SAVE_VIRTUAL_PATH_HANDLER;
    }
    apSaveGameBuffer->SaveDataEndian(&cFlags, sizeof(cFlags), 0);
    if (spCurrentRequest.m_pObject) {
        uint8_t cType = spCurrentRequest->GetType();
        apSaveGameBuffer->SaveDataEndian(&cType, sizeof(cType), 0);
        spCurrentRequest->SaveGame(apSaveGameBuffer);
    }
    if (spCurrentPath.m_pObject) {
        spCurrentPath->Save(apSaveGameBuffer);
    }
    if (pCurrentPathHandler) {
        pCurrentPathHandler->SaveGame(apSaveGameBuffer);
    }
}

void ActorMover::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(&iPreviousNode, sizeof(iPreviousNode));
    apLoadGameBuffer->LoadData(&iNextNode, sizeof(iNextNode));
    apLoadGameBuffer->LoadData(&bPathingFailed, sizeof(bPathingFailed));
    apLoadGameBuffer->LoadData(&uiPreferredMoveMode, sizeof(uiPreferredMoveMode));
    apLoadGameBuffer->LoadData(&bForceMoveMode, sizeof(bForceMoveMode));
    apLoadGameBuffer->LoadData(&uiForcedMoveMode, sizeof(uiForcedMoveMode));
    apLoadGameBuffer->LoadData(&bStopMovementNextUpdate, sizeof(bStopMovementNextUpdate));
    apLoadGameBuffer->LoadData(&bFaceTargetPoint, sizeof(bFaceTargetPoint));
    apLoadGameBuffer->LoadData(&LookAtPoint, sizeof(LookAtPoint));
    apLoadGameBuffer->LoadData(&CachedMoveDelta, sizeof(CachedMoveDelta));
    apLoadGameBuffer->LoadData(&iCachedLastValidMoveMode, sizeof(iCachedLastValidMoveMode));
    apLoadGameBuffer->LoadData(&bWaitingForPath, sizeof(bWaitingForPath));
    apLoadGameBuffer->LoadData(&bActorChangedProcessLevel, sizeof(bActorChangedProcessLevel));
    apLoadGameBuffer->LoadData(&bHaveValidDeltaThisFrame, sizeof(bHaveValidDeltaThisFrame));
    apLoadGameBuffer->LoadData(&bInDialog, sizeof(bInDialog));
    apLoadGameBuffer->LoadData(&bStopCurrentMoveAnim, sizeof(bStopCurrentMoveAnim));
    apLoadGameBuffer->LoadData(&eState, sizeof(eState));
    if (apLoadGameBuffer->GetVersion() >= 14u) {
        uint32_t uiTimeSinceLastFailure = 0;
        apLoadGameBuffer->LoadData(&uiTimeSinceLastFailure, sizeof(uiTimeSinceLastFailure));
        if (uiTimeSinceLastFailure < appTimer.iLastTime) {
            iLastFailureTimeStamp = appTimer.iLastTime - uiTimeSinceLastFailure;
        }
        apLoadGameBuffer->LoadData(&iContinuedFailureCount, sizeof(iContinuedFailureCount));
    }
    if (apLoadGameBuffer->GetVersion() >= 7u) {
        FailedPathDestination.LoadGame(apLoadGameBuffer);
    }
    apLoadGameBuffer->LoadFormID(DetectionDoorID);
    char cFlags = 0;
    apLoadGameBuffer->LoadData(&cFlags, sizeof(cFlags));
    if ((cFlags & SAVE_HAS_REQUEST) != 0) {
        uint8_t cType = 0;
        apLoadGameBuffer->LoadData(&cType, sizeof(cType));
        spCurrentRequest = PathingRequest::CreatePathingRequest(static_cast<PATHING_REQUEST_TYPE>(cType));
        spCurrentRequest->LoadGame(apLoadGameBuffer);
        if (pActor &&
            pActor->pCurrentProcess &&
            !pActor->pCurrentProcess->CurrentPackage.GetFlag(ActorPackage::SAVELOAD_SHARED_PACKAGE)) {
            spCurrentRequest->SetActorLockData(pActor);
        }
    }
    char cStalePath = 0;
    if ((cFlags & SAVE_HAS_PATH) != 0) {
        spCurrentPath = new PathingSolution;
        spCurrentPath->LoadGame(apLoadGameBuffer);
        for (uint32_t i = 0; i < spCurrentPath->GetVirtualNodeCount(); ++i) {
            VirtualPathingNode* pVirtualNode = spCurrentPath->GetVirtualNode(i);
            if (pVirtualNode->HasNavMesh()) {
                if (!spCurrentPath->GetVirtualNode(i)->Location.GetBestNavMeshInfo()) {
                    int32_t iFormID = pActor->iFormID;
                    const char* pEditorID = pActor->GetFormEditorID();
                    Warning(
                        "PATHFINDING: Actor %s (0x%08x) is using a stale path (the navmesh changed), forcing a "
                        "failure",
                        pEditorID,
                        iFormID
                    );
                    cStalePath = 1;
                    break;
                }
            }
        }
    }
    if ((cFlags & SAVE_VIRTUAL_PATH_HANDLER) != 0) {
        VirtualActorPathHandler*
            pPathHandler = new VirtualActorPathHandler(pActor, this, spCurrentPath, spCurrentRequest.m_pObject);
        pCurrentPathHandler = pPathHandler;
    }
    else if ((cFlags & SAVE_DETAILED_PATH_HANDLER) != 0) {
        DetailedActorPathHandler*
            pPathHandler = new DetailedActorPathHandler(pActor, this, spCurrentPath, spCurrentRequest.m_pObject);
        pCurrentPathHandler = pPathHandler;
    }
    if (pCurrentPathHandler) {
        pCurrentPathHandler->LoadGame(apLoadGameBuffer);
    }
    if (cStalePath) {
        ClearCurrentData();
        eState = ACTOR_MOVER_STATE_FAILED;
        SetPathingFailed(true);
    }
}

void ActorMover::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (pCurrentPathHandler) {
        pCurrentPathHandler->InitLoadGame(apLoadGameBuffer);
    }
    if (spCurrentRequest.m_pObject) {
        if (spCurrentRequest->GetType() == PATHING_REQUEST_LOS) {
            PathingRequestLOS* pLOSRequest = static_cast<PathingRequestLOS*>(spCurrentRequest.m_pObject);
            CombatController* pCombatController = pActor->GetCombatController();
            if (pCombatController && pCombatController->pCombatGroup) {
                NiPointer<PathingLOSGridMap> spLOSGridMap(pCombatController->pCombatGroup->GetSearchLOSGridMap());
                pLOSRequest->SetLOSGridMap(spLOSGridMap);
            }
        }
        spCurrentRequest->InitLoadGame(apLoadGameBuffer);
        spCurrentRequest->SetActorLockData(pActor);
    }
}

// Restarts a path search that was waiting when the game was saved. A line-of-sight request (type 5) without a grid
// map fails at once; otherwise the request is resubmitted with the (re-created if missing) message
// queue, and a refused request fails the move.
// KEEP: the exe copies BuildPath's by-value smart pointers into their arguments before it calls
// PathManager::QInstance for the receiver; C++17 evaluates the receiver QInstance() first, so src cannot reproduce
// the call order.
// The exe's cleanup-only EH frame is left out.
void ActorMover::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (!bWaitingForPath) {
        return;
    }
    if (!spCurrentRequest) {
        return;
    }
    bool bHideFailed = false;
    PathingRequest* pRequest = spCurrentRequest;
    if (pRequest->GetType() == 5) {
        PathingRequestLOS* pLOS = NiSmartPointerCast<PathingRequestLOS>(spCurrentRequest);
        if (!pLOS->spLOSGridMap.m_pObject) {
            bHideFailed = true;
        }
    }
    if (!spMessageQueue) {
        spMessageQueue = new ActorPathingMessageQueue;
    }
    if (!bHideFailed && PathManager::QInstance()->BuildPath(spCurrentRequest, spMessageQueue, true))
    // BuildPath takes both smart pointers by value and releases them.
    {
        return;
    }

    SetPathingFailed(true);
    bWaitingForPath = false;
}

void ActorMover::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (bWaitingForPath) {
        spMessageQueue = new ActorPathingMessageQueue;
    }
    eState = ACTOR_MOVER_STATE_INVALID;
    spCurrentRequest = static_cast<PathingRequest*>(nullptr);
    spCurrentPath = static_cast<PathingSolution*>(nullptr);
    if (pCurrentPathHandler) {
        delete pCurrentPathHandler;
    }
    pCurrentPathHandler = nullptr;
    uiPreferredMoveMode = 257;
    SetPathingFailed(false);
    iPreviousNode = -1;
    iNextNode = -1;
    bActorChangedProcessLevel = false;
    bStopMovementNextUpdate = false;
    bWaitingForPath = 0;
    bInDialog = false;
    LookAtPoint = NiPoint3::ZERO;
    CachedMoveDelta = NiPoint3::ZERO;
    bFaceTargetPoint = false;
    bForceMoveMode = 0;
    bStopCurrentMoveAnim = false;
    bHaveValidDeltaThisFrame = 0;
    uiForcedMoveMode = 0;
    iCachedLastValidMoveMode = 0;
    DetectionDoorID = 0;
    iContinuedFailureCount = 0;
    iLastFailureTimeStamp = 0;
    iFirstFailureTimeStamp = 0;
}

NiPoint3 ActorMover::GetAnticipatedLocation(float afTime) {
    if (pCurrentPathHandler) {
        return pCurrentPathHandler->ComputeApproximateLocation(afTime);
    }
    return pActor->GetLocationOnReference();
}

void ActorMover::RemoveReference(TESObjectREFR* apReference) {
    if (pCurrentPathHandler) {
        pCurrentPathHandler->HandleDeletedReference(apReference);
    }
}

bool ActorMover::GetCurrentGoal(PathingLocation* apRetLocation) {
    if (!spCurrentPath.m_pObject) {
        return false;
    }
    if (!spCurrentPath->IsValid()) {
        return false;
    }
    VirtualPathingNode* pNode = spCurrentPath->GetLastVirtualNode();
    if (!pNode) {
        return false;
    }
    *apRetLocation = *&pNode->Location;
    return true;
}

uint32_t ActorMover::GetCurrentNodeIndex() {
    if (pCurrentPathHandler) {
        return pCurrentPathHandler->GetCurrentNodeIndex();
    }
    return 0;
}

bool ActorMover::GetLocationAt(uint32_t auiNodeIndex, NiPoint3* apLocation) {
    return spCurrentPath && pCurrentPathHandler && pCurrentPathHandler->GetNodeLocation(auiNodeIndex, *apLocation);
}

void ActorMover::SetLookAtTarget(NiPoint3 aPoint) {
    LookAtPoint = aPoint;
    bFaceTargetPoint = true;
    if (pActor->GetCurrentProcessType() == PROCESS_TYPE_HIGH) {
        if (!pCurrentPathHandler) {
            DetailedActorPathHandler* pPathHandler = new DetailedActorPathHandler(pActor, this);
            pCurrentPathHandler = pPathHandler;
        }
        DebugOnEvent();
    }
}

// Finishes the mover: drops the request, path and path handler, gives a waiting mover a fresh message queue, and clears
// the forced move mode.
void ActorMover::ClearCurrentData() {
    eState = ACTOR_MOVER_STATE_FINISHED;
    spCurrentRequest = nullptr;
    spCurrentPath = nullptr;
    if (pCurrentPathHandler) {
        ActorPathHandler* pPathHandler = pCurrentPathHandler;
        delete pPathHandler;
        pCurrentPathHandler = nullptr;
    }
    if (bWaitingForPath) {
        spMessageQueue = new ActorPathingMessageQueue;
        bWaitingForPath = false;
    }
    ClearForcedMoveMode();
    DebugOnEvent();
}

// Requires an attached parent cell and a player who is not sleeping or resting.
bool ActorMover::IsActorInLoadedArea() {
    if (!pActor->GetParentCell()) {
        return false;
    }
    TESObjectCELL* pParentCell = pActor->GetParentCell();
    return pParentCell->GetAttached() && !pPlayer->IsSleepingorResting();
}

// Treats terminal states as complete and active rotation or a valid path as incomplete.
bool ActorMover::IsPathingComplete() {
    if (bWaitingForPath) {
        return false;
    }
    if (eState == ACTOR_MOVER_STATE_FINISHED || eState == ACTOR_MOVER_STATE_FAILED) {
        return true;
    }
    if (!spCurrentPath) {
        return eState != ACTOR_MOVER_STATE_ROTATING;
    }
    if (spCurrentPath->IsValid()) {
        return false;
    }
    return eState != ACTOR_MOVER_STATE_ROTATING;
}

// The destination of the last failed path, or null when none is recorded.
PathingLocation* ActorMover::GetFailedPathDestination() {
    if (FailedPathDestination.QUnused()) {
        return nullptr;
    }
    return &FailedPathDestination;
}

bool ActorMover::IsPathValid() {
    if (!spCurrentPath) {
        return false;
    }
    if (spCurrentPath->IsValid()) {
        return true;
    }
    return false;
}

void ActorMover::SetPathingDoorReference(TESObjectREFR* apDoor) {
    uint32_t uiDoorID = apDoor ? apDoor->TESForm::GetFormID() : 0;
    DetectionDoorID = uiDoorID;
}

void ActorMover::FinishMoving(bool abClearHandler) {
    ACTOR_MOVER_STATE eState = this->eState;
    if (eState < ACTOR_MOVER_STATE_FAILED || eState > ACTOR_MOVER_STATE_FAILED_PROCESS_UPDATE) {
        this->eState = ACTOR_MOVER_STATE_FINISHED;
    }
    spCurrentRequest = static_cast<PathingRequest*>(nullptr);
    spCurrentPath = static_cast<PathingSolution*>(nullptr);
    if (pCurrentPathHandler && abClearHandler) {
        pCurrentPathHandler->ClearMovement();
    }

    if (bWaitingForPath) {
        spMessageQueue = new ActorPathingMessageQueue;
        bWaitingForPath = 0;
    }
    SetPathingFailed(false);
    DebugOnEvent();
}

void ActorMover::ForceStopMoving() {
    DoStopMoving();
    if (bStopMovementNextUpdate) {
        bStopMovementNextUpdate = false;
        FinishMoving(true);
        iCachedLastValidMoveMode = GetPreferredMoveModeType();
        CachedMoveDelta = NiPoint3::ZERO;
        bHaveValidDeltaThisFrame = true;
    }
    DebugOnEvent();
}

// Rotating, waiting for a path, or following a valid path that has not failed; never while a move mode is forced.
bool ActorMover::IsPathing() {
    if (bForceMoveMode) {
        return false;
    }
    if (eState == ACTOR_MOVER_STATE_ROTATING) {
        return true;
    }
    if (bWaitingForPath) {
        return true;
    }
    if (!spCurrentPath) {
        return false;
    }
    if (!spCurrentPath->IsValid()) {
        return false;
    }
    if (!bPathingFailed) {
        return true;
    }
    return false;
}

bool ActorMover::IsWaitingForAvoidance() {
    return !pCurrentPathHandler ||
        !spCurrentPath ||
        !spCurrentPath->IsValid() ||
        pCurrentPathHandler->IsWaitingForAvoidance();
}

// KEEP: the exe copies both smart pointers into BuildPath's arguments (the NiPointer copy constructors, run in
// place) before it calls PathManager::QInstance; C++17 evaluates a member call's object expression before its
// arguments, so src calls QInstance first. Copying the pointers into locals first would add a copy and a release the
// exe does not make. The exe's cleanup-only C++ EH frame is left out.
bool ActorMover::BackGroundBuildPath(const PathingRequest* apRequest) {
    bool bResult = false;
    apRequest->CheckValid();
    eState = ACTOR_MOVER_STATE_INIT;
    ClearDataForNewPathingRequest(false);
    if (!apRequest->Destination.QUnused()) {
        const PathingLocation* pOrigin = &apRequest->Origin;
        const PathingLocation* pDestination = &apRequest->Destination;
        TESObjectCELL* pOriginCell = pOrigin->GetCell();
        bool bExterior = !pOriginCell || !pOriginCell->GetInterior();
        bool bSameSpace;
        if (bExterior) {
            bSameSpace = pOrigin->pWorldSpace == pDestination->pWorldSpace;
        }
        else {
            bSameSpace = pOriginCell == pDestination->GetCell();
        }
        NiPoint3 kOriginLocation = pOrigin->GetLocation();
        bool bOriginUnderWater = kOriginLocation.z < pActor->GetRelevantWaterHeight() + 50.0;
        NiPoint3 kDestinationLocation = pDestination->GetLocation();
        bool bDestinationUnderWater = kDestinationLocation.z < pActor->GetRelevantWaterHeight() + 50.0;
        if (bSameSpace && !bOriginUnderWater && !bDestinationUnderWater) {
            // Reads both locations again, the destination first.
            NiPoint3 kGoal = pDestination->GetLocation();
            NiPoint3 kStart = pOrigin->GetLocation();
            NiPoint3 kGoalDelta = kStart - kGoal;
            double dSqrDistance = kGoalDelta.SqrXYLength();
            // Already at the goal: within the target radius and the goal's height range.
            if (apRequest->fTargetRadius * apRequest->fTargetRadius >= dSqrDistance &&
                apRequest->fGoalZDelta >= std::fabs(kGoalDelta.z)) {
                FinishMoving(true);
                bool bRotated = true;
                if (apRequest->bFaceTargetAtGoal) {
                    bRotated = RequestRotateActor(apRequest->fGoalAngle, true);
                }
                return bRotated;
            }
        }
    }
    // The request is copied as its own subclass, which GetType names.
    switch (apRequest->GetType()) {
        case PATHING_REQUEST_STANDARD: {
            PathingRequest* pRequest = new PathingRequest(*apRequest);
            spCurrentRequest = pRequest;
            break;
        }
        case PATHING_REQUEST_FLEE: {
            PathingRequestFlee* pRequest = new PathingRequestFlee(*static_cast<const PathingRequestFlee*>(apRequest));
            spCurrentRequest = pRequest;
            break;
        }
        case PATHING_REQUEST_HIDE: {
            PathingRequestHide* pRequest = new PathingRequestHide(*static_cast<const PathingRequestHide*>(apRequest));
            spCurrentRequest = pRequest;
            break;
        }
        case PATHING_REQUEST_LOS: {
            PathingRequestLOS* pRequest = new PathingRequestLOS(*static_cast<const PathingRequestLOS*>(apRequest));
            spCurrentRequest = pRequest;
            break;
        }
        case PATHING_REQUEST_STRAIGHT_LINE: {
            PathingRequestSafeStraightLine* pRequest = new PathingRequestSafeStraightLine(
                *static_cast<const PathingRequestSafeStraightLine*>(apRequest)
            );
            spCurrentRequest = pRequest;
            break;
        }
        case PATHING_REQUEST_OPTIMAL_LOCATION: {
            PathingRequestOptimalLocation* pRequest = new PathingRequestOptimalLocation(
                *static_cast<const PathingRequestOptimalLocation*>(apRequest)
            );
            spCurrentRequest = pRequest;
            break;
        }
        case PATHING_REQUEST_COVERED_MOVE: {
            PathingRequestCoveredMove* pRequest = new PathingRequestCoveredMove(
                *static_cast<const PathingRequestCoveredMove*>(apRequest)
            );
            spCurrentRequest = pRequest;
            break;
        }
        default: break;
    }
    spCurrentRequest->Origin.ClearTriangleData();
    spCurrentRequest->Destination.ClearTriangleData();
    spCurrentRequest->SetActorLockData(pActor);
    spCurrentRequest->SetRetryCount(iContinuedFailureCount + 3);
    if (!bPathingFailed && pCurrentPathHandler) {
        pCurrentPathHandler->SetWaitForUpdatedPath(spCurrentRequest.m_pObject);
    }
    if (pCurrentPathHandler && pCurrentPathHandler->IsDetailedPathHandler()) {
        DetailedActorPathHandler::UpdateAvoidNodesForPath(
            pActor,
            spCurrentRequest.m_pObject,
            static_cast<DetailedActorPathHandler*>(pCurrentPathHandler)
        );
    }
    else if (pActor->GetParentCell() && pActor->GetParentCell()->GetAttached()) {
        DetailedActorPathHandler::UpdateAvoidNodesForPath(pActor, spCurrentRequest.m_pObject, nullptr);
    }
    bool bPlayerWaiting = pPlayer->IsSleepingorResting();
    if (!spMessageQueue.m_pObject) {
        ActorPathingMessageQueue* pQueue = new ActorPathingMessageQueue;
        spMessageQueue = pQueue;
    }
    // Both smart pointers go by value: their copy constructors fill the argument slots, BuildPath releases them.
    if (PathManager::QInstance()->BuildPath(spCurrentRequest, spMessageQueue, !bPlayerWaiting)) {
        bWaitingForPath = 1;
        bResult = true;
    }
    else {
        ClearCurrentData();
        eState = ACTOR_MOVER_STATE_FAILED;
        SetPathingFailed(true);
    }
    return bResult;
}

void ActorMover::ResumeMoving() {
    bStopMovementNextUpdate = false;
}

// Checks whether the mover is in the rotating state.
bool ActorMover::IsRotating() {
    return eState == ACTOR_MOVER_STATE_ROTATING;
}

void ActorMover::StopMoving() {
    DoStopMoving();
    DebugOnEvent();
}

void ActorMover::DoStopMoving() {
    if (eState != ACTOR_MOVER_STATE_FINISHED) {
        if (pCurrentPathHandler && !pCurrentPathHandler->IsDetailedPathHandler()) {
            eState = ACTOR_MOVER_STATE_FINISHED;
        }

        bStopMovementNextUpdate = true;
    }
    if (bPathingFailed) {
        bStopMovementNextUpdate = true;
    }
    SetPathingFailed(false);
}

void ActorMover::SetPathingFailed(bool abValue) {
    bPathingFailed = abValue;
    if (abValue) {
        if (pActor->GetCurrentProcessType() == PROCESS_TYPE_LOW &&
            (!pActor->GetParentCell() || !pActor->GetParentCell()->GetInterior())) {
            int32_t iCellX = static_cast<int32_t>(pActor->GetLocationOnReference().x) >> 12;
            int32_t iCellY = static_cast<int32_t>(pActor->GetLocationOnReference().y) >> 12;
            if (!iCellX && !iCellY) {
                // Passes each coordinate's low 16 bits, zero-extended.
                int32_t iCoord = TESObjectCELL::GetCoord(static_cast<uint16_t>(iCellX), static_cast<uint16_t>(iCellY));
                TESWorldSpace* pWorldSpace = pActor->GetWorldSpace();
                NavMeshInfoMap* pNavMeshInfoMap = TES::pInstance->GetNavMeshInfoMap();
                if (!pNavMeshInfoMap->FindNavMeshInfoForCellKey(pWorldSpace, iCoord)) {
                    TESWorldSpace* pDestWorldSpace = spCurrentRequest->Destination.pWorldSpace;
                    TESObjectCELL* pDestCell = spCurrentRequest->Destination.pCell;
                    pActor->MoveActorAndFollowers(
                        spCurrentRequest->Destination.GetLocation(),
                        0.0f,
                        pDestCell,
                        pDestWorldSpace,
                        false,
                        true
                    );
                }
            }
        }
        float fThreshold = pActor->IsVisible(7) ? 3.0f : 1.5f;
        if (spCurrentRequest.m_pObject) {
            TESObjectCELL* pOrCell = spCurrentRequest->Origin.GetCell();
            TESObjectCELL* pDestCell = spCurrentRequest->Destination.GetCell();
            if (pOrCell && pDestCell) {
                double dOrWaterHeight = TES::pInstance->GetWaterHeight(spCurrentRequest->Origin.GetLocation(), pOrCell);
                bool bOrUnderWater = spCurrentRequest->Origin.GetLocation().z < dOrWaterHeight;
                double dDestWaterHeight = TES::pInstance->GetWaterHeight(
                    spCurrentRequest->Destination.GetLocation(),
                    pDestCell
                );
                bool bDestUnderWater = spCurrentRequest->Destination.GetLocation().z < dDestWaterHeight;
                if (bOrUnderWater != bDestUnderWater) {
                    fThreshold = 1.0f;
                }
            }
        }
        float fTimeSinceLastFaillure = static_cast<double>(appTimer.iLastTime - iLastFailureTimeStamp) *
            0.0010000000474974513;
        float fTimeSinceFirstFaillure = static_cast<double>(appTimer.iLastTime - iFirstFailureTimeStamp) *
            0.0010000000474974513;
        if (!(fThreshold <= static_cast<double>(fTimeSinceLastFaillure))) {
            if (!(fThreshold <= static_cast<double>(fTimeSinceFirstFaillure)) || iContinuedFailureCount < 2) {
                ++iContinuedFailureCount;
                iLastFailureTimeStamp = appTimer.iLastTime;
            }
            else {
                WarpToNextNode();
                iContinuedFailureCount = 0;
                iLastFailureTimeStamp = appTimer.iLastTime;
                iFirstFailureTimeStamp = appTimer.iLastTime;
            }
        }
        else {
            iContinuedFailureCount = 0;
            iLastFailureTimeStamp = appTimer.iLastTime;
            iFirstFailureTimeStamp = appTimer.iLastTime;
        }
    }
    else {
        FailedPathDestination.SetUnused();
    }
}

// Clears the outstanding path request.
void ActorMover::ClearDataForNewPathingRequest(bool abClearWaitingFlag) {
    bStopMovementNextUpdate = false;
    ClearForcedMoveMode();
    spCurrentRequest = nullptr;
    if (bWaitingForPath) {
        spMessageQueue = new ActorPathingMessageQueue;
        if (abClearWaitingFlag) {
            bWaitingForPath = false;
        }
    }
    spCurrentPath = nullptr;
    SetPathingFailed(false);
}

void ActorMover::OnMoveToLow() {
    if (pCurrentPathHandler) {
        if (bActorChangedProcessLevel && !pCurrentPathHandler->IsDetailedPathHandler()) {
            bActorChangedProcessLevel = false;
        }
        else if (pCurrentPathHandler->IsDetailedPathHandler()) {
            bActorChangedProcessLevel = true;
        }
    }
}

void ActorMover::OnMoveToHigh() {
    if (pActor->GetCurrentAIProcess()->GetSitSleepState() != SIT_SLEEP_IS_SITTING) {
        if (pActor->GetCurrentAIProcess()->GetSitSleepState() != SIT_SLEEP_IS_SLEEPING) {
            if (pActor->GetCurrentAIProcess()->GetSitSleepState()) {
                pActor->GetOutofFurnitureQuick();
            }
            else if (pCurrentPathHandler && !IsPathingComplete()) {
                if (bActorChangedProcessLevel && pCurrentPathHandler->IsDetailedPathHandler()) {
                    if (spCurrentPath) {
                        if (iPreviousNode != iNextNode || iPreviousNode >= spCurrentPath->GetVirtualNodeCount() - 1) {
                            bActorChangedProcessLevel = false;
                        }
                    }
                }
                else if (!pCurrentPathHandler->IsDetailedPathHandler()) {
                    bActorChangedProcessLevel = true;
                }
            }
            else if (!pSaveLoadGame->GetSaveGameLoading()) {
                pActor->MoveToSafePoint(false);
            }
        }
    }
}

// Builds a request on the stack for the goal, sets the angle to face there and starts the search. The stack request is
// only destroyed if a call throws.
bool ActorMover::SetGoalAndAngle(
    const NiPoint3& aGoalLocation,
    TESObjectCELL* apCell,
    TESWorldSpace* apWorldSpace,
    float afAngleToFace,
    float afTargetRadius,
    PathingAvoidNodeArray* apAvoidNodes
) {
    alignas(PathingRequest) uint8_t aRequestStorage[sizeof(PathingRequest)];
    PathingRequest* pRequest = ::new (aRequestStorage) PathingRequest;
    BuildRequest(pActor, pRequest, aGoalLocation, apCell, apWorldSpace, afTargetRadius, apAvoidNodes);
    pRequest->SetGoalAngleToFace(afAngleToFace);
    bool bResult = SetGoal(pRequest);
    pRequest->~PathingRequest();
    return bResult;
}

// Reads the target's angle, worldspace, parent cell and location in that order, then passes them to the six-parameter
// overload; GetLocationOnReference is virtual and is called through the reference.
bool ActorMover::SetGoalAndAngle(
    TESObjectREFR* apTargetRef,
    float afTargetRadius,
    PathingAvoidNodeArray* apAvoidNodes
) {
    float fAngleToFace = apTargetRef->GetAngleOnReference().z;
    TESWorldSpace* pWorldSpace = apTargetRef->GetWorldSpace();
    TESObjectCELL* pParentCell = apTargetRef->GetParentCell();
    const NiPoint3* pLocation = &apTargetRef->GetLocationOnReference();
    return SetGoalAndAngle(*pLocation, pParentCell, pWorldSpace, fAngleToFace, afTargetRadius, apAvoidNodes);
}

bool ActorMover::SetGoal(const PathingRequest* apRequest) {
    return BackGroundBuildPath(const_cast<PathingRequest*>(apRequest));
}

bool ActorMover::SetGoal(
    const NiPoint3& aGoalLocation,
    TESObjectCELL* apCell,
    TESWorldSpace* apWorldSpace,
    float afTargetRadius,
    PathingAvoidNodeArray* apAvoidNodes
) {
    PathingRequest kRequest;
    ActorMover::BuildRequest(pActor, &kRequest, aGoalLocation, apCell, apWorldSpace, afTargetRadius, apAvoidNodes);
    return SetGoal(&kRequest);
}

void ActorMover::ClearLookAtTarget() {
    LookAtPoint = NiPoint3::ZERO;
    bFaceTargetPoint = false;
    DebugOnEvent();
}

bool ActorMover::RequestRotateActor(float afTargetAngle, bool abForceTurn) {
    bool bResult = false;
    ClearDataForNewPathingRequest(true);
    if (pActor->GetAnimation()) {
        if (pActor->GetAnimation()->ShouldBeMoving()) {
            bStopCurrentMoveAnim = true;
        }
    }
    if (pActor->CanMove() && pActor->GetSitSleepState() == SIT_SLEEP_NORMAL) {
        if (!IsActorInLoadedArea()) {
            eState = ACTOR_MOVER_STATE_FINISHED;
            pActor->SetHeading(afTargetAngle);
            bResult = true;
        }
    }
    else {
        eState = ACTOR_MOVER_STATE_FINISHED;
        bResult = true;
    }
    if (!bResult) {
        if (pCurrentPathHandler && !pCurrentPathHandler->IsDetailedPathHandler()) {
            if (pCurrentPathHandler) {
                delete pCurrentPathHandler;
            }
            pCurrentPathHandler = nullptr;
        }

        if (!pCurrentPathHandler) {
            DetailedActorPathHandler* pPathHandler = new DetailedActorPathHandler(pActor, this);
            pCurrentPathHandler = pPathHandler;
        }
        if (pCurrentPathHandler->InitRotationOnly(afTargetAngle, abForceTurn)) {
            eState = ACTOR_MOVER_STATE_ROTATING;
            bResult = true;
        }
        else {
            eState = ACTOR_MOVER_STATE_FINISHED;
            bResult = false;
        }
    }
    DebugOnEvent();
    return bResult;
}

// The by-value NiPoint3 is three dwords on the stack.
bool ActorMover::RequestRotateActor(NiPoint3 akTargetPos, bool abForceTurn) {
    NiPoint3 kDirection = akTargetPos - pActor->GetLocationOnReference();
    float fRotation = GetZAngleFromVector(kDirection);
    return RequestRotateActor(fRotation, abForceTurn);
}

uint8_t ActorMover::DidPathingFail() {
    return bPathingFailed;
}

uint8_t ActorMover::IsWaitingOnPath() {
    return bWaitingForPath;
}

uint8_t ActorMover::IsStoppingMovement() {
    return bStopMovementNextUpdate;
}

PathingRequest* ActorMover::GetCurrentRequest() {
    return spCurrentRequest.m_pObject;
}

bool ActorMover::ComputePathPassesThroughPlayersArea() {
    return spCurrentPath && spCurrentPath->IsValid() && pCurrentPathHandler->ComputePathPassesThroughPlayersArea();
}

void ActorMover::SetInDialog(bool abVal) {
    bInDialog = abVal;
}

bool ActorMover::IsMovementBlocked() const {
    return pCurrentPathHandler && spCurrentPath && spCurrentPath->IsValid() && pCurrentPathHandler->IsMovementBlocked();
}

float ActorMover::GetCurrentPercent() const {
    if (pCurrentPathHandler) {
        return pCurrentPathHandler->GetPercentTraveled();
    }

    return 0.0;
}

float ActorMover::GetDistanceTraveled() const {
    if (pCurrentPathHandler) {
        return pCurrentPathHandler->GetDistanceTraveled();
    }

    return 0.0;
}

float ActorMover::GetDistanceRemaining() const {
    if (pCurrentPathHandler) {
        return pCurrentPathHandler->GetDistanceRemaining();
    }

    return 0.0;
}

bool ActorMover::IsPathIncomplete() const {
    if (!spCurrentPath.m_pObject) {
        return false;
    }
    // IsIncomplete returns bIncompletePath's raw byte, so it is reduced to 0 or 1 here.
    return spCurrentPath->IsIncomplete() != 0;
}

bool ActorMover::MoveActorToLocation(const NiPoint3& aNewLoc) {
    if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
        Actor* pActor = this->pActor;
        TaskQueueInterface* pTaskQueue = TaskQueueInterface::GetInstance();
        pTaskQueue->QueueActorSetPosition(pActor, aNewLoc);
        pActor = this->pActor;
        pTaskQueue = TaskQueueInterface::GetInstance();
        pTaskQueue->QueueActorMoveCharController(pActor);
    }
    else {
        this->pActor->SetPosition(aNewLoc);
    }
    this->pActor->SetLocationOnReference(aNewLoc);
    TESWorldSpace* pWorld = this->pActor->GetWorldSpace();
    if (!pWorld) {
        return true;
    }
    TESObjectCELL* pParentCell = this->pActor->TESObjectREFR::GetParentCell();
    if (pParentCell != pWorld->GetCellFromWorldCoord(aNewLoc)) {
        TESObjectREFR::MoveRefToNewSpace(this->pActor, nullptr, pWorld);
    }
    return this->pActor->GetDesiredProcessLevel() || pPlayer->IsSleepingorResting();
}

ActorMover::END_OF_PATH_CHECK_RESULT ActorMover::EndOfCurrentPathCheck() {
    if (iNextNode) {
        iPreviousNode = iNextNode;
        if (spCurrentPath->GetNodeCount()) {
            PathingNode* pLastNode = spCurrentPath->GetLastNode();
            if (pLastNode->IsTeleportDoorNode()) {
                if (pLastNode->ActivateNode(pActor) || pActor->MobileObject::GetCurrentAIProcess()->GetFadeState()) {
                    spCurrentPath->ClearCurrentDetailedPath();
                    return ActorMover::END_OF_PATH_TELEPORT;
                }

                eState = ACTOR_MOVER_STATE_FAILED;
                SetPathingFailed(true);
                DoStopMoving();
                return ActorMover::END_OF_PATH_FAILED;
            }
        }

        uint32_t uiPreviousNode = iPreviousNode;
        if (uiPreviousNode >= pCurrentPathHandler->spPath->GetVirtualNodeCount() - 1) {
            DoStopMoving();
        }
        return ActorMover::END_OF_PATH_OK;
    }

    eState = ACTOR_MOVER_STATE_FAILED;
    SetPathingFailed(true);
    StopMoving();
    return ActorMover::END_OF_PATH_FAILED;
}

bool ActorMover::GetLookAtPoint(NiPoint3& apPoint) const {
    if (!HasLookAtTarget()) {
        return false;
    }
    apPoint = LookAtPoint;
    return true;
}

bool ActorMover::SetGoal(TESObjectREFR* apReference, float afTargetRadius, PathingAvoidNodeArray* apAvoidNodes) {
    TESWorldSpace* pWorldSpace = apReference->GetWorldSpace();
    TESObjectCELL* pParentCell = apReference->GetParentCell();
    const NiPoint3* pLocation = &apReference->GetLocationOnReference();
    return SetGoal(*pLocation, pParentCell, pWorldSpace, afTargetRadius, apAvoidNodes);
}

void ActorMover::DoStartMoving(float afDelta) {
    bool bDetailed;
    DebugOnEvent();
    if (spCurrentPath->GetNodeCount()) {
        if (pActor->GetCurrentProcessType()) {
            bDetailed = false;
            spCurrentRequest->SetActorLockData(pActor);
            Pathing::UPDATE_DETAILED_PATH_RESULT eUpdated = Pathing::UpdateSolutionMoveToLow(
                spCurrentRequest.m_pObject,
                spCurrentPath.m_pObject
            );
            if (eUpdated >= Pathing::FAILED && eUpdated <= Pathing::NO_CHANGE) {
                eState = ACTOR_MOVER_STATE_FAILED;
                SetPathingFailed(true);
                return;
            }
        }
        else {
            bDetailed = true;
        }
    }
    else if (pActor->GetCurrentProcessType()) {
        bDetailed = false;
    }
    else {
        bDetailed = true;
        spCurrentRequest->SetActorLockData(pActor);
        Pathing::UPDATE_DETAILED_PATH_RESULT eUpdated = Pathing::UpdateSolutionMoveToHigh(
            spCurrentRequest.m_pObject,
            spCurrentPath.m_pObject,
            0
        );
        if (eUpdated >= Pathing::FAILED && eUpdated <= Pathing::NO_CHANGE) {
            pActor->MoveToSafePoint(true);
            eState = ACTOR_MOVER_STATE_FAILED;
            SetPathingFailed(true);
            return;
        }
    }
    if (bDetailed) {
        iPreviousNode = spCurrentPath->iFirstLoadedVirtualNodeIndex;
        iNextNode = spCurrentPath->iLastLoadedVirtualNodeIndex;
        ActorPathHandler* pOldPathHandler = pCurrentPathHandler;
        DetailedActorPathHandler*
            pPathHandler = new DetailedActorPathHandler(pActor, this, spCurrentPath, spCurrentRequest.m_pObject);
        pCurrentPathHandler = pPathHandler;
        pCurrentPathHandler->HandlePreviousPathTransition(pOldPathHandler);
        if (pOldPathHandler) {
            delete pOldPathHandler;
        }
    }
    else {
        iPreviousNode = 0;
        iNextNode = spCurrentPath->GetVirtualNodeCount() - 1;
        if (pCurrentPathHandler) {
            delete pCurrentPathHandler;
        }
        VirtualActorPathHandler*
            pPathHandler = new VirtualActorPathHandler(pActor, this, spCurrentPath, spCurrentRequest.m_pObject);
        pCurrentPathHandler = pPathHandler;
    }
    ActorPathHandler::PATH_HANDLER_RESULT eResult = pCurrentPathHandler->Init(0.0f, afDelta);
    HandleMovementResult(eResult);
}

void ActorMover::UpdateMovement(float fdeltaT) {
    if (!bInDialog) {
        ActorPathHandler::PATH_HANDLER_RESULT eResult = pCurrentPathHandler->Update(fdeltaT);
        HandleMovementResult(eResult);
    }
}

void ActorMover::HandleProcessLevelChange() {
    if (!spCurrentPath.m_pObject || !spCurrentRequest.m_pObject) {
        ActorPathHandler* pCurrentPathHandler = this->pCurrentPathHandler;
        if (pCurrentPathHandler) {
            delete pCurrentPathHandler;
        }
        this->pCurrentPathHandler = nullptr;
        eState = ACTOR_MOVER_STATE_FINISHED;
        SetPathingFailed(false);
        return;
    }
    if (this->pActor->GetCurrentProcessType() == PROCESS_TYPE_HIGH) {
        uint32_t uiCurrentNode;
        if (this->pCurrentPathHandler->IsDetailedPathHandler()) {
            uiCurrentNode = this->iPreviousNode + 1;
        }
        else if (this->pCurrentPathHandler->GetCurrentNodeIndex()) {
            uiCurrentNode = this->pCurrentPathHandler->GetCurrentNodeIndex() - 1;
        }
        else {
            uiCurrentNode = 0;
        }
        if (uiCurrentNode >= spCurrentPath->GetVirtualNodeCount() - 1) {
            if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                VirtualPathingNode* pLastVirtualNode = spCurrentPath->GetLastVirtualNode();
                NiPoint3 kLocation = pLastVirtualNode->Location.GetLocation();
                Actor* pActor = this->pActor;
                TaskQueueInterface::GetInstance()->QueueActorSetPosition(pActor, kLocation);
            }
            else {
                VirtualPathingNode* pLastVirtualNode = spCurrentPath->GetLastVirtualNode();
                this->pActor->SetPosition(pLastVirtualNode->Location.GetLocation());
            }
            FinishMoving(true);
            return;
        }
        spCurrentRequest->SetActorLockData(this->pActor);
        Pathing::UPDATE_DETAILED_PATH_RESULT eUpdated = Pathing::UpdateSolutionMoveToHigh(
            spCurrentRequest.m_pObject,
            spCurrentPath.m_pObject,
            uiCurrentNode
        );
        switch (eUpdated) {
            case Pathing::CREATED_DETAILED_PATH:
                this->iPreviousNode = spCurrentPath->iFirstLoadedVirtualNodeIndex;
                iNextNode = spCurrentPath->iLastLoadedVirtualNodeIndex;
                break;
            case Pathing::FAILED:
            case Pathing::NO_CHANGE:
                if (spCurrentPath->IsIncomplete() &&
                    spCurrentRequest->bAllowIncompletePath &&
                    spCurrentPath->GetNodeCount()) {
                    this->iPreviousNode = spCurrentPath->iFirstLoadedVirtualNodeIndex;
                    iNextNode = spCurrentPath->iLastLoadedVirtualNodeIndex;
                }
                else {
                    this->pActor->MoveToSafePoint(true);
                    eState = ACTOR_MOVER_STATE_FAILED_PROCESS_UPDATE;
                    SetPathingFailed(true);
                    return;
                }
                break;
            default: break;
        }
        if (this->iPreviousNode == iNextNode) {
            FinishMoving(true);
        }
        else {
            float fPercentTraveled = 0.0f;
            if (!this->pCurrentPathHandler->IsDetailedPathHandler()) {
                uint32_t uiFirstLoadedVirtualNodeIndex = spCurrentPath->iFirstLoadedVirtualNodeIndex;
                VirtualPathingNode* pFirstLoadedVirtualNode = spCurrentPath->GetVirtualNode(
                    uiFirstLoadedVirtualNodeIndex
                );
                NiPoint3 kFirstLoadedVirtualNodeLoc = pFirstLoadedVirtualNode->Location.GetLocation();
                uint32_t uiLastLoadedVirtualNodeIndex = spCurrentPath->iLastLoadedVirtualNodeIndex;
                VirtualPathingNode* pLastLoadedVirtualNode = spCurrentPath->GetVirtualNode(
                    uiLastLoadedVirtualNodeIndex
                );
                NiPoint3 kLastLoadedVirtualNodeLoc = pLastLoadedVirtualNode->Location.GetLocation();
                float fLoadedPathDistance = (kLastLoadedVirtualNodeLoc - kFirstLoadedVirtualNodeLoc).Length();
                double dDistanceTraveled = this->pCurrentPathHandler->GetDistanceTraveled();
                fPercentTraveled = dDistanceTraveled / fLoadedPathDistance;
            }
            ActorPathHandler* pOldPathHandler = this->pCurrentPathHandler;
            if (pOldPathHandler) {
                delete pOldPathHandler;
            }
            DetailedActorPathHandler* pPathHandler = new DetailedActorPathHandler(
                this->pActor,
                this,
                spCurrentPath,
                spCurrentRequest.m_pObject
            );
            this->pCurrentPathHandler = pPathHandler;
            ActorPathHandler::PATH_HANDLER_RESULT eResult = this->pCurrentPathHandler->Init(fPercentTraveled, 0.0f);
            if (eResult == ActorPathHandler::PATH_HANDLER_RESULT_DONE ||
                eResult == ActorPathHandler::PATH_HANDLER_RESULT_DONE_SLEEP_SPECIAL) {
                HandleMovementResult(eResult);
                FinishMoving(true);
            }
        }
        return;
    }
    if (!this->pCurrentPathHandler->IsDetailedPathHandler()) {
        return;
    }
    int32_t iClosestVirtualNode;
    if (iNextNode == this->iPreviousNode) {
        iClosestVirtualNode = iNextNode + 1;
    }
    else {
        float fDist = this->pCurrentPathHandler->GetPercentTraveled();
        float fNodesTraveled = static_cast<double>(iNextNode - this->iPreviousNode) * fDist;
        iClosestVirtualNode = static_cast<int32_t>(std::ceilf(fNodesTraveled)) + this->iPreviousNode;
    }
    if (iClosestVirtualNode > spCurrentPath->GetVirtualNodeCount() - 1) {
        iClosestVirtualNode = spCurrentPath->GetVirtualNodeCount() - 1;
    }
    spCurrentRequest->SetActorLockData(this->pActor);
    Pathing::UPDATE_DETAILED_PATH_RESULT eUpdated = Pathing::UpdateSolutionMoveToLow(
        spCurrentRequest.m_pObject,
        spCurrentPath.m_pObject
    );
    if (eUpdated >= Pathing::FAILED) {
        if (eUpdated <= Pathing::NO_CHANGE) {
            eState = ACTOR_MOVER_STATE_FAILED;
            SetPathingFailed(true);
            return;
        }
        if (eUpdated == Pathing::CLEARED_DETAILED_PATH) {
            this->iPreviousNode = iClosestVirtualNode;
            iNextNode = spCurrentPath->GetVirtualNodeCount() - 1;
        }
    }
    ActorPathHandler* pOldPathHandler = this->pCurrentPathHandler;
    if (pOldPathHandler) {
        delete pOldPathHandler;
    }
    VirtualActorPathHandler*
        pPathHandler = new VirtualActorPathHandler(this->pActor, this, spCurrentPath, spCurrentRequest.m_pObject);
    this->pCurrentPathHandler = pPathHandler;
    this->pCurrentPathHandler->Init(static_cast<float>(this->iPreviousNode), 0.0f);
}

void ActorMover::SetCurrentFrameMoveModeAndDelta() {
    NiPoint3 kMoveDelta;
    bool bValid = false;
    uint32_t uiMoveMode = 0;
    kMoveDelta = NiPoint3::ZERO;
    if (bForceMoveMode) {
        uiMoveMode = uiForcedMoveMode;
        Animation* pAnim = pActor->GetAnimation();
        if (pAnim) {
            if (pActor->GetNoCollision()) {
                pAnim->GetDelta(&kMoveDelta, pActor, true, false);
            }
            else {
                pAnim->GetDelta(&kMoveDelta, pActor, false, true);
            }
        }
        else if (!pActor->IsVisible(7)) {
            double dCurrentSpeed = pActor->GetCurrentSpeed();
            float fForward = appTimer.fDelta * dCurrentSpeed;
            kMoveDelta = NiPoint3(0.0f, fForward, 0.0f);
        }
        bValid = true;
    }
    else if (pCurrentPathHandler) {
        uiMoveMode = pCurrentPathHandler->GetMoveMode();
        bValid = pCurrentPathHandler->GetMoveDelta(kMoveDelta);
        if (bStopCurrentMoveAnim) {
            uiMoveMode &= ~static_cast<ActorMover::MovementFlags>(
                ActorMover::MF_FORWARD | ActorMover::MF_BACKWARD | ActorMover::MF_LEFT | ActorMover::MF_RIGHT
            );
            kMoveDelta = NiPoint3::ZERO;
            bValid = true;
            ANIMATION_ACTION eAnimAction = pActor->GetAnimAction();
            if (eAnimAction < ANIM_ACTION_WAIT_FOR_LOWER_BODY_ANIM || eAnimAction > ANIM_ACTION_FORCE_SCRIPT_ANIM) {
                Animation* pAnim = pActor->GetAnimation();
                if (!pAnim || !pAnim->ShouldBeMoving()) {
                    bStopCurrentMoveAnim = false;
                }
            }
            else {
                bStopCurrentMoveAnim = false;
                bValid = false;
            }
        }
    }
    if ((uiMoveMode & ActorMover::MF_RUNNING) != 0 && !pActor->CanRun()) {
        uiMoveMode &= ~ActorMover::MF_RUNNING;
        uiMoveMode |= ActorMover::MF_WALKING;
    }
    if ((uiMoveMode & ActorMover::MF_SNEAKING) != 0 && !pActor->CanSneak()) {
        uiMoveMode &= ~ActorMover::MF_SNEAKING;
    }
    if ((uiMoveMode & ActorMover::MF_SNEAKING) != (iCachedLastValidMoveMode & ActorMover::MF_SNEAKING)) {
        Animation* pAnim = pActor->GetAnimation();
        if (!pAnim ||
            (pAnim->GetCurrentSequence(AGS_IDLE) &&
             pAnim->GetCurrentSequence(AGS_IDLE)->GetState() != NiControllerSequence::kAnimating) ||
            (pAnim->GetCurrentSequence(AGS_MOVEMENT) &&
             pAnim->GetCurrentSequence(AGS_MOVEMENT)->GetState() != NiControllerSequence::kAnimating) ||
            (pAnim->GetCurrentSequence(AGS_WEAPON) &&
             pAnim->GetCurrentSequence(AGS_WEAPON)->GetState() != NiControllerSequence::kAnimating) ||
            pActor->GetAnimAction() != ANIM_ACTION_NONE) {
            uiMoveMode &= ~ActorMover::MF_SNEAKING;
            uiMoveMode |= iCachedLastValidMoveMode & ActorMover::MF_SNEAKING;
        }
    }
    iCachedLastValidMoveMode = uiMoveMode;
    CachedMoveDelta = kMoveDelta;
    bHaveValidDeltaThisFrame = bValid;
    DebugOnEvent();
}

NiPoint3 ActorMover::GetNextNodeLocation() {
    if (spCurrentPath.m_pObject && pCurrentPathHandler) {
        return pCurrentPathHandler->GetNextNodeLocation();
    }

    return pActor->GetLocationOnReference();
}

bool ActorMover::GetCurrentGoal(NiPoint3* apRetLocation) {
    if (!spCurrentPath.m_pObject) {
        return false;
    }
    if (!spCurrentPath->IsValid()) {
        return false;
    }
    VirtualPathingNode* pLastVirtualNode = spCurrentPath->GetLastVirtualNode();
    if (!pLastVirtualNode) {
        return false;
    }
    *apRetLocation = pLastVirtualNode->Location.PathingLocation::GetLocation();
    return true;
}

void ActorMover::WarpToNextNode() {
    if (pCurrentPathHandler && pCurrentPathHandler->spPath && pCurrentPathHandler->WarpToNextNode()) {
        DebugOnEvent();
        return;
    }
    char cWarp = 0;
    if (spCurrentRequest.m_pObject) {
        if (pActor->IsFollowAccompanyPackageTarget(pPlayer)) {
            if (pCurrentPathHandler && spCurrentPath && !pCurrentPathHandler->IsDetailedPathHandler()) {
                if (!spCurrentPath->IsIncomplete()) {
                    pActor->InitiateMovementBlockedPackage();
                    return;
                }
            }

            const NiPoint3& kPlayerLocation = pPlayer->GetLocationOnReference();
            const NiPoint3& kActorLocation = pActor->GetLocationOnReference();
            double dDistance = (kActorLocation - kPlayerLocation).XYLength();
            double dTargetRadius = spCurrentRequest->GetTargetRadius();
            if (dTargetRadius + dTargetRadius < dDistance) {
                cWarp = 1;
            }
        }
        if (pActor->GetCurrentProcessType() == PROCESS_TYPE_HIGH) {
            TESObjectCELL* pOriginCell = spCurrentRequest->GetOrigin().GetCell();
            TESObjectCELL* pDestinationCell = spCurrentRequest->GetDestination().GetCell();
            if (pOriginCell && pDestinationCell) {
                NiPoint3 kOriginLocation = spCurrentRequest->GetOrigin().GetLocation();
                double dOriginWaterHeight = TES::pInstance->GetWaterHeight(kOriginLocation, pOriginCell);
                char cOriginUnderWater = spCurrentRequest->GetOrigin().GetLocation().z < dOriginWaterHeight;
                NiPoint3 kDestinationLocation = spCurrentRequest->GetDestination().GetLocation();
                double dDestinationWaterHeight = TES::pInstance->GetWaterHeight(kDestinationLocation, pDestinationCell);
                char cDestinationUnderWater = spCurrentRequest->GetDestination().GetLocation().z <
                    dDestinationWaterHeight;
                if (cOriginUnderWater != cDestinationUnderWater) {
                    cWarp = 1;
                }
            }

            PathingLocation kLocation(pActor);
            kLocation.ResolveNavMeshInfo(nullptr);
            if (!kLocation.HasNavMeshData()) {
                cWarp = 1;
            }
        }
        if (cWarp) {
            NiPoint3 kWarpLocation;
            PathingRequest kRequest;
            kRequest.SetOrigin(spCurrentRequest->GetDestination());
            {
                PathingLocation kActorLocation(pActor);
                kRequest.SetDestination(kActorLocation);
            }
            kRequest.SetTargetRadius(10.0f);
            float fActorRadius = pActor->GetWidth() * 0.6000000238418579;
            kRequest.SetActorRadius(fActorRadius);
            kRequest.SetActorLockData(pActor);
            kRequest.SetAllowIncompletePath(true);
            kRequest.SetIgnoreSmoothingIfFails(true);
            kRequest.SetCantOpenDoors(false);
            kRequest.SetCanSwim(pActor->GetActorBase()->DoesSwim());
            TESCreature* pCreature = static_cast<TESCreature*>(pActor->GetActorBase());
            kRequest.SetCanFly(pCreature->DoesFly());
            kRequest.SetGoDirectIfPathingFails(true);
            PathingSolution kSolution;
            if (Pathing::BuildPath(&kRequest, &kSolution)) {
                if (kSolution.GetNodeCount() >= 2) {
                    float fStopShort = 128.0f;
                    if (pActor->GetWidth() * 0.5 > 100.0) {
                        fStopShort = 256.0f;
                    }
                    float fPathLength = kSolution.ComputeDetailedPathLength();
                    float fWarpDistance = fPathLength - fStopShort;
                    if (fWarpDistance < 0.0) {
                        fWarpDistance = fPathLength * 0.5;
                    }
                    kWarpLocation = kSolution.ComputeApproxLocationForDistance(0.0, fWarpDistance);
                }
                else {
                    return;
                }
            }
            else {
                if (!kSolution.GetNodeCount()) {
                    return;
                }
                PathingNode* pLastNode;
                pLastNode = kSolution.GetLastNode();
                kWarpLocation = pLastNode->GetLocation().GetLocation();
            }
            DebugOnEvent();
            if (!pActor->IsFollowAccompanyPackageTarget(pPlayer) ||
                pActor->GetWorldSpace() == pPlayer->GetWorldSpace()) {
                MoveActorToLocation(kWarpLocation);
            }
            else {
                TESWorldSpace* pWorldSpace = pPlayer->GetWorldSpace();
                TESObjectCELL* pParentCell = pPlayer->GetParentCell();
                TESObjectREFR::MoveRefToNewSpace(pActor, pParentCell, pWorldSpace);
                pActor->SetPosition(kWarpLocation);
            }
        }
    }
}

void ActorMover::HandleMovementResult(ActorPathHandler::PATH_HANDLER_RESULT aeResult) {
    switch (aeResult) {
        case ActorPathHandler::PATH_HANDLER_RESULT_CANT_MOVE:
        case ActorPathHandler::PATH_HANDLER_RESULT_CANT_REACH_Z:
        case ActorPathHandler::PATH_HANDLER_RESULT_CANT_USE_DOOR:
        case ActorPathHandler::PATH_HANDLER_RESULT_FELL_OFF_PATH: {
            eState = ACTOR_MOVER_STATE_FAILED;
            DoStopMoving();
            SetPathingFailed(true);
            break;
        }
        case ActorPathHandler::PATH_HANDLER_RESULT_DONE:
        case ActorPathHandler::PATH_HANDLER_RESULT_DONE_SLEEP_SPECIAL: {
            ActorMover::END_OF_PATH_CHECK_RESULT endOfPathResult = EndOfCurrentPathCheck();
            if (spCurrentPath->IsIncomplete()) {
                if (endOfPathResult != ActorMover::END_OF_PATH_TELEPORT) {
                    // An incomplete path fails unless the actor stands, in XY, within the request's target radius of
                    // the last loaded virtual node. The squared distance and the radius are kept as doubles.
                    bool bFailed = true;
                    if (spCurrentPath->GetNodeCount()) {
                        const NiPoint3& rActorPos = pActor->GetLocationOnReference();
                        VirtualPathingNode* pVirtualNode = spCurrentPath->GetVirtualNode(
                            spCurrentPath->iLastLoadedVirtualNodeIndex
                        );
                        NiPoint3 nodeLocation = pVirtualNode->Location.GetLocation();
                        NiPoint3 diff = nodeLocation - rActorPos;
                        double fSqrXYDist = diff.SqrXYLength();
                        double fRadius = spCurrentRequest->fTargetRadius;
                        bFailed = !(spCurrentRequest->fTargetRadius * fRadius >= fSqrXYDist);
                    }
                    if (bFailed) {
                        eState = ACTOR_MOVER_STATE_FAILED;
                        SetPathingFailed(true);
                        ClearCurrentData();
                        DoStopMoving();
                    }
                }
            }
            else {
                if (aeResult == ActorPathHandler::PATH_HANDLER_RESULT_DONE_SLEEP_SPECIAL &&
                    eState != ACTOR_MOVER_STATE_FAILED) {
                    eState = ACTOR_MOVER_STATE_MOVING;
                }
                DoStopMoving();
            }
            break;
        }
        default: {
            return;
        }
    }
}

void ActorMover::BuildRequest(
    Actor* apActor,
    PathingRequest* apRequest,
    const NiPoint3& aGoalLocation,
    TESObjectCELL* apCell,
    TESWorldSpace* apWorldSpace,
    float afTargetRadius,
    PathingAvoidNodeArray* apAvoidNodes
) {
    PathingLocation dest(&aGoalLocation, apCell, apWorldSpace);
    apRequest->Initialize(apActor);
    apRequest->SetDestination(dest);
    if (!(afTargetRadius <= 0.0)) {
        apRequest->SetTargetRadius(afTargetRadius);
    }
    else {
        apRequest->SetTargetRadius(PathingRequest::GetDefaultRadius());
    }
    apRequest->SetAvoidNodeArray(apAvoidNodes);
    if (apActor->IsCreature()) {
        TESActorBaseData* pBaseData = static_cast<TESActorBaseData*>(apActor->GetActorBase());
        apRequest->SetCantOpenDoors(pBaseData->GetCantOpenDoors());
    }
    bool bDoesSwim = apActor->GetActorBase()->DoesSwim();
    apRequest->SetCanSwim(bDoesSwim);
    bool bDoesFly = static_cast<TESCreature*>(apActor->GetActorBase())->TESCreature::DoesFly();
    apRequest->SetCanFly(bDoesFly);
}
