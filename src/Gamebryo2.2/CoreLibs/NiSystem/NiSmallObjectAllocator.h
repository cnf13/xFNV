#pragma once

#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

#include <cstdint>

// Allocator of blocks of one size: the chunks of blocks it has, the ones it allocated from and freed to last, and the
// critical section guarding them. The section sits on its own 128-byte line.
class NiFixedAllocator {
public:
    // A run of blocks of the allocator's size: its memory, the first free block and how many are free.
    struct Chunk {
        // Allocates ucBlocks blocks of stBlockSize bytes and links them all as free.
        void Init(size_t stBlockSize, uint8_t ucBlocks) {
            m_pucData = static_cast<uint8_t*>(_NiExternalAlignedMalloc(stBlockSize * ucBlocks, 4));
            Reset(stBlockSize, ucBlocks);
        }

        // Links all ucBlocks blocks as free: each block's first byte holds the index of the next.
        void Reset(size_t stBlockSize, uint8_t ucBlocks) {
            m_ucFirstAvailableBlock = 0;
            m_ucBlocksAvailable = ucBlocks;
            uint8_t* pucBlock = m_pucData;
            for (uint8_t i = 0; i != ucBlocks;) {
                *pucBlock = ++i;
                pucBlock += stBlockSize;
            }
        }

        // Takes the first free block, or null when none is left.
        void* Allocate(size_t stBlockSize) {
            uint8_t ucBlocksAvailable = m_ucBlocksAvailable;
            if (!ucBlocksAvailable) {
                return nullptr;
            }
            uint8_t* pucResult = &m_pucData[stBlockSize * m_ucFirstAvailableBlock];
            m_ucFirstAvailableBlock = *pucResult;
            m_ucBlocksAvailable = static_cast<uint8_t>(ucBlocksAvailable - 1);
            return pucResult;
        }

        uint8_t* m_pucData;
        uint8_t m_ucFirstAvailableBlock;
        uint8_t m_ucBlocksAvailable;
    };

    void* Allocate();
    NiFixedAllocator::Chunk* VicinityFind(void* p);
    void DoDeallocate(void* p);
    void Shutdown();

private:
    // Grows the chunk array to hold at least stNewSize chunks.
    void Reserve(size_t stNewSize) {
        if (stNewSize > m_stMaxNumChunks) {
            m_pkChunks = static_cast<Chunk*>(_NiExternalRealloc(m_pkChunks, sizeof(Chunk) * stNewSize));
            m_stMaxNumChunks = stNewSize;
        }
    }

    void Push_Back(Chunk& kChunk) {
        size_t stCount = m_stNumChunks;
        Reserve(stCount + 1);
        m_pkChunks[stCount] = kChunk;
        ++m_stNumChunks;
    }

public:
    size_t m_stBlockSize;
    uint8_t m_ucNumBlocks;
    Chunk* m_pkChunks;
    size_t m_stNumChunks;
    size_t m_stMaxNumChunks;
    Chunk* m_pkAllocChunk;
    Chunk* m_pkDeallocChunk;
    Chunk* m_pkEmptyChunk;
    NiCriticalSection m_kCriticalSection;
};

// Equivalent to std::swap<NiFixedAllocator::Chunk>.
NiFixedAllocator::Chunk* SwapChunks(NiFixedAllocator::Chunk* pkLeft, NiFixedAllocator::Chunk* pkRight);

// The allocator for small objects: 256 fixed allocators (one per block size class) and the size of their chunks.
class NiSmallObjectAllocator {
public:
    NiSmallObjectAllocator(size_t stChunkSize);

    // Gives the block apMem of stSizeInBytes back to the pool.
    void Deallocate(void* p, size_t stNumBytes);
    void* Allocate(uint32_t stNumBytes);

    NiFixedAllocator m_kPool[256];
    size_t m_stChunkSize;
};
