#pragma once

#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"

// Allocator of blocks of one size: the chunks of blocks it has, the ones it allocated from and freed to last, and the
// critical section guarding them. The section sits on its own 128-byte line.
class NiFixedAllocator {
public:
    // A run of blocks of the allocator's size: its memory, the first free block and how many are free.
    struct Chunk {
        uint8_t* m_pucData;
        uint8_t m_ucFirstAvailableBlock;
        uint8_t m_ucBlocksAvailable;
    };

    void* Allocate();
    NiFixedAllocator::Chunk* VicinityFind(void* p);
    void DoDeallocate(void* p);
    void Shutdown();

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
NiFixedAllocator::Chunk* SwapChunks(NiFixedAllocator::Chunk* a1, NiFixedAllocator::Chunk* a2);

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
