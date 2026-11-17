#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialResource.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"

#include <stdint.h>

class NiD3DTextureStage;
class NiMaterialLibrary;
class NiNode;

// Unordered growable set of elements packed at [0, m_uiUsed); m_uiUsed counts the occupied slots of the m_uiAlloced
// allocated, and the storage doubles when full. The buffer comes from Allocator (NiTMallocInterface or
// NiTNewInterface).
template <class T, class Allocator>
class NiTSet : public NiMemObject {
    // These classes call Realloc(uiNewSize) from their own code (through Add and Realloc()):
    // NiMaterialLibrary::_SDMShutdown (Realloc(0)), NiSkinPartition::CreateInitialBoneSets and
    // CreateVertexInteractions, NiSkinData::ComputeBoneBounds, NiD3DGeometryGroupManager::CreateGroup,
    // NiScreenTexture::CopyMembers, and the pools' CreateNewObjects and ReleaseObject on their free lists.
    friend class NiMaterialLibrary;
    friend class NiSkinPartition;
    friend class NiSkinData;
    friend class NiD3DGeometryGroupManager;
    friend class NiScreenTexture;
    friend class NiBoneLODController;
    template <class, class>
    friend class NiTPool;

public:
    ~NiTSet();

    uint32_t GetSize() const;
    T* GetBase() const;
    T& GetAt(uint32_t uiIndex) const;

    void Add(const T& element);

    // Adds the element unless the set already holds it.
    void AddUnique(const T& element);

    // Moves the last element into uiIndex.
    void RemoveAt(uint32_t uiIndex);

    // Shifts the elements after uiIndex down by one, keeping their order.
    void OrderedRemoveAt(uint32_t uiIndex);

    void Remove(const T& element);
    void OrderedRemove(const T& element);
    void ReplaceAt(uint32_t uiIndex, const T& element);
    void RemoveAll();

    // Index of the first slot holding the element, or -1.
    int32_t Find(const T& element) const;

protected:
    // Doubles the storage (one slot when empty).
    void Realloc();

    // Moves the elements to a buffer of uiNewSize slots; does nothing when that is the current size. uiNewSize must not
    // be below m_uiUsed; this is deliberately unchecked, as the game does.
    void Realloc(uint32_t uiNewSize);

public:
    T* m_pBase;
    uint32_t m_uiAlloced;
    uint32_t m_uiUsed;
};

template <class T, class Allocator>
NiTSet<T, Allocator>::~NiTSet() {
    Allocator::Deallocate(m_pBase);
}
// Set of plain values: the buffer is raw heap memory.
template <class T>
class NiTPrimitiveSet : public NiTSet<T, NiTMallocInterface<T>> {
public:
    // An uiInitialSize-element buffer (none for 0), nothing used (Gamebryo's default size 0).
    NiTPrimitiveSet(uint32_t uiInitialSize = 0) {
        this->m_pBase = uiInitialSize ? NiTMallocInterface<T>::Allocate(uiInitialSize) : nullptr;
        this->m_uiAlloced = uiInitialSize;
        this->m_uiUsed = 0;
    }
};

// Set of objects: the buffer is constructed and destroyed with new[]/delete[].
template <class T>
class NiTObjectSet : public NiTSet<T, NiTNewInterface<T>> {};

#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.inl"
