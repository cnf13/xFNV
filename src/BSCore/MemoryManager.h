#pragma once

#include "BSCore/BSMap.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class IMemoryHeap;
class IMemoryManagerFile;
class ScrapHeap;

// What one heap reports about its memory: its name, the sizes it reserved and committed, and its counters of blocks
// and overhead. Names and signedness are the PDB's (AbstractHeap::GetHeapStats, 0xAAA0E0, compares the smallest and
// largest free block signed).
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

// The engine's heap. Memory the exe may free (or that the exe allocated) must go through Allocate/Deallocate: the
// engine's global operator new/delete route here (natively too: MemoryManager.cpp defines them so).
// Only the fields the reconstructed functions read are declared.
class MemoryManager {
public:
	MemoryManager();
	~MemoryManager();

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

	// The calling thread's scrap heap, created on first use. thiscall, ret 0, 0xAA42E0.
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
	// Empty on PC: the exe calls the shared empty thiscall function 0x483710 here; the PDB name
	// is assumed from MemoryManager's protected methods (the one between SpecifyPools and the memory-problem code).
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

	static MemoryManager& Instance();
	// Allocates aSize bytes aligned to 16; the byte before the returned pointer holds the padding length.
	static void* AlignedAllocate16(size_t aSize);
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
	uint8_t padding01[1];
	uint16_t iNumHeaps;
	uint16_t iNumPhysicalHeaps;
	uint8_t padding06[2];
	IMemoryHeap** ppHeaps;
	// Indexed by MEM_CONTEXT (MEM_CONTEXT_COUNT entries, declared below); MEM_CONTEXT_DEFAULT is the default heap.
	IMemoryHeap* pHeapsByContextA[66];
	// The threads' scrap heaps (the constructor, 0xAA3880, builds the map with 37 buckets and the
	// BSMap<uint32_t, ScrapHeap*> vtable 0x10A25F8).
	BSMap<uint32_t, ScrapHeap*> kThreadScrapHeapMap;
	IMemoryHeap** ppPhysicalHeaps;
	bool bSpecialHeaps;
	bool bAllowPoolUse;
	uint8_t padding12A[2];
	uint32_t iSysAllocBytes;
	uint32_t iMallocBytes;
	uint32_t uiEmergencyCleanupPassCount;

	static inline char bCheckFreeBlocksOnFree;
	static inline int32_t iDelayFreeBlocksEnabled;
};

// The heap context an allocation is charged to, per thread. The PDB's order; the exe confirms
// MEM_CONTEXT_INTERFACE = 13 (Tile::UpdateTile) and MEM_CONTEXT_INVENTORY = 54 (InventoryChanges::GetBestArmor).
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

// The engine's CRT free.
void ourFree(void* apPtr);
// The engine's CRT malloc.
void* ourMalloc(uint32_t aSize);
// The engine's CRT realloc (defined in ExtraDataList.cpp, 0x42F5D0).
void* ourRealloc(void* apPtr, uint32_t aSize);
// The engine's zeroing allocator.
void* ourCalloc(uint32_t aNum, uint32_t aSize);
// The engine's memset wrapper (it calls the CRT memset and returns its result; cdecl, 0x403D30).
void* ourMemset(void* apDest, int32_t aiValue, uint32_t aCount);
// The engine's memcpy wrapper (it calls the CRT memcpy and returns its result; cdecl, 0x401460, defined in
// BGSDehydration.cpp as in the PDB).
void* ourMemcpy(void* apDest, const void* apSrc, size_t aCount);
// Sets the calling thread's memory context to aeContext (IDA: Tls::SetHeapIndex; cdecl, ret 0, 0x404F30).
void SetMemContext(MEM_CONTEXT aeContext);
// The engine's aligned allocator: aSize bytes aligned to acAlignment.
void* ourMemalign(uint8_t acAlignment, size_t aSize);
int32_t cleanup_1();

inline int32_t& GetThreadMemContext() {
	static thread_local int32_t iMemContext = MEM_CONTEXT_STATICVARS;
	return iMemContext;
}
// The non-hybrid build keeps an equivalent per-thread flag word.
inline uint32_t& GetThreadSpecificFlags() {
	static thread_local uint32_t uiFlags = 0;
	return uiFlags;
}

// Bethesda's heap-context guard: charges the scope's allocations to one context and restores the thread's previous
// context on exit. The constructor saves the old context and stores the new one, the destructor stores the saved one
// back. Some exe functions inline both (Tile::UpdateTile); others call the out-of-line constructor and destructor
// (0x404EB0/0x404EE0, through Enter/Leave), whose file, line and flag arguments are ignored. The reads and writes are
// the same either way.
class AutoMemContext {
public:
	// The local the exe constructs in place (FNVD_INLINED_LOCAL + FNVD_RECONSTRUCT): C++ default-constructs it and
	// the reconstruct runs the destructor first, so it holds the current context, which the destructor restores
	// unchanged.
	AutoMemContext() : iOldMemContext(static_cast<MEM_CONTEXT>(GetThreadMemContext())) {}

	explicit AutoMemContext(MEM_CONTEXT aeMemContext) {
		iOldMemContext = static_cast<MEM_CONTEXT>(GetThreadMemContext());
		GetThreadMemContext() = aeMemContext;
	}

	AutoMemContext(const AutoMemContext&) = delete;
	AutoMemContext(MEM_CONTEXT memContext, bool overridable, const char* apFile, int32_t aiLine);
	~AutoMemContext() { GetThreadMemContext() = iOldMemContext; }

	AutoMemContext& operator=(const AutoMemContext&) = delete;
	void Enter(MEM_CONTEXT memContext, bool overridable, const char* apFile, int32_t aiLine);
	void Leave();

	MEM_CONTEXT iOldMemContext;
};

// MemoryManager::Singleton in the engine.
extern MemoryManager MemoryManager_Singleton;

// The pool names MemoryManager::SpecifyPools passes to the heap (the exe's own strings, "Pool NN" and its NUL). IDA
// names; not in the PDB.
using PoolNameText = const char[8];
inline PoolNameText aPool72{"Pool 72"};
inline PoolNameText aPool56{"Pool 56"};
inline PoolNameText aPool48{"Pool 48"};

// IDA name; not in the PDB. The exe compares it unsigned (cmp dword [0x11DE70C], 0x3A98; jae, 0x866C37).
inline uint32_t uiReallocOnCrossPageBoundaryPaddingST;
inline uint32_t data_11DE710;
