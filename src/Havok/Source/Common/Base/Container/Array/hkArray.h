#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Util/hkMemUtil.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkTypeTraits.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhaseHandlePair.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/BodyPairCollector/hkpRootCdBodyPair.h"
#include "SDK/MSVC/ehvec.h"

#include <cstdint>
#include <stdint.h>

class hkClass;
class hkpCollidable;
class hkpEntity;
class hkpPhantom;
class hkpRigidBody;
class hkProcess;
struct hkpRootCdPoint;
class hkpShape;
class hkpSimulationIsland;
class hkQsTransform;

namespace hkWorldOperation {
    struct BiggestOperation;
}

// The type the non-const element access returns. The void instance models only the three words of an untyped array and
// a reference to void is ill-formed, so that instance gets void.
template <typename T>
struct hkArrayElementReference {
    using Type = T&;
};

template <>
struct hkArrayElementReference<void> {
    using Type = void;
};

// The three words every hkArray starts with. A negative capacity marks storage the array must not reallocate,
// because it belongs to somebody else.
template <typename T>
class hkArrayBase {
public:
    enum {
        // Set in m_capacityAndFlags when the array must not free or grow its storage.
        DONT_DEALLOCATE_FLAG = 0x80000000u,
        // The number of elements the storage holds is the rest of the word.
        CAPACITY_MASK = 0x3FFFFFFFu,
        // Both flag bits.
        FLAG_MASK = 0xC0000000u,
    };

    // The default constructor leaves the members unset: the sites that declare an hkArrayBase local set its members.
    hkArrayBase() = default;

    // The constructor from an external array: the storage, its size and its capacity, whose DONT_DEALLOCATE bit is
    // set.
    hkArrayBase(T* apPtr, int32_t aiSize, int32_t aiCapacity)
      : m_data(apPtr), m_size(aiSize), m_capacityAndFlags(aiCapacity | DONT_DEALLOCATE_FLAG) {}

    // The SDK's finish constructor: a loaded array keeps what was loaded.
    hkArrayBase(hkFinishLoadedObjectFlag) {}

    // The number of elements the storage holds; defined below. _pushBack keeps an explicit specialization per element
    // type, next to the element type's owner.
    int32_t getCapacity() const;
    // The number of elements.
    int32_t getSize() const {
        return static_cast<int32_t>(m_size);
    }
    // The raw word: the capacity and the flags.
    int32_t getCapacityAndFlags() const {
        return static_cast<int32_t>(m_capacityAndFlags);
    }

    // Whether there are no elements.
    hkBool isEmpty() const {
        hkBool kResult;
        kResult.m_bool = (m_size == 0);
        return kResult;
    }

    T* begin() {
        return m_data;
    }
    const T* begin() const {
        return m_data;
    }
    T* end() {
        return m_data + m_size;
    }
    const T* end() const {
        return m_data + m_size;
    }
    // Removes numElemsToRemove elements from the end.
    void popBack(int32_t numToRemove = 1) {
        m_size -= numToRemove;
    }
    // Sets the size without growing the storage.
    void setSizeUnchecked(int32_t n) {
        m_size = static_cast<uint32_t>(n);
    }

    // Makes room for n more elements without growing the storage; returns the first.
    T* expandByUnchecked(int32_t n) {
        T* p = m_data + m_size;
        m_size += n;
        return p;
    }

    // Removes the element at index, moving the ones after it down.
    void removeAtAndCopy(int32_t index);
    // Removes numToRemove elements from index, moving the ones after them down.
    void removeAtAndCopy(int32_t index, int32_t numToRemove);
    // The index of the first element equal to t from start up to end (-1: the end of the array), or -1. The element
    // reference members are member templates so that hkArrayBase<void> stays declarable.
    template <typename U = T>
    int32_t indexOf(const U& t, int32_t startIdx = 0, int32_t endIdx = -1) const;
    template <typename U = T>
    int32_t lastIndexOf(const U& t) const;
    // Empties the array, keeping its storage; _clearAndDeallocate calls it on its own this. The inline body is defined
    // below for every element type; a class element type that needs its destruct gets an explicit specialization.
    void clear();
    // Frees the storage unless it is not the array's, and empties the array.
    void _clearAndDeallocate(hkMemoryAllocator& alloc);
    // Shrinks the storage to the elements and numFreeElemsLeft more.
    void _optimizeCapacity(hkMemoryAllocator& alloc, int32_t numFreeElemsLeft, int32_t shrinkExact = 0);
    // Grows the storage to exactly n elements.
    void _reserveExactly(hkMemoryAllocator& alloc, int32_t n);
    // The SDK's _setSize(alloc, n, fill) and hkArray::setSize(size, fill) are left out: registrations take the
    // address of _setSize, which a second overload would make ambiguous.
    // Adds one element at the end and returns it.
    template <typename U = T>
    U& _expandOne(hkMemoryAllocator& alloc);
    // Adds n elements at the end and returns the first.
    T* _expandBy(hkMemoryAllocator& alloc, int32_t n);
    // Opens a gap of numToInsert elements at index and returns its first.
    T* _expandAt(hkMemoryAllocator& alloc, int32_t index, int32_t numtoinsert);
    // Inserts a copy of t at index: the SDK's _insertAt(alloc, index, &t, 1), defined below.
    template <typename U = T>
    void _insertAt(hkMemoryAllocator& alloc, int32_t index, const U& t);
    // Appends numElems elements copied from a.
    void _append(hkMemoryAllocator& alloc, const T* a, int32_t numtoinsert);
    // Makes this a copy of a, a plain-data element type's: the storage is replaced by one of a's size when it is
    // smaller (the SDK's inline copyFromArray(alloc, a, hkTypeIsPod), which hkArray::operator= calls), defined below.
    hkArrayBase& copyFromArray(hkMemoryAllocator& alloc, const hkArrayBase& a, hkTypeIsPod);
    // The class element type's: the kept elements assigned, the rest copy-constructed, the surplus destroyed (the
    // SDK's inline copyFromArray(alloc, a, hkTypeIsClass)), defined below.
    hkArrayBase& copyFromArray(hkMemoryAllocator& alloc, const hkArrayBase& a, hkTypeIsClass);
    // Removes the element at index; the last element moves into its place.
    void removeAt(int32_t index);
    // Appends a copy of t, growing the storage through alloc when it is full. A member template so that
    // hkArrayBase<void> stays declarable.
    template <typename U = T>
    void _pushBack(hkMemoryAllocator& alloc, const U& t);
    // Grows the storage through alloc to hold at least n elements, doubling it at least.
    void _reserve(hkMemoryAllocator& alloc, int32_t n);
    // Resizes the array to n elements, growing the storage through alloc. The SDK's inline body is defined below.
    void _setSize(hkMemoryAllocator& alloc, int32_t n);
    // The element at index, without a bounds check. A member template like _pushBack so that hkArrayBase<void> stays
    // declarable.
    template <typename U = T>
    const U& operator[](int32_t index) const;
    // The element at index, without a bounds check. The return type is spelled through hkArrayElementReference so that
    // the void instance stays declarable.
    hkArrayElementReference<T>::Type operator[](int32_t index) {
        return m_data[index];
    }
    // Replaces numdel elements at index with numtoinsert copied from p, growing the storage through alloc. The body is
    // defined below; a class element type, whose copies the generic body cannot make, gets an explicit specialization.
    void _spliceInto(hkMemoryAllocator& alloc, int32_t index, int32_t numdel, const T* p, int32_t numtoinsert);

    // Inserts numtoinsert elements copied from p at index: a splice that deletes nothing.
    void _insertAt(hkMemoryAllocator& alloc, int32_t index, const T* p, int32_t numtoinsert) {
        _spliceInto(alloc, index, 0, p, numtoinsert);
    }

    // Copies n elements from src to dst, which must not overlap it from below.
    static void copy(T* dst, const T* src, int32_t n);

    T* m_data;
    int32_t m_size;
    int32_t m_capacityAndFlags;
};
// A dynamic array of elements stored contiguously. Growing and shrinking it is hkArrayUtil's job.
template <typename T, typename Allocator = hkContainerHeapAllocator>
class hkArray : public hkArrayBase<T> {
public:
    using AllocatorType = Allocator;

    hkArray() : hkArrayBase<T>(nullptr, 0, 0) {}
    // The array over an external buffer, flagged DONT_DEALLOCATE.
    hkArray(T* buffer, int32_t size, int32_t capacity) : hkArrayBase<T>(buffer, size, capacity) {}
    // The SDK's finish constructor: a loaded array keeps what was loaded.
    hkArray(hkFinishLoadedObjectFlag flag) : hkArrayBase<T>(flag) {}
    // The SDK's copy constructor and assignment (hkArray.inl): the elements copied into storage of this array's own.
    // The implicit ones would share a's buffer, and both arrays' destructors would free it.
    hkArray(const hkArray& a) : hkArrayBase<T>(nullptr, 0, 0) {
        *this = a;
    }
    // The SDK's ~hkArray: the elements destroyed and the storage freed through the allocator policy.
    ~hkArray() {
        clearAndDeallocate();
    }

    hkArray& operator=(const hkArray& a) {
        hkArrayBase<T>::copyFromArray(AllocatorType().get(this), a, typename hkIsPodType<T>::type());
        return *this;
    }

    // Appends a copy of the element. Each instantiation used gets an explicit specialization next to the element type's
    // owner. A member template so that hkArray<void> stays declarable.
    template <typename U = T>
    void pushBack(const U& e);
    // The wrappers below pass the allocator policy's allocator (hkContainerHeapAllocator::get) to the array's
    // underscore functions of the same name: the SDK's inline bodies.
    void clearAndDeallocate() {
        hkArrayBase<T>::_clearAndDeallocate(AllocatorType().get(this));
    }

    void optimizeCapacity(int32_t numFreeElemsLeft, int32_t shrinkExact = 0) {
        hkArrayBase<T>::_optimizeCapacity(AllocatorType().get(this), numFreeElemsLeft, shrinkExact);
    }

    void reserve(int32_t size) {
        hkArrayBase<T>::_reserve(AllocatorType().get(this), size);
    }
    void reserveExactly(int32_t size) {
        hkArrayBase<T>::_reserveExactly(AllocatorType().get(this), size);
    }
    void setSize(int32_t size) {
        hkArrayBase<T>::_setSize(AllocatorType().get(this), size);
    }
    // (setSize with a fill value: see hkArrayBase::_setSize.)
    template <typename U = T>
    U& expandOne();
    T* expandBy(int32_t n) {
        return hkArrayBase<T>::_expandBy(AllocatorType().get(this), n);
    }
    T* expandAt(int32_t i, int32_t n) {
        return hkArrayBase<T>::_expandAt(AllocatorType().get(this), i, n);
    }

    template <typename U = T>
    void insertAt(int32_t i, const U& e) {
        hkArrayBase<T>::template _insertAt<U>(AllocatorType().get(this), i, e);
    }

    void insertAt(int32_t i, const T* a, int32_t numElems) {
        hkArrayBase<T>::_insertAt(AllocatorType().get(this), i, a, numElems);
    }

    void append(const T* a, int32_t numElems) {
        hkArrayBase<T>::_append(AllocatorType().get(this), a, numElems);
    }

    void spliceInto(int32_t i, int32_t ndel, const T* p, int32_t numElems) {
        hkArrayBase<T>::_spliceInto(AllocatorType().get(this), i, ndel, p, numElems);
    }

    // Exchanges the three words with a's.
    void swap(hkArray& a);
};

// The hkArray<hkpRootCdBodyPair>'s clearAndDeallocate: the heap
// allocator's _clearAndDeallocate.
void clearAndDeallocate(hkArray<hkpRootCdBodyPair, hkContainerHeapAllocator>* apThis, [[maybe_unused]] void* apEdx);
// The vertices' array, the indices of the two edge vertices and of
// the vertex to point away from, the normal's result.
void calculateOrientedIntersectionLine(
    const hkArray<hkVector4>* pVertices,
    int32_t i,
    int32_t j,
    int32_t k,
    hkVector4* pNormalOut
);

// An hkArray with room for N elements inside the object: the array starts out pointing at m_storage and moves to
// the heap only when it outgrows it (flagged DONT_DEALLOCATE until then).
template <typename T, int32_t N, typename Allocator = hkContainerHeapAllocator>
class hkInplaceArray : public hkArray<T, Allocator> {
public:
    // The array over m_storage, size elements in use, flagged DONT_DEALLOCATE until it outgrows it.
    hkInplaceArray(int32_t size = 0) {
        this->m_data = m_storage;
        this->m_size = size;
        this->m_capacityAndFlags = N | hkArrayBase<T>::DONT_DEALLOCATE_FLAG;
    }

    // The SDK's empty ~hkInplaceArray: hkArray's destructor frees the storage once the array has left m_storage.
    ~hkInplaceArray() {}

    // Whether the array has grown out of the inline storage.
    hkBool wasReallocated() const {
        hkBool kResult;
        kResult.m_bool = (this->m_data != m_storage);
        return kResult;
    }

    // The flag word's DONT_DEALLOCATE bit: set while the array still uses the inline storage.
    int32_t stillInplaceUsingMask() const {
        return static_cast<int32_t>(this->m_capacityAndFlags & hkArrayBase<T>::DONT_DEALLOCATE_FLAG);
    }

    T m_storage[N];
};

// An hkInplaceArray whose inline storage is 16-byte aligned: a padding word after the array's three, then the storage.
template <typename T, int32_t N>
class hkInplaceArrayAligned16 : public hkArray<T> {
public:
    // The array of size elements over the inline storage, which it never frees.
    hkInplaceArrayAligned16(int32_t size = 0) {
        this->m_data = reinterpret_cast<T*>(m_storage);
        this->m_size = size;
        this->m_capacityAndFlags = N | hkArrayBase<T>::DONT_DEALLOCATE_FLAG;
    }

    // Whether the array has grown out of the inline storage.
    hkBool wasReallocated() const {
        hkBool kResult;
        kResult.m_bool = (reinterpret_cast<uintptr_t>(this->m_data) != reinterpret_cast<uintptr_t>(m_storage));
        return kResult;
    }

    int32_t m_padding;
    alignas(16) uint8_t m_storage[sizeof(T) * N];
};

using hkInplaceArray_hkpEntity = hkInplaceArray<hkpEntity*, 1>;

template <typename T>
inline int32_t hkArrayBase<T>::getCapacity() const {
    return static_cast<int32_t>(m_capacityAndFlags & CAPACITY_MASK);
}

// The element operations dispatch on hkIsPodType: a class element type's instance needs hkArrayUtil's class operations,
// which are defined only per element type (hkArrayUtil.h), so such an instance either has them or its own explicit
// specialization above.
template <typename T>
inline void hkArrayBase<T>::_reserveExactly(hkMemoryAllocator& alloc, int32_t n) {
    if (getCapacity() < n) {
        hkArrayUtil::_reserve(alloc, this, n, sizeof(T));
    }
}

template <typename T>
inline void hkArrayBase<T>::_reserve(hkMemoryAllocator& alloc, int32_t n) {
    const int32_t capacity = getCapacity();
    if (capacity < n) {
        int32_t cap2 = 2 * capacity;
        int32_t newSize = (n < cap2) ? cap2 : n;
        hkArrayUtil::_reserve(alloc, this, newSize, sizeof(T));
    }
}

template <typename T>
inline void hkArrayBase<T>::clear() {
    hkArrayUtil::destruct(m_data, m_size, typename hkIsPodType<T>::type());
    m_size = 0;
}

template <typename T>
inline void hkArrayBase<T>::_clearAndDeallocate(hkMemoryAllocator& alloc) {
    clear();
    if ((m_capacityAndFlags & DONT_DEALLOCATE_FLAG) == 0) {
        alloc._bufFree<T>(m_data, getCapacity());
    }
    m_data = nullptr;
    m_capacityAndFlags = DONT_DEALLOCATE_FLAG;
}

template <typename T>
inline void hkArrayBase<T>::_optimizeCapacity(hkMemoryAllocator& alloc, int32_t numFreeElemsLeft, int32_t shrinkExact) {
    int32_t totalCapacity = shrinkExact
        ? (m_size + numFreeElemsLeft)
        : static_cast<int32_t>(hkNextPowerOf2(static_cast<uint32_t>(m_size + numFreeElemsLeft)));
    if (totalCapacity < getCapacity()) {
        hkArrayUtil::_reduce(alloc, this, sizeof(T), nullptr, totalCapacity);
    }
}

template <typename T>
inline void hkArrayBase<T>::removeAt(int32_t index) {
    hkArrayUtil::destruct(&m_data[index], 1, typename hkIsPodType<T>::type());
    m_size--;
    if (m_size != index) {
        hkMemUtil::memCpyOneAligned<sizeof(T), alignof(T)>(m_data + index, m_data + m_size);
    }
}

template <typename T>
inline void hkArrayBase<T>::removeAtAndCopy(int32_t index) {
    hkArrayUtil::destruct(&m_data[index], 1, typename hkIsPodType<T>::type());
    m_size--;
    hkMemUtil::memCpy<alignof(T)>(
        m_data + index, m_data + index + 1, static_cast<int32_t>((m_size - index) * sizeof(T))
    );
}

template <typename T>
inline void hkArrayBase<T>::removeAtAndCopy(int32_t index, int32_t numToRemove) {
    hkArrayUtil::destruct(m_data + index, numToRemove, typename hkIsPodType<T>::type());
    m_size -= numToRemove;
    hkMemUtil::memCpy<alignof(T)>(
        m_data + index, m_data + index + numToRemove, static_cast<int32_t>((m_size - index) * sizeof(T))
    );
}

template <typename T>
inline void hkArrayBase<T>::_setSize(hkMemoryAllocator& alloc, int32_t n) {
    _reserve(alloc, n);
    hkArrayUtil::destruct(m_data + n, m_size - n, typename hkIsPodType<T>::type());
    hkArrayUtil::construct(m_data + m_size, n - m_size, typename hkIsPodType<T>::type());
    m_size = n;
}

template <typename T>
inline T* hkArrayBase<T>::_expandBy(hkMemoryAllocator& alloc, int32_t n) {
    int32_t oldsize = m_size;
    _reserve(alloc, m_size + n);
    hkArrayUtil::construct(m_data + m_size, n, typename hkIsPodType<T>::type());
    m_size += n;
    return m_data + oldsize;
}

template <typename T>
inline void
hkArrayBase<T>::_spliceInto(hkMemoryAllocator& alloc, int32_t index, int32_t numdel, const T* p, int32_t numtoinsert) {
    const int32_t newsize = numtoinsert + m_size - numdel;
    const int32_t numtomove = m_size - index - numdel;
    if (newsize > getCapacity()) {
        _reserve(alloc, newsize);
    }
    hkArrayUtil::destruct(m_data + index, numdel, typename hkIsPodType<T>::type());
    hkMemUtil::memMove(
        m_data + index + numtoinsert, m_data + index + numdel, static_cast<int32_t>(numtomove * sizeof(T))
    );
    hkArrayUtil::constructWithArray(m_data + index, numtoinsert, p, typename hkIsPodType<T>::type());
    m_size = newsize;
}

template <typename T>
inline void hkArrayBase<T>::_append(hkMemoryAllocator& alloc, const T* a, int32_t numtoinsert) {
    const int32_t newsize = m_size + numtoinsert;
    if (newsize > getCapacity()) {
        _reserve(alloc, newsize);
    }
    hkArrayUtil::constructWithArray(m_data + m_size, numtoinsert, a, typename hkIsPodType<T>::type());
    m_size = newsize;
}

template <typename T>
inline void hkArrayBase<T>::copy(T* dst, const T* src, int32_t n) {
    for (int32_t i = 0; i < n; ++i) {
        dst[i] = src[i];
    }
}

template <typename T>
inline hkArrayBase<T>& hkArrayBase<T>::copyFromArray(hkMemoryAllocator& alloc, const hkArrayBase& a, hkTypeIsPod) {
    if (getCapacity() < a.getSize()) {
        if ((m_capacityAndFlags & DONT_DEALLOCATE_FLAG) == 0) {
            alloc._bufFree<T>(m_data, getCapacity());
        }
        int32_t n = a.getSize();
        m_data = alloc._bufAlloc<T>(n);
        m_capacityAndFlags = n;
    }
    m_size = a.getSize();
    copy(m_data, a.m_data, m_size);
    return *this;
}

template <typename T>
inline hkArrayBase<T>& hkArrayBase<T>::copyFromArray(hkMemoryAllocator& alloc, const hkArrayBase& a, hkTypeIsClass) {
    int32_t oldSize = m_size;
    int32_t newSize = a.getSize();
    int32_t copiedSize = newSize > oldSize ? oldSize : newSize;
    _reserve(alloc, newSize);
    hkArrayUtil::destruct(m_data + newSize, oldSize - newSize, hkTypeIsClass());
    copy(m_data, a.m_data, copiedSize);
    hkArrayUtil::constructWithArray(m_data + copiedSize, newSize - copiedSize, a.m_data + copiedSize, hkTypeIsClass());
    m_size = newSize;
    return *this;
}

template <typename T>
inline T* hkArrayBase<T>::_expandAt(hkMemoryAllocator& alloc, int32_t index, int32_t numtoinsert) {
    const int32_t newsize = numtoinsert + m_size;
    const int32_t numtomove = m_size - index;
    if (newsize > getCapacity()) {
        _reserve(alloc, newsize);
    }
    hkMemUtil::memMove(m_data + index + numtoinsert, m_data + index, static_cast<int32_t>(numtomove * sizeof(T)));
    hkArrayUtil::construct(m_data + index, numtoinsert, typename hkIsPodType<T>::type());
    m_size = newsize;
    return m_data + index;
}

template <typename T>
template <typename U>
inline void hkArrayBase<T>::_insertAt(hkMemoryAllocator& alloc, int32_t index, const U& t) {
    _insertAt(alloc, index, &t, 1);
}

template <typename T, typename Allocator>
inline void hkArray<T, Allocator>::swap(hkArray<T, Allocator>& a) {
    T* d = this->m_data;
    this->m_data = a.m_data;
    a.m_data = d;
    int32_t s = this->m_size;
    this->m_size = a.m_size;
    a.m_size = s;
    int32_t c = this->m_capacityAndFlags;
    this->m_capacityAndFlags = a.m_capacityAndFlags;
    a.m_capacityAndFlags = c;
}
template <typename T>
template <typename U>
inline const U& hkArrayBase<T>::operator[](int32_t index) const {
    return m_data[index];
}

template <typename T>
template <typename U>
inline int32_t hkArrayBase<T>::indexOf(const U& t, int32_t startIdx, int32_t endIdx) const {
    if (endIdx < 0) {
        endIdx = m_size;
    }
    for (int32_t i = startIdx; i < endIdx; ++i) {
        if (m_data[i] == t) {
            return i;
        }
    }
    return -1;
}

template <typename T>
template <typename U>
inline void hkArrayBase<T>::_pushBack(hkMemoryAllocator& alloc, const U& t) {
    if (m_size == getCapacity()) {
        hkArrayUtil::_reserveMore(alloc, this, sizeof(T));
    }
    hkArrayUtil::constructWithCopy<T>(m_data + m_size, 1, t, typename hkIsPodType<T>::type());
    m_size++;
}

template <typename T, typename Allocator>
template <typename U>
inline void hkArray<T, Allocator>::pushBack(const U& e) {
    hkArrayBase<T>::template _pushBack<U>(AllocatorType().get(this), e);
}

template <typename T>
template <typename U>
inline U& hkArrayBase<T>::_expandOne(hkMemoryAllocator& alloc) {
    if (m_size == getCapacity()) {
        hkArrayUtil::_reserveMore(alloc, this, sizeof(T));
    }
    hkArrayUtil::construct<T>(m_data + m_size, 1, typename hkIsPodType<T>::type());
    return m_data[m_size++];
}

template <typename T, typename Allocator>
template <typename U>
inline U& hkArray<T, Allocator>::expandOne() {
    return hkArrayBase<T>::template _expandOne<U>(AllocatorType().get(this));
}
