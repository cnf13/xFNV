#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Queue/hkQueue.h"
#include "Havok/Source/Common/Base/Thread/CriticalSection/hkCriticalSection.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJob.h"
#include "Havok/Source/Common/Base/Thread/Semaphore/hkSemaphoreBusyWait.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// The job types the queue dispatches (13 enumerators; HK_JOB_TYPE_HAVOK_MAX and HK_JOB_TYPE_USER_0 share 0xA).
// m_jobFuncs holds one entry per type below HK_JOB_TYPE_MAX (11).
enum hkJobType : int32_t {
    HK_JOB_TYPE_DYNAMICS = 0x0,
    HK_JOB_TYPE_COLLIDE = 0x1,
    HK_JOB_TYPE_COLLISION_QUERY = 0x2,
    HK_JOB_TYPE_RAYCAST_QUERY = 0x3,
    HK_JOB_TYPE_ANIMATION = 0x4,
    HK_JOB_TYPE_BEHAVIOR = 0x5,
    HK_JOB_TYPE_CLOTH = 0x6,
    HK_JOB_TYPE_PATHFINDING = 0x7,
    HK_JOB_TYPE_UNIT_TEST = 0x8,
    HK_JOB_TYPE_CHARACTER_PROXY = 0x9,
    HK_JOB_TYPE_HAVOK_MAX = 0xA,
    HK_JOB_TYPE_USER_0 = 0xA,
    HK_JOB_TYPE_MAX = 0xB,
};

// The hardware setup the queue was built for.
struct hkJobQueueHwSetup {
    enum CellRules : int32_t {
        PPU_CAN_TAKE_SPU_TASKS = 0x0,
        PPU_CANNOT_TAKE_SPU_TASKS = 0x1,
    };

    hkJobQueueHwSetup();
    ~hkJobQueueHwSetup();

    // Copies acSetup into this one.
    hkJobQueueHwSetup& operator=(const hkJobQueueHwSetup& acSetup);

    CellRules m_cellRules;
    hkBool m_noSpu;
    uint8_t padding05[3];
    int32_t m_numCpuThreads;
    hkArray<hkArray<int32_t>> m_threadIdsSharingCaches;
};

// What a queue is built from.
struct hkJobQueueCinfo {
    hkJobQueueHwSetup m_jobQueueHwSetup;
    int32_t m_maxNumJobTypes;
};

// Havok's queue of jobs for its worker threads.
class hkJobQueue {
public:
    // JOB_QUEUE_ENTRY_SIZE is 128 for 4-byte pointers and 256 for 8-byte pointers.
    enum : int32_t {
#if FNVD_64BIT
        JOB_QUEUE_ENTRY_SIZE = 0x100,
#else
        JOB_QUEUE_ENTRY_SIZE = 0x80,
#endif
        // The queues DynamicData holds (the SDK's MAX_NUM_QUEUES).
        MAX_NUM_QUEUES = 16,
    };

    enum WaitPolicy : int32_t {
        WAIT_UNTIL_ALL_WORK_COMPLETE = 0x0,
        WAIT_INDEFINITELY = 0x1,
    };

    enum JobPriority : int32_t {
        JOB_HIGH_PRIORITY = 0x0,
        JOB_LOW_PRIORITY = 0x1,
    };

    enum JobPopFuncResult : int32_t {
        POP_QUEUE_ENTRY = 0x0,
        DO_NOT_POP_QUEUE_ENTRY = 0x1,
    };

    enum JobCreationStatus : int32_t {
        JOB_CREATED = 0x0,
        NO_JOB_CREATED = 0x1,
    };

    // What getNextJob found.
    enum JobStatus : int32_t {
        JOB_INVALID = -1,
        GOT_NEXT_JOB = 0,
        NO_JOBS_AVAILABLE = 1,
        ALL_JOBS_FINISHED = 2,
        JOBS_AVAILABLE_BUT_NOT_FOR_CURRENT_ELF = 3,
    };
    // Whether getNextJob blocks.
    enum WaitStatus : int32_t {
        WAIT_FOR_NEXT_JOB = 0,
        DO_NOT_WAIT_FOR_NEXT_JOB = 1,
    };
    // How finishJob finishes a job: normally, or only by calling the job type's finish callback.
    enum FinishJobFlag : int32_t {
        FINISH_FLAG_NORMAL = 0,
        FINISH_FLAG_JUST_CALL_FINISH_CALLBACK = 1,
    };

    struct JobQueueEntry;
    struct JobQueueEntryInput;
    struct DynamicData;

    // Callbacks of a job type: a process function runs a job, the pop callback says whether the job taken leaves its
    // queue, the finish callback may create a follow-up job.
    using ProcessJobFunc = JobStatus (*)(hkJobQueue& queue, JobQueueEntry& jobInOut);
    using PopJobFunc =
        JobPopFuncResult (*)(hkJobQueue& queue, DynamicData* data, JobQueueEntry& jobIn, JobQueueEntry& jobOut);
    using FinishJobFunc = JobCreationStatus (*)(
        hkJobQueue& queue,
        DynamicData* data,
        const JobQueueEntry& jobIn,
        JobQueueEntryInput& newJobCreatedOut
    );

    // The callbacks of one job type: the process functions, how many there are, and the pop and finish callbacks.
    struct hkJobHandlerFuncs {
        ProcessJobFunc* m_processJobFuncs;
        int32_t m_numProcessJobFuncs;
        PopJobFunc m_popJobFunc;
        FinishJobFunc m_finishJobFunc;
    };

    // A custom job type's queue: the type, its sub-type and the queue it was given.
    struct CustomJobType {
        hkJobType m_jobType;
        uint8_t m_jobSubType;
        uint8_t m_queueId;
        uint8_t padding06[2];
    };

    // A custom job type the user set up for a thread.
    struct CustomJobTypeSetup {
        hkJobType m_jobType;
        uint8_t m_jobSubType;
        uint8_t padding05[3];
        int32_t m_threadId;
    };

    // One queue entry: the hkJob header and the job's raw bytes (each job type reads them as its own class).
    struct JobQueueEntry : public hkJob {
        uint8_t m_data[JOB_QUEUE_ENTRY_SIZE - sizeof(hkJob)];
    };

    // A job with the priority to queue it at. The entry is 16-aligned (hkJob's HK_ALIGN16), so m_job follows padding
    // after m_jobPriority.
    struct JobQueueEntryInput {
        hkPadSpu<uint32_t> m_jobPriority;
        uint8_t padding04[0xC];
        JobQueueEntry m_job;
    };

    // A ring buffer of queue entries.
    class Queue : public hkQueue<JobQueueEntry> {
    public:
        Queue();
    };

    // The queue state the critical section guards: per-type active job counts, the waiting threads and the queues.
    struct DynamicData {
        // The record is 16-aligned, which pads it after the end of m_jobQueue.
        alignas(16) int16_t m_numActiveJobs[11];
        uint8_t padding16[2];
        int32_t m_masterThreadFinishingFlags;
        WaitPolicy m_waitPolicy;
        hkBool m_outOfMemory;
        uint8_t padding21;
        uint16_t m_numThreadsWaiting[5];
        Queue m_jobQueue[16];
    };

    // Finishes assertBuf (when given) and takes the next job into ostr.
    JobStatus finishJobAndGetNextJob(const JobQueueEntry* oldJob, JobQueueEntry& jobOut, WaitStatus waitStatus);
    // Takes the next job into jobOut without finishing one.
    JobStatus getNextJob(JobQueueEntry& job, WaitStatus waitStatus);

    // Takes the queue's critical section and returns the dynamic data; dynamicDataStorage is a buffer the size of
    // DynamicData.
    DynamicData* lockQueue(char* dynamicDataStorage);
    // Leaves the queue's critical section.
    void unlockQueue(DynamicData* dynamicDataStorage);
    // Sets the flags the master thread is finishing with, releasing the master thread's semaphore when jobs wait on
    // it.
    void setMasterThreadFinishingFlags(int32_t flags);
    // Sets the wait policy; WAIT_UNTIL_ALL_WORK_COMPLETE releases every waiting thread.
    void setWaitPolicy(WaitPolicy waitPolicy);
    // Sets the capacity of one job type's queue and of every custom job's queue.
    void setQueueCapacityForJobType(hkJobType jobType, int32_t queueCapacity);
    // The capacity of the queues held in a CPU's cache; nothing on PC.
    void setQueueCapacityForCpuCache(int32_t queueCapacity);
    // Registers the process functions, the pop callback and the finish callback of one job type.
    void registerJobHandler(hkJobType jobId, hkJobHandlerFuncs jobHandlerFuncs);
    // Releases the threads waiting on the queue's semaphores.
    void releaseWaitingThreads(DynamicData* data);
    // Rebuilds the job query rules from the job types.
    void updateJobQueryRules();
    // Processes jobs until the queue has none left for this thread.
    JobStatus processAllJobs();
    hkJobQueue(const hkJobQueueCinfo& acinfo);
    ~hkJobQueue();
    hkJobQueue::WaitPolicy getWaitPolicy();
    int32_t getMasterThreadFinishingFlags();
    // Havok SDK signature (hkBase.lib, tied by bytes,reloc).
    int32_t getQueueIndexForJob(const hkJob& job);
    // Havok SDK signature (hkBase.lib, tied by bytes,reloc).
    void checkQueueAndReleaseOneWaitingThread(int32_t queueIndex, hkJobQueue::DynamicData* data);
    // Havok SDK signature (hkBase.lib, tied by bytes,reloc).
    void addJobQueueLocked(
        hkJobQueue::DynamicData* data,
        const hkJobQueue::JobQueueEntry& job,
        hkJobQueue::JobPriority priority
    );
    // Adds job at priority under the queue's critical section.
    void addJob(hkJobQueue::JobQueueEntry& job, hkJobQueue::JobPriority priority);
    // Counts the finished job of oldJobType off, adds jobInOut at priority and gets the next job into jobInOut.
    hkJobQueue::JobStatus finishAddAndGetNextJob(
        hkJobType oldJobType,
        hkJobQueue::JobPriority priority,
        hkJobQueue::JobQueueEntry& jobInOut,
        hkJobQueue::WaitStatus waitStatus
    );

    // The critical section that guards the dynamic data. Its 64-byte alignment makes the whole class 64-aligned.
    alignas(64) hkCriticalSection m_criticalSection;
    // The queue's dynamic data.
    DynamicData* m_data;
    // The callback that dispatches a job to its job type's pop handler (defaultPopDispatchFunc).
    PopJobFunc m_popJobFunc;
    // The callback that dispatches a finished job to its job type's finish handler (defaultFinishDispatchFunc).
    FinishJobFunc m_finishJobFunc;
    int32_t m_numJobTypes;
    int32_t m_cpuCacheQueuesBegin;
    int32_t m_cpuCustomQueuesBegin;
    int32_t m_cpuTypesQueuesBegin;
    int32_t m_numJobQueues;
    // The job types users register as custom queues (CUSTOM_JOB_TYPE_MAX is 2).
    CustomJobType m_customJobs[2];
    int32_t m_numCustomJobs;
    int32_t m_cpuSemaphoreBegin;
    int32_t m_directMapSemaphoreEnd;
    int32_t m_masterThreadQueue;
    // The hardware setup, whose first member is the CellRules enum.
    hkJobQueueHwSetup m_hwSetup;
    int32_t m_numSpuElfs;
    hkBool m_queryRulesAreUpdated;
    uint8_t padding75[3];
    // MAX_NUM_THREAD_TYPES (CUSTOM_JOB_TYPE_MAX + 2 + 1) semaphores.
    hkSemaphoreBusyWait* m_queueSemaphores[5];
    int32_t m_numQueueSemaphores;
    // [MAX_NUM_THREAD_TYPES][MAX_NUM_QUEUES + 2], the SDK's macros expanding to 5 x 18.
    int8_t m_nextQueueToGet[5][18];
    int8_t m_cpuThreadIndexToSemaphoreIndex[4];
    uint8_t padding_EE[2];
    // One entry per job type (HK_JOB_TYPE_MAX is 11).
    hkJobHandlerFuncs m_jobFuncs[11];
    hkArray<CustomJobTypeSetup> m_customJobSetup;
    void releaseOneWaitingThread(hkJobQueue::DynamicData* data);
    // Copies job (its m_size bytes) into a JobQueueEntry and adds it at priority under the queue's critical section.
    void addJob(hkJob& job, hkJobQueue::JobPriority priority);
    // Copies each job into an entry and queues it at priority under one lock.
    void addJobBatch(const hkArrayBase<hkJob*>& jobs, hkJobQueue::JobPriority priority);
    // Havok SDK signature (hkBase.lib, tied by bytes).
    void finishJob(const hkJobQueue::JobQueueEntry* oldJob, hkJobQueue::FinishJobFlag flag);
    // Takes the next job for this thread from its queues, or waits for one.
    hkJobQueue::JobStatus findJobInternal(
        int32_t queueIndexOfNewJob,
        hkJobQueue::DynamicData* data,
        hkJobQueue::WaitStatus waitStatus,
        hkJobQueue::JobQueueEntry& jobOut
    );
    void registerJobWithCpuThread(hkJobType jobType, uint8_t subType, int32_t threadId);
};

hkJobQueue::JobCreationStatus hkDefaultFinishJobFunc(
    hkJobQueue& queue,
    hkJobQueue::DynamicData* data,
    const hkJobQueue::JobQueueEntry& a3,
    hkJobQueue::JobQueueEntryInput& a4
);
hkJobQueue::JobPopFuncResult hkDefaultPopJobFunc(
    hkJobQueue& queue,
    hkJobQueue::DynamicData* data,
    hkJobQueue::JobQueueEntry& a3,
    hkJobQueue::JobQueueEntry& a4
);
// Dispatch functions, which the constructor makes the queue's pop and finish callbacks: each forwards to the pop or
// finish handler of jobIn's job type with its own four arguments.
hkJobQueue::JobPopFuncResult defaultPopDispatchFunc(
    hkJobQueue& queue,
    hkJobQueue::DynamicData* data,
    hkJobQueue::JobQueueEntry& jobIn,
    hkJobQueue::JobQueueEntry& jobOut
);
hkJobQueue::JobCreationStatus defaultFinishDispatchFunc(
    hkJobQueue& queue,
    hkJobQueue::DynamicData* data,
    const hkJobQueue::JobQueueEntry& jobIn,
    hkJobQueue::JobQueueEntryInput& newJobCreatedOut
);
void freeJobQueue(void* a1);

// The TLS slot holding the calling thread's Havok thread number.
extern uint32_t hkThreadNumber;
