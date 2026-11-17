#pragma once

#include "BSCore/BSSemaphore.h"
#include "BSMain/Setting.h"
#include "BSShader/BSShaderAccumulator.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "BSMain/BSCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class BSCompoundFrustum;

// A worker thread of the render system: runs its task function once per frame when it is released.
class RenderingTaskThread : public BSLinearTaskThread {
public:
	RenderingTaskThread(void (*apfnTaskFunc)());
	~RenderingTaskThread() override;

	void OnStartup() override {}
	void OnShutdown() override {}
	void RunLinearTasks() override;

	void (*pfnTaskFunc)();
};

// The render worker threads: the accumulation tasks they run and the stage each thread has reached.
class MTRenderingSystem {
public:
	// The rendering threads (the PDB's; SetThreadStage 0xBA30F0 compares the index signed, `cmp eax, 2; jge`).
	enum THREAD_INDEX : int32_t {
		THREAD_1 = 0,
		THREAD_2 = 1,
		THREAD_COUNT = 2,
	};

	// One queued accumulation of a scene for a worker thread, at the stage that indexes it in pTaskArray.
	struct AccumTask {
		AccumTask();

		NiPointer<NiCamera> spCamera;
		BSCompoundFrustum* pFrustum;
		NiPointer<NiAVObject> spScene;
		NiTPointerList<NiAVObject*>* pSceneList1;
		NiTPointerList<NiAVObject*>* pSceneList2;
		NiPointer<BSShaderAccumulator> spAccum;
		BSCullingProcess::BSCPCullingType kCullType;
		uint32_t iBatchRendererIndex;
	};

	MTRenderingSystem();

	// Moves a thread to a stage. thiscall, ret 8.
	void SetThreadStage(MTRenderingSystem::THREAD_INDEX aiThreadIndex, int32_t aiStage);
	// Blocks until the thread reaches the stage. thiscall, ret 8.
	void WaitOnThreadStage(MTRenderingSystem::THREAD_INDEX aiThreadIndex, int32_t aiStage);
	// Creates the second rendering task thread when multithreaded rendering is on.
	void CreateThreads();
	// thiscall, ret 0. StartThreads releases the worker threads for a frame; WaitForThreads
	// blocks until every worker has finished it (0xBA2F30, 0xBA2FA0).
	void StartThreads();
	void WaitForThreads();
	// Creates the worker thread aiThreadIndex (0 or 1) running apfnTaskFunc on hardware
	// thread aiHWThread.
	void AddThread(MTRenderingSystem::THREAD_INDEX aiThreadIndex, uint32_t aiHWThread, void (*apfnTaskFunc)(),
		const char* apThreadName, BSThread::StackSize aeStackSize);
	// Queues an accumulation of a scene for a worker thread. thiscall, ret 0x24.
	void AddAccumTask(NiCamera* apCamera, BSCompoundFrustum* apFrustum, NiAVObject* apScene,
		NiTPointerList<NiAVObject*>* apSceneList1, NiTPointerList<NiAVObject*>* apSceneList2,
		BSShaderAccumulator* apAccum, BSCullingProcess::BSCPCullingType akCullType, int32_t aiStage,
		int32_t aiBatchRendererIndex);
	void DestroyThreads();

	// The worker task of the second rendering thread: for each stage after the first, waits for thread 0 to reach
	// it, runs the stage's queued accumulation and clears it, then signals the stage semaphore.
	static void Thread2TaskFunc();
	// The singleton.
	static MTRenderingSystem* QInstance();

	RenderingTaskThread* TaskThreadArray[3];
	int32_t TaskThreadStage[3];
	BSSemaphore TaskStageSema[3];
	// The task the second thread runs at each stage; Thread2TaskFunc walks stages 1 to 23.
	AccumTask pTaskArray[24];
};

inline constexpr fnvd::Vtable RenderingTaskThread_vftable{};

// The PDB's static MTRenderingSystem::Instance, overlaid outside the class because a static member of the class's own
// type cannot be declared inside it.
extern MTRenderingSystem MTRenderingSystem_Instance;

// The game settings MTRenderingSystem.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> iRenderingThread1HWThread;
extern SettingT<INISettingCollection> iRenderingThread2HWThread;
