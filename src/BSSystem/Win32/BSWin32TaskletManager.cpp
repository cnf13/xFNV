#include "BSSystem/Win32/BSWin32TaskletManager.h"

#include "BSCore/BSSemaphore.h"
#include "BSCore/MemoryManager.h"
#include "BSSystem/BSTaskletData.h"
#include "BSSystem/BSTaskletGroupData.h"
#include "BSSystem/BSTaskletManager.h"
#include "BSSystem/Win32/BSWin32TaskletData.h"
#include "BSSystem/Win32/BSWin32TaskletGroupData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

// The workers start suspended (CREATE_SUSPENDED, 4); the init thread resumes them once InstanceSema is signalled.
BSWin32TaskletManager::BSWin32TaskletManager(uint32_t auiNumThreads)
	: hInitThreadHandle(nullptr), uiNumThreads(auiNumThreads <= 6 ? auiNumThreads : 6), uiNumThreadsCreated(0),
	  bTerminated(false), kStartSemaphore(0, 0x7FFFFFFF) {
	InitializeCriticalSection(&kQueueLock);
	for (uint32_t i = 0; i < 64; ++i) {
		pQueueHead[i] = nullptr;
	}
	for (uint32_t i = 0; i < 6; ++i) {
		hThreadHandles[i] = nullptr;
		uiThreadIDs[i] = 0;
	}
	// The start routines take the manager as the thread's void* parameter; ThreadProc's result is the thread's exit
	// code. A thread that cannot start throws (std::system_error) where CreateThread returns null.
	for (uint32_t i = 0; i < uiNumThreads; ++i) {
		hThreadHandles[i] = new BSSystemThread(&BSWin32TaskletManager::ThreadProc, this, true);
		uiThreadIDs[i] = hThreadHandles[i]->Id();
		++uiNumThreadsCreated;
	}
	bAvailable = uiNumThreadsCreated != 0;
	if (bAvailable) {
		hInitThreadHandle = new BSSystemThread(&BSWin32TaskletManager::ThreadInitProc, this, false);
	}
}

// kStartSemaphore's and the base's InstanceSema destructors close their handles.
BSWin32TaskletManager::~BSWin32TaskletManager() {
	DeleteCriticalSection(&kQueueLock);
}

void BSWin32TaskletManager::ShutdownAllTasks() {
	bTerminated = 1;
	if (bSignaled) {
		for (uint32_t i = 0; i < uiNumThreadsCreated; ++i) {
			kStartSemaphore.hSemaphore->Release();
			InterlockedIncrementInt32(&kStartSemaphore.iCount);
		}
		// WaitForMultipleObjects(all, INFINITE) on the threads.
		for (uint32_t i = 0; i < uiNumThreadsCreated; ++i) {
			hThreadHandles[i]->Wait();
		}
	}
	for (uint32_t j = 0; j < uiNumThreadsCreated; ++j) {
		delete hThreadHandles[j];
		hThreadHandles[j] = nullptr;
	}
	delete hInitThreadHandle;
	hInitThreadHandle = nullptr;
}

// Whether the thread is one of the six workers.
bool BSWin32TaskletManager::IsTaskletThread(uint32_t auiThreadID) {
	for (uint32_t i = 0; i < 6; ++i) {
		if (uiThreadIDs[i] == auiThreadID) {
			return true;
		}
	}
	return false;
}

bool BSWin32TaskletManager::CreateTaskGroupData(BSTaskletGroupData*& a1) {
	BSWin32TaskletGroupData* pData;
	BSWin32TaskletGroupData* apMemory =
		static_cast<BSWin32TaskletGroupData*>(MemoryManager_Singleton.Allocate(sizeof(BSWin32TaskletGroupData)));
	if (apMemory) {
		pData = ::new (apMemory) BSWin32TaskletGroupData;
	} else {
		pData = 0;
	}
	a1 = pData;
	return pData != 0;
}

// The PC group data is a BSWin32TaskletGroupData: its lock, semaphore and flags are read.
bool BSWin32TaskletManager::OpenTaskGroupData(BSTaskletGroupData* apGroupData) {
	BSWin32TaskletGroupData* pGroupData = static_cast<BSWin32TaskletGroupData*>(apGroupData);
	bool bResult;
	EnterCriticalSection(&pGroupData->GroupDataLock);
	if (pGroupData->padding2C) {
		bResult = false;
	} else {
		pGroupData->kCompletionSemaphore.hSemaphore->Wait(0);
		bResult = true;
		pGroupData->padding2C = bResult;
		pGroupData->bComplete = false;
		pGroupData->bAllowTaskAttach = true;
		pGroupData->padding2E = 0;
		pGroupData->uiAttached = 0;
		pGroupData->uiCompleted = 0;
	}
	LeaveCriticalSection(&pGroupData->GroupDataLock);
	return bResult;
}

// The PC group and task data are BSWin32TaskletGroupData and BSWin32TaskletData.
bool BSWin32TaskletManager::AttachTaskData(BSTaskletGroupData* apGroupData, BSTaskletData* apTaskData,
	bool abLastTask) {
	BSWin32TaskletGroupData* pGroupData = static_cast<BSWin32TaskletGroupData*>(apGroupData);
	BSWin32TaskletData* pTaskData = static_cast<BSWin32TaskletData*>(apTaskData);
	bool bResult;
	EnterCriticalSection(&pGroupData->GroupDataLock);
	// TaskDataLock is taken with InterlockedCompareExchange, as in BSWin32TaskletData::Destruct.
	if (InterlockedCompareExchange32(reinterpret_cast<int32_t*>(&pTaskData->TaskDataLock), 1, 0)) {
		bResult = false;
	} else {
		bResult = pGroupData->padding2C && pGroupData->bAllowTaskAttach && !pTaskData->pGroupData;
		if (bResult) {
			pTaskData->pGroupData = pGroupData;
			pTaskData->bYielding = false;
			if (abLastTask) {
				pGroupData->bAllowTaskAttach = false;
			}
			bResult = QueueTaskData(pTaskData);
			++pGroupData->uiAttached;
		} else {
			pTaskData->TaskDataLock = 0;
		}
	}
	LeaveCriticalSection(&pGroupData->GroupDataLock);
	return bResult;
}

// The PC group data is a BSWin32TaskletGroupData.
bool BSWin32TaskletManager::CloseTaskGroupData(BSTaskletGroupData* apGroupData) {
	BSWin32TaskletGroupData* pGroupData = static_cast<BSWin32TaskletGroupData*>(apGroupData);
	bool bResult;
	EnterCriticalSection(&pGroupData->GroupDataLock);
	if (pGroupData->padding2C) {
		pGroupData->bAllowTaskAttach = false;
		bResult = true;
	} else {
		bResult = false;
	}
	LeaveCriticalSection(&pGroupData->GroupDataLock);
	return bResult;
}

void BSWin32TaskletManager::DestroyTaskGroupData(BSTaskletGroupData* a1) {
	if (a1) {
		delete a1;
	}
}

void BSWin32TaskletManager::ProcessQueue() {
	while (!bTerminated) {
		BSWin32TaskletData* pTaskletData = 0;
		BSWin32TaskletGroupData* pGroupData = 0;
		kStartSemaphore.hSemaphore->Wait();
		InterlockedDecrementInt32(&kStartSemaphore.iCount);
		if (!bTerminated) {
			EnterCriticalSection(&kQueueLock);
			uint32_t v4 = 0;
			BSWin32TaskletData** pQueueHead = this->pQueueHead;
			while (v4 < 64) {
				pTaskletData = *pQueueHead;
				if (*pQueueHead) {
					*pQueueHead = pTaskletData->pLink;
					pGroupData = pTaskletData->pGroupData;
					break;
				}
				++v4;
				++pQueueHead;
			}
			LeaveCriticalSection(&kQueueLock);
			if (pTaskletData) {
				if (pGroupData) {
					if (!pGroupData->padding2E && (!pTaskletData->bRunOnStartup || pTaskletData->OnStartup())) {
						pTaskletData->bRunOnStartup = 0;
						pTaskletData->Process();
					}
				}
			}
			if (pTaskletData) {
				if (pGroupData) {
					if (!pTaskletData->bYielding || pGroupData->padding2E) {
						pTaskletData->OnComplete();
						pTaskletData->pGroupData = 0;
						pTaskletData->TaskDataLock = 0;
						pGroupData->CompleteTask();
					} else {
						QueueTaskData(pTaskletData);
					}
				}
			}
		}
		Sleep(0);
	}
}

char BSWin32TaskletManager::QueueTaskData(BSWin32TaskletData* arData) {
	if (!arData->pGroupData) {
		return 0;
	}
	EnterCriticalSection(&kQueueLock);
	uint32_t uiPriority = arData->pGroupData->uiPriority;
	if (uiPriority >= 64) {
		uiPriority = 63;
	}
	if (!pQueueHead[uiPriority]) {
		ppQueueTail[uiPriority] = &pQueueHead[uiPriority];
	}
	*ppQueueTail[uiPriority] = arData;
	ppQueueTail[uiPriority] = &arData->pLink;
	arData->pLink = 0;
	arData->bYielding = 0;
	LeaveCriticalSection(&kQueueLock);
	kStartSemaphore.hSemaphore->Release();
	InterlockedIncrementInt32(&kStartSemaphore.iCount);
	return 1;
}

void BSWin32TaskletManager::Threads() {
	for (uint32_t i = 0; i < uiNumThreadsCreated; ++i) {
		hThreadHandles[i]->Resume();
	}
}

uint32_t BSWin32TaskletManager::ThreadInitProc(void* apData) {
	BSWin32TaskletManager* apThis = static_cast<BSWin32TaskletManager*>(apData);
	apThis->InstanceSema.hSemaphore->Wait();
	InterlockedDecrementInt32(&apThis->InstanceSema.iCount);
	apThis->Threads();
	return 0;
}

uint32_t BSWin32TaskletManager::ThreadProc(void* apData) {
	BSWin32TaskletManager* apThis = static_cast<BSWin32TaskletManager*>(apData);
	apThis->OnStartup();
	apThis->ProcessQueue();
	apThis->OnExit();
	return 0;
}
