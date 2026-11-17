#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Reconstruction.hpp"

class hkJobQueue;
enum hkJobType : int32_t;

// One worker thread's monitor stream range, the data appendTimerData gathers: where the thread's stream begins and
// ends (the PDB's members; the exe reserves 8 bytes an entry and copies the pair, 0xD2D2C0).
struct hkTimerData {
	const char* m_streamBegin;
	const char* m_streamEnd;
};

// The worker threads that process a job queue. The game's pool and its code are the exe's; src only calls into it.
class hkJobThreadPool : public hkReferencedObject {
public:
	// firstJobType is an hkJobType (src has no such enum, so it stays a fixed-width integer).
	virtual void processAllJobs(hkJobQueue* queue, hkJobType firstJobType) = 0;
	virtual void waitForCompletion() = 0;
	virtual bool isProcessing() = 0;
	virtual int32_t getNumThreads() = 0;
	virtual void setNumThreads(int32_t numThreads) = 0;
	virtual void appendTimerData(hkArrayBase<hkTimerData>& timerDataOut, hkMemoryAllocator& alloc) = 0;
	// Drops the monitor data the threads gathered.
	virtual void clearTimerData() = 0;
};
