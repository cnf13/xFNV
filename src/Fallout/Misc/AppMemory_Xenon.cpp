#include "BSCore/MemoryHeap.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/ZeroOverheadHeap.h"

void MemoryManager::SpecifyMemoryLayout() {
    ourMemset(pHeapsByContextA, 0, sizeof(pHeapsByContextA));
    iNumHeaps = 3;
    // A uint16_t count never overflows, so the size needs no overflow guard.
    ppHeaps = static_cast<IMemoryHeap**>(ourMalloc(iNumHeaps * sizeof(IMemoryHeap*)));

    void* pMem = ourMalloc(sizeof(MemoryHeap));
    MemoryHeap* pDefaultHeap = pMem ? ::new (pMem) MemoryHeap(0xC800000, 0x5500000, "Default Heap", false) : nullptr;
    ppHeaps[0] = pDefaultHeap;
    pHeapsByContextA[MEM_CONTEXT_DEFAULT] = ppHeaps[0];

    pMem = ourMalloc(sizeof(ZeroOverheadHeap));
    ZeroOverheadHeap* pStaticHeap = pMem ? ::new (pMem) ZeroOverheadHeap(0x183800, "Static Heap") : nullptr;
    ppHeaps[1] = pStaticHeap;
    pHeapsByContextA[MEM_CONTEXT_STATICVARS] = ppHeaps[1];
    pHeapsByContextA[MEM_CONTEXT_GAMESETTING] = ppHeaps[1];
    pHeapsByContextA[MEM_CONTEXT_POOL] = ppHeaps[1];

    pMem = ourMalloc(sizeof(MemoryHeap));
    MemoryHeap* pFileHeap = pMem ? ::new (pMem) MemoryHeap(0x4000000, 0x200000, "File Heap", false) : nullptr;
    ppHeaps[2] = pFileHeap;
    pHeapsByContextA[MEM_CONTEXT_BSFILE] = ppHeaps[2];

    iNumPhysicalHeaps = 0;
}
