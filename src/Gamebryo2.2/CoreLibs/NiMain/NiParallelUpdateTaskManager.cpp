#include "Gamebryo2.2/CoreLibs/NiMain/NiParallelUpdateTaskManager.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiTask.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTaskManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTask_NiTQueue.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSemaphore.h"

NiRTTI NiParallelUpdateTaskManager::ms_RTTI("NiParallelUpdateTaskManager", &NiTaskManager::ms_RTTI);
NiRTTI NiParallelUpdateTaskManager::SignalTask::ms_RTTI("NiParallelUpdateTaskManager::SignalTask", &NiTask::ms_RTTI);

bool NiParallelUpdateTaskManager::AddTask(NiTask* pkTask, NiTaskManager::TaskPriority ePriority) {
    return AddTask_Internal(false, pkTask, ePriority, true);
}

NiParallelUpdateTaskManager* NiParallelUpdateTaskManager::Get() {
    return ms_pkManager;
}

bool NiParallelUpdateTaskManager::IsActive() {
    return NiParallelUpdateTaskManager::ms_pkManager && NiParallelUpdateTaskManager::ms_pkManager->m_bActive;
}

void NiParallelUpdateTaskManager::EndUpdate() {
    NiParallelUpdateTaskManager* v0 = NiParallelUpdateTaskManager::ms_pkManager;
    if (NiParallelUpdateTaskManager::ms_pkManager) {
        if (NiParallelUpdateTaskManager::ms_pkManager->GetNumTasks(NiTaskManager::LOW) ||
            NiParallelUpdateTaskManager::ms_pkManager->GetNumTasks(NiTaskManager::MEDIUM) ||
            NiParallelUpdateTaskManager::ms_pkManager->GetNumTasks(NiTaskManager::HIGH) ||
            v0->GetNumTasks(NiTaskManager::IMMEDIATE)) {
            v0->AddTask_Internal(true, &v0->m_kSignalTask, NiTaskManager::LOW, false);
            NiParallelUpdateTaskManager::ms_pkManager->m_bActive = false;
        }
        else {
            NiParallelUpdateTaskManager::SignalRenderSema();
            NiParallelUpdateTaskManager::ms_pkManager->m_bActive = false;
        }
    }
}

int32_t NiParallelUpdateTaskManager::WaitRenderSema(uint32_t aiTime) {
    if (NiParallelUpdateTaskManager::ms_pkManager &&
        (NiParallelUpdateTaskManager::ms_pkManager->m_kRenderSema.Wait(aiTime) == 1)) {
        NiParallelUpdateTaskManager::End();
        NiParallelUpdateTaskManager::ms_pkManager->m_kRenderSema.Wait(0xFFFFFFFFu);
    }

    return 0;
}

int32_t NiParallelUpdateTaskManager::SignalRenderSema() {
    if (NiParallelUpdateTaskManager::ms_pkManager) {
        return NiParallelUpdateTaskManager::ms_pkManager->m_kRenderSema.Signal();
    }

    return 0;
}

void NiParallelUpdateTaskManager::BeginUpdate() {
    if (NiParallelUpdateTaskManager::ms_pkManager) {
        NiParallelUpdateTaskManager::ms_pkManager->m_bActive = true;
        NiParallelUpdateTaskManager::WaitRenderSema(0xFFFFFFFFu);
    }
}

void NiParallelUpdateTaskManager::End() {
    bEnd = true;
}

// Stops the manager (its virtual Stop) and deletes it.
void NiParallelUpdateTaskManager::Shutdown() {
    if (NiParallelUpdateTaskManager::ms_pkManager) {
        NiParallelUpdateTaskManager::ms_pkManager->Stop();
        if (NiParallelUpdateTaskManager::ms_pkManager) {
            delete NiParallelUpdateTaskManager::ms_pkManager;
        }
        NiParallelUpdateTaskManager::ms_pkManager = nullptr;
    }
}

int32_t NiTaskManager::GetNumTasks(NiTaskManager::TaskPriority ePriority) {
    uint32_t m_uiHead = m_akTaskQueues[ePriority].m_uiHead;
    NiTask_NiTQueue* v3 = &m_akTaskQueues[ePriority];
    if (m_uiHead == -1) {
        return 0;
    }
    uint32_t m_uiTail = m_akTaskQueues[ePriority].m_uiTail;
    if (m_uiHead < m_uiTail) {
        return m_uiTail - m_uiHead;
    }

    return m_uiTail + (v3->m_uiAlloced - m_uiHead);
}
