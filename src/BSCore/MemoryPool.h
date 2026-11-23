#pragma once

#include "BSCore/BSSpinLock.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <stdint.h>

// A pool's usage figures, as MemoryPool::GetStats reports them.
class MemoryPoolStats {
public:
    const char* pDesc;
    uint32_t iOverhead;
    uint32_t iBlockSize;
    uint32_t iReservedMem;
    uint32_t iReservedPages;
    uint32_t iCommittedMem;
    uint32_t iCommittedPages;
    uint32_t iHighCommittedPages;
    uint32_t iFreeBlocks;
    uint32_t iUsedBlocks;
};

// A fixed-block-size pool the MemoryManager serves small allocations from. Only the fields the MemoryManager code reads
// are declared.
class MemoryPool {
public:
    // A free block of the pool, linked to its neighbours.
    struct FreePoolBlock {
        FreePoolBlock* pPrevious;
        FreePoolBlock* pNext;
    };

    // Pools by block size / 4, for blocks up to 0x3FC bytes.
    using PoolArray = MemoryPool* [256];

    MemoryPool(uint32_t aiBlockSize, uint32_t aiPoolSize, const char* apDesc);
    ~MemoryPool();

    // Serves a block from the free list, committing a page when the list is empty.
    void* Allocate(uint32_t requested);

    // Returns apPointer's block to the free list and drops its page's use count, under the pool's lock.
    void Free(void* apPointer);
    void PlatformReserveHeap();
    bool PlatformCommitPage(uint32_t auiPageIndex, char*& apPageAddress);
    bool PlatformDecommitPage(uint32_t auiPageIndex, char* apPageAddress);
    void AddToLookupArrays();
    void AddPoolAsBlockSize(uint32_t aiSize);
    void RemovePoolAsBlockSize(uint32_t aiSize);
    // Walks the free list under the pool's lock (a debug check whose body is empty in this build).
    void CheckFreeBlocks();
    void PlatformReleaseHeap();

protected:
    uint32_t GetFreePageIndex();
    // Pushes apPointer onto the head of the free list.
    void AddFreeBlock(void* apPointer);
    void RemoveFreeBlock(MemoryPool::FreePoolBlock* apFreeBlock);
    // Decommits every page no block is allocated from, sorting the free list before the first one.
    void FreeUnusedPages();
    // Unlinks page aiPageIndex's blocks from the (sorted) free list and decommits the page.
    void FreePage(uint32_t aiPageIndex, bool abSortRequired);
    // Merge sorts the free block list apList by address; returns its new head.
    FreePoolBlock* MergeSortList(FreePoolBlock* apList);
    // Sorts the free list when it holds more than one block.
    void SortFreeBlockList();

public:
    void RemoveFromLookupArrays();
    // The pool's usage figures, returned through apResult.
    MemoryPoolStats* GetStats(MemoryPoolStats* apResult) const;

    static MemoryPool* GetPoolForSize(uint32_t aiBlockSize);
    static void InitPools();
    // Initialises the guarded critical section on first use, then checks the integrity of every pool.
    static void CheckPools();
    // Sorts the free list of the pool of block size index aiIndex under its lock.
    static void SortFreeBlocks(int32_t aiIndex);
    static void DeletePools();
    // Initialises the guarded critical section on first use, then decommits the unused pages of every pool.
    static void FreeUnusedPagesForAllPools();

    const char* pDescription;
    char* pHeap;
    FreePoolBlock* pFreeHead;
    uint8_t padding0C[0x14];
    BSSpinLock CriticalSectionO;
    uint32_t iAlignedBlockSize;
    uint32_t iHighCommitedPages;
    // Users per page; 0xFFFF marks an uncommitted page. Read zero-extended.
    uint16_t* pPageRefCount;
    uint32_t iNumPages;
    uint32_t iPoolSize;
    uint32_t iNumFreeBlocks;
    uint32_t iNumCommittedPages;
    // The constructor stores blocksPerPage / 6 in dword5C and blocksPerPage - blocksPerPage / 6 in dword60; no other
    // function reads them.
    uint32_t dword5C;
    uint32_t dword60;
    // The rest of the class. Nothing reads or writes any of these bytes, so they are left untyped.
    uint8_t padding64[0x1C];

    static inline PoolArray pMemoryPoolsBySize{};
    // Pools by the top byte of an address inside them.
    static inline PoolArray pMemoryPoolsByAddress{};
};
