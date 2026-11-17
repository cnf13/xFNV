#pragma once

#include "BSHavok/HavokThreadMemoryStruct.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Reconstruction.hpp"

class BSParallelArrayTask;
class BSParallelTaskThread;

// Runs tasks on the worker threads.
class BSParallelTaskManager {
public:
	// The task function the worker threads call with pData.
	typedef void (*TaskFunc)(void*);

	BSParallelTaskManager();
	~BSParallelTaskManager();

	// Runs apTaskFunc(apData) on the worker threads and waits for them.
	void RunTask(TaskFunc apTaskFunc, void* apData);
	// Runs apArrayTask on the worker threads until every item is done.
	void RunTask(BSParallelArrayTask* apArrayTask);
	// Runs the current task's items on the calling worker thread.
	void RunTask();
	// Starts the worker threads.
	void StartThreads();
	// Waits for the worker threads to finish the task.
	void WaitForThreads();

	// Frees through ourFree, as the exe's deleting destructor does.
	static void operator delete(void* apMemory);

	BSParallelTaskThread** pThreads;
	TaskFunc pTaskFunc;
	void* pData;
	bool bRunningTask;
	uint8_t padding0D[3];
};

// A worker thread of the parallel task manager: a linear task thread with its own Havok thread memory.
class BSParallelTaskThread : public BSLinearTaskThread {
public:
	// The exe inlines this constructor in BSParallelTaskManager's.
	explicit BSParallelTaskThread(BSParallelTaskManager* apManager);
	~BSParallelTaskThread() override;

	void OnStartup() override;
	void OnShutdown() override;
	void RunLinearTasks() override;

	// IDA name; not in the PDB. A TaskFunc: the exe passes it only as RunTask's task function, whose data is the
	// BSParallelTask to run.
	static void IteratorFunc(void* apTask);
	// Frees through MemoryManager::Deallocate, as the exe's deleting destructor does.
	static void operator delete(void* apMemory);

	BSParallelTaskManager* pManager;
	HavokThreadMemoryStruct HavokThreadMemory;
};

// The processor each worker thread runs on, indexed by thread (the exe reads [i*4+0x11AF714]; it starts one thread).
using BSParallelTaskManager_ThreadProcessorArray = uint32_t[1];
inline BSParallelTaskManager_ThreadProcessorArray aiThread{5};

// The engine's one task manager.
inline BSParallelTaskManager* pParallelTaskManager;
inline constexpr fnvd::Vtable BSParallelTaskThread_vftable{};
