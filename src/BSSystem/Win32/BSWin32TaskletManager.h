#pragma once

#include "BSCore/BSSemaphore.h"
#include "BSSystem/BSTaskletManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <stdint.h>

class BSWin32TaskletData;
class BSWin32TaskletManager;

// The PC tasklet manager, the counterpart of BSXenonTaskletManager: it owns up to six worker threads and the per-thread
// queues of tasklet data.
class BSWin32TaskletManager : public BSTaskletManager {
public:
    BSWin32TaskletManager(uint32_t auiNumThreads);
    ~BSWin32TaskletManager() override;

    void ShutdownAllTasks() override;
    bool IsTaskletThread(uint32_t) override;
    bool CreateTaskGroupData(BSTaskletGroupData*&) override;
    bool OpenTaskGroupData(BSTaskletGroupData*) override;
    bool AttachTaskData(BSTaskletGroupData*, BSTaskletData*, bool) override;
    bool CloseTaskGroupData(BSTaskletGroupData*) override;
    void DestroyTaskGroupData(BSTaskletGroupData*) override;

    void ProcessQueue();
    char QueueTaskData(BSWin32TaskletData* arData);
    void Threads();

    // The init thread's start routine; waits on InstanceSema, then resumes the workers. apThis is the manager, passed
    // as the thread start routine's parameter.
    static uint32_t ThreadInitProc(void* apThis);
    // The worker threads' start routine; it is passed to CreateThread with the manager as its parameter.
    static uint32_t ThreadProc(void* apThis);

    // Holds the init thread as a standard-C++ thread rather than a HANDLE, despite its name.
    BSSystemThread* hInitThreadHandle;
    uint32_t uiNumThreads;
    uint32_t uiNumThreadsCreated;
    // The worker threads, one per slot (threads, not thread handles, despite the name).
    BSSystemThread* hThreadHandles[6];
    uint32_t uiThreadIDs[6];
    bool bTerminated;
    uint8_t padding51[3];
    _RTL_CRITICAL_SECTION kQueueLock;
    BSWin32TaskletData* pQueueHead[64];
    BSWin32TaskletData** ppQueueTail[64];
    BSSemaphore kStartSemaphore;
    char cThreadNames[0x600];
};
