#pragma once

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"

// The allocator the Havok code allocates on: it keeps chains of free blocks for the small sizes and falls back to
// the global memory system for everything else.
class hkThreadMemory : public hkMemoryAllocator {
public:
	// One chain of equally sized free blocks, chained through their first word; m_numElem counts them.
	struct FreeList {
		// A free block, its first word linking to the next.
		struct FreeElem {
			FreeElem* m_next;
		};

		FreeElem* m_head;
		int32_t m_numElem;
	};

	enum {
		PAGE_ALIGN = 64,
		MEMORY_SMALL_BLOCK_RSHIFT_BITS = 4,
		MEMORY_SMALL_BLOCK_ADD = 0xF,
		MEMORY_MAX_SMALL_ROW = 13,
		MEMORY_MAX_ALL_ROW = MEMORY_MAX_SMALL_ROW + 4,
		MEMORY_MAX_SIZE_SMALL_BLOCK = 640,
		MEMORY_MAX_SIZE_LARGE_BLOCK = 8192,
		MEMORY_LARGE_BLOCK_RSHIFT_BITS = 10,
		MEMORY_SYSTEM_PAGE_SIZE = 8192,
		MEMORY_MAGIC_NUMBER = 0x3425234,
		// How many blocks a batch call moves.
		BATCH_SIZE = 4,
	};

	// The constructor sets up 17 empty free lists and the lookup tables, then a cleared memory system.
	hkThreadMemory();
	// Sets up 17 empty free lists and the lookup tables, then calls setMemory(memoryInstance, 8).
	hkThreadMemory(hkMemoryAllocator* memoryInstance);
	~hkThreadMemory() override;

	void* blockAlloc(int32_t nbytes) override;
	void blockFree(void* p, int32_t nbytes) override;
	void* bufAlloc(int32_t& reqNumInOut) override;
	void bufFree(void* p, int32_t numElem) override;
	// Allocates a buffer of *apiNumBytes, copies what fits of the old one into it and frees the old one.
	void* bufRealloc(void* pold, int32_t oldNum, int32_t& reqNumInOut) override;
	void blockAllocBatch(void** ptrsOut, int32_t numPtrs, int32_t blockSize) override;
	void blockFreeBatch(void** ptrsIn, int32_t numPtrs, int32_t blockSize) override;
	// forwards to the memory system's own canAllocTotal
	bool canAllocTotal(int32_t numBytes) override;
	void getMemoryStatistics(hkMemoryAllocator::MemoryStatistics& u) override;
	int32_t getAllocatedSize(const void* obj, int32_t numBytes) override;
	virtual void releaseCachedMemory();

	// The bytes the free lists hold, without the locks the checked version takes.
	uint32_t getCachedSizeUnchecked();

	// The free list row a request of nbytes bytes is served from.
	int32_t getRow(int32_t nbytes) const {
		return (nbytes <= MEMORY_MAX_SIZE_SMALL_BLOCK)
			? int32_t(m_small_size_to_row_lut[(nbytes + MEMORY_SMALL_BLOCK_ADD) >> MEMORY_SMALL_BLOCK_RSHIFT_BITS])
			: m_large_size_to_row_lut[(nbytes - 1) >> MEMORY_LARGE_BLOCK_RSHIFT_BITS];
	}

	// The block size of a row.
	int32_t rowToSize(int32_t row) const { return m_row_to_size_lut[row]; }
	// Hands a block back to the row's free list, giving a batch to the memory when the list is full.
	void onRowFull(int32_t rowIndex, void* p);
	// Frees every block of the row's free list.
	void clearRow(int32_t rowIndex);
	// Fills the size lookup tables.
	void initLookupTables();

	// Points the thread memory at another allocator; a null one clears it and leaves the limit alone.
	void setMemory(hkMemoryAllocator* memoryInstance, int32_t maxNumElemsOnFreeList);
	// blockAlloc's fallback when the free list of row is empty; returns a block of the row's
	// size.
	void* onRowEmpty(int32_t rowIndex);

	// The row for a size known at compile time.
	static int32_t constSizeToRow(int32_t size);

	hkMemoryAllocator* m_memory;
	int32_t m_maxNumElemsOnFreeList;
	FreeList m_free_list[17];
	int32_t m_row_to_size_lut[17];
	char m_small_size_to_row_lut[41];
	int32_t m_large_size_to_row_lut[8];
};
