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
    PathingNode* pNextNode;
    PathingLocation* v4;
    PathingNode* pNode;
    PathingLocation* v6;
    NiPoint3* v7;
    NiPoint3* v8;
    NiPoint3* location;
    NiPoint3 v12;
    NiPoint3 v13;
    NiPoint3 v14;
    if (aiNodeIndex < ((int32_t)(GetNodeCount() - 1))) {
        pNextNode = GetNode(aiNodeIndex + 1);
        v4 = &pNextNode->PathingNode::GetLocation();
        location = &(v13 = v4->GetLocation());
        pNode = GetNode(aiNodeIndex);
        v6 = &pNode->PathingNode::GetLocation();
        v7 = &(v14 = v6->GetLocation());
        v8 = &(v12 = *v7 - *location);
        return v8->Length();
    }

    return 0.0;
}

void PathingSolution::SetIncomplete(bool abIncomplete) {
    bIncompletePath = (uint8_t)((int8_t)abIncomplete);
}

VirtualPathingNode* PathingSolution::GetLastVirtualNode() {
    return &VirtualPathingNodes.GetLast();
}

int32_t PathingSolution::AddNode_Door(const NavMeshInfo* apNavMeshInfo, NiPoint3* apLocation, TESObjectREFR* apDoor) {
    uint32_t auiIndex = VirtualPathingNodes.AddUninitialized();
    VirtualPathingNode* apWhere = &VirtualPathingNodes.GetAt(auiIndex);
    VirtualPathingNode* apMemory = static_cast<VirtualPathingNode*>(::operator new(48u, apWhere));
    if (apMemory) {
        ::new (apMemory) VirtualPathingNode(apNavMeshInfo, apLocation, apDoor);
    }
    return auiIndex;
}

void PathingSolution::Save(BGSSaveFormBuffer* a2) {
    uint32_t v2;
    uint32_t v3;
    uint32_t v4;
    uint32_t FormFlags;
    a2->SaveDataEndian(&bIncompletePath, 1u, 0);
    a2->SaveDataEndian(&iFirstLoadedVirtualNodeIndex, 4u, 0);
    a2->SaveDataEndian(&iLastLoadedVirtualNodeIndex, 4u, 0);
    FormFlags = VirtualPathingNodes.iSize;
    a2->SaveVariableSizedValue(FormFlags);
    for (uint32_t aiIndex = 0; aiIndex < FormFlags; ++aiIndex) {
        VirtualPathingNode* v12 = &VirtualPathingNodes[aiIndex];
        v12->Save(a2);
    }
    // The address &CurrentPathingNodes is never null, so it is not tested.
    v4 = CurrentPathingNodes.iSize;
    a2->SaveVariableSizedValue(v4);
    for (uint32_t i = 0; i < v4; ++i) {
        PathingNode* v10 = &CurrentPathingNodes[i];
        v10->SaveGame(a2);
    }
    PathingAvoidNodeArray apMemory;
    v3 = apMemory.iSize;
    a2->SaveVariableSizedValue(v3);
    for (uint32_t auiIndex = 0; auiIndex < v3; ++auiIndex) {
        PathingAvoidNode* v8 = &apMemory.GetAt(auiIndex);
        v8->SaveGame(a2);
    }
    v2 = DoorIDs.iSize;
    a2->SaveVariableSizedValue(v2);
    for (uint32_t j = 0; j < v2; ++j) {
        a2->SaveFormID(DoorIDs[j], 0);
    }
}

ActorPathingMessageQueue::ActorPathingMessageQueue()
  : BSTCommonLLMessageQueue<ActorPathingMessage>(&MessagePool), NiRefObject() {}

ActorPathingMessageQueue::~ActorPathingMessageQueue() {}

// SetPathingFailed(false) only clears bPathingFailed: it is an initialiser here.
ActorMover::ActorMover(Actor* apActor)
  : LookAtPoint(NiPoint3_ZERO), CachedMoveDelta(NiPoint3_ZERO), spCurrentRequest(nullptr), spCurrentPath(nullptr),
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
    DetailedActorPathHandler* v2;
    void* v4;
    if ((auiPreferredMoveMode & 65280) != 0) {
        if (!pCurrentPathHandler) {
            v4 = ourMalloc(sizeof(DetailedActorPathHandler));
            if (v4) {
                v2 = ::new (v4) DetailedActorPathHandler(pActor, this);
            }
            else {
                v2 = nullptr;
            }
            pCurrentPathHandler = v2;
        }
        uiPreferredMoveMode = (auiPreferredMoveMode & 65280) | (uiPreferredMoveMode & 15);
    }
    if ((auiPreferredMoveMode & 15) != 0) {
        uiPreferredMoveMode = (uiPreferredMoveMode & 65280) | (auiPreferredMoveMode & 15);
    }
}

void ActorMover::ClearMoveModePreference(uint32_t auiPreferredMoveMode) {
    uiPreferredMoveMode &= (~auiPreferredMoveMode);
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
    BaseProcess* pprocess;
    if (bStopMovementNextUpdate) {
        bStopCurrentMoveAnim = true;
        FinishMoving(true);
        bStopMovementNextUpdate = false;
    }
    if (pActor->IsDead(false)) {
        if (pCurrentPathHandler) {
            pCurrentPathHandler->ClearMovement();
        }
        goto LABEL_45;
    }
    pprocess = pActor->pCurrentProcess;
    if ((pprocess) && (pprocess->GetFadeState() != 4)) {
        int32_t ProcessLevel = pprocess->eLevel;
        if (ProcessLevel == pActor->GetDesiredProcessLevel() &&
            (pprocess->GetSitSleepState() == SIT_SLEEP_NORMAL &&
             (!pActor->IsKnockedOut() &&
              (pprocess->GetKnockState() == KNOCK_STATE_NORMAL && !pprocess->GetCurrentlyReanimating())))) {
            if (bActorChangedProcessLevel) {
                bActorChangedProcessLevel = false;
                HandleProcessLevelChange();
            }
            if (!bPathingFailed) {
                if (!bWaitingForPath) {
                    if (spCurrentPath.m_pObject && spCurrentPath.m_pObject->IsValid()) {
                        eState = ACTOR_MOVER_STATE_MOVING;
                        UpdateMovement(fdeltaT);
                        goto LABEL_45;
                    }
                    goto LABEL_39;
                }
                ActorPathingMessage msg;
                ActorPathingMessageQueue* v3 = spMessageQueue.m_pObject;
                if (v3->TryPop(&msg)) {
                    if (msg.msgType) {
                        if (msg.msgType == 1) {
                            bWaitingForPath = 0;
                            spCurrentPath = msg.spSolution;
                            if (spCurrentPath.m_pObject &&
                                (spCurrentPath.m_pObject->IsValid() &&
                                 spCurrentRequest.m_pObject->bAllowIncompletePath)) {
                                eState = ACTOR_MOVER_STATE_MOVING;
                                DoStartMoving(fdeltaT);
                            }
                            else {
                                eState = ACTOR_MOVER_STATE_FAILED;
                                SetPathingFailed(true);
                                if (spCurrentPath.m_pObject) {
                                    uint32_t ilastLoadedVirtualNodeIndex =
                                        spCurrentPath.m_pObject->iLastLoadedVirtualNodeIndex;
                                    if ((ilastLoadedVirtualNodeIndex != -1) &&
                                        (spCurrentPath.m_pObject->GetVirtualNodeCount())) {
                                        VirtualPathingNode* plastLoadedVirtualNode =
                                            spCurrentPath.m_pObject->GetVirtualNode(ilastLoadedVirtualNodeIndex);
                                        if (plastLoadedVirtualNode) {
                                            FailedPathDestination.operator=(*&plastLoadedVirtualNode->Location);
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
                    goto LABEL_45;
                }
            }
        LABEL_39:;
            if ((!spCurrentPath.m_pObject || (!spCurrentPath.m_pObject->IsValid() || bPathingFailed)) &&
                (pCurrentPathHandler) && (pCurrentPathHandler->NoPathUpdate(fdeltaT) == 5)) {
                eState = ACTOR_MOVER_STATE_FINISHED;
            }
        }
    }

LABEL_45:;
    SetCurrentFrameMoveModeAndDelta();
    DebugOnEvent();
}

uint32_t ActorMover::GetMoveMode() {
    uint32_t uiMoveMode;
    if (bPathingFailed) {
        uiMoveMode = GetPreferredMoveModeType();
    }
    else {
        uiMoveMode = iCachedLastValidMoveMode;
    }
    return uiMoveMode;
}

// Copies and consumes the cached delta only when this frame has one.
bool ActorMover::GetMoveDelta(NiPoint3& aDelta) {
    bool result = false;
    if (bHaveValidDeltaThisFrame) {
        bHaveValidDeltaThisFrame = false;
        aDelta = CachedMoveDelta;
        result = true;
    }
    return result;
}

void ActorMover::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    uint8_t ctype;
    char v8;
    apSaveGameBuffer->SaveDataEndian(&iPreviousNode, 2u, 0);
    apSaveGameBuffer->SaveDataEndian(&iNextNode, 2u, 0);
    apSaveGameBuffer->SaveDataEndian(&bPathingFailed, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&uiPreferredMoveMode, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&bForceMoveMode, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&uiForcedMoveMode, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&bStopMovementNextUpdate, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bFaceTargetPoint, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&LookAtPoint, 12u, 0);
    apSaveGameBuffer->SaveDataEndian(&CachedMoveDelta, 12u, 0);
    apSaveGameBuffer->SaveDataEndian(&iCachedLastValidMoveMode, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&bWaitingForPath, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bActorChangedProcessLevel, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bHaveValidDeltaThisFrame, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bInDialog, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bStopCurrentMoveAnim, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&eState, 4u, 0);
    uint32_t uiTimeSinceLastFailure = appTimer.iLastTime - iLastFailureTimeStamp;
    apSaveGameBuffer->SaveDataEndian(&uiTimeSinceLastFailure, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&iContinuedFailureCount, 4u, 0);
    FailedPathDestination.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveFormID(DetectionDoorID, 0);
    v8 = 0;
    if (spCurrentRequest.m_pObject) {
        v8 |= 1u;
    }
    if (spCurrentPath.m_pObject) {
        v8 |= 2u;
    }
    if (pCurrentPathHandler) {
        bool v2 = pCurrentPathHandler->IsDetailedPathHandler();
        v8 |= v2 ? 8 : 4;
    }
    apSaveGameBuffer->SaveDataEndian(&v8, 1u, 0);
    if (spCurrentRequest.m_pObject) {
        PathingRequest* v4 = spCurrentRequest.m_pObject;
        ctype = v4->GetType();
        apSaveGameBuffer->SaveDataEndian(&ctype, 1u, 0);
        PathingRequest* v3 = spCurrentRequest.m_pObject;
        v3->SaveGame(apSaveGameBuffer);
    }
    if (spCurrentPath.m_pObject) {
        spCurrentPath.m_pObject->Save(apSaveGameBuffer);
    }
    if (pCurrentPathHandler) {
        pCurrentPathHandler->SaveGame(apSaveGameBuffer);
    }
}

void ActorMover::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    uint8_t ctype;
    char v19;
    apLoadGameBuffer->LoadData(&iPreviousNode, 2u);
    apLoadGameBuffer->LoadData(&iNextNode, 2u);
    apLoadGameBuffer->LoadData(&bPathingFailed, 1u);
    apLoadGameBuffer->LoadData(&uiPreferredMoveMode, 4u);
    apLoadGameBuffer->LoadData(&bForceMoveMode, 1u);
    apLoadGameBuffer->LoadData(&uiForcedMoveMode, 4u);
    apLoadGameBuffer->LoadData(&bStopMovementNextUpdate, 1u);
    apLoadGameBuffer->LoadData(&bFaceTargetPoint, 1u);
    apLoadGameBuffer->LoadData(&LookAtPoint, 12u);
    apLoadGameBuffer->LoadData(&CachedMoveDelta, 12u);
    apLoadGameBuffer->LoadData(&iCachedLastValidMoveMode, 4u);
    apLoadGameBuffer->LoadData(&bWaitingForPath, 1u);
    apLoadGameBuffer->LoadData(&bActorChangedProcessLevel, 1u);
    apLoadGameBuffer->LoadData(&bHaveValidDeltaThisFrame, 1u);
    apLoadGameBuffer->LoadData(&bInDialog, 1u);
    apLoadGameBuffer->LoadData(&bStopCurrentMoveAnim, 1u);
    apLoadGameBuffer->LoadData(&eState, 4u);
    if (apLoadGameBuffer->GetVersion() >= 14u) {
        uint32_t uiTimeSinceLastFailure = 0;
        apLoadGameBuffer->LoadData(&uiTimeSinceLastFailure, 4u);
        uint32_t Time = appTimer.iLastTime;
        if (uiTimeSinceLastFailure < Time) {
            uint32_t v3 = appTimer.iLastTime;
            iLastFailureTimeStamp = v3 - uiTimeSinceLastFailure;
        }
        apLoadGameBuffer->LoadData(&iContinuedFailureCount, 4u);
    }
    if (apLoadGameBuffer->GetVersion() >= 7u) {
        FailedPathDestination.LoadGame(apLoadGameBuffer);
    }
    apLoadGameBuffer->LoadFormID(DetectionDoorID);
    v19 = 0;
    apLoadGameBuffer->LoadData(&v19, 1u);
    if ((v19 & 1) != 0) {
        ctype = 0;
        apLoadGameBuffer->LoadData(&ctype, 1u);
        spCurrentRequest = PathingRequest::CreatePathingRequest(static_cast<PATHING_REQUEST_TYPE>(ctype));
        PathingRequest* v11 = spCurrentRequest.m_pObject;
        v11->LoadGame(apLoadGameBuffer);
        if ((pActor) && (pActor->pCurrentProcess) &&
            (!pActor->pCurrentProcess->CurrentPackage.GetFlag(ActorPackage::SAVELOAD_SHARED_PACKAGE))) {
            spCurrentRequest.m_pObject->SetActorLockData(pActor);
        }
    }
    char v20 = 0;
    if ((v19 & 2) != 0) {
        PathingSolution* v10;
        v10 = new PathingSolution;
        spCurrentPath = v10;
        spCurrentPath.m_pObject->LoadGame(apLoadGameBuffer);
        for (uint32_t i = 0; i < spCurrentPath.m_pObject->GetVirtualNodeCount(); ++i) {
            VirtualPathingNode* VirtualNode = spCurrentPath.m_pObject->GetVirtualNode(i);
            if (VirtualNode->HasNavMesh()) {
                VirtualPathingNode* v5 = spCurrentPath.m_pObject->GetVirtualNode(i);
                if (!(&v5->Location)->GetBestNavMeshInfo()) {
                    int32_t CastingType = pActor->iFormID;
                    const char* v6 = pActor->GetFormEditorID();
                    Warning(
                        "PATHFINDING: Actor %s (0x%08x) is using a stale path (the navmesh changed), forcing a "
                        "failure",
                        v6, CastingType
                    );
                    v20 = 1;
                    break;
                }
            }
        }
    }
    if ((v19 & 4) != 0) {
        VirtualActorPathHandler* v9;
        VirtualActorPathHandler* apMemory =
            static_cast<VirtualActorPathHandler*>(ourMalloc(sizeof(VirtualActorPathHandler)));
        if (apMemory) {
            v9 = ::new (apMemory) VirtualActorPathHandler(pActor, this, spCurrentPath, spCurrentRequest.m_pObject);
        }
        else {
            v9 = nullptr;
        }
        pCurrentPathHandler = (ActorPathHandler*)v9;
    }
    else {
        if ((v19 & 8) != 0) {
            DetailedActorPathHandler* v8;
            DetailedActorPathHandler* v13 =
                static_cast<DetailedActorPathHandler*>(ourMalloc(sizeof(DetailedActorPathHandler)));
            if (v13) {
                v8 = ::new (v13) DetailedActorPathHandler(pActor, this, spCurrentPath, spCurrentRequest.m_pObject);
            }
            else {
                v8 = nullptr;
            }
            pCurrentPathHandler = (ActorPathHandler*)v8;
        }
    }
    if (pCurrentPathHandler) {
        pCurrentPathHandler->LoadGame(apLoadGameBuffer);
    }
    if (v20) {
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
        PathingRequest* v3 = spCurrentRequest.m_pObject;
        if (v3->GetType() == PATHING_REQUEST_LOS) {
            PathingRequestLOS* plosRequest = static_cast<PathingRequestLOS*>(spCurrentRequest.m_pObject);
            CombatController* pCombatController = pActor->GetCombatController();
            if ((pCombatController) && (pCombatController->pCombatGroup)) {
                NiPointer<PathingLOSGridMap> spLOSGridMap(pCombatController->pCombatGroup->GetSearchLOSGridMap());
                plosRequest->SetLOSGridMap(spLOSGridMap);
            }
        }
        PathingRequest* v2 = spCurrentRequest.m_pObject;
        v2->InitLoadGame(apLoadGameBuffer);
        spCurrentRequest.m_pObject->SetActorLockData(pActor);
    }
}

// Restarts a path search that was waiting when the game was saved. A line-of-sight request (type 5) without a grid
// map fails at once; otherwise the request is resubmitted with the (re-created if missing) message
// queue, and a refused request fails the move.
// KEEP: the exe copies BuildPath's by-value smart pointers into their argument slots (0x6EBF30, 0x559A40) before it
// calls PathManager::QInstance (0x9DFB37) for the receiver; C++17 evaluates the receiver QInstance() first, so src
// cannot reproduce the call order.
// The exe's cleanup-only EH frame is left out.
void ActorMover::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (!bWaitingForPath) {
        return;
    }
    if (!spCurrentRequest) {
        return;
    }
    bool bHideFailed = false;
    PathingRequest* pRequest = spCurrentRequest.operator->();
    if (pRequest->GetType() == 5) {
        PathingRequestLOS* pLOS = static_cast<PathingRequestLOS*>(spCurrentRequest.operator->());
        if (!pLOS->spLOSGridMap.m_pObject) {
            bHideFailed = true;
        }
    }
    if (!spMessageQueue) {
        spMessageQueue = new ActorPathingMessageQueue;
    }
    if ((!bHideFailed) && (PathManager::QInstance()->BuildPath(spCurrentRequest, spMessageQueue, true)))
    // BuildPath takes both smart pointers by value and releases them.
    {
        return;
    }

    SetPathingFailed(true);
    bWaitingForPath = false;
}

void ActorMover::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (bWaitingForPath) {
        ActorPathingMessageQueue* v2;
        v2 = new ActorPathingMessageQueue;
        spMessageQueue = v2;
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
    LookAtPoint = NiPoint3_ZERO;
    CachedMoveDelta = NiPoint3_ZERO;
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
    if (!spCurrentPath.m_pObject->IsValid()) {
        return false;
    }
    VirtualPathingNode* pnode = spCurrentPath.m_pObject->GetLastVirtualNode();
    if (!pnode) {
        return false;
    }
    *apRetLocation = *&pnode->Location;
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
            DetailedActorPathHandler* v2;
            DetailedActorPathHandler* apMemory =
                static_cast<DetailedActorPathHandler*>(ourMalloc(sizeof(DetailedActorPathHandler)));
            if (apMemory) {
                v2 = ::new (apMemory) DetailedActorPathHandler(pActor, this);
            }
            else {
                v2 = nullptr;
            }
            pCurrentPathHandler = v2;
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
    ActorMover* pMover = this;
    pMover->ClearForcedMoveMode();
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
    uint8_t result = 0;
    if (!spCurrentPath) {
        return (bool)result;
    }
    PathingSolution* v1 = (&spCurrentPath)->operator->();
    if (v1->IsValid()) {
        return true;
    }
    return (bool)result;
}

void ActorMover::SetPathingDoorReference(TESObjectREFR* apDoor) {
    uint32_t v2;
    if (apDoor) {
        v2 = apDoor->TESForm::GetFormID();
    }
    else {
        v2 = 0;
    }
    DetectionDoorID = (uint32_t)((int32_t)v2);
}

void ActorMover::FinishMoving(bool abClearHandler) {
    ACTOR_MOVER_STATE eState = this->eState;
    if (eState < ACTOR_MOVER_STATE_FAILED || eState > ACTOR_MOVER_STATE_FAILED_PROCESS_UPDATE) {
        this->eState = ACTOR_MOVER_STATE_FINISHED;
    }
    spCurrentRequest = static_cast<PathingRequest*>(nullptr);
    spCurrentPath = static_cast<PathingSolution*>(nullptr);
    if ((pCurrentPathHandler) && (abClearHandler)) {
        pCurrentPathHandler->ClearMovement();
    }

    if (bWaitingForPath) {
        ActorPathingMessageQueue* v2;
        v2 = new ActorPathingMessageQueue;
        spMessageQueue = v2;
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
        CachedMoveDelta = NiPoint3_ZERO;
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
    PathingSolution* v1;
    uint8_t result = (uint8_t)(((!pCurrentPathHandler) ||
                                ((!spCurrentPath) || ((v1 = (&spCurrentPath)->operator->()), (!v1->IsValid())))) ||
                               pCurrentPathHandler->IsWaitingForAvoidance());
    return (bool)result;
}

// KEEP: the exe copies both smart pointers into BuildPath's argument slots (the NiPointer copy constructors 0x6EBF30
// and 0x559A40, run on the slots) before it calls PathManager::QInstance (0x47D0B0, at 0x9DB890); C++17 evaluates a
// member call's object expression before its arguments, so src calls QInstance first. Copying the pointers into
// locals first would add a copy and a release the exe does not make. The exe's cleanup-only C++ EH frame is left out.
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
            PathingRequestSafeStraightLine* pRequest =
                new PathingRequestSafeStraightLine(*static_cast<const PathingRequestSafeStraightLine*>(apRequest));
            spCurrentRequest = pRequest;
            break;
        }
        case PATHING_REQUEST_OPTIMAL_LOCATION: {
            PathingRequestOptimalLocation* pRequest =
                new PathingRequestOptimalLocation(*static_cast<const PathingRequestOptimalLocation*>(apRequest));
            spCurrentRequest = pRequest;
            break;
        }
        case PATHING_REQUEST_COVERED_MOVE: {
            PathingRequestCoveredMove* pRequest =
                new PathingRequestCoveredMove(*static_cast<const PathingRequestCoveredMove*>(apRequest));
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
            pActor, spCurrentRequest.m_pObject, static_cast<DetailedActorPathHandler*>(pCurrentPathHandler)
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
        if ((pCurrentPathHandler) && (!pCurrentPathHandler->IsDetailedPathHandler())) {
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
        float fthreshold;
        if (pActor->GetCurrentProcessType() == PROCESS_TYPE_LOW &&
            (!pActor->GetParentCell() || !pActor->GetParentCell()->GetInterior())) {
            int32_t cellX = static_cast<int32_t>((&pActor->GetLocationOnReference())->x) >> 12;
            int32_t cellY = static_cast<int32_t>((&pActor->GetLocationOnReference())->y) >> 12;
            if (!cellX && !cellY) {
                // Passes each coordinate's low 16 bits, zero-extended.
                int32_t Coord = TESObjectCELL::GetCoord(static_cast<uint16_t>(cellX), static_cast<uint16_t>(cellY));
                TESWorldSpace* WorldSpace = pActor->GetWorldSpace();
                NavMeshInfoMap* NavMeshInfoMap_ = TES::pInstance->GetNavMeshInfoMap();
                if (!NavMeshInfoMap_->FindNavMeshInfoForCellKey(WorldSpace, Coord)) {
                    TESWorldSpace* a3 = (&spCurrentRequest.m_pObject->Destination)->pWorldSpace;
                    TESObjectCELL* Count = (&spCurrentRequest.m_pObject->Destination)->pCell;
                    pActor->MoveActorAndFollowers(
                        (&spCurrentRequest.m_pObject->Destination)->GetLocation(), 0.0f, Count, a3, false, true
                    );
                }
            }
        }
        if (pActor->IsVisible(7)) {
            fthreshold = 3.0f;
        }
        else {
            fthreshold = 1.5f;
        }
        float fthreshold_2 = fthreshold;
        if (spCurrentRequest.m_pObject) {
            TESObjectCELL* porCell = (&spCurrentRequest.m_pObject->Origin)->GetCell();
            TESObjectCELL* pdestCell = (&spCurrentRequest.m_pObject->Destination)->GetCell();
            if ((porCell) && (pdestCell)) {
                double WaterHeight =
                    TES::pInstance->GetWaterHeight((&spCurrentRequest.m_pObject->Origin)->GetLocation(), porCell);
                bool borUnderWater = (&spCurrentRequest.m_pObject->Origin)->GetLocation().z < WaterHeight;
                double v12 = TES::pInstance->GetWaterHeight(
                    (&spCurrentRequest.m_pObject->Destination)->GetLocation(), pdestCell
                );
                bool bdestUnderWater = (&spCurrentRequest.m_pObject->Destination)->GetLocation().z < v12;
                if (borUnderWater != bdestUnderWater) {
                    fthreshold_2 = 1.0f;
                }
            }
        }
        float ftimeSinceLastFaillure =
            static_cast<double>(appTimer.iLastTime - iLastFailureTimeStamp) * 0.0010000000474974513;
        float ftimeSinceFirstFaillure =
            static_cast<double>(appTimer.iLastTime - iFirstFailureTimeStamp) * 0.0010000000474974513;
        if (!(fthreshold_2 <= static_cast<double>(ftimeSinceLastFaillure))) {
            if (!(fthreshold_2 <= static_cast<double>(ftimeSinceFirstFaillure)) || iContinuedFailureCount < 2) {
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
    ActorMover* pMover = this;
    pMover->ClearForcedMoveMode();
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
        if (bActorChangedProcessLevel && (!pCurrentPathHandler->IsDetailedPathHandler())) {
            bActorChangedProcessLevel = false;
        }
        else {
            if (pCurrentPathHandler->IsDetailedPathHandler()) {
                bActorChangedProcessLevel = true;
            }
        }
    }
}

void ActorMover::OnMoveToHigh() {
    BaseProcess* CurrentAIProcess = pActor->GetCurrentAIProcess();
    if (CurrentAIProcess->GetSitSleepState() != SIT_SLEEP_IS_SITTING) {
        BaseProcess* v4 = pActor->GetCurrentAIProcess();
        if (v4->GetSitSleepState() != SIT_SLEEP_IS_SLEEPING) {
            BaseProcess* v3 = pActor->GetCurrentAIProcess();
            if (v3->GetSitSleepState()) {
                pActor->GetOutofFurnitureQuick();
            }
            else {
                if (pCurrentPathHandler && !IsPathingComplete()) {
                    if (bActorChangedProcessLevel && pCurrentPathHandler->IsDetailedPathHandler()) {
                        if (spCurrentPath) {
                            uint32_t iPreviousNode;
                            PathingSolution* v2;
                            if (this->iPreviousNode != iNextNode ||
                                (iPreviousNode = this->iPreviousNode,
                                 (v2 = spCurrentPath.operator->(), iPreviousNode >= v2->GetVirtualNodeCount() - 1))) {
                                bActorChangedProcessLevel = false;
                            }
                        }
                    }
                    else {
                        if (!pCurrentPathHandler->IsDetailedPathHandler()) {
                            bActorChangedProcessLevel = true;
                        }
                    }
                }
                else {
                    if (!pSaveLoadGame->GetSaveGameLoading()) {
                        pActor->MoveToSafePoint(false);
                    }
                }
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
    return (bool)((uint8_t)BackGroundBuildPath(const_cast<PathingRequest*>(apRequest)));
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
    bool v8 = SetGoal(&kRequest);
    return v8;
}

void ActorMover::ClearLookAtTarget() {
    LookAtPoint = NiPoint3_ZERO;
    bFaceTargetPoint = false;
    DebugOnEvent();
}

bool ActorMover::RequestRotateActor(float afTargetAngle, bool abForceTurn) {
    bool v8 = false;
    ClearDataForNewPathingRequest(true);
    if (pActor->GetAnimation()) {
        Animation* v3 = pActor->GetAnimation();
        if (v3->ShouldBeMoving()) {
            bStopCurrentMoveAnim = true;
        }
    }
    if (pActor->CanMove() && pActor->GetSitSleepState() == SIT_SLEEP_NORMAL) {
        if (!IsActorInLoadedArea()) {
            eState = ACTOR_MOVER_STATE_FINISHED;
            pActor->SetHeading(afTargetAngle);
            v8 = true;
        }
    }
    else {
        eState = ACTOR_MOVER_STATE_FINISHED;
        v8 = true;
    }
    if (!v8) {
        if ((pCurrentPathHandler) && (!pCurrentPathHandler->IsDetailedPathHandler())) {
            if (pCurrentPathHandler) {
                delete pCurrentPathHandler;
            }
            pCurrentPathHandler = nullptr;
        }

        if (!pCurrentPathHandler) {
            DetailedActorPathHandler* v5;
            DetailedActorPathHandler* apMemory =
                static_cast<DetailedActorPathHandler*>(ourMalloc(sizeof(DetailedActorPathHandler)));
            if (apMemory) {
                v5 = ::new (apMemory) DetailedActorPathHandler(pActor, this);
            }
            else {
                v5 = nullptr;
            }
            pCurrentPathHandler = v5;
        }
        if (pCurrentPathHandler->InitRotationOnly(afTargetAngle, abForceTurn)) {
            eState = ACTOR_MOVER_STATE_ROTATING;
            v8 = true;
        }
        else {
            eState = ACTOR_MOVER_STATE_FINISHED;
            v8 = false;
        }
    }
    DebugOnEvent();
    return v8;
}

// The by-value NiPoint3 is three dwords on the stack.
bool ActorMover::RequestRotateActor(NiPoint3 akTargetPos, bool abForceTurn) {
    NiPoint3 a1;
    const NiPoint3* v3 = &pActor->GetLocationOnReference();
    (a1 = akTargetPos - *v3);
    float afRotation = GetZAngleFromVector(a1);
    return (bool)((uint8_t)RequestRotateActor(afRotation, abForceTurn));
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
    PathingSolution* v1;
    uint8_t result = (uint8_t)((spCurrentPath && ((v1 = (&spCurrentPath)->operator->()), v1->IsValid())) &&
                               pCurrentPathHandler->ComputePathPassesThroughPlayersArea());
    return (bool)result;
}

void ActorMover::SetInDialog(bool abVal) {
    bInDialog = (bool)((uint8_t)abVal);
}

bool ActorMover::IsMovementBlocked() const {
    PathingSolution* v1;
    uint8_t result =
        (uint8_t)((pCurrentPathHandler && (spCurrentPath && ((v1 = (&spCurrentPath)->operator->()), v1->IsValid()))) &&
                  pCurrentPathHandler->IsMovementBlocked());
    return (bool)result;
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
    return spCurrentPath.m_pObject->IsIncomplete() != 0;
}

bool ActorMover::MoveActorToLocation(const NiPoint3& aNewLoc) {
    TaskQueueInterface* instance;
    TaskQueueInterface* v3;
    Actor* pActor;
    Actor* v6;
    if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
        pActor = this->pActor;
        instance = TaskQueueInterface::GetInstance();
        instance->QueueActorSetPosition(pActor, aNewLoc);
        v6 = this->pActor;
        v3 = TaskQueueInterface::GetInstance();
        v3->QueueActorMoveCharController(v6);
    }
    else {
        this->pActor->SetPosition(aNewLoc);
    }
    this->pActor->SetLocationOnReference(aNewLoc);
    TESWorldSpace* apWorld = this->pActor->GetWorldSpace();
    if (!apWorld) {
        return true;
    }
    TESObjectCELL* parentCell = this->pActor->TESObjectREFR::GetParentCell();
    if (parentCell != apWorld->GetCellFromWorldCoord(aNewLoc)) {
        TESObjectREFR::MoveRefToNewSpace(this->pActor, nullptr, apWorld);
    }
    return (bool)((uint8_t)(this->pActor->GetDesiredProcessLevel() || pPlayer->IsSleepingorResting()));
}

ActorMover::END_OF_PATH_CHECK_RESULT ActorMover::EndOfCurrentPathCheck() {
    PathingSolution* v2;
    PathingSolution* v3;
    BaseProcess* currentAIProcess;
    PathingSolution* v5;
    uint32_t iPreviousNode;
    PathingSolution* v7;
    PathingNode* lastNode;
    if (iNextNode) {
        this->iPreviousNode = iNextNode;
        v2 = (&spCurrentPath)->operator->();
        if (v2->GetNodeCount() &&
            ((v3 = (&spCurrentPath)->operator->()), ((lastNode = v3->GetLastNode()), lastNode->IsTeleportDoorNode()))) {
            if (lastNode->ActivateNode(pActor) ||
                ((currentAIProcess = pActor->MobileObject::GetCurrentAIProcess()), currentAIProcess->GetFadeState())) {
                v5 = (&spCurrentPath)->operator->();
                v5->ClearCurrentDetailedPath();
                return ActorMover::END_OF_PATH_TELEPORT;
            }

            eState = ACTOR_MOVER_STATE_FAILED;
            SetPathingFailed(true);
            DoStopMoving();
            return ActorMover::END_OF_PATH_FAILED;
        }

        iPreviousNode = (uint32_t)this->iPreviousNode;
        v7 = (&pCurrentPathHandler->spPath)->operator->();
        if (iPreviousNode >= ((uint32_t)(((int32_t)v7->GetVirtualNodeCount()) - 1))) {
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
    TESWorldSpace* WorldSpace = apReference->GetWorldSpace();
    TESObjectCELL* ParentCell = apReference->GetParentCell();
    const NiPoint3* v4 = &apReference->GetLocationOnReference();
    return SetGoal(*v4, ParentCell, WorldSpace, afTargetRadius, apAvoidNodes);
}

void ActorMover::DoStartMoving(float afDelta) {
    bool bdetailed;
    DebugOnEvent();
    if (spCurrentPath.m_pObject->GetNodeCount()) {
        if (pActor->GetCurrentProcessType()) {
            bdetailed = false;
            spCurrentRequest.m_pObject->SetActorLockData(pActor);
            Pathing::UPDATE_DETAILED_PATH_RESULT updated =
                Pathing::UpdateSolutionMoveToLow(spCurrentRequest.m_pObject, spCurrentPath.m_pObject);
            if (updated >= Pathing::FAILED && updated <= Pathing::NO_CHANGE) {
                eState = ACTOR_MOVER_STATE_FAILED;
                SetPathingFailed(true);
                return;
            }
        }
        else {
            bdetailed = true;
        }
    }
    else {
        if (pActor->GetCurrentProcessType()) {
            bdetailed = false;
        }
        else {
            bdetailed = true;
            spCurrentRequest.m_pObject->SetActorLockData(pActor);
            Pathing::UPDATE_DETAILED_PATH_RESULT v4 =
                Pathing::UpdateSolutionMoveToHigh(spCurrentRequest.m_pObject, spCurrentPath.m_pObject, 0);
            if (v4 >= Pathing::FAILED && v4 <= Pathing::NO_CHANGE) {
                pActor->MoveToSafePoint(true);
                eState = ACTOR_MOVER_STATE_FAILED;
                SetPathingFailed(true);
                return;
            }
        }
    }
    if (bdetailed) {
        DetailedActorPathHandler* v3;
        iPreviousNode = spCurrentPath.m_pObject->iFirstLoadedVirtualNodeIndex;
        iNextNode = spCurrentPath.m_pObject->iLastLoadedVirtualNodeIndex;
        ActorPathHandler* poldPathHandler = pCurrentPathHandler;
        DetailedActorPathHandler* v8 =
            static_cast<DetailedActorPathHandler*>(ourMalloc(sizeof(DetailedActorPathHandler)));
        if (v8) {
            v3 = ::new (v8) DetailedActorPathHandler(pActor, this, spCurrentPath, spCurrentRequest.m_pObject);
        }
        else {
            v3 = nullptr;
        }
        pCurrentPathHandler = (ActorPathHandler*)v3;
        pCurrentPathHandler->HandlePreviousPathTransition(poldPathHandler);
        if (poldPathHandler) {
            delete poldPathHandler;
        }
    }
    else {
        VirtualActorPathHandler* v2;
        iPreviousNode = 0;
        iNextNode = spCurrentPath.m_pObject->GetVirtualNodeCount() - 1;
        if (pCurrentPathHandler) {
            delete pCurrentPathHandler;
        }
        VirtualActorPathHandler* apMemory =
            static_cast<VirtualActorPathHandler*>(ourMalloc(sizeof(VirtualActorPathHandler)));
        if (apMemory) {
            v2 = ::new (apMemory) VirtualActorPathHandler(pActor, this, spCurrentPath, spCurrentRequest.m_pObject);
        }
        else {
            v2 = nullptr;
        }
        pCurrentPathHandler = (ActorPathHandler*)v2;
    }
    ActorPathHandler::PATH_HANDLER_RESULT eresult = pCurrentPathHandler->Init(0.0f, afDelta);
    HandleMovementResult(eresult);
}

void ActorMover::UpdateMovement(float fdeltaT) {
    if (!bInDialog) {
        ActorPathHandler::PATH_HANDLER_RESULT eresult = pCurrentPathHandler->Update(fdeltaT);
        HandleMovementResult(eresult);
    }
}

void ActorMover::HandleProcessLevelChange() {
    VirtualActorPathHandler* v17;
    NiPoint3 result;
    int32_t iclosestVirtualNode;
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
        uint32_t icurrentNode_2;
        if (this->pCurrentPathHandler->IsDetailedPathHandler()) {
            icurrentNode_2 = this->iPreviousNode + 1;
        }
        else {
            uint32_t icurrentNode;
            if (this->pCurrentPathHandler->GetCurrentNodeIndex()) {
                icurrentNode = this->pCurrentPathHandler->GetCurrentNodeIndex() - 1;
            }
            else {
                icurrentNode = 0;
            }
            icurrentNode_2 = icurrentNode;
        }
        if (icurrentNode_2 >= spCurrentPath.m_pObject->GetVirtualNodeCount() - 1) {
            if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                VirtualPathingNode* LastVirtualNode = spCurrentPath.m_pObject->GetLastVirtualNode();
                NiPoint3* afX = ::new (&result) NiPoint3((&LastVirtualNode->Location)->GetLocation());
                Actor* pActor = this->pActor;
                TaskQueueInterface* Instance = TaskQueueInterface::GetInstance();
                Instance->QueueActorSetPosition(pActor, *afX);
            }
            else {
                VirtualPathingNode* v3 = spCurrentPath.m_pObject->GetLastVirtualNode();
                this->pActor->SetPosition((&v3->Location)->GetLocation());
            }
            FinishMoving(true);
            return;
        }
        spCurrentRequest.m_pObject->SetActorLockData(this->pActor);
        Pathing::UPDATE_DETAILED_PATH_RESULT updated =
            Pathing::UpdateSolutionMoveToHigh(spCurrentRequest.m_pObject, spCurrentPath.m_pObject, icurrentNode_2);
        if (updated < Pathing::FAILED) {
            goto LABEL_43;
        }
        if (updated <= Pathing::NO_CHANGE) {
            if (!spCurrentPath.m_pObject->IsIncomplete() ||
                (!spCurrentRequest.m_pObject->bAllowIncompletePath || !spCurrentPath.m_pObject->GetNodeCount())) {
                this->pActor->MoveToSafePoint(true);
                eState = ACTOR_MOVER_STATE_FAILED_PROCESS_UPDATE;
                SetPathingFailed(true);
                return;
            }
        }
        else {
            if (updated != Pathing::CREATED_DETAILED_PATH) {
                goto LABEL_43;
            }
        }
        this->iPreviousNode = spCurrentPath.m_pObject->iFirstLoadedVirtualNodeIndex;
        iNextNode = spCurrentPath.m_pObject->iLastLoadedVirtualNodeIndex;
    LABEL_43:;
        if (this->iPreviousNode == iNextNode) {
            FinishMoving(true);
        }
        else {
            DetailedActorPathHandler* v14;
            float fpercentTraveled = 0.0f;
            if (!this->pCurrentPathHandler->IsDetailedPathHandler()) {
                uint32_t Count = spCurrentPath.m_pObject->iFirstLoadedVirtualNodeIndex;
                VirtualPathingNode* VirtualNode = spCurrentPath.m_pObject->GetVirtualNode(Count);
                NiPoint3 firstLoadedVirtualNodeLoc = (&VirtualNode->Location)->GetLocation();
                uint32_t ShaderPropertyType = spCurrentPath.m_pObject->iLastLoadedVirtualNodeIndex;
                VirtualPathingNode* v8 = spCurrentPath.m_pObject->GetVirtualNode(ShaderPropertyType);
                NiPoint3 lastLoadedVirtualNodeLoc = (&v8->Location)->GetLocation();
                float floadedPathDistance = (lastLoadedVirtualNodeLoc - firstLoadedVirtualNodeLoc).Length();
                double v10 = this->pCurrentPathHandler->GetDistanceTraveled();
                fpercentTraveled = v10 / floadedPathDistance;
            }
            ActorPathHandler* v22 = this->pCurrentPathHandler;
            if (v22) {
                delete v22;
            }
            DetailedActorPathHandler* v21 =
                static_cast<DetailedActorPathHandler*>(ourMalloc(sizeof(DetailedActorPathHandler)));
            if (v21) {
                v14 =
                    ::new (v21) DetailedActorPathHandler(this->pActor, this, spCurrentPath, spCurrentRequest.m_pObject);
            }
            else {
                v14 = nullptr;
            }
            this->pCurrentPathHandler = (ActorPathHandler*)v14;
            ActorPathHandler::PATH_HANDLER_RESULT res = this->pCurrentPathHandler->Init(fpercentTraveled, 0.0f);
            if (res == ActorPathHandler::PATH_HANDLER_RESULT_DONE ||
                res == ActorPathHandler::PATH_HANDLER_RESULT_DONE_SLEEP_SPECIAL) {
                HandleMovementResult(res);
                FinishMoving(true);
            }
        }
        return;
    }
    if (!this->pCurrentPathHandler->IsDetailedPathHandler()) {
        return;
    }
    if (iNextNode == this->iPreviousNode) {
        iclosestVirtualNode = iNextNode + 1;
    }
    else {
        float fdist = this->pCurrentPathHandler->GetPercentTraveled();
        float v19 = static_cast<double>(iNextNode - this->iPreviousNode) * fdist;
        iclosestVirtualNode = static_cast<int32_t>(std::ceilf(v19)) + this->iPreviousNode;
    }
    if (iclosestVirtualNode > spCurrentPath.m_pObject->GetVirtualNodeCount() - 1) {
        iclosestVirtualNode = spCurrentPath.m_pObject->GetVirtualNodeCount() - 1;
    }
    spCurrentRequest.m_pObject->SetActorLockData(this->pActor);
    Pathing::UPDATE_DETAILED_PATH_RESULT v18 =
        Pathing::UpdateSolutionMoveToLow(spCurrentRequest.m_pObject, spCurrentPath.m_pObject);
    if (v18 >= Pathing::FAILED) {
        if (v18 <= Pathing::NO_CHANGE) {
            eState = ACTOR_MOVER_STATE_FAILED;
            SetPathingFailed(true);
            return;
        }
        if (v18 == Pathing::CLEARED_DETAILED_PATH) {
            this->iPreviousNode = iclosestVirtualNode;
            iNextNode = spCurrentPath.m_pObject->GetVirtualNodeCount() - 1;
        }
    }
    ActorPathHandler* v28 = this->pCurrentPathHandler;
    if (v28) {
        delete v28;
    }
    void* apMemory = ourMalloc(sizeof(VirtualActorPathHandler));
    if (apMemory) {
        v17 = ::new (apMemory) VirtualActorPathHandler(this->pActor, this, spCurrentPath, spCurrentRequest.m_pObject);
    }
    else {
        v17 = nullptr;
    }
    this->pCurrentPathHandler = (ActorPathHandler*)v17;
    this->pCurrentPathHandler->Init(static_cast<float>(this->iPreviousNode), 0.0f);
}

void ActorMover::SetCurrentFrameMoveModeAndDelta() {
    NiPoint3 dst;
    NiPoint3 movedelta;
    bool bvalid = false;
    uint32_t imovemode = 0;
    movedelta = NiPoint3_ZERO;
    if (bForceMoveMode) {
        imovemode = uiForcedMoveMode;
        Animation* panim = pActor->GetAnimation();
        if (panim) {
            if (pActor->GetNoCollision()) {
                panim->GetDelta(&movedelta, pActor, true, false);
            }
            else {
                panim->GetDelta(&movedelta, pActor, false, true);
            }
        }
        else {
            if (!pActor->IsVisible(7)) {
                double CurrentSpeed = pActor->GetCurrentSpeed();
                float v5 = appTimer.fDelta * CurrentSpeed;
                movedelta = *::new (&dst) NiPoint3(0.0f, v5, 0.0f);
            }
        }
        bvalid = true;
    }
    else {
        if (pCurrentPathHandler) {
            imovemode = pCurrentPathHandler->GetMoveMode();
            bvalid = pCurrentPathHandler->GetMoveDelta(movedelta);
            if (bStopCurrentMoveAnim) {
                imovemode &= ~static_cast<ActorMover::MovementFlags>(
                    ActorMover::MF_FORWARD | ActorMover::MF_BACKWARD | ActorMover::MF_LEFT | ActorMover::MF_RIGHT
                );
                movedelta = NiPoint3_ZERO;
                bvalid = true;
                ANIMATION_ACTION AnimAction = pActor->GetAnimAction();
                if (AnimAction < ANIM_ACTION_WAIT_FOR_LOWER_BODY_ANIM || AnimAction > ANIM_ACTION_FORCE_SCRIPT_ANIM) {
                    Animation* panim_2 = pActor->GetAnimation();
                    if (!panim_2 || !panim_2->ShouldBeMoving()) {
                        bStopCurrentMoveAnim = false;
                    }
                }
                else {
                    bStopCurrentMoveAnim = false;
                    bvalid = false;
                }
            }
        }
    }
    if ((imovemode & ActorMover::MF_RUNNING) != 0 && !pActor->CanRun()) {
        imovemode &= ~ActorMover::MF_RUNNING;
        imovemode |= ActorMover::MF_WALKING;
    }
    if ((imovemode & ActorMover::MF_SNEAKING) != 0 && !pActor->CanSneak()) {
        imovemode &= ~ActorMover::MF_SNEAKING;
    }
    if ((imovemode & ActorMover::MF_SNEAKING) != (iCachedLastValidMoveMode & ActorMover::MF_SNEAKING)) {
        BSAnimGroupSequence* CurrentSequence;
        BSAnimGroupSequence* v2;
        BSAnimGroupSequence* v3;
        Animation* panim_3 = pActor->GetAnimation();
        if (!panim_3 ||
            ((panim_3->GetCurrentSequence(AGS_IDLE) &&
              (CurrentSequence = panim_3->GetCurrentSequence(AGS_IDLE),
               CurrentSequence->GetState() != NiControllerSequence::kAnimating)) ||
             ((panim_3->GetCurrentSequence(AGS_MOVEMENT) &&
               (v2 = panim_3->GetCurrentSequence(AGS_MOVEMENT), v2->GetState() != NiControllerSequence::kAnimating)) ||
              ((panim_3->GetCurrentSequence(AGS_WEAPON) &&
                (v3 = panim_3->GetCurrentSequence(AGS_WEAPON), v3->GetState() != NiControllerSequence::kAnimating)) ||
               pActor->GetAnimAction() != ANIM_ACTION_NONE)))) {
            imovemode &= ~ActorMover::MF_SNEAKING;
            imovemode |= iCachedLastValidMoveMode & ActorMover::MF_SNEAKING;
        }
    }
    iCachedLastValidMoveMode = imovemode;
    CachedMoveDelta = movedelta;
    bHaveValidDeltaThisFrame = bvalid;
    DebugOnEvent();
}

NiPoint3 ActorMover::GetNextNodeLocation() {
    if (spCurrentPath.m_pObject && pCurrentPathHandler) {
        return pCurrentPathHandler->GetNextNodeLocation();
    }

    return pActor->GetLocationOnReference();
}

bool ActorMover::GetCurrentGoal_009DE2B0(NiPoint3* apRetLocation) {
    if (!spCurrentPath.m_pObject) {
        return false;
    }
    if (!spCurrentPath.m_pObject->IsValid()) {
        return false;
    }
    VirtualPathingNode* LastVirtualNode = spCurrentPath.m_pObject->GetLastVirtualNode();
    if (!LastVirtualNode) {
        return false;
    }
    *apRetLocation = (&LastVirtualNode->Location)->PathingLocation::GetLocation();
    return true;
}

void ActorMover::WarpToNextNode() {
    NiPoint3 v43;
    NiPoint3 v45;
    NiPoint3 result;
    if (pCurrentPathHandler && (pCurrentPathHandler->spPath && pCurrentPathHandler->WarpToNextNode())) {
        DebugOnEvent();
        return;
    }
    char v59 = 0;
    if (spCurrentRequest.m_pObject) {
        if (pActor->IsFollowAccompanyPackageTarget(pPlayer)) {
            if ((pCurrentPathHandler) && (spCurrentPath) && (!pCurrentPathHandler->IsDetailedPathHandler())) {
                PathingSolution* v1 = spCurrentPath.operator->();
                if (!v1->IsIncomplete()) {
                    pActor->InitiateMovementBlockedPackage();
                    return;
                }
            }

            const NiPoint3* a2 = &pPlayer->GetLocationOnReference();
            const NiPoint3* v2 = &pActor->GetLocationOnReference();
            NiPoint3* v3 = &(result = *v2 - *a2);
            double v37 = v3->XYLength();
            PathingRequest* v4 = spCurrentRequest.m_pObject;
            double TargetRadius = v4->GetTargetRadius();
            if (TargetRadius + TargetRadius < v37) {
                v59 = 1;
            }
        }
        if (pActor->GetCurrentProcessType() == PROCESS_TYPE_HIGH) {
            PathingRequest* v6 = spCurrentRequest.m_pObject;
            PathingLocation* v7 = static_cast<PathingLocation*>(&v6->GetOrigin());
            TESObjectCELL* apCell = v7->GetCell();
            PathingRequest* v8 = spCurrentRequest.m_pObject;
            PathingLocation* LocalTransform = static_cast<PathingLocation*>(&v8->GetDestination());
            TESObjectCELL* Cell = LocalTransform->GetCell();
            if ((apCell) && (Cell)) {
                NiPoint3 v42;
                NiPoint3 v44;
                PathingRequest* v10 = spCurrentRequest.m_pObject;
                PathingLocation* v11 = static_cast<PathingLocation*>(&v10->GetOrigin());
                NiPoint3* Location_ = &(v45 = v11->GetLocation());
                double WaterHeight = TES::pInstance->GetWaterHeight(*Location_, apCell);
                PathingRequest* v14 = spCurrentRequest.m_pObject;
                PathingLocation* v15 = static_cast<PathingLocation*>(&v14->GetOrigin());
                char v55 = (&(v44 = v15->GetLocation()))->z < WaterHeight;
                PathingRequest* v16 = spCurrentRequest.m_pObject;
                PathingLocation* v17 = static_cast<PathingLocation*>(&v16->GetDestination());
                NiPoint3* v18 = &(v43 = v17->GetLocation());
                double v19 = TES::pInstance->GetWaterHeight(*v18, Cell);
                PathingRequest* v20 = spCurrentRequest.m_pObject;
                PathingLocation* v21 = static_cast<PathingLocation*>(&v20->GetDestination());
                char v54 = (&(v42 = v21->GetLocation()))->z < v19;
                if (v55 != v54) {
                    v59 = 1;
                }
            }

            PathingLocation apLocation(pActor);
            apLocation.ResolveNavMeshInfo(nullptr);
            if (!apLocation.HasNavMeshData()) {
                v59 = 1;
            }
        }
        if (v59) {
            NiPoint3 v39;
            NiPoint3 ecx0;
            PathingRequest apRequest;
            PathingRequest* v22 = spCurrentRequest.m_pObject;
            PathingLocation* v23 = static_cast<PathingLocation*>(&v22->GetDestination());
            apRequest.SetOrigin(*v23);
            {
                PathingLocation apMemory(pActor);
                apRequest.SetDestination(apMemory);
            }
            apRequest.SetTargetRadius(10.0f);
            float v36 = pActor->GetWidth() * 0.6000000238418579;
            apRequest.SetActorRadius(v36);
            apRequest.SetActorLockData(pActor);
            apRequest.SetAllowIncompletePath(true);
            apRequest.SetIgnoreSmoothingIfFails(true);
            apRequest.SetCantOpenDoors(false);
            TESActorBase* ActorBase = pActor->GetActorBase();
            bool DoesSwim = ActorBase->DoesSwim();
            apRequest.SetCanSwim(DoesSwim);
            TESCreature* v26 = static_cast<TESCreature*>(pActor->GetActorBase());
            bool DoesFly = v26->DoesFly();
            apRequest.SetCanFly(DoesFly);
            apRequest.SetGoDirectIfPathingFails(true);
            TESObjectCELL* ParentCell = pPlayer->GetParentCell();
            PathingSolution apSolution;
            if (Pathing::BuildPath(&apRequest, &apSolution)) {
                if (apSolution.GetNodeCount() >= 2) {
                    NiPoint3 v40;
                    float v49 = 128.0f;
                    if (pActor->GetWidth() * 0.5 > 100.0) {
                        v49 = 256.0f;
                    }
                    float v48 = apSolution.ComputeDetailedPathLength();
                    float v47 = v48 - v49;
                    if (v47 < 0.0) {
                        v47 = v48 * 0.5;
                    }
                    ecx0 = (v40 = apSolution.ComputeApproxLocationForDistance(0.0, v47));
                    goto LABEL_34;
                }
            }
            else {
                if (apSolution.GetNodeCount()) {
                    PathingNode* LastNode;
                    PathingLocation* ActorPackageThatIsRunning;
                    TESWorldSpace* WorldSpace;
                    LastNode = apSolution.GetLastNode();
                    ActorPackageThatIsRunning = static_cast<PathingLocation*>(&LastNode->GetLocation());
                    ecx0 = (v39 = ActorPackageThatIsRunning->GetLocation());
                LABEL_34:;
                    DebugOnEvent();
                    if (!pActor->IsFollowAccompanyPackageTarget(pPlayer) ||
                        (WorldSpace = pActor->GetWorldSpace(), WorldSpace == pPlayer->GetWorldSpace())) {
                        MoveActorToLocation(ecx0);
                    }
                    else {
                        TESWorldSpace* a2c = pPlayer->GetWorldSpace();
                        TESObjectCELL* v31 = pPlayer->GetParentCell();
                        TESObjectREFR::MoveRefToNewSpace(pActor, v31, a2c);
                        pActor->SetPosition(ecx0);
                    }
                    return;
                }
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
            if (spCurrentPath.m_pObject->IsIncomplete()) {
                if (endOfPathResult != ActorMover::END_OF_PATH_TELEPORT) {
                    // An incomplete path fails unless the actor stands, in XY, within the request's target radius of
                    // the last loaded virtual node. The squared distance and the radius are kept as doubles.
                    bool bFailed = true;
                    if (spCurrentPath.m_pObject->GetNodeCount()) {
                        const NiPoint3& rActorPos = pActor->GetLocationOnReference();
                        VirtualPathingNode* pVirtualNode = spCurrentPath.m_pObject->GetVirtualNode(
                            spCurrentPath.m_pObject->iLastLoadedVirtualNodeIndex
                        );
                        NiPoint3 nodeLocation = pVirtualNode->Location.GetLocation();
                        NiPoint3 diff = nodeLocation - rActorPos;
                        double fSqrXYDist = diff.SqrXYLength();
                        double fRadius = spCurrentRequest.m_pObject->fTargetRadius;
                        bFailed = !(spCurrentRequest.m_pObject->fTargetRadius * fRadius >= fSqrXYDist);
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
    PathingLocation dest((&aGoalLocation), apCell, apWorldSpace);
    apRequest->Initialize(apActor);
    apRequest->SetDestination(dest);
    if (!(afTargetRadius <= 0.0)) {
        apRequest->SetTargetRadius(afTargetRadius);
    }
    else {
        float a2 = PathingRequest::GetDefaultRadius();
        apRequest->SetTargetRadius(a2);
    }
    apRequest->SetAvoidNodeArray(apAvoidNodes);
    if (apActor->IsCreature()) {
        TESActorBaseData* v10 = static_cast<TESActorBaseData*>(apActor->GetActorBase());
        apRequest->SetCantOpenDoors(v10->GetCantOpenDoors());
    }
    bool DoesSwim = apActor->GetActorBase()->DoesSwim();
    apRequest->SetCanSwim(DoesSwim);
    bool DoesFly = static_cast<TESCreature*>(apActor->GetActorBase())->TESCreature::DoesFly();
    apRequest->SetCanFly(DoesFly);
}
