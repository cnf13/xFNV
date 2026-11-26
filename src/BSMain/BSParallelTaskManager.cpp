#include "BSMain/BSParallelTaskManager.h"

#include "BSCore/BSSemaphore.h"
#include "BSCore/BSString.h"
#include "BSCore/BSThread.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSHavok/bhkThreadMemoryRouter.h"
#include "BSMain/BSParallelTask.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"

// One worker thread, named and pinned to its processor from aiThread.
BSParallelTaskManager::BSParallelTaskManager()
  : pThreads(nullptr), pTaskFunc(nullptr), pData(nullptr), bRunningTask(false) {
    pThreads = static_cast<BSParallelTaskThread**>(MemoryManager::Instance().Allocate(sizeof(BSParallelTaskThread*)));
    for (int32_t i = 0; i < 1; ++i) {
        char acName[264];
        pThreads[i] = new BSParallelTaskThread(this);
        BSsprintf(acName, 260, "Parallel Task Thread %i", aiThread[i]);
        pThreads[i]->Initialize(BSThread::MEDIUM_STACK_SIZE, acName, false);
        pThreads[i]->SetName(acName);
        pThreads[i]->SetThreadProcessor(aiThread[i]);
        pThreads[i]->SetThreadPriority(15);
    }
}

// Shuts each worker thread down, waits for it to finish and deletes it.
BSParallelTaskManager::~BSParallelTaskManager() {
    for (int32_t i = 0; i < 1; ++i) {
        pThreads[i]->Shutdown();
        pThreads[i]->FinishedProcessing.Wait();
        delete pThreads[i];
        pThreads[i] = nullptr;
    }
    MemoryManager::Instance().Deallocate(pThreads);
}

void BSParallelTaskManager::RunTask(TaskFunc apTaskFunc, void* apData) {
    bRunningTask = true;
    pTaskFunc = apTaskFunc;
    pData = apData;
    StartThreads();
    RunTask();
    WaitForThreads();
    bRunningTask = false;
}

void BSParallelTaskManager::RunTask(BSParallelArrayTask* apArrayTask) {
    RunTask(&BSParallelTaskThread::IteratorFunc, apArrayTask);
}

void BSParallelTaskManager::RunTask() {
    pTaskFunc(pData);
}

// Signals each worker thread's StartProcessing semaphore.
void BSParallelTaskManager::StartThreads() {
    for (int32_t i = 0; i < 1; ++i) {
        BSParallelTaskThread* pThread = pThreads[i];
        pThread->StartProcessing.hSemaphore->Release();
        InterlockedIncrementInt32(&pThread->StartProcessing.iCount);
    }
}

void BSParallelTaskManager::WaitForThreads() {
    for (int32_t i = 0; !i; i = 1) {
        BSParallelTaskThread* pThread = *pThreads;
        pThread->FinishedProcessing.hSemaphore->Wait();
        InterlockedDecrementInt32(&pThread->FinishedProcessing.iCount);
    }
}

void BSParallelTaskManager::operator delete(void* apMemory) {
    ourFree(apMemory);
}

BSParallelTaskThread::BSParallelTaskThread(BSParallelTaskManager* apManager) : pManager(apManager) {}

BSParallelTaskThread::~BSParallelTaskThread() {}

void BSParallelTaskThread::OnStartup() {
    HavokThreadMemory.pHavokThreadRouter = nullptr;
    HavokThreadMemory.pHavokThreadStackBuffer = nullptr;
    HavokThreadMemory.iThreadID = m_ThreadID;
    bhkThreadMemoryRouter::Initialize(&HavokThreadMemory);
}

void BSParallelTaskThread::OnShutdown() {
    bhkThreadMemoryRouter::Quit(&HavokThreadMemory);
}

void BSParallelTaskThread::RunLinearTasks() {
    pManager->RunTask();
}

void BSParallelTaskThread::IteratorFunc(void* apTask) {
    BSParallelTask* pTask = static_cast<BSParallelTask*>(apTask);
    uint32_t uiPos = 0;
    while (uiPos < pTask->uiSize) {
        uiPos = InterlockedIncrementInt32(reinterpret_cast<int32_t*>(&pTask->uiPosition)) - 1;
        if (uiPos < pTask->uiSize) {
            pTask->pCallBack(pTask->pArray, uiPos);
        }
    }
}

void BSParallelTaskThread::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}
