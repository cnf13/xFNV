#pragma once

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkFreeListAllocatorServer;

// A free list allocator: the memory system hands it blocks out of which the elements are carved and it hands them
// out one at a time.
class hkFreeList {
public:
	// One element of the list; the free elements are chained through it.
	struct Element {
		Element* m_next;
	};

	// One block the elements were carved out of.
	struct Block {
		Block* m_next;
		uint32_t m_blockSize;
		uint8_t* m_start;
		uint32_t m_numElements;
	};

	hkFreeList();
	// The constructor: _init on apMemory (thiscall, ret 0x10, 0xDD7650; Havok SDK symbol, hkBase.lib, tied by bytes
	// and reloc).
	hkFreeList(uint32_t aiElementSize, uint32_t aiAlignment, uint32_t aiBlockSize,
		hkFreeListAllocatorServer* apMemoryServer);

	// Returns every block the list owns to the memory server.
	void Free();
	// Returns the blocks on the free-block list to the memory server
	// and empties it: freeAllFreeBlocks, its count dropped.
	void _freeFreeBlocks() { freeAllFreeBlocks(); }
	// Checks up to aiNumBlocks used blocks, from where the last call stopped, and moves each whose elements are all
	// free onto the free blocks; aiNumBlocksOut gets the count checked. True once the walk has passed the last block.
	hkBool* incrementalFindGarbage(hkBool* arResult, int32_t aiNumBlocks, int32_t& aiNumBlocksOut);
	// Takes apBlock's elements off the free list; nonzero (and the elements stay off) when they were all free
	// (thiscall, ret 4, 0xDD7B40). The PDB name is inferred from the shape; IDA has none.
	int32_t _calcBlockFree(Block* block);
	// Fills stats with the list's figures.
	void calculateStatistics(hkMemoryAllocator::MemoryStatistics& stats);
	// Takes a block (a free one, else a new one from the memory server), makes its elements the ones handed out next
	// and returns the first one, null when the server has no memory.
	void* addSpace();
	// The address has no PDB name (IDA's is _sub_DD74B0); the name is the PDB's hkFreeList::_init, the only member
	// whose shape fits: the constructor (0xDD7650) and init (0xDD7510) call it and it only fills the fields. Sets
	// the list up for elements of aiElementSize rounded up to aiAlignment, blocks of aiBlockSize and apMemoryServer
	// as the memory server (thiscall, ret 0x10, 0xDD74B0). The eax it leaves (the largest block size) is read by no
	// caller.
	void _init(uint32_t elementSize, uint32_t align, uint32_t blockSize, hkFreeListAllocatorServer* memoryServer);
	// The elements of both lists, the free blocks' first (thiscall, ret 0, 0xDD72D0; Havok SDK symbol, hkBase.lib,
	// tied by bytes). The SDK's getTotalNumElements is the inline read of m_totalNumElements.
	uint32_t _calcTotalNumElements() const;
	void freeAll();
	// Returns the free blocks to the memory server and empties the list; the number of blocks freed.
	int32_t freeAllFreeBlocks();
	// Returns the blocks of the list from cur to the memory server, taking their elements off the counts; the number
	// of blocks freed.
	int32_t _freeBlocks(Block* cur);

	// The SDK's inline: the block's elements become the ones handed out from m_top on.
	void _addBlockElements(Block* block) {
		m_top = block->m_start;
		m_blockEnd = block->m_start + block->m_numElements * m_elementSize;
	}

	// Whether every free element is aligned and on an element boundary of a valid used block (thiscall, ret 4,
	// 0xDD7140; Havok SDK signature, hkBase.lib, tied by bytes,reloc). The SDK's hkBool result comes back through
	// the hidden result pointer apResult.
	hkBool* _checkFreeBlocks(hkBool* apResult);
	uint32_t _calcNumFreeElements();
	// Havok SDK signature (hkBase.lib, tied by bytes,reloc).
	uint32_t _calculateBlockAllocatedSize(hkFreeList::Block* cur);
	// Havok SDK signature (hkBase.lib, tied by bytes,reloc).
	void getBlocks(void** blocks, int32_t numBlocks);
	// Frees the list's memory, then sets it up again as _init does (thiscall, ret 0x10, 0xDD7510; Havok SDK symbol,
	// hkBase.lib, tied by bytes).
	void init(uint32_t elementSize, uint32_t align, uint32_t blockSize, hkFreeListAllocatorServer* memoryServer);
	// Hands callback every element of the blocks from block on, as not allocated (thiscall, ret 0x10, 0xDD72F0;
	// Havok SDK signature, hkBase.lib, tied by bytes,reloc).
	void _walkMemoryBlockList(hkFreeList::Block* block, void (*callback)(void*, uint32_t, hkBool, int32_t, void*),
		int32_t pool, void* param);
	// Havok SDK signature (hkBase.lib, tied by bytes,reloc).
	void walkMemory(void (*callback)(void*, uint32_t, hkBool, int32_t, void*), int32_t pool, void* param);
	void freeSmallFreeBlocks(uint32_t minSize);
	// Moves the used blocks whose elements are all free onto the free blocks; the number moved, -1 when the memory
	// server has no memory.
	int32_t findGarbage();

	// The block size for elementSpace bytes of elements: the space plus align, at least 16, for the block header.
	static uint32_t bestBlockSize(uint32_t elementSpace, uint32_t align);
	// The elements of the blocks in the list from apBlock.
	static uint32_t _calcTotalNumElements(Block* apBlock);

	Element* m_free;
	uint32_t m_elementSize;
	Block* m_lastIncrementalBlock;
	Block* m_blocks;
	Block* m_freeBlocks;
	uint32_t m_blockSize;
	uint32_t m_align;
	uint32_t m_maxBlockSize;
	uint8_t* m_top;
	uint8_t* m_blockEnd;
	uint32_t m_numFreeElements;
	uint32_t m_totalNumElements;
	hkFreeListAllocatorServer* m_memoryServer;
};
