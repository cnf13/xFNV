#include "BSCore/MemoryPool.h"

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSpinLock.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/Windows/winnt.h"
#include "platform/Memory.h"

MemoryPool::MemoryPool(uint32_t aiBlockSize, uint32_t aiPoolSize, const char* apDesc)
  : pDescription(apDesc), pHeap(nullptr), pFreeHead(nullptr), iAlignedBlockSize(aiBlockSize), iHighCommitedPages(0),
    pPageRefCount(nullptr), iNumPages(aiPoolSize >> 12), iPoolSize(aiPoolSize), iNumFreeBlocks(0),
    iNumCommittedPages(0) {
    // Block sizes are rounded up to 4 bytes.
    if (iAlignedBlockSize) {
        iAlignedBlockSize = (iAlignedBlockSize + 3) & 0xFFFFFFFCu;
    }
    PlatformReserveHeap();
    if (pHeap) {
        pPageRefCount = static_cast<uint16_t*>(MemoryManager::Instance().Allocate(sizeof(uint16_t) * iNumPages));
        uint32_t uiBlocksPerPage = 0x1000 / iAlignedBlockSize;
        dword5C = uiBlocksPerPage / 6;
        dword60 = uiBlocksPerPage - dword5C;
        // Every page starts uncommitted.
        for (uint32_t i = 0; i < iNumPages; ++i) {
            pPageRefCount[i] = 0xFFFF;
        }
        AddToLookupArrays();
    }
}

MemoryPool::~MemoryPool() {
    if (pHeap) {
        PlatformReleaseHeap();
        RemoveFromLookupArrays();
    }
    MemoryManager::Instance().Deallocate(pPageRefCount);
}

void* MemoryPool::Allocate(uint32_t requested) {
    CriticalSectionO.Lock("MemoryPool::Allocate");
    if (!pFreeHead) {
        uint32_t inewPageIndex = GetFreePageIndex();
        if (inewPageIndex == -1) {
            if (!--CriticalSectionO.uiLockCount) {
                CriticalSectionO.uiOwningThread = 0;
            }
            return nullptr;
        }
        char* pfirstFreePage = nullptr;
        uint32_t ientriesPerPage = 0x1000 / iAlignedBlockSize;
        if (!PlatformCommitPage(inewPageIndex, pfirstFreePage)) {
            if (!--CriticalSectionO.uiLockCount) {
                CriticalSectionO.uiOwningThread = 0;
            }
            return nullptr;
        }
        pPageRefCount[inewPageIndex] = 0;
        for (uint32_t i = 0; i < ientriesPerPage; ++i) {
            AddFreeBlock(&pfirstFreePage[iAlignedBlockSize * i]);
        }
        if (++iNumCommittedPages > iHighCommitedPages) {
            iHighCommitedPages = iNumCommittedPages;
        }
    }
    void* pmem = nullptr;
    if (pFreeHead) {
        pmem = pFreeHead;
        MemoryPool::FreePoolBlock* ptemp = pFreeHead;
        pFreeHead = ptemp->pNext;
        RemoveFreeBlock(ptemp);
        // The page index is the unsigned distance from the heap base in 4 KiB pages.
        ++pPageRefCount[static_cast<uint32_t>(static_cast<char*>(pmem) - pHeap) >> 12];
    }
    if (!--CriticalSectionO.uiLockCount) {
        CriticalSectionO.uiOwningThread = 0;
    }
    return pmem;
}

void MemoryPool::Free(void* apPointer) {
    if (!apPointer) {
        return;
    }
    CriticalSectionO.Lock("MemoryPool::Free");
    AddFreeBlock(apPointer);
    // The page index is the unsigned distance from the heap base in 4 KiB pages.
    uint32_t uiPageIndex = static_cast<uint32_t>(static_cast<char*>(apPointer) - pHeap) >> 12;
    pPageRefCount[uiPageIndex] = static_cast<uint16_t>(pPageRefCount[uiPageIndex] - 1);
    BSSpinLock* pLock = &CriticalSectionO;
    pLock->uiLockCount = pLock->uiLockCount - 1;
    if (!pLock->uiLockCount) {
        pLock->uiOwningThread = 0;
    }
}

void MemoryPool::PlatformReserveHeap() {
    for (uint32_t i = 1; !pHeap; ++i) {
        // Probes virtual address ranges in 16 MiB steps: a reservation at exactly that address.
        pHeap = static_cast<char*>(platform::Reserve(iPoolSize, reinterpret_cast<void*>(uintptr_t{i} << 24)));
        // pMemoryPoolsByAddress maps each 16 MiB of the first 4 GB to its pool (AddToLookupArrays), so the pool
        // must end below 4 GB. A range past that (which the exe's 32-bit process cannot reserve) is given back and
        // the probe goes on.
        if (pHeap && static_cast<uint64_t>(reinterpret_cast<uintptr_t>(pHeap)) + iPoolSize > 0x100000000ull) {
            platform::Release(pHeap);
            pHeap = nullptr;
        }
        if (i >= 0xFF) {
            break;
        }
    }
}

bool MemoryPool::PlatformCommitPage(uint32_t auiPageIndex, char*& apPageAddress) {
    // The reserved heap is raw storage; committing a page exposes its first free block.
    apPageAddress = pHeap + (auiPageIndex << 12);
    return platform::Commit(apPageAddress, 0x1000);
}

bool MemoryPool::PlatformDecommitPage(uint32_t auiPageIndex, char* apPageAddress) {
    return platform::Decommit(apPageAddress, 0x1000);
}

void MemoryPool::AddToLookupArrays() {
    pMemoryPoolsBySize[iAlignedBlockSize >> 2] = this;
    uint32_t uiCount = iPoolSize >> 24;
    if (iPoolSize % 0x1000000u) {
        ++uiCount;
    }
    // The address lookup is indexed with the high byte of the raw heap address. PlatformReserveHeap keeps the pool
    // below 4 GB, so every index is within the 256 entries.
    const uintptr_t uiFirst = reinterpret_cast<uintptr_t>(pHeap) >> 24;
    for (uint32_t i = 0; i < uiCount; ++i) {
        pMemoryPoolsByAddress[uiFirst + i] = this;
    }
}

void MemoryPool::AddPoolAsBlockSize(uint32_t aiSize) {
    MemoryPool::pMemoryPoolsBySize[aiSize >> 2] = this;
}

void MemoryPool::RemovePoolAsBlockSize(uint32_t aiSize) {
    MemoryPool::pMemoryPoolsBySize[aiSize >> 2] = nullptr;
}

// Walks the free list without doing anything with it.
void MemoryPool::CheckFreeBlocks() {
    CriticalSectionO.Lock("CheckFreeBlocks");
    if (iAlignedBlockSize > 8) {
        for (FreePoolBlock* pBlock = pFreeHead; pBlock; pBlock = pBlock->pNext) {}
    }
    BSSpinLock* pLock = &CriticalSectionO;
    pLock->uiLockCount = pLock->uiLockCount - 1;
    if (!pLock->uiLockCount) {
        pLock->uiOwningThread = 0;
    }
}

void MemoryPool::PlatformReleaseHeap() {
    platform::Release(pHeap);
}

uint32_t MemoryPool::GetFreePageIndex() {
    uint32_t i;
    for (i = 0; i < iNumPages; ++i) {
        if (pPageRefCount[i] == 0xFFFF) {
            return i;
        }
    }
    return static_cast<uint32_t>(-1);
}

void MemoryPool::AddFreeBlock(void* apPointer) {
    FreePoolBlock* pBlock = static_cast<FreePoolBlock*>(apPointer);
    pBlock->pPrevious = nullptr;
    pBlock->pNext = pFreeHead;
    if (pFreeHead) {
        pFreeHead->pPrevious = pBlock;
    }
    pFreeHead = pBlock;
    ++iNumFreeBlocks;
}

void MemoryPool::RemoveFreeBlock(MemoryPool::FreePoolBlock* apFreeBlock) {
    if (apFreeBlock->pPrevious) {
        apFreeBlock->pPrevious->pNext = apFreeBlock->pNext;
    }
    if (apFreeBlock->pNext) {
        apFreeBlock->pNext->pPrevious = apFreeBlock->pPrevious;
    }
    --iNumFreeBlocks;
}

void MemoryPool::FreeUnusedPages() {
    CriticalSectionO.Lock("MemoryPool::FreeUnusedPagesForAllPools()");
    bool bSorted = false;
    for (uint32_t i = 0; i < iNumPages; ++i) {
        if (!pPageRefCount[i]) {
            if (!bSorted) {
                SortFreeBlockList();
                bSorted = true;
            }
            FreePage(i, false);
        }
    }
    BSSpinLock* pLock = &CriticalSectionO;
    pLock->uiLockCount = pLock->uiLockCount - 1;
    if (!pLock->uiLockCount) {
        pLock->uiOwningThread = 0;
    }
}

// The page's blocks are contiguous in the sorted free list: the first block's predecessor is linked to the last block's
// successor. The heap is raw, untyped storage, so its blocks are found by address.
void MemoryPool::FreePage(uint32_t aiPageIndex, bool abSortRequired) {
    char* pPage = pHeap + (aiPageIndex << 12);
    if (abSortRequired) {
        SortFreeBlockList();
    }
    FreePoolBlock* pFirst = reinterpret_cast<FreePoolBlock*>(pPage);
    uint32_t uiBlocksPerPage = 0x1000u / iAlignedBlockSize;
    FreePoolBlock* pLast = reinterpret_cast<FreePoolBlock*>(pPage + (uiBlocksPerPage - 1) * iAlignedBlockSize);
    if (pFirst->pPrevious) {
        pFirst->pPrevious->pNext = pLast->pNext;
    }
    else {
        pFreeHead = pLast->pNext;
    }
    if (pLast->pNext) {
        pLast->pNext->pPrevious = pFirst->pPrevious;
    }
    iNumFreeBlocks -= uiBlocksPerPage;
    PlatformDecommitPage(aiPageIndex, pPage);
    pPageRefCount[aiPageIndex] = 0xFFFF;
    --iNumCommittedPages;
}

// A bottom-up merge sort of the doubly linked list by block address, merging runs of uiInSize blocks per pass until one
// pass makes a single merge. The run counters are compared signed and the pass index unsigned.
MemoryPool::FreePoolBlock* MemoryPool::MergeSortList(FreePoolBlock* apList) {
    int32_t iNumMerges = 0;
    if (!apList) {
        return nullptr;
    }
    uint32_t uiInSize = 1;
    while (iNumMerges != 1) {
        FreePoolBlock* pP = apList;
        apList = nullptr;
        FreePoolBlock* pTail = nullptr;
        iNumMerges = 0;
        while (pP) {
            ++iNumMerges;
            FreePoolBlock* pQ = pP;
            int32_t iPSize = 0;
            for (uint32_t i = 0; i < uiInSize; ++i) {
                ++iPSize;
                pQ = pQ->pNext;
                if (!pQ) {
                    break;
                }
            }
            int32_t iQSize = static_cast<int32_t>(uiInSize);
            while (iPSize > 0 || (iQSize > 0 && pQ)) {
                FreePoolBlock* pE;
                if (!iPSize) {
                    pE = pQ;
                    pQ = pQ->pNext;
                    --iQSize;
                }
                else if (!iQSize || !pQ) {
                    pE = pP;
                    pP = pP->pNext;
                    --iPSize;
                }
                else if (pQ < pP) {
                    pE = pQ;
                    pQ = pQ->pNext;
                    --iQSize;
                }
                else {
                    pE = pP;
                    pP = pP->pNext;
                    --iPSize;
                }
                if (pTail) {
                    pTail->pNext = pE;
                }
                else {
                    apList = pE;
                }
                pE->pPrevious = pTail;
                pTail = pE;
            }
            pP = pQ;
        }
        pTail->pNext = nullptr;
        uiInSize *= 2;
    }
    return apList;
}

void MemoryPool::SortFreeBlockList() {
    if (iNumFreeBlocks > 1) {
        pFreeHead = MergeSortList(pFreeHead);
    }
}

void MemoryPool::RemoveFromLookupArrays() {
    MemoryPool::pMemoryPoolsBySize[iAlignedBlockSize >> 2] = nullptr;
    uint32_t iPoolSize_high = static_cast<uint8_t>(iPoolSize >> 24);
    if (iPoolSize % 0x1000000) {
        ++iPoolSize_high;
    }
    // The by-address table is indexed by the top byte of the pool's heap address: the address as a number.
    uintptr_t uiAddressIndex = reinterpret_cast<uintptr_t>(pHeap) >> 24;
    for (uint32_t i = 0; i < iPoolSize_high; ++i) {
        *(&MemoryPool::pMemoryPoolsByAddress[i] + uiAddressIndex) = nullptr;
    }
}

// Fills a zeroed local and copies it out whole.
MemoryPoolStats* MemoryPool::GetStats(MemoryPoolStats* apResult) const {
    MemoryPoolStats kStats;
    std::memset(&kStats, 0, sizeof(kStats));
    kStats.pDesc = pDescription;
    kStats.iOverhead = 2 * iNumPages + 128;
    kStats.iBlockSize = iAlignedBlockSize;
    kStats.iReservedMem = iPoolSize;
    kStats.iReservedPages = iNumPages;
    kStats.iCommittedMem = iNumCommittedPages << 12;
    kStats.iCommittedPages = iNumCommittedPages;
    kStats.iHighCommittedPages = iHighCommitedPages;
    kStats.iFreeBlocks = iNumFreeBlocks;
    kStats.iUsedBlocks = kStats.iCommittedMem / iAlignedBlockSize - iNumFreeBlocks;
    *apResult = kStats;
    return apResult;
}

void MemoryPool::InitPools() {
    for (MemoryPool*& rpPool : pMemoryPoolsBySize) {
        rpPool = nullptr;
    }
    for (MemoryPool*& rpPool : pMemoryPoolsByAddress) {
        rpPool = nullptr;
    }
}

// The lock is a function-local BSCriticalSection built on first use, as in FreeUnusedPagesForAllPools.
void MemoryPool::CheckPools() {
    static BSCriticalSection localCriticalSection;
    EnterCriticalSection(&localCriticalSection.CriticalSection);
    for (MemoryPool* pcurrentPool : MemoryPool::pMemoryPoolsBySize) {
        if (pcurrentPool) {
            pcurrentPool->CheckFreeBlocks();
        }
    }
    LeaveCriticalSection(&localCriticalSection.CriticalSection);
}

// Reloads the pool from the table before each use.
void MemoryPool::SortFreeBlocks(int32_t aiIndex) {
    if (pMemoryPoolsBySize[aiIndex]) {
        pMemoryPoolsBySize[aiIndex]->CriticalSectionO.Lock("SortFreeBlocks");
        pMemoryPoolsBySize[aiIndex]->SortFreeBlockList();
        BSSpinLock* pLock = &pMemoryPoolsBySize[aiIndex]->CriticalSectionO;
        pLock->uiLockCount = pLock->uiLockCount - 1;
        if (!pLock->uiLockCount) {
            pLock->uiOwningThread = 0;
        }
    }
}

void MemoryPool::DeletePools() {
    for (MemoryPool*& rpPool : MemoryPool::pMemoryPoolsBySize) {
        delete rpPool;
        rpPool = nullptr;
    }
}

// The lock, the function-local BSCriticalSection localCriticalSection, is built on first use.
void MemoryPool::FreeUnusedPagesForAllPools() {
    static BSCriticalSection localCriticalSection;
    EnterCriticalSection(&localCriticalSection.CriticalSection);
    for (MemoryPool* pPool : MemoryPool::pMemoryPoolsBySize) {
        if (pPool) {
            pPool->FreeUnusedPages();
        }
    }
    LeaveCriticalSection(&localCriticalSection.CriticalSection);
}
