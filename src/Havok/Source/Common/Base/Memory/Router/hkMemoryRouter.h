#pragma once

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/MemoryClasses/hkMemoryClasses.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"

struct hkpRootCdPoint;

// Frees a block from the thread's stack allocator.
template <typename T>
void hkDeallocateStack(T* ptr);
// Takes n elements of T from the thread's stack allocator, the byte count rounded up to a multiple of 128; what names
// the use.
template <typename T>
T* hkAllocateStack(int32_t n, const char* what);
// hkAllocateStack<hkpRootCdPoint>: allocates n elements' bytes (rounded up to 128) from the thread's stack allocator.
// what is passed to hkStackMemory::allocateStack.
void* hkAllocateStackRootCdPoints(int32_t n, char* what);

// The calling thread's set of allocators. Only some of its members are declared.
class hkMemoryRouter {
public:
    // The header easyAlloc and alignedAlloc put in front of the block they return, which easyFree and alignedFree read
    // back. m_allocator is the allocator, m_numBytes is the byte count passed to the allocator less the header's size,
    // and m_offset (alignedAlloc only) is the distance back to the allocator's block.
    struct BlockHeader {
        hkMemoryAllocator* m_allocator;
        int32_t m_numBytes;
        int32_t m_offset;
        // Never written or read; it counts in the header's size.
        int32_t m_padding;
    };

    hkMemoryRouter();
    ~hkMemoryRouter();

    // Clears the peak figures the router's allocators keep.
    void resetPeakMemoryStatistics();
    // The temp allocator: short-lived blocks.
    hkMemoryAllocator& temp() {
        return *m_temp;
    }
    void setTemp(hkMemoryAllocator* a) {
        m_temp = a;
    }
    // The heap allocator: long-lived blocks.
    hkMemoryAllocator& heap() {
        return *m_heap;
    }
    void setHeap(hkMemoryAllocator* a) {
        m_heap = a;
    }
    hkMemoryAllocator& debug() {
        return *m_debug;
    }
    void setDebug(hkMemoryAllocator* a) {
        m_debug = a;
    }
    hkMemoryAllocator& solver() {
        return *m_solver;
    }
    void setSolver(hkMemoryAllocator* a) {
        m_solver = a;
    }
    hkMemoryAllocator& global() {
        return *m_global;
    }
    void setGlobal(hkMemoryAllocator* a) {
        m_global = a;
    }
    void* getUserData() const {
        return m_userData;
    }
    void setUserData(void* p) {
        m_userData = p;
    }
    hkMemoryAllocator* GetHeap();

    // The router of the calling thread.
    static hkMemoryRouter& getInstance();
    // Makes the given router the calling thread's router.
    static void replaceInstance(hkMemoryRouter* a);
    // The calling thread's router, or null.
    static hkMemoryRouter* getInstancePtr();
    // Frees a block apPtr that easyAlloc made from apAllocator, reading its size from the block header.
    static void easyFree(hkMemoryAllocator& b, void* p);
    // Allocates aiNumBytes from apAllocator, aligned to aiAlign (a power of two), with a BlockHeader in front of the
    // returned pointer.
    static void* alignedAlloc(hkMemoryAllocator& b, int32_t nbytes, int32_t alignment);
    // Frees a block alignedAlloc made.
    static void alignedFree(hkMemoryAllocator& b, void* p);
    // Allocates aiNumBytes from apAllocator, with a BlockHeader in front of the returned pointer.
    static void* easyAlloc(hkMemoryAllocator& b, int32_t nbytes);

    hkStackMemory m_stack;
    hkMemoryAllocator* m_temp;
    hkMemoryAllocator* m_heap;
    hkMemoryAllocator* m_debug;
    hkMemoryAllocator* m_solver;
    hkMemoryAllocator* m_global;
    void* m_userData;

    // The TLS slot holding each thread's router.
    static uint32_t s_memoryRouter;
};

// These allocate from the thread's router's stack allocator.
template <typename T>
T* hkAllocateStack(int32_t n, const char* what) {
    const int32_t numBytes = (n * static_cast<int32_t>(sizeof(T)) + 127) & ~127;
    return static_cast<T*>(hkMemoryRouter::getInstance().m_stack.allocateStack(numBytes, what));
}

template <typename T>
void hkDeallocateStack(T* ptr) {
    hkMemoryRouter::getInstance().m_stack.deallocateStack(ptr);
}

// Keeps the first n elements of the last block from the thread's stack allocator, rounded up to 128 bytes as
// hkAllocateStack rounds.
template <typename T>
void hkShrinkAllocatedStack(T* ptr, int32_t n) {
    hkMemoryRouter::getInstance().m_stack.shrinkAllocatedStack(ptr, (n * static_cast<int32_t>(sizeof(T)) + 127) & ~127);
}

// numberOfObjects elements of T from the thread's heap.
template <typename T>
T* hkAllocateChunk(int32_t numberOfObjects, [[maybe_unused]] HK_MEMORY_CLASS cl) {
    return static_cast<T*>(
        hkMemoryRouter::getInstance().heap().blockAlloc(numberOfObjects * hkSizeOfTypeOrVoid<T>::val)
    );
}

// Gives numberOfObjects elements of T at ptr back to the thread's heap.
template <typename T>
void hkDeallocateChunk(T* ptr, int32_t numberOfObjects, [[maybe_unused]] HK_MEMORY_CLASS cl) {
    hkMemoryRouter::getInstance().heap().blockFree(ptr, numberOfObjects * hkSizeOfTypeOrVoid<T>::val);
}
