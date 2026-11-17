#include "Fallout Shared/Pathfinding/PathManager.h"

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSPerformanceTimer.h"
#include "BSCore/BSReadWriteLock.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSpinLock.h"
#include "BSCore/BSTFreeList.h"
#include "BSCore/BSTStaticFreeList.h"
#include "BSCore/MemoryManager.h"
#include "BSSystem/BSTMessageQueue.h"
#include "BSSystem/BSTasklet.h"
#include "BSSystem/BSTaskletData.h"
#include "BSSystem/BSTaskletGroup.h"
#include "BSSystem/BSTaskletGroupData.h"
#include "BSSystem/BSTaskletManager.h"
#include "BSSystem/Win32/BSWin32TaskletData.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/LockFreeMap.h"
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

// The lock names Pause and Resume pass, overlaid on the exe's copies. Not in the PDB.
using PathManagerImpl_Pause_Name_t = const char[23];
static PathManagerImpl_Pause_Name_t PathManagerImpl_Pause_Name{"PathManagerImpl::Pause"};
using PathManagerImpl_Resume_Name_t = const char[24];
static PathManagerImpl_Resume_Name_t PathManagerImpl_Resume_Name{"PathManagerImpl::Resume"};

// The game settings this file defines (PDB), with the IDs and values the exe's initializers pass, in its initialization order.
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
	return (bool)((uint8_t)pImpl->QIdle());
}

void PathManager::CancelCurrentPathsAndPause() {
	pImpl->CancelCurrentPathsAndPause();
}

// The exe's cleanup-only C++ EH frame is left out. The by-value smart pointers are released where the function
// returns (0x45CEC0 on [ebp+8], 0x6EBF90 on [ebp+0xC]), as C++ destroys the parameters. The pointers' operator-> is
// the shared one-field getter 0x559450.
bool PathManager::BuildPath(NiPointer<PathingRequest> aspRequest, NiPointer<ActorPathingMessageQueue> aspMessageQueue,
	bool abQueue) {
	if (bBackgroundPathing.Bool() && abQueue) {
		// The implementation's BuildPath takes both smart pointers by value: the exe copies them into its argument
		// slots, the queue first, and the callee releases them.
		return pImpl->BuildPath(aspRequest, aspMessageQueue);
	}
	FNVD_INLINED_LOCAL(NiPointer<PathingSolution>, spSolution, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(ActorPathingMessage, kOkMessage, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(ActorPathingMessage, kErrorMessage, std::nullopt, std::nullopt);
	void* pMemory = NiMemObject::operator new(sizeof(PathingSolution));
	PathingSolution* pSolution;
	if (pMemory) {
		pSolution = ::new (pMemory) PathingSolution;
	} else {
		pSolution = 0;
	}
	::new (&spSolution) NiPointer<PathingSolution>(pSolution);
	if (Pathing::BuildPath(aspRequest.m_pObject, spSolution.m_pObject)) {
		ActorPathingMessageQueue* pQueue = aspMessageQueue.m_pObject;
		pQueue->Push(::new (&kOkMessage) ActorPathingMessage(PMT_OK, spSolution));
		kOkMessage.~ActorPathingMessage();
	} else {
		ActorPathingMessageQueue* pQueue = aspMessageQueue.m_pObject;
		pQueue->Push(::new (&kErrorMessage) ActorPathingMessage(PMT_ERROR, spSolution));
		kErrorMessage.~ActorPathingMessage();
	}
	spSolution.~NiPointer<PathingSolution>();
	return true;
}

RAYCASTPATH_RESULT PathManager::GetRayCastPath(PathingRequest* apRequest, PathingLocation& aStartLocation,
	PathingLocation& aGoalLocation, BSSimpleArray<PathingLocation, 1024>& aRetPath) {
	return static_cast<RAYCASTPATH_RESULT>(pImpl->GetRayCastPath(apRequest, aStartLocation, aGoalLocation, aRetPath));
}

RAYCASTPATH_RESULT PathManager::GetRayCastPathToNavmesh(PathingRequest* apRequest,
	const BSSimpleArray<NavMeshEdgeLocation, 1024>& aEdges, PathingLocation* aGoalLocation,
	BSSimpleArray<PathingLocation, 1024>& aRetPath, bool a6) {
	return pImpl->GetRayCastPathToNavmesh(apRequest, aEdges, *aGoalLocation, aRetPath, a6);
}

bool PathManager::DrawPath(NiPointer<NiNode>& aspNode, bool abDraw) {
	return pImpl->DrawPath(aspNode, abDraw);
}

// The exe's cleanup-only C++ EH frame is left out.
bool PathManager::Initialize() {
	PathManagerImpl* v2;
	FNVD_CONSTRUCTED_LOCAL(AutoMemContext, apMemory, &AutoMemContext::Destruct, MEM_CONTEXT_PATHING, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\PathManager.cpp", 1386);
	PathManagerImpl* v5 = static_cast<PathManagerImpl*>(ourMalloc(sizeof(PathManagerImpl)));
	if (v5) {
		v2 = ::new (v5) PathManagerImpl;
	} else {
		v2 = 0;
	}
	pImpl = v2;
	bool v4 = pImpl->Initialize();
	return v4;
}

// The singleton, constructed on first use (guard bit 1 of 0x11C5298); the exe's atexit thunk 0xFC8480 (the PDB's
// `PathManager::QInstance'::`2'::`dynamic atexit destructor for 'instance'') destroys it. The exe's cleanup-only C++
// EH frame is left out.
PathManager* PathManager::QInstance() {
	static PathManager instance;
	return &instance;
}

PathingTaskData::PathingTaskData(const NiPointer<PathingRequest>& aspRequest,
	NiPointer<ActorPathingMessageQueue> aspMessageQueue)
	: bWaitingForRayCasts(false) {
	Setup(aspRequest, aspMessageQueue);
}

// The exe walks both arrays by the request count.
PathingTaskData::~PathingTaskData() {
	for (uint32_t uiIndex = 0; uiIndex < RayCastRequests.iSize; ++uiIndex) {
		delete RayCastRequests.GetAt(uiIndex);
		delete RayCastPaths.GetAt(uiIndex);
	}
}

// The exe's cleanup-only C++ EH frame is left out. The exe passes PathManager::QInstance's result at the start and the
// end to 0x717E50 and the empty 0x483710 (trivial), and destroys each PathingMessage with the empty 0x483710: only
// the QInstance calls remain. Where it waits for ray casts, the exe stores 0 to a stack dword nothing reads.
void PathingTaskData::Process() {
	FNVD_CONSTRUCTED_LOCAL(AutoMemContext, memContext, &AutoMemContext::Destruct, MEM_CONTEXT_PATHING, true,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\PathManager.cpp", 355);
	PathManager::QInstance();
	PathManager* pManager = PathManager::QInstance();
	if (pManager->pImpl->TaskTryLock()) {
		if (AllRayCastPathComputed()) {
			void* pMemory = NiMemObject::operator new(sizeof(PathingSolution));
			PathingSolution* pSolution = pMemory ? ::new (pMemory) PathingSolution : nullptr;
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
					FNVD_CONSTRUCTED_LOCAL(ActorPathingMessage, actorMsg, &ActorPathingMessage::Destruct, eType,
						spSolution);
					pQueue->Push(&actorMsg);
				}
				PathingMessage msg;
				msg.~PathingMessage();
				::new (&msg) PathingMessage(eType, this);
				PathManager* pSendManager = PathManager::QInstance();
				pSendManager->pImpl->SendMessageTo(msg);
			}
		} else {
			TaskYield();
		}
		PathManager* pUnlockManager = PathManager::QInstance();
		pUnlockManager->pImpl->TaskUnlock();
	} else {
		PathManager* pCancelManager = PathManager::QInstance();
		if (pCancelManager->pImpl->QCancelPaths()) {
			ActorPathingMessageQueue* pQueue = spMessageQueue.m_pObject;
			{
				FNVD_CONSTRUCTED_LOCAL(ActorPathingMessage, actorMsg, &ActorPathingMessage::Destruct, PMT_ERROR,
					spSolution);
				pQueue->Push(&actorMsg);
			}
			PathingMessage msg;
			msg.~PathingMessage();
			::new (&msg) PathingMessage(PMT_ERROR, this);
			PathManager* pSendManager = PathManager::QInstance();
			pSendManager->pImpl->SendMessageTo(msg);
		} else {
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
	return (uint8_t)bWaitingForRayCasts;
}

bool PathingTaskData::AllRayCastPathComputed() {
	RayCastLock.Lock(0);
	for (uint32_t uiIndex = 0; uiIndex < RayCastPaths.iSize; ++uiIndex) {
		if (!RayCastPaths.GetAt(uiIndex)->eResult) {
			RayCastLock.Unlock();
			return 0;
		}
	}
	RayCastLock.Unlock();
	return 1;
}

// The exe's cleanup-only C++ EH frame is left out.
RAYCASTPATH_RESULT PathingTaskData::GetRayCastPath(PathingLocation& aStartLocation, PathingLocation& aGoalLocation,
	BSSimpleArray<PathingLocation, 1024>& aRetPath) {
	RayCastPath* ppath;
	RayCastPathRequest* prequest;
	RAYCASTPATH_RESULT eret = RCP_NONE;
	RayCastLock.Lock(0);
	for (uint32_t i = 0; i < RayCastPaths.iSize; ++i) {
		if (RayCastPaths.GetAt(static_cast<int32_t>(i))->eType == RCRT_NOMESH &&
			(RayCastRequests.GetAt(static_cast<int32_t>(i))->StartLocation.IsCloseTo(aStartLocation, 0.1f) &&
				RayCastRequests.GetAt(static_cast<int32_t>(i))->GoalLocation.IsCloseTo(aGoalLocation, 0.1f))) {
			eret = RayCastPaths.GetAt(static_cast<int32_t>(i))->eResult;
			if (eret) {
				if (eret == RCP_SUCCESS) {
					RayCastPaths.GetAt(static_cast<int32_t>(i))->Nodes.CopyTo(&aRetPath);
				} else {
					if (eret != RCP_FAILED) {
						eret = RCP_FAILED;
					}
				}
			} else {
				TaskYield();
			}
			RayCastLock.Unlock();
			return eret;
		}
	}
	RayCastPathRequest* apMemory = static_cast<RayCastPathRequest*>(ourMalloc(sizeof(RayCastPathRequest)));
	if (apMemory) {
		prequest = ::new (apMemory) RayCastPathRequest;
	} else {
		prequest = 0;
	}
	RayCastPathRequest* prequest_2 = prequest;
	prequest->pInitialPathRequest = spRequest.m_pObject;
	prequest_2->StartLocation.operator=(aStartLocation);
	prequest_2->GoalLocation.operator=(aGoalLocation);
	prequest_2->eType = RCRT_NOMESH;
	RayCastPath* v8 = static_cast<RayCastPath*>(ourMalloc(sizeof(RayCastPath)));
	if (v8) {
		ppath = ::new (v8) RayCastPath;
	} else {
		ppath = 0;
	}
	RayCastPath* ppath_2 = ppath;
	ppath->eType = prequest_2->eType;
	ppath_2->eResult = RCP_NONE;
	RayCastRequests.Add(prequest_2);
	RayCastPaths.Add(ppath_2);
	RayCastLock.Unlock();
	bWaitingForRayCasts = 1;
	return eret;
}

// The exe's cleanup-only C++ EH frame is left out.
RAYCASTPATH_RESULT PathingTaskData::GetRayCastPathToNavmesh(const BSSimpleArray<NavMeshEdgeLocation, 1024>& aEdges,
	PathingLocation& aGoalLocation, BSSimpleArray<PathingLocation, 1024>& aRetPath, bool abStart) {
	RayCastPath* ppath;
	RayCastPathRequest* prequest;
	RAYCASTPATH_RESULT eret = RCP_NONE;
	RayCastLock.Lock(0);
	for (uint32_t i = 0; i < RayCastPaths.iSize; ++i) {
		if (RayCastPaths.GetAt(static_cast<int32_t>(i))->eType == !abStart &&
			RayCastRequests.GetAt(static_cast<int32_t>(i))->GoalLocation.IsCloseTo(aGoalLocation, 0.1f)) {
			eret = RayCastPaths.GetAt(static_cast<int32_t>(i))->eResult;
			if (eret) {
				if (eret == RCP_SUCCESS) {
					RayCastPaths.GetAt(static_cast<int32_t>(i))->Nodes.CopyTo(&aRetPath);
				} else {
					if (eret != RCP_FAILED) {
						eret = RCP_FAILED;
					}
				}
			} else {
				TaskYield();
			}
			RayCastLock.Unlock();
			return eret;
		}
	}
	RayCastPathRequest* apMemory = static_cast<RayCastPathRequest*>(ourMalloc(sizeof(RayCastPathRequest)));
	if (apMemory) {
		prequest = ::new (apMemory) RayCastPathRequest;
	} else {
		prequest = 0;
	}
	RayCastPathRequest* prequest_2 = prequest;
	prequest->pInitialPathRequest = spRequest.m_pObject;
	prequest_2->GoalLocation.operator=(aGoalLocation);
	aEdges.CopyTo(&prequest_2->Edges);
	prequest_2->eType = static_cast<RAYCAST_REQUEST_TYPE>(!abStart);
	RayCastPath* v9 = static_cast<RayCastPath*>(ourMalloc(sizeof(RayCastPath)));
	if (v9) {
		ppath = ::new (v9) RayCastPath;
	} else {
		ppath = 0;
	}
	RayCastPath* ppath_2 = ppath;
	ppath->eType = prequest_2->eType;
	ppath_2->eResult = RCP_NONE;
	RayCastRequests.Add(prequest_2);
	RayCastPaths.Add(ppath_2);
	RayCastLock.Unlock();
	bWaitingForRayCasts = 1;
	return eret;
}

// The by-value aspMessageQueue is released where the function returns (the exe's 0x6EBF90 on [ebp+0xC]), as C++
// destroys the parameter.
// The exe's cleanup-only C++ EH frame is left out.
void PathingTaskData::Setup(const NiPointer<PathingRequest>& aspRequest,
	NiPointer<ActorPathingMessageQueue> aspMessageQueue) {
	spRequest.operator=(aspRequest);
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
	// The exe calls the queues' Push (slot 1) through their vtables; calls on the members themselves would be
	// devirtualised, so they go through base pointers.
	if (abDraw) {
		BSTMessageQueue* ppathsToDraw = &PathsToDraw;
		ppathsToDraw->Push(&aspNode);
	} else {
		BSTMessageQueue* ppathsToRemove = &PathsToRemove;
		ppathsToRemove->Push(&aspNode);
	}
	return 1;
}

RAYCASTPATH_RESULT PathManagerImpl::GetRayCastPathToNavmesh(PathingRequest* apRequest,
	const BSSimpleArray<NavMeshEdgeLocation, 1024>& aEdges, PathingLocation& aGoalLocation,
	BSSimpleArray<PathingLocation, 1024>& aRetPath, bool abStart) {
	PathingTaskData* ptaskletData = 0;
	// The exe calls the map's GetAt (slot 2) through its vtable; a call on the member itself would be
	// devirtualised, so it goes through a pointer.
	LockFreeMap<PathingRequest*, PathingTaskData*>* prequestMap = &RequestToTaskletDataMap;
	if (prequestMap->GetAt(apRequest, ptaskletData)) {
		return ptaskletData->GetRayCastPathToNavmesh(aEdges, aGoalLocation, aRetPath, abStart);
	} else {
		return RCP_FAILED;
	}
}

RAYCASTPATH_RESULT PathManagerImpl::GetRayCastPath(PathingRequest* apRequest, PathingLocation& aStartLocation,
	PathingLocation& aGoalLocation, BSSimpleArray<PathingLocation, 1024>& aRetPath) {
	PathingTaskData* ptaskletData = 0;
	if (RequestToTaskletDataMap.GetAt(apRequest, ptaskletData)) {
		return ptaskletData->GetRayCastPath(aStartLocation, aGoalLocation, aRetPath);
	} else {
		return RCP_FAILED;
	}
}

// The exe's cleanup-only C++ EH frame is left out. The walk's GetFinished is the shared exe copy 0x6EBDE0, which src
// defines inline in LockFreeMap.h; the request's lock data comes from the shared one-field address getter 0x6C9900.
void PathManagerImpl::ClearInventoryPointersFromCurrentRequests(InventoryChanges* apInventory) {
	UpdateCriticalSection.Enter("PathManagerImpl::CancelCurrentPathsAndPause");
	using it_t = LockFreeMap<PathingRequest*, PathingTaskData*>::LockFreeMapIterator;
	FNVD_CONSTRUCTED_LOCAL(it_t, it, &it_t::Destruct);
	while (!it.GetFinished()) {
		PathingRequest* prequest;
		PathingTaskData* ptaskData;
		if (RequestToTaskletDataMap.GetNext(it, prequest, ptaskData, 1) &&
			prequest->LockData.pInventory == apInventory) {
			prequest->LockData.pInventory = 0;
			prequest->LockData.pActorBase = 0;
		}
	}
	UpdateCriticalSection.Leave();
}

// The exe's cleanup-only C++ EH frame is left out. The by-value smart pointers are released where the function
// returns (0x45CEC0 on [ebp+8], 0x6EBF90 on [ebp+0xC]), as C++ destroys the parameters. The pointers' operator-> is
// the shared one-field getter 0x559450, the tasklet's data setter the shared 0x6ECD40.
bool PathManagerImpl::BuildPath(NiPointer<PathingRequest> aspRequest,
	NiPointer<ActorPathingMessageQueue> aspMessageQueue) {
	FNVD_INLINED_LOCAL(BSTasklet, task, std::nullopt, std::nullopt);
	void* pMemory = ourMalloc(sizeof(PathingTaskData));
	PathingTaskData* pTaskData;
	if (pMemory) {
		// Construct's by-value queue is a copy of aspMessageQueue (the exe's 0x6EBF30 into the argument slot).
		pTaskData = ::new (pMemory) PathingTaskData(aspRequest, aspMessageQueue);
	} else {
		pTaskData = 0;
	}
	::new (&task) BSTasklet;
	task.pData = pTaskData;
	// The exe calls the map's SetAt (slot 4) and RemoveAt (slot 5) through its vtable; calls on the member itself
	// would be devirtualised, so they go through a pointer.
	LockFreeMap<PathingRequest*, PathingTaskData*>* prequestMap = &RequestToTaskletDataMap;
	prequestMap->SetAt(aspRequest.m_pObject, pTaskData, true);
	bool bResult;
	if (AttachTask(task, 0)) {
		bResult = true;
	} else {
		prequestMap->RemoveAt(aspRequest.m_pObject);
		if (pTaskData) {
			delete pTaskData;
		}
		bResult = false;
	}
	task.~BSTasklet();
	return bResult;
}

// The exe's cleanup-only C++ EH frame is left out. The walk's GetFinished is the shared exe copy 0x6EBDE0, which src
// defines inline in LockFreeMap.h.
void PathManagerImpl::Update() {
	FNVD_INLINED_LOCAL(NiPointer<NiAVObject>, spnode, std::nullopt, std::nullopt);
	PathingMessage msg;
	UpdateCriticalSection.Enter("PathManagerImpl::Update");
	PathManagerImpl::PATH_MANAGER_STATE eState = this->eState;
	if (eState == PathManagerImpl::PATH_MANAGER_STATE_WAITING_FOR_PAUSE) {
		if (SpinLock.TryLockForWriteAndReserve()) {
			this->eState = PathManagerImpl::PATH_MANAGER_STATE_PAUSED;
		} else {
			++iPauseRetryCount;
		}
	} else {
		if (eState == PathManagerImpl::PATH_MANAGER_STATE_WAITING_FOR_RESUME) {
			SpinLock.UnlockWrite();
			this->eState = PathManagerImpl::PATH_MANAGER_STATE_IDLE;
			iPauseRetryCount = 0;
		}
	}
	uint32_t itotalRayCasts = 0;
	using it_t = LockFreeMap<PathingRequest*, PathingTaskData*>::LockFreeMapIterator;
	FNVD_CONSTRUCTED_LOCAL(it_t, it, &it_t::Destruct);
	while (!it.GetFinished()) {
		PathingRequest* prequest;
		PathingTaskData* ptaskData;
		if (RequestToTaskletDataMap.GetNext(it, prequest, ptaskData, 1) && ptaskData->bWaitingForRayCasts) {
			int32_t v1 = ptaskData->ComputeRayCasts();
			itotalRayCasts += v1;
			if (itotalRayCasts > 10) {
				break;
			}
			if (ptaskData->AllRayCastPathComputed()) {
				ptaskData->bWaitingForRayCasts = 0;
				FNVD_CONSTRUCTED_LOCAL(BSTasklet, task, &BSTasklet::Destruct);
				task.pData = ptaskData;
				AttachTask(task, 0);
			}
		}
	}
	msg.~PathingMessage();
	::new (&msg) PathingMessage;
	// The exe calls the queues' TryPop (slot 4) and the map's RemoveAt (slot 5) through their vtables; calls on the
	// members themselves would be devirtualised, so they go through base pointers.
	BSTMessageQueue* pmessageQueue = &MessageQueue;
	LockFreeMap<PathingRequest*, PathingTaskData*>* prequestMap = &RequestToTaskletDataMap;
	BSTMessageQueue* ppathsToRemove = &PathsToRemove;
	BSTMessageQueue* ppathsToDraw = &PathsToDraw;
	while (pmessageQueue->TryPop(&msg)) {
		// The exe tests the signed range PMT_OK..PMT_ERROR (jl 0, jle 1): PMT_UNDEF is skipped.
		switch (msg.msgType) {
		case PMT_OK:
		case PMT_ERROR:
			prequestMap->RemoveAt(msg.pTaskData->spRequest.m_pObject);
			if (msg.pTaskData) {
				delete msg.pTaskData;
			}
			break;
		default:
			break;
		}
	}
	bool bpathsChanged = 0;
	::new (&spnode) NiPointer<NiAVObject>(static_cast<NiAVObject*>(0));
	while (ppathsToRemove->TryPop(&spnode)) {
		if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
			NiAVObject* v7 = spnode.m_pObject;
			TaskQueueInterface* Instance = TaskQueueInterface::GetInstance();
			Instance->QueueNodeDetach(v7);
		} else {
			NiNode* v10 = NavMeshRender::spRootDrawNode.m_pObject;
			v10->DetachChild(spnode.m_pObject);
		}
		bpathsChanged = 1;
	}
	while (ppathsToDraw->TryPop(&spnode)) {
		if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
			NiNode* afTime = NavMeshRender::spRootDrawNode.m_pObject;
			NiAVObject* v5 = spnode.m_pObject;
			TaskQueueInterface* v3 = TaskQueueInterface::GetInstance();
			v3->QueueNodeAttach(v5, afTime, 1, 0);
		} else {
			NiNode* v9 = NavMeshRender::spRootDrawNode.m_pObject;
			v9->AttachChild(spnode.m_pObject, 1);
		}
		bpathsChanged = 1;
	}
	if (bpathsChanged) {
		if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
			NiNode* v8 = NavMeshRender::spRootDrawNode.m_pObject;
			TaskQueueInterface* v4 = TaskQueueInterface::GetInstance();
			v4->QueueObjectUpdate(v8);
		} else {
			FNVD_CONSTRUCTED_LOCAL(NiUpdateData, updateData, nullptr, 0.0f, 0, 0);
			NavMeshRender::spRootDrawNode.m_pObject->Update(updateData);
		}
	}
	UpdateCriticalSection.Leave();
	spnode.~NiPointer<NiAVObject>();
}

void PathManagerImpl::Pause() {
	UpdateCriticalSection.Enter(PathManagerImpl_Pause_Name);
	switch (eState) {
	case PATH_MANAGER_STATE_IDLE:
		eState = PATH_MANAGER_STATE_WAITING_FOR_PAUSE;
		iPauseRetryCount = 0;
		break;
	case PATH_MANAGER_STATE_WAITING_FOR_RESUME:
		eState = PATH_MANAGER_STATE_PAUSED;
		break;
	default:
		break;
	}
	UpdateCriticalSection.Leave();
}

bool PathManagerImpl::QPaused() {
	return (bool)((uint8_t)(eState == 2));
}

bool PathManagerImpl::QIdle() {
	return (eState == 0);
}

void PathManagerImpl::Resume() {
	UpdateCriticalSection.Enter(PathManagerImpl_Resume_Name);
	switch (eState) {
	case PATH_MANAGER_STATE_WAITING_FOR_PAUSE:
		eState = PATH_MANAGER_STATE_IDLE;
		break;
	case PATH_MANAGER_STATE_PAUSED:
		eState = PATH_MANAGER_STATE_WAITING_FOR_RESUME;
		break;
	default:
		break;
	}
	UpdateCriticalSection.Leave();
}

void PathManagerImpl::Release() {
	Close();
	BSTaskletManager::QInstance().DestroyTaskGroup(*this);
}

// The exe's cleanup-only C++ EH frame is left out. The walk's GetFinished is the shared exe copy 0x6EBDE0, which src
// defines inline in LockFreeMap.h; the queue entry's message is destroyed through ActorPathingMessage's destructor
// fold (0x800190).
void PathManagerImpl::CancelCurrentPathsAndPause() {
	FNVD_INLINED_LOCAL(ActorPathingMessage, v4, std::nullopt, std::nullopt);
	PathingMessage msg;
	using it_t = LockFreeMap<PathingRequest*, PathingTaskData*>::LockFreeMapIterator;
	FNVD_INLINED_LOCAL(it_t, it, std::nullopt, std::nullopt);
	UpdateCriticalSection.Enter("PathManagerImpl::CancelCurrentPathsAndPause");
	Close();
	bCancelPaths = 1;
	WaitForCompletion(0);
	msg.~PathingMessage();
	::new (&msg) PathingMessage;
	// The exe calls the queue's TryPop (slot 4) and the map's RemoveAt (slot 5) through their vtables; calls on the
	// members themselves would be devirtualised, so they go through base pointers.
	BSTMessageQueue* pmessageQueue = &MessageQueue;
	LockFreeMap<PathingRequest*, PathingTaskData*>* prequestMap = &RequestToTaskletDataMap;
	while (pmessageQueue->TryPop(&msg)) {
		// The exe tests the signed range PMT_OK..PMT_ERROR (jl 0, jle 1): PMT_UNDEF is skipped.
		switch (msg.msgType) {
		case PMT_OK:
		case PMT_ERROR:
			prequestMap->RemoveAt(msg.pTaskData->spRequest.m_pObject);
			if (msg.pTaskData) {
				delete msg.pTaskData;
			}
			break;
		default:
			break;
		}
	}
	::new (&it) LockFreeMap<PathingRequest*, PathingTaskData*>::LockFreeMapIterator;
	while (!it.GetFinished()) {
		PathingRequest* apThis_16;
		PathingTaskData* ptaskData;
		if (RequestToTaskletDataMap.GetNext(it, apThis_16, ptaskData, 1)) {
			ActorPathingMessageQueue* v2 = ptaskData->spMessageQueue.m_pObject;
			ActorPathingMessage* v1 = ::new (&v4) ActorPathingMessage(1u, ptaskData->spSolution);
			v2->Push(v1);
			v4.~ActorPathingMessage();
			if (ptaskData) {
				delete ptaskData;
			}
		}
	}
	RequestToTaskletDataMap.RemoveAll(false);
	SpinLock.LockForWrite();
	eState = PathManagerImpl::PATH_MANAGER_STATE_PAUSED;
	bCancelPaths = 0;
	Open();
	UpdateCriticalSection.Leave();
	it.~LockFreeMapIterator();
}

bool PathManagerImpl::Initialize() {
	BSTaskletManager* singleton = &BSTaskletManager::QInstance();
	if (!singleton->CreateTaskGroup(*this)) {
		return (bool)0;
	}
	if (!BSTaskletGroup::Open()) {
		return (bool)0;
	}
	eState = PATH_MANAGER_STATE_IDLE;
	iPauseRetryCount = (uint32_t)0;
	bCancelPaths = (bool)0;
	return (bool)1;
}

bool PathManagerImpl::QCancelPaths() const {
	return (bool)((uint8_t)bCancelPaths);
}

void PathManagerImpl::TaskUnlock() {
	(&SpinLock)->UnlockRead();
}

bool PathManagerImpl::TaskTryLock() {
	return (&SpinLock)->TryLockForRead();
}

void PathManagerImpl::SendMessageTo(const PathingMessage& aMsg) {
	// The exe calls the queue's Push (slot 1) through its vtable; a call on the member itself would be
	// devirtualised, so it goes through a base pointer.
	BSTMessageQueue* pmessageQueue = &MessageQueue;
	pmessageQueue->Push(&aMsg);
}

void PathManagerImpl::operator delete(void* apMemory) {
	ourFree(apMemory);
}

void BSTaskletData::TaskYield() {
	bYielding = true;
}

// The exe passes abPoll zero-extended (movzx) to the data's WaitForCompletion (slot 1); a plain argument forwards
// the whole incoming stack dword. bool args from MSVC callers are 0/1; the `and 1` clang adds is a no-op on them.
void BSTaskletGroup::WaitForCompletion(bool abPoll) {
	if (pData && !pData->QComplete())
		pData->WaitForCompletion(abPoll);
}

ActorPathingMessage& ActorPathingMessage::operator=(const ActorPathingMessage& aMsg) {
	msgType = aMsg.msgType;
	spSolution = aMsg.spSolution;
	return *this;
}

int32_t PathingSearchRayCast::GetTotalRayCasts() {
	return iRayCastCount;
}
