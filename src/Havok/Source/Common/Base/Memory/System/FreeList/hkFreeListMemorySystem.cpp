#include "Havok/Source/Common/Base/Memory/System/FreeList/hkFreeListMemorySystem.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Memory/Allocator/Checking/hkLeakDetectAllocator.h"
#include "Havok/Source/Common/Base/Memory/Allocator/FreeList/hkFreeListAllocator.h"
#include "Havok/Source/Common/Base/Memory/Allocator/FreeList/hkMemoryBlockServer.h"
#include "Havok/Source/Common/Base/Memory/Allocator/FreeList/SystemMemoryBlockServer/hkSystemMemoryBlockServer.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Allocator/Solver/hkSolverAllocator.h"
#include "Havok/Source/Common/Base/Memory/Allocator/Thread/hkThreadMemory.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/System/hkMemorySystem.h"
#include "Havok/Source/Common/Base/Memory/Util/hkMemUtil.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/System/StackTracer/hkStackTracer.h"
#include "Havok/Source/Common/Base/Thread/CriticalSection/hkCriticalSection.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "SDK/Windows/winbase.h"

#if FNVD_SYSTEM_ALLOC
// xfnv/debugheap/SystemAlloc.h's aligned pair, declared here: src/Havok includes no header outside Havok and the
// SDK.
namespace fnvd::sysalloc {
void* AlignedAllocate(size_t aSize, size_t aAlign);
void AlignedFree(void* apMem);
}  // namespace fnvd::sysalloc

// Every allocator of a router threadInit sets up is this one, so blocks and buffers bypass the free lists, the
// large-block allocator, the thread caches and the solver buffer. 16-aligned, as the free-list allocator's blocks are;
// frees ignore the size; no statistics.
class hkPassThroughAllocator : public hkMemoryAllocator {
public:
	void* blockAlloc(int32_t aiNumBytes) override {
		return fnvd::sysalloc::AlignedAllocate(static_cast<size_t>(aiNumBytes), 16);
	}
	void blockFree(void* apMem, int32_t) override { fnvd::sysalloc::AlignedFree(apMem); }
	void getMemoryStatistics(MemoryStatistics& arStats) override { arStats = MemoryStatistics{}; }
	int32_t getAllocatedSize(const void*, int32_t aiNumBytes) override { return aiNumBytes; }
};

static hkPassThroughAllocator kPassThroughAllocator;
#endif

hkFreeListMemorySystemStorage hkFreeListMemorySystem_Instance{};

// Flag 1: takes the first free thread slot under the lock and gives aRouter that slot's thread memory and the system's
// allocators, the slot's index kept in the router's user data. When every slot is taken this goes on with index 32, one
// past the table, as the game does. Flag 2: gives aRouter a stack from the free-list allocator, the slot's thread
// memory as its temp allocator and the solver allocator.
void hkFreeListMemorySystem::threadInit(hkMemoryRouter& router, const char* name,
	hkFlags<hkMemorySystem::FlagBits, int32_t> flags) {
	if ((flags.get() & 1) != 0) {
		EnterCriticalSection(&m_threadDataLock.m_section);
		int32_t i;
		for (i = 0; i < 32; ++i) {
			if (!m_threadData[i].m_inUse.m_bool) {
				m_threadData[i].m_inUse.m_bool = 1;
				break;
			}
		}
		LeaveCriticalSection(&m_threadDataLock.m_section);
		ThreadData& rThreadData = m_threadData[i];
		rThreadData.m_name = name;
		rThreadData.m_heapThreadMemory.setMemory(m_heapAllocator, 8);
		router.m_temp = nullptr;
		router.m_heap = &rThreadData.m_heapThreadMemory;
		router.m_global = m_heapAllocator;
		router.m_debug = m_systemAllocator;
		router.m_solver = nullptr;
		// The SDK keeps the slot's index in the user-data pointer.
		router.m_userData = reinterpret_cast<void*>(static_cast<intptr_t>(i));
		router.m_stack.m_start.m_storage = nullptr;
		router.m_stack.m_next.m_storage = nullptr;
		router.m_stack.m_end.m_storage = nullptr;
	}
	if ((flags.get() & 2) != 0) {
		int32_t iStackSize = m_frameInfo.m_stackSize;
		ThreadData& rThreadData = m_threadData[static_cast<int32_t>(reinterpret_cast<intptr_t>(router.getUserData()))];
		if (iStackSize) {
			// blockAlloc is called through the vtable. Going through a pointer keeps clang from calling it directly.
			hkMemoryAllocator* pFreeListAllocator = &m_freeListAllocator;
			char* pStack = static_cast<char*>(pFreeListAllocator->blockAlloc(iStackSize));
			router.m_stack.m_start.m_storage = pStack;
			router.m_stack.m_next.m_storage = pStack;
			router.m_stack.m_end.m_storage = pStack + iStackSize;
			rThreadData.m_stackMemoryAllocated = iStackSize;
			rThreadData.m_stackMemory = &router.m_stack;
		}
		router.m_temp = &rThreadData.m_heapThreadMemory;
		router.m_solver = &m_solverAllocator;
	}
#if FNVD_SYSTEM_ALLOC
	aRouter.m_heap = &kPassThroughAllocator;
	aRouter.m_global = &kPassThroughAllocator;
	aRouter.m_debug = &kPassThroughAllocator;
	if ((aFlags.get() & 2) != 0) {
		aRouter.m_temp = &kPassThroughAllocator;
		aRouter.m_solver = &kPassThroughAllocator;
	}
#endif
}

// The members' constructors build the servers and allocators; each thread slot starts unnamed and unused.
hkFreeListMemorySystem::hkFreeListMemorySystem()
	: m_systemAllocator(nullptr), m_freeListMemoryServer(nullptr, 0), m_freeListAllocator(&m_freeListMemoryServer),
	  m_heapAllocator(&m_freeListAllocator), m_threadDataLock(0) {
	for (int32_t i = 0; i < 32; ++i) {
		m_threadData[i].m_name = nullptr;
		m_threadData[i].m_inUse.m_bool = 0;
		m_threadData[i].m_stackMemoryAllocated = 0;
	}
	s_freeListMemory = &m_freeListAllocator;
}

// The members go with their own destructors.
hkFreeListMemorySystem::~hkFreeListMemorySystem() {
	s_freeListMemory = nullptr;
}

hkMemoryRouter* hkFreeListMemorySystem::mainInit(const hkMemorySystem::FrameInfo& info,
	hkFlags<hkMemorySystem::FlagBits, int32_t> flags) {
	m_frameInfo = info;
	char v4 = flags.get();
	if ((flags.get() & 1) != 0) {
		threadInit(m_mainRouter, "main", flags);
		v4 = flags.get();
	}
	if ((v4 & 2) != 0) {
		int32_t m_solverBufferSize = info.m_solverBufferSize;
		if (m_solverBufferSize) {
			int32_t v8 = info.m_solverBufferSize;
			void* v6 = m_systemAllocator->blockAlloc(m_solverBufferSize);
			m_solverAllocator.setBuffer(v6, v8);
		}
	}
	return &m_mainRouter;
}

hkResult hkFreeListMemorySystem::mainQuit(hkFlags<hkMemorySystem::FlagBits, int32_t> flags) {
	int32_t v2 = flags.get();
	if ((flags.get() & 2) != 0 && m_solverAllocator.m_bufferEnd != m_solverAllocator.m_bufferStart) {
		m_systemAllocator->blockFree(m_solverAllocator.m_bufferStart,
			m_solverAllocator.m_bufferEnd - m_solverAllocator.m_bufferStart);
		m_solverAllocator.setBuffer(0, 0);
		v2 = flags.get();
	}
	if ((v2 & 1) != 0) {
		threadQuit(m_mainRouter, v2);
		// garbageCollect is deliberately called through the vtable, as the game does: a pointer keeps clang from
		// calling it directly.
		hkFreeListAllocator* pFreeListAllocator = &m_freeListAllocator;
		pFreeListAllocator->garbageCollect();
	}
	m_systemAllocator = 0;
	return HK_SUCCESS;
}

// Flag 2: frees the router's stack and empties the slot's thread memory; flag 1: empties it again, frees the slot
// and clears the router. The slot's index is the router's user data (threadInit).
void hkFreeListMemorySystem::threadQuit(hkMemoryRouter& router, hkFlags<hkMemorySystem::FlagBits, int32_t> flags) {
	ThreadData& rThreadData = m_threadData[static_cast<int32_t>(reinterpret_cast<intptr_t>(router.getUserData()))];
	// releaseCachedMemory and blockFree are called through the vtable, as the game does: the pointers keep clang from
	// calling them directly.
	hkThreadMemory* pThreadMemory = &rThreadData.m_heapThreadMemory;
	if ((flags.get() & 2) != 0) {
		if (rThreadData.m_stackMemoryAllocated) {
			void* pStart = router.m_stack.m_start.m_storage;
			int32_t iNumBytes =
				static_cast<int32_t>(static_cast<char*>(router.m_stack.m_end.m_storage) - static_cast<char*>(pStart));
			if (iNumBytes) {
				hkMemoryAllocator* pFreeListAllocator = &m_freeListAllocator;
				pFreeListAllocator->blockFree(pStart, iNumBytes);
				router.m_stack.m_start.m_storage = nullptr;
				router.m_stack.m_next.m_storage = nullptr;
				router.m_stack.m_end.m_storage = nullptr;
			}
		}
		pThreadMemory->releaseCachedMemory();
		router.m_temp = nullptr;
		router.m_solver = nullptr;
	}
	if ((flags.get() & 1) != 0) {
		pThreadMemory->releaseCachedMemory();
		rThreadData.m_name = nullptr;
		rThreadData.m_inUse.m_bool = 0;
		hkMemUtil::memSet(&router, 0, sizeof(hkMemoryRouter));
	}
}

// Prints the overview header, then hkMemorySystem's statistics over the heap allocator, the free-list allocator's soft
// limit and the thread memory and stack of each slot in use.
void hkFreeListMemorySystem::printStatistics(hkOstream& ostr) {
	ostr.operator<<("hkFreeListMemorySystem memory overview:\n");
	ostr.operator<<("=======================================\n\n");
	// The SDK's inline constructor: no allocator, empty arrays, zero figures.
	hkMemorySystem::_PrintStatisticsInput kInput;
	kInput.m_heapAllocator = nullptr;
	kInput.m_threadMemories.m_data = nullptr;
	kInput.m_threadMemories.m_size = 0;
	kInput.m_threadMemories.m_capacityAndFlags = hkArrayBase<hkThreadMemory*>::DONT_DEALLOCATE_FLAG;
	kInput.m_stacks.m_data = nullptr;
	kInput.m_stacks.m_size = 0;
	kInput.m_stacks.m_capacityAndFlags = hkArrayBase<hkStackMemory*>::DONT_DEALLOCATE_FLAG;
	kInput.m_sumAllocatedStackMemory = 0;
	// getMemorySoftLimit is called through the vtable: a pointer keeps clang from calling it directly.
	hkFreeListAllocator* pFreeListAllocator = &m_freeListAllocator;
	kInput.m_softLimit = static_cast<int32_t>(pFreeListAllocator->getMemorySoftLimit());
	kInput.m_heapAllocator = m_heapAllocator;
	for (int32_t i = 0; i < 32; ++i) {
		ThreadData& rThreadData = m_threadData[i];
		if (rThreadData.m_inUse.m_bool) {
			// EXE-INLINED none: hkArray<hkThreadMemory*, hkContainerDebugAllocator>::pushBack
			hkMemoryRouter& rRouter = hkMemoryRouter::getInstance();
			if (kInput.m_threadMemories.m_size ==
				(kInput.m_threadMemories.m_capacityAndFlags & hkArrayBase<hkThreadMemory*>::CAPACITY_MASK)) {
				hkArrayUtil::_reserveMore(rRouter.debug(), &kInput.m_threadMemories,
					static_cast<int32_t>(sizeof(hkThreadMemory*)));
			}
			kInput.m_threadMemories.m_data[kInput.m_threadMemories.m_size++] = &rThreadData.m_heapThreadMemory;
			// EXE-INLINED none: hkArray<hkStackMemory*, hkContainerDebugAllocator>::pushBack
			hkMemoryRouter& rRouter2 = hkMemoryRouter::getInstance();
			if (kInput.m_stacks.m_size ==
				(kInput.m_stacks.m_capacityAndFlags & hkArrayBase<hkStackMemory*>::CAPACITY_MASK)) {
				hkArrayUtil::_reserveMore(rRouter2.debug(), &kInput.m_stacks,
					static_cast<int32_t>(sizeof(hkStackMemory*)));
			}
			kInput.m_stacks.m_data[kInput.m_stacks.m_size++] = rThreadData.m_stackMemory;
		}
	}
	_printStatistics(ostr, kInput);
}

hkResult hkFreeListMemorySystem::walkMemory(void (*callback)(void*, uint32_t, hkBool, int32_t, void*), void* param) {
	return m_freeListAllocator.walkMemory(callback, param);
}

void hkFreeListMemorySystem::garbageCollect() {
	(&m_freeListAllocator)->garbageCollect();
}

hkResult hkFreeListMemorySystem::getMemorySnapshot(hkDebugMemorySnapshot& snapshot) {
	return HK_FAILURE;
}

// Adds up the cached bytes of the thread memories and the stack bytes of the slots in use; no solver memory comes
// from the heap.
void hkFreeListMemorySystem::getMemoryStatistics(hkMemorySystem::MemoryStatistics& out) {
	for (int32_t i = 0; i < 32; ++i) {
		ThreadData& rThreadData = m_threadData[i];
		if (rThreadData.m_inUse.m_bool) {
			out.m_sumBytesInThreadMemory += rThreadData.m_heapThreadMemory.getCachedSizeUnchecked();
			out.m_sizeStackAllocatedFromHeap += rThreadData.m_stackMemoryAllocated;
		}
	}
	out.m_solverAllocatedFromHeap = 0;
}

void hkFreeListMemorySystem::setAllocator(hkMemoryAllocator* a1) {
	m_systemAllocator = a1;
	m_freeListMemoryServer.m_system = a1;
}

hkMemoryBlockServer::hkMemoryBlockServer(uint32_t aMinAllocSize) : m_minAllocSize(aMinAllocSize) {}

hkMemoryBlockServer::~hkMemoryBlockServer() {}

// The cached blocks go back to the memory before the base's destructor.
hkThreadMemory::~hkThreadMemory() {
	hkThreadMemory::releaseCachedMemory();
}

hkMemorySystem::~hkMemorySystem() {}
