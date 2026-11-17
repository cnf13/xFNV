#pragma once

#include "Fallout Shared/LockFreeMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

class BSTaskManagerThread;

// The states a BSTask moves through.
enum BS_TASK_STATE : int32_t {
	BS_TASK_STATE_PENDING = 0,
	BS_TASK_STATE_QUEUED = 1,
	BS_TASK_STATE_MOVING = 2,
	BS_TASK_STATE_RUNNING = 3,
	BS_TASK_STATE_FINISHED = 4,
	BS_TASK_STATE_COMPLETED = 5,
	BS_TASK_STATE_CANCELED = 6,
	BS_TASK_STATE_FORCE_DWORD = 0x7FFFFFFF,
};

// A unit of work for a BSTaskManager: reference counted, with a state and the key the manager sorts it by. The virtual
// functions are the destructor, Run, Finish, Cancel and GetDescription, in that order. Cancel and GetDescription do
// nothing here.
template <class T> class BSTask {
public:
	BSTask();
	virtual ~BSTask();

	virtual void Run() = 0;
	virtual void Finish() = 0;
	// The first argument is the state the task was in, the second the task's owner (the manager's CancelTask
	// argument).
	virtual void Cancel(BS_TASK_STATE, void*) {}
	// Writes the task's description into its first argument (a buffer of the second's size).
	virtual bool GetDescription(char*, uint32_t) { return false; }

	// Takes a reference (an atomic increment of iRefCount).
	void IncRefCount();
	// Whether the task is in BS_TASK_STATE_COMPLETED.
	bool IsCompleted();
	// Releases a reference; the last one deletes the task through its destructor.
	void DecRefCount();
	// iRefCount.
	int32_t GetRefCount() { return iRefCount; }
	// Moves the task to eNewState if it is in eCurrentState; true when it did.
	bool SwitchState(BS_TASK_STATE eCurrentState, BS_TASK_STATE eNewState);
	// Puts the task in BS_TASK_STATE_CANCELED.
	void SetTaskCanceled();

	static void operator delete(void* apMemory);

	// The first field follows the vtable pointer at 8, the alignment of Key.
	volatile int32_t iRefCount;
	BS_TASK_STATE eState;
	T Key;
};

// The engine's NiPointer of a task takes and releases references through the task's IncRefCount and DecRefCount.
inline void NiIncRefCount(BSTask<int64_t>* apTask) {
	apTask->IncRefCount();
}

inline void NiDecRefCount(BSTask<int64_t>* apTask) {
	apTask->DecRefCount();
}

// The thread pool that runs BSTasks, keyed by T in its map of tasks. Its own virtuals follow LockFreeMap's 18
// slots: AddTask (18), RunTask (19), FinishTask (20), CancelAllTasks (21) and GetThreadForTask (22).
template <class T> class BSTaskManager : public LockFreeMap<T, NiPointer<BSTask<T>>> {
public:
	BSTaskManager(uint32_t aiNumInterfaceThreads, uint32_t aiNumTaskThreads, uint32_t aiNumTaskBuckets);
	~BSTaskManager() override;

	// Queues apTask; whether it was added.
	virtual bool AddTask(BSTask<T>* apTask);
	virtual void RunTask(BSTask<T>* apTask);
	// Returns void because its caller, BSTaskManagerThread::ThreadUpdate, does not read a result.
	virtual void FinishTask(BSTask<T>* apTask);
	// Cancels every task in the map.
	virtual void CancelAllTasks();

private:
	// The index in pThreads of the thread that runs apTask.
	virtual uint32_t GetThreadForTask(BSTask<T>* apTask);

public:
	// Wakes the manager's threads.
	void ForceSignal();
	// Tells the worker threads to leave their loops: sets bExit.
	void SetThreadExiting() { bExit = true; }
	// Cancels apTask, whatever state it is in.
	void CancelTask(BSTask<T>* apTask, void* apTaskOwner);
	// The number of tasks in the map.
	int32_t GetQueuedCount();
	// The number of tasks in the buckets 0 to aiBucket.
	uint32_t GetCumulativeBucketCount(uint32_t aiBucket);
	// Takes apTask out of the map; true when it was there.
	bool RemoveTask(BSTask<T>* apTask);
	// Takes apTask out of the map, gives it aNewKey and queues it again; false while the task is running or being
	// moved.
	bool RequeueTask(BSTask<T>* apTask, int64_t aNewKey);
	// Sleeps until apTask is neither running nor completed.
	void WaitForTask(BSTask<T>* apTask);
	// Releases one pause; the last one resumes the paused worker threads and returns true.
	bool Resume();

	static void operator delete(void* apMemory);

	// Set by SetThreadExiting so the worker threads leave their loops.
	bool bExit;
	uint8_t padding41[3];
	int32_t iNumThreadsPaused;
	int32_t iPauseCount;
	uint32_t iNumTaskThreads;
	// The worker threads.
	BSTaskManagerThread** pThreads;
	// The number of tasks per bucket.
	uint32_t* pBucketCounts;
	int32_t iBucketCountTotal;
};
// The iterator a BSTaskManager walks its tasks with. It adds nothing to the base.
template <class T> class BSTaskManagerIterator : public LockFreeMap<T, NiPointer<BSTask<T>>>::LockFreeMapIterator {};

// The task map's per-thread interface.
using BSTaskMapInterface = LockFreeMap<int64_t, NiPointer<BSTask<int64_t>>>::LockFreeMapInterface;
using BSTaskMapHashListNode = LockFreeMap<int64_t, NiPointer<BSTask<int64_t>>>::HashListNode;

// Header static: each file that includes this header gets its own zero-initialised copy. Only one copy is read, the one
// BSTask<__int64>'s constructor and destructor count with. Every other copy is unreferenced.
static volatile int32_t iTotalBSTaskCount = 0;
