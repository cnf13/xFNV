#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTask.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTaskManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSemaphore.h"

class NiThread;

// The task manager that runs update tasks on its own thread. Only what callers reach is declared.
class NiParallelUpdateTaskManager : public NiTaskManager {
public:
    // The thread procedure; only a pointer to it is needed, so it is only forward-declared.
    class ManagerProc;

    // The task that signals the render semaphore. The class has no members of its own beyond NiTask's.
    class SignalTask : public NiTask {
    public:
        void DoTask() override {
            NiParallelUpdateTaskManager::Get()->SignalRenderSema();
            m_eStatus = COMPLETED;
        }

        static NiRTTI ms_RTTI;
    };

    ~NiParallelUpdateTaskManager() override;

    bool AddTask(NiTask* pkTask, TaskPriority ePriority) override;
    // The return is AddTask's bool; EndUpdate ignores it.
    virtual bool
    AddTask_Internal(bool bAddingSignalTask, NiTask* pkTask, TaskPriority ePriority, bool abRendererDependent) = 0;

    // Returns the manager, null when there is none.
    static NiParallelUpdateTaskManager* Get();
    // True when the manager exists and its thread is running.
    static bool IsActive();
    // Each does nothing without a manager. EndUpdate finishes the update pass; WaitRenderSema waits up to aiTime ms
    // (-1: forever) on the render semaphore and returns 0; SignalRenderSema signals it.
    static void EndUpdate();
    static int32_t WaitRenderSema(uint32_t aiTime);
    static int32_t SignalRenderSema();
    static void BeginUpdate();
    // Sets bEnd.
    static void End();
    // Stops the manager and deletes it, then clears ms_pkManager; does nothing without a manager.
    static void Shutdown();

    uint32_t m_uiMaxQueueSize;
    NiSemaphore m_kUpdateSema;
    NiSemaphore m_kRenderSema;
    SignalTask m_kSignalTask;
    NiThread* m_pkThread;
    ManagerProc* m_pkThreadProc;
    bool m_bActive;

    static inline NiParallelUpdateTaskManager* ms_pkManager;
    static inline bool bEnd;
    static NiRTTI ms_RTTI;
};
