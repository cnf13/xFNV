#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"

#include <stdint.h>

struct FormAndFlags;
class TESForm;
class TESTopicInfo;

// Growable array with 32-bit counts; otherwise as NiTArray: empty slots hold the element type's zero value, m_uiSize
// is the highest used index plus one, m_uiESize the number of occupied slots.
template <class T, class Allocator>
class NiTLargeArray : public NiMemObject {
public:
    // auiMaxSize slots, growing by auiGrowBy (Gamebryo's default arguments).
    NiTLargeArray(uint32_t auiMaxSize = 0, uint32_t auiGrowBy = 1);
    // the destructor is the array's only vtable slot
    virtual ~NiTLargeArray();

    uint32_t GetSize() const;
    uint32_t GetEffectiveSize() const;
    uint32_t GetAllocatedSize() const;
    uint32_t GetGrowBy() const;

    // Reallocates the buffer to uiMaxSize slots; slots at or past it are cleared first.
    void SetSize(uint32_t uiMaxSize);

    void SetGrowBy(uint32_t uiGrowBy);

    T* GetBase() const;
    T& GetAt(uint32_t uiIndex) const;

    // The slots below GetSize(), empty ones included as GetAt returns them, for range-for. Not in Gamebryo.
    T* begin() const {
        return m_pBase;
    }
    T* end() const {
        return m_pBase + m_uiSize;
    }

    // uiIndex must be below the allocated size.
    void SetAt(uint32_t uiIndex, const T& element);

    uint32_t SetAtGrow(uint32_t uiIndex, const T& element);
    uint32_t Add(const T& element);

    // Stores a non-zero element in the first empty slot (or appends it); returns the index used.
    uint32_t AddFirstEmpty(const T& element);

    // Empties slot uiIndex and returns what it held; removing the last slot shrinks the size by one.
    T RemoveAt(uint32_t uiIndex);

    // The last slot moves into uiIndex, so a packed array stays packed.
    T RemoveAtAndFill(uint32_t uiIndex);

    T RemoveEnd();
    void RemoveAll();

    // Empties the first slot holding the element; returns its index, or 0xFFFFFFFF.
    uint32_t Remove(const T& element);

    // Moves the non-zero elements to the front in order and shrinks the storage to them.
    void Compact();

    // Drops trailing empty slots from the size.
    void UpdateSize();

    // The deleting destructors free the array through ourFree.
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    T* m_pBase;
    uint32_t m_uiMaxSize;
    uint32_t m_uiSize;
    uint32_t m_uiESize;
    uint32_t m_uiGrowBy;
};

template <class T, class Allocator>
NiTLargeArray<T, Allocator>::NiTLargeArray(uint32_t auiMaxSize, uint32_t auiGrowBy)
  : m_pBase(auiMaxSize ? Allocator::Allocate(auiMaxSize) : nullptr), m_uiMaxSize(auiMaxSize), m_uiSize(0), m_uiESize(0),
    m_uiGrowBy(auiGrowBy) {}

template <class T, class Allocator>
NiTLargeArray<T, Allocator>::~NiTLargeArray() {
    Allocator::Deallocate(m_pBase);
}

// Large array of plain values: the buffer is raw heap memory.
template <class T>
class NiTLargePrimitiveArray : public NiTLargeArray<T, NiTMallocInterface<T>> {
public:
    NiTLargePrimitiveArray(uint32_t auiMaxSize = 0, uint32_t auiGrowBy = 1)
      : NiTLargeArray<T, NiTMallocInterface<T>>(auiMaxSize, auiGrowBy) {}
    ~NiTLargePrimitiveArray() override;
};

// The NiTLargeArray base's destructor frees the buffer.
template <class T>
NiTLargePrimitiveArray<T>::~NiTLargePrimitiveArray() {}

// The dialogue infos of a quest (TopicInfoArray); the vtable, constructor and destructor of this instantiation are
// defined in Fallout Shared/TESTopic.cpp.
using NiTLargeArrayTESTopicInfoPtr = NiTLargeArray<TESTopicInfo*, NiTMallocInterface<TESTopicInfo*>>;

// Large array of objects: the buffer is constructed and destroyed with new[]/delete[].
template <class T>
class NiTLargeObjectArray : public NiTLargeArray<T, NiTNewInterface<T>> {
public:
    NiTLargeObjectArray(uint32_t auiMaxSize = 0, uint32_t auiGrowBy = 1)
      : NiTLargeArray<T, NiTNewInterface<T>>(auiMaxSize, auiGrowBy) {}
};

// The two large arrays the save loader builds in place (TESSaveLoadGame's init items and their form flags). Defined in
// Fallout Shared/TESSaveLoadGame.cpp.
using NiTLargeArrayUInt = NiTLargeArray<uint32_t, NiTMallocInterface<uint32_t>>;
using NiTLargeArrayFormAndFlags = NiTLargeArray<FormAndFlags*, NiTMallocInterface<FormAndFlags*>>;

using NiTLargeArray_FormAndFlags_NiTMallocInterface_FormAndFlags = NiTLargeArray<
    FormAndFlags*,
    NiTMallocInterface<FormAndFlags*>>;

// The init-item array's own type.
using NiTLargeArrayTESFormPtr = NiTLargeArray<TESForm*, NiTMallocInterface<TESForm*>>;

#include "Gamebryo2.2/CoreLibs/NiMain/NiTLargeArray.inl"
