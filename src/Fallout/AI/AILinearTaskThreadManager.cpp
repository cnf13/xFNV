#include "Fallout/AI/AILinearTaskThreadManager.h"

#include "BSCore/BSSemaphore.h"
#include "BSCore/BSString.h"
#include "BSCore/BSThread.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleManager.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout/AI/AILinearTaskThread.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/System/hkMemorySystem.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/System/hkBaseSystem.h"
#include "SDK/MSVC/ehvec.h"

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> iAIThread1HWThread("iAIThread1HWThread:General", 3);
SettingT<INISettingCollection> iAIThread2HWThread("iAIThread2HWThread:General", 4);
BSSemaphore AILinearTaskThreadManager::RenderSema;
AILinearTaskThreadManager AILinearTaskThreadManager::Instance;

AILinearTaskThreadManager::AILinearTaskThreadManager() : AITaskStageSema{{0, 20}, {0, 20}, {0, 20}} {
    for (int32_t i = 0; i < 3; ++i) {
        AITaskThreadArray[i] = nullptr;
        AITaskThreadStage[i] = 0;
    }
    pEvents[0] = pFirstThreadEvents;
    pEvents[1] = pSecondThreadEvents;
}

AILinearTaskThreadManager::~AILinearTaskThreadManager() {}

void AILinearTaskThreadManager::WaitOnThreadStage(
    AILinearTaskThreadManager::AI_THREAD_INDEX aiThreadIndex,
    uint32_t aiStage
) {
    pEvents[aiThreadIndex][aiStage]->Wait();
}

void AILinearTaskThreadManager::SetThreadStage(
    AILinearTaskThreadManager::AI_THREAD_INDEX aiThreadIndex,
    uint32_t aiStage
) {
    if (aiThreadIndex < 2 && AITaskThreadArray[aiThreadIndex]) {
        pEvents[aiThreadIndex][aiStage]->Set();
        AITaskThreadStage[aiThreadIndex] = aiStage;
    }
}

uint32_t AILinearTaskThreadManager::GetMyThreadStage(AI_THREAD_INDEX aiThreadIndex) {
    uint32_t uiStage = 0;
    if (aiThreadIndex < AI_THREAD_COUNT) {
        int32_t iCount = AITaskStageSema[aiThreadIndex].QCount();
        return AITaskThreadStage[aiThreadIndex] + iCount;
    }
    return uiStage;
}

void AILinearTaskThreadManager::WaitForThreads() {
    uint32_t i;
    for (i = 0; i < 2; ++i) {
        if (AITaskThreadArray[i]) {
            AITaskThreadArray[i]->BSLinearTaskThread::WaitForCompletion();
        }
    }
    AILinearTaskThreadManager::bIsRunningThreads = false;
}

void AILinearTaskThreadManager::StartThreads() {
    for (int32_t i = 0; i <= 11; ++i) {
        pFirstThreadEvents[i]->Reset();
    }
    for (int32_t j = 0; j <= 7; ++j) {
        pSecondThreadEvents[j]->Reset();
    }
    for (uint32_t k = 0; k < 2; ++k) {
        AITaskThreadStage[k] = 0;
    }
    AILinearTaskThreadManager::bIsRunningThreads = true;
    for (uint32_t m = 0; m < 2; ++m) {
        if (AITaskThreadArray[m]) {
            AITaskThreadArray[m]->StartProcessingTasks();
        }
    }
}

void AILinearTaskThreadManager::SetThreadStageDirect(
    AILinearTaskThreadManager::AI_THREAD_INDEX aiThreadIndex,
    uint32_t aiStage
) {
    AITaskThreadStage[aiThreadIndex] = aiStage;
}

// The hardware thread settings are ints; AddThread takes them as uint32_t.
void AILinearTaskThreadManager::CreateThreads() {
    for (int32_t i = 0; i <= 11; ++i) {
        pFirstThreadEvents[i] = new BSSystemEvent(true, false);
    }
    for (int32_t j = 0; j <= 7; ++j) {
        pSecondThreadEvents[j] = new BSSystemEvent(true, false);
    }
    if (iNumHWThreads.Int() <= 2) {
        AddThread(
            AILinearTaskThreadManager::AI_THREAD_1,
            iAIThread1HWThread.Int(),
            &AILinearTaskThreadManager::AIThreadCombined,
            "AI Linear Task Thread",
            BSThread::LARGE_STACK_SIZE
        );
    }
    else {
        AddThread(
            AILinearTaskThreadManager::AI_THREAD_1,
            iAIThread1HWThread.Int(),
            &AILinearTaskThreadManager::AIThread1TaskFunc,
            "AI Linear Task Thread 1",
            BSThread::LARGE_STACK_SIZE
        );
        AddThread(
            AILinearTaskThreadManager::AI_THREAD_2,
            iAIThread2HWThread.Int(),
            &AILinearTaskThreadManager::AIThread2TaskFunc,
            "AI Linear Task Thread 2",
            BSThread::LARGE_STACK_SIZE
        );
    }
    AnimThreadID = AITaskThreadArray[0]->m_ThreadID;
}

void AILinearTaskThreadManager::DestroyThreads() {
    for (uint32_t ithreadIdx = 0; ithreadIdx < 2; ++ithreadIdx) {
        if (AITaskThreadArray[ithreadIdx]) {
            AITaskThreadArray[ithreadIdx]->Shutdown();
            AITaskThreadArray[ithreadIdx]->WaitForCompletion();
            if (AITaskThreadArray[ithreadIdx]) {
                delete AITaskThreadArray[ithreadIdx];
            }
            AITaskThreadArray[ithreadIdx] = nullptr;
        }
    }
    for (int32_t i = 0; i <= 11; ++i) {
        delete pFirstThreadEvents[i];
        pFirstThreadEvents[i] = nullptr;
    }
    for (int32_t j = 0; j <= 7; ++j) {
        delete pSecondThreadEvents[j];
        pSecondThreadEvents[j] = nullptr;
    }
}

void AILinearTaskThreadManager::AddThread(
    AILinearTaskThreadManager::AI_THREAD_INDEX aiThreadIndex,
    uint32_t aiHWThread,
    void (*apfnTaskFunc)(),
    const char* apThreadName,
    BSThread::StackSize aeStackSize
) {
    if (aiThreadIndex <= AILinearTaskThreadManager::AI_THREAD_2 && !AITaskThreadArray[aiThreadIndex]) {
        AITaskThreadArray[aiThreadIndex] = new AILinearTaskThread(apfnTaskFunc);
        AITaskThreadArray[aiThreadIndex]->Initialize(aeStackSize, apThreadName, false);
        AITaskThreadArray[aiThreadIndex]->SetThreadProcessor(aiHWThread);
        AITaskThreadArray[aiThreadIndex]->SetThreadPriority(0);
        if (!apThreadName) {
            char acName[268];
            BSsprintf(acName, 260u, "AI Linear Task Thread DEF %i", aiThreadIndex);
            AITaskThreadArray[aiThreadIndex]->SetName(acName);
        }
    }
}

// Returns the address of the manager.
AILinearTaskThreadManager* AILinearTaskThreadManager::QInstance() {
    return &Instance;
}

bool AILinearTaskThreadManager::QRunningThreads() {
    return AILinearTaskThreadManager::bIsRunningThreads;
}

void AILinearTaskThreadManager::WaitMainRendering() {
    AILinearTaskThreadManager::WaitRenderSema();
}

// Stores abVal (false there) before signalling, then stores it again.
void AILinearTaskThreadManager::SetMainRendering(bool abVal) {
    if (bMainRendering && !abVal) {
        bMainRendering = abVal;
        SignalRenderSema();
    }
    bMainRendering = abVal;
}

void AILinearTaskThreadManager::WaitRenderSema() {
    RenderSema.Wait();
}

void AILinearTaskThreadManager::SignalRenderSema() {
    RenderSema.Signal();
}

bool AILinearTaskThreadManager::ShouldQueue3DTask() {
    bool bretval = false;
    bool bretvalBug44071 = false;
    if (iNumHWThreads.Int() > 1 && AILinearTaskThreadManager::QRunningThreads()) {
        bretvalBug44071 = TaskQueueInterface::GetInstance()->QQueueFurtherTasks();
        uint32_t icurrthread = BSGetCurrentThreadID();
        if (icurrthread == appMain->uiThreadID) {
            if (!AILinearTaskThreadManager::GetMainRendering()) {
                bretval = true;
                if (AILinearTaskThreadManager::QInstance()->GetMyThreadStage(AILinearTaskThreadManager::AI_THREAD_1) >=
                    AILinearTaskThreadManager::AI_1_DONE) {
                    if (AILinearTaskThreadManager::QInstance()->GetMyThreadStage(
                            AILinearTaskThreadManager::AI_THREAD_2
                        ) >= AILinearTaskThreadManager::AI_2_DONE) {
                        bretval = false;
                    }
                }
            }
        }
        else if (
            AILinearTaskThreadManager::GetMainRendering() ||
            AILinearTaskThreadManager::QInstance()->GetMyThreadStage(AILinearTaskThreadManager::AI_THREAD_2) <
                AILinearTaskThreadManager::AI_2_WORLDUPDATE
        ) {
            bretval = true;
        }
        if (!bretval) {
            uint32_t iprocessthread = TaskQueueInterface::GetInstance()->iProcessingThread;
            if (iprocessthread) {
                bretval = icurrthread != iprocessthread;
            }
        }
    }
    return bretval || bretvalBug44071;
}

uint8_t AILinearTaskThreadManager::GetMainRendering() {
    return AILinearTaskThreadManager::bMainRendering;
}

void AILinearTaskThreadManager::AIThread1TaskFunc() {
    QInstance()->SetThreadStage(AI_THREAD_1, AI_1_START);
    appMain->OnIdle_DoInterfaceIdle();
    QInstance()->SetThreadStage(AI_THREAD_1, AI_1_TILEUPDATE);
    TaskQueueInterface::GetInstance()->ThreadBeginInput();
    bool bPaused = Main::QFaderToBlackVisible();
    pCombatManager->Update(bPaused);
    ReferenceProcessLists.RunActorAnimationUpdates();
    ReferenceProcessLists.ParallelActorAnimationMovementUpdates();
    QInstance()->SetThreadStage(AI_THREAD_1, AI_1_ANIMUPDATE);
    TES::pInstance->RunAnimations();
    QInstance()->SetThreadStage(AI_THREAD_1, AI_1_CELLANIMUPDATE);
    pCombatManager->UpdateCombatants();
    QInstance()->SetThreadStage(AI_THREAD_1, AI_1_COMBAT_UPDATE);
    ReferenceProcessLists.RunActorMagicUpdates();
    QInstance()->SetThreadStage(AI_THREAD_1, AI_1_MAGICUPDATE);
    ReferenceProcessLists.UpdateHighListPackages(0.0f, false);
    QInstance()->SetThreadStage(AI_THREAD_1, AI_1_PACKAGE_UPDATE);
    ProcessLists::UpdatePlayerFollowers();
    QInstance()->SetThreadStage(AI_THREAD_1, AI_1_PLAYER_FOLLOWERS);
    QInstance()->WaitOnThreadStage(AI_THREAD_2, AI_2_DETECTION);
    QInstance()->WaitOnThreadStage(AI_THREAD_2, AI_2_ACTORMOVEMENT);
    ReferenceProcessLists.RunActorUpdates();
    QInstance()->SetThreadStage(AI_THREAD_1, AI_1_ACTORUPDATE);
    TaskQueueInterface::GetInstance()->ThreadEndInput();
    QInstance()->SetThreadStage(AI_THREAD_1, AI_1_PRERENDERACTIONS);
    QInstance()->WaitOnThreadStage(AI_THREAD_2, AI_2_DONE);
    WaitMainRendering();
    QInstance()->SetThreadStage(AI_THREAD_1, AI_1_POSTRENDERACTIONS);
    if (bUseObstacleAvoidance.Bool()) {
        NavMeshObstacleManager::GetInstance()->Update();
    }
    Script::ClearOptimizations();
    ScriptLocals::ClearOptimizations();
    QInstance()->SetThreadStage(AI_THREAD_1, AI_1_DONE);
}

// The single-threaded AI frame.
void AILinearTaskThreadManager::AIThreadCombined() {
    QInstance()->SetThreadStage(AI_THREAD_1, AI_1_START);
    appMain->OnIdle_DoInterfaceIdle();
    QInstance()->SetThreadStage(AI_THREAD_1, AI_1_TILEUPDATE);
    TaskQueueInterface::GetInstance()->ThreadBeginInput();
    bool bPaused = Main::QFaderToBlackVisible();
    pCombatManager->Update(bPaused);
    ReferenceProcessLists.kUpdateLock.Lock(nullptr);
    ReferenceProcessLists.RunActorAnimationUpdates();
    ReferenceProcessLists.ParallelActorAnimationMovementUpdates();
    ReferenceProcessLists.kUpdateLock.Unlock();
    TES::pInstance->RunAnimations();
    ReferenceProcessLists.kUpdateLock.Lock(nullptr);
    ReferenceProcessLists.RunActorMagicUpdates();
    ReferenceProcessLists.kUpdateLock.Unlock();
    pCombatManager->UpdateCombatants();
    ReferenceProcessLists.kUpdateLock.Lock(nullptr);
    ReferenceProcessLists.UpdateHighListPackages(0.0f, false);
    ProcessLists::UpdatePlayerFollowers();
    ReferenceProcessLists.RunDetectionForAllActors();
    ReferenceProcessLists.UpdateActorsMovement(0.0f, false, PROCESS_TYPE_HIGH);
    ReferenceProcessLists.UpdateEmptyStep(0.0f, false);
    ReferenceProcessLists.RunActorUpdates();
    ReferenceProcessLists.RunActorRagdollAnimationUpdates();
    BGSDestructibleObjectForm::UpdateDestructibleObjects();
    ReferenceProcessLists.kUpdateLock.Unlock();
    float fFrameAnimTime = Main::QFrameAnimTime();
    TES::pInstance->UpdateCellAnimations(fFrameAnimTime);
    QInstance()->SetThreadStageDirect(AI_THREAD_2, AI_2_WORLDUPDATE);
    WaitMainRendering();
    if (bUseObstacleAvoidance.Bool()) {
        NavMeshObstacleManager::GetInstance()->Update();
    }
    TaskQueueInterface::GetInstance()->ThreadEndInput();
    Script::ClearOptimizations();
    ScriptLocals::ClearOptimizations();
    QInstance()->SetThreadStageDirect(AI_THREAD_1, AI_1_DONE);
    QInstance()->SetThreadStageDirect(AI_THREAD_2, AI_2_DONE);
}

void AILinearTaskThreadManager::AIThread2TaskFunc() {
    QInstance()->SetThreadStage(AI_THREAD_2, AI_2_START);
    QInstance()->WaitOnThreadStage(AI_THREAD_1, AI_1_TILEUPDATE);
    TaskQueueInterface::GetInstance()->ThreadBeginInput();
    ReferenceProcessLists.RunDetectionForAllActors();
    QInstance()->SetThreadStage(AI_THREAD_2, AI_2_DETECTION);
    QInstance()->SetThreadStage(AI_THREAD_2, AI_2_WORLDANIMS);
    QInstance()->WaitOnThreadStage(AI_THREAD_1, AI_1_ANIMUPDATE);
    ReferenceProcessLists.UpdateActorsMovement(0.0f, false, PROCESS_TYPE_HIGH);
    QInstance()->SetThreadStage(AI_THREAD_2, AI_2_ACTORMOVEMENT);
    ReferenceProcessLists.UpdateEmptyStep(0.0f, false);
    ReferenceProcessLists.RunActorRagdollAnimationUpdates();
    QInstance()->SetThreadStage(AI_THREAD_2, AI_2_ACTOR_RAGANIM);
    BGSDestructibleObjectForm::UpdateDestructibleObjects();
    QInstance()->SetThreadStage(AI_THREAD_2, AI_2_OBJECTDESTRUCT);
    QInstance()->WaitOnThreadStage(AI_THREAD_1, AI_1_COMBAT_UPDATE);
    float fFrameAnimTime = Main::QFrameAnimTime();
    TES::pInstance->UpdateCellAnimations(fFrameAnimTime);
    QInstance()->SetThreadStage(AI_THREAD_2, AI_2_WORLDUPDATE);
    TaskQueueInterface::GetInstance()->ThreadEndInput();
    QInstance()->WaitOnThreadStage(AI_THREAD_1, AI_1_PRERENDERACTIONS);
    QInstance()->SetThreadStage(AI_THREAD_2, AI_2_DONE);
    Script::ClearOptimizations();
    ScriptLocals::ClearOptimizations();
    QInstance()->WaitOnThreadStage(AI_THREAD_1, AI_1_POSTRENDERACTIONS);
    QInstance()->WaitOnThreadStage(AI_THREAD_1, AI_1_DONE);
}

AILinearTaskThread::AILinearTaskThread(void (*apfnTaskFunc)()) : pfnTaskFunc(apfnTaskFunc) {}

AILinearTaskThread::~AILinearTaskThread() {}

// 0x32000 bytes: the thread's Havok stack, allocated from and returned to the router's heap.
void AILinearTaskThread::OnStartup() {
    hkMemorySystem::getInstance().threadInit(
        ThreadMemory,
        "AILinearTaskThread",
        hkFlags<hkMemorySystem::FlagBits, int32_t>(hkMemorySystem::FLAG_ROUTER)
    );
    hkBaseSystem::initThread(&ThreadMemory);
    ThreadMemory.setTemp(&ThreadMemory.heap());
    ThreadMemory.setSolver(&ThreadMemory.heap());
    pThreadStackBuffer = ThreadMemory.heap().blockAlloc(0x32000);
    ThreadMemory.m_stack.initMemory(pThreadStackBuffer, 0x32000);
    pDataHandler->CreateThreadSafeFiles();
}

void AILinearTaskThread::OnShutdown() {
    ThreadMemory.heap().blockFree(pThreadStackBuffer, 0x32000);
    hkBaseSystem::quitThread();
    hkMemorySystem::getInstance().threadQuit(
        ThreadMemory,
        hkFlags<hkMemorySystem::FlagBits, int32_t>(hkMemorySystem::FLAG_ROUTER)
    );
    ThreadMemory.m_stack.initMemory(nullptr, 0);
}

void AILinearTaskThread::RunLinearTasks() {
    if (pfnTaskFunc) {
        pfnTaskFunc();
    }
}
