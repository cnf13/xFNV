#pragma once

#include "BSCore/BSSpinLock.h"

#include <cstddef>

// A per-thread stack allocator for short-lived scratch memory. Only some of its members are declared.
class ScrapHeap {
public:
    // The block header written before each allocation.
    struct Header;
    // A free block.
    struct FreeBlock;

    ScrapHeap();
    ScrapHeap(uint32_t a1);
    ~ScrapHeap();

    void* Allocate(uint32_t aSize, uint32_t aAlignment);
    void Deallocate(void* apMem);
    void Init(uint32_t aiInitialSize);
    void* Allocate(uint32_t aiSize, const char* pcFile, int32_t aiLine, uint32_t aAlignment);
    // The size word of the block header before apMem, its flag bits masked off.
    uint32_t Size(void* apMem);
    void Clean(bool abCheck);
    void CheckForLeaks();
    uint32_t GetAllocationCount();

protected:
    // The members are the buffer, the next free byte, the buffer end and the last block.
    char* pMemHeap;

public:
    char* pCurrentStackLoc;

protected:
    char* pEndHeap;
    Header* pLastBlock;

public:
    // The default alignment; it is read from memory at each call site, so it is a variable rather than a constant.
    static inline uint32_t ALIGNMENT = 4;
};

// Allocates nbytes from arScrapHeap at ScrapHeap::ALIGNMENT; defined in ScrapHeap.cpp.
void* operator new(size_t nbytes, ScrapHeap& arScrapHeap);

// The block header: the size, its top bit set once the block is freed, then the previous block.
struct ScrapHeap::Header {
    uint32_t uiSizeAndFlags;
    Header* pPrevious;
};

// A free block: the header and the links to the free blocks to its left and right.
struct ScrapHeap::FreeBlock : ScrapHeap::Header {
    FreeBlock* pLeft;
    FreeBlock* pRight;
};

// Owns the memory buffers the scrap heaps carve from: each buffer's base and committed size, and the lock that guards
// the list.
class ScrapHeapManager {
public:
    struct Buffer {
        char* pBuffer;
        uint32_t uiCommittedSize;
    };

    ScrapHeapManager();
    ~ScrapHeapManager();

    // A buffer of at least arSize bytes; arSize receives the size handed out.
    char* RequestBuffer(uint32_t& ariRequestedInitialSize);
    // Releases the committed buffer apBuffer. VirtualFree's result is not returned, and no caller needs it.
    void FreeBuffer(char* apBuffer, uint32_t aiCommittedSize);
    // Keeps apBuffer for reuse while there is room in kBuffers; a full list frees it at once.
    void ReleaseBuffer(char* apBuffer, uint32_t aiCommittedSize);
    // Frees every kept buffer.
    void FreeAllBuffers();
    // Frees kept buffers, last first, until at least aiSize bytes are freed or none is left; returns the bytes freed.
    uint32_t FreeBuffers(uint32_t aiSize);
    void ShrinkBuffer(char* apBuffer, uint32_t aiBufferSize, uint32_t aiShrinkSize);
    char* AllocateBuffer(uint32_t aiInitialSize);
    void ExpandBuffer(char* apBuffer, uint32_t aiBufferSize, uint32_t aiAdditionalSize);
    void RemoveBuffer(uint32_t aiIndex);

    // The singleton.
    static ScrapHeapManager* GetSingleton();
    static void Init();
    static void Shutdown();

    Buffer Buffers[64];
    uint32_t iBufferCount;
    uint32_t iBufferTotalSize;
    uint8_t padding208[24];
    BSSpinLock BufferLock;

    static inline ScrapHeapManager* pInstance;
};
