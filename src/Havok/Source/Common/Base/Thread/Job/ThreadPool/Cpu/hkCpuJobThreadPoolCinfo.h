#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"

// Configures the CPU worker threads, stacks and monitor buffers.
class hkCpuJobThreadPoolCinfo {
public:
    hkCpuJobThreadPoolCinfo();
    ~hkCpuJobThreadPoolCinfo();

    int32_t m_numThreads;
    int32_t m_havokStackSize;
    int32_t m_stackSize;
    int32_t m_timerBufferPerThreadAllocation;
    hkArray<int32_t, hkContainerHeapAllocator> m_hardwareThreadIds;
    const char* m_threadName;
};
