#include "BSSystem/Win32/BSWin32TaskletGroupData.h"

#include "BSCore/BSSemaphore.h"
#include "BSCore/MemoryManager.h"
#include "BSSystem/BSTaskletGroupData.h"

// The base constructor sets bComplete; kCompletionSemaphore's default constructor creates the (0, 1) semaphore.
BSWin32TaskletGroupData::BSWin32TaskletGroupData()
  : BSTaskletGroupData(), padding2C(0), bAllowTaskAttach(false), padding2E(0), uiPriority(0), uiAttached(0),
    uiCompleted(0) {
    InitializeCriticalSection(&GroupDataLock);
}

// Waits for an unfinished group, then deletes the lock; kCompletionSemaphore's destructor closes its handle.
BSWin32TaskletGroupData::~BSWin32TaskletGroupData() {
    if (!bComplete) {
        WaitForCompletion(false);
    }
    DeleteCriticalSection(&GroupDataLock);
}

void BSWin32TaskletGroupData::WaitForCompletion(bool a1) {
    if (uiCompleted != uiAttached) {
        kCompletionSemaphore.hSemaphore->Wait();
        InterlockedDecrementInt32(&kCompletionSemaphore.iCount);
    }
    bComplete = 1;
    padding2C = 0;
}

void BSWin32TaskletGroupData::CompleteTask() {
    EnterCriticalSection(&GroupDataLock);
    ++uiCompleted;
    bool v1 = !bAllowTaskAttach && uiCompleted == uiAttached;
    LeaveCriticalSection(&GroupDataLock);
    if (v1) {
        kCompletionSemaphore.hSemaphore->Release();
        InterlockedIncrementInt32(&kCompletionSemaphore.iCount);
    }
}
