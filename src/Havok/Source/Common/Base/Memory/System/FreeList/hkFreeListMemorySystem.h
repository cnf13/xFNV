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

class hkDebugMemorySnapshot;

// Havok's free-list memory system, the one the game installs. The slots below are its overrides.
class hkFreeListMemorySystem : public hkMemorySystem {
public:
    // What the system keeps per thread: the thread's heap allocator, its router and name, and the stack allocator it
    // was given.
    struct ThreadData {
        hkThreadMemory m_heapThreadMemory;
        hkMemoryRouter* m_memoryRouter;
        const char* m_name;
        hkBool m_inUse;
        // The bytes of stack memory the thread has taken.
        uint32_t m_stackMemoryAllocated;
        hkStackMemory* m_stackMemory;
    };

    hkFreeListMemorySystem();
    ~hkFreeListMemorySystem() override;

    hkMemoryRouter*
    mainInit(const hkMemorySystem::FrameInfo& info, hkFlags<hkMemorySystem::FlagBits, int32_t> flags) override;
    hkResult mainQuit(hkFlags<hkMemorySystem::FlagBits, int32_t> flags) override;
    void
    threadInit(hkMemoryRouter& router, const char* name, hkFlags<hkMemorySystem::FlagBits, int32_t> flags) override;
    void threadQuit(hkMemoryRouter& router, hkFlags<hkMemorySystem::FlagBits, int32_t> flags) override;
    void printStatistics(hkOstream& ostr) override;
    hkResult walkMemory(void (*callback)(void*, uint32_t, hkBool, int32_t, void*), void* param) override;
    void garbageCollect() override;
    // No snapshots: always HK_FAILURE (1).
    hkResult getMemorySnapshot(hkDebugMemorySnapshot& snapshot) override;
    void getMemoryStatistics(hkMemorySystem::MemoryStatistics& out) override;

    void setAllocator(hkMemoryAllocator* a);

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

// The static object bhkWorld::InitSystem constructs.
extern hkFreeListMemorySystemStorage hkFreeListMemorySystem_Instance;
inline hkFreeListAllocator* s_freeListMemory;
