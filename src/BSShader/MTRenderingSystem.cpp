#include "BSShader/MTRenderingSystem.h"

#include "BSCore/BSSemaphore.h"
#include "BSCore/BSString.h"
#include "BSCore/BSThread.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/BSCompoundFrustum.h"
#include "BSMain/BSCullingProcess.h"
#include "BSMain/Setting.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderUtil.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "SDK/MSVC/ehvec.h"

// The game settings this file defines, with their IDs and default values, in initialization order.
SettingT<INISettingCollection> iRenderingThread1HWThread("iRenderingThread1HWThread:General", 0);
SettingT<INISettingCollection> iRenderingThread2HWThread("iRenderingThread2HWThread:General", 1);
MTRenderingSystem MTRenderingSystem::Instance;

RenderingTaskThread::RenderingTaskThread(void (*apfnTaskFunc)()) : pfnTaskFunc(apfnTaskFunc) {}

// The two semaphore members' destructors close their handles.
RenderingTaskThread::~RenderingTaskThread() {}

void RenderingTaskThread::RunLinearTasks() {
    void (*pfnTaskFunc)() = this->pfnTaskFunc;
    if (pfnTaskFunc) {
        pfnTaskFunc();
    }
}

MTRenderingSystem::AccumTask::AccumTask() : spCamera(nullptr), spScene(nullptr), spAccum(nullptr) {}

MTRenderingSystem::MTRenderingSystem() : TaskStageSema{{0, 20}, {0, 20}, {0, 20}} {
    for (int32_t i = 0; i < 3; ++i) {
        TaskThreadArray[i] = nullptr;
        TaskThreadStage[i] = 0;
    }
}

void MTRenderingSystem::SetThreadStage(MTRenderingSystem::THREAD_INDEX aiThreadIndex, int32_t aiStage) {
    if (BSShaderManager::bMTRendering && aiThreadIndex < THREAD_COUNT) {
        BSSemaphore* v3 = &TaskStageSema[aiThreadIndex];
        v3->hSemaphore->Release();
        InterlockedIncrementInt32(&v3->iCount);
    }
}

void MTRenderingSystem::WaitOnThreadStage(MTRenderingSystem::THREAD_INDEX aiThreadIndex, int32_t aiStage) {
    if (BSShaderManager::bMTRendering && TaskThreadStage[aiThreadIndex] < aiStage) {
        BSSemaphore* v4 = &TaskStageSema[aiThreadIndex];
        do {
            v4->hSemaphore->Wait();
            InterlockedDecrementInt32(&v4->iCount);
            ++TaskThreadStage[aiThreadIndex];
        }
        while (TaskThreadStage[aiThreadIndex] < aiStage);
    }
}

void MTRenderingSystem::CreateThreads() {
    if (BSShaderManager::bMTRendering) {
        AddThread(
            THREAD_2,
            iRenderingThread2HWThread.uValue.i,
            &MTRenderingSystem::Thread2TaskFunc,
            "Rendering Task Thread 2",
            BSThread::MAINTHREAD_STACK_SIZE
        );
    }
}

void MTRenderingSystem::StartThreads() {
    if (BSShaderManager::bMTRendering) {
        for (int32_t i = 0; i < THREAD_COUNT; ++i) {
            TaskThreadStage[i] = 0;
        }
        for (uint32_t i = 0; i < 2; ++i) {
            RenderingTaskThread* pThread = TaskThreadArray[i];
            if (pThread) {
                pThread->StartProcessing.Signal();
            }
        }
    }
}

void MTRenderingSystem::WaitForThreads() {
    if (BSShaderManager::bMTRendering) {
        for (uint32_t i = 0; i < 2; ++i) {
            RenderingTaskThread* v2 = TaskThreadArray[i];
            if (v2) {
                int32_t* p_iCount = &v2->FinishedProcessing.iCount;
                v2->FinishedProcessing.hSemaphore->Wait();
                InterlockedDecrementInt32(p_iCount);
            }
        }
    }
}

void MTRenderingSystem::AddThread(
    MTRenderingSystem::THREAD_INDEX aiThreadIndex,
    uint32_t aiHWThread,
    void (*apfnTaskFunc)(),
    const char* apThreadName,
    BSThread::StackSize aeStackSize
) {
    if (aiThreadIndex <= MTRenderingSystem::THREAD_2 && !TaskThreadArray[aiThreadIndex]) {
        RenderingTaskThread* pThread = new RenderingTaskThread(apfnTaskFunc);
        TaskThreadArray[aiThreadIndex] = pThread;
        pThread->Initialize(aeStackSize, apThreadName, false);
        TaskThreadArray[aiThreadIndex]->SetThreadProcessor(aiHWThread);
        TaskThreadArray[aiThreadIndex]->SetThreadPriority(0);
        if (!apThreadName) {
            char DstBuf[260];
            BSsprintf(DstBuf, 260, "Rendering Task Thread DEF %i", aiThreadIndex);
            TaskThreadArray[aiThreadIndex]->SetName(DstBuf);
        }
    }
}

void MTRenderingSystem::AddAccumTask(
    NiCamera* apCamera,
    BSCompoundFrustum* apFrustum,
    NiAVObject* apScene,
    NiTPointerList<NiAVObject*>* apSceneList1,
    NiTPointerList<NiAVObject*>* apSceneList2,
    BSShaderAccumulator* apAccum,
    BSCullingProcess::BSCPCullingType akCullType,
    int32_t aiStage,
    int32_t aiBatchRendererIndex
) {
    if (BSShaderManager::bMTRendering) {
        AccumTask& rTask = pTaskArray[aiStage];
        rTask.spCamera = apCamera;
        BSCompoundFrustum** p_pFrustum = &rTask.pFrustum;
        *p_pFrustum = nullptr;
        rTask.spScene = apScene;
        rTask.pSceneList1 = apSceneList1;
        rTask.pSceneList2 = apSceneList2;
        rTask.spAccum = apAccum;
        rTask.kCullType = akCullType;
        rTask.iBatchRendererIndex = aiBatchRendererIndex;
        if (apFrustum) {
            BSCompoundFrustum* v17 = new BSCompoundFrustum;
            *p_pFrustum = v17;
            apFrustum->CopyMembers(v17);
        }
    }
    else {
        BSCullingProcess kculler(nullptr);
        if (apAccum) {
            kculler.spAccumulator.m_pObject = apAccum;
            NiIncRefCount(apAccum);
        }
        kculler.pCompoundFrustum = apFrustum;
        kculler.kCullMode = akCullType;
        if (apScene) {
            BSShaderUtil::AccumulateScene(apCamera, apScene, kculler);
        }
        if (apSceneList1) {
            BSShaderUtil::AccumulateSceneList(apCamera, *apSceneList1, kculler);
        }
        if (apSceneList2) {
            BSShaderUtil::AccumulateSceneList(apCamera, *apSceneList2, kculler);
        }
    }
}

void MTRenderingSystem::DestroyThreads() {
    if (BSShaderManager::bMTRendering) {
        for (uint32_t ithreadIdx = 0; ithreadIdx < 2; ++ithreadIdx) {
            RenderingTaskThread* v3 = TaskThreadArray[ithreadIdx];
            if (v3) {
                int32_t* p_iCount = &v3->StartProcessing.iCount;
                v3->bKillThread = true;
                v3->StartProcessing.hSemaphore->Release();
                InterlockedIncrementInt32(p_iCount);
                RenderingTaskThread* v5 = TaskThreadArray[ithreadIdx];
                v5->FinishedProcessing.hSemaphore->Wait();
                InterlockedDecrementInt32(&v5->FinishedProcessing.iCount);
                RenderingTaskThread* v6 = TaskThreadArray[ithreadIdx];
                if (v6) {
                    delete v6;
                }
                TaskThreadArray[ithreadIdx] = nullptr;
            }
        }
    }
}

// The culler is a local built and destroyed by BSCullingProcess's constructor and destructor.
void MTRenderingSystem::Thread2TaskFunc() {
    for (int32_t iStage = 1; iStage < 24; ++iStage) {
        Instance.WaitOnThreadStage(THREAD_1, iStage);
        AccumTask& rTask = Instance.pTaskArray[iStage];
        if (rTask.spAccum.m_pObject) {
            BSShaderAccumulator::iBatchRendererIndex = rTask.iBatchRendererIndex;
            BSCullingProcess kCuller(nullptr);
            BSCullingProcess* pCuller = &kCuller;
            BSShaderAccumulator* pAccum = rTask.spAccum.m_pObject;
            if (pAccum) {
                pCuller->spAccumulator.m_pObject = pAccum;
                NiIncRefCount(pAccum);
            }
            NiAVObject* pScene = rTask.spScene.m_pObject;
            pCuller->kCullMode = rTask.kCullType;
            pCuller->pCompoundFrustum = rTask.pFrustum;
            if (pScene) {
                BSShaderUtil::AccumulateScene(rTask.spCamera.m_pObject, pScene, *pCuller);
            }
            if (rTask.pSceneList1) {
                BSShaderUtil::AccumulateSceneList(rTask.spCamera.m_pObject, *rTask.pSceneList1, *pCuller);
            }
            if (rTask.pSceneList2) {
                BSShaderUtil::AccumulateSceneList(rTask.spCamera.m_pObject, *rTask.pSceneList2, *pCuller);
            }
            rTask.iBatchRendererIndex = 0;
            rTask.pSceneList1 = nullptr;
            rTask.pSceneList2 = nullptr;
            if (BSShaderAccumulator* pOldAccum = rTask.spAccum.m_pObject) {
                NiDecRefCount(pOldAccum);
                rTask.spAccum.m_pObject = nullptr;
            }
            if (NiCamera* pOldCamera = rTask.spCamera.m_pObject) {
                NiDecRefCount(pOldCamera);
                rTask.spCamera.m_pObject = nullptr;
            }
            if (NiAVObject* pOldScene = rTask.spScene.m_pObject) {
                NiDecRefCount(pOldScene);
                rTask.spScene.m_pObject = nullptr;
            }
            if (BSCompoundFrustum* pFrustum = rTask.pFrustum) {
                delete pFrustum;
                rTask.pFrustum = nullptr;
            }
            // The culler's destructor runs here, at the block's end.
        }
        if (BSShaderManager::bMTRendering) {
            Instance.TaskStageSema[1].Signal();
        }
    }
}

MTRenderingSystem* MTRenderingSystem::QInstance() {
    return &Instance;
}
