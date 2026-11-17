#pragma once

#include "BSCore/BSSemaphore.h"
#include "BSCore/BSThread.h"
#include "BSCore/MemoryManager.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Reconstruction.hpp"

// A worker thread of the AI task manager: it waits on StartProcessing, runs its share of the AI tasks with its own
// Havok memory router and signals FinishedProcessing.
class AITaskThread : public BSThread {
public:
	AITaskThread(uint32_t aiThread);
	~AITaskThread() override;

	uint32_t ThreadProc() override;

	void OnStartup();
	void OnShutdown();
	void StartProcessingTasks();
	void WaitForCompletion();

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	BSSemaphore StartProcessing;
	BSSemaphore FinishedProcessing;
	bool bKillThread;
	uint8_t padding49[3];
	uint32_t iThread;
	hkMemoryRouter ThreadMemory;
	void* pThreadStackBuffer;
};
inline constexpr fnvd::Vtable AITaskThread_vftable{};
