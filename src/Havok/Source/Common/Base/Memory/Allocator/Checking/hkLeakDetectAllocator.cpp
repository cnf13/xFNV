#include "Havok/Source/Common/Base/Memory/Allocator/Checking/hkLeakDetectAllocator.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/System/Util/hkDebugMemorySnapshot.h"
#include "Havok/Source/Common/Base/Memory/Util/hkMemUtil.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/StackTracer/hkStackTracer.h"
#include "Havok/Source/Common/Base/Thread/CriticalSection/hkCriticalSection.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "SDK/Windows/winbase.h"

static void clearMap(hkLeakDetectAllocatorMap* a1);

static void clearMap(hkLeakDetectAllocatorMap* a1) {
    int32_t v1 = a1->m_hashMod + 1;
    if (v1 > 0) {
        int32_t v2 = 0;
        do {
            a1->m_elem[v2++].key = -1;
            --v1;
        }
        while (v1);
    }
    a1->m_numElems &= hkLeakDetectAllocatorMap::DONT_DEALLOCATE_FLAG;
}

// As hkFreeListMemorySystem's constructor builds its m_leakDetectAllocator: an empty call tree without an
// allocator (its own constructor) and a critical section without spin count. The other members are set by init.
hkLeakDetectAllocator::hkLeakDetectAllocator() : m_criticalSection(0) {}

void* hkLeakDetectAllocator::blockAlloc(int32_t numBytes) {
    return checkedAlloc(0, numBytes);
}

void hkLeakDetectAllocator::blockFree(void* p, int32_t numBytes) {
    checkedFree(0, p, numBytes);
}

void* hkLeakDetectAllocator::bufAlloc(int32_t& reqNumInOut) {
    return checkedAlloc(1, reqNumInOut);
}

void hkLeakDetectAllocator::bufFree(void* p, int32_t num) {
    checkedFree(1, p, num);
}

// The new buffer gets the requested size; the smaller of the two sizes is copied (signed compare).
void* hkLeakDetectAllocator::bufRealloc(void* pold, int32_t oldNum, int32_t& reqNumInOut) {
    void* pNewMem = checkedAlloc(1, reqNumInOut);
    int32_t iNumBytes = reqNumInOut < oldNum ? reqNumInOut : oldNum;
    hkMemUtil::memCpy(pNewMem, pold, iNumBytes);
    checkedFree(1, pold, oldNum);
    return pNewMem;
}

void hkLeakDetectAllocator::getMemoryStatistics(hkMemoryAllocator::MemoryStatistics& u) {
    m_childAllocator->getMemoryStatistics(u);
}

int32_t hkLeakDetectAllocator::getAllocatedSize(const void* obj, int32_t numBytes) {
    return m_childAllocator->getAllocatedSize(obj, numBytes);
}

// The call tree and the critical section go with their own destructors.
hkLeakDetectAllocator::~hkLeakDetectAllocator() {}

// KEEP (unregistered): the exe stores only the byte of aiIsBuf in the record's isBuf frame slot (setne al; mov
// [esp+0x18], al) and copies that slot's whole dword into the record it passes to insert by value, which stores it in
// the map: three bytes of stack garbage src cannot reproduce (thiscall, ret 8, 0xD5F0A0).
void* hkLeakDetectAllocator::checkedAlloc(int32_t isBuf, int32_t numBytes) {
    EnterCriticalSection(&m_criticalSection.m_section);
    void* pMem = m_childAllocator->blockAlloc(numBytes);
    m_inUse += m_childAllocator->getAllocatedSize(pMem, numBytes);
    hkLeakDetectAllocator::AllocInfo info;
    info.isBuf.m_bool = isBuf != 0;
    if (m_stackTracer) {
        hkUlong aTrace[32];
        int32_t iNumTrace = m_stackTracer->getStackTrace(aTrace, 32);
        info.traceId = m_callTree.insertCallStack(aTrace, iNumTrace);
    }
    else {
        info.traceId = 0;
    }
    info.size = numBytes;
    // The SDK's hkUlong(p): the map is keyed by the address's value.
    m_dynamic->m_activePointers.insert(*m_debugAllocator, reinterpret_cast<hkUlong>(pMem), info);
    LeaveCriticalSection(&m_criticalSection.m_section);
    return pMem;
}

void hkLeakDetectAllocator::checkedFree(int32_t isBuf, void* p, int32_t numBytes) {
    EnterCriticalSection(&m_criticalSection.m_section);
    if (p) {
        if (numBytes) {
            // The SDK's hkUlong(p): the map is keyed by the address's value.
            hkLeakDetectAllocatorMap::Iterator it = m_dynamic->m_activePointers.findKey(reinterpret_cast<hkUlong>(p));
            // The SDK's isValid(it): the slot index the iterator carries (HK_MAP_ITERATOR_TO_INDEX, an int) is at
            // most m_hashMod.
            int32_t iIndex = static_cast<int32_t>(reinterpret_cast<uintptr_t>(it));
            if (iIndex > m_dynamic->m_activePointers.m_hashMod) {
                m_outputFunc("Freeing block not from this allocator", m_outputFuncArg);
                __builtin_debugtrap();
            }
            hkLeakDetectAllocator::AllocInfo info = m_dynamic->m_activePointers.m_elem[iIndex].val;
            if (info.size != numBytes) {
                danger("Freeing block of incorrect size", p, info);
                __builtin_debugtrap();
            }
            m_dynamic->m_activePointers.remove(it);
            m_inUse -= m_childAllocator->getAllocatedSize(p, numBytes);
            m_childAllocator->blockFree(p, numBytes);
            LeaveCriticalSection(&m_criticalSection.m_section);
        }
        else {
            m_outputFunc("Given zero bytes to free with an nonnull address\n", m_outputFuncArg);
            __builtin_debugtrap();
            LeaveCriticalSection(&m_criticalSection.m_section);
        }
    }
    else {
        if (numBytes) {
            m_outputFunc("Given nonzero bytes to free with an null address\n", m_outputFuncArg);
            __builtin_debugtrap();
        }
        LeaveCriticalSection(&m_criticalSection.m_section);
    }
}

void hkLeakDetectAllocator::danger(
    const char* message,
    const void* ptr,
    const hkLeakDetectAllocator::AllocInfo& info
) const {
    char acBuffer[256];
    hkString::snprintf(acBuffer, 256, "\nAddress=0x%p size=%i\n", ptr, info.size);
    m_outputFunc(message, m_outputFuncArg);
    m_outputFunc(acBuffer, m_outputFuncArg);
    hkUlong aTrace[32];
    int32_t iNumTrace = m_callTree.getCallStack(info.traceId, aTrace, 32);
    m_stackTracer->dumpStackTrace(aTrace, iNumTrace, m_outputFunc, m_outputFuncArg);
    m_outputFunc("-------------------------------------------------------------------\n\n", m_outputFuncArg);
}

// A map slot whose key is -1 is empty.
hkResult hkLeakDetectAllocator::getMemorySnapshot(hkDebugMemorySnapshot& snapshot) {
    EnterCriticalSection(&m_criticalSection.m_section);
    snapshot.reset();
    Dynamic* pDynamic = m_dynamic;
    int32_t iNumPointers = pDynamic->m_activePointers.m_numElems & hkLeakDetectAllocatorMap::NUM_ELEMS_MASK;
    if (!iNumPointers) {
        LeaveCriticalSection(&m_criticalSection.m_section);
        return HK_SUCCESS;
    }
    snapshot.m_numPointers = iNumPointers;
    snapshot.m_info = static_cast<hkMemoryPointerInfo*>(
        hkMemoryRouter::getInstance().debug().blockAlloc(sizeof(hkMemoryPointerInfo) * iNumPointers)
    );
    hkMemUtil::memSet(snapshot.m_info, 0, sizeof(hkMemoryPointerInfo) * iNumPointers);
    void** ppPointer =
        static_cast<void**>(hkMemoryRouter::getInstance().debug().blockAlloc(sizeof(void*) * iNumPointers));
    hkMemoryPointerInfo* pInfo = snapshot.m_info;
    snapshot.m_pointers = ppPointer;
    hkMemoryRouter& rRouter = hkMemoryRouter::getInstance();
    int32_t iHashMod = pDynamic->m_activePointers.m_hashMod;
    int32_t i = 0;
    while (i <= iHashMod && pDynamic->m_activePointers.m_elem[i].key == static_cast<hkUlong>(-1)) {
        ++i;
    }
    while (i <= iHashMod) {
        hkLeakDetectAllocatorMap::Pair& rPair = pDynamic->m_activePointers.m_elem[i];
        int32_t iSize = rPair.val.size;
        int32_t iTraceId = rPair.val.traceId;
        // The SDK's (void*)key: the map is keyed by the address's value.
        *ppPointer = reinterpret_cast<void*>(static_cast<uintptr_t>(rPair.key));
        hkUlong aTrace[16];
        int32_t iNumTrace = m_callTree.getCallStack(iTraceId, aTrace, 16);
        pInfo->m_numStackTrace = iNumTrace;
        hkString::memCpy(pInfo->m_stackTraces, aTrace, static_cast<int32_t>(sizeof(hkUlong)) * iNumTrace);
        pInfo->m_numBytes = iSize;
        // Asks the heap for the block's size and drops the result, as the game does.
        rRouter.heap().getAllocatedSize(*ppPointer, iSize);
        iHashMod = pDynamic->m_activePointers.m_hashMod;
        ++i;
        ++pInfo;
        ++ppPointer;
        while (i <= iHashMod && pDynamic->m_activePointers.m_elem[i].key == static_cast<hkUlong>(-1)) {
            ++i;
        }
    }
    LeaveCriticalSection(&m_criticalSection.m_section);
    return HK_SUCCESS;
}

// Reports every allocation still in the map, then frees the call tree, the map and its holder and lets go of the stack
// tracer.
void hkLeakDetectAllocator::quit() {
    if (m_dynamic->m_activePointers.m_numElems & hkLeakDetectAllocatorMap::NUM_ELEMS_MASK) {
        char acBuffer[256];
        m_outputFunc(
            "**************************************************************\n* Begin hkLeakDetectorAllocator  "
            "                            *\n**************************************************************\n",
            m_outputFuncArg
        );
        Dynamic* pDynamic = m_dynamic;
        int32_t iHashMod = pDynamic->m_activePointers.m_hashMod;
        int32_t i = 0;
        while (i <= iHashMod && pDynamic->m_activePointers.m_elem[i].key == static_cast<hkUlong>(-1)) {
            ++i;
        }
        if (i <= iHashMod) {
            do {
                hkLeakDetectAllocatorMap::Pair& rPair = pDynamic->m_activePointers.m_elem[i];
                hkLeakDetectAllocator::AllocInfo info = rPair.val;
                // The SDK's (void*)key: the map is keyed by the address's value.
                danger("Memory Leaked:\n", reinterpret_cast<const void*>(static_cast<uintptr_t>(rPair.key)), info);
                pDynamic = m_dynamic;
                ++i;
                iHashMod = pDynamic->m_activePointers.m_hashMod;
                while (i <= iHashMod && pDynamic->m_activePointers.m_elem[i].key == static_cast<hkUlong>(-1)) {
                    ++i;
                }
            }
            while (i <= pDynamic->m_activePointers.m_hashMod);
        }
        hkString::snprintf(
            acBuffer, 256, "\n%i leaks found\n",
            m_dynamic->m_activePointers.m_numElems & hkLeakDetectAllocatorMap::NUM_ELEMS_MASK
        );
        m_outputFunc(acBuffer, m_outputFuncArg);
        m_outputFunc(
            "**************************************************************\n* End hkLeadDetectorAllocator    "
            "                            *\n**************************************************************\n",
            m_outputFuncArg
        );
        __builtin_debugtrap();
    }
    // EXE-INLINED none: hkStackTracer::CallTree::quit (with hkArrayBase<Node>::_clearAndDeallocate)
    m_callTree.m_nodes.m_size = 0;
    if ((m_callTree.m_nodes.m_capacityAndFlags & hkArrayBase<hkStackTracer::CallTree::Node>::DONT_DEALLOCATE_FLAG) ==
        0) {
        m_callTree.m_allocator->_bufFree<hkStackTracer::CallTree::Node>(
            m_callTree.m_nodes.m_data, m_callTree.m_nodes.getCapacity()
        );
    }
    m_callTree.m_nodes.m_data = nullptr;
    m_callTree.m_nodes.m_capacityAndFlags = hkArrayBase<hkStackTracer::CallTree::Node>::DONT_DEALLOCATE_FLAG;
    m_callTree.m_allocator = nullptr;
    m_dynamic->m_activePointers.clearAndDeallocate(*m_debugAllocator);
    m_debugAllocator->blockFree(m_dynamic, sizeof(Dynamic));
    m_stackTracer->removeReference();
}

void hkLeakDetectAllocator::init(
    hkMemoryAllocator* child,
    hkMemoryAllocator* debug,
    void (*output)(const char*, void*),
    void* outputUserData
) {
    m_childAllocator = child;
    m_debugAllocator = debug;
    m_outputFunc = output;
    m_outputFuncArg = outputUserData;
    m_inUse = 0;
    hkStackTracer* pTracer = hkStackTracer::s_instance;
    if (!pTracer) {
        hkStackTracer* pNewTracer = hkStackTracer::create();
        // EXE-INLINED none: hkSingleton<hkStackTracer>::replaceInstance
        if (hkStackTracer::s_instance) {
            hkStackTracer::s_instance->removeReferenceLockUnchecked();
        }
        pTracer = pNewTracer;
        hkStackTracer::s_instance = pNewTracer;
    }
    m_stackTracer = pTracer;
    pTracer->addReferenceLockUnchecked();
    hkLeakDetectAllocator::Dynamic* v8 =
        static_cast<hkLeakDetectAllocator::Dynamic*>(debug->blockAlloc(sizeof(hkLeakDetectAllocator::Dynamic)));
    if (v8) {
        v8->m_activePointers.m_elem = nullptr;
        v8->m_activePointers.m_numElems = 0;
        v8->m_activePointers.m_hashMod = -1;
        m_callTree.m_allocator = debug;
        m_dynamic = v8;
    }
    else {
        m_callTree.m_allocator = debug;
        m_dynamic = nullptr;
    }
}

// The live-allocation map's members.
int32_t hkLeakDetectAllocatorMap::insert(hkMemoryAllocator& alloc, hkUlong key, hkLeakDetectAllocator::AllocInfo val) {
    int32_t m_hashMod = this->m_hashMod;
    if (2 * m_numElems > m_hashMod) {
        resizeTable(alloc, 2 * m_hashMod + 2);
    }
    int32_t v6 = this->m_hashMod;
    Pair* m_elem = this->m_elem;
    uint32_t v8 = v6 & -0x61C8864F * (key >> 4);
    int32_t v9 = v8;
    uint32_t result = 1;
    if (this->m_elem[v8].key != -1) {
        while (m_elem[v9].key != key) {
            v8 = v6 & v8 + 1;
            v9 = v8;
            if (m_elem[v8].key == -1) {
                result = 1;
                goto LABEL_8;
            }
        }
        result = 0;
    }
LABEL_8:;
    m_numElems += result;
    m_elem[v8].key = key;
    this->m_elem[v8].val = val;
    return result;
}

// The slot index travels as the SDK's Iterator (HK_MAP_INDEX_TO_ITERATOR).
hkLeakDetectAllocatorMap::Iterator hkLeakDetectAllocatorMap::findKey(hkUlong key) const {
    int32_t m_hashMod = this->m_hashMod;
    if (m_hashMod <= 0) {
        return reinterpret_cast<Iterator>(static_cast<uintptr_t>(m_hashMod + 1));
    }
    Pair* m_elem = this->m_elem;
    int32_t result = m_hashMod & -0x61C8864F * (key >> 4);
    hkUlong v5 = this->m_elem[result].key;
    if (v5 == -1) {
        return reinterpret_cast<Iterator>(static_cast<uintptr_t>(m_hashMod + 1));
    }
    while (v5 != key) {
        result = m_hashMod & result + 1;
        v5 = m_elem[result].key;
        if (v5 == -1) {
            return reinterpret_cast<Iterator>(static_cast<uintptr_t>(m_hashMod + 1));
        }
    }
    return reinterpret_cast<Iterator>(static_cast<uintptr_t>(result));
}

// The pairs after slot it whose hash does not place them past the emptied slot move back into it, one at a time. The
// compares are unsigned.
void hkLeakDetectAllocatorMap::remove(Iterator it) {
    // The SDK's HK_MAP_ITERATOR_TO_INDEX: the iterator is no pointer but carries the slot index.
    uint32_t i = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(it));
    --m_numElems;
    m_elem[i].key = hkUlong(-1);
    uint32_t lo = (i + m_hashMod) & m_hashMod;
    while (m_elem[lo].key != hkUlong(-1)) {
        lo = (lo + m_hashMod) & m_hashMod;
    }
    lo = (lo + 1) & m_hashMod;
    uint32_t empty = i;
    for (i = (i + 1) & m_hashMod; m_elem[i].key != hkUlong(-1); i = (i + 1) & m_hashMod) {
        // The SDK's hkMapOperations<hkUlong>::hash.
        uint32_t hash = (m_elem[i].key >> 4) * 2654435761u & m_hashMod;
        if (i >= lo && hash > empty) {
            continue;
        }
        if (i < empty && (hash > empty || hash <= i)) {
            continue;
        }
        if (hash > empty && hash < lo) {
            continue;
        }
        m_elem[empty].key = m_elem[i].key;
        m_elem[empty].val = m_elem[i].val;
        m_elem[i].key = hkUlong(-1);
        empty = i;
    }
}

void hkLeakDetectAllocatorMap::clearAndDeallocate(hkMemoryAllocator& alloc) {
    clearMap(this);
    if (m_numElems >= 0) {
        alloc.blockFree(m_elem, static_cast<int32_t>(sizeof(Pair)) * (m_hashMod + 1));
    }
}

void hkLeakDetectAllocatorMap::resizeTable(hkMemoryAllocator& alloc, int32_t newcap) {
    int32_t v3 = newcap;
    if (newcap <= 8) {
        v3 = 8;
    }
    uint32_t v10 = m_numElems & DONT_DEALLOCATE_FLAG;
    Pair* m_elem = this->m_elem;
    int32_t v5 = m_hashMod + 1;
    this->m_elem = static_cast<Pair*>(alloc.blockAlloc(sizeof(Pair) * v3));
    if (v3 > 0) {
        int32_t v6 = 0;
        int32_t v7 = v3;
        do {
            this->m_elem[v6++].key = -1;
            --v7;
        }
        while (v7);
    }
    m_numElems = 0;
    m_hashMod = v3 - 1;
    if (v5 > 0) {
        Pair* v8 = m_elem;
        int32_t newcapa = v5;
        do {
            if (v8->key != -1) {
                insert(alloc, v8->key, v8->val);
            }
            ++v8;
            --newcapa;
        }
        while (newcapa);
    }
    if (!v10) {
        alloc.blockFree(m_elem, static_cast<int32_t>(sizeof(Pair)) * v5);
    }
}
