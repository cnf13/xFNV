#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkSmallArrayUtil.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/hkMemUtil.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"

#include <stdint.h>

// Frees the storage of a small array of pointers: aiCount pointers at apData go back to the Havok heap.
// hkSmallArray<void*>'s destructor calls it.
void releasePointerSmallArrayMemory(void** apData, int32_t aiCount);

// An array whose size and capacity fit into 16 bits; its storage is allocated and moved by hkSmallArrayUtil, so the
// element type only ever matters to the callers.
template <typename T>
class hkSmallArray {
public:
    enum {
        // The number of elements the low bits of m_capacityAndFlags hold.
        CAPACITY_MASK = 0x3FFF,
        // Set while the array's storage is not owned by the array, so it must not be handed back when the array
        // points at other storage again.
        DONT_DEALLOCATE_FLAG = 0x8000,
        // Kept across a reallocation; the array utilities never clear it.
        LOCKED_FLAG = 0x4000,
        FLAG_MASK = 0xC000,
    };

    hkSmallArray() : m_data(nullptr), m_size(0), m_capacityAndFlags(DONT_DEALLOCATE_FLAG) {}
    ~hkSmallArray() {
        releaseMemory();
    }

    // The number of elements.
    int32_t getSize() const {
        return m_size;
    }
    // The number of elements the storage holds.
    int32_t getCapacity() const {
        return m_capacityAndFlags & CAPACITY_MASK;
    }
    // The element at index, without a bounds check.
    T& operator[](int32_t index) {
        return m_data[index];
    }
    const T& operator[](int32_t index) const {
        return m_data[index];
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
    // Inserts numElems elements copied from p at index i, growing the storage.
    void insertAt(int32_t i, const T* p, int32_t numElems);
    // Inserts a copy of t at index i: insertAt(i, &t, 1).
    void insertAt(int32_t i, const T& t);

    // Appends a copy of t, growing the storage when it is full.
    void pushBack(const T& t) {
        if (m_size == getCapacity()) {
            hkSmallArrayUtil::_reserveMore(this, static_cast<int32_t>(sizeof(T)));
        }
        m_data[m_size++] = t;
    }

    // The index of the first element equal to t, or -1.
    int32_t indexOf(const T& t) const {
        for (int32_t i = 0; i < m_size; ++i) {
            if (m_data[i] == t) {
                return i;
            }
        }
        return -1;
    }

    // Removes the element at index; the last element moves into its place.
    void removeAt(int32_t i) {
        m_size--;
        m_data[i] = m_data[m_size];
    }

    // The SDK's inline releaseMemory: hands the storage back to the calling thread's heap unless the array does not
    // own it.
    void releaseMemory() {
        if ((m_capacityAndFlags & DONT_DEALLOCATE_FLAG) == 0) {
            T* data = m_data;
            int32_t capacity = m_capacityAndFlags & CAPACITY_MASK;
            hkMemoryRouter::getInstance().heap().blockFree(data, static_cast<int32_t>(sizeof(T)) * capacity);
        }
    }

    // The SDK's inline clearAndDeallocate: releases the storage and empties the array, keeping LOCKED_FLAG.
    void clearAndDeallocate() {
        releaseMemory();
        m_data = nullptr;
        m_size = 0;
        m_capacityAndFlags = static_cast<uint16_t>(DONT_DEALLOCATE_FLAG | (m_capacityAndFlags & LOCKED_FLAG));
    }

    // Removes the element at index; the elements after it move down one place, keeping the order (Havok's
    // hkSmallArray.inl, HK_FORCE_INLINE).
    void removeAtAndCopy(int32_t index) {
        m_size--;
        for (int32_t i = index; i < m_size; ++i) {
            m_data[i] = m_data[i + 1];
        }
    }

    T* m_data;
    uint16_t m_size;
    uint16_t m_capacityAndFlags;
};

// The storage grows to twice its capacity, or to the new size when that is larger; the tail moves up and the new
// elements are copied in as bytes.
template <typename T>
void hkSmallArray<T>::insertAt(int32_t i, const T* p, int32_t numElems) {
    const int32_t newSize = m_size + numElems;
    const int32_t numToMove = m_size - i;
    const int32_t capacity = getCapacity();
    if (capacity < newSize) {
        const int32_t doubled = 2 * capacity;
        hkSmallArrayUtil::_reserve(this, newSize < doubled ? doubled : newSize, static_cast<int32_t>(sizeof(T)));
    }
    hkMemUtil::memCpyBackwards(m_data + i + numElems, m_data + i, numToMove * static_cast<int32_t>(sizeof(T)));
    hkMemUtil::memCpyBackwards(m_data + i, p, numElems * static_cast<int32_t>(sizeof(T)));
    m_size = static_cast<uint16_t>(newSize);
}

// One element: insertAt(i, &t, 1).
template <typename T>
void hkSmallArray<T>::insertAt(int32_t i, const T& t) {
    insertAt(i, &t, 1);
}
