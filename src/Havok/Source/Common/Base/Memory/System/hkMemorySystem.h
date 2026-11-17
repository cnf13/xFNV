#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

class hkDebugMemorySnapshot;
struct hkMemoryPointerInfo;
class hkOstream;
class hkThreadMemory;

// Prints one allocator's figures under apName against aiSoftLimit (0 or 0x7FFFFFFF: none).
void printDetailedStatistics(
    hkOstream& aOstr,
    const hkMemoryAllocator::MemoryStatistics& arStats,
    uint32_t aiSoftLimit,
    const char* apName
);

// Havok's top-level memory system: it sets up the memory routers of the main thread and of every worker thread and
// answers the debugging queries. The single instance is hkMemorySystem::s_instance. Most virtual functions are abstract
// here; the ones with a body do nothing.
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
    // Returns 1 without looking at its two arguments.
    virtual hkResult getAllocationCallStack(const void* ptr, hkMemoryPointerInfo& info);
    virtual void getMemoryStatistics(MemoryStatistics& out) {}

protected:
    // Prints the heap, thread memory, stack, solver and debug memory figures to ostr.
    void _printStatistics(hkOstream& ostr, const _PrintStatisticsInput& input);

public:
    static void replaceInstance(hkMemorySystem* a1);
    static hkMemorySystem* getInstancePtr();
    // Returns the single instance.
    static hkMemorySystem& getInstance();

    static inline hkMemorySystem* s_instance;
};
