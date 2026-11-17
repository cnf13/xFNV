#pragma once

#include "BSCore/BSSemaphore.h"
#include "BSHavok/HavokThreadMemoryStruct.h"

#include <stdint.h>

// The task threads' start routine (CreateThread's): Havok's per-thread memory is set up, then the thread's loop body,
// ThreadUpdate, runs. The thread is CreateThread's parameter, as its start routine takes it.
uint32_t BSTaskThreadProc(void* apTaskThread);

// A worker thread that runs tasks: it waits on a task semaphore and a pause semaphore. Derived classes supply
// ThreadUpdate, the thread's loop body. TestPauseSemaphore and sUpdateSleepValue are not declared.
class BSTaskThread {
public:
    BSTaskThread(uint32_t aiProcessor, const char* apThreadName);
    virtual ~BSTaskThread();

    virtual void ThreadUpdate() = 0;

    void Sleep();
    // Signals the thread's task semaphore.
    void SignalTask();
    // Resumes the thread (it is created suspended). No caller reads the suspend count ResumeThread returns.
    void StartThread();
    // Signals the task semaphore only while its iCount is 0.
    void SignalSemaphore();
    void SignalPauseSemaphore();
    // Signals the task semaphore, then waits on the pause semaphore.
    void Pause();
    // Signals the task semaphore and the pause semaphore.
    void Resume();
    void WaitTask();

protected:
    // Waits on the task semaphore.
    void WaitSemaphore();
    // Waits on the pause semaphore.
    void WaitPauseSemaphore();

public:
    static void SetPauseSleepEnabled(bool abEnable);
    static void SetPauseSleepValue(int32_t aiValue);
    static void SetUpdateSleepEnabled(bool abEnable);
    // Frees through MemoryManager::Deallocate.
    static void operator delete(void* apMemory);
    static void SetUpdateSleepValue(int32_t a1);

    BSSystemThread* hThreadHandle;
    uint32_t iThreadID;
    BSSemaphore Semaphore;
    BSSemaphore PauseSemaphore;
    HavokThreadMemoryStruct HavokThreadMemory;

    // Whether the thread sleeps on a pause, and for how many milliseconds.
    static inline bool sDoPauseSleep;
    static inline int32_t sPauseSleepValue = 30;
    // Whether the thread sleeps between updates.
    static inline bool sDoUpdateSleep = true;
    static inline int32_t sUpdateSleepValue = 5;
};
