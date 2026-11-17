#pragma once

#include "BSMain/BSTaskManager.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

template <class T> class LockFreeQueue;

// The IO manager's task priorities, most urgent first.
enum IO_TASK_PRIORITY : int32_t {
	IO_TASK_PRIORITY_CRITICAL = 0,
	IO_TASK_PRIORITY_VERY_HIGH = 1,
	IO_TASK_PRIORITY_HIGH = 2,
	IO_TASK_PRIORITY_MID = 3,
	IO_TASK_PRIORITY_LOW = 4,
	IO_TASK_PRIORITY_VERY_LOW = 5,
	IO_TASK_PRIORITY_COUNT = 6,
	IO_TASK_PRIORITY_INVALID = 7,
	IO_TASK_PRIORITY_HIGHEST = 0,
	IO_TASK_PRIORITY_LOWEST = 5,
};

// The kinds of file an IO task loads.
enum IO_FILE_INDEX : int32_t {
	IO_FILE_INDEX_TEXTURES = 0,
	IO_FILE_INDEX_MESHES = 1,
	IO_FILE_INDEX_SOUNDS = 2,
	IO_FILE_INDEX_MISC = 3,
	IO_FILE_INDEX_COUNT = 4,
};

// A task run by the IO manager; its priority is kept in the task's key.
// Run, Finish, Cancel and GetDescription (slots 1 to 4) come from BSTask.
class IOTask : public BSTask<int64_t> {
public:
	IOTask(IO_TASK_PRIORITY aePriority);

	// Runs on the main thread after Finish, for the work that cannot be done on the IO thread.
	virtual void PostProcess() {}
	// Builds the task's key from its counter and priority.
	virtual void GenerateKey();
	// Moves the task to the queue of aeNewPriority.
	virtual void Requeue(IO_TASK_PRIORITY aeNewPriority);

	// Sets the key of the task from its file index, offset, priority and counter.
	void SetKey(char acFileIndex, uint32_t aiOffset, uint8_t acPriority, uint16_t asCounter);
	// The priority stored in the task's key.
	uint8_t GetPriority();
	// Whether the task is in the pending state.
	bool IsPending();
	// The priority stored in the key.
	IO_TASK_PRIORITY GetIOTaskPriority();
	void SetPriority(uint8_t acPriority);
	void SetIOTaskPriority(IO_TASK_PRIORITY aePriority) { SetPriority(aePriority); }
	void AddToPostProcessQueue();
	bool IsTaskFinished();
	bool IsCancelled();
	void SetCompleted();
	int64_t GetKey();
	// Gets the next task offset (thiscall, ret 0, 0x43DC30; GenerateKey passes the task in
	// ecx, which the body stores).
	uint32_t GetNextOffset();
	// Gets the next task counter (thiscall, ret 0, 0x43DC10). The exe returns the atomic increment's whole eax; its
	// one caller, GenerateKey (0x43DB7F), zero-extends ax.
	uint16_t GetNextCounter();

	// The priority stored in the key arKey.
	static uint8_t GetPriorityFromKey(int64_t& ariKey);

	// The counter and offset the task keys are built from.
	static inline int32_t iStaticCounter;
	static inline int32_t iStaticOffset;
};

NiSmartPointer(IOTask);

// One lock-free queue per task priority (the PDB's template; LockFreePriorityQueue<NiPointer<IOTask>> is the exe's
// only instance, its vtable 0x10C16DC, IDA's name; no RTTI).
template <class T> class LockFreePriorityQueue {
public:
	LockFreePriorityQueue(uint32_t aiPriorityCount, uint32_t aiMaxThreads, uint32_t aiDeleteBatchSize);
	// Slot 0: the deleting destructor.
	virtual ~LockFreePriorityQueue();

	virtual void IncrementCount();
	virtual void DecrementCount();
	// The number of queued elements.
	virtual uint32_t GetCount();

	// Queues the element at aiPriority.
	void Insert(int32_t aiPriority, T* apTask);
	// Empties every priority queue.
	void Clear();
	bool Pop(T& arData);

	static void operator delete(void* apMemory);

	// The queues, one per priority (IDA: LockFreeQueue** ppQueues).
	LockFreeQueue<T>** pQueues;
	// IDA name uiSize; the PDB names it iPriorityCount.
	int32_t iPriorityCount;
	int32_t iCount;
};

// The task manager of the background file loads. Only the entry points the reconstructed code calls are declared.
class IOManager : public BSTaskManager<int64_t> {
public:
	IOManager();
	~IOManager() override;

	uint32_t GetBucketForKey(int64_t aiKey) override;
	bool AddTask(BSTask<int64_t>* apTask) override;
	// Cancels every task, then empties the post-process queue.
	void CancelAllTasks() override;

	// Moves apTask to the queue of priority aeNewPriority.
	bool RequeueTask(IOTask* apTask, IO_TASK_PRIORITY aeNewPriority);
	void Pause();
	// Resumes the paused tasks.
	void Resume();
	// Whether a queued priority is being loaded: eLoadingQueuedPriority is not
	// IO_TASK_PRIORITY_COUNT.
	bool IsLoadingQueuedPriority();
	// The number of tasks waiting for their post-processing.
	uint32_t GetPostProcessingCount();
	// Loads every queued priority now (thiscall, ret 0, 0x456520). Out of line: every exe caller calls 0x456520.
	void LoadAllQueuedPriorities();
	// Loads the queued tasks of aePriority and the more urgent ones now, reporting progress through
	// pLoadQueuedPriorityCallback.
	void LoadQueuedPriority(IO_TASK_PRIORITY aePriority);
	// Moves the tasks of priority aeOldPriority and above to aeNewPriority.
	void RequeuePrioritiesToNewPriority(IO_TASK_PRIORITY aeOldPriority, IO_TASK_PRIORITY aeNewPriority);
	// Runs one pass over the queues; true when it did work.
	bool UpdateQueue();
	void AddPostProcessTask(IOTask* apTask);
	void ForceHighestPriority();
	void PauseThread(int32_t a2);
	void WaitForThread(int32_t a2);
	bool DoSleep();
	bool TryCancelTask(IOTask* apTask, void* apTaskOwner);
	// IDA name; not in the PDB. The thread ID of worker a2 (BSTaskThread::iThreadID; the exe returns that dword
	// through the folded accessor 0x44DDC0, no pointer).
	uint32_t GetTaskThreadID(int32_t a2);

	uint8_t padding05C[4];
	uint32_t iIOManagerThreadID;
	LockFreePriorityQueue<NiPointer<IOTask>>* pPostProcessQueue;
	IO_TASK_PRIORITY eLoadingQueuedPriority;
	// The extra count LoadQueuedPriority waits for (ModelLoader::GetExternalCountCallback; cdecl, no arguments).
	uint32_t (*pGetExternalCountCallback)();
	// 0x70 called by UpdateQueue after each post-processed task (cdecl, no arguments; null when not set)
	void (*pUpdateQueueCallback)();
	// 0x74 called by UpdateQueue before it pops the post-process queue; its result is UpdateQueue's (cdecl, no
	// arguments; the constructor 0xC3DA50 leaves it unwritten)
	bool (*pUpdateSecondaryQueueCallback)();
	// PDB name; the constructor 0xC3DA50 nulls it. Called with the progress in percent (cdecl: the caller pops the
	// float).
	void (*pLoadQueuedPriorityCallback)(float afPercent);
	// 0x7C called by Pause once DoSleep has paused the queue (cdecl, no arguments; null when not set)
	void (*pPauseCallback)();
	// 0x80 called by Resume once the task manager has resumed (cdecl, no arguments; null when not set)
	void (*pResumeCallback)();
	uint8_t padding084[0x1C];

	static inline bool bUpdating;
};

inline constexpr fnvd::Vtable IOTask_vftable{};
inline constexpr fnvd::Vtable IOManager_vftable{};
inline IOManager* pIOManager;

// INISettingCollection: 0xfbb82b calls 0x44f570.
extern SettingT<INISettingCollection> gs_iPostProcessMilliseconds_BackgroundLoad;

// INISettingCollection: 0xfbb8cb calls 0x44f570.
extern SettingT<INISettingCollection> gs_iPostProcessMillisecondsLoadingQueuedPriority_BackgroundLoad;
inline constexpr fnvd::Vtable LockFreePriorityQueue_NiPointer_IOTask_vftable{};
