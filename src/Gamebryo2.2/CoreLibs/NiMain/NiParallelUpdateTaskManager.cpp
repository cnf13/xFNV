#include "Gamebryo2.2/CoreLibs/NiMain/NiParallelUpdateTaskManager.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiTask.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTask_NiTQueue.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTaskManager.h"
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
		uint32_t m_uiTail;
		uint32_t v3;
		uint32_t v4;
		uint32_t v5;
		uint32_t v6;
		uint32_t m_uiHead = NiParallelUpdateTaskManager::ms_pkManager->m_akTaskQueues[0].m_uiHead;
		if ((m_uiHead != -1 &&
				((m_uiTail = NiParallelUpdateTaskManager::ms_pkManager->m_akTaskQueues[0].m_uiTail, m_uiHead < m_uiTail)
						? v3 = m_uiTail - m_uiHead
						: v3 = m_uiTail +
							(NiParallelUpdateTaskManager::ms_pkManager->m_akTaskQueues[0].m_uiAlloced - m_uiHead),
					v3)) ||
			(((v4 = NiParallelUpdateTaskManager::ms_pkManager->m_akTaskQueues[1].m_uiHead, v4 != -1) &&
				 ((v5 = NiParallelUpdateTaskManager::ms_pkManager->m_akTaskQueues[1].m_uiTail, v4 < v5)
						 ? v6 = v5 - v4
						 : v6 = v5 + (NiParallelUpdateTaskManager::ms_pkManager->m_akTaskQueues[1].m_uiAlloced - v4),
					 v6)) ||
				(NiParallelUpdateTaskManager::ms_pkManager->GetNumTasks(NiTaskManager::HIGH) ||
					v0->GetNumTasks(NiTaskManager::IMMEDIATE)))) {
			v0->AddTask_Internal(true, &v0->m_kSignalTask, NiTaskManager::LOW, false);
			NiParallelUpdateTaskManager::ms_pkManager->m_bActive = 0;
		} else {
			NiParallelUpdateTaskManager::SignalRenderSema();
			NiParallelUpdateTaskManager::ms_pkManager->m_bActive = 0;
		}
	}
}

int32_t NiParallelUpdateTaskManager::WaitRenderSema(uint32_t aiTime) {
	if (NiParallelUpdateTaskManager::ms_pkManager) {
		if (((int32_t)(&NiParallelUpdateTaskManager::ms_pkManager->m_kRenderSema)->Wait(aiTime)) == 1) {
			NiParallelUpdateTaskManager::End();
			(&NiParallelUpdateTaskManager::ms_pkManager->m_kRenderSema)->Wait(0xFFFFFFFFu);
		}
	}
	return 0;
}

int32_t NiParallelUpdateTaskManager::SignalRenderSema() {
	if (NiParallelUpdateTaskManager::ms_pkManager) {
		return (&NiParallelUpdateTaskManager::ms_pkManager->m_kRenderSema)->Signal();
	} else {
		return 0;
	}
}

void NiParallelUpdateTaskManager::BeginUpdate() {
	if (NiParallelUpdateTaskManager::ms_pkManager) {
		NiParallelUpdateTaskManager::ms_pkManager->m_bActive = (bool)1;
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
		NiParallelUpdateTaskManager::ms_pkManager = 0;
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
	} else {
		return m_uiTail + (v3->m_uiAlloced - m_uiHead);
	}
}
