#include "Havok/Source/Common/Base/Thread/Job/ThreadPool/Cpu/hkCpuJobThreadPool.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/System/hkMemorySystem.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/hkBaseSystem.h"
#include "Havok/Source/Common/Base/Thread/Job/ThreadPool/Cpu/hkCpuJobThreadPoolCinfo.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Thread/Pool/hkJobThreadPool.h"
#include "Havok/Source/Common/Base/Thread/Semaphore/hkSemaphore.h"
#include "platform/System.h"

#include <thread>
#include <xmmintrin.h>

// The exe's router is a stack local it constructs with the exe's constructor, so a raw buffer holds it here.
void* hkWorkerThreadFunc(void* v) {
	WorkerThreadData& data = *static_cast<WorkerThreadData*>(v);
	SharedThreadData* pShared = data.m_sharedThreadData;
	// The TLS slot holds the thread number as its raw pointer-sized value.
	TlsSetValue(hkThreadNumber, reinterpret_cast<void*>(static_cast<uintptr_t>(data.m_threadId)));
	// Flush denormals to zero (MXCSR bit 15).
	_mm_setcsr(_mm_getcsr() | 0x8000);
	platform::SetCurrentThreadIdealProcessor(static_cast<uint32_t>(data.m_hardwareThreadId));
	alignas(hkMemoryRouter) uint8_t routerMemory[sizeof(hkMemoryRouter)];
	hkMemoryRouter& router = *::new (routerMemory) hkMemoryRouter;
	hkMemorySystem::getInstance().threadInit(router, "hkCpuJobThreadPool", hkMemorySystem::FLAG_ALL);
	hkBaseSystem::initThread(&router);
	if (pShared->m_timerBufferAllocation > 0) {
		hkMonitorStream::getInstance().resize(pShared->m_timerBufferAllocation);
	}
	data.m_monitorStreamBegin = hkMonitorStream::getInstance().m_start;
	data.m_monitorStreamEnd = hkMonitorStream::getInstance().m_end;
	data.m_semaphore.acquire();
	while (!data.m_killThread) {
		if (data.m_clearTimers) {
			hkMonitorStream::getInstance().reset();
			data.m_monitorStreamEnd = hkMonitorStream::getInstance().m_end;
			data.m_clearTimers = false;
		}
		pShared->m_jobQueue->processAllJobs();
		data.m_monitorStreamEnd = hkMonitorStream::getInstance().m_end;
		pShared->m_workerThreadFinished.release(1);
		data.m_semaphore.acquire();
	}
	hkBaseSystem::quitThread();
	hkMemorySystem::getInstance().threadQuit(router, hkMemorySystem::FLAG_ALL);
	pShared->m_workerThreadFinished.release(1);
	return nullptr;
}

hkCpuJobThreadPool::WorkerThreadData::WorkerThreadData()
	: m_sharedThreadData(nullptr), m_threadId(-1), m_killThread(false), m_semaphore(0, 1),
	  m_monitorStreamBegin(nullptr), m_monitorStreamEnd(nullptr) {}

hkCpuJobThreadPool::SharedThreadData::SharedThreadData() : m_workerThreadFinished(0, 6) {}

hkCpuJobThreadPool::hkCpuJobThreadPool(const hkCpuJobThreadPoolCinfo& ci)
	: m_isRunning(false), m_threadName(ci.m_threadName), m_stackSize(ci.m_stackSize) {
	m_sharedThreadData.m_localHavokStackSize = ci.m_havokStackSize;
	m_sharedThreadData.m_timerBufferAllocation = ci.m_timerBufferPerThreadAllocation;
	// The exe clamps a count above MAX_NUM_THREADS to 5, not 6 (cmp ebx, 6; jle; mov ebx, 5 at 0xD2D17B).
	int32_t iNumThreads = ci.m_numThreads;
	if (iNumThreads > MAX_NUM_THREADS) {
		iNumThreads = 5;
	}
	m_sharedThreadData.m_numThreads = iNumThreads;
	// GetSystemInfo's dwNumberOfProcessors: the logical processors.
	int32_t numCores = static_cast<int32_t>(std::thread::hardware_concurrency());
	for (int32_t i = 0; i < iNumThreads; ++i) {
		WorkerThreadData& data = m_workerThreads[i];
		data.m_sharedThreadData = &m_sharedThreadData;
		data.m_threadId = i + 1;
		data.m_monitorStreamBegin = nullptr;
		data.m_monitorStreamEnd = nullptr;
		data.m_killThread = false;
		data.m_clearTimers = false;
		if (ci.m_hardwareThreadIds.m_size > 0) {
			data.m_hardwareThreadId = ci.m_hardwareThreadIds.m_data[i];
		} else {
			data.m_hardwareThreadId = (i + 1) % numCores;
		}
		data.m_thread.startThread(hkWorkerThreadFunc, &data, m_threadName, m_stackSize);
	}
	hkReferencedObject::setLockMode(hkReferencedObject::LOCK_MODE_AUTO);
}

// The members' destructors (the semaphores and threads) follow, last worker first.
hkCpuJobThreadPool::~hkCpuJobThreadPool() {
	waitForCompletion();
	// Tell every worker to quit and wake it.
	for (int32_t i = 0; i < m_sharedThreadData.m_numThreads; ++i) {
		WorkerThreadData& data = m_workerThreads[i];
		data.m_killThread = true;
		data.m_semaphore.release(1);
	}
	for (int32_t i = 0; i < m_sharedThreadData.m_numThreads; ++i) {
		m_sharedThreadData.m_workerThreadFinished.acquire();
	}
	hkReferencedObject::setLockMode(hkReferencedObject::LOCK_MODE_NONE);
}

void hkCpuJobThreadPool::processAllJobs(hkJobQueue* queue, hkJobType firstJobType_unused) {
	m_sharedThreadData.m_jobQueue = queue;
	m_isRunning.m_bool = 1;
	int32_t v3 = m_sharedThreadData.m_numThreads - 1;
	if (v3 >= 0) {
		// Indexed by element: WorkerThreadData holds pointers, so its size differs on x64.
		do {
			m_workerThreads[v3].m_semaphore.release(1);
			--v3;
		} while (v3 >= 0);
	}
}

// Waits once per worker thread for its finished signal. The exe tests and clears m_isRunning's byte inline.
void hkCpuJobThreadPool::waitForCompletion() {
	if (m_isRunning.m_bool) {
		for (int32_t i = 0; i < m_sharedThreadData.m_numThreads; ++i) {
			m_sharedThreadData.m_workerThreadFinished.acquire();
		}
		m_isRunning.m_bool = 0;
	}
}

bool hkCpuJobThreadPool::isProcessing() {
	return (m_isRunning.m_bool != 0);
}

int32_t hkCpuJobThreadPool::getNumThreads() {
	return m_sharedThreadData.m_numThreads;
}

// Starts or stops worker threads until there are numThreads (at most MAX_NUM_THREADS).
void hkCpuJobThreadPool::setNumThreads(int32_t numThreads) {
	if (numThreads > MAX_NUM_THREADS) {
		numThreads = MAX_NUM_THREADS;
	}
	while (m_sharedThreadData.m_numThreads < numThreads) {
		addThread();
	}
	while (m_sharedThreadData.m_numThreads > numThreads) {
		removeThread();
	}
}

void hkCpuJobThreadPool::appendTimerData(hkArrayBase<hkTimerData>& timerDataOut, hkMemoryAllocator& alloc) {
	int32_t v4 = 0;
	if (m_sharedThreadData.m_numThreads > 0) {
		// Indexed by element: WorkerThreadData holds pointers, so its size differs on x64.
		do {
			if (timerDataOut.m_size == (timerDataOut.m_capacityAndFlags & hkArrayBase<hkTimerData>::CAPACITY_MASK)) {
				hkArrayUtil::_reserveMore(alloc, &timerDataOut, sizeof(hkTimerData));
			}
			int32_t m_size = timerDataOut.m_size;
			hkTimerData* v7 = &timerDataOut.m_data[m_size];
			timerDataOut.m_size = m_size + 1;
			v7->m_streamBegin = m_workerThreads[v4].m_monitorStreamBegin;
			v7->m_streamEnd = m_workerThreads[v4].m_monitorStreamEnd;
			++v4;
		} while (v4 < m_sharedThreadData.m_numThreads);
	}
}

void hkCpuJobThreadPool::clearTimerData() {
	for (int32_t i = 0; i < m_sharedThreadData.m_numThreads; ++i) {
		m_workerThreads[i].m_monitorStreamEnd = m_workerThreads[i].m_monitorStreamBegin;
		m_workerThreads[i].m_clearTimers = true;
	}
}

void hkCpuJobThreadPool::removeThread() {
	m_sharedThreadData.m_numThreads--;
	WorkerThreadData& data = m_workerThreads[m_sharedThreadData.m_numThreads];
	data.m_killThread = true;
	data.m_semaphore.release(1);
	m_sharedThreadData.m_workerThreadFinished.acquire();
	data.m_thread.stopThread();
}

// Starts one more worker thread on the next free WorkerThreadData; its hardware thread is its id modulo the core count.
void hkCpuJobThreadPool::addThread() {
	WorkerThreadData* pData = &m_workerThreads[m_sharedThreadData.m_numThreads];
	pData->m_sharedThreadData = &m_sharedThreadData;
	int32_t iNumThreads = m_sharedThreadData.m_numThreads;
	pData->m_monitorStreamBegin = 0;
	pData->m_monitorStreamEnd = 0;
	pData->m_killThread = false;
	pData->m_clearTimers = false;
	pData->m_threadId = iNumThreads + 1;
	// GetSystemInfo's dwNumberOfProcessors: the logical processors. The SDK keeps the count in an int: the exe
	// divides signed (idiv).
	pData->m_hardwareThreadId = pData->m_threadId % static_cast<int32_t>(std::thread::hardware_concurrency());
	pData->m_thread.startThread(&hkWorkerThreadFunc, pData, m_threadName, m_stackSize);
	++m_sharedThreadData.m_numThreads;
}

hkCpuJobThreadPoolCinfo::hkCpuJobThreadPoolCinfo()
	: m_numThreads(1), m_havokStackSize(0x3D0900), m_stackSize(0), m_timerBufferPerThreadAllocation(0),
	  m_threadName("HavokWorkerThread") {}
