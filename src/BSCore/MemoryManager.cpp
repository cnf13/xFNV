#include "BSCore/MemoryManager.h"

#include "BSCore/IMemoryHeap.h"
#include "BSCore/MemoryPool.h"
#include "BSCore/TLSData.h"
#include "BSMain/NiObjectGarbage.h"
#include "BSSystemUtilities/BSSystemUtility.h"
#include "Fallout Shared/TES.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Misc/GarbageCollector.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/BSNiAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiInitOptions.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiStaticDataManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "platform/Memory.h"
#include "platform/System.h"

#include <new>

#if XFNV_SYSTEM_ALLOC
#include "xfnv/debugheap/SystemAlloc.h"
#endif

uint32_t _tls_index;

MemoryManager MemoryManager_Singleton;

// The global operator new and delete are the engine's heap, so memory freed with ourFree or Deallocate (and the
// reverse) stays on one heap. Before the singleton's constructor ran (static initialisation) Allocate and Deallocate
// fall back to malloc and free, as the game does, and a pointer in no pool or heap is freed with free later.
void* operator new(size_t aSize) {
    return MemoryManager_Singleton.Allocate(aSize);
}

void* operator new[](size_t aSize) {
    return MemoryManager_Singleton.Allocate(aSize);
}

void* operator new(size_t aSize, const std::nothrow_t&) noexcept {
    return MemoryManager_Singleton.Allocate(aSize);
}

void* operator new[](size_t aSize, const std::nothrow_t&) noexcept {
    return MemoryManager_Singleton.Allocate(aSize);
}

void operator delete(void* apMem) noexcept {
    MemoryManager_Singleton.Deallocate(apMem);
}

void operator delete[](void* apMem) noexcept {
    MemoryManager_Singleton.Deallocate(apMem);
}

void operator delete(void* apMem, size_t) noexcept {
    MemoryManager_Singleton.Deallocate(apMem);
}

void operator delete[](void* apMem, size_t) noexcept {
    MemoryManager_Singleton.Deallocate(apMem);
}

void operator delete(void* apMem, const std::nothrow_t&) noexcept {
    MemoryManager_Singleton.Deallocate(apMem);
}

void operator delete[](void* apMem, const std::nothrow_t&) noexcept {
    MemoryManager_Singleton.Deallocate(apMem);
}

// Over-aligned forms. They use the layout the game gives its 16-aligned Havok objects: aSize plus the alignment from
// Allocate, moved up to the alignment, the byte before the result holding how far (1 to the alignment; 256 is stored as
// 0). The delete forms read that byte back.
static void* AlignedAllocate(size_t aSize, std::align_val_t aAlign) noexcept {
    const size_t uiAlign = static_cast<size_t>(aAlign);
    if (uiAlign > 256) {
        return nullptr;
    }
    uint8_t* pMemory = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(aSize + uiAlign));
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
    MemoryManager_Singleton.Deallocate(pMemory - uiPadding);
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
    GetThreadTLSData()->eMemContext = MEM_CONTEXT_STATICVARS;
}

void MemoryManager::AddPool(uint32_t aiBlockSize, uint32_t aiPoolSize, const char* apDesc) {
    const MEM_CONTEXT eOldContext = GetThreadTLSData()->eMemContext;
    GetThreadTLSData()->eMemContext = MEM_CONTEXT_POOL;
#if XFNV_64BIT
    // The 64-bit program makes only the pools whose blocks are a multiple of 16 bytes, so that every block is aligned
    // as the platform's allocators align (and holds the free list's two pointers); the other sizes go to the next
    // larger pool (GetPoolForSize rounds up to 16) or to the heap (a departure from the exe).
    if (aiBlockSize % 16 != 0) {
#else
    if (aiBlockSize < 8) {
#endif
        GetThreadTLSData()->eMemContext = eOldContext;
        return;
    }
    bAllowPoolUse = false;
    // Allocates and frees through the singleton, not this.
    void* pMem = MemoryManager_Singleton.Allocate(sizeof(MemoryPool));
    MemoryPool* pPool = pMem ? ::new (pMem) MemoryPool(aiBlockSize, aiPoolSize, apDesc) : nullptr;
    // The pool is read without a null check, as the game does; the delete below checks.
    if ((!pPool->pHeap) && (pPool)) {
        pPool->~MemoryPool();
        MemoryManager_Singleton.Deallocate(pPool);
    }

    bAllowPoolUse = true;
    GetThreadTLSData()->eMemContext = eOldContext;
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
    const MEM_CONTEXT eMemContext = GetThreadTLSData()->eMemContext;
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
    // Pools lie below 4 GB (PlatformReserveHeap); a higher address, only possible on 64 bits, has no entry in the
    // 256-entry table.
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
    // Pools lie below 4 GB (PlatformReserveHeap); a higher address, only possible on 64 bits, has no entry in the
    // 256-entry table.
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
        ScrapHeap* apMemory = static_cast<ScrapHeap*>(MemoryManager_Singleton.Allocate(sizeof(ScrapHeap)));
        if (apMemory) {
            pscrapHeap_2 = ::new (apMemory) ScrapHeap;
        }
        else {
            pscrapHeap_2 = nullptr;
        }
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
    IMemoryHeap* v5 = nullptr;
    for (uint16_t i = 0; i < static_cast<int32_t>(iNumPhysicalHeaps) && !v5; ++i) {
        if (ppPhysicalHeaps[i]->PointerInHeap(apMem)) {
            v5 = ppPhysicalHeaps[i];
        }
    }
    return v5;
}

void* MemoryManager::DoSystemAllocation(size_t aSize, bool abLog) {
    return std::malloc(aSize);
}

void MemoryManager::DoSystemDeallocation(void* apMem, bool abLog) {
    std::free(apMem);
}

void MemoryManager::SpecifyPools() {
    // AddPool takes the pool description as char*; the names are constant text, hence the const_cast.
    AddPool(8, 6291456, const_cast<char*>("Pool 8"));
    AddPool(12, 5242880, const_cast<char*>("Pool 12"));
    AddPool(16, 4194304, const_cast<char*>("Pool 16"));
    AddPool(20, 4194304, const_cast<char*>("Pool 20"));
    AddPool(24, 8388608, const_cast<char*>("Pool 24"));
    AddPool(28, 4194304, const_cast<char*>("Pool 28"));
    AddPool(32, 8388608, const_cast<char*>("Pool 32"));
    AddPool(36, 3145728, const_cast<char*>("Pool 36"));
    AddPool(40, 2097152, const_cast<char*>("Pool 40"));
    AddPool(44, 4194304, const_cast<char*>("Pool 44"));
    AddPool(48, 2097152, const_cast<char*>(aPool48));
    AddPool(52, 3145728, const_cast<char*>("Pool 52"));
    AddPool(56, 1048576, const_cast<char*>(aPool56));
    AddPool(64, 2097152, const_cast<char*>("Pool 64"));
    AddPool(72, 13631488, const_cast<char*>(aPool72));
    AddPool(76, 1048576, const_cast<char*>("Pool 76"));
    AddPool(80, 3145728, const_cast<char*>("Pool 80"));
    AddPool(92, 2097152, const_cast<char*>("Pool 92"));
    AddPool(96, 15728640, const_cast<char*>("Pool 96"));
    AddPool(100, 1048576, const_cast<char*>("Pool 100"));
    AddPool(104, 4194304, const_cast<char*>("Pool 104"));
    AddPool(120, 4194304, const_cast<char*>("Pool 120"));
    AddPool(128, 1048576, const_cast<char*>("Pool 128"));
    AddPool(160, 2097152, const_cast<char*>("Pool 160"));
    AddPool(192, 2097152, const_cast<char*>("Pool 192"));
    AddPool(200, 3145728, const_cast<char*>("Pool 200"));
    AddPool(208, 3145728, const_cast<char*>("Pool 208"));
    AddPool(240, 3145728, const_cast<char*>("Pool 240"));
    AddPool(256, 1048576, const_cast<char*>("Pool 256"));
    AddPool(272, 2097152, const_cast<char*>("Pool 272"));
    AddPool(288, 3145728, const_cast<char*>("Pool 288"));
    AddPool(400, 1048576, const_cast<char*>("Pool 400"));
    AddPool(448, 1048576, const_cast<char*>("Pool 448"));
    AddPool(480, 4194304, const_cast<char*>("Pool 480"));
    AddPool(508, 4194304, const_cast<char*>("Pool 508"));
    AddPool(576, 2097152, const_cast<char*>("Pool 576"));
    AddPool(596, 1048576, const_cast<char*>("Pool 596"));
}

void MemoryManager::InitializeStackTrace() {}

uint32_t MemoryManager::ProcessMemoryProblem(IMemoryHeap* apHeap, int32_t aiMemoryPass, bool* apbAllowSystemAllocs) {
    *apbAllowSystemAllocs = true;
    uint32_t v4 = BSGetCurrentThreadID();
    uint32_t v5 = appMain->uiThreadID;
    switch (aiMemoryPass) {
        case 0: {
            uiReallocOnCrossPageBoundaryPaddingST = 0;
            if (v4 == v5) {
                TES::pInstance->CleanUpUnusedTextures(false);
            }
            return aiMemoryPass + 1;
        }
        case 1: {
            ScrapHeapManager* Singleton = ScrapHeapManager::GetSingleton();
            Singleton->FreeAllBuffers();
            return aiMemoryPass + 1;
        }
        case 2: {
            if (BSFaceGenManager::GetModelCache()) {
                BSFaceGenModelMap* ModelCache = BSFaceGenManager::GetModelCache();
                if ((ModelCache->bHasFaceGenData) && (BSFaceGenManager::TryLockFaceGenAccess())) {
                    if (BSFaceGenModelMap::GetCSEnterCounter() == 1) {
                        BSFaceGenModelMap* v8 = BSFaceGenManager::GetModelCache();
                        v8->UnloadAllEGMAndEGTData(false);
                    }
                    BSFaceGenManager::UnlockFaceGenAccess();
                }
            }
            return aiMemoryPass + 1;
        }
        case 3: {
            goto LABEL_21;
        }
        case 4: {
            goto LABEL_19;
        }
        case 5: {
            if (v4 != v5) {
                return aiMemoryPass + 1;
            }
            GarbageCollector::CanRunGarbageCollector(false);
            if (TES::pInstance->ClearOldestBufferedCell()) {
                --aiMemoryPass;
            }
            else {
                TES::pInstance->ClearAllCells(true, false);
            }
            TES::pInstance->CleanUpUnusedTextures(false);
            GarbageCollector::CanRunGarbageCollector(true);
            GarbageCollector::ClearAll(true);
        LABEL_19:;
            if (ReferenceProcessLists.kUpdateLock.TryLock()) {
                GarbageCollector::ClearAll(true);
                ReferenceProcessLists.kUpdateLock.Unlock();
            }
        LABEL_21:;
            NiObjectGarbage::ClearAll(true);
            break;
        }
        case 6: {
            if (v4 == v5) {
                MemoryPool::FreeUnusedPagesForAllPools();
            }
            return aiMemoryPass + 1;
        }
        case 7: {
            if (*apbAllowSystemAllocs || v4 != v5) {
                goto LABEL_28;
            }
            *apbAllowSystemAllocs = true;
            break;
        }
        case 8: {
        LABEL_28:;
            if (v4 != v5) {
                uiEmergencyCleanupPassCount = 6;
                if (uiReallocOnCrossPageBoundaryPaddingST >= 0x3A98u) {
                    if (!*apbAllowSystemAllocs) {
                        *apbAllowSystemAllocs = true;
                    }
                }
                else {
                    uint32_t TaskThreadID = pIOManager->GetTaskThreadID(0);
                    if (TaskThreadID == BSGetCurrentThreadID()) {
                        pIOManager->PauseThread(0);
                        pIOManager->WaitForThread(0);
                    }
                    uint32_t v10 = pIOManager->GetTaskThreadID(1);
                    if (v10 == BSGetCurrentThreadID()) {
                        pIOManager->PauseThread(1);
                        pIOManager->WaitForThread(1);
                    }
                    BSSleep(1u);
                    ++uiReallocOnCrossPageBoundaryPaddingST;
                    --aiMemoryPass;
                }
            }
            break;
        }
        default: {
            return aiMemoryPass + 1;
        }
    }
    return aiMemoryPass + 1;
}

bool MemoryManager::GetHeapStats(uint32_t aiIndex, bool abFullBlockInfo, HeapStats* apStats) const {
    uint8_t v6 = 0;
    IMemoryHeap* pHeap = GetHeapByIndex(aiIndex);
    if (pHeap) {
        v6 = 1;
        pHeap->GetHeapStats(apStats, abFullBlockInfo);
    }
    return (bool)v6;
}

bool MemoryManager::GetDefaultHeapStats(uint32_t aiDefaultHeapIndex, bool abFullBlockInfo, HeapStats* apStats) const {
    return GetHeapStats(aiDefaultHeapIndex, abFullBlockInfo, apStats);
}

int32_t MemoryManager::OutputDebugInfo(IMemoryManagerFile* apFile, bool abOutputWalk) {
    return 0;
}

IMemoryHeap* MemoryManager::GetHeapByIndex(uint32_t aiIndex) const {
    IMemoryHeap* v3 = nullptr;
    if (aiIndex < ((uint32_t)((int32_t)iNumHeaps))) {
        return ppHeaps[aiIndex];
    }
    if ((aiIndex - iNumHeaps) < ((uint32_t)((int32_t)iNumPhysicalHeaps))) {
        return ppPhysicalHeaps[aiIndex - iNumHeaps];
    }
    return v3;
}

void MemoryManager::CheckMemoryIntegrity(bool abThreaded) {
    MemoryPool::CheckPools();
    int32_t iCount = iNumHeaps + iNumPhysicalHeaps;
    for (int32_t i = 0; i < iCount; ++i) {
        GetHeapByIndex(i)->CheckHeapIntegrity();
    }
}

bool MemoryManager::GetPhysicalHeapStats(uint32_t aiPhysicalHeapIndex, bool abFullBlockInfo, HeapStats* apStats) const {
    return (bool)((uint8_t)GetHeapStats(aiPhysicalHeapIndex + iNumHeaps, abFullBlockInfo, apStats));
}

void* MemoryManager::ClearMainThreadMemoryProblemPassSignal() {
    uiEmergencyCleanupPassCount = (uint32_t)0;
    return reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(uiEmergencyCleanupPassCount));
}

void MemoryManager::InitializeStandard() {
    GetThreadTLSData()->eMemContext = MEM_CONTEXT_STATICVARS;
    bInitialized = true;
}

void MemoryManager::RegisterMemoryManager() {
    NiInitOptions* inited;
    NiInitOptions* v5 = static_cast<NiInitOptions*>(ourMalloc(sizeof(NiInitOptions)));
    if (v5) {
        BSNiAllocator* v3;
        BSNiAllocator* apMemory = static_cast<BSNiAllocator*>(ourMalloc(sizeof(BSNiAllocator)));
        if (apMemory) {
            v3 = ::new (apMemory) BSNiAllocator;
        }
        else {
            v3 = nullptr;
        }
        inited = ::new (v5) NiInitOptions(v3);
    }
    else {
        inited = nullptr;
    }
    NiStaticDataManager::ms_pkInitOptions = inited;
    NiMemManager::_SDMInit();
    BSSystemUtility* v1 = &BSSystemUtility::QInstance();
    v1->Initialize(nullptr, nullptr, 0);
}

MemoryManager& MemoryManager::Instance() {
    return MemoryManager_Singleton;
}

void* MemoryManager::AlignedAllocate16(size_t aSize) {
    uint8_t* pMemory = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(aSize + 0x10));
    const uint8_t ucPadding = static_cast<uint8_t>(0x10 - (reinterpret_cast<uintptr_t>(pMemory) & 0xF));
    pMemory += ucPadding;
    pMemory[-1] = ucPadding;
    return pMemory;
}

void MemoryManager::CheckDelayedFreeBlocks() {
    IMemoryHeap* heapByIndex;
    int32_t i;
    int32_t v2 = MemoryManager_Singleton.iNumPhysicalHeaps + MemoryManager_Singleton.iNumHeaps;
    for (i = 0; i < v2; ++i) {
        heapByIndex = (&MemoryManager_Singleton)->GetHeapByIndex(i);
        heapByIndex->CheckDelayedFreeBlocks();
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
    return (int8_t)bCheckFreeBlocksOnFree;
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
