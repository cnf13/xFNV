#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/Windows/winbase.h"

uint32_t hkMemoryRouter::s_memoryRouter = TlsAlloc();

// An empty stack (hkStackMemory's own constructor) and no allocators yet.
hkMemoryRouter::hkMemoryRouter()
  : m_temp(nullptr), m_heap(nullptr), m_debug(nullptr), m_solver(nullptr), m_global(nullptr) {}

hkMemoryRouter::~hkMemoryRouter() {}

void hkMemoryRouter::resetPeakMemoryStatistics() {
    m_debug->resetPeakMemoryStatistics();
    m_global->resetPeakMemoryStatistics();
    m_heap->resetPeakMemoryStatistics();
    m_solver->resetPeakMemoryStatistics();
    m_temp->resetPeakMemoryStatistics();
}

void* hkMemoryRouter::GetHeap() {
    return m_heap;
}

hkMemoryRouter& hkMemoryRouter::getInstance() {
    return *static_cast<hkMemoryRouter*>(TlsGetValue(s_memoryRouter));
}

void hkMemoryRouter::replaceInstance(hkMemoryRouter* a) {
    TlsSetValue(s_memoryRouter, a);
}

hkMemoryRouter* hkMemoryRouter::getInstancePtr() {
    return static_cast<hkMemoryRouter*>(TlsGetValue(s_memoryRouter));
}

void hkMemoryRouter::easyFree(hkMemoryAllocator& b, void* p) {
    if (p) {
        BlockHeader* pHeader = static_cast<BlockHeader*>(p) - 1;
        int32_t iNumBytes = pHeader->m_numBytes + static_cast<int32_t>(sizeof(BlockHeader));
        pHeader->m_allocator = nullptr;
        b.blockFree(pHeader, iNumBytes);
    }
}

// Takes nbytes + alignment bytes and a BlockHeader from the allocator and returns the first alignment-aligned address
// with room for the header in front of it; the header keeps the allocator, the byte count and the distance back to the
// block. The block is treated as raw, untyped memory: the address is rounded as an integer.
void* hkMemoryRouter::alignedAlloc(hkMemoryAllocator& b, int32_t nbytes, int32_t alignment) {
    int32_t iNumBytes = nbytes + alignment;
    char* pBlock = static_cast<char*>(b.blockAlloc(iNumBytes + static_cast<int32_t>(sizeof(BlockHeader))));
    char* pAligned = reinterpret_cast<char*>(
        reinterpret_cast<uintptr_t>(pBlock + alignment + sizeof(BlockHeader) - 1) &
        ~static_cast<uintptr_t>(alignment - 1)
    );
    BlockHeader* pHeader = static_cast<BlockHeader*>(static_cast<void*>(pAligned)) - 1;
    pHeader->m_numBytes = iNumBytes;
    pHeader->m_allocator = &b;
    pHeader->m_offset = static_cast<int32_t>(pAligned - pBlock);
    return pAligned;
}

// Frees a block alignedAlloc made: the allocator's byte count and block come from the BlockHeader, whose allocator
// word is cleared first.
void hkMemoryRouter::alignedFree(hkMemoryAllocator& b, void* p) {
    if (p) {
        BlockHeader* pHeader = static_cast<BlockHeader*>(p) - 1;
        int32_t iNumBytes = pHeader->m_numBytes + static_cast<int32_t>(sizeof(BlockHeader));
        char* pBlock = static_cast<char*>(p) - pHeader->m_offset;
        pHeader->m_allocator = nullptr;
        b.blockFree(pBlock, iNumBytes);
    }
}

// Allocates nbytes from b with a BlockHeader in front of the block and returns the payload: stores the size, then the
// allocator.
void* hkMemoryRouter::easyAlloc(hkMemoryAllocator& b, int32_t nbytes) {
    BlockHeader* pHeader = static_cast<BlockHeader*>(b.blockAlloc(nbytes + static_cast<int32_t>(sizeof(BlockHeader))));
    pHeader->m_numBytes = nbytes;
    pHeader->m_allocator = &b;
    return pHeader + 1;
}

hkStackMemory& hkStackMemory::getInstance() {
    return hkMemoryRouter::getInstance().m_stack;
}
