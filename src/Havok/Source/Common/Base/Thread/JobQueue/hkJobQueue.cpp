#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/Queue/hkQueue.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/System/Hardware/hkHardwareInfo.h"
#include "Havok/Source/Common/Base/System/hkBaseSystem.h"
#include "Havok/Source/Common/Base/Thread/CriticalSection/hkCriticalSection.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJob.h"
#include "Havok/Source/Common/Base/Thread/Semaphore/hkSemaphore.h"
#include "Havok/Source/Common/Base/Thread/Semaphore/hkSemaphoreBusyWait.h"
#include "Havok/Source/Common/Base/Thread/Thread/hkThreadLocalData.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "SDK/MSVC/string.h"
#include "SDK/Windows/winbase.h"

#include <new>

uint32_t hkThreadNumber = TlsAlloc();

hkJobQueueHwSetup::hkJobQueueHwSetup() : m_cellRules(PPU_CANNOT_TAKE_SPU_TASKS), m_noSpu(true) {
    hkHardwareInfo info;
    hkGetHardwareInfo(info);
    m_numCpuThreads = info.m_numThreads;
}

// The member array frees itself and its inner arrays.
hkJobQueueHwSetup::~hkJobQueueHwSetup() {}

// Member-wise copy. The member array's operator= is copyFromArray for a class element type: _reserve, then the surplus
// inner arrays' clearAndDeallocate, the shared ones' operator= (copyFromArray for plain data), and the copy constructor
// for the new ones.
hkJobQueueHwSetup& hkJobQueueHwSetup::operator=(const hkJobQueueHwSetup& acSetup) {
    m_cellRules = acSetup.m_cellRules;
    m_noSpu.m_bool = acSetup.m_noSpu.m_bool;
    m_numCpuThreads = acSetup.m_numCpuThreads;

    // EXE-INLINED none: hkArray<hkArray<int>>::operator=
    hkArray<hkArray<int32_t>>& caches = m_threadIdsSharingCaches;
    const hkArray<hkArray<int32_t>>& from = acSetup.m_threadIdsSharingCaches;
    hkMemoryAllocator& alloc = hkContainerHeapAllocator().get(&caches);
    int32_t oldSize = caches.m_size;
    int32_t newSize = from.m_size;
    int32_t copiedSize = newSize > oldSize ? oldSize : newSize;
    caches._reserve(alloc, newSize);
    hkArrayUtil::destruct(caches.m_data + newSize, oldSize - newSize, hkTypeIsClass());
    for (int32_t i = 0; i < copiedSize; ++i) {
        caches.m_data[i] = from.m_data[i];
    }
    // The constructWithArray loop runs over an element count computed with a wrapping subtraction, not over an index
    // range.
    // EXE-INLINED none: hkArrayUtil::constructWithArray<hkArray<int>>
    hkArray<int32_t>* pNew = caches.m_data + copiedSize;
    const hkArray<int32_t>* pFrom = from.m_data + copiedSize;
    int32_t numNew = static_cast<int32_t>(static_cast<uint32_t>(newSize) - static_cast<uint32_t>(copiedSize));
    for (int32_t i = 0; i < numNew; ++i) {
        hkArray<int32_t>* pDst = &pNew[i];
        const hkArray<int32_t>& src = pFrom[i];
        // Placement new's null test.
        if (pDst) {
            // EXE-INLINED none: hkArray<int>::hkArray(const hkArray&)
            pDst->m_data = nullptr;
            pDst->m_size = 0;
            pDst->m_capacityAndFlags = hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG;
            int32_t n = src.m_size;
            hkMemoryAllocator& a = hkContainerHeapAllocator().get(pDst);
            int32_t* p = n ? a._bufAlloc<int32_t>(n) : nullptr;
            int32_t cap = n ? n : hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG;
            pDst->m_data = p;
            pDst->m_size = n;
            pDst->m_capacityAndFlags = cap;
            hkArrayUtil::constructWithArray(pDst->m_data, pDst->m_size, src.m_data, hkTypeIsPod());
        }
    }
    caches.m_size = newSize;
    return *this;
}

hkJobQueue::Queue::Queue() {}

// Finishes oldJob (the job just done; null when there is none) and takes the next job into jobOut. The finish callback
// may create a follow-up job, which is queued (the queue's enqueue or enqueueInFront) before the job is counted off.
// findJobInternal unlocks the queue; JOB_INVALID means it found no job yet and the queue is locked again, with nothing
// left to finish.
hkJobQueue::JobStatus hkJobQueue::finishJobAndGetNextJob(
    const hkJobQueue::JobQueueEntry* oldJob,
    hkJobQueue::JobQueueEntry& jobOut,
    hkJobQueue::WaitStatus waitStatus
) {
    HK_TIMER_BEGIN("GetNextJob", nullptr);
    // The SDK's lock storage: a 16-aligned byte buffer lockQueue builds the DynamicData in.
    alignas(16) char dynamicDataStorage[sizeof(DynamicData)];
    JobStatus result;
    do {
        DynamicData* data = lockQueue(dynamicDataStorage);
        int32_t queueIndexOfNewJob = -1;
        if (oldJob) {
            JobQueueEntryInput newJob;
            if (m_finishJobFunc(*this, data, *oldJob, newJob) == JOB_CREATED) {
                queueIndexOfNewJob = getQueueIndexForJob(newJob.m_job);
                Queue& queue = data->m_jobQueue[queueIndexOfNewJob];
                if (newJob.m_jobPriority.m_storage == JOB_HIGH_PRIORITY) {
                    queue.enqueueInFront(newJob.m_job);
                }
                else {
                    queue.enqueue(newJob.m_job);
                }
            }
            --data->m_numActiveJobs[oldJob->m_jobType.m_storage];
            oldJob = nullptr;
        }
        result = findJobInternal(queueIndexOfNewJob, data, waitStatus, jobOut);
    }
    while (result == JOB_INVALID);
    HK_TIMER_END();
    return result;
}

hkJobQueue::JobStatus hkJobQueue::getNextJob(JobQueueEntry& job, WaitStatus waitStatus) {
    return finishJobAndGetNextJob(nullptr, job, waitStatus);
}

// Takes the queue's critical section and returns the dynamic data.
hkJobQueue::DynamicData* hkJobQueue::lockQueue(char* dynamicDataStorage) {
    EnterCriticalSection(&m_criticalSection.m_section);
    return m_data;
}

// Leaves the queue's critical section. dynamicDataStorage is unused.
void hkJobQueue::unlockQueue(DynamicData* dynamicDataStorage) {
    LeaveCriticalSection(&m_criticalSection.m_section);
}

// Stores the flags the master thread is finishing with, then releases the master thread's semaphore once when a thread
// waits on it. The flags go through m_data, the wait count through lockQueue's result, as the game does.
// m_cpuThreadIndexToSemaphoreIndex[0] is reloaded at each of its three uses.
void hkJobQueue::setMasterThreadFinishingFlags(int32_t flags) {
    // The SDK's lock storage: a 16-aligned byte buffer lockQueue builds the DynamicData in.
    alignas(16) char dynamicDataStorage[sizeof(DynamicData)];
    DynamicData* pData = lockQueue(dynamicDataStorage);
    m_data->m_masterThreadFinishingFlags = flags;
    if (pData->m_numThreadsWaiting[m_cpuThreadIndexToSemaphoreIndex[0]] > 0) {
        --pData->m_numThreadsWaiting[m_cpuThreadIndexToSemaphoreIndex[0]];
        // The busy-wait semaphore is passed to the hkSemaphore form of release as it is.
        hkSemaphore::release(m_queueSemaphores[m_cpuThreadIndexToSemaphoreIndex[0]], 1);
    }
    unlockQueue(pData);
}

// Sets the wait policy through m_data; WAIT_UNTIL_ALL_WORK_COMPLETE releases every thread waiting on the queue.
void hkJobQueue::setWaitPolicy(WaitPolicy waitPolicy) {
    // The SDK's lock storage: a 16-aligned byte buffer lockQueue builds the DynamicData in.
    alignas(16) char dynamicDataStorage[sizeof(DynamicData)];
    DynamicData* pData = lockQueue(dynamicDataStorage);
    m_data->m_waitPolicy = waitPolicy;
    if (waitPolicy == WAIT_UNTIL_ALL_WORK_COMPLETE) {
        releaseWaitingThreads(pData);
    }
    unlockQueue(pData);
}

// Sets the capacity of the queue of one job type, and of the queue of every custom job of that type.
void hkJobQueue::setQueueCapacityForJobType(hkJobType jobType, int32_t queueCapacity) {
    // The SDK's lock storage: a 16-aligned byte buffer lockQueue builds the DynamicData in.
    alignas(16) char dynamicDataStorage[sizeof(DynamicData)];
    DynamicData* pData = lockQueue(dynamicDataStorage);
    pData->m_jobQueue[m_cpuTypesQueuesBegin + jobType].setCapacity(queueCapacity);
    for (int32_t i = 0; i < m_numCustomJobs; ++i) {
        pData->m_jobQueue[m_customJobs[i].m_queueId].setCapacity(queueCapacity);
    }
    unlockQueue(pData);
}

void hkJobQueue::setQueueCapacityForCpuCache(int32_t queueCapacity) {
    ;
}

void hkJobQueue::registerJobHandler(hkJobType jobId, hkJobQueue::hkJobHandlerFuncs jobHandlerFuncs) {
    m_jobFuncs[jobId] = jobHandlerFuncs;
}

// Releases the threads waiting on the queue's semaphores.
void hkJobQueue::releaseWaitingThreads(hkJobQueue::DynamicData* data) {
    hkJobQueue* pThis = this;
    int32_t v2 = 0;
    hkJobQueue* v6 = pThis;
    if (pThis->m_numQueueSemaphores > 0) {
        hkSemaphore** m_queueSemaphores = pThis->m_queueSemaphores;
        uint16_t* m_numThreadsWaiting = data->m_numThreadsWaiting;
        do {
            int32_t v5 = *m_numThreadsWaiting;
            *m_numThreadsWaiting = 0;
            if (v5) {
                do {
                    hkSemaphore::release(*m_queueSemaphores, 1);
                    --v5;
                }
                while (v5 > 0);
                pThis = v6;
            }
            ++v2;
            ++m_numThreadsWaiting;
            ++m_queueSemaphores;
        }
        while (v2 < pThis->m_numQueueSemaphores);
    }
}

// Rebuilds the job query rules: where each kind of queue begins, which semaphore each CPU thread waits on, the order
// in which each semaphore's threads look through the queues, the custom job types' queues, and the semaphores.
void hkJobQueue::updateJobQueryRules() {
    hkArray<hkArray<int32_t>>& caches = m_hwSetup.m_threadIdsSharingCaches;
    int32_t iNumCacheSize = caches.m_size;
    int32_t numSharedCaches = 1;
    if (iNumCacheSize) {
        numSharedCaches = iNumCacheSize;
    }
    int32_t iNumCustomJobs = m_customJobSetup.m_size;
    m_numCustomJobs = iNumCustomJobs;
    if (!m_hwSetup.m_noSpu.m_bool) {
        m_cpuCacheQueuesBegin = m_numJobTypes;
    }
    else {
        m_cpuCacheQueuesBegin = 0;
    }
    int32_t iCacheQueuesBegin = m_cpuCacheQueuesBegin;
    m_cpuCustomQueuesBegin = iCacheQueuesBegin + numSharedCaches;
    int32_t iTypesQueuesBegin = m_cpuCustomQueuesBegin + iNumCustomJobs;
    m_cpuSemaphoreBegin = iCacheQueuesBegin;
    m_cpuTypesQueuesBegin = iTypesQueuesBegin;
    m_numJobQueues = iTypesQueuesBegin + m_numJobTypes;
    m_directMapSemaphoreEnd = iTypesQueuesBegin;

    // Each CPU thread waits on the semaphore of the cache it shares, or of the one cache queue.
    if (iNumCacheSize > 0) {
        for (int32_t i = 0; i < m_hwSetup.m_numCpuThreads; ++i) {
            m_cpuThreadIndexToSemaphoreIndex[i] = -1;
        }
        for (int32_t c = 0; c < caches.m_size; ++c) {
            for (int32_t j = 0; j < caches.m_data[c].m_size; ++j) {
                m_cpuThreadIndexToSemaphoreIndex[caches.m_data[c].m_data[j]] =
                    static_cast<int8_t>(m_cpuCacheQueuesBegin + c);
            }
        }
    }
    if (caches.m_size == 0) {
        for (int32_t i = 0; i < m_hwSetup.m_numCpuThreads; ++i) {
            m_cpuThreadIndexToSemaphoreIndex[i] = static_cast<int8_t>(m_cpuCacheQueuesBegin);
        }
    }

    // The queues each cache's threads look through: its own, the other caches', the CPU type queues, the SPU type
    // queues where the PPU may take SPU tasks, then the custom queues; -1 ends the list.
    for (int32_t m = 0; m < numSharedCaches; ++m) {
        int8_t* pQueue = m_nextQueueToGet[m_cpuCacheQueuesBegin + m];
        *pQueue++ = static_cast<int8_t>(m_cpuCacheQueuesBegin + m);
        for (int32_t c = 0; c < caches.m_size; ++c) {
            if (m != c) {
                *pQueue++ = static_cast<int8_t>(m_cpuCacheQueuesBegin + c);
            }
        }
        for (int32_t t = 0; t < m_numJobTypes; ++t) {
            *pQueue++ = static_cast<int8_t>(m_cpuTypesQueuesBegin + t);
        }
        if (m_hwSetup.m_cellRules == hkJobQueueHwSetup::PPU_CAN_TAKE_SPU_TASKS) {
            for (int32_t t = 0; t < m_numJobTypes; ++t) {
                *pQueue++ = static_cast<int8_t>(t);
            }
        }
        for (int32_t k = 0; k < m_numCustomJobs; ++k) {
            *pQueue++ = static_cast<int8_t>(m_cpuCustomQueuesBegin + k);
        }
        *pQueue = -1;
    }

    // Each custom job type gets its own queue and semaphore: its thread moves to that semaphore, which looks through
    // the custom queue first and then the thread's former semaphore's list without the custom queue.
    for (int32_t i = 0; i < m_numCustomJobs; ++i) {
        CustomJobType& job = m_customJobs[i];
        job.m_jobType = m_customJobSetup.m_data[i].m_jobType;
        job.m_jobSubType = m_customJobSetup.m_data[i].m_jobSubType;
        job.m_queueId = static_cast<uint8_t>(m_cpuCustomQueuesBegin + i);
        int32_t iThreadId = m_customJobSetup.m_data[i].m_threadId;
        int32_t iOldSemaphore = m_cpuThreadIndexToSemaphoreIndex[iThreadId];
        m_cpuThreadIndexToSemaphoreIndex[iThreadId] = static_cast<int8_t>(job.m_queueId);
        int8_t* pQueue = m_nextQueueToGet[m_cpuCustomQueuesBegin + i];
        *pQueue++ = static_cast<int8_t>(m_cpuCustomQueuesBegin + i);
        for (const int8_t* pOld = m_nextQueueToGet[iOldSemaphore]; *pOld != -1; ++pOld) {
            if (*pOld != job.m_queueId) {
                *pQueue++ = *pOld;
            }
        }
        *pQueue = -1;
    }

    // The master thread gets a semaphore of its own when another thread shares its semaphore.
    int8_t cMasterQueue = m_cpuThreadIndexToSemaphoreIndex[0];
    m_masterThreadQueue = cMasterQueue;
    int32_t iNumSemaphores = m_cpuTypesQueuesBegin;
    for (int32_t i = 1; i < m_hwSetup.m_numCpuThreads; ++i) {
        if (m_cpuThreadIndexToSemaphoreIndex[i] == cMasterQueue) {
            m_cpuThreadIndexToSemaphoreIndex[0] = static_cast<int8_t>(iNumSemaphores);
            ++iNumSemaphores;
            break;
        }
    }

    if (m_queryRulesAreUpdated.m_bool) {
        for (int32_t i = 0; i < m_numQueueSemaphores; ++i) {
            hkSemaphoreBusyWait* pSemaphore = m_queueSemaphores[i];
            if (pSemaphore) {
                pSemaphore->~hkSemaphore();
                hkMemoryRouter::getInstance().m_heap->blockFree(pSemaphore, sizeof(hkSemaphoreBusyWait));
            }
        }
    }
    m_numQueueSemaphores = iNumSemaphores;
    for (int32_t i = 0; i < m_numQueueSemaphores; ++i) {
        m_data->m_numThreadsWaiting[i] = 0;
        void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkSemaphoreBusyWait));
        hkSemaphoreBusyWait* pSemaphore = nullptr;
        if (pMemory) {
            pSemaphore = ::new (pMemory) hkSemaphore(0, 1000);
        }
        m_queueSemaphores[i] = pSemaphore;
    }
    m_queryRulesAreUpdated.m_bool = 1;
}

// The SDK's processAllJobs: takes jobs until getNextJob finds none, timing each under its job type's name and running
// it through its type's process function for its sub-type.
hkJobQueue::JobStatus hkJobQueue::processAllJobs() {
    JobQueueEntry job;
    JobStatus jobStatus = getNextJob(job, WAIT_FOR_NEXT_JOB);
    while (jobStatus == GOT_NEXT_JOB) {
        switch (job.m_jobType.m_storage) {
            case HK_JOB_TYPE_DYNAMICS:
            case HK_JOB_TYPE_COLLIDE: HK_TIMER_BEGIN("Physics", nullptr); break;
            case HK_JOB_TYPE_COLLISION_QUERY: HK_TIMER_BEGIN("Collision Query", nullptr); break;
            case HK_JOB_TYPE_RAYCAST_QUERY: HK_TIMER_BEGIN("RayCast Query", nullptr); break;
            case HK_JOB_TYPE_ANIMATION: HK_TIMER_BEGIN("Animation", nullptr); break;
            case HK_JOB_TYPE_BEHAVIOR: HK_TIMER_BEGIN("Behavior", nullptr); break;
            case HK_JOB_TYPE_CLOTH: HK_TIMER_BEGIN("Cloth", nullptr); break;
            case HK_JOB_TYPE_PATHFINDING: HK_TIMER_BEGIN("Pathfinding", nullptr); break;
            case HK_JOB_TYPE_CHARACTER_PROXY: HK_TIMER_BEGIN("Character Proxy", nullptr); break;
            case HK_JOB_TYPE_USER_0: HK_TIMER_BEGIN("UserJob", nullptr); break;
            default: HK_TIMER_BEGIN("UnknownJobs", nullptr); break;
        }
        jobStatus = m_jobFuncs[job.m_jobType.m_storage].m_processJobFuncs[job.m_jobSubType](*this, job);
        HK_TIMER_END();
    }
    return jobStatus;
}

hkJobQueue::hkJobQueue(const hkJobQueueCinfo& acinfo)
  : m_criticalSection(0), m_popJobFunc(defaultPopDispatchFunc), m_finishJobFunc(defaultFinishDispatchFunc),
    m_numJobTypes(acinfo.m_maxNumJobTypes), m_numSpuElfs(0), m_queryRulesAreUpdated(false), m_numQueueSemaphores(0) {
    TlsSetValue(hkThreadNumber, nullptr);
    void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(static_cast<int32_t>(sizeof(DynamicData)));
    DynamicData* pData = new (pMemory) DynamicData;
    m_data = pData;
    pData->m_outOfMemory = false;
    pData->m_waitPolicy = WAIT_UNTIL_ALL_WORK_COMPLETE;
    m_hwSetup = acinfo.m_jobQueueHwSetup;
    for (int32_t i = 0; i < MAX_NUM_QUEUES; ++i) {
        pData->m_jobQueue[i].setCapacity(JOB_QUEUE_ENTRY_SIZE);
    }
    pData->m_masterThreadFinishingFlags = 0;
    for (int32_t i = 0; i < m_numJobTypes; ++i) {
        m_jobFuncs[i].m_numProcessJobFuncs = 0;
        m_jobFuncs[i].m_processJobFuncs = nullptr;
        m_jobFuncs[i].m_popJobFunc = hkDefaultPopJobFunc;
        m_jobFuncs[i].m_finishJobFunc = hkDefaultFinishJobFunc;
        pData->m_numActiveJobs[i] = 0;
        pData->m_masterThreadFinishingFlags |= 1 << i;
    }
    updateJobQueryRules();
}

// The dynamic data and the queue semaphores are the queue's own blocks; the members destroy themselves.
hkJobQueue::~hkJobQueue() {
    if (m_data) {
        m_data->~DynamicData();
        hkMemoryRouter::getInstance().m_heap->blockFree(m_data, static_cast<int32_t>(sizeof(DynamicData)));
    }
    if (m_queryRulesAreUpdated.m_bool) {
        for (int32_t i = 0; i < m_numQueueSemaphores; ++i) {
            hkSemaphoreBusyWait* pSemaphore = m_queueSemaphores[i];
            if (pSemaphore) {
                pSemaphore->~hkSemaphoreBusyWait();
                hkMemoryRouter::getInstance().m_heap->blockFree(
                    pSemaphore, static_cast<int32_t>(sizeof(hkSemaphoreBusyWait))
                );
            }
        }
    }
}

hkJobQueue::WaitPolicy hkJobQueue::getWaitPolicy() {
    return static_cast<hkJobQueue::WaitPolicy>((int32_t)m_data->m_waitPolicy);
}

int32_t hkJobQueue::getMasterThreadFinishingFlags() {
    return m_data->m_masterThreadFinishingFlags;
}

int32_t hkJobQueue::getQueueIndexForJob(const hkJob& a1) {
    int32_t m_numCustomJobs = this->m_numCustomJobs;
    int32_t v3 = 0;
    if (m_numCustomJobs <= 0) {
        return m_cpuTypesQueuesBegin + a1.m_jobType.m_storage;
    }
    for (uint8_t* i = &m_customJobs[0u].m_jobSubType; a1.m_jobType.m_storage != *(i - 4) || a1.m_jobSubType != *i;
         i += 8) {
        if (++v3 >= m_numCustomJobs) {
            return m_cpuTypesQueuesBegin + a1.m_jobType.m_storage;
        }
    }
    return m_customJobs[v3].m_queueId;
}

void hkJobQueue::checkQueueAndReleaseOneWaitingThread(int32_t queueIndex, hkJobQueue::DynamicData* data) {
    uint16_t v3;
    if (queueIndex < m_numQueueSemaphores && (v3 = data->m_numThreadsWaiting[queueIndex]) != 0) {
        data->m_numThreadsWaiting[queueIndex] = v3 - 1;
        hkSemaphore::release(m_queueSemaphores[queueIndex], 1);
    }
    else {
        releaseOneWaitingThread(data);
    }
}

void hkJobQueue::addJobQueueLocked(
    hkJobQueue::DynamicData* data,
    const hkJobQueue::JobQueueEntry& job,
    hkJobQueue::JobPriority priority
) {
    int32_t QueueIndexForJob = getQueueIndexForJob(job);
    hkJobQueue::Queue* v6 = &data->m_jobQueue[QueueIndexForJob];
    if (priority) {
        v6->enqueue(job);
    }
    else {
        v6->enqueueInFront(job);
    }
    checkQueueAndReleaseOneWaitingThread(QueueIndexForJob, data);
}

// The SDK's lock storage: a 16-aligned byte buffer lockQueue builds the DynamicData in.
void hkJobQueue::addJob(hkJobQueue::JobQueueEntry& job, hkJobQueue::JobPriority priority) {
    alignas(16) char dynamicDataStorage[sizeof(hkJobQueue::DynamicData)];
    hkJobQueue::DynamicData* v4 = lockQueue(dynamicDataStorage);
    addJobQueueLocked(v4, job, priority);
    unlockQueue(v4);
}

// Counts a job of oldJobType off, queues jobInOut at priority (the queue's enqueue or enqueueInFront) and takes the
// next job into jobInOut. findJobInternal unlocks the queue; JOB_INVALID means it found no job yet and the queue is
// locked again, with nothing left to add.
hkJobQueue::JobStatus hkJobQueue::finishAddAndGetNextJob(
    hkJobType oldJobType,
    hkJobQueue::JobPriority priority,
    hkJobQueue::JobQueueEntry& jobInOut,
    hkJobQueue::WaitStatus waitStatus
) {
    HK_TIMER_BEGIN("GetNextJob", nullptr);
    // The SDK's lock storage: a 16-aligned byte buffer lockQueue builds the DynamicData in.
    alignas(16) char dynamicDataStorage[sizeof(DynamicData)];
    bool bJobToAdd = true;
    JobStatus result;
    do {
        DynamicData* data = lockQueue(dynamicDataStorage);
        int32_t queueIndexOfNewJob = -1;
        if (bJobToAdd) {
            --data->m_numActiveJobs[oldJobType];
            queueIndexOfNewJob = getQueueIndexForJob(jobInOut);
            Queue& queue = data->m_jobQueue[queueIndexOfNewJob];
            if (priority == JOB_HIGH_PRIORITY) {
                queue.enqueueInFront(jobInOut);
            }
            else {
                queue.enqueue(jobInOut);
            }
            bJobToAdd = false;
        }
        result = findJobInternal(queueIndexOfNewJob, data, waitStatus, jobInOut);
    }
    while (result == JOB_INVALID);
    HK_TIMER_END();
    return result;
}

void hkJobQueue::releaseOneWaitingThread(hkJobQueue::DynamicData* data) {
    int32_t m_numJobQueues;
    int32_t m_numQueueSemaphores;
    int32_t m_cpuSemaphoreBegin = this->m_cpuSemaphoreBegin;
    int32_t m_directMapSemaphoreEnd = this->m_directMapSemaphoreEnd;
    char v5 = 0;
    if (m_cpuSemaphoreBegin < m_directMapSemaphoreEnd) {
        uint16_t* v6 = &data->m_numThreadsWaiting[m_cpuSemaphoreBegin];
        // Indexed by element: Queue holds a pointer, so its size differs on x64.
        do {
            if (data->m_jobQueue[m_cpuSemaphoreBegin].m_elementsInUse) {
                v5 = 1;
                if (*v6) {
                    goto LABEL_16;
                }
            }
            ++m_cpuSemaphoreBegin;
            ++v6;
        }
        while (m_cpuSemaphoreBegin < m_directMapSemaphoreEnd);
    }
    m_numJobQueues = this->m_numJobQueues;
    if (m_cpuSemaphoreBegin >= m_numJobQueues) {
    LABEL_10:;
        if (!v5) {
            return;
        }
    }
    else {
        while (!data->m_jobQueue[m_cpuSemaphoreBegin].m_elementsInUse) {
            ++m_cpuSemaphoreBegin;
            if (m_cpuSemaphoreBegin >= m_numJobQueues) {
                goto LABEL_10;
            }
        }
    }
    m_cpuSemaphoreBegin = this->m_cpuSemaphoreBegin;
    m_numQueueSemaphores = this->m_numQueueSemaphores;
    if (m_cpuSemaphoreBegin < m_numQueueSemaphores) {
        for (uint16_t* i = &data->m_numThreadsWaiting[m_cpuSemaphoreBegin]; !*i; ++i) {
            if (++m_cpuSemaphoreBegin >= m_numQueueSemaphores) {
                return;
            }
        }
    LABEL_16:;
        --data->m_numThreadsWaiting[m_cpuSemaphoreBegin];
        hkSemaphore::release(m_queueSemaphores[m_cpuSemaphoreBegin], 1);
    }
}

void hkJobQueue::addJob(hkJob& job, hkJobQueue::JobPriority priority) {
    hkJobQueue::JobQueueEntry apDest;
    hkString::memCpy(&apDest, &job, job.m_size);
    addJob(apDest, priority);
}

// Queues each job (its m_size bytes copied into a 16-aligned entry) under one lock: a high-priority job in front, any
// other at the back; then wakes a waiting thread for that queue.
// KEEP: the whole 128-byte entry is copied into the queue (rep movsd of 0x20 dwords), but only the job's m_size bytes
// of it are written: the rest is uninitialised stack that reaches the queue's memory.
void hkJobQueue::addJobBatch(const hkArrayBase<hkJob*>& jobs, hkJobQueue::JobPriority priority) {
    HK_TIMER_BEGIN("AddJobBatch", nullptr);
    // The SDK's lock storage: a 16-aligned byte buffer lockQueue builds the DynamicData in.
    alignas(16) char dynamicDataStorage[sizeof(DynamicData)];
    DynamicData* data = lockQueue(dynamicDataStorage);
    for (int32_t i = 0; i < jobs.m_size; ++i) {
        JobQueueEntry entry;
        hkString::memCpy(&entry, jobs.m_data[i], jobs.m_data[i]->m_size);
        const int32_t queueIndex = getQueueIndexForJob(entry);
        hkQueue<JobQueueEntry>& queue = data->m_jobQueue[queueIndex];
        if (priority == JOB_HIGH_PRIORITY) {
            queue.enqueueInFront(entry);
        }
        else {
            queue.enqueue(entry);
        }
        checkQueueAndReleaseOneWaitingThread(queueIndex, data);
    }
    unlockQueue(data);
    HK_TIMER_END();
}

// Finishes oldJob: its job type's finish callback may create a follow-up job, which is queued at its priority; a
// normal finish counts the job off.
void hkJobQueue::finishJob(const hkJobQueue::JobQueueEntry* oldJob, hkJobQueue::FinishJobFlag flag) {
    HK_TIMER_BEGIN("finishJob", nullptr);
    // The SDK's lock storage: a 16-aligned byte buffer lockQueue builds the DynamicData in.
    alignas(16) char dynamicDataStorage[sizeof(DynamicData)];
    JobQueueEntryInput newJob;
    DynamicData* data = lockQueue(dynamicDataStorage);
    if (m_finishJobFunc(*this, data, *oldJob, newJob) == JOB_CREATED) {
        int32_t queueIndex = getQueueIndexForJob(newJob.m_job);
        if (newJob.m_jobPriority.m_storage == JOB_HIGH_PRIORITY) {
            data->m_jobQueue[queueIndex].enqueueInFront(newJob.m_job);
        }
        else {
            data->m_jobQueue[queueIndex].enqueue(newJob.m_job);
        }
        checkQueueAndReleaseOneWaitingThread(queueIndex, data);
    }
    if (flag == FINISH_FLAG_NORMAL) {
        --data->m_numActiveJobs[oldJob->m_jobType.m_storage];
    }
    unlockQueue(data);
    HK_TIMER_END();
}

// The thread number is an integer the hkThreadNumber TLS slot holds in its untyped void* (the SDK's
// HK_THREAD_LOCAL_GET), so it is read back through uintptr_t; thread 0 is the master thread.
hkJobQueue::JobStatus hkJobQueue::findJobInternal(
    int32_t queueIndexOfNewJob,
    hkJobQueue::DynamicData* data,
    hkJobQueue::WaitStatus waitStatus,
    hkJobQueue::JobQueueEntry& jobOut
) {
    int32_t mySemaphore;
    if (TlsGetValue(hkThreadNumber)) {
        mySemaphore = m_cpuThreadIndexToSemaphoreIndex[reinterpret_cast<uintptr_t>(TlsGetValue(hkThreadNumber))];
    }
    else {
        mySemaphore = m_masterThreadQueue;
    }
    // Take a job from the first non-empty queue of this thread's list (ended by a negative index); -1 when none has
    // one, whatever negative value ended the list.
    const int8_t* nextQueue = m_nextQueueToGet[mySemaphore];
    int32_t queueIndex = -1;
    for (int32_t index = *nextQueue; index >= 0; index = *++nextQueue) {
        hkJobQueue::Queue& queue = data->m_jobQueue[index];
        if (queue.m_elementsInUse) {
            hkJobQueue::JobQueueEntry job;
            queue.dequeue(job);
            if (m_popJobFunc(*this, data, job, jobOut) == hkJobQueue::DO_NOT_POP_QUEUE_ENTRY) {
                queue.enqueueInFront(job);
                checkQueueAndReleaseOneWaitingThread(index, data);
            }
            ++data->m_numActiveJobs[job.m_jobType.m_storage];
            queueIndex = index;
            break;
        }
    }
    if (queueIndexOfNewJob != -1 && queueIndexOfNewJob != queueIndex) {
        checkQueueAndReleaseOneWaitingThread(queueIndexOfNewJob, data);
    }
    if (queueIndex >= 0) {
        unlockQueue(data);
        return hkJobQueue::GOT_NEXT_JOB;
    }

    // No job: are there any left in the cpu queues, or active of any type?
    int32_t numJobs = 0;
    for (int32_t i = m_cpuSemaphoreBegin; i < m_directMapSemaphoreEnd; ++i) {
        numJobs += data->m_jobQueue[i].m_elementsInUse;
    }
    if (!numJobs) {
        int32_t activeTypes = 0;
        for (int32_t i = 0; i < m_numJobTypes; ++i) {
            int32_t active =
                data->m_numActiveJobs[i] > 0 || data->m_jobQueue[m_cpuTypesQueuesBegin + i].m_elementsInUse;
            activeTypes |= active << i;
        }
        bool noActiveJobs = activeTypes == 0;
        if ((activeTypes & data->m_masterThreadFinishingFlags) == 0) {
            if (!TlsGetValue(hkThreadNumber)) {
                if (noActiveJobs) {
                    releaseWaitingThreads(data);
                }
                unlockQueue(data);
                return hkJobQueue::ALL_JOBS_FINISHED;
            }
            // Wake the master thread.
            if (data->m_numThreadsWaiting[m_cpuThreadIndexToSemaphoreIndex[0]]) {
                --data->m_numThreadsWaiting[m_cpuThreadIndexToSemaphoreIndex[0]];
                hkSemaphore::release(m_queueSemaphores[m_cpuThreadIndexToSemaphoreIndex[0]], 1);
            }
        }
        if (noActiveJobs && data->m_waitPolicy != hkJobQueue::WAIT_INDEFINITELY) {
            releaseWaitingThreads(data);
            unlockQueue(data);
            return hkJobQueue::ALL_JOBS_FINISHED;
        }
    }
    if (waitStatus == hkJobQueue::DO_NOT_WAIT_FOR_NEXT_JOB) {
        unlockQueue(data);
        return hkJobQueue::NO_JOBS_AVAILABLE;
    }
    int32_t semaphore = m_cpuThreadIndexToSemaphoreIndex[reinterpret_cast<uintptr_t>(TlsGetValue(hkThreadNumber))];
    ++data->m_numThreadsWaiting[semaphore];
    unlockQueue(data);
    HK_TIMER_BEGIN("NoJobAvailable", nullptr);
    hkSemaphore::acquire(m_queueSemaphores[semaphore]);
    HK_TIMER_END();
    return hkJobQueue::JOB_INVALID;
}

void hkJobQueue::registerJobWithCpuThread(hkJobType jobType, uint8_t subType, int32_t threadId) {
    hkArray<hkJobQueue::CustomJobTypeSetup, hkContainerHeapAllocator>* p_m_customJobSetup = &m_customJobSetup;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_customJobSetup->m_size ==
        (p_m_customJobSetup->m_capacityAndFlags & hkArrayBase<hkJobQueue::CustomJobTypeSetup>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_customJobSetup, sizeof(hkJobQueue::CustomJobTypeSetup));
    }
    int32_t m_size = p_m_customJobSetup->m_size;
    hkJobQueue::CustomJobTypeSetup* m_data = p_m_customJobSetup->begin();
    p_m_customJobSetup->m_size = m_size + 1;
    hkJobQueue::CustomJobTypeSetup* v9 = &m_data[m_size];
    v9->m_jobType = jobType;
    v9->m_jobSubType = subType;
    v9->m_threadId = threadId;
    updateJobQueryRules();
}

hkJobQueue::JobCreationStatus hkDefaultFinishJobFunc(
    hkJobQueue& queue,
    hkJobQueue::DynamicData* data,
    const hkJobQueue::JobQueueEntry& a3,
    hkJobQueue::JobQueueEntryInput& a4
) {
    return hkJobQueue::NO_JOB_CREATED;
}

hkJobQueue::JobPopFuncResult hkDefaultPopJobFunc(
    hkJobQueue& queue,
    hkJobQueue::DynamicData* data,
    hkJobQueue::JobQueueEntry& a3,
    hkJobQueue::JobQueueEntry& a4
) {
    return hkJobQueue::POP_QUEUE_ENTRY;
}

// Forwards to the job type's pop function with its own arguments.
hkJobQueue::JobPopFuncResult defaultPopDispatchFunc(
    hkJobQueue& queue,
    hkJobQueue::DynamicData* data,
    hkJobQueue::JobQueueEntry& jobIn,
    hkJobQueue::JobQueueEntry& jobOut
) {
    return queue.m_jobFuncs[jobIn.m_jobType.m_storage].m_popJobFunc(queue, data, jobIn, jobOut);
}

// Forwards to the job type's finish function with its own arguments.
hkJobQueue::JobCreationStatus defaultFinishDispatchFunc(
    hkJobQueue& queue,
    hkJobQueue::DynamicData* data,
    const hkJobQueue::JobQueueEntry& jobIn,
    hkJobQueue::JobQueueEntryInput& newJobCreatedOut
) {
    return queue.m_jobFuncs[jobIn.m_jobType.m_storage].m_finishJobFunc(queue, data, jobIn, newJobCreatedOut);
}

// hkJobQueue's class allocator delete, used when bhkMultithreadedSystem::InitMTSystem's new unwinds.
void freeJobQueue(void* a1) {
    if (a1) {
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        Instance->m_heap->blockFree(a1, sizeof(hkJobQueue));
    }
}
