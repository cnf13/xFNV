#include "BSHavok/bhkThreadMemoryRouter.h"
#include "BSMain/BSTaskManager.h"

// Only sets up Havok's per-thread memory; it is not shut down after the loop body.
uint32_t BSTaskThreadProc(void* apTaskThread) {
    BSTaskThread* pTaskThread = static_cast<BSTaskThread*>(apTaskThread);
    bhkThreadMemoryRouter::Initialize(&pTaskThread->HavokThreadMemory);
    pTaskThread->ThreadUpdate();
    return 0;
}

// The thread is created suspended (StartThread resumes it); aiProcessor is deliberately unused.
BSTaskThread::BSTaskThread(uint32_t aiProcessor, const char* apThreadName)
  : Semaphore(0, std::numeric_limits<int32_t>::max()), PauseSemaphore(1, 1) {
    hThreadHandle = new BSSystemThread(&BSTaskThreadProc, this, true);
    iThreadID = hThreadHandle->Id();
    if (apThreadName) {
        hThreadHandle->SetName(apThreadName);
    }
    HavokThreadMemory.pHavokThreadRouter = nullptr;
    HavokThreadMemory.pHavokThreadStackBuffer = nullptr;
    HavokThreadMemory.iThreadID = iThreadID;
}

// The semaphores close their own handles. The thread is suspended (at its next wait: BSSystemThread) and closed.
BSTaskThread::~BSTaskThread() {
    hThreadHandle->Suspend();
    delete hThreadHandle;
}

void BSTaskThread::StartThread() {
    hThreadHandle->Resume();
}

void BSTaskThread::Sleep() {
    SignalTask();
    WaitPauseSemaphore();
}

void BSTaskThread::SignalTask() {
    if (!Semaphore.iCount) {
        Semaphore.Signal();
    }
}

void BSTaskThread::WaitTask() {
    Semaphore.Wait();
}

void BSTaskThread::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}
