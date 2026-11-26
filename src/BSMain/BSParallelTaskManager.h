#pragma once

#include "BSCore/BSLinearTaskThread.h"
#include "BSHavok/HavokThreadMemoryStruct.h"

class BSParallelTaskThread;

// A task that the worker threads run once per item of an array: each call gets the array and the next unclaimed index.
// Only some of the members are declared.
class BSParallelArrayTask {
public:
    // the function the worker threads run on every item
    using TaskFunc_t = void (*)(void* apData, uint32_t auiIndex);

    BSParallelArrayTask() = default;
    BSParallelArrayTask(TaskFunc_t apTaskFunc, void* apData, uint32_t aiSize);

    // 0x0
    TaskFunc_t pTaskFunc;
    // 0x4
    void* pData;
    // 0x8
    uint32_t iSize;
    // 0xC the next index a worker claims
    volatile uint32_t iIndex;
};

// Runs tasks on the worker threads.
class BSParallelTaskManager {
public:
    // The task function the worker threads call with pData.
    using TaskFunc = void (*)(void*);

    BSParallelTaskManager();
    ~BSParallelTaskManager();

    // Runs apTaskFunc(apData) on the worker threads and waits for them.
    void RunTask(TaskFunc apTaskFunc, void* apData);
    // Runs apArrayTask on the worker threads until every item is done.
    void RunTask(BSParallelArrayTask* apArrayTask);
    // Runs the current task's items on the calling worker thread.
    void RunTask();
    // Starts the worker threads.
    void StartThreads();
    // Waits for the worker threads to finish the task.
    void WaitForThreads();

    // Frees through ourFree.
    static void operator delete(void* apMemory);

    BSParallelTaskThread** pThreads;
    TaskFunc pTaskFunc;
    void* pData;
    bool bRunningTask;
};

// A worker thread of the parallel task manager: a linear task thread with its own Havok thread memory.
class BSParallelTaskThread : public BSLinearTaskThread {
public:
    explicit BSParallelTaskThread(BSParallelTaskManager* apManager);
    ~BSParallelTaskThread() override;

    void OnStartup() override;
    void OnShutdown() override;
    void RunLinearTasks() override;

    // A TaskFunc, passed only as RunTask's task function, whose data is the BSParallelTask to run.
    static void IteratorFunc(void* apTask);
    // Frees through MemoryManager::Deallocate, as the game's deleting destructor does.
    static void operator delete(void* apMemory);

    BSParallelTaskManager* pManager;
    HavokThreadMemoryStruct HavokThreadMemory;
};

// The processor each worker thread runs on, indexed by thread; one thread is started.
using BSParallelTaskManager_ThreadProcessorArray = uint32_t[1];
inline BSParallelTaskManager_ThreadProcessorArray aiThread{5};

// The engine's one task manager.
inline BSParallelTaskManager* pParallelTaskManager;

inline BSParallelArrayTask::BSParallelArrayTask(TaskFunc_t apTaskFunc, void* apData, uint32_t aiSize)
  : pTaskFunc(apTaskFunc), pData(apData), iSize(aiSize), iIndex(0) {}
