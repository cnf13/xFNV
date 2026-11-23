#include "Havok/Source/Common/Base/Memory/Allocator/FreeList/hkFreeList.h"

#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalBuffer.h"
#include "Havok/Source/Common/Base/Memory/Allocator/FreeList/hkFreeListAllocator.h"
#include "Havok/Source/Common/Base/Memory/Allocator/FreeList/hkFreeListAllocatorServer.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "xfnv/Compiler.h"

// Every field starts empty except m_lastIncrementalBlock, which the engine leaves as the caller had it.
hkFreeList::hkFreeList() {
    m_free = nullptr;
    m_blocks = nullptr;
    m_freeBlocks = nullptr;
    m_blockSize = 0;
    m_align = 0;
    m_top = nullptr;
    m_blockEnd = nullptr;
    m_numFreeElements = 0;
    m_totalNumElements = 0;
    m_memoryServer = nullptr;
    m_elementSize = 0;
    m_maxBlockSize = 0;
}

hkFreeList::hkFreeList(
    uint32_t aiElementSize,
    uint32_t aiAlignment,
    uint32_t aiBlockSize,
    hkFreeListAllocatorServer* apMemoryServer
) {
    _init(aiElementSize, aiAlignment, aiBlockSize, apMemoryServer);
}

// Both block lists go back to the memory server, and the list forgets everything it handed out.
void hkFreeList::Free() {
    _freeBlocks(m_blocks);
    Block* pFreeBlocks = m_freeBlocks;
    m_blocks = nullptr;
    _freeBlocks(pFreeBlocks);
    m_freeBlocks = nullptr;
    m_free = nullptr;
    m_blockSize = 0;
    m_numFreeElements = 0;
    m_totalNumElements = 0;
    m_top = nullptr;
    m_blockEnd = nullptr;
    m_lastIncrementalBlock = nullptr;
}

// m_lastIncrementalBlock is the block before the next one to check, or null to start from m_blocks.
hkBool hkFreeList::incrementalFindGarbage(int32_t aiNumBlocks, int32_t& aiNumBlocksOut) {
    int32_t i = 0;
    for (; i < aiNumBlocks; ++i) {
        Block* pBlock;
        if (m_lastIncrementalBlock) {
            pBlock = m_lastIncrementalBlock->m_next;
            if (!pBlock) {
                aiNumBlocksOut = i;
                m_lastIncrementalBlock = nullptr;
                return 1;
            }
            if (_calcBlockFree(pBlock)) {
                m_lastIncrementalBlock->m_next = pBlock->m_next;
                pBlock->m_next = m_freeBlocks;
                m_freeBlocks = pBlock;
                continue;
            }
        }
        else {
            pBlock = m_blocks;
            if (!pBlock) {
                aiNumBlocksOut = i;
                m_lastIncrementalBlock = nullptr;
                return 1;
            }
            if (_calcBlockFree(pBlock)) {
                m_blocks = pBlock->m_next;
                pBlock->m_next = m_freeBlocks;
                m_freeBlocks = pBlock;
                continue;
            }
        }
        m_lastIncrementalBlock = pBlock;
    }
    aiNumBlocksOut = i;
    return m_lastIncrementalBlock == nullptr;
}

// Takes apBlock's elements off the free list into a stack buffer. When they are all free (counting the unused space
// from m_top, which is then dropped), they stay off and the result is 1; otherwise they go back on the front of the
// list, sorted by address, and the result is 0.
int32_t hkFreeList::_calcBlockFree(hkFreeList::Block* block) {
    hkLocalBuffer<void*> kElements(static_cast<int32_t>(block->m_numElements), nullptr);

    uint8_t* pStart = block->m_start;
    uint8_t* pEnd = pStart + m_elementSize * block->m_numElements;
    int32_t numFound = 0;
    // The link to rewrite when an element is taken off: m_free, then the m_next of the last element left on.
    Element** ppPrev = &m_free;
    for (Element* pCur = m_free; pCur; pCur = pCur->m_next) {
        // The element's address against the block's byte range, compared unsigned.
        uint8_t* pAddress = static_cast<uint8_t*>(static_cast<void*>(pCur));
        if (pAddress >= pStart && pAddress < pEnd) {
            kElements[numFound] = pCur;
            *ppPrev = pCur->m_next;
            ++numFound;
        }
        else {
            ppPrev = &pCur->m_next;
        }
    }

    uint8_t* pTop = m_top;
    bool bAllFree;
    if (pTop >= pStart && pTop < pEnd) {
        bAllFree = numFound + static_cast<uint32_t>(m_blockEnd - pTop) / m_elementSize == block->m_numElements;
        if (bAllFree) {
            m_top = nullptr;
            m_blockEnd = nullptr;
        }
    }
    else {
        bAllFree = static_cast<uint32_t>(numFound) == block->m_numElements;
    }
    if (bAllFree) {
        return 1;
    }

    if (numFound > 0) {
        if (numFound > 1) {
            hkAlgorithm::quickSortRecursive<void*, hkBool (*)(const void*, const void*)>(
                kElements.begin(),
                0,
                numFound - 1,
                &hkFreeListAllocator::_comparePointers
            );
        }
        Element* pPrev = static_cast<Element*>(kElements[0]);
        for (int32_t i = 1; i < numFound; ++i) {
            Element* pNext = static_cast<Element*>(kElements[i]);
            pPrev->m_next = pNext;
            pPrev = pNext;
        }
        static_cast<Element*>(kElements[numFound - 1])->m_next = m_free;
        m_free = static_cast<Element*>(kElements[0]);
    }
    return 0;
}

void hkFreeList::calculateStatistics(hkMemoryAllocator::MemoryStatistics& stats) {
    uint32_t v3 = _calculateBlockAllocatedSize(m_freeBlocks);
    stats.m_allocated = _calculateBlockAllocatedSize(m_blocks) + v3;
    uint32_t m_numFreeElements = this->m_numFreeElements;
    uint32_t v5 = m_numFreeElements * m_elementSize;
    uint32_t v6 = m_totalNumElements - m_numFreeElements;
    stats.m_available = v5;
    stats.m_inUse = m_elementSize * v6;
    stats.m_largestBlock = m_elementSize;
    stats.m_totalAvailable = v5;
}

// Takes the first free block when there is one; else asks the memory server for a new block: the first of 256 bytes'
// worth of elements (at least one), each later one half as large again (at least 8 elements) up to m_maxBlockSize.
// The block's header sits at its start and its elements from the next multiple of m_align after the header. Returns
// the block's first element, null when the server has no memory.
void* hkFreeList::addSpace() {
    Block* pBlock = m_freeBlocks;
    if (pBlock) {
        m_freeBlocks = pBlock->m_next;
        _addBlockElements(pBlock);
        pBlock->m_next = m_blocks;
        m_numFreeElements--;
        m_blocks = pBlock;
        void* pElement = m_top;
        m_top += m_elementSize;
        return pElement;
    }
    if (!m_blockSize) {
        uint32_t uiNumElements = 256 / m_elementSize;
        if (uiNumElements < 1) {
            uiNumElements = 1;
        }
        m_blockSize = bestBlockSize(uiNumElements * m_elementSize, m_align);
    }
    else if (m_blockSize < m_maxBlockSize) {
        uint32_t uiNumElements = m_blockSize / m_elementSize;
        if (uiNumElements < 8) {
            uiNumElements = 8;
        }
        else {
            uiNumElements += uiNumElements >> 1;
        }
        m_blockSize = bestBlockSize(uiNumElements * m_elementSize, m_align);
    }
    Block* pNew = static_cast<Block*>(m_memoryServer->alloc(m_blockSize));
    if (!pNew) {
        return nullptr;
    }
    uint32_t uiAllocatedSize = m_memoryServer->getAllocatedSize(pNew, m_blockSize);
    pNew->m_blockSize = uiAllocatedSize;
    // The block is raw memory from the server: the elements start at the first multiple of m_align past the header,
    // and take the rest of the allocation.
    uint8_t* pStart = reinterpret_cast<uint8_t*>(
        (reinterpret_cast<uintptr_t>(pNew + 1) + m_align - 1) & ~static_cast<uintptr_t>(m_align - 1)
    );
    pNew->m_start = pStart;
    pNew->m_numElements = static_cast<uint32_t>(reinterpret_cast<uint8_t*>(pNew) + uiAllocatedSize - pStart) /
        m_elementSize;
    _addBlockElements(pNew);
    pNew->m_next = m_blocks;
    m_blocks = pNew;
    m_totalNumElements += pNew->m_numElements;
    m_numFreeElements += pNew->m_numElements - 1;
    void* pElement = m_top;
    m_top += m_elementSize;
    return pElement;
}

void hkFreeList::_init(
    uint32_t elementSize,
    uint32_t align,
    uint32_t blockSize,
    hkFreeListAllocatorServer* memoryServer
) {
    m_free = nullptr;
    m_blocks = nullptr;
    m_freeBlocks = nullptr;
    m_blockSize = blockSize;
    m_top = nullptr;
    m_blockEnd = nullptr;
    m_numFreeElements = 0;
    m_totalNumElements = 0;
    m_lastIncrementalBlock = nullptr;
    m_align = align;
    m_memoryServer = memoryServer;
    // The element size is compared with the alignment unsigned, and the larger is rounded up to the alignment.
    uint32_t uiElementSize;
    if (elementSize >= align) {
        uiElementSize = (elementSize + align - 1) & ~(align - 1);
    }
    else {
        uiElementSize = align;
    }
    m_elementSize = uiElementSize;
    m_maxBlockSize = bestBlockSize(0xFF0u, align);
}

// The elements of both block lists, the free blocks' first.
uint32_t hkFreeList::_calcTotalNumElements() const {
    return _calcTotalNumElements(m_freeBlocks) + _calcTotalNumElements(m_blocks);
}

void hkFreeList::freeAll() {
    m_numFreeElements = m_totalNumElements;
    hkFreeList::Block* result = m_blocks;
    if (result) {
        for (m_free = nullptr; result->m_next; result = result->m_next) {
            ;
        }
        result->m_next = m_freeBlocks;
        result = m_blocks;
        m_freeBlocks = result;
        m_blocks = nullptr;
        m_top = nullptr;
        m_blockEnd = nullptr;
        m_lastIncrementalBlock = nullptr;
    }
}

int32_t hkFreeList::freeAllFreeBlocks() {
    int32_t iFreed = _freeBlocks(m_freeBlocks);
    m_freeBlocks = nullptr;
    return iFreed;
}

int32_t hkFreeList::_freeBlocks(hkFreeList::Block* cur) {
    hkFreeList::Block* cur_1 = cur;
    int32_t v3 = 0;
    if (cur) {
        hkFreeList::Block* m_next;
        do {
            m_totalNumElements -= cur_1->m_numElements;
            m_numFreeElements -= cur_1->m_numElements;
            m_next = cur_1->m_next;
            ++v3;
            m_memoryServer->ourFree(cur_1);
            cur_1 = m_next;
        }
        while (m_next);
    }
    return v3;
}

// The SDK's consistency check: every free element is aligned and sits on an element boundary of a used block the
// memory server holds as valid. The blocks are searched in list order, each tested with isValidAlloc first.
hkBool hkFreeList::_checkFreeBlocks() {
    for (Element* pElement = m_free; pElement; pElement = pElement->m_next) {
        if (reinterpret_cast<uintptr_t>(pElement) & (m_align - 1)) {
            return 0;
        }
        // A free element is untyped block memory with its link overlaid: compared as the block's bytes.
        uint8_t* pElementBytes = reinterpret_cast<uint8_t*>(pElement);
        for (Block* pBlock = m_blocks;; pBlock = pBlock->m_next) {
            if (!pBlock || !m_memoryServer->isValidAlloc(pBlock).m_bool) {
                return 0;
            }
            uint8_t* pStart = pBlock->m_start;
            if (pElementBytes >= pStart && pElementBytes < pStart + m_elementSize * pBlock->m_numElements) {
                // The division is unsigned.
                if (static_cast<uint32_t>(pElementBytes - pStart) % m_elementSize) {
                    return 0;
                }
                break;
            }
        }
    }
    return 1;
}

uint32_t hkFreeList::_calcNumFreeElements() {
    int32_t i;
    hkFreeList::Element* m_free = this->m_free;
    for (i = 0; m_free; ++i) {
        m_free = m_free->m_next;
    }
    uint32_t v3 = (m_blockEnd - m_top) / m_elementSize + i;
    for (hkFreeList::Block* j = m_freeBlocks; j; j = j->m_next) {
        v3 += j->m_numElements;
    }
    return v3;
}

uint32_t hkFreeList::_calculateBlockAllocatedSize(hkFreeList::Block* cur) {
    uint32_t i;
    uint32_t v5;
    hkFreeList::Block* v2 = cur;
    for (i = 0; v2; i += v5) {
        v5 = m_memoryServer->getAllocatedSize(v2, v2->m_blockSize);
        v2 = v2->m_next;
    }
    return i;
}

void hkFreeList::getBlocks(void** blocks, int32_t numBlocks) {
    for (hkFreeList::Block* i = m_blocks; i; ++blocks) {
        *blocks = i;
        i = i->m_next;
    }
    for (hkFreeList::Block* j = m_freeBlocks; j; ++blocks) {
        *blocks = j;
        j = j->m_next;
    }
}

void hkFreeList::init(
    uint32_t elementSize,
    uint32_t align,
    uint32_t blockSize,
    hkFreeListAllocatorServer* memoryServer
) {
    Free();
    _init(elementSize, align, blockSize, memoryServer);
}

// Hands the callback every element of the blocks from block on, each as not allocated.
void hkFreeList::_walkMemoryBlockList(
    Block* block,
    void (*callback)(void*, uint32_t, hkBool, int32_t, void*),
    int32_t pool,
    void* param
) {
    for (; block; block = block->m_next) {
        // The elements are raw memory walked by address: the element's address is compared with the block's end as an
        // unsigned integer, wrap included, because a pointer compare lets clang test only a nonzero byte count.
        const uintptr_t uiEnd = reinterpret_cast<uintptr_t>(block->m_start) + m_elementSize * block->m_numElements;
        for (uintptr_t uiElement = reinterpret_cast<uintptr_t>(block->m_start); uiElement < uiEnd;
             uiElement += m_elementSize) {
            callback(reinterpret_cast<void*>(uiElement), m_elementSize, hkBool(false), pool, param);
        }
    }
}

// Walks the free blocks (all free), then every element of the used blocks in address order, telling the callback
// whether each is allocated: an element is free when it is on the free list or in the unused space from m_top on. Both
// arrays are sorted by address with the pointer less-than. The cursor into the free elements is tested against its end
// only once per block, so in the block holding the last free element the rest of that block's elements are compared
// with the word past the array's end.
void hkFreeList::walkMemory(void (*callback)(void*, uint32_t, hkBool, int32_t, void*), int32_t pool, void* param) {
    _walkMemoryBlockList(m_freeBlocks, callback, pool, param);
    if (!m_blocks && !m_free) {
        return;
    }
    int32_t numBlocks = 0;
    for (Block* pBlock = m_blocks; pBlock; pBlock = pBlock->m_next) {
        ++numBlocks;
    }
    const uint32_t uiBlocksBytes = sizeof(void*) * numBlocks;
    void** ppBlocks = static_cast<void**>(m_memoryServer->criticalAlloc(uiBlocksBytes));
    if (!ppBlocks) {
        return;
    }
    void** ppBlockOut = ppBlocks;
    for (Block* pBlock = m_blocks; pBlock; pBlock = pBlock->m_next) {
        *ppBlockOut++ = pBlock;
    }
    if (numBlocks > 1) {
        hkAlgorithm::quickSortRecursive<void*, hkBool (*)(const void*, const void*)>(
            ppBlocks,
            0,
            numBlocks - 1,
            &hkFreeListAllocator::_comparePointers
        );
    }

    int32_t numFree = static_cast<int32_t>(static_cast<uint32_t>(m_blockEnd - m_top) / m_elementSize);
    for (Element* pElement = m_free; pElement; pElement = pElement->m_next) {
        ++numFree;
    }
    const uint32_t uiFreeBytes = sizeof(void*) * numFree;
    void** ppFree = static_cast<void**>(m_memoryServer->criticalAlloc(uiFreeBytes));
    if (!ppFree) {
        m_memoryServer->ourFree(ppBlocks);
        return;
    }
    int32_t iFree = 0;
    for (Element* pElement = m_free; pElement; pElement = pElement->m_next) {
        ppFree[iFree++] = pElement;
    }
    for (uint8_t* pTop = m_top; pTop < m_blockEnd; pTop += m_elementSize) {
        ppFree[iFree++] = pTop;
    }
    if (numFree > 1) {
        hkAlgorithm::quickSortRecursive<void*, hkBool (*)(const void*, const void*)>(
            ppFree,
            0,
            numFree - 1,
            &hkFreeListAllocator::_comparePointers
        );
    }

    void** ppNextFree = ppFree;
    void** ppFreeEnd = ppFree + numFree;
    int32_t i = 0;
    // The blocks while free elements are left: each element is free when it is the next free one.
    for (; i < numBlocks; ++i) {
        if (ppNextFree >= ppFreeEnd) {
            break;
        }
        Block* pBlock = static_cast<Block*>(ppBlocks[i]);
        // The elements are raw memory walked by address: the element's address is compared with the block's end as an
        // unsigned integer, wrap included, because a pointer compare lets clang test only a nonzero byte count.
        const uintptr_t uiEnd = reinterpret_cast<uintptr_t>(pBlock->m_start) + m_elementSize * pBlock->m_numElements;
        for (uintptr_t uiElement = reinterpret_cast<uintptr_t>(pBlock->m_start); uiElement < uiEnd;
             uiElement += m_elementSize) {
            void* pElement = reinterpret_cast<void*>(uiElement);
            if (pElement == *ppNextFree) {
                callback(pElement, m_elementSize, hkBool(false), pool, param);
                ++ppNextFree;
            }
            else {
                callback(pElement, m_elementSize, hkBool(true), pool, param);
            }
        }
    }
    // The remaining blocks: every element allocated.
    for (; i < numBlocks; ++i) {
        Block* pBlock = static_cast<Block*>(ppBlocks[i]);
        uint8_t* pElement = pBlock->m_start;
        for (uint32_t j = 0; j < pBlock->m_numElements; ++j) {
            callback(pElement, m_elementSize, hkBool(true), pool, param);
            pElement += m_elementSize;
        }
    }
    m_memoryServer->criticalFree(ppFree, uiFreeBytes);
    m_memoryServer->criticalFree(ppBlocks, uiBlocksBytes);
}

void hkFreeList::freeSmallFreeBlocks(uint32_t minSize) {
    hkFreeList::Block* m_freeBlocks = this->m_freeBlocks;
    hkFreeList::Block** p_m_freeBlocks = &this->m_freeBlocks;
    while (m_freeBlocks) {
        if (m_freeBlocks->m_blockSize > minSize) {
            p_m_freeBlocks = &m_freeBlocks->m_next;
            m_freeBlocks = m_freeBlocks->m_next;
        }
        else {
            hkFreeList::Block* m_next = m_freeBlocks->m_next;
            *p_m_freeBlocks = m_freeBlocks->m_next;
            m_totalNumElements -= m_freeBlocks->m_numElements;
            hkFreeListAllocatorServer* m_memoryServer = this->m_memoryServer;
            m_numFreeElements -= m_freeBlocks->m_numElements;
            m_memoryServer->ourFree(m_freeBlocks);
            m_freeBlocks = m_next;
        }
    }
}

// Sorts the used blocks and the free elements (the free list's and those between m_top and m_blockEnd) by address,
// moves every block whose elements are all free onto m_freeBlocks, and rebuilds both lists in address order. Returns
// the number of blocks moved, -1 when the memory server has no memory for the two arrays. XFNV_NO_BUILTIN keeps clang
// from turning the inline zeroing loop into a call of the DLL's memset.
XFNV_NO_BUILTIN("memset") int32_t hkFreeList::findGarbage() {
    if (!m_blocks) {
        return 0;
    }
    int32_t iNumBlocks = 0;
    for (Block* pBlock = m_blocks; pBlock; pBlock = pBlock->m_next) {
        ++iNumBlocks;
    }
    uint32_t uiBlocksSize = iNumBlocks * static_cast<uint32_t>(sizeof(void*));
    void** ppBlocks = static_cast<void**>(m_memoryServer->criticalAlloc(uiBlocksSize));
    if (!ppBlocks) {
        return -1;
    }
    void** ppBlockOut = ppBlocks;
    for (Block* pBlock = m_blocks; pBlock; pBlock = pBlock->m_next) {
        *ppBlockOut++ = pBlock;
    }
    if (iNumBlocks > 1) {
        hkAlgorithm::quickSortRecursive<void*, hkBool (*)(const void*, const void*)>(
            ppBlocks,
            0,
            iNumBlocks - 1,
            &hkFreeListAllocator::_comparePointers
        );
    }

    // The division is unsigned.
    int32_t iNumElements = static_cast<uint32_t>(m_blockEnd - m_top) / m_elementSize;
    for (Element* pElement = m_free; pElement; pElement = pElement->m_next) {
        ++iNumElements;
    }
    uint32_t uiElementsSize = iNumElements * static_cast<uint32_t>(sizeof(void*));
    void** ppElements = static_cast<void**>(m_memoryServer->criticalAlloc(uiElementsSize));
    if (!ppElements) {
        m_memoryServer->criticalFree(ppBlocks, uiBlocksSize);
        return -1;
    }
    int32_t iElement = 0;
    for (Element* pElement = m_free; pElement; pElement = pElement->m_next) {
        ppElements[iElement++] = pElement;
    }
    for (uint8_t* pTop = m_top; pTop < m_blockEnd; pTop += m_elementSize) {
        ppElements[iElement++] = pTop;
    }
    m_blockEnd = nullptr;
    m_top = nullptr;
    if (iNumElements > 1) {
        hkAlgorithm::quickSortRecursive<void*, hkBool (*)(const void*, const void*)>(
            ppElements,
            0,
            iNumElements - 1,
            &hkFreeListAllocator::_comparePointers
        );
    }

    // Both arrays are sorted: walk the blocks and the elements together. A block whose elements are a run of the free
    // elements, from its start to its end, is all free: it goes onto m_freeBlocks and its entries are cleared from
    // both arrays.
    void** ppCur = ppElements;
    void** ppEnd = ppElements + iNumElements;
    int32_t iNumFreed = 0;
    for (uint32_t i = 0; i < static_cast<uint32_t>(iNumBlocks); ++i) {
        if (ppCur >= ppEnd) {
            break;
        }
        Block* pBlock = static_cast<Block*>(ppBlocks[i]);
        uint32_t uiElementSize = m_elementSize;
        uint32_t uiNumElements = pBlock->m_numElements;
        uint8_t* pElement = pBlock->m_start;
        uint8_t* pBlockEnd = pElement + uiElementSize * uiNumElements;
        do {
            if (static_cast<uint8_t*>(*ppCur) != pElement || pElement >= pBlockEnd) {
                break;
            }
            ++ppCur;
            pElement += uiElementSize;
        }
        while (ppCur < ppEnd);
        if (pElement == pBlockEnd) {
            ++iNumFreed;
            pBlock->m_next = m_freeBlocks;
            m_freeBlocks = pBlock;
            ppBlocks[i] = nullptr;
            for (void** ppClear = ppCur - uiNumElements; ppClear < ppCur; ++ppClear) {
                *ppClear = nullptr;
            }
        }
        else {
            while (ppCur < ppEnd && static_cast<uint8_t*>(*ppCur) < pBlockEnd) {
                ++ppCur;
            }
        }
    }

    // The lists are rebuilt through the address of each last link.
    Block** ppBlockLink = &m_blocks;
    for (uint32_t i = 0; i < static_cast<uint32_t>(iNumBlocks); ++i) {
        Block* pBlock = static_cast<Block*>(ppBlocks[i]);
        if (pBlock) {
            *ppBlockLink = pBlock;
            ppBlockLink = &pBlock->m_next;
        }
    }
    *ppBlockLink = nullptr;
    Element** ppElementLink = &m_free;
    for (void** ppElement = ppElements; ppElement < ppEnd; ++ppElement) {
        Element* pFree = static_cast<Element*>(*ppElement);
        if (pFree) {
            *ppElementLink = pFree;
            ppElementLink = &pFree->m_next;
        }
    }
    *ppElementLink = nullptr;
    m_memoryServer->criticalFree(ppElements, uiElementsSize);
    m_memoryServer->criticalFree(ppBlocks, uiBlocksSize);
    m_lastIncrementalBlock = nullptr;
    return iNumFreed;
}

// The compare of align with 16 is unsigned.
uint32_t hkFreeList::bestBlockSize(uint32_t elementSpace, uint32_t align) {
    if (align > 16) {
        return elementSpace + align;
    }
    return elementSpace + 16;
}

uint32_t hkFreeList::_calcTotalNumElements(Block* apBlock) {
    uint32_t iNumElements = 0;
    for (; apBlock; apBlock = apBlock->m_next) {
        iNumElements += apBlock->m_numElements;
    }
    return iNumElements;
}
