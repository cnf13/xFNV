#pragma once

#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Reconstruction.hpp"

// An AI worker thread with its own Havok memory router and stack buffer: it runs the task function it was given.
class AILinearTaskThread : public BSLinearTaskThread {
public:
	AILinearTaskThread(void (*apfnTaskFunc)());
	~AILinearTaskThread() override;

	void OnStartup() override;
	void OnShutdown() override;
	void RunLinearTasks() override;

	// The task the thread runs each frame; the exe calls it with no arguments (cdecl).
	void (*pfnTaskFunc)();
	hkMemoryRouter ThreadMemory;
	void* pThreadStackBuffer;
};
inline constexpr fnvd::Vtable AILinearTaskThread_vftable{};
