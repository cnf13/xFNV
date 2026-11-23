#pragma once

#include "BSCore/HeapBlock.h"
#include "BSCore/IMemoryHeap.h"
#include "BSCore/LogItem.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

// A free block: the HeapBlock header followed by the free-list links stored in the block's memory.
class FreeHeapBlock : public HeapBlock {
public:
    // Unlinks the block from its free list's neighbours and clears its links.
    void Remove();

    FreeHeapBlock* pPrevFree;
    FreeHeapBlock* pNextFree;
};

// A free list: its first and last free blocks.
class FreeHeapBlockList {
public:
    FreeHeapBlock* pHead;
    FreeHeapBlock* pTail;
};

// One sub-bucket of the small free lists: the count of free blocks in it and its first list.
struct HeapFreeSubBucket {
    uint32_t iCount;
    FreeHeapBlockList* pFirst;
};

// Common part of the block-based heaps. Only some of the fields are declared.
class AbstractHeap : public IMemoryHeap {
public:
    AbstractHeap(uint32_t aiSize, uint32_t aiInitialSize, const char* apName, uint32_t aiPageSize);
    ~AbstractHeap() override;

    const char* GetName() override {
        return pName;
    }
    bool PointerInHeap(const void* apPointer) override;
    void CheckHeapIntegrity() override;
    void GetHeapStats(HeapStats* apStats, bool abFullBlockInfo) override;
    int32_t OutputBucketInfo(IMemoryManagerFile* apFile) override;
    int32_t OutputBlockInfo(IMemoryManagerFile* apFile, bool abOutputWalk) override;
    void CheckDelayedFreeBlocks() override {}
    virtual void* DoHeapAllocation(uint32_t aiSize, uint32_t aiInitialSize) = 0;
    virtual void DoHeapFree(void* apPtr) = 0;
    virtual uint32_t CreateMorePages(void*, uint32_t, uint32_t) {
        return 0;
    }
    virtual uint32_t CleanExtraPages(void*, uint32_t, uint32_t) {
        return 0;
    }
    virtual void DecommitPages(FreeHeapBlock*) {}
    virtual void CommitPages(FreeHeapBlock*, uint32_t) {}

    // protected in the engine; public here so the replacement table can name them
    void* BaseAllocate(uint32_t aSize);
    void BaseFree(void* apPointer);
    void MergeFreeBlocks(FreeHeapBlock* apFree1, FreeHeapBlock* apFree2);
    void AddBlockToList(HeapBlock* apNewBlock);
    // Merges a free block with its free neighbours.
    void TryAndDefragment(FreeHeapBlock* apNewFree);
    // Links a block into its free list.
    void AddBlockToFreeList(FreeHeapBlock* apBlock);
    // Unlinks a block from the given free list.
    void RemoveBlockFromFreeList(FreeHeapBlockList* apList, FreeHeapBlock* apBlock);
    // Unlinks a block from its free list.
    void RemoveBlockFromFreeList(FreeHeapBlock* apBlock);

    void CleanEnd();
    void IncrementSubBucket(int32_t aiBucketSize);
    void DecrementSubBucket(int32_t aiBucketSize);
    void FreeDelayedFreeBlocks();
    void AddDelayedFreeBlock(HeapBlock* apBlock);
    // The entry's index in the heap's log chain: how many entries are reachable from apLogItem through its links;
    // -1 for a null entry.
    int32_t GetLogIndex(LogItem* apLogItem);

protected:
    void CheckFreeBlock(FreeHeapBlock* apBlock);

public:
    void InitHeap();
    void DeleteHeap();
    void OutputBlockFailure(uint32_t desired_size, FreeHeapBlockList* list);

    static void operator delete(void* apMemory) {
        MemoryManager::Instance().Deallocate(apMemory);
    }

    // The block heaps' geometry. A block's header (HeapBlock) sits right before its memory and the next block follows
    // that memory; a block's memory has room for a free block's two links; block sizes are steps of the alignment, and
    // the small free lists and their sub-buckets cover 0x400 steps. The header is 16 bytes (a pointer and the size
    // word), the smallest memory 16, and blocks are aligned to 16 bytes as the platform's allocators do, deliberately
    // differing from the game's 8-byte header, 8-byte smallest memory and 4-byte alignment.
    static constexpr uint32_t kHeader = sizeof(HeapBlock);
    static constexpr uint32_t kMinMemory = sizeof(FreeHeapBlock) - sizeof(HeapBlock);
    static constexpr uint32_t kAlignment = 16;
    static constexpr int32_t kSubBucketLimit = 0x400 * kAlignment;

    const char* pName;
    uint32_t iDefaultAlignment;
    uint32_t MinFreeBlockSize;
    uint32_t iPageSize;
    uint32_t MemHeapSize;
    uint32_t iInitialSize;
    uint32_t iCurrentSize;
    uint32_t iWastedMemory;
    int32_t iMemAllocated;
    int32_t iMemAllocatedHigh;
    int32_t iBlockMemAllocated;
    char* pMemHeap;
    int32_t iNumBlocks;
    HeapBlock* pBlockHead;
    HeapBlock* pBlockTail;
    int32_t iNumFreeBlocks;
    FreeHeapBlockList pSmallFreeList[0x400];
    FreeHeapBlockList* pLastSmallList;
    FreeHeapBlockList LargeFreeList;
    HeapBlock** pDelayedFreeBlocksA;
    uint32_t iNextDelayedFreeIndex;
    HeapFreeSubBucket pSubBuckets[0x10];
    _RTL_CRITICAL_SECTION CriticalSectionO;
};
