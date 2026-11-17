#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Thread/Pool/hkJobThreadPool.h"
#include "Havok/Source/Common/Base/Thread/Semaphore/hkSemaphore.h"
#include "Havok/Source/Common/Base/Thread/Thread/Win32/hkWin32Thread.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

class hkCpuJobThreadPoolCinfo;
class hkJobQueue;

// The body each worker thread runs: apThreadData is its WorkerThreadData. Sets up the thread's Havok state, then
// processes the shared job queue each time its semaphore is released, until it is told to quit. Returns null.
void* hkWorkerThreadFunc(void* v);

// The pool of worker threads that process a job queue on the CPU: up to six threads, each with its own data, and
// the data they share.
class hkCpuJobThreadPool : public hkJobThreadPool {
public:
	// Values from the Xbox PDB.
	enum : int32_t {
		MAX_NUM_THREADS = 0x6,
	};

	struct SharedThreadData;

	// One worker thread of the pool: its thread, ids, kill and timer flags, semaphore and monitor stream (the PDB's
	// and the SDK's nested struct).
	struct WorkerThreadData {
		WorkerThreadData();

		SharedThreadData* m_sharedThreadData;
		// hkThread is 8-byte aligned (its m_threadId is a uint64_t), so the compiler leaves the four bytes after the
		// shared data; the exe constructs the thread at +0x8 (lea ecx, [esi + 8], 0xD2CDA3), and the PDB's
		// WorkerThreadData has m_thread at 0x8 too.
		hkThread m_thread;
		int32_t m_threadId;
		int32_t m_hardwareThreadId;
		bool m_killThread;
		bool m_clearTimers;
		uint8_t padding22[2];
		hkSemaphore m_semaphore;
		char* m_monitorStreamBegin;
		char* m_monitorStreamEnd;
	};

	// The thread data the job queue and the worker threads share (the PDB's and the SDK's nested struct).
	struct SharedThreadData {
		SharedThreadData();

		hkJobQueue* m_jobQueue;
		hkSemaphore m_workerThreadFinished;
		int32_t m_numThreads;
		int32_t m_localHavokStackSize;
		int32_t m_timerBufferAllocation;
	};

	hkCpuJobThreadPool(const hkCpuJobThreadPoolCinfo& ci);
	~hkCpuJobThreadPool() override;

	void processAllJobs(hkJobQueue* queue, hkJobType firstJobType_unused) override;
	void waitForCompletion() override;
	bool isProcessing() override;
	int32_t getNumThreads() override;
	void setNumThreads(int32_t numThreads) override;
	void appendTimerData(hkArrayBase<hkTimerData>& timerDataOut, hkMemoryAllocator& alloc) override;
	void clearTimerData() override;

	// Stops the last worker thread: tells it to quit, waits for it to finish and destroys its thread.
	void removeThread();
	void addThread();

	WorkerThreadData m_workerThreads[6];
	SharedThreadData m_sharedThreadData;
	hkBool m_isRunning;
	uint8_t padding13D[3];
	const char* m_threadName;
	int32_t m_stackSize;
};

// IDA's name for hkCpuJobThreadPool::WorkerThreadData.
using WorkerThreadData = hkCpuJobThreadPool::WorkerThreadData;
// IDA's name for hkCpuJobThreadPool::SharedThreadData.
using SharedThreadData = hkCpuJobThreadPool::SharedThreadData;

inline constexpr fnvd::Vtable hkCpuJobThreadPool_vftable{};
