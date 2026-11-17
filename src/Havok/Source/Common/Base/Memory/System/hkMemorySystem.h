#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Reconstruction.hpp"

class hkDebugMemorySnapshot;
struct hkMemoryPointerInfo;
class hkOstream;
class hkThreadMemory;

// Prints one allocator's figures under apName against aiSoftLimit (0 or 0x7FFFFFFF: none)
// (0xCBC000, usercall): arStats in ecx, aiSoftLimit in eax, the name on the stack (caller-popped). The caller keeps
// its stream in esi, which is passed along in case the helper prints through it.
void printDetailedStatistics(hkOstream& aOstr, const hkMemoryAllocator::MemoryStatistics& arStats, uint32_t aiSoftLimit,
	const char* apName);

// Havok's top-level memory system: it sets up the memory routers of the main thread and of every worker thread and
// answers the debugging queries. The exe's single instance is hkMemorySystem::s_instance. Most slots are abstract
// here; the ones with a body in the exe do nothing.
class hkMemorySystem {
public:
	enum FlagBits : int32_t {
		FLAG_ROUTER = 0x1,
		FLAG_FRAME = 0x2,
		FLAG_ALL = 0x3,
	};

	// What the main thread's router is sized for.
	struct FrameInfo {
		// Fills in the frame's sizes: a 200 KiB stack and a 6 MiB solver buffer.
		FrameInfo();

		int32_t m_stackSize;
		// If set, a buffer is allocated and given to hkSolverAllocator::s_solverAllocator.
		int32_t m_solverBufferSize;
	};

	// The memory statistics slot 15 fills (hk_size_t is four bytes in the exe). The PDB's struct is not laid out in
	// the exe; the SDK's is used.
	struct MemoryStatistics {
		uint32_t m_sizeStackAllocatedFromHeap;
		uint32_t m_sumBytesInThreadMemory;
		uint32_t m_solverAllocatedFromHeap;
	};

	// What _printStatistics prints: the heap, the thread memories' caches and the threads' stacks.
	struct _PrintStatisticsInput {
		~_PrintStatisticsInput();

		hkMemoryAllocator* m_heapAllocator;
		hkArray<hkThreadMemory*> m_threadMemories;
		hkArray<hkStackMemory*> m_stacks;
		int32_t m_sumAllocatedStackMemory;
		int32_t m_softLimit;
	};

	virtual ~hkMemorySystem();

	// Abstract in the exe (the slots 1 to 6 point at the pure-virtual stub), so their stack bytes are not checked.
	virtual hkMemoryRouter* mainInit(const FrameInfo& aFrameInfo, hkFlags<FlagBits, int32_t> aFlags) = 0;
	virtual hkResult mainQuit(hkFlags<FlagBits, int32_t> aFlags) = 0;
	virtual void threadInit(hkMemoryRouter& aRouter, const char* aName, hkFlags<FlagBits, int32_t> aFlags) = 0;
	virtual void threadQuit(hkMemoryRouter& aRouter, hkFlags<FlagBits, int32_t> aFlags) = 0;
	virtual void printStatistics(hkOstream& aStream) = 0;
	virtual hkResult walkMemory(void (*apCallback)(void*, uint32_t, hkBool, int32_t, void*), void* apUserData) = 0;
	virtual void garbageCollect();
	virtual void advanceFrame() {}
	virtual const void* debugFindBaseAddress(const void* p, int32_t nbytes);
	virtual hkResult getMemorySnapshot(hkDebugMemorySnapshot& snapshot);
	virtual void debugLockBaseAddress(const void* baseAddress) {}
	virtual void debugUnlockBaseAddress(const void* baseAddress) {}
	virtual void debugTagAddress(const void* baseAddress, const void* tag) {}
	// The exe's slot 14 (0xCBC440) returns 1 without looking at its two stack words; IDA calls it debugTagAddress,
	// the PDB puts getAllocationCallStack at this position.
	virtual hkResult getAllocationCallStack(const void* ptr, hkMemoryPointerInfo& info);
	virtual void getMemoryStatistics(MemoryStatistics& out) {}

protected:
	// Prints the heap, thread memory, stack, solver and debug memory figures to aOstr (thiscall, ecx unused, ret 8).
	void _printStatistics(hkOstream& ostr, const _PrintStatisticsInput& input);

public:
	// Inferred type: a1: hkMemorySystem* from 0xc682f1 (result of hkFreeListMemorySystem::Construct)
	static void replaceInstance(hkMemorySystem* a1);
	static hkMemorySystem* getInstancePtr();
	// The exe's single instance; the exe calls it out of line (cdecl, ret 0, 0xCBBFC0).
	static hkMemorySystem& getInstance();

	static inline hkMemorySystem* s_instance;
};

inline constexpr fnvd::Vtable hkMemorySystem_vftable{};
