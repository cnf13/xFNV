#pragma once

#include "BSCore/BSSpinLock.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"

// Base of the node allocators of the lists and maps. A node holds the free-list link, a data pointer and one element;
// a list or map reuses the node memory as its own item type. The pool's state (block list, free list, lock) is static
// per instantiation in the engine; the primary template declares the lock, the shared pointer pool below all of it.
template <class T>
class NiTAbstractPoolAllocator {
public:
    class AllocNode : public NiMemObject {
    public:
        // Both the scalar and array forms free through MemoryManager::Deallocate.
        static void operator delete(void* apMemory) {
            MemoryManager::Instance().Deallocate(apMemory);
        }
        static void operator delete[](void* apMemory) {
            MemoryManager::Instance().Deallocate(apMemory);
        }

        AllocNode* m_pkNext;
        void* m_pkData;
        T m_element;
    };

    // Allocates a block of ms_uiBlockSize nodes and makes all but its first the free list; the first links the block
    // into ms_pkBlockHeader.
    static void CreateFreeMem();

    // The pool's lock, one per instantiation.
    static BSSpinLock ms_kSpinLock;
    // The blocks CreateFreeMem has allocated, linked through their first node's m_pkNext, and the free nodes, linked
    // through m_pkNext; one pair per instantiation.
    static AllocNode* ms_pkBlockHeader;
    static AllocNode* ms_pkFreeMem;
    // The nodes CreateFreeMem allocates at a time.
    static uint32_t ms_uiBlockSize;
};

template <class T>
BSSpinLock NiTAbstractPoolAllocator<T>::ms_kSpinLock;
template <class T>
NiTAbstractPoolAllocator<T>::AllocNode* NiTAbstractPoolAllocator<T>::ms_pkBlockHeader = nullptr;
template <class T>
NiTAbstractPoolAllocator<T>::AllocNode* NiTAbstractPoolAllocator<T>::ms_pkFreeMem = nullptr;
// Gamebryo's block size for the timeline pool (NiAllocatorDeclareStatics(NiActorManager::TimelineData, 256)), the one
// instance besides the shared pointer pool, which declares its own.
template <class T>
uint32_t NiTAbstractPoolAllocator<T>::ms_uiBlockSize = 256;

// The block comes from new[], so the pool's shutdown frees it with delete[].
template <class T>
void NiTAbstractPoolAllocator<T>::CreateFreeMem() {
    ms_pkFreeMem = new AllocNode[ms_uiBlockSize];
    AllocNode* pkCurr = ms_pkFreeMem + ms_uiBlockSize - 1;
    pkCurr->m_pkNext = nullptr;
    uint32_t i = 1;
    do {
        pkCurr = ms_pkFreeMem + i;
        pkCurr->m_pkNext = pkCurr + 1;
    }
    while (i++ < ms_uiBlockSize - 2);

    if (ms_pkBlockHeader) {
        ms_pkFreeMem->m_pkNext = ms_pkBlockHeader;
        ms_pkBlockHeader = ms_pkFreeMem;
    }
    else {
        ms_pkBlockHeader = ms_pkFreeMem;
        ms_pkBlockHeader->m_pkNext = nullptr;
    }
    ms_pkFreeMem = ms_pkFreeMem + 1;
}

// The pool of 24-byte nodes (12 in the exe) every NiTPointerList and NiTPointerMap draws from.
template <>
class NiTAbstractPoolAllocator<uint32_t> {
public:
    class AllocNode : public NiMemObject {
    public:
        AllocNode* m_pkNext;
        void* m_pkData;
        uintptr_t m_element;
    };

    // KEEP: the forwarder stays. A caller reads a caller-saved register after the call, relying on the value this
    // body leaves there, which clang does not guarantee.
    // Allocates a block of nodes and makes them the free list. cdecl.
    static void CreateFreeMem();
    static void Lock() {
        ms_kSpinLock.Lock(nullptr);
    }

    // BSSpinLock::Unlock without its InitializationFlow call, inline in the engine.
    static void Unlock() {
        if (--ms_kSpinLock.uiLockCount == 0) {
            ms_kSpinLock.uiOwningThread = 0;
        }
    }

    static void Deallocate(NiTAbstractPoolAllocator<uint32_t>::AllocNode* pkDel);
    // Frees every block of ms_pkBlockHeader through MemoryManager::Deallocate and clears ms_pkFreeMem and
    // ms_pkBlockHeader.
    static void _SDMShutdown();

    // The free nodes, linked through m_pkNext.
    static AllocNode* ms_pkFreeMem;
    static BSSpinLock ms_kSpinLock;
    // The blocks CreateFreeMem has allocated, linked through their first node's m_pkNext.
    static AllocNode* ms_pkBlockHeader;
    // The nodes CreateFreeMem allocates at a time.
    static uint32_t ms_uiBlockSize;
};
