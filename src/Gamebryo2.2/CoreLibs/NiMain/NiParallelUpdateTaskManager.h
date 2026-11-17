#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTask.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTaskManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSemaphore.h"
#include "Reconstruction.hpp"

class NiThread;

// The task manager that runs update tasks on its own thread. Only what callers reach is declared.
class NiParallelUpdateTaskManager : public NiTaskManager {
public:
	// The PDB's NiParallelUpdateTaskManager::ManagerProc, the thread procedure; only a pointer to it is needed.
	class ManagerProc;

	// The task that signals the render semaphore; the exe hands its address (0x19C) to AddTask_Internal (0xA81A80).
	// The PDB gives the class no members of its own, so it is NiTask's 0xC bytes. The exe never constructs one, so
	// its RTTI, its vtable and an out-of-line DoTask were eliminated: DoTask is the SDK's inline body.
	class SignalTask : public NiTask {
	public:
		void DoTask() {
			NiParallelUpdateTaskManager::Get()->SignalRenderSema();
			m_eStatus = COMPLETED;
		}

		static NiRTTI ms_RTTI;
	};

	~NiParallelUpdateTaskManager() override;

	// NiTaskManager's AddTask (slot 35), 0xA3A730. No exe vtable holds it (the exe has none of this class);
	// NiGeomMorpherController::Update (0xA3B080) calls it directly.
	bool AddTask(NiTask* pkTask, TaskPriority ePriority) override;
	// Vtable slot 40 (0xA0), the PDB's virtual AddTask_Internal(bAddingSignalTask, pkTask, ePriority,
	// abRendererDependent); thiscall, four dwords. Pure here: the exe has no vtable of the class and no body for the
	// slot. Guessed type: the return is AddTask's bool; EndUpdate ignores it.
	virtual bool AddTask_Internal(bool bAddingSignalTask, NiTask* pkTask, TaskPriority ePriority,
		bool abRendererDependent) = 0;

	// The manager, null when there is none. Defined out of line: the exe calls its copy at 0x683A60.
	static NiParallelUpdateTaskManager* Get();
	// True when the manager exists and its thread is running.
	static bool IsActive();
	// Each is cdecl and does nothing without a manager. EndUpdate finishes the update pass;
	// WaitRenderSema waits up to aiTime ms (-1: forever) on the render semaphore and returns 0; SignalRenderSema
	// signals it.
	static void EndUpdate();
	static int32_t WaitRenderSema(uint32_t aiTime);
	static int32_t SignalRenderSema();
	static void BeginUpdate();
	// Sets bEnd.
	static void End();
	// Stops the manager and deletes it, then clears ms_pkManager; does nothing without a manager.
	static void Shutdown();

	// The task queues at +8 are NiTaskManager::m_akTaskQueues.
	uint8_t padding48[0x138];
	// The PC layout is the Xbox PDB's with the 0x10-byte m_akRendererDependentQueue removed, which the PC build does
	// not have: every member from here on is 0x10 earlier than on Xbox, and the exe proves three of them
	// (m_kRenderSema at 0x190, the m_kSignalTask AddTask_Internal is handed at 0x19C, m_bActive at 0x1B0).
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
