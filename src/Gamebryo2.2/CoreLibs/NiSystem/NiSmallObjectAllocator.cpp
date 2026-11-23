#include "Gamebryo2.2/CoreLibs/NiSystem/NiSmallObjectAllocator.h"

#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

#include <cstdint>

// KEEP: the exe builds the new chunk's second dword on the stack from two byte stores (m_ucFirstAvailableBlock,
// m_ucBlocksAvailable) and copies the whole dword into m_pkChunks, so the chunk's two padding bytes get stack bytes
// nothing wrote (an uninitialised read); src cannot reproduce them.
void* NiFixedAllocator::Allocate() {
    m_kCriticalSection.Lock();
    if (m_pkAllocChunk && !m_pkAllocChunk->m_ucBlocksAvailable) {
        m_pkAllocChunk = nullptr;
    }
    if (m_pkEmptyChunk) {
        m_pkAllocChunk = m_pkEmptyChunk;
        m_pkEmptyChunk = nullptr;
    }
    if (!m_pkAllocChunk) {
        size_t stNumChunks = m_stNumChunks;
        for (size_t i = 0; i < stNumChunks; ++i) {
            if (m_pkChunks[i].m_ucBlocksAvailable) {
                m_pkAllocChunk = &m_pkChunks[i];
                break;
            }
        }
        if (!m_pkAllocChunk) {
            Reserve(stNumChunks + 1);
            NiFixedAllocator::Chunk kNewChunk;
            kNewChunk.Init(m_stBlockSize, m_ucNumBlocks);
            Push_Back(kNewChunk);
            m_pkAllocChunk = &m_pkChunks[m_stNumChunks - 1];
            m_pkDeallocChunk = m_pkChunks;
        }
    }
    void* pvResult = m_pkAllocChunk->Allocate(m_stBlockSize);
    m_kCriticalSection.Unlock();
    return pvResult;
}

NiFixedAllocator::Chunk* NiFixedAllocator::VicinityFind(void* p) {
    NiFixedAllocator::Chunk* m_pkDeallocChunk = this->m_pkDeallocChunk;
    NiFixedAllocator::Chunk* m_pkChunks = this->m_pkChunks;
    size_t v4 = m_stBlockSize * m_ucNumBlocks;
    NiFixedAllocator::Chunk* result = m_pkDeallocChunk + 1;
    NiFixedAllocator::Chunk* v6 = &m_pkChunks[m_stNumChunks];
    if (&m_pkDeallocChunk[1] == v6) {
        result = nullptr;
    }
    while (true) {
        if (m_pkDeallocChunk) {
            if (m_pkDeallocChunk->m_pucData <= p && p < &m_pkDeallocChunk->m_pucData[v4]) {
                return m_pkDeallocChunk;
            }
            if (m_pkDeallocChunk != m_pkChunks) {
                --m_pkDeallocChunk;
            }
            else {
                m_pkDeallocChunk = nullptr;
                if (!result) {
                    return result;
                }
            }
        }
        if (result) {
            if (result->m_pucData <= p && p < &result->m_pucData[v4]) {
                return result;
            }
            if (++result == v6) {
                result = nullptr;
                if (!m_pkDeallocChunk) {
                    return result;
                }
            }
        }
    }
}

// Gives block p back to m_pkDeallocChunk; when that chunk is wholly free and another free chunk is kept, the last
// chunk is released (swapped with the kept one first) and the kept one stays.
void NiFixedAllocator::DoDeallocate(void* p) {
    NiFixedAllocator::Chunk* pkChunk = m_pkDeallocChunk;
    size_t stBlockSize = m_stBlockSize;
    uint8_t* pucToRelease = static_cast<uint8_t*>(p);
    *pucToRelease = pkChunk->m_ucFirstAvailableBlock;
    uint8_t ucIndex = static_cast<uint8_t>(static_cast<size_t>(pucToRelease - pkChunk->m_pucData) / stBlockSize);
    ++pkChunk->m_ucBlocksAvailable;
    pkChunk->m_ucFirstAvailableBlock = ucIndex;
    NiFixedAllocator::Chunk* pkDeallocChunk = m_pkDeallocChunk;
    if (pkDeallocChunk->m_ucBlocksAvailable == m_ucNumBlocks) {
        NiFixedAllocator::Chunk* pkEmptyChunk = m_pkEmptyChunk;
        if (pkEmptyChunk) {
            NiFixedAllocator::Chunk* pkLastChunk = &m_pkChunks[m_stNumChunks - 1];
            if (pkLastChunk == pkDeallocChunk) {
                m_pkDeallocChunk = pkEmptyChunk;
            }
            else if (pkLastChunk != pkEmptyChunk) {
                SwapChunks(pkEmptyChunk, pkLastChunk);
            }
            _NiExternalAlignedFree(pkLastChunk->m_pucData);
            pkDeallocChunk = m_pkDeallocChunk;
            --m_stNumChunks;
            m_pkAllocChunk = pkDeallocChunk;
        }
        m_pkEmptyChunk = pkDeallocChunk;
    }
}

void NiFixedAllocator::Shutdown() {
    for (size_t i = 0; i < m_stNumChunks; _NiExternalAlignedFree(m_pkChunks[i++].m_pucData)) {
        ;
    }
    _NiExternalFree(m_pkChunks);
    DeleteCriticalSection(&m_kCriticalSection.m_kCriticalSection);
}

NiFixedAllocator::Chunk* SwapChunks(NiFixedAllocator::Chunk* pkLeft, NiFixedAllocator::Chunk* pkRight) {
    if (pkLeft != pkRight) {
        NiFixedAllocator::Chunk kTmp = *pkLeft;
        *pkLeft = *pkRight;
        *pkRight = kTmp;
    }
    return pkLeft;
}

// Each fixed allocator's fields are cleared (its critical section's constructor has initialised the section), then
// Init's block size and block count are set; the count divides a fixed chunk size, 25600, not stChunkSize.
NiSmallObjectAllocator::NiSmallObjectAllocator(size_t stChunkSize) : m_stChunkSize(stChunkSize) {
    for (uint32_t st = 1; st <= 256; ++st) {
        NiFixedAllocator& rkPool = m_kPool[st - 1];
        rkPool.m_pkChunks = nullptr;
        rkPool.m_stNumChunks = 0;
        rkPool.m_stMaxNumChunks = 0;
        rkPool.m_pkAllocChunk = nullptr;
        rkPool.m_pkDeallocChunk = nullptr;
        rkPool.m_pkEmptyChunk = nullptr;
        rkPool.m_stBlockSize = st;
        uint32_t uiNumBlocks = 25600 / st;
        if (uiNumBlocks > 255) {
            uiNumBlocks = 255;
        }
        else if (uiNumBlocks == 0) {
            uiNumBlocks = 8 * st;
        }
        rkPool.m_ucNumBlocks = static_cast<uint8_t>(uiNumBlocks);
    }
}

void NiSmallObjectAllocator::Deallocate(void* p, size_t stNumBytes) {
    NiFixedAllocator* v3 = &m_kPool[stNumBytes - 1];
    EnterCriticalSection(&v3->m_kCriticalSection.m_kCriticalSection);
    v3->m_pkDeallocChunk = v3->VicinityFind(p);
    v3->DoDeallocate(p);
    LeaveCriticalSection(&v3->m_kCriticalSection.m_kCriticalSection);
}

void* NiSmallObjectAllocator::Allocate(uint32_t stNumBytes) {
    return m_kPool[stNumBytes - 1].Allocate();
}
