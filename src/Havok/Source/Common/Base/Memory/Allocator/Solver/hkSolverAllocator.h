#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Thread/CriticalSection/hkCriticalSection.h"

// The allocator for the constraint solver's scratch memory: one fixed buffer handed out from its start (blocks and
// buffers that are freed in any order leave free elements behind, kept in a small inplace list).
class hkSolverAllocator : public hkMemoryAllocator {
public:
    struct Element {
        char* m_start;
        int32_t m_size;
    };

    hkSolverAllocator();
    ~hkSolverAllocator() override;

    void* blockAlloc(int32_t numBytes) override;
    void blockFree(void* p, int32_t numBytes) override;
    void* bufAlloc(int32_t& reqNumInOut) override;
    void bufFree(void* data, int32_t size) override;
    bool canAllocSingleBlock(int32_t numBytes) override;
    void getMemoryStatistics(hkMemoryAllocator::MemoryStatistics& u) override;
    int32_t getAllocatedSize(const void* obj, int32_t nbytes) override {
        return nbytes;
    }
    void resetPeakMemoryStatistics() override;

    void setBuffer(void* vbuffer, int32_t bufferSize);

    char* m_bufferStart;
    char* m_bufferEnd;
    char* m_currentEnd;
    int32_t m_numAllocatedBlocks;
    uint32_t m_peakUse;
    hkArrayBase<Element> m_freeElems;
    Element m_elemsBuf[32];
    hkCriticalSection m_criticalSection;
};
