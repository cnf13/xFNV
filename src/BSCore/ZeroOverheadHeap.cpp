#include "BSCore/ZeroOverheadHeap.h"

#include "BSCore/IMemoryHeap.h"
#include "BSCore/IMemoryManagerFile.h"
#include "BSCore/MemoryManager.h"
#include "Reconstruction.hpp"

void* ZeroOverheadHeap::Allocate(uint32_t aiSize, uint32_t aiAlignment) {
    if (!pHeap) {
        return nullptr;
    }
    void* pmem = nullptr;
#if FNVD_64BIT
    // The 64-bit program aligns each block to 16 bytes, as the platform's allocators do (a departure: the exe's
    // blocks follow each other with only the size word between them); the size word stays right before the block.
    char* pBlock = reinterpret_cast<char*>((reinterpret_cast<uintptr_t>(pCurrentFree) + 4 + 15) & ~uintptr_t(15));
    if (pBlock + aiSize < &pHeap[iSize]) {
        CriticalSectionO.Lock(nullptr);
        reinterpret_cast<uint32_t*>(pBlock)[-1] = aiSize;
        pmem = pBlock;
        pCurrentFree = pBlock + aiSize;
        ++iAllocations;
        if (!--CriticalSectionO.uiLockCount) {
            CriticalSectionO.uiOwningThread = 0;
        }
    }
#else
    if (&pCurrentFree[aiSize + 4] < &pHeap[iSize]) {
        CriticalSectionO.Lock(nullptr);
        // Each block starts with its size, which Size reads back: the heap's bytes are untyped memory.
        *reinterpret_cast<uint32_t*>(pCurrentFree) = aiSize;
        pmem = pCurrentFree + 4;
        pCurrentFree = static_cast<char*>(pmem) + aiSize;
        ++iAllocations;
        // EXE-INLINED none: BSSpinLock::Unlock
        if (!--CriticalSectionO.uiLockCount) {
            CriticalSectionO.uiOwningThread = 0;
        }
    }
#endif
    return pmem;
}

ZeroOverheadHeap::ZeroOverheadHeap(uint32_t aiSize, const char* apName)
  : iSize(aiSize), pName(apName), iAllocations(0) {
    pHeap = static_cast<char*>(MemoryManager_Singleton.DoSystemAllocation(aiSize, true));
    pCurrentFree = pHeap;
}

ZeroOverheadHeap::~ZeroOverheadHeap() {
    MemoryManager_Singleton.DoSystemDeallocation(pHeap, true);
}

const char* ZeroOverheadHeap::GetName() {
    return pName;
}

// Individual allocations are never freed.
void ZeroOverheadHeap::Deallocate(void* apMem, uint32_t aiUnused) {}

bool ZeroOverheadHeap::PointerInHeap(const void* apPointer) {
    return apPointer > pHeap && apPointer < &pHeap[iSize];
}

void ZeroOverheadHeap::GetHeapStats(HeapStats* apStats, bool abFullBlockInfo) {
    if (apStats) {
        apStats->pHeapName = GetName();
        apStats->MemHeapSize = iSize;
        apStats->MemHeapCommitted = iSize;
        apStats->iMemAllocatedToBlocks = pCurrentFree - pHeap;
        apStats->iNumBlocks = iAllocations;
        apStats->iNumFreeBlocks = 0;
        apStats->iMemFreeInBlocks = 0;
        apStats->iMemUsedInBlocks = pCurrentFree - pHeap;
        apStats->iSmallestFreeBlock = 0;
        apStats->iLargestFreeBlock = 0;
        apStats->iHeapOverhead = 64;
        apStats->iFreeListOverhead = 0;
        apStats->iBlockOverhead = 4 * iAllocations;
        apStats->iMemDebugOverhead = 0;
    }
}

int32_t ZeroOverheadHeap::OutputBucketInfo(IMemoryManagerFile* apFile) {
    int32_t v3;
    if (!apFile) {
        return 0;
    }
    v3 = 0;
    return apFile->Write(&v3, 4u);
}

int32_t ZeroOverheadHeap::OutputBlockInfo(IMemoryManagerFile* apFile, bool abFullWalk) {
    if (!apFile) {
        return 0;
    }
    int32_t v4 = 0;
    return apFile->Write(&v4, 4u);
}

uint32_t ZeroOverheadHeap::Size(const void* apMem, uint32_t aiUnused) {
    return *(static_cast<const uint32_t*>(apMem) - 1);
}

uint32_t ZeroOverheadHeap::TotalSize(const void* apMem) {
    return Size(apMem, 0) + 4;
}
