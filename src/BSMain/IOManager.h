#pragma once

#include "BSMain/BSTaskManager.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

template <class T>
class LockFreeQueue;

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

// A task run by the IO manager; its priority is kept in the task's key. Run, Finish, Cancel and GetDescription come
// from BSTask.
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
    void SetIOTaskPriority(IO_TASK_PRIORITY aePriority) {
        SetPriority(aePriority);
    }
    void AddToPostProcessQueue();
    bool IsTaskFinished();
    bool IsCancelled();
    void SetCompleted();
    int64_t GetKey();
    // Gets the next task offset; GenerateKey passes the task, which the body stores.
    uint32_t GetNextOffset();
    // Gets the next task counter. Its one caller, GenerateKey, uses only the low 16 bits of the incremented value.
    uint16_t GetNextCounter();

    // The priority stored in the key arKey.
    static uint8_t GetPriorityFromKey(int64_t& ariKey);

    // The counter and offset the task keys are built from.
    static inline int32_t iStaticCounter;
    static inline int32_t iStaticOffset;
};

NiSmartPointer(IOTask);

// The task manager of the background file loads. Only some of its entry points are declared.
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
    // The external count callback's count, or 0 without a callback.
    uint32_t GetExternalCount() {
        return pGetExternalCountCallback ? pGetExternalCountCallback() : 0;
    }
    // Loads every queued priority now.
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
    void PauseThread(int32_t aiThread);
    void WaitForThread(int32_t aiThread);
    bool DoSleep();
    bool TryCancelTask(IOTask* apTask, void* apTaskOwner);
    // The thread ID of worker aiThread (BSTaskThread::iThreadID), returned by value.
    uint32_t GetTaskThreadID(int32_t aiThread);

    uint8_t padding05C[4];
    uint32_t iIOManagerThreadID;
    LockFreePriorityQueue<NiPointer<IOTask>>* pPostProcessQueue;
    IO_TASK_PRIORITY eLoadingQueuedPriority;
    // The extra count LoadQueuedPriority waits for (ModelLoader::GetExternalCountCallback).
    uint32_t (*pGetExternalCountCallback)();
    // Called by UpdateQueue after each post-processed task; null when not set.
    void (*pUpdateQueueCallback)();
    // Called by UpdateQueue before it pops the post-process queue; its result is UpdateQueue's result. The constructor
    // leaves it unwritten.
    bool (*pUpdateSecondaryQueueCallback)();
    // The constructor nulls it. Called with the progress in percent.
    void (*pLoadQueuedPriorityCallback)(float afPercent);
    // Called by Pause once DoSleep has paused the queue; null when not set.
    void (*pPauseCallback)();
    // Called by Resume once the task manager has resumed; takes no arguments and is null when not set.
    void (*pResumeCallback)();
    uint8_t padding084[0x1C];

    static inline bool bUpdating;
};

inline IOManager* pIOManager;

extern SettingT<INISettingCollection> iPostProcessMilliseconds;

extern SettingT<INISettingCollection> iPostProcessMillisecondsLoadingQueuedPriority;

inline IOTask::IOTask(IO_TASK_PRIORITY aePriority) {
    SetIOTaskPriority(aePriority);
}

// The key: file index 3, the next offset, the task's priority and the low 16 bits of the next counter, fetched in the
// order counter, priority, offset.
inline void IOTask::GenerateKey() {
    uint16_t sCounter = GetNextCounter();
    uint8_t cPriority = GetPriority();
    uint32_t iOffset = GetNextOffset();
    SetKey(3, iOffset, cPriority, sCounter);
}

inline uint8_t IOTask::GetPriority() {
    return IOTask::GetPriorityFromKey(Key);
}

inline void IOTask::SetKey(char acFileIndex, uint32_t aiOffset, uint8_t acPriority, uint16_t asCounter) {
    uint64_t uiPriorityBits = static_cast<uint64_t>(acPriority) << 16;
    Key = static_cast<int64_t>(
        (static_cast<uint64_t>(Key) & 0xFFFFFFFF00000000ull) |
        static_cast<uint64_t>(static_cast<uint32_t>(asCounter | (uiPriorityBits | aiOffset << 24)))
    );
    Key = static_cast<int64_t>(
        (static_cast<uint64_t>(Key) & 0xFFFFFFFFull) |
        (static_cast<uint64_t>(static_cast<uint32_t>(
             (static_cast<uint32_t>(uiPriorityBits >> 32) | (static_cast<uint64_t>(aiOffset) >> 8 | acFileIndex << 24))
         ))
         << 32)
    );
}

inline void IOTask::SetPriority(uint8_t acPriority) {
    Key = (static_cast<uint64_t>(acPriority) << 16) +
        ((static_cast<uint64_t>(static_cast<uint32_t>(Key >> 32)) << 32) | static_cast<uint32_t>(Key & 0xFF00FFFFu));
}
