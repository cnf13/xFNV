#include "BSCore/AbstractHeap.h"

#include "BSCore/BSCoreMessage.h"
#include "BSCore/BSString.h"
#include "BSCore/HeapBlock.h"
#include "BSCore/IMemoryHeap.h"
#include "BSCore/IMemoryManagerFile.h"
#include "BSCore/LogItem.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "platform/Memory.h"

#include <algorithm>

void FreeHeapBlock::Remove() {
    if (pPrevFree) {
        pPrevFree->pNextFree = pNextFree;
    }
    if (pNextFree) {
        pNextFree->pPrevFree = pPrevFree;
    }
    pNextFree = nullptr;
    pPrevFree = nullptr;
}

AbstractHeap::AbstractHeap(uint32_t aiSize, uint32_t aiInitialSize, const char* apName, uint32_t aiPageSize)
  : pName(apName), iDefaultAlignment(kAlignment), MinFreeBlockSize(sizeof(FreeHeapBlock)), iPageSize(aiPageSize),
    MemHeapSize(aiSize), iInitialSize(aiInitialSize), iCurrentSize(aiInitialSize), iWastedMemory(0), iMemAllocated(0),
    iMemAllocatedHigh(0), iBlockMemAllocated(0), pMemHeap(nullptr), iNumBlocks(0), pBlockHead(nullptr),
    pBlockTail(nullptr), iNumFreeBlocks(0), pDelayedFreeBlocksA(nullptr), iNextDelayedFreeIndex(0) {
    InitializeCriticalSection(&CriticalSectionO);
}

AbstractHeap::~AbstractHeap() {
    DeleteCriticalSection(&CriticalSectionO);
}

bool AbstractHeap::PointerInHeap(const void* apPointer) {
    const char* pPointer = static_cast<const char*>(apPointer);
    return pPointer >= pMemHeap && pPointer < pMemHeap + MemHeapSize;
}

void AbstractHeap::CheckHeapIntegrity() {
    int32_t j;
    int32_t i;
    HeapBlock* k;
    FreeHeapBlock* pFreeBlock;
    FreeHeapBlock* pSmallFreeBlock;
    FreeHeapBlock* pLargeFreeBlock;
    EnterCriticalSection(&CriticalSectionO);
    for (i = 0; i < 16; ++i) {
        if (pSubBuckets[i].iCount) {
            for (pFreeBlock = pSubBuckets[i].pFirst->pHead; pFreeBlock; pFreeBlock = pFreeBlock->pNextFree) {
                CheckFreeBlock(pFreeBlock);
            }
        }
    }
    for (j = 0; j < 1024; ++j) {
        for (pSmallFreeBlock = pSmallFreeList[j].pHead; pSmallFreeBlock; pSmallFreeBlock = pSmallFreeBlock->pNextFree) {
            CheckFreeBlock(pSmallFreeBlock);
        }
    }
    for (pLargeFreeBlock = LargeFreeList.pHead; pLargeFreeBlock; pLargeFreeBlock = pLargeFreeBlock->pNextFree) {
        CheckFreeBlock(pLargeFreeBlock);
    }
    for (k = pBlockTail; k; k = k->pPrevious) {
        ;
    }
    LeaveCriticalSection(&CriticalSectionO);
}

void AbstractHeap::GetHeapStats(HeapStats* apStats, bool abFullBlockInfo) {
    EnterCriticalSection(&CriticalSectionO);
    apStats->pHeapName = pName;
    apStats->MemHeapSize = MemHeapSize;
    apStats->MemHeapCommitted = iCurrentSize;
    apStats->iMemAllocatedToBlocks = iMemAllocated;
    apStats->iNumBlocks = iNumBlocks;
    apStats->iNumFreeBlocks = iNumFreeBlocks;
    apStats->iHeapOverhead = 0x20F0;
    apStats->iFreeListOverhead = 0x2000;
    apStats->iFreeListOverhead += 128;
    apStats->iBlockOverhead = kHeader * iNumBlocks;
    apStats->iMemDebugOverhead = 0;
    HeapBlock* pcur_2 = pBlockHead;
    apStats->iMemFreeInBlocks = iMemAllocated - iBlockMemAllocated;
    apStats->iMemUsedInBlocks = iBlockMemAllocated;
    apStats->iSmallestFreeBlock = 0xFFFFFFF;
    apStats->iLargestFreeBlock = 0;
    if (abFullBlockInfo) {
        apStats->iMemFreeInBlocks = 0;
        apStats->iMemUsedInBlocks = 0;
        while (pcur_2) {
            HeapBlock* pcur;
            if ((pcur_2->iMemSize & HeapBlock::IS_FREE) != 0) {
                int32_t isize = pcur_2->iMemSize & HeapBlock::SIZE_MASK;
                apStats->iMemFreeInBlocks += isize + kHeader;
                if (isize < apStats->iSmallestFreeBlock) {
                    apStats->iSmallestFreeBlock = isize;
                }
                if (isize > apStats->iLargestFreeBlock) {
                    apStats->iLargestFreeBlock = isize;
                }
            }
            else {
                apStats->iMemUsedInBlocks += (pcur_2->iMemSize & HeapBlock::SIZE_MASK) + kHeader;
            }
            if (pcur_2 == pBlockTail) {
                pcur = nullptr;
            }
            else {
                // The heap's memory is raw bytes walked as blocks: the next block starts right after this one's header
                // and its data.
                pcur = reinterpret_cast<HeapBlock*>(
                    reinterpret_cast<char*>(pcur_2) + ((pcur_2->iMemSize & HeapBlock::SIZE_MASK) + kHeader)
                );
            }
            pcur_2 = pcur;
        }
        if (!apStats->iNumFreeBlocks) {
            apStats->iSmallestFreeBlock = 0;
        }
    }
    apStats->iTotalFree = apStats->iMemFreeInBlocks + (apStats->MemHeapSize - apStats->iMemAllocatedToBlocks);
    LeaveCriticalSection(&CriticalSectionO);
}

// Writes the bucket count and then, for every bucket, its size and its number of free blocks. The index compare is
// signed after an unsigned division.
int32_t AbstractHeap::OutputBucketInfo(IMemoryManagerFile* apFile) {
    if (!apFile) {
        return 0;
    }
    EnterCriticalSection(&CriticalSectionO);
    uint32_t uiWritten = 0;
    int32_t iNumBuckets = 1025;
    uiWritten += static_cast<uint32_t>(apFile->Write(&iNumBuckets, 4));
    for (int32_t i = 0; i < iNumBuckets; ++i) {
        uint32_t aBucketInfo[2];
        aBucketInfo[0] = iDefaultAlignment * static_cast<uint32_t>(i + 1);
        aBucketInfo[1] = 0;
        const int32_t iBucket = static_cast<int32_t>(aBucketInfo[0] / iDefaultAlignment) - 1;
        FreeHeapBlockList* pList;
        if (iBucket < 0x400) {
            pList = &pSmallFreeList[iBucket];
        }
        else {
            pList = &LargeFreeList;
        }
        for (FreeHeapBlock* pBlock = pList->pHead; pBlock; pBlock = pBlock->pNextFree) {
            ++aBucketInfo[1];
        }
        uiWritten += static_cast<uint32_t>(apFile->Write(aBucketInfo, 8));
    }
    LeaveCriticalSection(&CriticalSectionO);
    return static_cast<int32_t>(uiWritten);
}

// Writes a 24-byte record per block of which only the first, second, third and sixth 32-bit words are initialised; the
// fourth and fifth are never written and go to the file uninitialised on the full-walk path.
int32_t AbstractHeap::OutputBlockInfo(IMemoryManagerFile* apFile, bool abOutputWalk) {
    if (!apFile) {
        return 0;
    }
    EnterCriticalSection(&CriticalSectionO);
    int32_t iWritten = 0;
    if (abOutputWalk) {
        iWritten += apFile->Write(&iNumBlocks, 4u);
        uint32_t auiStats[6];
        auiStats[0] = kHeader;
        HeapBlock* pBlock = pBlockHead;
        for (int32_t i = 0; i < iNumBlocks; ++i) {
            auiStats[1] = pBlock->iMemSize & HeapBlock::SIZE_MASK;
            auiStats[2] = (pBlock->iMemSize & HeapBlock::IS_FREE) != 0;
            auiStats[5] = 0xFFFFFFFF;
            iWritten += apFile->Write(auiStats, 24u);
            // The next block follows this one's header and its memory: the heap's raw bytes, walked untyped.
            pBlock = reinterpret_cast<HeapBlock*>(
                reinterpret_cast<char*>(pBlock) + (pBlock->iMemSize & HeapBlock::SIZE_MASK) + kHeader
            );
        }
    }
    else {
        int32_t iZero = 0;
        iWritten += apFile->Write(&iZero, 4u);
    }
    LeaveCriticalSection(&CriticalSectionO);
    return iWritten;
}

// Blocks live in the heap's raw memory: a block's header sits right before its memory and the next block follows its
// memory, so those addresses are computed on char pointers.
void* AbstractHeap::BaseAllocate(uint32_t aSize) {
    EnterCriticalSection(&CriticalSectionO);
    if (aSize > kMinMemory) {
        aSize = (aSize + iDefaultAlignment - 1) & ~(iDefaultAlignment - 1);
    }
    else {
        aSize = kMinMemory;
    }
    void* pAllocation = nullptr;
    // The index compare is signed after an unsigned division.
    int32_t uiFreeIndex = aSize / iDefaultAlignment - 1;
    FreeHeapBlockList* pBlock;
    if (uiFreeIndex < 0x400) {
        pBlock = &pSmallFreeList[uiFreeIndex];
    }
    else {
        pBlock = &LargeFreeList;
    }
    if (!pBlock->pTail && pBlock != &LargeFreeList) {
        pBlock += MinFreeBlockSize / iDefaultAlignment;
        if (pBlock > pLastSmallList) {
            pBlock = &LargeFreeList;
        }
        else {
            // The compare and the division are signed.
            int32_t iSize = aSize + MinFreeBlockSize;
            int32_t iStart;
            if (iSize > kSubBucketLimit) {
                iStart = -1;
            }
            else {
                iStart = (iSize / static_cast<int32_t>(iDefaultAlignment) - 1) / 64;
            }
            int32_t i;
            for (i = iStart; i < 0x10 && !pBlock->pTail; ++i) {
                if (pSubBuckets[i].iCount) {
                    if (pSubBuckets[i].pFirst > pBlock) {
                        pBlock = pSubBuckets[i].pFirst;
                    }
                    while (!pBlock->pTail &&
                           (i < 0x10 ? pBlock < pSubBuckets[i + 1].pFirst : pBlock < pLastSmallList)) {
                        ++pBlock;
                    }
                }
            }
            if (i == 0x10) {
                pBlock = &LargeFreeList;
            }
        }
    }
    if (pBlock->pTail) {
        for (FreeHeapBlock* pIter = pBlock->pHead; pIter; pIter = pIter->pNextFree) {
            uint32_t uiSize = pIter->iMemSize & HeapBlock::SIZE_MASK;
            uint32_t uiSizeDiff = uiSize - aSize;
            // The compare is signed.
            if (uiSizeDiff && static_cast<int32_t>(uiSizeDiff) < static_cast<int32_t>(MinFreeBlockSize)) {
                continue;
            }
            bool bDecommited = (pIter->iMemSize & HeapBlock::DECOMMITTED) != 0;
            RemoveBlockFromFreeList(pBlock, pIter);
            if (uiSizeDiff) {
                if (bDecommited) {
                    // This block's header and the free block split off after it.
                    uint32_t uiCommitSize = aSize + kHeader + sizeof(FreeHeapBlock);
                    bool bCommitted = platform::Commit(pIter, uiCommitSize);
                    if (!bCommitted) {
                        AddBlockToFreeList(pIter);
                        LeaveCriticalSection(&CriticalSectionO);
                        return nullptr;
                    }
                    pIter->iMemSize &= ~HeapBlock::DECOMMITTED;
                    char acBuffer[0x100];
                    BSsprintf(acBuffer, 0x100, "Committing Entire Free Block: %08X, %08X\r\n", pIter, uiCommitSize);
                    if (BSCoreMessage::pHandler) {
                        BSCoreMessage* pHandler = BSCoreMessage::pHandler;
                        pHandler->Message(BSCoreMessage::MS_PROGFLOW, acBuffer, nullptr, 0, 0);
                    }
                }
                FreeHeapBlock* pNewFree = reinterpret_cast<FreeHeapBlock*>(
                    reinterpret_cast<char*>(pIter) + aSize + kHeader
                );
                uint32_t uiMemorySize = uiSize - (aSize + kHeader);
                pNewFree->pPrevFree = nullptr;
                pNewFree->pNextFree = nullptr;
                pNewFree->Init(uiMemorySize, nullptr);
                pNewFree->pPrevious = pIter;
                if (pIter == pBlockTail) {
                    pBlockTail = pNewFree;
                }
                else {
                    HeapBlock* pNext = reinterpret_cast<HeapBlock*>(
                        reinterpret_cast<char*>(pIter) + (pIter->iMemSize & HeapBlock::SIZE_MASK) + kHeader
                    );
                    pNext->pPrevious = pNewFree;
                }
                ++iNumBlocks;
                AddBlockToFreeList(pNewFree);
                TryAndDefragment(pNewFree);
            }
            else if (bDecommited) {
                bool bCommitted = platform::Commit(pIter, aSize + kHeader);
                if (!bCommitted) {
                    AddBlockToFreeList(pIter);
                    LeaveCriticalSection(&CriticalSectionO);
                    return nullptr;
                }
                pIter->iMemSize &= ~HeapBlock::DECOMMITTED;
                char acBuffer[0x100];
                BSsprintf(
                    acBuffer,
                    0x100,
                    "Committing perfectly matched Free Block: %08X, %08X\r\n",
                    pIter,
                    aSize + kHeader
                );
                if (BSCoreMessage::pHandler) {
                    BSCoreMessage* pHandler = BSCoreMessage::pHandler;
                    pHandler->Message(BSCoreMessage::MS_PROGFLOW, acBuffer, nullptr, 0, 0);
                }
            }
            pIter->iMemSize = (pIter->iMemSize & ~HeapBlock::SIZE_MASK) | aSize;
            pAllocation = reinterpret_cast<char*>(pIter) + kHeader;
            iBlockMemAllocated += (pIter->iMemSize & HeapBlock::SIZE_MASK) + kHeader;
            break;
        }
    }
    if (!pAllocation) {
        uint32_t uiBlockSize = aSize + kHeader;
        uint32_t uiTotalSize = iMemAllocated + uiBlockSize;
        if (iPageSize && uiTotalSize > iCurrentSize && uiTotalSize <= MemHeapSize) {
            iCurrentSize += CreateMorePages(pMemHeap, iCurrentSize, uiBlockSize);
        }
        if (uiTotalSize <= iCurrentSize) {
            HeapBlock* pNewBlock = reinterpret_cast<HeapBlock*>(pMemHeap + iMemAllocated);
            if (iPageSize) {
                bool bCommitted = platform::Commit(pNewBlock, aSize + kHeader);
                if (!bCommitted) {
                    LeaveCriticalSection(&CriticalSectionO);
                    return nullptr;
                }
                char acBuffer[0x100];
                BSsprintf(
                    acBuffer,
                    0x100,
                    "Committing memory on end of heap: %08X, %08X\r\n",
                    pNewBlock,
                    aSize + kHeader
                );
                if (BSCoreMessage::pHandler) {
                    BSCoreMessage* pHandler = BSCoreMessage::pHandler;
                    pHandler->Message(BSCoreMessage::MS_PROGFLOW, acBuffer, nullptr, 0, 0);
                }
            }
            pNewBlock->Init(aSize, nullptr);
            AddBlockToList(pNewBlock);
            iMemAllocated += uiBlockSize;
            if (iMemAllocated > iMemAllocatedHigh) {
                iMemAllocatedHigh = iMemAllocated;
            }
            pAllocation = reinterpret_cast<char*>(pNewBlock) + kHeader;
            iBlockMemAllocated += uiBlockSize;
        }
    }
    LeaveCriticalSection(&CriticalSectionO);
    return pAllocation;
}

void AbstractHeap::BaseFree(void* apPointer) {
    if (!apPointer) {
        return;
    }
    EnterCriticalSection(&CriticalSectionO);
    FreeHeapBlock* pBlock = reinterpret_cast<FreeHeapBlock*>(static_cast<char*>(apPointer) - kHeader);
    uint32_t uiSize = pBlock->iMemSize & HeapBlock::SIZE_MASK;
    iBlockMemAllocated -= uiSize + kHeader;
    AddBlockToFreeList(pBlock);
    TryAndDefragment(pBlock);
    LeaveCriticalSection(&CriticalSectionO);
}

void AbstractHeap::MergeFreeBlocks(FreeHeapBlock* apFree1, FreeHeapBlock* apFree2) {
    RemoveBlockFromFreeList(apFree1);
    RemoveBlockFromFreeList(apFree2);
    if ((apFree2->iMemSize & HeapBlock::DECOMMITTED) != 0) {
        apFree1->iMemSize |= HeapBlock::DECOMMITTED;
    }
    uint32_t uiSize1 = apFree1->iMemSize & HeapBlock::SIZE_MASK;
    uint32_t uiSize2 = apFree2->iMemSize & HeapBlock::SIZE_MASK;
    apFree1->iMemSize = (uiSize1 + uiSize2 + kHeader) | (apFree1->iMemSize & ~HeapBlock::SIZE_MASK);
    if (apFree2 == pBlockTail) {
        pBlockTail = apFree1;
    }
    else {
        reinterpret_cast<HeapBlock*>(
            reinterpret_cast<char*>(apFree2) + (apFree2->iMemSize & HeapBlock::SIZE_MASK) + kHeader
        )
            ->pPrevious = apFree1;
    }
    --iNumBlocks;
    AddBlockToFreeList(apFree1);
}

void AbstractHeap::AddBlockToList(HeapBlock* apNewBlock) {
    if (pBlockTail) {
        apNewBlock->pPrevious = pBlockTail;
        pBlockTail = apNewBlock;
    }
    else {
        pBlockHead = apNewBlock;
        pBlockTail = pBlockHead;
    }
    ++iNumBlocks;
}

// Walks headers in raw heap memory: the next header follows the payload and the header.
void AbstractHeap::TryAndDefragment(FreeHeapBlock* apNewFree) {
    FreeHeapBlock* pIter = apNewFree;
    if (pIter) {
        while (true) {
            HeapBlock* pPrevious = pIter->pPrevious;
            if (!pPrevious || !(pPrevious->iMemSize & HeapBlock::IS_FREE)) {
                break;
            }
            MergeFreeBlocks(static_cast<FreeHeapBlock*>(pPrevious), pIter);
            pIter = static_cast<FreeHeapBlock*>(pPrevious);
        }
    }
    if (pIter) {
        while (pIter != pBlockTail) {
            FreeHeapBlock* pNext = reinterpret_cast<FreeHeapBlock*>(
                reinterpret_cast<char*>(pIter) + (pIter->iMemSize & HeapBlock::SIZE_MASK) + kHeader
            );
            if (!(pNext->iMemSize & HeapBlock::IS_FREE)) {
                break;
            }
            MergeFreeBlocks(
                pIter,
                reinterpret_cast<FreeHeapBlock*>(
                    reinterpret_cast<char*>(pIter) + (pIter->iMemSize & HeapBlock::SIZE_MASK) + kHeader
                )
            );
        }
    }
    CleanEnd();
}

void AbstractHeap::AddBlockToFreeList(FreeHeapBlock* apBlock) {
    FreeHeapBlockList* pBlock;
    FreeHeapBlock* pTail;
    FreeHeapBlock* pNextFree;
    apBlock->pNextFree = nullptr;
    apBlock->pPrevFree = nullptr;
    apBlock->iMemSize |= HeapBlock::IS_FREE;
    int32_t iListIndex = static_cast<int32_t>(((apBlock->iMemSize & HeapBlock::SIZE_MASK) / iDefaultAlignment) - 1);
    if (iListIndex < 1024) {
        pBlock = &pSmallFreeList[iListIndex];
    }
    else {
        pBlock = &LargeFreeList;
    }
    if (pBlock->pTail) {
        pTail = pBlock->pTail;
        apBlock->pPrevFree = pTail;
        pNextFree = pTail->pNextFree;
        apBlock->pNextFree = pNextFree;
        if (pNextFree) {
            pNextFree->pPrevFree = apBlock;
        }
        pTail->pNextFree = apBlock;
        pBlock->pTail = apBlock;
    }
    else {
        pBlock->pHead = apBlock;
        pBlock->pTail = apBlock;
    }
    ++iNumFreeBlocks;
    IncrementSubBucket(apBlock->iMemSize & HeapBlock::SIZE_MASK);
}

void AbstractHeap::RemoveBlockFromFreeList(FreeHeapBlockList* apList, FreeHeapBlock* apBlock) {
    FreeHeapBlock* pnext = apBlock->pNextFree;
    FreeHeapBlock* pprev = apBlock->pPrevFree;
    apBlock->Remove();
    apBlock->iMemSize &= ~HeapBlock::IS_FREE;
    if (apBlock == apList->pHead) {
        apList->pHead = pnext;
    }
    if (apBlock == apList->pTail) {
        apList->pTail = pprev;
    }
    // DecrementSubBucket takes an int; the masked size converts implicitly.
    DecrementSubBucket(apBlock->iMemSize & HeapBlock::SIZE_MASK);
    --iNumFreeBlocks;
}

void AbstractHeap::RemoveBlockFromFreeList(FreeHeapBlock* apBlock) {
    // Unsigned division is followed by a signed index comparison.
    int32_t iIndex = (apBlock->iMemSize & HeapBlock::SIZE_MASK) / iDefaultAlignment - 1;
    FreeHeapBlockList* pList;
    if (iIndex < 0x400) {
        pList = &pSmallFreeList[iIndex];
    }
    else {
        pList = &LargeFreeList;
    }
    RemoveBlockFromFreeList(pList, apBlock);
}

// Frees the tail free blocks whose payload ends exactly at the allocated mark, then gives back whole unused pages. The
// heap is treated as raw memory here: a block's payload starts after its header, and the block's end is the header
// address plus the payload size, compared with pMemHeap + iMemAllocated.
void AbstractHeap::CleanEnd() {
    for (HeapBlock* pcur = pBlockTail; pcur &&
         (pcur->iMemSize & HeapBlock::IS_FREE) != 0 &&
         reinterpret_cast<char*>(&pcur[1]) + (pcur->iMemSize & HeapBlock::SIZE_MASK) == pMemHeap + iMemAllocated;
         pcur = pBlockTail) {
        RemoveBlockFromFreeList(static_cast<FreeHeapBlock*>(pcur));
        iMemAllocated -= (pcur->iMemSize & HeapBlock::SIZE_MASK) + kHeader;
        --iNumBlocks;
        if (pBlockHead == pBlockTail) {
            pBlockTail = nullptr;
            pBlockHead = nullptr;
            break;
        }
        pBlockTail = pcur->pPrevious;
    }
    if (iPageSize) {
        uint32_t ifreeSize = iCurrentSize - iMemAllocated;
        if (iCurrentSize - ifreeSize < iInitialSize) {
            ifreeSize = iCurrentSize - iInitialSize;
        }
        if (ifreeSize >= iPageSize) {
            iCurrentSize -= CleanExtraPages(pMemHeap, iCurrentSize, ifreeSize);
        }
    }
}

// The sub-bucket calculation uses signed comparison and division.
void AbstractHeap::IncrementSubBucket(int32_t aiBucketSize) {
    int32_t iIndex = -1;
    if (aiBucketSize <= kSubBucketLimit) {
        int32_t iBucket = static_cast<int32_t>(
            static_cast<uint32_t>(aiBucketSize / static_cast<int32_t>(iDefaultAlignment)) - 1
        );
        iIndex = iBucket / 64;
    }
    if (iIndex != -1) {
        ++pSubBuckets[iIndex].iCount;
    }
}

// The sub-bucket calculation uses signed comparison and division.
void AbstractHeap::DecrementSubBucket(int32_t aiBucketSize) {
    int32_t iIndex = -1;
    if (aiBucketSize <= kSubBucketLimit) {
        int32_t iBucket = static_cast<int32_t>(
            static_cast<uint32_t>(aiBucketSize / static_cast<int32_t>(iDefaultAlignment)) - 1
        );
        iIndex = iBucket / 64;
    }
    if (iIndex != -1) {
        --pSubBuckets[iIndex].iCount;
    }
}

void AbstractHeap::FreeDelayedFreeBlocks() {
    ;
}

void AbstractHeap::AddDelayedFreeBlock(HeapBlock* apBlock) {
    ;
}

int32_t AbstractHeap::GetLogIndex(LogItem* apLogItem) {
    if (!apLogItem) {
        return -1;
    }
    int32_t iindex = 0;
    while (apLogItem->pPrev) {
        ++iindex;
        apLogItem = apLogItem->pPrev;
    }
    return iindex;
}

void AbstractHeap::CheckFreeBlock(FreeHeapBlock* apBlock) {
    ;
}

void AbstractHeap::InitHeap() {
    pMemHeap = static_cast<char*>(DoHeapAllocation(MemHeapSize, iCurrentSize));
    if (!pMemHeap) {
        std::exit(0);
    }
    pLastSmallList = &pSmallFreeList[1023];
    for (int32_t icnt = 0; icnt < 16; ++icnt) {
        pSubBuckets[icnt].iCount = 0;
        pSubBuckets[icnt].pFirst = &pSmallFreeList[64 * icnt];
    }
    std::memset(pSmallFreeList, 0, sizeof(pSmallFreeList));
    std::memset(&LargeFreeList, 0, sizeof(LargeFreeList));
}

void AbstractHeap::DeleteHeap() {
    for (int32_t i = 0; i < 1024; ++i) {
        while (pSmallFreeList[i].pTail) {
            RemoveBlockFromFreeList(&pSmallFreeList[i], pSmallFreeList[i].pTail);
        }
    }
    while (LargeFreeList.pTail) {
        RemoveBlockFromFreeList(&LargeFreeList, LargeFreeList.pTail);
    }
    DoHeapFree(pMemHeap);
    pMemHeap = nullptr;
    MemHeapSize = 0;
}

// Reports a failed allocation: the block size asked for, then for the given free list its block count, its largest
// block and its total free space, and the machine's available physical memory.
void AbstractHeap::OutputBlockFailure(uint32_t desired_size, FreeHeapBlockList* list) {
    if (BSCoreMessage::pHandler) {
        char apDest[256];
        int32_t uiFreeBlocks = 0;
        uint32_t uiLargestFreeBlock = 0;
        int32_t uiTotalFreeSpace = 0;
        BSsprintf(apDest, 256, "AbstractHeap: Checking list for block size %u\n", desired_size);
        BSCoreMessage::pHandler->Message(BSCoreMessage::MS_PROGFLOW, apDest, nullptr, 0, 0);
        if (list) {
            // Walks the list as raw heap memory: each block's payload starts right after its header.
            for (HeapBlock* block = list->pHead; block;
                 block = reinterpret_cast<HeapBlock*>(
                     reinterpret_cast<char*>(block) + (block->iMemSize & HeapBlock::SIZE_MASK) + kHeader
                 )) {
                ++uiFreeBlocks;
                uint32_t uiBlockSize = block->iMemSize & HeapBlock::SIZE_MASK;
                uiTotalFreeSpace += uiBlockSize;
                if (uiBlockSize > uiLargestFreeBlock) {
                    uiLargestFreeBlock = uiBlockSize;
                }
            }
        }
        // GlobalMemoryStatus's dwAvailPhys: a 32-bit figure, capped (the platform layer gives the 64-bit one).
        uint32_t uiAvailPhys = static_cast<uint32_t>(
            std::min<uint64_t>(platform::MemoryStatus().availablePhysical, UINT32_MAX)
        );
        BSsprintf(
            apDest,
            256,
            "\tNumber of blocks free: %u\n\tLargest free block: %u\n\tTotal space in free blocks: %u\n\tAvailable "
            "global memory: %u\n",
            uiFreeBlocks,
            uiLargestFreeBlock,
            uiTotalFreeSpace,
            uiAvailPhys
        );
        BSCoreMessage::pHandler->Message(BSCoreMessage::MS_PROGFLOW, apDest, nullptr, 0, 0);
    }
}
