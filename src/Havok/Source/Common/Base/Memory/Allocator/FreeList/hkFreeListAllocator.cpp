#include "Havok/Source/Common/Base/Memory/Allocator/FreeList/hkFreeListAllocator.h"

#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"
#include "Havok/Source/Common/Base/Memory/Allocator/FreeList/hkFreeList.h"
#include "Havok/Source/Common/Base/Memory/Allocator/FreeList/hkLargeBlockAllocator.h"
#include "Havok/Source/Common/Base/Memory/Allocator/FreeList/hkMemoryBlockServer.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Thread/CriticalSection/hkCriticalSection.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "SDK/MSVC/string.h"

// Sets the memory system's state.
void hkSetOutOfMemoryState(hkMemoryAllocator::MemoryState state) {
    hkOutOfMemoryState = state;
}

// Sets up the critical section, the large-block allocator and the 41 free lists, then files the lists by size: the
// 512-, 256-, 0- and 544-byte lists first (the 544 one serves 528 to 640 bytes), then every size from 640 down to 16
// in steps of 16 that has none yet, in blocks of 16 times the size from 256 bytes on, else 0x1000.
hkFreeListAllocator::hkFreeListAllocator(hkMemoryBlockServer* server)
  : m_criticalSection(4000), m_totalBytesInFreeLists(0), m_peakInUse(0), m_server(server), m_largeAllocator(server),
    m_numFreeLists(0), m_topFreeList(m_freeListMemory), m_lastFreeList(m_freeListMemory + MAX_UNIQUE_FREELISTS),
    m_incrementalFreeListIndex(0) {
    hkFreeListAllocator::setMemorySoftLimit(0x7FFFFFFFu);
    for (int32_t i = 0; i < MAX_FREELISTS; ++i) {
        m_sizeToFreeList[i] = nullptr;
    }
    m_sizeToFreeList[32] = _newFreeList(512u, 128u, 0x10000u);
    m_sizeToFreeList[16] = _newFreeList(256u, 32u, 0x8000u);
    m_sizeToFreeList[0] = _newFreeList(0, 4u, 256u);
    hkFreeList* pList544 = _newFreeList(544u, 128u, 0x5000u);
    for (int32_t j = 528; j <= 640; ++j) {
        m_sizeToFreeList[j >> 4] = pList544;
    }
    for (int32_t k = 640; k >= 16; k -= 16) {
        if (!m_sizeToFreeList[k >> 4]) {
            uint32_t uiAlign = k >= 64 ? 32u : 16u;
            uint32_t uiBlockSize = static_cast<uint32_t>(k) < 256u ? 0x1000u : 16u * k;
            m_sizeToFreeList[k >> 4] = _newFreeList(k, uiAlign, uiBlockSize);
        }
    }
}

// The vtable, quit, every free list freed from the last one down, then the large-block allocator, the critical
// section and the base.
// The large-block allocator, the critical section and the base go with their own destructors.
hkFreeListAllocator::~hkFreeListAllocator() {
    hkFreeListAllocator::quit();
    for (int32_t i = MAX_UNIQUE_FREELISTS - 1; i >= 0; --i) {
        m_freeListMemory[i].Free();
    }
}

void* hkFreeListAllocator::blockAlloc(int32_t nbytes) {
    return bufAlloc(nbytes);
}

// A block over the largest free-list size goes back to the large-block allocator; any other is pushed onto the free
// list for its size, its first word chained to the list's old free element, and m_totalBytesInFreeLists grows by one
// element. A null block is ignored, and the size is rounded up the way blockAlloc rounds it.
void hkFreeListAllocator::blockFree(void* p, int32_t nbytes) {
    if (p) {
        EnterCriticalSection(&m_criticalSection.m_section);
        if (nbytes > MAX_FREELIST_SIZE) {
            (&m_largeAllocator)->ourFree(p);
        }
        else {
            hkFreeList* pFreeList = m_sizeToFreeList[(nbytes + 15) >> 4];
            uint32_t uiNumFreeElements = pFreeList->m_numFreeElements;
            pFreeList->m_numFreeElements = uiNumFreeElements + 1;
            hkFreeList::Element* pElement = static_cast<hkFreeList::Element*>(p);
            pElement->m_next = pFreeList->m_free;
            pFreeList->m_free = pElement;
            m_totalBytesInFreeLists += pFreeList->m_elementSize * (pFreeList->m_numFreeElements - uiNumFreeElements);
        }
        LeaveCriticalSection(&m_criticalSection.m_section);
    }
}

// A block over the largest free-list size comes from the large-block allocator; any other from its size's free list
// (the SDK's inline hkFreeList::alloc: the free chain, else the current block's top, else a new block), and the
// size is raised to the list's element size. The peak in use follows, and the soft limit sets the out-of-memory state.
void* hkFreeListAllocator::bufAlloc(int32_t& reqNumInOut) {
    void* pResult;
    int32_t auiReqSize = reqNumInOut;
    hkCriticalSection* lpCriticalSection = &m_criticalSection;
    EnterCriticalSection(&m_criticalSection.m_section);
    if (auiReqSize > 640) {
        // alloc is called through a pointer so that it is a virtual call; without the pointer clang would call it
        // directly.
        pResult = (&m_largeAllocator)->alloc(auiReqSize);
    }
    else {
        hkFreeList* v4 = m_sizeToFreeList[(auiReqSize + 15) >> 4];
        hkFreeList::Element* pFree = v4->m_free;
        uint32_t m_numFreeElements = v4->m_numFreeElements;
        // EXE-INLINED none: hkFreeList::alloc
        if (pFree) {
            v4->m_numFreeElements = m_numFreeElements - 1;
            v4->m_free = pFree->m_next;
            pResult = pFree;
        }
        else {
            uint8_t* pTop = v4->m_top;
            if (pTop < v4->m_blockEnd) {
                uint32_t m_elementSize = v4->m_elementSize;
                v4->m_numFreeElements = m_numFreeElements - 1;
                v4->m_top = pTop + m_elementSize;
                pResult = pTop;
            }
            else {
                pResult = v4->addSpace();
            }
        }
        m_totalBytesInFreeLists += v4->m_elementSize * (v4->m_numFreeElements - m_numFreeElements);
        reqNumInOut = v4->m_elementSize;
    }
    uint32_t m_peakInUse_1 = m_largeAllocator.m_sumAllocatedSize - m_totalBytesInFreeLists;
    uint32_t m_peakInUse = this->m_peakInUse;
    if (m_peakInUse_1 > m_peakInUse) {
        m_peakInUse = m_largeAllocator.m_sumAllocatedSize - m_totalBytesInFreeLists;
    }
    this->m_peakInUse = m_peakInUse;
    if (m_peakInUse_1 >= m_softLimit) {
        hkSetOutOfMemoryState(hkMemoryAllocator::MEMORY_STATE_OUT_OF_MEMORY);
    }
    LeaveCriticalSection(&lpCriticalSection->m_section);
    return pResult;
}

// Large blocks are taken one at a time from the large-block allocator. Small ones are drained off the free list
// first, then carved off the block top, with addSpace taking a fresh block once the top reaches the end; every
// element handed out shrinks m_numFreeElements and grows m_totalBytesInFreeLists by one element size. The in-use
// figure is refreshed against the peak and the memory soft limit, which raises the out-of-memory state.
void hkFreeListAllocator::blockAllocBatch(void** blocksOut, int32_t nblocks, int32_t nbytes) {
    EnterCriticalSection(&m_criticalSection.m_section);
    if (nbytes > MAX_FREELIST_SIZE) {
        for (int32_t i = 0; i < nblocks; ++i) {
            blocksOut[i] = (&m_largeAllocator)->alloc(nbytes);
        }
    }
    else {
        hkFreeList* pFreeList = m_sizeToFreeList[(nbytes + 15) >> 4];
        // Compares against the end address computed with address arithmetic, so that clang does not fold the first
        // compare to a count test.
        uintptr_t uiEnd = reinterpret_cast<uintptr_t>(blocksOut) + nblocks * sizeof(void*);
        uint32_t uiNumFreeElements = pFreeList->m_numFreeElements;
        hkFreeList::Element* pFree = pFreeList->m_free;
        void** p = blocksOut;
        for (; reinterpret_cast<uintptr_t>(p) < uiEnd && pFree; ++p) {
            *p = pFree;
            pFree = pFree->m_next;
        }
        pFreeList->m_free = pFree;
        pFreeList->m_numFreeElements -= static_cast<uint32_t>(p - blocksOut);
        while (reinterpret_cast<uintptr_t>(p) < uiEnd) {
            void** pStart = p;
            if (pFreeList->m_top >= pFreeList->m_blockEnd) {
                *p = pFreeList->addSpace();
                ++p;
            }
            else {
                do {
                    if (reinterpret_cast<uintptr_t>(p) >= uiEnd) {
                        break;
                    }
                    *p = pFreeList->m_top;
                    pFreeList->m_top += pFreeList->m_elementSize;
                    ++p;
                }
                while (pFreeList->m_top < pFreeList->m_blockEnd);
                pFreeList->m_numFreeElements -= static_cast<uint32_t>(p - pStart);
            }
        }
        m_totalBytesInFreeLists += pFreeList->m_elementSize * (pFreeList->m_numFreeElements - uiNumFreeElements);
    }
    uint32_t uiInUse = m_largeAllocator.m_sumAllocatedSize - m_totalBytesInFreeLists;
    uint32_t uiPeakInUse = m_peakInUse;
    if (uiInUse > uiPeakInUse) {
        uiPeakInUse = uiInUse;
    }
    bool bOverSoftLimit = !(uiInUse < m_softLimit);
    m_peakInUse = uiPeakInUse;
    if (bOverSoftLimit) {
        hkSetOutOfMemoryState(hkMemoryAllocator::MEMORY_STATE_OUT_OF_MEMORY);
    }
    LeaveCriticalSection(&m_criticalSection.m_section);
}

void hkFreeListAllocator::blockFreeBatch(void** blocks, int32_t nblocks, int32_t nbytes) {
    hkCriticalSection* lpCriticalSection = &m_criticalSection;
    EnterCriticalSection(&m_criticalSection.m_section);
    if (nbytes > 640) {
        for (int32_t i = 0; i < nblocks; ++i) {
            if (blocks[i]) {
                // ourFree is called through the vtable: a pointer keeps clang from calling it directly.
                (&m_largeAllocator)->ourFree(blocks[i]);
            }
        }
        LeaveCriticalSection(&lpCriticalSection->m_section);
    }
    else {
        void** v5 = blocks;
        hkFreeList* v6 = m_sizeToFreeList[(nbytes + 15) >> 4];
        uint32_t m_numFreeElements = v6->m_numFreeElements;
        hkFreeList::Element* m_free = v6->m_free;
        v6->m_numFreeElements = m_numFreeElements + nblocks;
        for (int32_t j = nblocks - 1; j >= 0; --j) {
            hkFreeList::Element* v10 = static_cast<hkFreeList::Element*>(*v5++);
            if (v10) {
                v10->m_next = m_free;
                m_free = v10;
            }
            else {
                --v6->m_numFreeElements;
            }
        }
        int32_t v11 = v6->m_elementSize * (v6->m_numFreeElements - m_numFreeElements);
        v6->m_free = m_free;
        m_totalBytesInFreeLists += v11;
        LeaveCriticalSection(&lpCriticalSection->m_section);
    }
}

bool hkFreeListAllocator::canAllocTotal(int32_t a2) {
    uint32_t m_peakInUse;
    uint32_t v2 = m_largeAllocator.m_sumAllocatedSize - m_totalBytesInFreeLists;
    m_peakInUse = this->m_peakInUse;
    if (v2 > m_peakInUse) {
        m_peakInUse = m_largeAllocator.m_sumAllocatedSize - m_totalBytesInFreeLists;
    }
    this->m_peakInUse = m_peakInUse;
    return (bool)((uint8_t)(v2 < (m_softLimit - a2)));
}

// The large allocator's figures first, then every free list's: a list adds its available bytes and its in-use ones,
// the latter its in-use figure less its allocated one, and raises the largest block to its element size while it
// still has an element to hand out. The peak is copied and the total available comes from the block server, or -1
// when that reports no limit.
void hkFreeListAllocator::getMemoryStatistics(hkMemoryAllocator::MemoryStatistics& stats) {
    EnterCriticalSection(&m_criticalSection.m_section);
    m_largeAllocator.calculateStatistics(stats);
    uint32_t uiAvailable = 0;
    uint32_t uiInUse = 0;
    uint32_t uiAllocated = 0;
    for (int32_t i = 0; i < m_numFreeLists; ++i) {
        hkFreeList* pFreeList = m_freeLists[i];
        hkMemoryAllocator::MemoryStatistics listStats = {};
        pFreeList->calculateStatistics(listStats);
        if ((pFreeList->m_free || pFreeList->m_top < pFreeList->m_blockEnd) &&
            (pFreeList->m_elementSize > stats.m_largestBlock)) {
            stats.m_largestBlock = pFreeList->m_elementSize;
        }

        uiAvailable += listStats.m_available;
        uiInUse += listStats.m_inUse;
        uiAllocated += listStats.m_allocated;
    }
    stats.m_available += uiAvailable;
    stats.m_inUse += uiInUse - uiAllocated;
    stats.m_peakInUse = m_peakInUse;
    uint32_t uiTotalAvailable = m_server->getTotalAvailableMemory();
    if (uiTotalAvailable == 0xFFFFFFFF) {
        stats.m_totalAvailable = uiTotalAvailable;
    }
    else {
        stats.m_totalAvailable = stats.m_available + uiTotalAvailable;
    }
    LeaveCriticalSection(&m_criticalSection.m_section);
}

int32_t hkFreeListAllocator::getAllocatedSize(const void* p, int32_t nbytes) {
    int32_t result = nbytes;
    if (nbytes > 640) {
        return (&m_largeAllocator)->getAllocatedSize(p, nbytes);
    }
    if (!nbytes) {
        return result;
    }
    hkFreeList* v4 = m_sizeToFreeList[(nbytes + 15) >> 4];
    if (v4) {
        return (int32_t)v4->m_elementSize;
    }

    return (&m_largeAllocator)->getAllocatedSize(p, nbytes);
}

void hkFreeListAllocator::resetPeakMemoryStatistics() {
    hkMemoryAllocator::MemoryStatistics v2;
    std::memset(&v2, 0, sizeof(v2));
    getMemoryStatistics(v2);
    m_peakInUse = v2.m_inUse;
}

// Deletes every free list once and clears the later pointers that duplicate it, then quits the large block
// allocator.
void hkFreeListAllocator::quit() {
    for (int32_t i = 0; i < m_numFreeLists; ++i) {
        hkFreeList* pFreeList = m_freeLists[i];
        if (pFreeList) {
            _deleteFreeList(pFreeList);
            for (int32_t j = i + 1; j < m_numFreeLists; ++j) {
                if (m_freeLists[j] == pFreeList) {
                    m_freeLists[j] = nullptr;
                }
            }
        }
    }
    m_largeAllocator.quit();
}

void hkFreeListAllocator::printStatistics(hkOstream* a1) {
    a1->printf("printStatistics is not supported on hkFreeListAllocator - use hkDebugMemory or hkPoolMemory\n");
}

void hkFreeListAllocator::reservePagesFromSystemMemory(int32_t a1) {
    ;
}

void hkFreeListAllocator::garbageCollect() {
    char v5;
    hkCriticalSection* lpCriticalSection = &m_criticalSection;
    EnterCriticalSection(&m_criticalSection.m_section);
    char v6 = 0;
    do {
        int32_t v2 = 0;
        v5 = 0;
        if (m_numFreeLists <= 0) {
            break;
        }
        hkFreeList** m_freeLists = this->m_freeLists;
        do {
            hkFreeList* v4 = *m_freeLists;
            uint32_t m_numFreeElements = (*m_freeLists)->m_numFreeElements;
            if ((*m_freeLists)->findGarbage() < 0) {
                v6 = 1;
            }
            if (v4->m_freeBlocks) {
                v5 = 1;
            }
            v4->freeAllFreeBlocks();
            ++v2;
            m_totalBytesInFreeLists += v4->m_elementSize * (v4->m_numFreeElements - m_numFreeElements);
            ++m_freeLists;
        }
        while (v2 < m_numFreeLists);
    }
    while (v5 && v6);
    m_largeAllocator.garbageCollect();
    LeaveCriticalSection(&lpCriticalSection->m_section);
}

// Walks the free lists from the list the last call stopped at, releasing the blocks that have become entirely free
// and counting the elements that went with them; it stops once the requested number of blocks has been checked or a
// whole turn has been made. A list that reported its walk finished moves the index to the next one.
void hkFreeListAllocator::incrementalGarbageCollect(int32_t numBlocks) {
    EnterCriticalSection(&m_criticalSection.m_section);
    int32_t iNumBlocks = numBlocks;
    if (iNumBlocks < 1) {
        iNumBlocks = 1;
    }
    int32_t iIndex = m_incrementalFreeListIndex;
    const int32_t iStartIndex = iIndex;
    do {
        hkFreeList* pFreeList = m_freeLists[iIndex];
        const uint32_t uiNumFreeElementsBefore = pFreeList->m_numFreeElements;
        hkBool kFound;
        int32_t iNumBlocksOut;
        if ((pFreeList->incrementalFindGarbage(&kFound, iNumBlocks, iNumBlocksOut)->m_bool) &&
            (++iIndex >= m_numFreeLists)) {
            iIndex = 0;
        }

        pFreeList->_freeFreeBlocks();
        iNumBlocks -= iNumBlocksOut;
        m_totalBytesInFreeLists += (pFreeList->m_numFreeElements - uiNumFreeElementsBefore) * pFreeList->m_elementSize;
    }
    while (iNumBlocks > 0 && iIndex != iStartIndex);
    m_incrementalFreeListIndex = iIndex;
    LeaveCriticalSection(&m_criticalSection.m_section);
}

void hkFreeListAllocator::optimize() {
    hkCriticalSection* p_m_criticalSection = &m_criticalSection;
    hkCriticalSection* v7 = &m_criticalSection;
    EnterCriticalSection(&m_criticalSection.m_section);
    int32_t v3 = 0;
    if (m_numFreeLists > 0) {
        hkFreeList** m_freeLists = this->m_freeLists;
        do {
            hkFreeList* v5 = *m_freeLists;
            uint32_t m_numFreeElements = (*m_freeLists)->m_numFreeElements;
            if ((*m_freeLists)->findGarbage() > 0) {
                v5->freeSmallFreeBlocks(v5->m_blockSize - v5->m_blockSize / 10);
            }
            ++v3;
            m_totalBytesInFreeLists += v5->m_elementSize * (v5->m_numFreeElements - m_numFreeElements);
            ++m_freeLists;
        }
        while (v3 < m_numFreeLists);
        p_m_criticalSection = v7;
    }
    LeaveCriticalSection(&p_m_criticalSection->m_section);
}

void hkFreeListAllocator::setMemorySoftLimit(uint32_t a1) {
    hkCriticalSection* p_m_criticalSection = &m_criticalSection;
    EnterCriticalSection(&m_criticalSection.m_section);
    m_softLimit = a1;
    LeaveCriticalSection(&p_m_criticalSection->m_section);
}

uint32_t hkFreeListAllocator::getMemorySoftLimit() {
    return m_softLimit;
}

void hkFreeListAllocator::releaseUnusedPagesToSystemMemory() {
    garbageCollect();
}

// True when every free list's free-block check passes; the first failing list ends the walk. The result is set before
// the walk and cleared on a failure.
hkBool hkFreeListAllocator::isOk() const {
    hkBool result;
    hkBool bListOk;
    EnterCriticalSection(&m_criticalSection.m_section);
    result.m_bool = 1;
    for (int32_t i = 0; i < m_numFreeLists; ++i) {
        m_freeLists[i]->_checkFreeBlocks(&bListOk);
        if (!bListOk.m_bool) {
            result.m_bool = 0;
            break;
        }
    }
    LeaveCriticalSection(&m_criticalSection.m_section);
    return result;
}

// Gathers every block of every free list into one array from the block server (failure when it has no memory), sorts it
// by address and walks the large-block allocator's blocks: a free list's block is skipped (the free lists report their
// own elements afterwards, pool 1 upwards), any other goes to the callback with its size and in-use flag in pool 0.
hkResult hkFreeListAllocator::walkMemory(hkFreeListAllocator::MemoryWalkCallback callback, void* param) {
    int32_t iTotalBlocks = 0;
    for (int32_t i = 0; i < m_numFreeLists; ++i) {
        // EXE-INLINED none: hkFreeList::getTotalNumBlocks
        int32_t iNumFree = 0;
        for (hkFreeList::Block* pBlock = m_freeLists[i]->m_freeBlocks; pBlock; pBlock = pBlock->m_next) {
            ++iNumFree;
        }
        int32_t iNumUsed = 0;
        for (hkFreeList::Block* pBlock = m_freeLists[i]->m_blocks; pBlock; pBlock = pBlock->m_next) {
            ++iNumUsed;
        }
        iTotalBlocks += iNumFree + iNumUsed;
    }
    uint32_t uiSize = iTotalBlocks * sizeof(void*);
    void** ppBlocks = static_cast<void**>(m_server->criticalAlloc(uiSize));
    if (!ppBlocks) {
        return HK_FAILURE;
    }
    void** ppDest = ppBlocks;
    for (int32_t i = 0; i < m_numFreeLists; ++i) {
        hkFreeList* pFreeList = m_freeLists[i];
        // EXE-INLINED none: hkFreeList::getTotalNumBlocks
        int32_t iNumFree = 0;
        for (hkFreeList::Block* pBlock = pFreeList->m_freeBlocks; pBlock; pBlock = pBlock->m_next) {
            ++iNumFree;
        }
        int32_t iNumUsed = 0;
        for (hkFreeList::Block* pBlock = pFreeList->m_blocks; pBlock; pBlock = pBlock->m_next) {
            ++iNumUsed;
        }
        int32_t iNumBlocks = iNumFree + iNumUsed;
        pFreeList->getBlocks(ppDest, iNumBlocks);
        ppDest += iNumBlocks;
    }
    if (iTotalBlocks > 1) {
        hkAlgorithm::quickSortRecursive<void*, hkBool* (*)(hkBool*, const void*, const void*)>(
            ppBlocks, 0, iTotalBlocks - 1, &hkFreeListAllocator::_comparePointers
        );
    }
    void** ppNextBlock = ppBlocks;
    hkLargeBlockAllocator::Iterator iter;
    m_largeAllocator.getIterator(&iter);
    while (iter.m_chunk) {
        void* pPayload = iter.m_chunk->getPayload();
        if (pPayload == *ppNextBlock) {
            ++ppNextBlock;
        }
        else {
            callback(pPayload, iter.m_chunk->getChunkSize(), hkBool(iter.m_chunk->isInuse()), 0, param);
        }
        hkBool bMore;
        m_largeAllocator.nextBlock(&bMore, iter);
    }
    m_server->criticalFree(ppBlocks, uiSize);
    for (int32_t i = 0; i < m_numFreeLists; ++i) {
        m_freeLists[i]->walkMemory(callback, i + 1, param);
    }
    return HK_SUCCESS;
}

void hkFreeListAllocator::init(hkMemoryBlockServer* a1) {
    m_server = a1;
    m_largeAllocator.m_server = a1;
}

// Takes the next list out of m_freeListMemory and initialises it, then files it in m_freeLists. When a list for the
// same element size is already filed, that one is kept and the new one dropped. Traps if the pool of lists has run out.
hkFreeList* hkFreeListAllocator::_newFreeList(uint32_t elementSize, uint32_t alignment, uint32_t blockSize) {
    if (m_topFreeList >= m_lastFreeList) {
        __builtin_debugtrap();
    }
    hkFreeList* pFreeList = m_topFreeList;
    m_topFreeList = pFreeList + 1;
    if (pFreeList) {
        ::new (pFreeList) hkFreeList(elementSize, alignment, blockSize, &m_largeAllocator);
    }
    int32_t iNumFreeLists = m_numFreeLists;
    if (iNumFreeLists > 0) {
        uint32_t uiElementSize = pFreeList->m_elementSize;
        for (int32_t i = 0; i < iNumFreeLists; ++i) {
            if (m_freeLists[i]->m_elementSize == uiElementSize) {
                _deleteFreeList(pFreeList);
                return m_freeLists[i];
            }
        }
    }
    m_freeLists[iNumFreeLists] = pFreeList;
    ++m_numFreeLists;
    return pFreeList;
}

void hkFreeListAllocator::_deleteFreeList(hkFreeList* freeList) {
    freeList->Free();
    hkFreeList* m_topFreeList = this->m_topFreeList;
    if (&freeList[1] == m_topFreeList) {
        this->m_topFreeList = m_topFreeList - 1;
    }
}

// The two addresses are compared as unsigned integers; the pointees are never read.
hkBool* hkFreeListAllocator::_comparePointers(hkBool* arResult, const void* a, const void* b) {
    arResult->m_bool = reinterpret_cast<uintptr_t>(a) < reinterpret_cast<uintptr_t>(b);
    return arResult;
}
