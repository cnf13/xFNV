#include "BSHavok/bhkMultithreadedSystem.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
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
	if (uiMTResourceOwner != uiThreadID)
		InitMTSystem(uiThreadID, abLargerSPUBufferSize);
}

// Gives the Havok multithreading resources to the thread arIdToAcquireFor: frees the old ones, then makes a CPU job
// thread pool of iNumThreads threads and a job queue of 10 job types, and registers the world's jobs with the queue.
// The exe inlines the pool info's hkArray destructor at the end. The exe's cleanup-only C++ EH frame is left out.
void bhkMultithreadedSystem::InitMTSystem(const uint32_t& arIdToAcquireFor, int32_t abLargerSPUBufferSize) {
	TLSData* pTLSData = GetThreadTLSData();
	MEM_CONTEXT eMemContext = pTLSData->eMemContext;
	pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
	QuitMTSystem();
	uiMTResourceOwner = arIdToAcquireFor;
	FNVD_INLINED_LOCAL(hkCpuJobThreadPoolCinfo, ci, std::nullopt, std::nullopt);
	::new (&ci) hkCpuJobThreadPoolCinfo;
	ci.m_numThreads = iNumThreads.uValue.i;
	hkCpuJobThreadPool* pPoolMemory =
		static_cast<hkCpuJobThreadPool*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkCpuJobThreadPool)));
	pPoolMemory->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkCpuJobThreadPool));
	pJobThreadPool = ::new (pPoolMemory) hkCpuJobThreadPool(ci);
	hkJobQueueCinfo kQueueInfo;
	kQueueInfo.m_jobQueueHwSetup.~hkJobQueueHwSetup();
	::new (&kQueueInfo.m_jobQueueHwSetup) hkJobQueueHwSetup;
	kQueueInfo.m_maxNumJobTypes = 10;
	hkJobQueue* pQueueMemory =
		static_cast<hkJobQueue*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkJobQueue)));
	hkJobQueue* pQueue;
	if (pQueueMemory) {
		pQueue = ::new (pQueueMemory) hkJobQueue(kQueueInfo);
	} else {
		pQueue = nullptr;
	}
	pJobQueue = pQueue;
	hkpWorld::registerWithJobQueue(pQueue);
	// EXE-INLINED 0xCBF3C0
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	ci.m_hardwareThreadIds.m_size = 0;
	if (ci.m_hardwareThreadIds.m_capacityAndFlags >= 0) {
		pHeap->bufFree(ci.m_hardwareThreadIds.begin(),
			static_cast<int32_t>(ci.m_hardwareThreadIds.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK) *
				static_cast<int32_t>(sizeof(int32_t)));
	}
	pTLSData->eMemContext = eMemContext;
}

// The exe's cleanup-only C++ EH frame is left out.
void bhkMultithreadedSystem::QuitMTSystem() {
	TLSData* v2 = GetThreadTLSData();
	MEM_CONTEXT eMemContext = v2->eMemContext;
	v2->eMemContext = MEM_CONTEXT_HAVOK;
	if (uiMTResourceOwner) {
		hkJobQueue* pJobQueue = this->pJobQueue;
		if (pJobQueue) {
			this->pJobQueue->~hkJobQueue();
			hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
			Instance->m_heap->blockFree(pJobQueue, sizeof(hkJobQueue));
		}
		hkJobThreadPool* pJobThreadPool = this->pJobThreadPool;
		this->pJobQueue = 0;
		if (pJobThreadPool) {
			pJobThreadPool->removeReference();
		}
		this->pJobThreadPool = 0;
		uiMTResourceOwner = 0;
		v2->eMemContext = eMemContext;
	} else {
		uiMTResourceOwner = 0;
		v2->eMemContext = eMemContext;
	}
}

// The hardware thread id array is freed by its own destructor.
hkCpuJobThreadPoolCinfo::~hkCpuJobThreadPoolCinfo() {}
