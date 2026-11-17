#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTask_NiTQueue.h"
#include "Reconstruction.hpp"

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

	// The PDB's virtuals, in its slot order after NiObject's: AddTask 35 (0x8C), DoTasks 36, DoSingleTask 37,
	// Clear 38, Stop 39 (0x9C). The exe has no vtable of the class (no RTTI, never constructed): the slots are
	// proven by the exe's calls (NiParticleSystem::UpdateSystem 0xC1AE40 calls AddTask at +0x8C,
	// NiParallelUpdateTaskManager::Shutdown 0xA819E0 calls Stop at +0x9C) and the PDB's NiParallelUpdateTaskManager
	// bodies (Stop calls Clear at +0x98, AddTask calls AddTask_Internal at +0xA0). The exe has no body for any of
	// them, so they are pure here; the class's vtable is never emitted.
	~NiTaskManager() override;

	virtual bool AddTask(NiTask* pkTask, TaskPriority ePriority) = 0;
	virtual ManagerStatus DoTasks() = 0;
	virtual bool DoSingleTask(TaskPriority ePriority) = 0;
	virtual bool Clear() = 0;
	virtual ManagerStatus Stop() = 0;

	int32_t GetNumTasks(NiTaskManager::TaskPriority ePriority);

	// One queue per TaskPriority, held in place: GetNumTasks (0xA81A40) indexes 16-byte queues from +8, and
	// NiParallelUpdateTaskManager::EndUpdate (0xA81A80) reads queue 1's head at +0x20. IDA types them as four
	// NiTask_NiTQueue pointers.
	NiTask_NiTQueue m_akTaskQueues[4];

	static NiRTTI ms_RTTI;
};
