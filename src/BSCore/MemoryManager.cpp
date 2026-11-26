#include "BSCore/MemoryManager.h"

#include "BSCore/IMemoryHeap.h"
#include "BSCore/MemoryPool.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/ScrapHeap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "platform/Memory.h"
#include "platform/System.h"
#include "xfnv/Compiler.h"

#include <new>

#if XFNV_SYSTEM_ALLOC
#include "xfnv/debugheap/SystemAlloc.h"
#endif

uint32_t _tls_index;

XFNV_CONSTRUCT_FIRST MemoryManager MemoryManager::s_Instance;

// The global operator new and delete are the engine's heap, so memory freed with ourFree or Deallocate (and the
// reverse) stays on one heap. Before the singleton's constructor ran (static initialisation) Allocate and Deallocate
// fall back to malloc and free, and a pointer in no pool or heap is freed with free later.
void* operator new(size_t aSize) {
    return MemoryManager::Instance().Allocate(aSize);
}

void* operator new[](size_t aSize) {
    return MemoryManager::Instance().Allocate(aSize);
}

void* operator new(size_t aSize, const std::nothrow_t&) noexcept {
    return MemoryManager::Instance().Allocate(aSize);
}

void* operator new[](size_t aSize, const std::nothrow_t&) noexcept {
    return MemoryManager::Instance().Allocate(aSize);
}

void operator delete(void* apMem) noexcept {
    MemoryManager::Instance().Deallocate(apMem);
}

void operator delete[](void* apMem) noexcept {
    MemoryManager::Instance().Deallocate(apMem);
}

void operator delete(void* apMem, size_t) noexcept {
    MemoryManager::Instance().Deallocate(apMem);
}

void operator delete[](void* apMem, size_t) noexcept {
    MemoryManager::Instance().Deallocate(apMem);
}

void operator delete(void* apMem, const std::nothrow_t&) noexcept {
    MemoryManager::Instance().Deallocate(apMem);
}

void operator delete[](void* apMem, const std::nothrow_t&) noexcept {
    MemoryManager::Instance().Deallocate(apMem);
}

// Over-aligned forms: aSize plus the alignment from Allocate, moved up to the alignment, the byte before the result
// holding how far (1 to the alignment; 256 is stored as 0). The delete forms read that byte back.
static void* AlignedAllocate(size_t aSize, std::align_val_t aAlign) noexcept {
    const size_t uiAlign = static_cast<size_t>(aAlign);
    if (uiAlign > 256) {
        return nullptr;
    }
    uint8_t* pMemory = static_cast<uint8_t*>(MemoryManager::Instance().Allocate(aSize + uiAlign));
    const size_t uiPadding = uiAlign - (reinterpret_cast<uintptr_t>(pMemory) & (uiAlign - 1));
    pMemory += uiPadding;
    pMemory[-1] = static_cast<uint8_t>(uiPadding);
    return pMemory;
}

static void AlignedDeallocate(void* apMem) noexcept {
    if (!apMem) {
        return;
    }
    uint8_t* pMemory = static_cast<uint8_t*>(apMem);
    const size_t uiPadding = pMemory[-1] ? pMemory[-1] : 256;
    MemoryManager::Instance().Deallocate(pMemory - uiPadding);
}

void* operator new(size_t aSize, std::align_val_t aAlign) {
    void* pMemory = AlignedAllocate(aSize, aAlign);
    if (!pMemory) {
        throw std::bad_alloc();
    }
    return pMemory;
}

void* operator new[](size_t aSize, std::align_val_t aAlign) {
    return operator new(aSize, aAlign);
}

void* operator new(size_t aSize, std::align_val_t aAlign, const std::nothrow_t&) noexcept {
    return AlignedAllocate(aSize, aAlign);
}

void* operator new[](size_t aSize, std::align_val_t aAlign, const std::nothrow_t&) noexcept {
    return AlignedAllocate(aSize, aAlign);
}

void operator delete(void* apMem, std::align_val_t) noexcept {
    AlignedDeallocate(apMem);
}

void operator delete[](void* apMem, std::align_val_t) noexcept {
    AlignedDeallocate(apMem);
}

void operator delete(void* apMem, size_t, std::align_val_t) noexcept {
    AlignedDeallocate(apMem);
}

void operator delete[](void* apMem, size_t, std::align_val_t) noexcept {
    AlignedDeallocate(apMem);
}

void operator delete(void* apMem, std::align_val_t, const std::nothrow_t&) noexcept {
    AlignedDeallocate(apMem);
}

void operator delete[](void* apMem, std::align_val_t, const std::nothrow_t&) noexcept {
    AlignedDeallocate(apMem);
}

void MemoryManager::Initialize() {
    MemoryPool::InitPools();
#if !XFNV_SYSTEM_ALLOC
    SpecifyMemoryLayout();
#endif
    bInitialized = true;
#if !XFNV_SYSTEM_ALLOC
    SpecifyPools();
#endif
    InitializeStackTrace();
    eMemContext = MEM_CONTEXT_STATICVARS;
}

void MemoryManager::AddPool(uint32_t aiBlockSize, uint32_t aiPoolSize, const char* apDesc) {
    AutoMemContext kMemContext(MEM_CONTEXT_POOL);
    // Only the pools whose blocks are a multiple of 16 bytes are made, so that every block is aligned as the
    // platform's allocators align (and holds the free list's two pointers); the other sizes go to the next larger pool
    // (GetPoolForSize rounds up to 16) or to the heap (a departure from the exe).
    if (aiBlockSize % 16 != 0) {
        return;
    }
    bAllowPoolUse = false;
    // Allocates and frees through the singleton, not this.
    MemoryPool* pPool = new MemoryPool(aiBlockSize, aiPoolSize, apDesc);
    if ((!pPool->pHeap) && pPool) {
        delete pPool;
    }

    bAllowPoolUse = true;
}

void* MemoryManager::Allocate(size_t aSize) {
#if XFNV_SYSTEM_ALLOC
    // No pool or heap, the pass-through only; SpecifyMemoryLayout and SpecifyPools are skipped, so nothing else reaches
    // them.
    return xfnv::sysalloc::Allocate(aSize);
#endif
    if (!bInitialized) {
        return DoSystemAllocation(aSize, false);
    }
    IMemoryHeap* pDefaultHeap = pHeapsByContextA[MEM_CONTEXT_DEFAULT];
    if (!pDefaultHeap) {
        return DoSystemAllocation(aSize, false);
    }
    const size_t uiRequested = aSize;
    void* pMemory = nullptr;
    aSize = aSize > 8 ? (aSize + 3) & ~3u : 8;
    const MEM_CONTEXT eMemContext = MemoryManager::eMemContext;
    if (bAllowPoolUse) {
        MemoryPool* pPool = aSize > 0x3FC ? nullptr : MemoryPool::pMemoryPoolsBySize[aSize >> 2];
        if (!pPool) {
            pPool = MemoryPool::GetPoolForSize(aSize);
        }
        if (pPool) {
            pMemory = pPool->Allocate(uiRequested);
            if (pMemory) {
                return pMemory;
            }
        }
    }
    bool bAllowSystemAllocs = false;
    int32_t iMemoryPass = 0;
    IMemoryHeap* pHeap = pHeapsByContextA[eMemContext];
    do {
        IMemoryHeap* pCurrentHeap = pHeap ? pHeap : pDefaultHeap;
        pMemory = pCurrentHeap->Allocate(aSize, 0);
        if (!pMemory && pCurrentHeap != pDefaultHeap) {
            pCurrentHeap = pDefaultHeap;
            pMemory = pCurrentHeap->Allocate(aSize, 0);
        }
        if (!pMemory) {
            iMemoryPass = ProcessMemoryProblem(pHeap ? pHeap : pDefaultHeap, iMemoryPass, &bAllowSystemAllocs);
            if (bAllowSystemAllocs) {
                pMemory = DoSystemAllocation(aSize, false);
            }
        }
    }
    while (!pMemory);
    return pMemory;
}

void MemoryManager::Deallocate(void* apMem) {
#if XFNV_SYSTEM_ALLOC
    xfnv::sysalloc::Free(apMem);
    return;
#endif
    if (!apMem) {
        return;
    }
    if (!bInitialized || !pHeapsByContextA[MEM_CONTEXT_DEFAULT]) {
        DoSystemDeallocation(apMem, false);
        return;
    }
    // The pool lookup keys on the address's top byte, treating the pointer as a raw address.
    const uintptr_t uiTop = reinterpret_cast<uintptr_t>(apMem) >> 24;
    // Pools lie below 4 GB (PlatformReserveHeap); a higher address (which the exe's process cannot have) has no entry
    // in the 256-entry table.
    MemoryPool* pPool = uiTop < 0x100 ? MemoryPool::pMemoryPoolsByAddress[uiTop] : nullptr;
    if (pPool) {
        const char* pMem = static_cast<const char*>(apMem);
        const bool bInPool = pMem >= pPool->pHeap && pMem < pPool->pHeap + pPool->iPoolSize;
        if (bInPool) {
            pPool->Free(apMem);
            return;
        }
    }
    IMemoryHeap* pHeap = GetHeapForPointer(apMem);
    if (pHeap) {
        pHeap->Deallocate(apMem, 0);
    }
    else {
        DoSystemDeallocation(apMem, false);
    }
}

void* MemoryManager::Reallocate(void* apMem, size_t aSize) {
#if XFNV_SYSTEM_ALLOC
    return xfnv::sysalloc::Reallocate(apMem, aSize);
#endif
    void* pNewMem = nullptr;
    size_t uiSize = Size(apMem);
    if (uiSize) {
        pNewMem = Allocate(aSize);
        if (uiSize > aSize) {
            uiSize = aSize;
        }
        rt::memcpy(pNewMem, apMem, uiSize);
        Deallocate(apMem);
    }
    else {
        pNewMem = std::realloc(apMem, aSize);
    }
    return pNewMem;
}

size_t MemoryManager::Size(const void* apPtr) {
#if XFNV_SYSTEM_ALLOC
    return apPtr ? xfnv::sysalloc::Size(apPtr) : 0;
#endif
    // The pool lookup keys on the address's top byte, treating the pointer as a raw address.
    const uintptr_t uiTop = reinterpret_cast<uintptr_t>(apPtr) >> 24;
    // Pools lie below 4 GB (PlatformReserveHeap); a higher address (which the exe's process cannot have) has no entry
    // in the 256-entry table.
    MemoryPool* pPool = uiTop < 0x100 ? MemoryPool::pMemoryPoolsByAddress[uiTop] : nullptr;
    if (pPool) {
        const char* pMem = static_cast<const char*>(apPtr);
        const bool bInPool = pMem >= pPool->pHeap && pMem < pPool->pHeap + pPool->iPoolSize;
        if (bInPool) {
            return pPool->iAlignedBlockSize;
        }
    }
    IMemoryHeap* pHeap = GetHeapForPointer(apPtr);
    if (pHeap) {
        return pHeap->Size(apPtr, 0);
    }
    pHeap = GetHeapForPhysicalPointer(apPtr);
    if (pHeap) {
        return pHeap->Size(apPtr, 0);
    }
    // _msize takes a non-const pointer.
    return _msize(const_cast<void*>(apPtr));
}

void* MemoryManager::Reallocate(
    void* apMem,
    size_t aSize,
    const char* apFile,
    const int32_t aiLine,
    const char* apFunction
) {
#if XFNV_SYSTEM_ALLOC
    return xfnv::sysalloc::Reallocate(apMem, aSize);
#endif
    uint32_t ioldaSize = Size(apMem);
    if (!ioldaSize) {
        return std::realloc(apMem, aSize);
    }
    void* pmem = Allocate(aSize);
    if (ioldaSize > aSize) {
        ioldaSize = aSize;
    }
    rt::memcpy(pmem, apMem, ioldaSize);
    Deallocate(apMem);
    return pmem;
}

MemoryManager::MemoryManager()
  : bInitialized(false), iNumHeaps(0), iNumPhysicalHeaps(0), ppHeaps(nullptr), kThreadScrapHeapMap(37u),
    ppPhysicalHeaps(nullptr), bSpecialHeaps(true), bAllowPoolUse(true), iSysAllocBytes(0), iMallocBytes(0),
    uiEmergencyCleanupPassCount(0) {
    std::memset(pHeapsByContextA, 0, sizeof(pHeapsByContextA));
    // GlobalMemoryStatusEx's ullAvailPageFile: the memory a commit can still get. MB_ICONERROR (16).
    if (platform::MemoryStatus().availablePageFile < 0x20000000) {
        platform::ShowMessageBox("Not enough memory to run application.", "Memory Error", 16u);
        std::exit(0);
    }
    Initialize();
    RegisterMemoryManager();
    ScrapHeapManager::Init();
}

// Frees the threads' scrap heaps, the pools and the heaps; the map frees its own items.
MemoryManager::~MemoryManager() {
    BSMapItem<uint32_t, ScrapHeap*>* pkPos = nullptr;
    for (uint32_t i = 0; i < kThreadScrapHeapMap.m_uiHashSize; ++i) {
        if (kThreadScrapHeapMap.m_ppkHashTable[i]) {
            pkPos = kThreadScrapHeapMap.m_ppkHashTable[i];
            break;
        }
    }
    while (pkPos) {
        uint32_t uiThreadID;
        ScrapHeap* pScrapHeap;
        kThreadScrapHeapMap.GetNext(pkPos, uiThreadID, pScrapHeap);
        if (pScrapHeap) {
            pScrapHeap->~ScrapHeap();
            Deallocate(pScrapHeap);
        }
    }
    kThreadScrapHeapMap.RemoveAll_0();
    ScrapHeapManager::Shutdown();
    InitializeStackTrace();
    MemoryPool::DeletePools();
    std::memset(pHeapsByContextA, 0, sizeof(pHeapsByContextA));
    bInitialized = false;
    if (ppHeaps) {
        for (uint16_t j = 0; j < iNumHeaps; ++j) {
            delete ppHeaps[j];
            ppHeaps[j] = nullptr;
        }
        Deallocate(ppHeaps);
    }
    if (ppPhysicalHeaps) {
        for (uint16_t k = 0; k < iNumPhysicalHeaps; ++k) {
            delete ppPhysicalHeaps[k];
            ppPhysicalHeaps[k] = nullptr;
        }
        Deallocate(ppPhysicalHeaps);
    }
}

void* MemoryManager::AllocatePhysical(size_t aSize, uint32_t aiAttributes) {
    return nullptr;
}

void MemoryManager::DeallocatePhysical(void* apMem, uint32_t aiAttributes, size_t aiSize) {}

void* MemoryManager::Allocate(size_t aSize, const char* apFile, const int32_t aiLine, const char* apFunction) {
    return Allocate(aSize);
}

ScrapHeap* MemoryManager::GetThreadScrapHeap() {
    ScrapHeap* pscrapHeap;
    uint32_t threadid = GetCurrentThreadId();
    pscrapHeap = nullptr;
    kThreadScrapHeapMap.GetAt(threadid, pscrapHeap);
    if (!pscrapHeap) {
        ScrapHeap* pscrapHeap_2;
        pscrapHeap_2 = new ScrapHeap;
        pscrapHeap = pscrapHeap_2;
        kThreadScrapHeapMap.SetAt(threadid, pscrapHeap_2);
    }
    return pscrapHeap;
}

uint32_t MemoryManager::GetMainThreadMemoryProblemPassSignal() const {
    return uiEmergencyCleanupPassCount;
}

IMemoryHeap* MemoryManager::GetHeapForPointer(const void* apMem) {
    for (uint16_t i = 0; i < iNumHeaps; ++i) {
        IMemoryHeap* pHeap = ppHeaps[i];
        if (pHeap->PointerInHeap(apMem)) {
            return pHeap;
        }
    }
    return nullptr;
}

IMemoryHeap* MemoryManager::GetHeapForPhysicalPointer(const void* apMem) {
    IMemoryHeap* pHeap = nullptr;
    for (uint16_t i = 0; i < static_cast<int32_t>(iNumPhysicalHeaps) && !pHeap; ++i) {
        if (ppPhysicalHeaps[i]->PointerInHeap(apMem)) {
            pHeap = ppPhysicalHeaps[i];
        }
    }
    return pHeap;
}

void* MemoryManager::DoSystemAllocation(size_t aSize, bool abLog) {
    return std::malloc(aSize);
}

void MemoryManager::DoSystemDeallocation(void* apMem, bool abLog) {
    std::free(apMem);
}

void MemoryManager::InitializeStackTrace() {}

bool MemoryManager::GetHeapStats(uint32_t aiIndex, bool abFullBlockInfo, HeapStats* apStats) const {
    uint8_t ucResult = 0;
    IMemoryHeap* pHeap = GetHeapByIndex(aiIndex);
    if (pHeap) {
        ucResult = 1;
        pHeap->GetHeapStats(apStats, abFullBlockInfo);
    }
    return ucResult;
}

bool MemoryManager::GetDefaultHeapStats(uint32_t aiDefaultHeapIndex, bool abFullBlockInfo, HeapStats* apStats) const {
    return GetHeapStats(aiDefaultHeapIndex, abFullBlockInfo, apStats);
}

int32_t MemoryManager::OutputDebugInfo(IMemoryManagerFile* apFile, bool abOutputWalk) {
    return 0;
}

IMemoryHeap* MemoryManager::GetHeapByIndex(uint32_t aiIndex) const {
    IMemoryHeap* pHeap = nullptr;
    if (aiIndex < iNumHeaps) {
        return ppHeaps[aiIndex];
    }
    if ((aiIndex - iNumHeaps) < iNumPhysicalHeaps) {
        return ppPhysicalHeaps[aiIndex - iNumHeaps];
    }
    return pHeap;
}

void MemoryManager::CheckMemoryIntegrity(bool abThreaded) {
    MemoryPool::CheckPools();
    int32_t iCount = iNumHeaps + iNumPhysicalHeaps;
    for (int32_t i = 0; i < iCount; ++i) {
        GetHeapByIndex(i)->CheckHeapIntegrity();
    }
}

bool MemoryManager::GetPhysicalHeapStats(uint32_t aiPhysicalHeapIndex, bool abFullBlockInfo, HeapStats* apStats) const {
    return GetHeapStats(aiPhysicalHeapIndex + iNumHeaps, abFullBlockInfo, apStats);
}

void* MemoryManager::ClearMainThreadMemoryProblemPassSignal() {
    uiEmergencyCleanupPassCount = 0;
    return reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(uiEmergencyCleanupPassCount));
}

void MemoryManager::InitializeStandard() {
    eMemContext = MEM_CONTEXT_STATICVARS;
    bInitialized = true;
}

void MemoryManager::CheckDelayedFreeBlocks() {
    IMemoryHeap* pHeap;
    int32_t i;
    int32_t iCount = MemoryManager::Instance().iNumPhysicalHeaps + MemoryManager::Instance().iNumHeaps;
    for (i = 0; i < iCount; ++i) {
        pHeap = MemoryManager::Instance().GetHeapByIndex(i);
        pHeap->CheckDelayedFreeBlocks();
    }
}

// Drops one user of delayed frees: decrements the count cleanup_1 raises and QDelayedFreeEnabled tests.
void MemoryManager::DisableDelayedFree() {
    InterlockedDecrementInt32(&iDelayFreeBlocksEnabled);
}

char MemoryManager::SetCheckMemOnFree(char abVal) {
    bCheckFreeBlocksOnFree = abVal;
    return abVal;
}

bool MemoryManager::QDelayedFreeEnabled() {
    return (iDelayFreeBlocksEnabled != 0);
}

char MemoryManager::QCheckMemOnFree() {
    return bCheckFreeBlocksOnFree;
}

int32_t cleanup_1() {
    return InterlockedIncrementInt32(&MemoryManager::iDelayFreeBlocksEnabled);
}

AutoMemContext::AutoMemContext(MEM_CONTEXT memContext, bool overridable, const char* apFile, int32_t aiLine) {
    Enter(memContext, overridable, apFile, aiLine);
}

void AutoMemContext::Enter(MEM_CONTEXT memContext, bool overridable, const char* apFile, int32_t aiLine) {
    iOldMemContext = QMemContext();
    SetMemContext(memContext);
}

void AutoMemContext::Leave() {
    SetMemContext(iOldMemContext);
}

MemoryPool* MemoryPool::GetPoolForSize(uint32_t aiBlockSize) {
    MemoryPool* pPool = nullptr;
    if (aiBlockSize < 300) {
        pPool = pMemoryPoolsBySize[((aiBlockSize + 15) & 0xFFFFFFF0u) >> 2];
    }
    else if (aiBlockSize < 512) {
        pPool = pMemoryPoolsBySize[((aiBlockSize + 15) & 0xFFFFFFF0u) >> 2];
        if (!pPool) {
            pPool = pMemoryPoolsBySize[((aiBlockSize + 31) & 0xFFFFFFE0u) >> 2];
        }
    }
    else if (aiBlockSize < 576) {
        pPool = pMemoryPoolsBySize[144];
    }
    return pPool;
}
