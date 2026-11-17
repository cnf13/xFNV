#include "Havok/Source/Common/Base/Memory/Allocator/Solver/hkSolverAllocator.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Thread/CriticalSection/hkCriticalSection.h"

// No buffer yet; the free elements live in m_elemsBuf (32 of them, never freed).
hkSolverAllocator::hkSolverAllocator()
  : m_bufferStart(nullptr), m_bufferEnd(nullptr), m_currentEnd(nullptr), m_numAllocatedBlocks(0), m_peakUse(0),
    m_criticalSection(0) {
    m_freeElems.m_data = m_elemsBuf;
    m_freeElems.m_size = 0;
    m_freeElems.m_capacityAndFlags = hkArrayBase<Element>::DONT_DEALLOCATE_FLAG | 32;
}

// The critical section goes with its own destructor.
hkSolverAllocator::~hkSolverAllocator() {}

void* hkSolverAllocator::blockAlloc(int32_t numBytes) {
    return nullptr;
}

void hkSolverAllocator::blockFree(void* p, int32_t numBytes) {
    ;
}

void* hkSolverAllocator::bufAlloc(int32_t& reqNumInOut) {
    EnterCriticalSection(&m_criticalSection.m_section);
    int32_t iNumBytes = reqNumInOut;
    int32_t iBestIndex = -1;
    int32_t iBestSize = 0x7FFFFFF;
    // Fails while the free list could not take the elements freeing every block might leave behind.
    if (m_numAllocatedBlocks + 1 >= 2 * m_freeElems.getCapacity() - m_freeElems.m_size) {
        LeaveCriticalSection(&m_criticalSection.m_section);
        return nullptr;
    }
    // The best fit: the smallest free element that holds the request, the last one of equal size.
    Element* pElems = m_freeElems.m_data;
    for (int32_t i = m_freeElems.m_size - 1; i >= 0; --i) {
        int32_t iSize = pElems[i].m_size;
        if (iSize >= iNumBytes && iSize < iBestSize) {
            iBestSize = iSize;
            iBestIndex = i;
        }
    }
    if (iBestIndex >= 0) {
        char* pStart = pElems[iBestIndex].m_start;
        if (2 * iBestSize <= 3 * iNumBytes || iNumBytes <= 1024) {
            // Hands out the whole element.
            reqNumInOut = pElems[iBestIndex].m_size;
            m_freeElems.removeAtAndCopy(iBestIndex);
        }
        else {
            pElems[iBestIndex].m_size -= iNumBytes;
            pElems[iBestIndex].m_start = pStart + iNumBytes;
        }
        uint32_t uiInUse = m_currentEnd - m_bufferStart;
        ++m_numAllocatedBlocks;
        if (uiInUse > m_peakUse) {
            m_peakUse = uiInUse;
        }
        LeaveCriticalSection(&m_criticalSection.m_section);
        return pStart;
    }
    char* pCurrentEnd = m_currentEnd;
    if (m_bufferEnd - pCurrentEnd < iNumBytes) {
        LeaveCriticalSection(&m_criticalSection.m_section);
        return nullptr;
    }
    m_currentEnd = pCurrentEnd + iNumBytes;
    reqNumInOut = iNumBytes;
    uint32_t uiInUse = m_currentEnd - m_bufferStart;
    ++m_numAllocatedBlocks;
    if (uiInUse > m_peakUse) {
        m_peakUse = uiInUse;
    }
    LeaveCriticalSection(&m_criticalSection.m_section);
    return pCurrentEnd;
}

void hkSolverAllocator::bufFree(void* data, int32_t size) {
    EnterCriticalSection(&m_criticalSection.m_section);
    char* pEnd = static_cast<char*>(data) + size;
    --m_numAllocatedBlocks;
    if (pEnd == m_currentEnd) {
        // The block ends the used part: shrink it, then take back the free elements that ended at the block.
        m_currentEnd -= size;
        // Every element's end is compared with apMem, not with the shrunk end, as the game does.
        while (m_freeElems.m_size != 0 &&
               m_freeElems.m_data[m_freeElems.m_size - 1].m_start + m_freeElems.m_data[m_freeElems.m_size - 1].m_size ==
                   data) {
            m_currentEnd = m_freeElems.m_data[m_freeElems.m_size - 1].m_start;
            --m_freeElems.m_size;
        }
        LeaveCriticalSection(&m_criticalSection.m_section);
        return;
    }
    if (m_freeElems.m_size == 0) {
        Element* pElem = &m_freeElems.m_data[m_freeElems.m_size];
        m_freeElems.m_size = m_freeElems.m_size + 1;
        pElem->m_start = static_cast<char*>(data);
        pElem->m_size = size;
        LeaveCriticalSection(&m_criticalSection.m_section);
        return;
    }
    // The free list is sorted by address: find the last element below the block.
    int32_t iLast = m_freeElems.m_size - 1;
    int32_t i = iLast;
    if (i >= 0) {
        Element* pElems = m_freeElems.m_data;
        while (!(pElems[i].m_start < data)) {
            --i;
            if (i < 0) {
                break;
            }
        }
        if (i >= 0) {
            int32_t iSize = pElems[i].m_size;
            if (pElems[i].m_start + iSize == data) {
                // The block follows that element: grow it, and merge the next element when the block meets it.
                iSize += size;
                pElems[i].m_size = iSize;
                if (i < m_freeElems.m_size - 1 && pEnd == m_freeElems.m_data[i + 1].m_start) {
                    pElems[i].m_size = m_freeElems.m_data[i + 1].m_size + iSize;
                    m_freeElems.removeAtAndCopy(i + 1);
                }
                LeaveCriticalSection(&m_criticalSection.m_section);
                return;
            }
        }
    }
    if (i < iLast) {
        Element* pNext = &m_freeElems.m_data[i + 1];
        if (pNext->m_start == pEnd) {
            // The block precedes the next element: grow that one downwards.
            pNext->m_size += size;
            pNext->m_start = static_cast<char*>(data);
            LeaveCriticalSection(&m_criticalSection.m_section);
            return;
        }
    }
    Element kElem;
    kElem.m_start = static_cast<char*>(data);
    kElem.m_size = size;
    // No allocator is passed (a null reference): the inplace list never grows past its 32 elements, and bufAlloc
    // refuses blocks that could make it grow.
    m_freeElems._insertAt(*static_cast<hkMemoryAllocator*>(nullptr), i + 1, kElem);
    LeaveCriticalSection(&m_criticalSection.m_section);
}

bool hkSolverAllocator::canAllocSingleBlock(int32_t numBytes) {
    return numBytes <= m_bufferEnd - m_bufferStart;
}

void hkSolverAllocator::getMemoryStatistics(hkMemoryAllocator::MemoryStatistics& u) {
    u.m_allocated = m_bufferEnd - m_bufferStart;
    u.m_available = m_bufferEnd - m_currentEnd;
    u.m_inUse = m_currentEnd - m_bufferStart;
    u.m_peakInUse = m_peakUse;
}

void hkSolverAllocator::resetPeakMemoryStatistics() {
    m_peakUse = m_currentEnd - m_bufferStart;
}

void hkSolverAllocator::setBuffer(void* vbuffer, int32_t bufferSize) {
    hkCriticalSection* p_m_criticalSection = &m_criticalSection;
    EnterCriticalSection(&m_criticalSection.m_section);
    m_freeElems.m_data = m_elemsBuf;
    m_freeElems.m_capacityAndFlags = -0x7FFFFFE0;
    m_bufferStart = static_cast<char*>(vbuffer);
    m_currentEnd = static_cast<char*>(vbuffer);
    m_bufferEnd = static_cast<char*>(vbuffer) + bufferSize;
    m_numAllocatedBlocks = 0;
    LeaveCriticalSection(&p_m_criticalSection->m_section);
}
