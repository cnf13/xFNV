#include "BSCore/ScrapHeap.h"

#include "BSCore/BSSpinLock.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "platform/Memory.h"

#if FNVD_SYSTEM_ALLOC
#include "xfnv/debugheap/SystemAlloc.h"

// Each block is an aligned allocation of the pass-through, laid out as [the allocation's start][Header][block], so that
// Size reads the header; the headers stay linked through pPrevious from pLastBlock, which Deallocate unlinks from and
// Clean (the stack's reset, which frees no block one by one) walks to free every block left. The buffer is still
// requested and released, never used.
static void* ScrapPassAllocate(ScrapHeap::Header*& arLastBlock, uint32_t aSize, uint32_t aAlignment) {
    const size_t uiAlign = aAlignment > 4 ? aAlignment : 4;
    const size_t uiOffset = (sizeof(ScrapHeap::Header) + sizeof(void*) + uiAlign - 1) & ~(uiAlign - 1);
    char* pStart = static_cast<char*>(fnvd::sysalloc::AlignedAllocate(aSize + uiOffset, uiAlign));
    char* pMem = pStart + uiOffset;
    ScrapHeap::Header* pHeader = reinterpret_cast<ScrapHeap::Header*>(pMem) - 1;
    reinterpret_cast<char**>(pHeader)[-1] = pStart;
    pHeader->uiSizeAndFlags = aSize;
    pHeader->pPrevious = arLastBlock;
    arLastBlock = pHeader;
    return pMem;
}

static void ScrapPassFree(ScrapHeap::Header* apHeader) {
    fnvd::sysalloc::AlignedFree(reinterpret_cast<char**>(apHeader)[-1]);
}
#endif

// Carves aSize bytes, aligned to aAlignment, off the top of the stack, its header right below the memory; grows the
// committed buffer (by doubling, up to 8 MB in use) when the block would pass its end. Null when it still does not fit.
void* ScrapHeap::Allocate(uint32_t aSize, uint32_t aAlignment) {
#if FNVD_SYSTEM_ALLOC
    return ScrapPassAllocate(pLastBlock, aSize, aAlignment);
#endif
    void* pResult = nullptr;
    char* pmemLoc = pCurrentStackLoc + sizeof(Header);
    uint32_t uialignmentOffset;
    if (aAlignment) {
        uialignmentOffset = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pmemLoc) & (aAlignment - 1));
    }
    else {
        uialignmentOffset = 0;
    }
    // The heap is raw memory: the header is placed in it by address.
    Header* pheader;
    if (uialignmentOffset) {
        pmemLoc += aAlignment - uialignmentOffset;
        pheader = reinterpret_cast<Header*>(pmemLoc) - 1;
    }
    else {
        pheader = reinterpret_cast<Header*>(pCurrentStackLoc);
    }
    char* pnextStackLoc = pmemLoc + aSize;
    if (pnextStackLoc >= pEndHeap) {
        uint32_t imemAllocated = pEndHeap - pMemHeap;
        if (imemAllocated < 0x800000) {
            uint32_t imemNeeded = pnextStackLoc - pEndHeap;
            uint32_t imemToCommit = imemAllocated;
            uint32_t inewSize = imemAllocated + imemToCommit;
            while (imemToCommit < imemNeeded) {
                inewSize <<= 1;
                imemToCommit = inewSize - imemAllocated;
            }
            uint32_t icommitted = pEndHeap - pMemHeap;
            char* pbuffer = pMemHeap;
            ScrapHeapManager* pmanager = ScrapHeapManager::GetSingleton();
            pmanager->ExpandBuffer(pbuffer, icommitted, imemToCommit);
            pEndHeap += imemToCommit;
        }
    }
    if (pnextStackLoc < pEndHeap) {
        pheader->uiSizeAndFlags = aSize;
        pheader->pPrevious = pLastBlock;
        pLastBlock = pheader;
        pCurrentStackLoc = pnextStackLoc;
        pResult = pmemLoc;
    }
    return pResult;
}

// Marks apMem's block freed (the size's top bit), drops the freed blocks off the top of the stack, and gives back
// half the committed buffer when more than half of it is unused and it is over 64 KB.
void ScrapHeap::Deallocate(void* apMem) {
    if (!apMem) {
        return;
    }
#if FNVD_SYSTEM_ALLOC
    Header* pFreed = static_cast<Header*>(apMem) - 1;
    for (Header** ppLink = &pLastBlock; *ppLink; ppLink = &(*ppLink)->pPrevious) {
        if (*ppLink == pFreed) {
            *ppLink = pFreed->pPrevious;
            ScrapPassFree(pFreed);
            return;
        }
    }
    return;
#endif
    Header* pheader = static_cast<Header*>(apMem) - 1;
    if (pheader->uiSizeAndFlags & 0x80000000) {
        return;
    }
    pheader->uiSizeAndFlags |= 0x80000000;
    while (true) {
        if (!pLastBlock) {
            pCurrentStackLoc = pMemHeap;
            break;
        }
        if (!(pLastBlock->uiSizeAndFlags & 0x80000000)) {
            // The stack top is the raw byte after the live block's memory.
            pCurrentStackLoc = reinterpret_cast<char*>(pLastBlock + 1) + pLastBlock->uiSizeAndFlags;
            break;
        }
        pLastBlock = pLastBlock->pPrevious;
    }
    uint32_t imemUnused = pEndHeap - pCurrentStackLoc;
    uint32_t uiSize = pEndHeap - pMemHeap;
    uint32_t imemToFree = uiSize >> 1;
    if (imemUnused > imemToFree && uiSize > 0x10000) {
        uint32_t icommitted = pEndHeap - pMemHeap;
        char* pbuffer = pMemHeap;
        ScrapHeapManager* pmanager = ScrapHeapManager::GetSingleton();
        pmanager->ShrinkBuffer(pbuffer, icommitted, imemToFree);
        pEndHeap -= imemToFree;
    }
}

void ScrapHeap::Clean(bool abCheck) {
#if FNVD_SYSTEM_ALLOC
    while (Header* pHeader = pLastBlock) {
        pLastBlock = pHeader->pPrevious;
        ScrapPassFree(pHeader);
    }
#endif
    pCurrentStackLoc = pMemHeap;
    pLastBlock = 0;
}

// Hands the buffer back to the manager.
ScrapHeap::~ScrapHeap() {
#if FNVD_SYSTEM_ALLOC
    Clean(false);
#endif
    ScrapHeapManager::GetSingleton()->ReleaseBuffer(pMemHeap, static_cast<uint32_t>(pEndHeap - pMemHeap));
    pMemHeap = nullptr;
    pCurrentStackLoc = nullptr;
}

ScrapHeap::ScrapHeap() {
    Init(0x10000u);
}

// A size that is not a whole number of 64 KiB is rounded up to the next one.
ScrapHeap::ScrapHeap(uint32_t a1) {
    Init(static_cast<uint16_t>(a1) ? (a1 + 0x10000u) & 0xFFFF0000u : a1);
}

void ScrapHeap::Init(uint32_t aiInitialSize) {
    ScrapHeapManager* singleton = ScrapHeapManager::GetSingleton();
    pMemHeap = singleton->RequestBuffer(aiInitialSize);
    pCurrentStackLoc = pMemHeap;
    pEndHeap = static_cast<char*>(static_cast<void*>(&pMemHeap[aiInitialSize]));
    pLastBlock = nullptr;
}

void* ScrapHeap::Allocate(uint32_t aiSize, const char* pcFile, int32_t aiLine, uint32_t aAlignment) {
    return Allocate(aiSize, aAlignment);
}

uint32_t ScrapHeap::Size(void* apMem) {
    if (apMem) {
        // The allocation's header sits directly before the block; its size word carries flags in its low bits.
        return (static_cast<Header*>(apMem) - 1)->uiSizeAndFlags & 0x7FFFFFFF;
    }
    else {
        return 0;
    }
}

void ScrapHeap::CheckForLeaks() {
    if (pCurrentStackLoc != pMemHeap) {
        for (ScrapHeap::Header* i = pLastBlock; i; i = static_cast<ScrapHeap::Header*>(i->pPrevious)) {
            ;
        }
    }
}

uint32_t ScrapHeap::GetAllocationCount() {
    uint32_t icount = 0;
    if (pCurrentStackLoc != pMemHeap) {
        for (ScrapHeap::Header* pheader = pLastBlock; pheader;
             pheader = static_cast<ScrapHeap::Header*>(pheader->pPrevious)) {
            if ((pheader->uiSizeAndFlags & 0x80000000u) == 0) {
                ++icount;
            }
        }
    }
    return icount;
}

void* operator new(size_t nbytes, ScrapHeap& arScrapHeap) {
    return arScrapHeap.Allocate(static_cast<uint32_t>(nbytes), ScrapHeap::ALIGNMENT);
}

ScrapHeapManager::ScrapHeapManager() : iBufferCount(0), iBufferTotalSize(0) {}

ScrapHeapManager::~ScrapHeapManager() {
    FreeAllBuffers();
}

char* ScrapHeapManager::RequestBuffer(uint32_t& ariRequestedInitialSize) {
    int8_t* pMemory;
    uint32_t auiBuffer;
    char* pNewMemory;
    int8_t* a1;
    uint32_t uiLargestBufferIndex;
    uint32_t uiLargestAvailableSize;
    if (!iBufferCount) {
        return AllocateBuffer(ariRequestedInitialSize);
    }
    (&BufferLock)->Lock("ScrapHeapManager::RequestBuffer()");
    if (iBufferCount) {
        uiLargestBufferIndex = 0;
        uiLargestAvailableSize = 0;
        for (auiBuffer = 0; auiBuffer < iBufferCount; ++auiBuffer) {
            if (Buffers[auiBuffer].uiCommittedSize >= (ariRequestedInitialSize)) {
                ariRequestedInitialSize = Buffers[auiBuffer].uiCommittedSize;
                pMemory = static_cast<int8_t*>(static_cast<void*>(Buffers[auiBuffer].pBuffer));
                RemoveBuffer(auiBuffer);
                if (!(--BufferLock.uiLockCount)) {
                    BufferLock.uiOwningThread = (uint32_t)0;
                }
                return static_cast<char*>(static_cast<void*>(pMemory));
            }
            if (Buffers[auiBuffer].uiCommittedSize > uiLargestAvailableSize) {
                uiLargestAvailableSize = Buffers[auiBuffer].uiCommittedSize;
                uiLargestBufferIndex = auiBuffer;
            }
        }
        a1 = static_cast<int8_t*>(static_cast<void*>(Buffers[uiLargestBufferIndex].pBuffer));
        RemoveBuffer(uiLargestBufferIndex);
        ExpandBuffer(
            static_cast<char*>(static_cast<void*>(a1)), uiLargestAvailableSize,
            (ariRequestedInitialSize)-uiLargestAvailableSize
        );
        if (!(--BufferLock.uiLockCount)) {
            BufferLock.uiOwningThread = (uint32_t)0;
        }
        return static_cast<char*>(static_cast<void*>(a1));
    }
    else {
        pNewMemory = AllocateBuffer(ariRequestedInitialSize);
        if (!(--BufferLock.uiLockCount)) {
            BufferLock.uiOwningThread = (uint32_t)0;
        }
        return pNewMemory;
    }
}

void ScrapHeapManager::FreeBuffer(char* apBuffer, uint32_t aiCommittedSize) {
    platform::Release(apBuffer);
}

// A full list frees the buffer before taking the lock and, still full under the lock, frees it again.
void ScrapHeapManager::ReleaseBuffer(char* apBuffer, uint32_t aiCommittedSize) {
    if (iBufferCount >= 64) {
        FreeBuffer(apBuffer, aiCommittedSize);
    }
    BufferLock.Lock("ScrapHeapManager::ReleaseBuffer()");
    if (iBufferCount < 64) {
        Buffers[iBufferCount].pBuffer = apBuffer;
        Buffers[iBufferCount].uiCommittedSize = aiCommittedSize;
        ++iBufferCount;
        iBufferTotalSize += aiCommittedSize;
    }
    else {
        FreeBuffer(apBuffer, aiCommittedSize);
    }
    // BSSpinLock::Unlock.
    if (--BufferLock.uiLockCount == 0) {
        BufferLock.uiOwningThread = 0;
    }
}

void ScrapHeapManager::FreeAllBuffers() {
    if (!iBufferCount) {
        return;
    }
    BufferLock.Lock("ScrapHeapManager::FreeAllBuffers()");
    for (uint32_t i = 0; i < iBufferCount; ++i) {
        FreeBuffer(Buffers[i].pBuffer, Buffers[i].uiCommittedSize);
    }
    iBufferCount = 0;
    iBufferTotalSize = 0;
    // BSSpinLock::Unlock.
    if (--BufferLock.uiLockCount == 0) {
        BufferLock.uiOwningThread = 0;
    }
}

uint32_t ScrapHeapManager::FreeBuffers(uint32_t aiSize) {
    if (!iBufferCount) {
        return 0;
    }
    BufferLock.Lock("ScrapHeapManager::FreeBuffers()");
    uint32_t uiFreedCount = 0;
    while (iBufferCount && uiFreedCount < aiSize) {
        uint32_t uiLastBufferIndex = iBufferCount - 1;
        uiFreedCount += Buffers[uiLastBufferIndex].uiCommittedSize;
        FreeBuffer(Buffers[uiLastBufferIndex].pBuffer, Buffers[uiLastBufferIndex].uiCommittedSize);
        --iBufferCount;
    }
    iBufferTotalSize -= uiFreedCount;
    // BSSpinLock::Unlock.
    if (--BufferLock.uiLockCount == 0) {
        BufferLock.uiOwningThread = 0;
    }
    return uiFreedCount;
}

void ScrapHeapManager::ShrinkBuffer(char* apBuffer, uint32_t aiBufferSize, uint32_t aiShrinkSize) {
    platform::Decommit(&apBuffer[aiBufferSize - aiShrinkSize], aiShrinkSize);
}

char* ScrapHeapManager::AllocateBuffer(uint32_t aiInitialSize) {
    uint8_t apbAllowSystemAllocs;
    void* lpAddress = platform::Reserve(0x800000u);
    int32_t aiMemoryPass = 0;
    while (!platform::Commit(lpAddress, aiInitialSize)) {
        apbAllowSystemAllocs = 1;
        aiMemoryPass =
            (&MemoryManager_Singleton)->ProcessMemoryProblem(nullptr, aiMemoryPass, (bool*)(&apbAllowSystemAllocs));
    }
    return static_cast<char*>(static_cast<void*>(static_cast<int8_t*>(lpAddress)));
}

void ScrapHeapManager::ExpandBuffer(char* apBuffer, uint32_t aiBufferSize, uint32_t aiAdditionalSize) {
    bool ballowSystemAlloc;
    int32_t imemoryPass = 0;
    while (!platform::Commit(&apBuffer[aiBufferSize], aiAdditionalSize)) {
        ballowSystemAlloc = 1;
        imemoryPass = MemoryManager_Singleton.ProcessMemoryProblem(0, imemoryPass, &ballowSystemAlloc);
    }
}

void ScrapHeapManager::RemoveBuffer(uint32_t aiIndex) {
    --this->iBufferCount;
    iBufferTotalSize -= Buffers[aiIndex].uiCommittedSize;
    if (aiIndex < this->iBufferCount) {
        uint32_t iBufferCount = this->iBufferCount;
        char* pBuffer = Buffers[iBufferCount].pBuffer;
        uint32_t uiCommittedSize = Buffers[iBufferCount].uiCommittedSize;
        Buffers[aiIndex].pBuffer = pBuffer;
        Buffers[aiIndex].uiCommittedSize = uiCommittedSize;
    }
}

ScrapHeapManager* ScrapHeapManager::GetSingleton() {
    ScrapHeapManager* v1;
    if (ScrapHeapManager::pInstance) {
        return ScrapHeapManager::pInstance;
    }
    void* v2 = ourMalloc(sizeof(ScrapHeapManager));
    if (v2) {
        v1 = ::new (v2) ScrapHeapManager;
    }
    else {
        v1 = nullptr;
    }
    ScrapHeapManager::pInstance = v1;
    return ScrapHeapManager::pInstance;
}

void ScrapHeapManager::Init() {
    ScrapHeapManager* v0;
    void* v1 = (&MemoryManager_Singleton)->Allocate(sizeof(ScrapHeapManager));
    if (v1) {
        v0 = ::new (v1) ScrapHeapManager;
    }
    else {
        v0 = nullptr;
    }
    ScrapHeapManager::pInstance = v0;
}

void ScrapHeapManager::Shutdown() {
    ScrapHeapManager* block = ScrapHeapManager::pInstance;
    if (ScrapHeapManager::pInstance) {
        ScrapHeapManager::pInstance->~ScrapHeapManager();
        (&MemoryManager_Singleton)->Deallocate(block);
    }
    ScrapHeapManager::pInstance = nullptr;
}
