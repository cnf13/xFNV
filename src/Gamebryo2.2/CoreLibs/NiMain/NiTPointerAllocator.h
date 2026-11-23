#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTAbstractPoolAllocator.h"

// Node allocator of NiTPointerList and NiTPointerMap: every instantiation draws from the engine's one pool of 12-byte
// nodes (two pointers and one pointer-sized element).
template <class T>
class NiTPointerAllocator : public NiTAbstractPoolAllocator<T> {
public:
    void* Allocate();
    void Deallocate(void* pkDel);
};
// Takes a cleared node from the pool: pops the free list under the pool's lock, refilling the list first when it is
// empty.
template <class T>
void* NiTPointerAllocator<T>::Allocate() {
    using Pool = NiTAbstractPoolAllocator<T>;
    Pool::ms_kSpinLock.Lock(nullptr);
    if (!Pool::ms_pkFreeMem) {
        Pool::CreateFreeMem();
    }
    typename Pool::AllocNode* pNode = Pool::ms_pkFreeMem;
    Pool::ms_pkFreeMem = Pool::ms_pkFreeMem->m_pkNext;
    pNode->m_element = 0;
    pNode->m_pkNext = nullptr;
    pNode->m_pkData = nullptr;
    Pool::ms_kSpinLock.Unlock();
    return pNode;
}

// Returns the node to the pool: its data pointer cleared, linked at the free list's head under the pool's lock.
template <class T>
void NiTPointerAllocator<T>::Deallocate(void* pkDel) {
    using Pool = NiTAbstractPoolAllocator<T>;
    Pool::ms_kSpinLock.Lock(nullptr);
    typename Pool::AllocNode* pkAllocNode = static_cast<Pool::AllocNode*>(pkDel);
    pkAllocNode->m_pkData = nullptr;
    pkAllocNode->m_pkNext = Pool::ms_pkFreeMem;
    Pool::ms_pkFreeMem = pkAllocNode;
    Pool::ms_kSpinLock.Unlock();
}
