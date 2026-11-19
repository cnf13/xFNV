#include "BSMain/IOManager.h"

#include "BSCore/BSPrecisionTimer.h"
#include "BSCore/BSSpinLock.h"
#include "BSCore/BSThread.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/BSTaskManagerThread.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/BSTaskThread.h"
#include "Fallout Shared/LockFreeMap.h"
#include "Fallout Shared/ModelLoader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/stdlib.h"

SettingT<INISettingCollection> gs_iPostProcessMilliseconds_BackgroundLoad("iPostProcessMilliseconds:BackgroundLoad", 5);
SettingT<INISettingCollection> gs_iPostProcessMillisecondsLoadingQueuedPriority_BackgroundLoad(
    "iPostProcessMillisecondsLoadingQueuedPriority:BackgroundLoad",
    20
);
static SettingT<INISettingCollection>
    iPostProcessMillisecondsEditor("iPostProcessMillisecondsEditor:BackgroundLoad", 50);
static SettingT<INISettingCollection>
    iPostProcessTaskWarningMilliseconds("iPostProcessTaskWarningMilliseconds:BackgroundLoad", 20);

IO_TASK_PRIORITY IOTask::GetIOTaskPriority() {
    return static_cast<IO_TASK_PRIORITY>(GetPriority());
}

bool IOManager::IsLoadingQueuedPriority() {
    return (bool)((uint8_t)(((int32_t)eLoadingQueuedPriority) != 6));
}

uint32_t IOManager::GetPostProcessingCount() {
    return pPostProcessQueue->GetCount();
}

void IOTask::AddToPostProcessQueue() {
    pIOManager->AddPostProcessTask(this);
}

void IOManager::AddPostProcessTask(IOTask* apTask) {
    NiPointer<IOTask> apMemory(apTask);
    int32_t Priority = apTask->GetPriority();
    pPostProcessQueue->Insert(Priority, &apMemory);
}

bool IOTask::IsTaskFinished() {
    return (bool)((uint8_t)(((int32_t)eState) >= 4));
}

bool IOTask::IsCancelled() {
    return (bool)((uint8_t)(((int32_t)eState) == 6));
}

void IOTask::SetCompleted() {
    eState = BS_TASK_STATE_COMPLETED;
}

int64_t IOTask::GetKey() {
    return Key;
}

void IOManager::LoadAllQueuedPriorities() {
    LoadQueuedPriority(IO_TASK_PRIORITY_VERY_LOW);
}

// Requeues the tasks of aePriority and above as critical and runs the queue until they, the post-process queue and the
// external count (the model loader's) are done, reporting the percentage done; worker threads stop sleeping between
// updates meanwhile. A null external count callback counts 0. The count callback takes no arguments.
void IOManager::LoadQueuedPriority(IO_TASK_PRIORITY aePriority) {
    if (eLoadingQueuedPriority == IO_TASK_PRIORITY_INVALID) {
        return;
    }
    BSTaskThread::sDoUpdateSleep = false;
    // EXE-INLINED none: IOManager::GetExternalCount
    if (uiCount || pPostProcessQueue->GetCount() || (pGetExternalCountCallback ? pGetExternalCountCallback() : 0)) {
        eLoadingQueuedPriority = aePriority;
        RequeuePrioritiesToNewPriority(aePriority, IO_TASK_PRIORITY_CRITICAL);
        uint32_t uiPostProcessCount = pPostProcessQueue->GetCount();
        // EXE-INLINED none: IOManager::GetExternalCount
        uint32_t uiExternalCount = pGetExternalCountCallback ? pGetExternalCountCallback() : 0;
        uint32_t uiTotal = GetCumulativeBucketCount(4 * aePriority + 3) + uiPostProcessCount + uiExternalCount;
        uint32_t uiRemaining = uiTotal;
        bool bUpdated = false;
        while (uiRemaining != 0 || !bUpdated) {
            float fPercent = static_cast<double>(uiTotal - uiRemaining) / static_cast<double>(uiTotal) * 100.0;
            if (pLoadQueuedPriorityCallback) {
                pLoadQueuedPriorityCallback(fPercent);
            }
            uint32_t uiQueuedCount = uiCount;
            if (uiQueuedCount > pPostProcessQueue->GetCount()) {
                Sleep(50);
            }
            bUpdated = UpdateQueue();
            uiPostProcessCount = pPostProcessQueue->GetCount();
            // EXE-INLINED none: IOManager::GetExternalCount
            uiExternalCount = pGetExternalCountCallback ? pGetExternalCountCallback() : 0;
            uiRemaining = GetCumulativeBucketCount(4 * aePriority + 3) + uiPostProcessCount + uiExternalCount;
        }
        eLoadingQueuedPriority = IO_TASK_PRIORITY_COUNT;
        if (pLoadQueuedPriorityCallback) {
            pLoadQueuedPriorityCallback(100.0f);
        }
        BSTaskThread::sDoUpdateSleep = true;
    }
    else {
        eLoadingQueuedPriority = IO_TASK_PRIORITY_COUNT;
        BSTaskThread::sDoUpdateSleep = true;
    }
}

using BSTaskMapIterator = LockFreeMap<int64_t, NiPointer<BSTask<int64_t>>>::LockFreeMapIterator;

void IOManager::Pause() {
    if ((DoSleep()) && (pPauseCallback)) {
        pPauseCallback();
    }
}

void IOManager::ForceHighestPriority() {
    eLoadingQueuedPriority = IO_TASK_PRIORITY_VERY_LOW;
}

void IOManager::PauseThread(int32_t a2) {
    pThreads[a2]->BSTaskThread::SignalPauseSemaphore();
}

void IOManager::WaitForThread(int32_t a2) {
    pThreads[a2]->BSTaskThread::WaitTask();
}

void IOManager::CancelAllTasks() {
    BSTaskManager<int64_t>::CancelAllTasks();
    pPostProcessQueue->Clear();
}

void IOTask::Requeue(IO_TASK_PRIORITY aeNewPriority) {
    pIOManager->RequeueTask(this, aeNewPriority);
}

uint8_t IOTask::GetPriorityFromKey(int64_t& ariKey) {
    return (uint8_t)(ariKey >> 16);
}

bool IOTask::IsPending() {
    return (((int32_t)eState) == 0);
}

uint32_t IOManager::GetBucketForKey(int64_t aiKey) {
    return (uint32_t)(((((uint8_t)(aiKey >> 56)) + 4) % 20) * ((uint8_t)(aiKey >> 16)));
}

uint16_t IOTask::GetNextCounter() {
    return static_cast<uint16_t>(NiAtomicIncrement(IOTask::iStaticCounter));
}

uint32_t IOTask::GetNextOffset() {
    return NiAtomicIncrement(IOTask::iStaticOffset);
}

bool IOManager::DoSleep() {
    uint32_t i;
    BSTaskThread::sDoPauseSleep = false;
    if (InterlockedIncrementInt32(&iPauseCount) != 1) {
        return false;
    }
    while (iNumThreadsPaused) {
        Sleep(1u);
    }
    for (i = 0; i < iNumTaskThreads; ++i) {
        pThreads[i]->Sleep();
        InterlockedIncrementInt32(&iNumThreadsPaused);
    }
    return true;
}

// Until the manager exits: waits for a task signal, then walks the manager's tasks, running and finishing each queued
// one it can claim (QUEUED to RUNNING), between waits on the pause semaphore; a pause restarts the walk at bucket 0.
void BSTaskManagerThread::ThreadUpdate() {
    using Iterator = BSTaskManagerIterator<int64_t>;
    while (!pOwner->bExit) {
        Semaphore.hSemaphore->Wait();
        NiAtomicDecrement(Semaphore.iCount);
        Iterator kIterator;
        bool bRanTask = false;
        while (!kIterator.GetFinished()) {
            // WAIT_TIMEOUT: the manager is paused; wait for it to resume and restart the walk.
            if (PauseSemaphore.hSemaphore->Wait(0) == BSSystemWait::TIMED_OUT) {
                PauseSemaphore.hSemaphore->Wait();
                NiAtomicDecrement(PauseSemaphore.iCount);
                kIterator.SetBucket(0);
            }
            NiPointer<BSTask<int64_t>> spTask;
            int64_t iKey;
            if (pOwner->GetNext(kIterator, iKey, spTask, !bRanTask)) {
                if (spTask.m_pObject->eState == BS_TASK_STATE_QUEUED &&
                    InterlockedCompareExchange32(
                        reinterpret_cast<volatile int32_t*>(&spTask.m_pObject->eState), BS_TASK_STATE_RUNNING,
                        BS_TASK_STATE_QUEUED
                    ) == BS_TASK_STATE_QUEUED) {
                    bRanTask = true;
                    pOwner->RunTask(spTask.m_pObject);
                    pOwner->FinishTask(spTask.m_pObject);
                }
            }
            else if (!kIterator.GetFinished()) {
                kIterator.SetBucket(0);
                bRanTask = false;
            }
            spTask = nullptr;
            // EXE-INLINED 0x442550
            PauseSemaphore.hSemaphore->Release();
            NiAtomicIncrement(PauseSemaphore.iCount);
        }
    }
}
bool IOManager::TryCancelTask(IOTask* apTask, void* apTaskOwner) {
    while (true) {
        bool result;
        BS_TASK_STATE eState = apTask->eState;
        switch (eState) {
            case BS_TASK_STATE_PENDING: {
                if (!apTask->SwitchState(eState, BS_TASK_STATE_CANCELED)) {
                    continue;
                }
                goto LABEL_3;
            }
            case BS_TASK_STATE_QUEUED: {
                if (!apTask->SwitchState(eState, BS_TASK_STATE_CANCELED)) {
                    continue;
                }
                apTask->IncRefCount();
                RemoveTask(apTask);
                apTask->Cancel(eState, apTaskOwner);
                apTask->DecRefCount();
                return true;
            }
            case BS_TASK_STATE_MOVING: {
                while (apTask->eState == BS_TASK_STATE_RUNNING) {
                    BSSleep(0);
                }
                continue;
            }
            case BS_TASK_STATE_RUNNING:
            case BS_TASK_STATE_FINISHED: {
                return false;
            }
            case BS_TASK_STATE_COMPLETED: {
                if (!apTask->SwitchState(eState, BS_TASK_STATE_CANCELED)) {
                    continue;
                }
            LABEL_3:;
                apTask->Cancel(eState, apTaskOwner);
                result = true;
                break;
            }
            case BS_TASK_STATE_CANCELED: {
                result = true;
                break;
            }
            default: {
                result = true;
                break;
            }
        }
        return result;
    }
}

uint32_t IOManager::GetTaskThreadID(int32_t a2) {
    return pThreads[a2]->iThreadID;
}

bool IOManager::AddTask(BSTask<int64_t>* apTask) {
    if (eLoadingQueuedPriority != IO_TASK_PRIORITY_COUNT &&
        static_cast<uint8_t>(static_cast<uint32_t>(apTask->Key) >> 16) <= eLoadingQueuedPriority) {
        apTask->Key =
            ((static_cast<uint64_t>(static_cast<uint32_t>(apTask->Key >> 32)) << 32) |
             static_cast<uint32_t>((apTask->Key & 0xFF00FFFFu)));
    }
    static_cast<IOTask*>(apTask)->GenerateKey();
    while (!BSTaskManager<int64_t>::AddTask(apTask)) {
        apTask->Key = static_cast<uint16_t>(InterlockedIncrementInt32(&IOTask::iStaticCounter)) +
            ((static_cast<uint64_t>(static_cast<uint32_t>(apTask->Key >> 32)) << 32) |
             static_cast<uint32_t>((apTask->Key & 0xFFFF0000u)));
    }
    return true;
}
// One queue per priority, allocated under the task-overhead memory context.
template <class T>
LockFreePriorityQueue<T>::LockFreePriorityQueue(
    uint32_t aiPriorityCount,
    uint32_t aiMaxThreads,
    uint32_t aiDeleteBatchSize
)
  : iPriorityCount(aiPriorityCount), iCount(0) {
    MEM_CONTEXT eMemContext = GetThreadTLSData()->eMemContext;
    GetThreadTLSData()->eMemContext = MEM_CONTEXT_TASK_OVERHEAD;
    pQueues =
        static_cast<LockFreeQueue<T>**>(MemoryManager_Singleton.Allocate(sizeof(LockFreeQueue<T>*) * iPriorityCount));
    for (int32_t i = 0; i < iPriorityCount; ++i) {
        pQueues[i] = new LockFreeQueue<T>(aiMaxThreads, aiDeleteBatchSize);
    }
    GetThreadTLSData()->eMemContext = eMemContext;
}

template <class T>
LockFreePriorityQueue<T>::~LockFreePriorityQueue() {
    for (int32_t i = 0; i < iPriorityCount; ++i) {
        delete pQueues[i];
    }
    MemoryManager_Singleton.Deallocate(pQueues);
}

// Deleting frees through the engine's heap.
template <class T>
void LockFreePriorityQueue<T>::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}

template LockFreePriorityQueue<NiPointer<IOTask>>::LockFreePriorityQueue(
    uint32_t aiPriorityCount,
    uint32_t aiMaxThreads,
    uint32_t aiDeleteBatchSize
);
template LockFreePriorityQueue<NiPointer<IOTask>>::~LockFreePriorityQueue();
template void LockFreePriorityQueue<NiPointer<IOTask>>::operator delete(void* apMemory);

// Queues the element at aiPriority and counts it.
template <class T>
void LockFreePriorityQueue<T>::Insert(int32_t aiPriority, T* apTask) {
    pQueues[aiPriority]->Push(*apTask);
    IncrementCount();
}

// Empties every priority queue. The count is cleared first; the index compare is unsigned.
template <class T>
void LockFreePriorityQueue<T>::Clear() {
    iCount = 0;
    for (uint32_t i = 0; i < static_cast<uint32_t>(iPriorityCount); ++i) {
        pQueues[i]->RemoveAll(false);
    }
}

template <class T>
void LockFreePriorityQueue<T>::IncrementCount() {
    InterlockedIncrementInt32(&iCount);
}

template <class T>
void LockFreePriorityQueue<T>::DecrementCount() {
    InterlockedDecrementInt32(&iCount);
}

template <class T>
uint32_t LockFreePriorityQueue<T>::GetCount() {
    return iCount;
}

// Takes the first element of the highest priority (the lowest index) that has one.
template <class T>
bool LockFreePriorityQueue<T>::Pop(T& arData) {
    for (uint32_t i = 0; i < static_cast<uint32_t>(iPriorityCount); ++i) {
        if (pQueues[i]->Pop(arData)) {
            DecrementCount();
            return true;
        }
    }
    return false;
}

template void LockFreePriorityQueue<NiPointer<IOTask>>::Insert(int32_t aiPriority, NiPointer<IOTask>* apTask);
template void LockFreePriorityQueue<NiPointer<IOTask>>::Clear();
template void LockFreePriorityQueue<NiPointer<IOTask>>::IncrementCount();
template void LockFreePriorityQueue<NiPointer<IOTask>>::DecrementCount();
template uint32_t LockFreePriorityQueue<NiPointer<IOTask>>::GetCount();
template bool LockFreePriorityQueue<NiPointer<IOTask>>::Pop(NiPointer<IOTask>& arData);
// 10 interface threads, 1 task thread, 120 buckets; the post-process queue has 6 priorities. The IO manager's thread is
// its first worker thread. pUpdateSecondaryQueueCallback is deliberately left unwritten, as the game does.
IOManager::IOManager()
  : BSTaskManager<int64_t>(10u, 1u, 120u), eLoadingQueuedPriority(IO_TASK_PRIORITY_COUNT),
    pGetExternalCountCallback(nullptr), pUpdateQueueCallback(nullptr), pLoadQueuedPriorityCallback(nullptr),
    pPauseCallback(nullptr), pResumeCallback(nullptr) {
    pPostProcessQueue = new LockFreePriorityQueue<NiPointer<IOTask>>(6u, 7u, 8u);
    iIOManagerThreadID = pThreads[0]->iThreadID;
}

IOManager::~IOManager() {
    delete pPostProcessQueue;
}

bool IOManager::UpdateQueue() {
    LARGE_INTEGER llStartTime;
    uint32_t imaxTime;
    if (IOManager::bUpdating) {
        return true;
    }
    bool bqueueResult = true;
    IOManager::bUpdating = true;
    if (eLoadingQueuedPriority == IO_TASK_PRIORITY_COUNT) {
        Setting::Info* iPostProcessMilliseconds;
        // The setting's address is never null.
        iPostProcessMilliseconds = &gs_iPostProcessMilliseconds_BackgroundLoad.uValue;
        imaxTime = iPostProcessMilliseconds->uint;
    }
    else {
        Setting::Info* iPostProcessMillisecondsLoadingQueuedPriority;
        // The setting's address is never null.
        iPostProcessMillisecondsLoadingQueuedPriority =
            &gs_iPostProcessMillisecondsLoadingQueuedPriority_BackgroundLoad.uValue;
        imaxTime = iPostProcessMillisecondsLoadingQueuedPriority->uint;
    }
    llStartTime.QuadPart = BSPrecisionTimer::GetTimer();
    int64_t llMaxTime = static_cast<uint64_t>(static_cast<double>(imaxTime) * BSPrecisionTimer::fFrequencyMS);
    if (pUpdateSecondaryQueueCallback) {
        bqueueResult = pUpdateSecondaryQueueCallback();
    }
    NiPointer<IOTask> sptask;
    pPostProcessQueue->Pop(sptask);
    while (sptask.m_pObject) {
        sptask.m_pObject->PostProcess();
        if (pUpdateQueueCallback) {
            pUpdateQueueCallback();
        }
        if (BSPrecisionTimer::GetTimer() - llStartTime.QuadPart > llMaxTime) {
            sptask = nullptr;
        }
        else {
            pPostProcessQueue->Pop(sptask);
        }
    }
    IOManager::bUpdating = false;
    return bqueueResult;
}

void IOManager::RequeuePrioritiesToNewPriority(IO_TASK_PRIORITY aeOldPriority, IO_TASK_PRIORITY aeNewPriority) {
    using Iterator = BSTaskManagerIterator<int64_t>;
    if (aeOldPriority > aeNewPriority) {
        Pause();
        Iterator kIterator;
        kIterator.SetBucket(4 * aeNewPriority + 4);
        while (!kIterator.GetFinished()) {
            int64_t ikey = 0;
            NiPointer<BSTask<int64_t>> sptask;
            if (GetNext(kIterator, ikey, sptask, true)) {
                // The key's priority byte, compared signed with the int-sized enum.
                if (static_cast<uint8_t>(ikey >> 16) > aeOldPriority) {
                    break;
                }
                // The map holds IO tasks only.
                static_cast<IOTask*>(sptask.m_pObject)->Requeue(aeNewPriority);
            }
        }
        Resume();
    }
}

void IOManager::Resume() {
    if ((BSTaskManager<int64_t>::Resume()) && (pResumeCallback)) {
        pResumeCallback();
    }
}
bool IOManager::RequeueTask(IOTask* apTask, IO_TASK_PRIORITY aeNewPriority) {
    return BSTaskManager<int64_t>::RequeueTask(
        apTask,
        (static_cast<int64_t>(aeNewPriority) << 16) +
            ((static_cast<uint64_t>(static_cast<uint32_t>(apTask->Key >> 32)) << 32) |
             static_cast<uint32_t>((apTask->Key & 0xFF00FFFFu)))
    );
}
