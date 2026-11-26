#pragma once

#include "BSCore/BSMap.h"

#include <stdint.h>

class IMemoryHeap;
class IMemoryManagerFile;
class ScrapHeap;

// What one heap reports about its memory: its name, the sizes it reserved and committed, and its counters of blocks and
// overhead. AbstractHeap::GetHeapStats compares the smallest and largest free block signed.
struct HeapStats {
    const char* pHeapName;
    uint32_t MemHeapSize;
    uint32_t MemHeapCommitted;
    int32_t iMemAllocatedToBlocks;
    int32_t iNumBlocks;
    int32_t iNumFreeBlocks;
    int32_t iMemFreeInBlocks;
    int32_t iMemUsedInBlocks;
    int32_t iSmallestFreeBlock;
    int32_t iLargestFreeBlock;
    int32_t iHeapOverhead;
    int32_t iFreeListOverhead;
    int32_t iBlockOverhead;
    int32_t iMemDebugOverhead;
    uint32_t iTotalFree;
};

// The heap context an allocation is charged to, per thread.
enum MEM_CONTEXT : int32_t {
    MEM_CONTEXT_STATICVARS,
    MEM_CONTEXT_EXTERNAL,
    MEM_CONTEXT_DEBUG,
    MEM_CONTEXT_POOL,
    MEM_CONTEXT_RENDERER_OVERHEAD,
    MEM_CONTEXT_SHADER_OVERHEAD,
    MEM_CONTEXT_TASK_OVERHEAD,
    MEM_CONTEXT_EFFECTS_OVERHEAD,
    MEM_CONTEXT_STRING_TABLES,
    MEM_CONTEXT_GAMESETTING,
    MEM_CONTEXT_GAME_OVERHEAD,
    MEM_CONTEXT_AUDIO,
    MEM_CONTEXT_FONTS,
    MEM_CONTEXT_INTERFACE,
    MEM_CONTEXT_LOCALMAP,
    MEM_CONTEXT_GAMEBRIO,
    MEM_CONTEXT_HAVOK,
    MEM_CONTEXT_SAVELOAD,
    MEM_CONTEXT_LOADERS,
    MEM_CONTEXT_ARCHIVES,
    MEM_CONTEXT_MOVIEPLAYER,
    MEM_CONTEXT_SCRIPTS,
    MEM_CONTEXT_MASTER_FILE,
    MEM_CONTEXT_BSFILE,
    MEM_CONTEXT_FILESYSTEM,
    MEM_CONTEXT_WORLD_SG,
    MEM_CONTEXT_CELLS,
    MEM_CONTEXT_LAND,
    MEM_CONTEXT_GRASS,
    MEM_CONTEXT_WATER,
    MEM_CONTEXT_TREEART,
    MEM_CONTEXT_TREESYSTEM,
    MEM_CONTEXT_SHADOWS,
    MEM_CONTEXT_SKY,
    MEM_CONTEXT_LODMANAGER,
    MEM_CONTEXT_LODLANDSCAPE,
    MEM_CONTEXT_LODBUILDINGS,
    MEM_CONTEXT_LODTREES,
    MEM_CONTEXT_SHADERS,
    MEM_CONTEXT_IMAGE_SPACE,
    MEM_CONTEXT_HAIRSHADER,
    MEM_CONTEXT_PRECIPITATION,
    MEM_CONTEXT_PROCESSES,
    MEM_CONTEXT_COMBAT_AI,
    MEM_CONTEXT_GLOBAL_LISTS,
    MEM_CONTEXT_PATHING,
    MEM_CONTEXT_NAVMESH,
    MEM_CONTEXT_TES_DEFAULT_OBJECTS,
    MEM_CONTEXT_TES_FORMS,
    MEM_CONTEXT_TES_REFERENCES,
    MEM_CONTEXT_TES_ACTOR,
    MEM_CONTEXT_ANIMATION,
    MEM_CONTEXT_PLAYER,
    MEM_CONTEXT_DIALOGUE,
    MEM_CONTEXT_INVENTORY,
    MEM_CONTEXT_FACEGEN_SYSTEM,
    MEM_CONTEXT_FACEGEN_EGM,
    MEM_CONTEXT_FACEGEN_EGT,
    MEM_CONTEXT_FACEGEN_GEOM,
    MEM_CONTEXT_UNKNOWN,
    MEM_CONTEXT_UNKNOWN_DIRECTX,
    MEM_CONTEXT_DEBUG_CONTEXT_A,
    MEM_CONTEXT_DEBUG_CONTEXT_B,
    MEM_CONTEXT_DEBUG_CONTEXT_C,
    MEM_CONTEXT_DEBUG_CONTEXT_D,
    MEM_CONTEXT_TOTAL,
    MEM_CONTEXT_DEFAULT = MEM_CONTEXT_TOTAL,
    MEM_CONTEXT_COUNT,
};

// The engine's heap. Memory the engine may free (or that it allocated) must go through Allocate/Deallocate: the
// engine's global operator new/delete route here (MemoryManager.cpp defines them so). Only some of the fields are
// declared.
class MemoryManager {
public:
    MemoryManager();
    ~MemoryManager();

    // Every block is 16-aligned, so the blocks the exe aligns to 16 by hand (over-allocating and keeping the padding
    // length in the byte before the object) are plain allocations here.
    void* Allocate(size_t aSize);
    void Deallocate(void* apMem);
    // Resizes apMem to aSize bytes.
    void* Reallocate(void* apMem, size_t aSize);
    // Physical (write-combined) memory; none on PC: AllocatePhysical returns null and DeallocatePhysical does nothing.
    void* AllocatePhysical(size_t aSize, uint32_t aiAttributes);
    void DeallocatePhysical(void* apMem, uint32_t aiAttributes, size_t aiSize);

    // Allocate with the extra arguments of the engine's allocation macro (file, line, function), which it ignores.
    void* Allocate(size_t aSize, const char* apFile, const int32_t aiLine, const char* apFunction);
    // Resizes apMem with the allocation macro's extra arguments, which it ignores.
    void* Reallocate(void* apMem, size_t aSize, const char* apFile, const int32_t aiLine, const char* apFunction);

    // The calling thread's scrap heap, created on first use.
    ScrapHeap* GetThreadScrapHeap();

    // Number of emergency cleanup passes run when memory ran short.
    uint32_t GetMainThreadMemoryProblemPassSignal() const;
    // Size of apPtr's block: the pool's block size, else the owning heap's answer, else the CRT's _msize.
    size_t Size(const void* apPtr);
    // The heap among ppHeaps whose range holds apMem, or null.
    IMemoryHeap* GetHeapForPointer(const void* apMem);
    IMemoryHeap* GetHeapForPhysicalPointer(const void* apMem);
    // malloc; abLog is unused on PC.
    void* DoSystemAllocation(size_t aSize, bool abLog);
    void DoSystemDeallocation(void* apMem, bool abLog);
    // Creates a pool of aiBlockSize-byte blocks; drops it when its memory could not be reserved.
    void AddPool(uint32_t aiBlockSize, uint32_t aiPoolSize, const char* apDesc);

    void Initialize();
    // Creates the default, static and file heaps and maps the heap contexts to them.
    void SpecifyMemoryLayout();
    void SpecifyPools();
    // Empty on PC.
    void InitializeStackTrace();
    // Runs one emergency cleanup pass; returns the next pass.
    uint32_t ProcessMemoryProblem(IMemoryHeap* apHeap, int32_t aiMemoryPass, bool* apbAllowSystemAllocs);
    bool GetHeapStats(uint32_t aiIndex, bool abFullBlockInfo, HeapStats* apStats) const;
    bool GetDefaultHeapStats(uint32_t aiDefaultHeapIndex, bool abFullBlockInfo, HeapStats* apStats) const;
    int32_t OutputDebugInfo(IMemoryManagerFile* apFile, bool abOutputWalk);
    IMemoryHeap* GetHeapByIndex(uint32_t aiIndex) const;
    // Checks the pools, then every heap. abThreaded is unused on PC.
    void CheckMemoryIntegrity(bool abThreaded);
    bool GetPhysicalHeapStats(uint32_t aiPhysicalHeapIndex, bool abFullBlockInfo, HeapStats* apStats) const;
    void* ClearMainThreadMemoryProblemPassSignal();
    void InitializeStandard();
    void RegisterMemoryManager();

    static MemoryManager& Instance() {
        return s_Instance;
    }
    static void CheckDelayedFreeBlocks();
    // Drops one user of delayed frees; the count is what QDelayedFreeEnabled tests.
    static void DisableDelayedFree();
    // Sets the check-memory-on-free flag; returns abVal.
    static char SetCheckMemOnFree(char abVal);
    // Whether delayed frees are enabled: the count DisableDelayedFree drops is above 0.
    static bool QDelayedFreeEnabled();
    // The check-memory-on-free flag.
    static char QCheckMemOnFree();

    bool bInitialized;
    uint16_t iNumHeaps;
    uint16_t iNumPhysicalHeaps;
    IMemoryHeap** ppHeaps;
    // Indexed by MEM_CONTEXT (MEM_CONTEXT_COUNT entries); MEM_CONTEXT_DEFAULT is the default heap.
    IMemoryHeap* pHeapsByContextA[66];
    // The threads' scrap heaps; the constructor builds the map with 37 buckets.
    BSMap<uint32_t, ScrapHeap*> kThreadScrapHeapMap;
    IMemoryHeap** ppPhysicalHeaps;
    bool bSpecialHeaps;
    bool bAllowPoolUse;
    uint32_t iSysAllocBytes;
    uint32_t iMallocBytes;
    uint32_t uiEmergencyCleanupPassCount;

    static inline char bCheckFreeBlocksOnFree;
    static inline int32_t iDelayFreeBlocksEnabled;
    // The calling thread's heap context, which QMemContext/SetMemContext read and write.
    static inline thread_local MEM_CONTEXT eMemContext = MEM_CONTEXT_UNKNOWN;

protected:
    // Constructed before every other object and destroyed after them: an object's destructor that frees heap memory
    // after this one's would pass it to free (the !bInitialized path in Deallocate).
    static MemoryManager s_Instance;
};

// The calling thread's memory context, MemoryManager::eMemContext (the exe's inlined sites read and write it directly).
// Defined in EffectItem.cpp.
MEM_CONTEXT QMemContext();
// Sets the calling thread's memory context to aeContext. Defined in EffectItem.cpp.
void SetMemContext(MEM_CONTEXT aeContext);
int32_t cleanup_1();

// Bethesda's heap-context guard: charges the scope's allocations to one context and restores the thread's previous
// context on exit. The constructor saves the old context and stores the new one, the destructor stores the saved one
// back; both access MemoryManager::eMemContext directly, not through QMemContext/SetMemContext.
class AutoMemContext {
public:
    // A default-constructed AutoMemContext holds the current context, which the destructor restores unchanged.
    AutoMemContext() : iOldMemContext(MemoryManager::eMemContext) {}

    explicit AutoMemContext(MEM_CONTEXT aeMemContext) {
        iOldMemContext = MemoryManager::eMemContext;
        MemoryManager::eMemContext = aeMemContext;
    }

    AutoMemContext(const AutoMemContext&) = delete;
    AutoMemContext(MEM_CONTEXT memContext, bool overridable, const char* apFile, int32_t aiLine);
    ~AutoMemContext() {
        MemoryManager::eMemContext = iOldMemContext;
    }

    AutoMemContext& operator=(const AutoMemContext&) = delete;
    void Enter(MEM_CONTEXT memContext, bool overridable, const char* apFile, int32_t aiLine);
    void Leave();

    MEM_CONTEXT iOldMemContext;
};

// The pool names MemoryManager::SpecifyPools passes to the heap ("Pool NN" and its NUL).
using PoolNameText = const char[8];
inline PoolNameText aPool72{"Pool 72"};
inline PoolNameText aPool56{"Pool 56"};
inline PoolNameText aPool48{"Pool 48"};

// Compared unsigned against 0x3A98.
inline uint32_t uiReallocOnCrossPageBoundaryPaddingST;

inline MEM_CONTEXT QMemContext() {
    return MemoryManager::eMemContext;
}

inline void SetMemContext(MEM_CONTEXT aeContext) {
    MemoryManager::eMemContext = aeContext;
}
