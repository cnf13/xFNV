#pragma once

#include "Havok/Source/Common/Base/Memory/Allocator/FreeList/hkFreeList.h"
#include "Havok/Source/Common/Base/Memory/Allocator/FreeList/hkLargeBlockAllocator.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Thread/CriticalSection/hkCriticalSection.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkMemoryBlockServer;
class hkOstream;

// Sets the memory system's out-of-memory state (cdecl, 0xD88040; file-local in Havok, called from other files here).
void hkSetOutOfMemoryState(hkMemoryAllocator::MemoryState state);

// Havok's memory allocator that serves small blocks from per-size free lists and large ones from a large-block
// allocator, under a critical section.
class hkFreeListAllocator : public hkMemoryAllocator {
public:
	// Values from the Xbox PDB.
	enum : int32_t {
		FREELIST_ALIGNMENT = 0x10,
		MAX_FREELIST_SIZE = 0x280,
		FREELIST_SHIFT = 0x4,
		MAX_FREELISTS = 0x29,
		MAX_UNIQUE_FREELISTS = 0x29,
	};

	// The PDB's nested hkFreeListAllocator::MemoryWalkCallback: void (*)(void*, unsigned, hkBool, int, void*).
	typedef void (*MemoryWalkCallback)(void*, uint32_t, hkBool, int32_t, void*);

	hkFreeListAllocator(hkMemoryBlockServer* server);
	~hkFreeListAllocator() override;

	void* blockAlloc(int32_t nbytes) override;
	void blockFree(void* p, int32_t nbytes) override;
	void* bufAlloc(int32_t& reqNumInOut) override;
	void blockAllocBatch(void** blocksOut, int32_t nblocks, int32_t nbytes) override;
	void blockFreeBatch(void** blocks, int32_t nblocks, int32_t nbytes) override;
	bool canAllocTotal(int32_t) override;
	void getMemoryStatistics(hkMemoryAllocator::MemoryStatistics& stats) override;
	int32_t getAllocatedSize(const void* p, int32_t nbytes) override;
	void resetPeakMemoryStatistics() override;
	virtual void quit();
	// Havok SDK signature (hkBase.lib, tied by string,vtable).
	virtual void printStatistics(hkOstream* c);
	// Havok SDK signature (hkBase.lib, tied by vtable).
	virtual void reservePagesFromSystemMemory(int32_t numBytes);
	virtual void garbageCollect();
	virtual void incrementalGarbageCollect(int32_t numBlocks);
	virtual void optimize();
	// Havok SDK signature (hkBase.lib, tied by bytes,reloc,vtable).
	virtual void setMemorySoftLimit(uint32_t maxMemory);
	virtual uint32_t getMemorySoftLimit();
	virtual void releaseUnusedPagesToSystemMemory();
	virtual hkBool isOk() const;
	virtual hkResult walkMemory(MemoryWalkCallback callback, void* param);

	void init(hkMemoryBlockServer* a1);

protected:
	// The free list for elements of this size: takes the next unused list of m_freeListMemory, sets it up and files it
	// in m_freeLists. A list already filed for the same element size wins and the new one is dropped.
	hkFreeList* _newFreeList(uint32_t elementSize, uint32_t alignment, uint32_t blockSize);
	// Drops a list whose element size a filed list already covers.
	void _deleteFreeList(hkFreeList* freeList);

public:
	// The pointer less-than the exe folds for its sorts of pointer arrays (the quickSortRecursive calls of
	// hkFreeList.cpp and hkFreeListAllocator.cpp pass it): *arResult = a < b, compared as unsigned addresses
	// (cmp/setb), the hkBool returned through the hidden result pointer, which it also returns (cdecl, ret 0,
	// 0xDD70F0). PDB: static hkBool _comparePointers(void*, void*).
	static hkBool* _comparePointers(hkBool* arResult, const void* a, const void* b);

	// Held by every method that touches the free lists and the large-block allocator; mutable as in the Havok SDK,
	// since the const isOk takes it too.
	mutable hkCriticalSection m_criticalSection;
	uint32_t m_totalBytesInFreeLists;
	uint32_t m_peakInUse;
	hkMemoryBlockServer* m_server;
	hkLargeBlockAllocator m_largeAllocator;
	hkFreeList* m_sizeToFreeList[41];
	hkFreeList* m_freeLists[41];
	int32_t m_numFreeLists;
	hkFreeList* m_topFreeList;
	hkFreeList* m_lastFreeList;
	hkFreeList m_freeListMemory[41];
	uint32_t m_softLimit;
	int32_t m_incrementalFreeListIndex;
};
inline constexpr fnvd::Vtable hkFreeListAllocator_vftable{};
