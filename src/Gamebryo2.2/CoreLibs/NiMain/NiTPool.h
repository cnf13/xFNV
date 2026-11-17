#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"

#include <stdint.h>

template <class T, class TAlloc>
class NiTPoolContainer;

// One block of objects a pool made: the objects, how many there are and the next block (the pool's containers form a
// list).
template <class T, class TAlloc>
class NiTPoolContainer : public NiMemObject {
public:
    // uiSize objects from TAlloc (none for 0), no next container.
    NiTPoolContainer(uint32_t uiSize);
    ~NiTPoolContainer();

    // The deleting destructor frees the container through MemoryManager::Deallocate.
    static void operator delete(void* apMemory) {
        MemoryManager_Singleton.Deallocate(apMemory);
    }

    T* m_pkObjectArray;
    uint32_t m_uiSize;
    NiTPoolContainer* m_pkNext;
};

template <class T, class TAlloc>
NiTPoolContainer<T, TAlloc>::NiTPoolContainer(uint32_t uiSize)
  : m_pkObjectArray(uiSize ? TAlloc::Allocate(uiSize) : nullptr), m_uiSize(uiSize), m_pkNext(nullptr) {}

// Frees the objects, then the rest of the container list.
template <class T, class TAlloc>
NiTPoolContainer<T, TAlloc>::~NiTPoolContainer() {
    TAlloc::Deallocate(m_pkObjectArray);
    delete m_pkNext;
}

// A pool of reusable objects: released objects wait in m_kFreeObjects, and an empty pool makes m_uiCurrentSize new ones
// in another container (the size doubles each time). Only some of the members are declared.
template <class T, class TAlloc>
class NiTPool {
public:
    // An empty free set, no container, both sizes a2 (Gamebryo's default initial size 8).
    NiTPool(uint32_t a2 = 8) : m_kFreeObjects(0), m_pkContainers(nullptr), m_uiCurrentSize(a2), m_uiInitialSize(a2) {}
    ~NiTPool();

    // Puts the object on the free list unless it is already there. Only instantiations with an explicit specialisation
    // are defined.
    void ReleaseObject(T* const& pkObject);
    // Makes a container of uiSize new objects, puts each on the free list and links the container first.
    void CreateNewObjects(uint32_t uiSize);

    NiTPrimitiveSet<T*> m_kFreeObjects;
    NiTPoolContainer<T, TAlloc>* m_pkContainers;
    uint32_t m_uiCurrentSize;
    uint32_t m_uiInitialSize;
};

// Empties the free set and deletes the containers; the free set's destructor frees its buffer.
template <class T, class TAlloc>
NiTPool<T, TAlloc>::~NiTPool() {
    m_kFreeObjects.RemoveAll();
    delete m_pkContainers;
}

// Puts the object on the free list unless it is already there.
template <class T, class TAlloc>
void NiTPool<T, TAlloc>::ReleaseObject(T* const& pkObject) {
    m_kFreeObjects.AddUnique(pkObject);
}

// A container of uiSize new objects, each put on the free list, linked first.
template <class T, class TAlloc>
void NiTPool<T, TAlloc>::CreateNewObjects(uint32_t uiSize) {
    NiTPoolContainer<T, TAlloc>* pkContainer = new NiTPoolContainer<T, TAlloc>(uiSize);
    for (uint32_t i = 0; i < uiSize; i++) {
        T* pkObject = i < pkContainer->m_uiSize ? &pkContainer->m_pkObjectArray[i] : nullptr;
        m_kFreeObjects.Add(pkObject);
    }
    pkContainer->m_pkNext = m_pkContainers;
    m_pkContainers = pkContainer;
}

// A pool whose objects are made with new[].
template <class T>
class NiTObjectPool : public NiTPool<T, NiTNewInterface<T>> {
public:
    // NiTPool's (Gamebryo's default initial size 8); the destructor is NiTPool's.
    NiTObjectPool(uint32_t auiInitialSize = 8) : NiTPool<T, NiTNewInterface<T>>(auiInitialSize) {}
};

// A pool whose objects are made with malloc. The constructor only calls NiTPool's.
template <class T>
class NiTPrimitivePool : public NiTPool<T, NiTMallocInterface<T>> {
public:
    NiTPrimitivePool(uint32_t a2 = 8) : NiTPool<T, NiTMallocInterface<T>>(a2) {}
};
