#include "Fallout Shared/Pathfinding/PathManager.h"

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSPerformanceTimer.h"
#include "BSCore/BSReadWriteLock.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSpinLock.h"
#include "BSCore/BSTFreeList.h"
#include "BSCore/BSTStaticFreeList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSMain/ThreadSafeStructures.h"
#include "BSMain/XINISetting.h"
#include "BSSystem/BSTMessageQueue.h"
#include "BSSystem/BSTasklet.h"
#include "BSSystem/BSTaskletData.h"
#include "BSSystem/BSTaskletGroup.h"
#include "BSSystem/BSTaskletGroupData.h"
#include "BSSystem/BSTaskletManager.h"
#include "BSSystem/Win32/BSWin32TaskletData.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Pathfinding/ActorPathingMessage.h"
#include "Fallout Shared/Pathfinding/NavMeshEdgeLocation.h"
#include "Fallout Shared/Pathfinding/NavMeshRender.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Pathfinding/PathingSearchRayCast.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameSubBuffer.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"

// The game settings this file defines, in their initialization order.
SettingT<INISettingCollection> bBackgroundPathing("bBackgroundPathing:Pathfinding", true);
MessagePool_t ActorPathingMessageQueue::MessagePool;

PathManager::PathManager() : pImpl(nullptr) {}

PathManager::~PathManager() {
    delete pImpl;
}

void PathManager::Release() {
    pImpl->Release();
    delete pImpl;
    pImpl = nullptr;
}

void PathManager::ClearInventoryPointersFromCurrentRequests(InventoryChanges* apInventory) {
    pImpl->ClearInventoryPointersFromCurrentRequests(apInventory);
}

void PathManager::Update() {
    pImpl->Update();
}

void PathManager::Pause() {
    pImpl->Pause();
}

bool PathManager::QPaused() {
    return pImpl->QPaused();
}

void PathManager::Resume() {
    pImpl->Resume();
}

bool PathManager::QIdle() {
    return pImpl->QIdle();
}

void PathManager::CancelCurrentPathsAndPause() {
    pImpl->CancelCurrentPathsAndPause();
}

bool PathManager::BuildPath(
    NiPointer<PathingRequest> aspRequest,
    NiPointer<ActorPathingMessageQueue> aspMessageQueue,
    bool abQueue
) {
    if (bBackgroundPathing.Bool() && abQueue) {
        // The implementation's BuildPath takes both smart pointers by value, and the callee releases them.
        return pImpl->BuildPath(aspRequest, aspMessageQueue);
    }
    PathingSolution* pSolution = new PathingSolution;
    NiPointer<PathingSolution> spSolution(pSolution);
    if (Pathing::BuildPath(aspRequest.m_pObject, spSolution.m_pObject)) {
        ActorPathingMessageQueue* pQueue = aspMessageQueue.m_pObject;
        ActorPathingMessage kOkMessage(PMT_OK, spSolution);
        pQueue->Push(&kOkMessage);
    }
    else {
        ActorPathingMessageQueue* pQueue = aspMessageQueue.m_pObject;
        ActorPathingMessage kErrorMessage(PMT_ERROR, spSolution);
        pQueue->Push(&kErrorMessage);
    }
    return true;
}

RAYCASTPATH_RESULT PathManager::GetRayCastPath(
    PathingRequest* apRequest,
    PathingLocation& aStartLocation,
    PathingLocation& aGoalLocation,
    BSSimpleArray<PathingLocation, 1024>& aRetPath
) {
    return static_cast<RAYCASTPATH_RESULT>(pImpl->GetRayCastPath(apRequest, aStartLocation, aGoalLocation, aRetPath));
}

RAYCASTPATH_RESULT PathManager::GetRayCastPathToNavmesh(
    PathingRequest* apRequest,
    const BSSimpleArray<NavMeshEdgeLocation, 1024>& aEdges,
    PathingLocation* aGoalLocation,
    BSSimpleArray<PathingLocation, 1024>& aRetPath,
    bool abStart
) {
    return pImpl->GetRayCastPathToNavmesh(apRequest, aEdges, *aGoalLocation, aRetPath, abStart);
}

bool PathManager::DrawPath(NiPointer<NiNode>& aspNode, bool abDraw) {
    return pImpl->DrawPath(aspNode, abDraw);
}

bool PathManager::Initialize() {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_PATHING,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\PathManager.cpp",
        1386
    );
    pImpl = new PathManagerImpl;
    return pImpl->Initialize();
}

// The singleton, constructed on first use and destroyed at exit.
PathManager* PathManager::QInstance() {
    static PathManager instance;
    return &instance;
}

PathingTaskData::PathingTaskData(
    const NiPointer<PathingRequest>& aspRequest,
    NiPointer<ActorPathingMessageQueue> aspMessageQueue
)
  : bWaitingForRayCasts(false) {
    Setup(aspRequest, aspMessageQueue);
}

// Walks both arrays by the request count.
PathingTaskData::~PathingTaskData() {
    for (uint32_t uiIndex = 0; uiIndex < RayCastRequests.iSize; ++uiIndex) {
        delete RayCastRequests.GetAt(uiIndex);
        delete RayCastPaths.GetAt(uiIndex);
    }
}

void PathingTaskData::Process() {
    AutoMemContext memContext(
        MEM_CONTEXT_PATHING,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\PathManager.cpp",
        355
    );
    PathManager::QInstance();
    if (PathManager::QInstance()->pImpl->TaskTryLock()) {
        if (AllRayCastPathComputed()) {
            PathingSolution* pSolution = new PathingSolution;
            spSolution = pSolution;
            bool bBuilt = Pathing::BuildPath(spRequest.m_pObject, spSolution.m_pObject);
            if (!bWaitingForRayCasts) {
                PATHING_MESSAGE_TYPE eType = PMT_ERROR;
                if (bBuilt) {
                    if (bYielding) {
                        bYielding = false;
                    }
                    eType = PMT_OK;
                }
                ActorPathingMessageQueue* pQueue = spMessageQueue.m_pObject;
                {
                    ActorPathingMessage actorMsg(eType, spSolution);
                    pQueue->Push(&actorMsg);
                }
                PathingMessage msg(eType, this);
                PathManager::QInstance()->pImpl->SendMessageTo(msg);
            }
        }
        else {
            TaskYield();
        }
        PathManager::QInstance()->pImpl->TaskUnlock();
    }
    else {
        if (PathManager::QInstance()->pImpl->QCancelPaths()) {
            ActorPathingMessageQueue* pQueue = spMessageQueue.m_pObject;
            {
                ActorPathingMessage actorMsg(PMT_ERROR, spSolution);
                pQueue->Push(&actorMsg);
            }
            PathingMessage msg(PMT_ERROR, this);
            PathManager::QInstance()->pImpl->SendMessageTo(msg);
        }
        else {
            TaskYield();
        }
    }
    PathManager::QInstance();
}

int32_t PathingTaskData::ComputeRayCasts() {
    int32_t iTotalRayCasts = 0;
    RayCastLock.Lock(nullptr);
    for (uint32_t i = 0; i < RayCastRequests.GetSize(); ++i) {
        if (RayCastPaths.GetAt(i)->eResult == 0) {
            alignas(PathingSearchRayCast) uint8_t searchStorage[sizeof(PathingSearchRayCast)];
            PathingSearchRayCast* pSearch = ::new (searchStorage) PathingSearchRayCast;
            RayCastPath* pPath = RayCastPaths.GetAt(i);
            pSearch->BuildPath(*RayCastRequests.GetAt(i), *pPath);
            iTotalRayCasts += pSearch->GetTotalRayCasts();
            if (iTotalRayCasts > 10) {
                pSearch->~PathingSearchRayCast();
                break;
            }
            pSearch->~PathingSearchRayCast();
        }
    }
    RayCastLock.Unlock();
    return iTotalRayCasts;
}

uint8_t PathingTaskData::GetWaitingForRayCasts() {
    return bWaitingForRayCasts;
}

bool PathingTaskData::AllRayCastPathComputed() {
    RayCastLock.Lock(nullptr);
    for (RayCastPath* pPath : RayCastPaths) {
        if (!pPath->eResult) {
            RayCastLock.Unlock();
            return false;
        }
    }
    RayCastLock.Unlock();
    return true;
}

RAYCASTPATH_RESULT PathingTaskData::GetRayCastPath(
    PathingLocation& aStartLocation,
    PathingLocation& aGoalLocation,
    BSSimpleArray<PathingLocation, 1024>& aRetPath
) {
    RAYCASTPATH_RESULT eret = RCP_NONE;
    RayCastLock.Lock(nullptr);
    for (uint32_t i = 0; i < RayCastPaths.iSize; ++i) {
        if (RayCastPaths.GetAt(static_cast<int32_t>(i))->eType == RCRT_NOMESH &&
            RayCastRequests.GetAt(static_cast<int32_t>(i))->StartLocation.IsCloseTo(aStartLocation, 0.1f) &&
            RayCastRequests.GetAt(static_cast<int32_t>(i))->GoalLocation.IsCloseTo(aGoalLocation, 0.1f)) {
            eret = RayCastPaths.GetAt(static_cast<int32_t>(i))->eResult;
            if (eret) {
                if (eret == RCP_SUCCESS) {
                    RayCastPaths.GetAt(static_cast<int32_t>(i))->Nodes.CopyTo(&aRetPath);
                }
                else if (eret != RCP_FAILED) {
                    eret = RCP_FAILED;
                }
            }
            else {
                TaskYield();
            }
            RayCastLock.Unlock();
            return eret;
        }
    }
    RayCastPathRequest* prequest = new RayCastPathRequest;
    prequest->pInitialPathRequest = spRequest.m_pObject;
    prequest->StartLocation = aStartLocation;
    prequest->GoalLocation = aGoalLocation;
    prequest->eType = RCRT_NOMESH;
    RayCastPath* ppath = new RayCastPath;
    ppath->eType = prequest->eType;
    ppath->eResult = RCP_NONE;
    RayCastRequests.Add(prequest);
    RayCastPaths.Add(ppath);
    RayCastLock.Unlock();
    bWaitingForRayCasts = true;
    return eret;
}

RAYCASTPATH_RESULT PathingTaskData::GetRayCastPathToNavmesh(
    const BSSimpleArray<NavMeshEdgeLocation, 1024>& aEdges,
    PathingLocation& aGoalLocation,
    BSSimpleArray<PathingLocation, 1024>& aRetPath,
    bool abStart
) {
    RAYCASTPATH_RESULT eret = RCP_NONE;
    RayCastLock.Lock(nullptr);
    for (uint32_t i = 0; i < RayCastPaths.iSize; ++i) {
        if (RayCastPaths.GetAt(static_cast<int32_t>(i))->eType == !abStart &&
            RayCastRequests.GetAt(static_cast<int32_t>(i))->GoalLocation.IsCloseTo(aGoalLocation, 0.1f)) {
            eret = RayCastPaths.GetAt(static_cast<int32_t>(i))->eResult;
            if (eret) {
                if (eret == RCP_SUCCESS) {
                    RayCastPaths.GetAt(static_cast<int32_t>(i))->Nodes.CopyTo(&aRetPath);
                }
                else if (eret != RCP_FAILED) {
                    eret = RCP_FAILED;
                }
            }
            else {
                TaskYield();
            }
            RayCastLock.Unlock();
            return eret;
        }
    }
    RayCastPathRequest* prequest = new RayCastPathRequest;
    prequest->pInitialPathRequest = spRequest.m_pObject;
    prequest->GoalLocation = aGoalLocation;
    aEdges.CopyTo(&prequest->Edges);
    prequest->eType = static_cast<RAYCAST_REQUEST_TYPE>(!abStart);
    RayCastPath* ppath = new RayCastPath;
    ppath->eType = prequest->eType;
    ppath->eResult = RCP_NONE;
    RayCastRequests.Add(prequest);
    RayCastPaths.Add(ppath);
    RayCastLock.Unlock();
    bWaitingForRayCasts = true;
    return eret;
}

// The by-value aspMessageQueue is released where the function returns, as C++ destroys the parameter.
void PathingTaskData::Setup(
    const NiPointer<PathingRequest>& aspRequest,
    NiPointer<ActorPathingMessageQueue> aspMessageQueue
) {
    spRequest = aspRequest;
    spMessageQueue.SetPointer_(&aspMessageQueue);
}

void PathingTaskData::operator delete(void* apMemory) {
    ourFree(apMemory);
}

PathingMessage::PathingMessage() : msgType(PMT_UNDEF), pTaskData(nullptr) {}

PathingMessage::PathingMessage(PATHING_MESSAGE_TYPE aType, PathingTaskData* apTaskData)
  : msgType(aType), pTaskData(apTaskData) {}

PathingMessage& PathingMessage::operator=(const PathingMessage& aMsg) {
    msgType = aMsg.msgType;
    pTaskData = aMsg.pTaskData;
    return *this;
}

void PathingMessage::operator delete(void* apMemory) {
    ourFree(apMemory);
}

PathManagerImpl::PathManagerImpl()
  : MessageQueue(&MessagePool), eState(PATH_MANAGER_STATE_IDLE), iPauseRetryCount(0), bCancelPaths(false),
    RequestToTaskletDataMap(12u, 37u, 12u) {}

PathManagerImpl::~PathManagerImpl() {}

bool PathManagerImpl::DrawPath(NiPointer<NiNode>& aspNode, bool abDraw) {
    // The queues' Push is called through base pointers so that it is dispatched virtually; calls on the members
    // themselves would be devirtualised.
    if (abDraw) {
        BSTMessageQueue* ppathsToDraw = &PathsToDraw;
        ppathsToDraw->Push(&aspNode);
    }
    else {
        BSTMessageQueue* ppathsToRemove = &PathsToRemove;
        ppathsToRemove->Push(&aspNode);
    }
    return true;
}

RAYCASTPATH_RESULT PathManagerImpl::GetRayCastPathToNavmesh(
    PathingRequest* apRequest,
    const BSSimpleArray<NavMeshEdgeLocation, 1024>& aEdges,
    PathingLocation& aGoalLocation,
    BSSimpleArray<PathingLocation, 1024>& aRetPath,
    bool abStart
) {
    PathingTaskData* ptaskletData = nullptr;
    // GetAt is called through a pointer so that it goes through the map's vtable; a call on the member itself would be
    // devirtualised.
    LockFreeMap<PathingRequest*, PathingTaskData*>* prequestMap = &RequestToTaskletDataMap;
    if (prequestMap->GetAt(apRequest, ptaskletData)) {
        return ptaskletData->GetRayCastPathToNavmesh(aEdges, aGoalLocation, aRetPath, abStart);
    }

    return RCP_FAILED;
}

RAYCASTPATH_RESULT PathManagerImpl::GetRayCastPath(
    PathingRequest* apRequest,
    PathingLocation& aStartLocation,
    PathingLocation& aGoalLocation,
    BSSimpleArray<PathingLocation, 1024>& aRetPath
) {
    PathingTaskData* ptaskletData = nullptr;
    if (RequestToTaskletDataMap.GetAt(apRequest, ptaskletData)) {
        return ptaskletData->GetRayCastPath(aStartLocation, aGoalLocation, aRetPath);
    }

    return RCP_FAILED;
}

void PathManagerImpl::ClearInventoryPointersFromCurrentRequests(InventoryChanges* apInventory) {
    UpdateCriticalSection.Enter("PathManagerImpl::CancelCurrentPathsAndPause");
    using it_t = LockFreeMap<PathingRequest*, PathingTaskData*>::LockFreeMapIterator;
    it_t it;
    while (!it.GetFinished()) {
        PathingRequest* prequest;
        PathingTaskData* ptaskData;
        if (RequestToTaskletDataMap.GetNext(it, prequest, ptaskData, true) &&
            prequest->LockData.pInventory == apInventory) {
            prequest->LockData.pInventory = nullptr;
            prequest->LockData.pActorBase = nullptr;
        }
    }
    UpdateCriticalSection.Leave();
}

bool PathManagerImpl::BuildPath(
    NiPointer<PathingRequest> aspRequest,
    NiPointer<ActorPathingMessageQueue> aspMessageQueue
) {
    // Construct's by-value queue is a copy of aspMessageQueue.
    PathingTaskData* pTaskData = new PathingTaskData(aspRequest, aspMessageQueue);
    BSTasklet task;
    task.pData = pTaskData;
    // The map's SetAt and RemoveAt are called through its vtable; calls on the member itself would be devirtualised, so
    // they go through a pointer.
    LockFreeMap<PathingRequest*, PathingTaskData*>* prequestMap = &RequestToTaskletDataMap;
    prequestMap->SetAt(aspRequest.m_pObject, pTaskData, true);
    bool bResult;
    if (AttachTask(task, false)) {
        bResult = true;
    }
    else {
        prequestMap->RemoveAt(aspRequest.m_pObject);
        if (pTaskData) {
            delete pTaskData;
        }
        bResult = false;
    }
    return bResult;
}

void PathManagerImpl::Update() {
    UpdateCriticalSection.Enter("PathManagerImpl::Update");
    PathManagerImpl::PATH_MANAGER_STATE eState = this->eState;
    if (eState == PathManagerImpl::PATH_MANAGER_STATE_WAITING_FOR_PAUSE) {
        if (SpinLock.TryLockForWriteAndReserve()) {
            this->eState = PathManagerImpl::PATH_MANAGER_STATE_PAUSED;
        }
        else {
            ++iPauseRetryCount;
        }
    }
    else if (eState == PathManagerImpl::PATH_MANAGER_STATE_WAITING_FOR_RESUME) {
        SpinLock.UnlockWrite();
        this->eState = PathManagerImpl::PATH_MANAGER_STATE_IDLE;
        iPauseRetryCount = 0;
    }
    uint32_t itotalRayCasts = 0;
    using it_t = LockFreeMap<PathingRequest*, PathingTaskData*>::LockFreeMapIterator;
    it_t it;
    while (!it.GetFinished()) {
        PathingRequest* prequest;
        PathingTaskData* ptaskData;
        if (RequestToTaskletDataMap.GetNext(it, prequest, ptaskData, true) && ptaskData->bWaitingForRayCasts) {
            itotalRayCasts += ptaskData->ComputeRayCasts();
            if (itotalRayCasts > 10) {
                break;
            }
            if (ptaskData->AllRayCastPathComputed()) {
                ptaskData->bWaitingForRayCasts = false;
                BSTasklet task;
                task.pData = ptaskData;
                AttachTask(task, false);
            }
        }
    }
    PathingMessage msg;
    // The queues' TryPop and the map's RemoveAt are deliberately called through base pointers so they dispatch
    // virtually; calls on the members themselves would be devirtualised.
    BSTMessageQueue* pmessageQueue = &MessageQueue;
    LockFreeMap<PathingRequest*, PathingTaskData*>* prequestMap = &RequestToTaskletDataMap;
    BSTMessageQueue* ppathsToRemove = &PathsToRemove;
    BSTMessageQueue* ppathsToDraw = &PathsToDraw;
    while (pmessageQueue->TryPop(&msg)) {
        // Tests the signed range PMT_OK..PMT_ERROR: PMT_UNDEF is skipped.
        switch (msg.msgType) {
            case PMT_OK:
            case PMT_ERROR:
                prequestMap->RemoveAt(msg.pTaskData->spRequest.m_pObject);
                if (msg.pTaskData) {
                    delete msg.pTaskData;
                }
                break;
            default: break;
        }
    }
    bool bpathsChanged = false;
    NiPointer<NiAVObject> spnode(static_cast<NiAVObject*>(nullptr));
    while (ppathsToRemove->TryPop(&spnode)) {
        if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
            NiAVObject* pNode = spnode.m_pObject;
            TaskQueueInterface::GetInstance()->QueueNodeDetach(pNode);
        }
        else {
            NavMeshRender::spRootDrawNode->DetachChild(spnode.m_pObject);
        }
        bpathsChanged = true;
    }
    while (ppathsToDraw->TryPop(&spnode)) {
        if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
            NiNode* pRoot = NavMeshRender::spRootDrawNode.m_pObject;
            NiAVObject* pNode = spnode.m_pObject;
            TaskQueueInterface::GetInstance()->QueueNodeAttach(pNode, pRoot, true, false);
        }
        else {
            NavMeshRender::spRootDrawNode->AttachChild(spnode.m_pObject, 1);
        }
        bpathsChanged = true;
    }
    if (bpathsChanged) {
        if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
            NiNode* pRoot = NavMeshRender::spRootDrawNode.m_pObject;
            TaskQueueInterface::GetInstance()->QueueObjectUpdate(pRoot);
        }
        else {
            NiUpdateData updateData(0.0f, false, false);
            NavMeshRender::spRootDrawNode->Update(updateData);
        }
    }
    UpdateCriticalSection.Leave();
}

void PathManagerImpl::Pause() {
    UpdateCriticalSection.Enter("PathManagerImpl::Pause");
    switch (eState) {
        case PATH_MANAGER_STATE_IDLE:
            eState = PATH_MANAGER_STATE_WAITING_FOR_PAUSE;
            iPauseRetryCount = 0;
            break;
        case PATH_MANAGER_STATE_WAITING_FOR_RESUME: eState = PATH_MANAGER_STATE_PAUSED; break;
        default: break;
    }
    UpdateCriticalSection.Leave();
}

bool PathManagerImpl::QPaused() {
    return eState == PATH_MANAGER_STATE_PAUSED;
}

bool PathManagerImpl::QIdle() {
    return eState == 0;
}

void PathManagerImpl::Resume() {
    UpdateCriticalSection.Enter("PathManagerImpl::Resume");
    switch (eState) {
        case PATH_MANAGER_STATE_WAITING_FOR_PAUSE: eState = PATH_MANAGER_STATE_IDLE; break;
        case PATH_MANAGER_STATE_PAUSED: eState = PATH_MANAGER_STATE_WAITING_FOR_RESUME; break;
        default: break;
    }
    UpdateCriticalSection.Leave();
}

void PathManagerImpl::Release() {
    Close();
    BSTaskletManager::QInstance().DestroyTaskGroup(*this);
}

void PathManagerImpl::CancelCurrentPathsAndPause() {
    using it_t = LockFreeMap<PathingRequest*, PathingTaskData*>::LockFreeMapIterator;
    UpdateCriticalSection.Enter("PathManagerImpl::CancelCurrentPathsAndPause");
    Close();
    bCancelPaths = true;
    WaitForCompletion(false);
    PathingMessage msg;
    // The queue's TryPop and the map's RemoveAt are called through base pointers so that they stay virtual calls; calls
    // on the members themselves would be devirtualised.
    BSTMessageQueue* pmessageQueue = &MessageQueue;
    LockFreeMap<PathingRequest*, PathingTaskData*>* prequestMap = &RequestToTaskletDataMap;
    while (pmessageQueue->TryPop(&msg)) {
        // Tests the signed range PMT_OK..PMT_ERROR: PMT_UNDEF is skipped.
        switch (msg.msgType) {
            case PMT_OK:
            case PMT_ERROR:
                prequestMap->RemoveAt(msg.pTaskData->spRequest.m_pObject);
                if (msg.pTaskData) {
                    delete msg.pTaskData;
                }
                break;
            default: break;
        }
    }
    it_t it;
    while (!it.GetFinished()) {
        PathingRequest* prequest;
        PathingTaskData* ptaskData;
        if (RequestToTaskletDataMap.GetNext(it, prequest, ptaskData, true)) {
            ActorPathingMessageQueue* pQueue = ptaskData->spMessageQueue.m_pObject;
            {
                ActorPathingMessage kMessage(1u, ptaskData->spSolution);
                pQueue->Push(&kMessage);
            }
            if (ptaskData) {
                delete ptaskData;
            }
        }
    }
    RequestToTaskletDataMap.RemoveAll(false);
    SpinLock.LockForWrite();
    eState = PathManagerImpl::PATH_MANAGER_STATE_PAUSED;
    bCancelPaths = false;
    Open();
    UpdateCriticalSection.Leave();
}

bool PathManagerImpl::Initialize() {
    if (!BSTaskletManager::QInstance().CreateTaskGroup(*this)) {
        return false;
    }
    if (!BSTaskletGroup::Open()) {
        return false;
    }
    eState = PATH_MANAGER_STATE_IDLE;
    iPauseRetryCount = 0;
    bCancelPaths = false;
    return true;
}

bool PathManagerImpl::QCancelPaths() const {
    return bCancelPaths;
}

void PathManagerImpl::TaskUnlock() {
    SpinLock.UnlockRead();
}

bool PathManagerImpl::TaskTryLock() {
    return SpinLock.TryLockForRead();
}

void PathManagerImpl::SendMessageTo(const PathingMessage& aMsg) {
    // Push is called through a base pointer so that it goes through the vtable; a call on the member itself would be
    // devirtualised.
    BSTMessageQueue* pmessageQueue = &MessageQueue;
    pmessageQueue->Push(&aMsg);
}

void PathManagerImpl::operator delete(void* apMemory) {
    ourFree(apMemory);
}

ActorPathingMessage& ActorPathingMessage::operator=(const ActorPathingMessage& aMsg) {
    msgType = aMsg.msgType;
    spSolution = aMsg.spSolution;
    return *this;
}

int32_t PathingSearchRayCast::GetTotalRayCasts() {
    return iRayCastCount;
}
