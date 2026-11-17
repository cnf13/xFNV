#pragma once

#include "Havok/Source/Common/Base/Memory/Allocator/Checking/hkLeakDetectAllocator.h"
#include "Havok/Source/Common/Base/Memory/Allocator/FreeList/SystemMemoryBlockServer/hkSystemMemoryBlockServer.h"
#include "Havok/Source/Common/Base/Memory/Allocator/FreeList/hkFreeListAllocator.h"
#include "Havok/Source/Common/Base/Memory/Allocator/Solver/hkSolverAllocator.h"
#include "Havok/Source/Common/Base/Memory/Allocator/Thread/hkThreadMemory.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/System/hkMemorySystem.h"
#include "Havok/Source/Common/Base/Thread/CriticalSection/hkCriticalSection.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

class hkDebugMemorySnapshot;

// Havok's free-list memory system, the one the game installs. The slots below are its overrides. The layout is the
// PDB's (13188 bytes): the exe's constructor (0xCD0A60) steps m_threadData by 312 bytes and builds the lock at
// 0x336C, and threadInit (0xCD0830) stores the thread's name at 0x128 of its entry.
class hkFreeListMemorySystem : public hkMemorySystem {
public:
	// What the system keeps per thread (the exe's table holds 32 of them): the thread's heap allocator, its router
	// and name, and the stack allocator it was given.
	struct ThreadData {
		hkThreadMemory m_heapThreadMemory;
		hkMemoryRouter* m_memoryRouter;
		const char* m_name;
		hkBool m_inUse;
		// The bytes of stack memory the thread has taken (the PDB's hk_size_t).
		uint32_t m_stackMemoryAllocated;
		hkStackMemory* m_stackMemory;
	};

	hkFreeListMemorySystem();
	~hkFreeListMemorySystem() override;

	hkMemoryRouter* mainInit(const hkMemorySystem::FrameInfo& info,
		hkFlags<hkMemorySystem::FlagBits, int32_t> flags) override;
	// Thiscall, ret 4, 0xCD07B0.
	hkResult mainQuit(hkFlags<hkMemorySystem::FlagBits, int32_t> flags) override;
	// Thiscall, ret 0xC, 0xCD0830.
	void threadInit(hkMemoryRouter& router, const char* name,
		hkFlags<hkMemorySystem::FlagBits, int32_t> flags) override;
	// Thiscall, ret 8, 0xCD0940.
	void threadQuit(hkMemoryRouter& router, hkFlags<hkMemorySystem::FlagBits, int32_t> flags) override;
	// Thiscall, ret 4, 0xCD0B30.
	void printStatistics(hkOstream& ostr) override;
	hkResult walkMemory(void (*callback)(void*, uint32_t, hkBool, int32_t, void*), void* param) override;
	void garbageCollect() override;
	// No snapshots: always HK_FAILURE (1).
	hkResult getMemorySnapshot(hkDebugMemorySnapshot& snapshot) override;
	// Thiscall, ret 4, 0xCD06F0.
	void getMemoryStatistics(hkMemorySystem::MemoryStatistics& out) override;

	void setAllocator(hkMemoryAllocator* a1);

	hkMemoryAllocator* m_systemAllocator;
	hkSystemMemoryBlockServer m_freeListMemoryServer;
	hkFreeListAllocator m_freeListAllocator;
	hkLeakDetectAllocator m_leakDetectAllocator;
	hkMemorySystem::FrameInfo m_frameInfo;
	hkMemoryAllocator* m_heapAllocator;
	hkMemoryRouter m_mainRouter;
	hkSolverAllocator m_solverAllocator;
	ThreadData m_threadData[32];
	hkCriticalSection m_threadDataLock;

	// The game's free-list memory system, which bhkWorld::InitSystem installs and
	// bhkWorld::QuitSystem tears down.
	static inline hkFreeListMemorySystem* pSingleton;
};

// The storage of hkFreeListMemorySystem_Instance: a union, so that
// nothing constructs or destroys the system at start-up or exit; bhkWorld::InitSystem placement-constructs it.
union hkFreeListMemorySystemStorage {
	constexpr hkFreeListMemorySystemStorage() {}
	~hkFreeListMemorySystemStorage() {}

	hkFreeListMemorySystem m_system;
};

inline constexpr fnvd::Vtable hkFreeListMemorySystem_vftable{};

// IDA name (hkFreeListMemorySystem::Instance); not in the PDB. The static object bhkWorld::InitSystem constructs.
extern hkFreeListMemorySystemStorage hkFreeListMemorySystem_Instance;
inline hkFreeListAllocator* s_freeListMemory;
