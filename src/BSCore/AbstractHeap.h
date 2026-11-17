#pragma once

#include "BSCore/HeapBlock.h"
#include "BSCore/IMemoryHeap.h"
#include "BSCore/LogItem.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"

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

// Common part of the block-based heaps. Only the fields the reconstructed functions read are declared.
class AbstractHeap : public IMemoryHeap {
public:
	AbstractHeap(uint32_t aiSize, uint32_t aiInitialSize, const char* apName, uint32_t aiPageSize);
	~AbstractHeap() override;

	const char* GetName() override { return pName; }
	bool PointerInHeap(const void* apPointer) override;
	void CheckHeapIntegrity() override;
	void GetHeapStats(HeapStats* apStats, bool abFullBlockInfo) override;
	int32_t OutputBucketInfo(IMemoryManagerFile* apFile) override;
	int32_t OutputBlockInfo(IMemoryManagerFile* apFile, bool abOutputWalk) override;
	void CheckDelayedFreeBlocks() override {}
	virtual void* DoHeapAllocation(uint32_t aiSize, uint32_t aiInitialSize) = 0;
	virtual void DoHeapFree(void* apPtr) = 0;
	virtual uint32_t CreateMorePages(void*, uint32_t, uint32_t) { return 0; }
	virtual uint32_t CleanExtraPages(void*, uint32_t, uint32_t) { return 0; }
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

	static void operator delete(void* apMemory) { MemoryManager_Singleton.Deallocate(apMemory); }

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
inline constexpr fnvd::Vtable AbstractHeap_vftable{};
