#pragma once

#include "BSCore/BSSemaphore.h"
#include "BSCore/BSSpinLock.h"
#include "BSCore/BSThread.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class AILinearTaskThread;

// The AI worker threads that run the linear (staged) AI tasks alongside the main thread.
class AILinearTaskThreadManager {
public:
    enum AI_THREAD_INDEX : int32_t {
        AI_THREAD_1 = 0,
        AI_THREAD_2 = 1,
        AI_THREAD_COUNT = 2,
    };

    enum AI_THREAD_1_STAGE : int32_t {
        AI_1_START = 0,
        AI_1_TILEUPDATE = 1,
        AI_1_ANIMUPDATE = 2,
        AI_1_CELLANIMUPDATE = 3,
        AI_1_COMBAT_UPDATE = 4,
        AI_1_MAGICUPDATE = 5,
        AI_1_PACKAGE_UPDATE = 6,
        AI_1_ACTORUPDATE = 7,
        AI_1_PRERENDERACTIONS = 8,
        AI_1_POSTRENDERACTIONS = 9,
        AI_1_PLAYER_FOLLOWERS = 10,
        AI_1_DONE = 11,
    };

    enum AI_THREAD_2_STAGE : int32_t {
        AI_2_START = 0,
        AI_2_DETECTION = 1,
        AI_2_WORLDANIMS = 2,
        AI_2_ACTORMOVEMENT = 3,
        AI_2_ACTOR_RAGANIM = 4,
        AI_2_OBJECTDESTRUCT = 5,
        AI_2_WORLDUPDATE = 6,
        AI_2_DONE = 7,
    };

    AILinearTaskThreadManager();
    ~AILinearTaskThreadManager();

    // Blocks until the thread reaches the stage.
    void WaitOnThreadStage(AILinearTaskThreadManager::AI_THREAD_INDEX aiThreadIndex, uint32_t aiStage);
    void SetThreadStage(AILinearTaskThreadManager::AI_THREAD_INDEX aiThreadIndex, uint32_t aiStage);
    // The stage the thread has reached, plus the count of its stage semaphore.
    uint32_t GetMyThreadStage(AI_THREAD_INDEX aiThreadIndex);
    void WaitForThreads();
    // Starts the AI threads' frame.
    void StartThreads();

protected:
    void SetThreadStageDirect(AILinearTaskThreadManager::AI_THREAD_INDEX aiThreadIndex, uint32_t aiStage);

public:
    void CreateThreads();
    void DestroyThreads();
    void AddThread(
        AILinearTaskThreadManager::AI_THREAD_INDEX aiThreadIndex,
        uint32_t aiHWThread,
        void (*apfnTaskFunc)(),
        const char* apThreadName,
        BSThread::StackSize aeStackSize
    );

    // The manager, AILinearTaskThreadManager_Instance.
    static AILinearTaskThreadManager* QInstance();
    // True while the AI threads run.
    static bool QRunningThreads();
    // Waits on the render semaphore.
    static void WaitMainRendering();
    // Clears the main-rendering flag; when it was set and abVal is false, signals the render semaphore first.
    static void SetMainRendering(bool abVal);
    // Waits on the render semaphore RenderSema.
    static void WaitRenderSema();
    // Signals the render semaphore RenderSema.
    static void SignalRenderSema();
    // Whether 3D work should be queued to the background threads instead of done now.
    static bool ShouldQueue3DTask();

    // Main.cpp's name for QInstance.
    static AILinearTaskThreadManager* GetSingletonPtr() {
        return QInstance();
    }

    static uint8_t GetMainRendering();

protected:
    // The first AI thread's frame: interface idle, combat, animations, magic, packages, actor updates, in stages.
    static void AIThread1TaskFunc();
    // The AI frame run on one thread: the work of both AI threads in order, under ReferenceProcessLists.kUpdateLock.
    static void AIThreadCombined();
    // The second AI thread's frame: detection, movement, ragdolls, destruction and world animations, in stages.
    static void AIThread2TaskFunc();

public:
    AILinearTaskThread* AITaskThreadArray[3];
    uint32_t AITaskThreadStage[3];
    BSSemaphore AITaskStageSema[3];
    // The manual-reset events CreateThreads makes, held as standard-C++ objects rather than HANDLEs.
    BSSystemEvent* pFirstThreadEvents[12];
    BSSystemEvent* pSecondThreadEvents[8];
    // The constructor points these at the two event arrays.
    BSSystemEvent** pEvents[2];

    // A byte flag: the whole byte is tested against 0.
    static inline volatile uint8_t bMainRendering;
    static inline bool bIsRunningThreads;
    // The semaphore the main thread's rendering and the AI threads hand off through.
    static BSSemaphore RenderSema;
};

// One AI worker thread: it waits on StartProcessing, runs its linear tasks and signals FinishedProcessing.
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
    uint8_t padding49[3];
};

extern AILinearTaskThreadManager AILinearTaskThreadManager_Instance;

// The game settings AILinearTaskThreadManager.cpp defines.
extern SettingT<INISettingCollection> iAIThread2HWThread;
extern SettingT<INISettingCollection> iAIThread1HWThread;
inline uint32_t AnimThreadID;
