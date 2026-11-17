#pragma once

#include "Havok/Source/Common/Base/Memory/Allocator/FreeList/hkFreeListAllocatorServer.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"

#include <cstddef>
#include <stdint.h>

class hkMemoryBlockServer;
struct hkMemTreeChunk;

// The header of one block in a page: the size of the block before it (when free) and this block's size and flags.
struct hkMemChunk {
    // Inlines over the chunk header: the page is raw memory the allocator carves into chunks.
    bool isPinuse() const {
        return (head & PINUSE_BIT) != 0;
    }
    bool isInuse() const {
        return (head & CINUSE_BIT) != 0;
    }
    uint32_t getChunkSize() const {
        return head & ~INUSE_BITS;
    }
    uint32_t getPayloadSize() const {
        return getChunkSize() - PAYLOAD_OFFSET;
    }
    void* getPayload() {
        return reinterpret_cast<char*>(this) + PAYLOAD_OFFSET;
    }
    hkMemChunk* chunkPlusOffset(size_t s) {
        return reinterpret_cast<hkMemChunk*>(reinterpret_cast<char*>(this) + s);
    }
    hkMemChunk* chunkMinusOffset(size_t s) {
        return reinterpret_cast<hkMemChunk*>(reinterpret_cast<char*>(this) - s);
    }
    hkMemChunk* nextChunk() {
        return chunkPlusOffset(getChunkSize());
    }
    hkMemChunk* previousChunk() {
        return chunkMinusOffset(prevFoot);
    }
    void setFoot(uint32_t s) {
        chunkPlusOffset(s)->prevFoot = s;
    }

    static hkMemChunk* toChunk(void* in) {
        return reinterpret_cast<hkMemChunk*>(static_cast<char*>(in) - PAYLOAD_OFFSET);
    }

    static const hkMemChunk* toChunk(const void* in) {
        return reinterpret_cast<const hkMemChunk*>(static_cast<const char*>(in) - PAYLOAD_OFFSET);
    }

    static bool isAligned(const void* ptr) {
        return (reinterpret_cast<uintptr_t>(ptr) & ALIGN_MASK) == 0;
    }

    uint32_t prevFoot;
    uint32_t head;

    // The low bits of head: the previous chunk and this chunk are in use; the rest is the chunk size.
    static constexpr uint32_t PINUSE_BIT = 1;
    static constexpr uint32_t CINUSE_BIT = 2;
    static constexpr uint32_t INUSE_BITS = 3;
    // Chunks are 16-aligned; a used chunk's payload starts 16 bytes past its header.
    static constexpr uintptr_t ALIGN_MASK = 15;
    static constexpr uint32_t PAYLOAD_OFFSET = 16;
};

// One page taken from the system: a doubly linked list node with the allocation count and the page's range.
struct hkMemPage {
    // The page's first chunk and its footer chunk, PAYLOAD_OFFSET bytes before its end (the page's contents are raw
    // memory the allocator carves into chunks).
    hkMemChunk* getFirstChunk() const {
        return reinterpret_cast<hkMemChunk*>(m_start);
    }
    hkMemChunk* getFooter() const {
        return reinterpret_cast<hkMemChunk*>(m_end - hkMemChunk::PAYLOAD_OFFSET);
    }

    hkMemPage* m_prev;
    hkMemPage* m_next;
    int32_t m_numAllocs;
    uint32_t m_size;
    char* m_start;
    char* m_end;
};

// A free chunk: the doubly linked bin list around it.
struct hkFreeMemChunk : hkMemChunk {
    hkFreeMemChunk* next;
    hkFreeMemChunk* prev;
};

// A free chunk large enough for the size trees: its children, parent and tree bin.
struct hkMemTreeChunk : hkFreeMemChunk {
    hkMemTreeChunk* child[2];
    hkMemTreeChunk* parent;
    uint32_t index;
};

// Told by a large-block allocator when an allocation fails or would pass the memory limit.
class hkLargeBlockLimitedMemoryListener {
public:
    virtual ~hkLargeBlockLimitedMemoryListener() {}

    virtual void cannotAllocate(uint32_t auiSize) = 0;
    virtual void allocationFailure(uint32_t auiSize) = 0;
};

// Havok's large-block allocator (a dlmalloc-style heap over pages from an hkMemoryBlockServer): the allocator behind
// hkFreeListAllocator's blocks too big for a free list.
class hkLargeBlockAllocator : public hkFreeListAllocatorServer {
public:
    // A walk over every block of every page: the current chunk, its page and the page's end chunk.
    class Iterator {
    public:
        hkMemChunk* m_chunk;
        hkMemPage* m_page;
        hkMemChunk* m_end;
    };

    enum AllocType : int32_t {
        CRITICAL_ALLOC = 0x0,
        NORMAL_ALLOC = 0x1,
    };

    hkLargeBlockAllocator(hkMemoryBlockServer* server);
    ~hkLargeBlockAllocator() override;

    void* alloc(uint32_t bytes) override;
    void ourFree(void* apPtr) override;
    hkBool isValidAlloc(const void* in) override;
    uint32_t getAllocatedSize(const void* mem, uint32_t) override;
    uint32_t getAllocatedSizeWithOverhead(const void* mem, uint32_t) override;
    void* criticalAlloc(uint32_t size) override;
    void criticalFree(void* ptr, uint32_t size) override;

    // Resets the allocator to empty.
    void _init();
    // Frees every page back to the block server and resets the
    // allocator.
    void quit();
    // The largest chunk size in the tree under t, or largest when that is bigger: walks the right-most path, taking the
    // left child where there is no right one.
    uint32_t _findLargestTreeBlockSize(hkMemTreeChunk* t, uint32_t largest);
    // Fills stats with the allocator's totals, use and largest free block.
    void calculateStatistics(hkMemoryAllocator::MemoryStatistics& stats);
    // .
    // Havok SDK signature (hkBase.lib, tied by bytes,reloc).
    uint32_t findLargestBlockSize();
    // The bytes of all the pages the allocator holds.
    uint32_t calculateMemoryUsedByThisAllocator() const;
    // Havok SDK signature (hkBase.lib, tied by bytes,reloc).
    void _insertLargeChunk(hkMemTreeChunk* x, uint32_t s);
    // Havok SDK signature (hkBase.lib, tied by bytes,reloc).
    void _unlinkLargeChunk(hkMemTreeChunk* x);
    // Havok SDK signature (hkBase.lib, tied by bytes,reloc).
    void _makeTopValid() const;
    hkBool* _resizeSingleBlockServerPage(hkBool* apResult, uint32_t newSize);
    // Havok SDK signature (hkBase.lib, tied by bytes,reloc).
    void* _allocLarge(uint32_t nb);
    // Havok SDK signature (hkBase.lib, tied by bytes,reloc).
    void* _allocFromTop(uint32_t nb);
    // Havok SDK signature (hkBase.lib, tied by bytes,reloc).
    void* _alloc(uint32_t nb);
    // Havok SDK signature (hkBase.lib, tied by bytes,reloc).
    void garbageCollect();
    // Returns the first block of the first page in apResult; a null chunk when there are no pages.
    Iterator* getIterator(Iterator* apResult);
    // Steps iter to the next block; false at the end.
    hkBool* nextBlock(hkBool* apResult, Iterator& arIter);
    // The bytes of all used blocks.
    uint32_t _calculateUsed();
    // Whether mem is a live allocation.
    bool _checkUsedAlloc(const void* mem);
    // Whether p is a well-formed free chunk.
    hkBool* _checkFreeChunk(hkBool* apResult, hkMemChunk* p);
    hkBool* checkAllocations(hkBool* apResult, void** allocs, int32_t size);
    // Calls callback(payload, payload size, param) for every used block.
    void forAllAllocs(void (*callback)(void*, uint32_t, void*), void* param);

    // forAllAllocs' callback appending data to the hkArray<void*> param.
    static void _addAlloc(void* data, uint32_t size, void* param);
    // The index of the single set bit of x.
    static uint32_t _bitToIndex(uint32_t x);
    // The tree bin of a chunk of s bytes.
    static uint32_t _computeTreeIndex(uint32_t s);

    hkLargeBlockLimitedMemoryListener* m_limitedListener;
    uint32_t m_treemap;
    char* m_leastAddr;
    hkMemChunk* m_top;
    uint32_t m_topsize;
    hkMemTreeChunk* m_treebins[32];
    hkMemoryBlockServer* m_server;
    hkMemChunk m_zeroChunk;
    hkMemPage m_pages;
    uint32_t m_sumAllocatedWithMgrOverhead;
    uint32_t m_sumAllocatedSize;
};
