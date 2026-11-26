#include "BSHavok/bhkMultithreadedSystem.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Thread/Job/ThreadPool/Cpu/hkCpuJobThreadPool.h"
#include "Havok/Source/Common/Base/Thread/Job/ThreadPool/Cpu/hkCpuJobThreadPoolCinfo.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Thread/Pool/hkJobThreadPool.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"

// iNumThreads registers itself with its default of 1.
bhkMultithreadedSystem::bhkMultithreadedSystem()
  : iNumThreads("iNumThreads:HAVOK", Setting::Info{.i = 1}), uiMTResourceOwner(0), pCurrentMTWorld(nullptr),
    pJobQueue(nullptr), pJobThreadPool(nullptr) {}

bhkMultithreadedSystem::~bhkMultithreadedSystem() {
    QuitMTSystem();
}

void bhkMultithreadedSystem::AcquireMTResources(int32_t abLargerSPUBufferSize) {
    uint32_t uiThreadID = GetCurrentThreadId();
    if (uiMTResourceOwner != uiThreadID) {
        InitMTSystem(uiThreadID, abLargerSPUBufferSize);
    }
}

// Gives the Havok multithreading resources to the thread arIdToAcquireFor: frees the old ones, then makes a CPU job
// thread pool of iNumThreads threads and a job queue of 10 job types, and registers the world's jobs with the queue.
void bhkMultithreadedSystem::InitMTSystem(const uint32_t& arIdToAcquireFor, int32_t abLargerSPUBufferSize) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    QuitMTSystem();
    uiMTResourceOwner = arIdToAcquireFor;
    hkCpuJobThreadPoolCinfo ci;
    ci.m_numThreads = iNumThreads.uValue.i;
    pJobThreadPool = new hkCpuJobThreadPool(ci);
    hkJobQueueCinfo kQueueInfo;
    kQueueInfo.m_jobQueueHwSetup.~hkJobQueueHwSetup();
    ::new (&kQueueInfo.m_jobQueueHwSetup) hkJobQueueHwSetup;
    kQueueInfo.m_maxNumJobTypes = 10;
    hkJobQueue* pQueue = new hkJobQueue(kQueueInfo);
    pJobQueue = pQueue;
    hkpWorld::registerWithJobQueue(pQueue);
}

void bhkMultithreadedSystem::QuitMTSystem() {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (uiMTResourceOwner) {
        hkJobQueue* pJobQueue = this->pJobQueue;
        delete pJobQueue;
        hkJobThreadPool* pJobThreadPool = this->pJobThreadPool;
        this->pJobQueue = nullptr;
        if (pJobThreadPool) {
            pJobThreadPool->removeReference();
        }
        this->pJobThreadPool = nullptr;
        uiMTResourceOwner = 0;
    }
    else {
        uiMTResourceOwner = 0;
    }
}
