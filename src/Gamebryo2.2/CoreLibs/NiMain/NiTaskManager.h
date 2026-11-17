#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTask_NiTQueue.h"

class NiTask;

// Runs NiTasks from per-priority queues. Only what callers reach is declared; the lock and status after the queues
// are not modelled yet.
class NiTaskManager : public NiObject {
public:
    enum TaskPriority : int32_t {
        LOW = 0,
        MEDIUM = 1,
        HIGH = 2,
        IMMEDIATE = 3,
    };

    enum ManagerStatus : int32_t {
        STOPPED = 0,
        STOPPING = 1,
        RUNNING = 2,
    };

    // The virtuals follow NiObject's in this order: AddTask, DoTasks, DoSingleTask, Clear, Stop. The class is never
    // constructed, so they are pure here.
    ~NiTaskManager() override;

    virtual bool AddTask(NiTask* pkTask, TaskPriority ePriority) = 0;
    virtual ManagerStatus DoTasks() = 0;
    virtual bool DoSingleTask(TaskPriority ePriority) = 0;
    virtual bool Clear() = 0;
    virtual ManagerStatus Stop() = 0;

    int32_t GetNumTasks(NiTaskManager::TaskPriority ePriority);

    // One queue per TaskPriority, held in place rather than through pointers.
    NiTask_NiTQueue m_akTaskQueues[4];

    static NiRTTI ms_RTTI;
};
