#include "BSMain/BSTaskManager.h"

#include "BSCore/BSThread.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSMain/BSTaskManagerThread.h"
#include "BSMain/IOManager.h"
#include "Fallout Shared/BSTaskThread.h"
#include "Fallout Shared/LockFreeMap.h"
#include "Fallout Shared/ModelLoader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/string.h"

#include <atomic>

// The thread takes the next table slot (the count is compared unsigned, jae), then gets its interface from the
// class (slot 1), stores the interface before its thread id, and sets the thread's TLS value.
InterfacedClass::BaseInterface* ThreadSpecificInterfaceManager::AddInterface_0(InterfacedClass* apInterfacedClass) {
	InterfacedClass::BaseInterface* pInterface = nullptr;
	uint32_t uiIndex = static_cast<uint32_t>(InterlockedIncrementInt32(&uiThreadCount) - 1);
	if (uiIndex < uiMaxThreads) {
		pInterface = apInterfacedClass->AllocateInterface(uiIndex);
		pInterfaces[uiIndex].pInterface = pInterface;
		pInterfaces[uiIndex].iThreadID = GetCurrentThreadId();
		TlsSetValue(uiTLSIndex, pInterface);
	}
	return pInterface;
}

// iTotalBSTaskCount counts the live tasks.
template <class T> BSTask<T>::BSTask() : iRefCount(0), eState(BS_TASK_STATE_PENDING), Key(0) {
	NiAtomicIncrement(const_cast<int32_t&>(iTotalBSTaskCount));
}

template <class T> BSTask<T>::~BSTask() {
	NiAtomicDecrement(const_cast<int32_t&>(iTotalBSTaskCount));
}

// The exe's deleting destructor (0x449A20) frees through ourFree.
template <class T> void BSTask<T>::operator delete(void* apMemory) {
	ourFree(apMemory);
}

template BSTask<int64_t>::BSTask();
template BSTask<int64_t>::~BSTask();
template void BSTask<int64_t>::operator delete(void* apMemory);

// The bucket counts start at zero; one worker thread per task thread is created and started, on processors 2 and
// up.
template <class T>
BSTaskManager<T>::BSTaskManager(uint32_t aiNumInterfaceThreads, uint32_t aiNumTaskThreads, uint32_t aiNumTaskBuckets)
	: LockFreeMap<T, NiPointer<BSTask<T>>>(aiNumTaskThreads + aiNumInterfaceThreads, aiNumTaskBuckets, 12u),
	  bExit(false), iNumThreadsPaused(0), iPauseCount(0), iNumTaskThreads(aiNumTaskThreads),
	  pBucketCounts(static_cast<uint32_t*>(MemoryManager_Singleton.Allocate(sizeof(uint32_t) * aiNumTaskBuckets))),
	  iBucketCountTotal(0) {
	std::memset(pBucketCounts, 0, sizeof(uint32_t) * aiNumTaskBuckets);
	pThreads = static_cast<BSTaskManagerThread**>(MemoryManager_Singleton.Allocate(sizeof(BSTaskManagerThread*) *
		iNumTaskThreads));
	for (uint32_t i = 0; i < iNumTaskThreads; ++i) {
		pThreads[i] = new BSTaskManagerThread(this, i + 2);
		pThreads[i]->StartThread();
	}
}

template BSTaskManager<int64_t>::BSTaskManager(uint32_t aiNumInterfaceThreads, uint32_t aiNumTaskThreads,
	uint32_t aiNumTaskBuckets);
// Deletes the worker threads, then frees the thread and bucket-count tables.
template <class T> BSTaskManager<T>::~BSTaskManager() {
	for (uint32_t i = 0; i < iNumTaskThreads; ++i) {
		delete pThreads[i];
	}
	MemoryManager_Singleton.Deallocate(pThreads);
	MemoryManager_Singleton.Deallocate(pBucketCounts);
}

// The exe's deleting destructor (0xC40960) frees through the engine's heap.
template <class T> void BSTaskManager<T>::operator delete(void* apMemory) {
	MemoryManager_Singleton.Deallocate(apMemory);
}

template BSTaskManager<int64_t>::~BSTaskManager();
template void BSTaskManager<int64_t>::operator delete(void* apMemory);
using LockFreeMap_int64_t_NiPointer_BSTask_int64_t_MarkedPointer_LockFreeMap_int64_t_NiPointer_BSTask_int64_t_HashListNode =
	LockFreeMap<int64_t,
		NiPointer<BSTask<int64_t>>>::MarkedPointer<LockFreeMap<int64_t, NiPointer<BSTask<int64_t>>>::HashListNode>;
// The generic bodies of BSTask's and BSTaskManager's members (the exe's instances: BSTask<__int64> and
// BSTaskManager<__int64>), instantiated below for the one key type.
template <class T> bool BSTask<T>::IsCompleted() {
	return eState == BS_TASK_STATE_COMPLETED;
}

template <class T> void BSTask<T>::IncRefCount() {
	NiAtomicIncrement(const_cast<int32_t&>(iRefCount));
}

// The last reference deletes the task.
template <class T> void BSTask<T>::DecRefCount() {
	if (!NiAtomicDecrement(const_cast<int32_t&>(iRefCount))) {
		delete this;
	}
}

// A compare-and-swap on the state word.
template <class T> bool BSTask<T>::SwitchState(BS_TASK_STATE eCurrentState, BS_TASK_STATE eNewState) {
	return std::atomic_ref<BS_TASK_STATE>(eState).compare_exchange_strong(eCurrentState, eNewState);
}

template <class T> void BSTask<T>::SetTaskCanceled() {
	eState = BS_TASK_STATE_CANCELED;
}

template bool BSTask<int64_t>::IsCompleted();
template void BSTask<int64_t>::IncRefCount();
template void BSTask<int64_t>::DecRefCount();
template bool BSTask<int64_t>::SwitchState(BS_TASK_STATE eCurrentState, BS_TASK_STATE eNewState);
template void BSTask<int64_t>::SetTaskCanceled();

// Marks the task queued, puts it in the map under its key and wakes the thread that runs it; the bucket's count and
// the total count follow. The map's reference is taken through a task pointer released before the counts change.
template <class T> bool BSTaskManager<T>::AddTask(BSTask<T>* apTask) {
	apTask->eState = BS_TASK_STATE_QUEUED;
	uint32_t uiBucket = this->GetBucketForKey(apTask->Key);
	uint32_t uiThread = GetThreadForTask(apTask);
	bool bAdded;
	{
		NiPointer<BSTask<T>> spTask(apTask);
		bAdded = this->SetAt(apTask->Key, spTask, false);
	}
	if (!bAdded) {
		return false;
	}
	InterlockedIncrementUInt32(&pBucketCounts[uiBucket]);
	InterlockedIncrementInt32(&iBucketCountTotal);
	pThreads[uiThread]->SignalTask();
	return true;
}

template <class T> void BSTaskManager<T>::RunTask(BSTask<T>* apTask) {
	apTask->Run();
}

template <class T> void BSTaskManager<T>::FinishTask(BSTask<T>* apTask) {
	apTask->eState = BS_TASK_STATE_FINISHED;
	apTask->Finish();
	apTask->eState = BS_TASK_STATE_COMPLETED;
	RemoveTask(apTask);
}

// Walks the map until a whole pass cancels nothing.
template <class T> void BSTaskManager<T>::CancelAllTasks() {
	bool bCanceled = true;
	while (bCanceled) {
		bCanceled = false;
		BSTaskManagerIterator<T> kIterator;
		while (!kIterator.GetFinished()) {
			T kKey = 0;
			NiPointer<BSTask<T>> spTask;
			if (this->GetNext(kIterator, kKey, spTask, true)) {
				CancelTask(spTask.m_pObject, nullptr);
				bCanceled = true;
			}
		}
	}
}

template <class T> void BSTaskManager<T>::ForceSignal() {
	for (uint32_t i = 0; i < iNumTaskThreads; ++i) {
		pThreads[i]->SignalTask();
	}
}

// Retries until the task's state is switched to canceled from the state it was read in: a queued task is taken out
// of the map first (under a reference of its own); a running, moving or finishing one is waited for.
template <class T> void BSTaskManager<T>::CancelTask(BSTask<T>* apTask, void* apTaskOwner) {
	BSTaskThread::SetPauseSleepEnabled(false);
	while (true) {
		BS_TASK_STATE eState = apTask->eState;
		switch (eState) {
		case BS_TASK_STATE_PENDING:
		case BS_TASK_STATE_COMPLETED:
			if (apTask->SwitchState(eState, BS_TASK_STATE_CANCELED)) {
				apTask->Cancel(eState, apTaskOwner);
				return;
			}
			break;
		case BS_TASK_STATE_QUEUED:
			if (apTask->SwitchState(eState, BS_TASK_STATE_CANCELED)) {
				apTask->IncRefCount();
				RemoveTask(apTask);
				apTask->Cancel(eState, apTaskOwner);
				apTask->DecRefCount();
				return;
			}
			break;
		// The exe waits on BS_TASK_STATE_RUNNING for a moving task too.
		case BS_TASK_STATE_MOVING:
		case BS_TASK_STATE_RUNNING:
			while (apTask->eState == BS_TASK_STATE_RUNNING) {
				BSSleep(0);
			}
			break;
		case BS_TASK_STATE_FINISHED:
			while (apTask->eState == BS_TASK_STATE_FINISHED) {
				BSSleep(0);
			}
			break;
		default:
			return;
		}
	}
}

template <class T> int32_t BSTaskManager<T>::GetQueuedCount() {
	return LockFreeMap<T, NiPointer<BSTask<T>>>::GetCount();
}

template <class T> uint32_t BSTaskManager<T>::GetCumulativeBucketCount(uint32_t aiBucket) {
	uint32_t uiCount = 0;
	for (uint32_t i = 0; i <= aiBucket; ++i) {
		uiCount += pBucketCounts[i];
	}
	return uiCount;
}

// Takes the task out of the map; the counts drop only when it was there. Returns whether it was there.
template <class T> bool BSTaskManager<T>::RemoveTask(BSTask<T>* apTask) {
	uint32_t uiBucket = this->GetBucketForKey(apTask->Key);
	if (this->RemoveAt(apTask->Key)) {
		InterlockedDecrementUInt32(&pBucketCounts[uiBucket]);
		NiAtomicDecrement(iBucketCountTotal);
		return true;
	}
	return false;
}

// Both state changes are compare-and-swaps of the state word; the state read first is the comparand.
template <class T> bool BSTaskManager<T>::RequeueTask(BSTask<T>* apTask, int64_t aNewKey) {
	BS_TASK_STATE eState = apTask->eState;
	if (eState == BS_TASK_STATE_PENDING) {
		apTask->Key = aNewKey;
		return true;
	}
	if (eState >= BS_TASK_STATE_RUNNING) {
		return false;
	}
	if (!apTask->SwitchState(eState, BS_TASK_STATE_MOVING)) {
		return false;
	}
	RemoveTask(apTask);
	apTask->Key = aNewKey;
	if (apTask->SwitchState(BS_TASK_STATE_MOVING, BS_TASK_STATE_PENDING)) {
		AddTask(apTask);
	}
	return true;
}

template <class T> void BSTaskManager<T>::WaitForTask(BSTask<T>* apTask) {
	while (apTask->eState == BS_TASK_STATE_RUNNING || apTask->eState == BS_TASK_STATE_COMPLETED) {
		BSSleep(0);
	}
}

// The last release of a pause waits until every worker thread has paused, then resumes them.
template <class T> bool BSTaskManager<T>::Resume() {
	if (!iPauseCount) {
		return false;
	}
	if (InterlockedDecrementInt32(&iPauseCount)) {
		return false;
	}
	while (static_cast<uint32_t>(iNumThreadsPaused) != iNumTaskThreads) {
		Sleep(1);
	}
	for (uint32_t i = 0; i < iNumTaskThreads; ++i) {
		pThreads[i]->Resume();
		InterlockedDecrementInt32(&iNumThreadsPaused);
	}
	return true;
}

// Every task runs on the first worker thread (the exe's slot 22 is a shared body returning 0, 0x8D0560).
template <class T> uint32_t BSTaskManager<T>::GetThreadForTask(BSTask<T>*) {
	return 0;
}

template bool BSTaskManager<int64_t>::AddTask(BSTask<int64_t>* apTask);
template void BSTaskManager<int64_t>::RunTask(BSTask<int64_t>* apTask);
template void BSTaskManager<int64_t>::FinishTask(BSTask<int64_t>* apTask);
template void BSTaskManager<int64_t>::CancelAllTasks();
template void BSTaskManager<int64_t>::ForceSignal();
template void BSTaskManager<int64_t>::CancelTask(BSTask<int64_t>* apTask, void* apTaskOwner);
template int32_t BSTaskManager<int64_t>::GetQueuedCount();
template uint32_t BSTaskManager<int64_t>::GetCumulativeBucketCount(uint32_t aiBucket);
template bool BSTaskManager<int64_t>::RemoveTask(BSTask<int64_t>* apTask);
template bool BSTaskManager<int64_t>::RequeueTask(BSTask<int64_t>* apTask, int64_t aNewKey);
template void BSTaskManager<int64_t>::WaitForTask(BSTask<int64_t>* apTask);
template bool BSTaskManager<int64_t>::Resume();
template uint32_t BSTaskManager<int64_t>::GetThreadForTask(BSTask<int64_t>* apTask);
