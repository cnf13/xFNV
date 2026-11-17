#pragma once

#include "BSCore/MemoryManager.h"

class IMemoryManagerFile;

// Interface of the engine's heaps.
class IMemoryHeap {
public:
    virtual ~IMemoryHeap();

    virtual const char* GetName() = 0;
    virtual void* Allocate(uint32_t aiSize, uint32_t aiAlignment) = 0;
    virtual void Deallocate(void* apMem, uint32_t aiUnused) = 0;
    virtual bool PointerInHeap(const void* apPointer) = 0;
    virtual void CheckHeapIntegrity() = 0;
    virtual void GetHeapStats(HeapStats* apStats, bool abFullBlockInfo) = 0;
    virtual int32_t OutputBucketInfo(IMemoryManagerFile* apFile) = 0;
    virtual int32_t OutputBlockInfo(IMemoryManagerFile* apFile, bool abFullWalk) = 0;
    virtual uint32_t Size(const void* apMem, uint32_t aiUnused) = 0;
    virtual uint32_t TotalSize(const void* apMem) = 0;
    virtual void CheckDelayedFreeBlocks() {}

    static void operator delete(void* apMemory) {
        MemoryManager_Singleton.Deallocate(apMemory);
    }
};
