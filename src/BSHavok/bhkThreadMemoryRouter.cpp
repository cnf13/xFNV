#include "BSHavok/bhkThreadMemoryRouter.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/HavokThreadMemoryStruct.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/System/hkMemorySystem.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/System/hkBaseSystem.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

BSCriticalSection bhkThreadMemoryRouter::kCriticalSection;

// Under the Havok heap context and the routers' critical section: a router from the engine's heap (null when the
// allocation fails), set up by the memory system and hkBaseSystem, its temp and solver allocators pointed at its heap,
// and a 0x5000-byte stack block from that heap. A router is also constructed on the stack and never used, as the game
// does.
void bhkThreadMemoryRouter::Initialize(HavokThreadMemoryStruct* apData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (apData->pHavokThreadRouter) {
        return;
    }
    EnterCriticalSection(&kCriticalSection.CriticalSection);
    hkMemoryRouter kUnusedRouter;
    kUnusedRouter.~hkMemoryRouter();
    ::new (&kUnusedRouter) hkMemoryRouter;
    void* pMemory = MemoryManager_Singleton.Allocate(sizeof(hkMemoryRouter));
    hkMemoryRouter* pRouter = nullptr;
    if (pMemory) {
        pRouter = ::new (pMemory) hkMemoryRouter;
    }
    hkMemorySystem::getInstance().threadInit(*pRouter, "HavokThread", hkMemorySystem::FLAG_ROUTER);
    hkBaseSystem::initThread(pRouter);
    hkMemoryAllocator* pHeap = pRouter->m_heap;
    pRouter->m_temp = pHeap;
    pRouter->m_solver = pHeap;
    char* pStackBuffer = static_cast<char*>(pHeap->blockAlloc(0x5000));
    apData->pHavokThreadStackBuffer = pStackBuffer;
    // EXE-INLINED none: hkStackMemory::initMemory
    pRouter->m_stack.m_start.m_storage = pStackBuffer;
    pRouter->m_stack.m_next.m_storage = pStackBuffer;
    pRouter->m_stack.m_end.m_storage = pStackBuffer + 0x5000;
    apData->pHavokThreadRouter = pRouter;
    LeaveCriticalSection(&kCriticalSection.CriticalSection);
}

void bhkThreadMemoryRouter::Quit(HavokThreadMemoryStruct* apData) {
    if (apData->pHavokThreadRouter) {
        EnterCriticalSection(&bhkThreadMemoryRouter::kCriticalSection.CriticalSection);
        hkMemoryRouter* pHavokThreadRouter = apData->pHavokThreadRouter;
        apData->pHavokThreadRouter->m_heap->blockFree(apData->pHavokThreadStackBuffer, 0x5000);
        apData->pHavokThreadStackBuffer = 0;
        if (GetCurrentThreadId() == apData->iThreadID) {
            hkBaseSystem::quitThread();
        }
        hkMemorySystem* Instance = &hkMemorySystem::getInstance();
        Instance->threadQuit(*pHavokThreadRouter, 1);
        pHavokThreadRouter->m_stack.m_start.m_storage = nullptr;
        pHavokThreadRouter->m_stack.m_next.m_storage = 0;
        pHavokThreadRouter->m_stack.m_end.m_storage = nullptr;
        apData->pHavokThreadRouter = 0;
        LeaveCriticalSection(&bhkThreadMemoryRouter::kCriticalSection.CriticalSection);
    }
}
