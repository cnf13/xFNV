#pragma once

#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"

#include <stdint.h>

class Actor;
class Animation;
class BGSCameraPath;
class bhkRigidBody;
class BipedAnim;
class CombatGroup;
struct FurnitureMark;
class MobileObject;
class NiAVObject;
class NiD3DTextureStage;
class NiGeometry;
class NiNode;
class NiObjectGroup;
class NiParticles;
class NiScreenTexture;
class NiShaderConstantMapEntry;
class NiStream;
class NiTexture;
class NiVBBlock;
class NiVBChip;
class TESIdleForm;
class TESObjectCELL;
class TESObjectREFR;
class Tile;

template <class T>
class NiTPrimitiveSet;

// Growable array of elements with 16-bit counts. Empty slots hold T(0); m_usSize is the
// highest used index plus one, m_usESize the number of occupied (non-zero) slots. The element buffer comes from
// Allocator (NiTMallocInterface or NiTNewInterface).
template <class T, class Allocator = NiTMallocInterface<T>>
class NiTArray : public NiMemObject {
public:
    // uiMaxSize slots, growing by uiGrowBy (Gamebryo's default arguments).
    NiTArray(uint32_t uiMaxSize = 0, uint32_t uiGrowBy = 1);
    // The array has a vtable on PC; the destructor is its only slot.
    virtual ~NiTArray();

    // Index of the element, without a bounds check.
    T& operator[](uint32_t uiIndex);

    uint32_t GetSize() const;
    uint32_t GetEffectiveSize() const;
    uint32_t GetAllocatedSize() const;
    uint32_t GetGrowBy() const;

    // Reallocates the buffer to uiMaxSize slots; slots at or past it are cleared first, so smart pointers drop their
    // references.
    void SetSize(uint32_t uiMaxSize);

    void SetGrowBy(uint16_t uiGrowBy);

    T* GetBase() const;

    // The slots below GetSize(), empty ones included as GetAt returns them, for range-for. Not in Gamebryo.
    T* begin() const {
        return m_pBase;
    }
    T* end() const {
        return m_pBase + m_usSize;
    }

    const T& GetAt(uint32_t uiIndex) const;

    // uiIndex must be below the allocated size.
    void SetAt(uint32_t uiIndex, const T& element);

    // Stores the element at uiIndex, growing the buffer by m_usGrowBy past uiIndex when uiIndex is beyond it; returns
    // uiIndex.
    uint32_t SetAtGrow(uint32_t uiIndex, const T& element);

    // Appends at m_usSize, growing the buffer when needed; returns the index used.
    uint32_t Add(const T& element);

    // Stores a non-zero element in the first empty slot (or appends it); returns the index used.
    uint32_t AddFirstEmpty(const T& element);

    // Shifts the slots from uiIndex up by one (growing when full) and puts the element at uiIndex. A zero element is
    // not inserted.
    void InsertAt(uint32_t uiIndex, const T& element);

    // Empties slot uiIndex and returns what it held; removing the last slot shrinks the size by one.
    T RemoveAt(uint32_t uiIndex);

    // The last slot moves into uiIndex, so a packed array stays packed.
    T RemoveAtAndFill(uint32_t uiIndex);

    T RemoveEnd();
    void RemoveAll();

    // Moves the non-zero elements to the front in order and shrinks the storage to them.
    void Compact();

    // Drops trailing empty slots from the size.
    void UpdateSize();
    // Initialises the array with uiMaxSize slots growing by uiGrowBy and returns `this`; only per-instantiation
    // definitions exist.
    NiTArray* Init(uint32_t uiMaxSize, uint32_t uiGrowBy);

    // The deleting destructors free the array through ourFree.
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    T* m_pBase;
    uint16_t m_usMaxSize;
    uint16_t m_usSize;
    uint16_t m_usESize;
    uint16_t m_usGrowBy;
};

template <class T, class Allocator>
NiTArray<T, Allocator>::NiTArray(uint32_t uiMaxSize, uint32_t uiGrowBy)
  : m_pBase(nullptr), m_usMaxSize(static_cast<uint16_t>(uiMaxSize)), m_usSize(0), m_usESize(0),
    m_usGrowBy(static_cast<uint16_t>(uiGrowBy)) {
    if (m_usMaxSize) {
        m_pBase = Allocator::Allocate(m_usMaxSize);
    }
}

template <class T, class Allocator>
NiTArray<T, Allocator>::~NiTArray() {
    Allocator::Deallocate(m_pBase);
}

// Re-initialises the array in place: uiMaxSize slots growing by uiGrowBy, none used. Returns this.
template <class T, class Allocator>
NiTArray<T, Allocator>* NiTArray<T, Allocator>::Init(uint32_t uiMaxSize, uint32_t uiGrowBy) {
    m_usMaxSize = static_cast<uint16_t>(uiMaxSize);
    m_usGrowBy = static_cast<uint16_t>(uiGrowBy);
    m_usSize = 0;
    m_usESize = 0;
    if (m_usMaxSize != 0) {
        m_pBase = Allocator::Allocate(m_usMaxSize);
    }
    else {
        m_pBase = nullptr;
    }
    return this;
}

using NiTArrayUInt = NiTArray<uint32_t, NiTMallocInterface<uint32_t>>;

// The child array of the scene-graph nodes; Fallout Shared/BipedAnim.cpp defines its destructor.
using NiTArrayNiPointerNiAVObjectNew = NiTArray<NiPointer<NiAVObject>, NiTNewInterface<NiPointer<NiAVObject>>>;

using NiTArrayNiPointerNiShaderConstantMapEntryNew = NiTArray<
    NiPointer<NiShaderConstantMapEntry>,
    NiTNewInterface<NiPointer<NiShaderConstantMapEntry>>>;

using NiTArrayNiPointerNiTextureNew = NiTArray<NiPointer<NiTexture>, NiTNewInterface<NiPointer<NiTexture>>>;

using NiTArrayNiPointerBSTempEffectNew = NiTArray<NiPointer<BSTempEffect>, NiTNewInterface<NiPointer<BSTempEffect>>>;
using NiTArrayNiTPrimitiveSetNiNodePtrMalloc = NiTArray<
    NiTPrimitiveSet<NiNode*>*,
    NiTMallocInterface<NiTPrimitiveSet<NiNode*>*>>;
using NiTArrayNiPointerNiAVObjectMalloc = NiTArray<NiPointer<NiAVObject>, NiTMallocInterface<NiPointer<NiAVObject>>>;

using NiTArrayREFR = NiTArray<TESObjectREFR*, NiTMallocInterface<TESObjectREFR*>>;

using NiTArrayBGSCameraPath = NiTArray<BGSCameraPath*, NiTMallocInterface<BGSCameraPath*>>;

using NiTArrayIdleForm = NiTArray<TESIdleForm*, NiTMallocInterface<TESIdleForm*>>;

// The particle effect's child array: its out-of-line destructor is defined in
// Fallout Shared/TempEffects/BSTempEffectParticle.cpp.
using NiTArrayAVObject = NiTArray<NiAVObject*, NiTMallocInterface<NiAVObject*>>;

using NiTArrayFurnitureMark = NiTArray<FurnitureMark, NiTNewInterface<FurnitureMark>>;
using NiTArrayNiParticlesPtr = NiTArray<NiParticles*, NiTMallocInterface<NiParticles*>>;

// Array of plain values (pointers, numbers): the buffer is raw heap memory.
template <class T>
class NiTPrimitiveArray : public NiTArray<T, NiTMallocInterface<T>> {
public:
    NiTPrimitiveArray(uint32_t uiMaxSize = 0, uint32_t uiGrowBy = 1);
    ~NiTPrimitiveArray() override;
};

template <class T>
NiTPrimitiveArray<T>::NiTPrimitiveArray(uint32_t uiMaxSize, uint32_t uiGrowBy)
  : NiTArray<T, NiTMallocInterface<T>>(uiMaxSize, uiGrowBy) {}

// The NiTArray base's destructor frees the buffer.
template <class T>
NiTPrimitiveArray<T>::~NiTPrimitiveArray() {}

// Array of objects (smart pointers, strings): the buffer is constructed and destroyed with new[]/delete[].
template <class T>
class NiTObjectArray : public NiTArray<T, NiTNewInterface<T>> {
public:
    NiTObjectArray(uint32_t uiMaxSize = 0, uint32_t uiGrowBy = 1);
    ~NiTObjectArray() override;
};

template <class T>
NiTObjectArray<T>::NiTObjectArray(uint32_t uiMaxSize, uint32_t uiGrowBy)
  : NiTArray<T, NiTNewInterface<T>>(uiMaxSize, uiGrowBy) {}

// The NiTArray base's destructor frees the buffer.
template <class T>
NiTObjectArray<T>::~NiTObjectArray() {}

// The combat manager's combat groups array (CombatGroups, a NiTPrimitiveArray); its vtable and destructors are defined
// in Fallout/AI/Combat/CombatManager.cpp.
using NiTArrayCombatGroup = NiTArray<CombatGroup*, NiTMallocInterface<CombatGroup*>>;

#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.inl"
