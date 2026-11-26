#pragma once

#include "BSCore/BSSemaphore.h"
#include "BSCore/BSThread.h"
#include "BSCore/NewOverloads.h"

#include <stdint.h>

// A worker thread that runs linear tasks: it waits on StartProcessing, runs its tasks and signals FinishedProcessing.
class BSLinearTaskThread : public BSThread {
public:
    BSLinearTaskThread();
    ~BSLinearTaskThread() override;

    uint32_t ThreadProc() override;
    virtual void OnStartup() {}
    virtual void OnShutdown() {}
    virtual void RunLinearTasks() = 0;

    void WaitForCompletion();
    void StartProcessingTasks();
    void Shutdown();

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    BSSemaphore StartProcessing;
    BSSemaphore FinishedProcessing;
    bool bKillThread;
};

inline BSLinearTaskThread::BSLinearTaskThread() : bKillThread(false) {}

inline BSLinearTaskThread::~BSLinearTaskThread() {}

inline uint32_t BSLinearTaskThread::ThreadProc() {
    OnStartup();
    while (!bKillThread) {
        StartProcessing.Wait();
        if (bKillThread) {
            break;
        }
        RunLinearTasks();
        FinishedProcessing.Signal();
    }
    OnShutdown();
    FinishedProcessing.Signal();
    return 0;
}

// ~BSParallelTaskThreadManager and AILinearTaskThreadManager::DestroyThreads call this before waiting for the thread:
// it sets bKillThread and wakes the thread.
inline void BSLinearTaskThread::Shutdown() {
    bKillThread = true;
    StartProcessing.Signal();
}

inline void BSLinearTaskThread::WaitForCompletion() {
    FinishedProcessing.Wait();
}

inline void BSLinearTaskThread::StartProcessingTasks() {
    StartProcessing.Signal();
}
