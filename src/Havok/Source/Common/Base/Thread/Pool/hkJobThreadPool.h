#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

class hkJobQueue;

enum hkJobType : int32_t;

// One worker thread's monitor stream range, the data appendTimerData gathers: where the thread's stream begins and
// ends.
struct hkTimerData {
    const char* m_streamBegin;
    const char* m_streamEnd;
};

// The worker threads that process a job queue.
class hkJobThreadPool : public hkReferencedObject {
public:
    virtual void processAllJobs(hkJobQueue* queue, hkJobType firstJobType) = 0;
    virtual void waitForCompletion() = 0;
    virtual bool isProcessing() = 0;
    virtual int32_t getNumThreads() = 0;
    virtual void setNumThreads(int32_t numThreads) = 0;
    virtual void appendTimerData(hkArrayBase<hkTimerData>& timerDataOut, hkMemoryAllocator& alloc) = 0;
    // Drops the monitor data the threads gathered.
    virtual void clearTimerData() = 0;
};
