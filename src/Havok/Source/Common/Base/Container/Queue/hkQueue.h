#pragma once

#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"

#include <cstddef>
#include <stdint.h>

class hkpRigidBody;

// Havok's ring buffer of T: the storage, its capacity, the head and tail indices and the number of elements in use.
template <typename T>
class hkQueue {
public:
    hkQueue() : m_data(nullptr), m_capacity(0), m_head(0), m_tail(0), m_elementsInUse(0) {}

    ~hkQueue() {
        if (m_capacity) {
            hkMemoryRouter::getInstance().m_heap->blockFree(m_data, static_cast<int32_t>(sizeof(T)) * m_capacity);
        }
    }

    // The class allocator: blocks of the Havok heap.
    static void* operator new(size_t aSize) noexcept {
        return hkMemoryRouter::getInstance().m_heap->blockAlloc(static_cast<int32_t>(aSize));
    }
    static void operator delete(void* apMemory, size_t aSize) {
        hkMemoryRouter::getInstance().m_heap->blockFree(apMemory, static_cast<int32_t>(aSize));
    }
    static void* operator new(size_t, void* apMemory) {
        return apMemory;
    }
    static void operator delete(void*, void*) noexcept {}

    // Grows the storage to at least aiCapacity elements, keeping the ring's order. Each instantiation used is defined
    // next to the element type's owner.
    void grow(int32_t aiCapacity);
    // Grows the storage to aiCapacity elements (at least twice the old capacity), keeping the ring's order. Returns the
    // new or old capacity, which enqueueInFront returns.
    int32_t setCapacity(int32_t n);
    // Appends element at the tail, growing the storage when full; returns 1.
    int32_t enqueue(const T& element);
    // Inserts element at the head, growing the storage when full; returns the capacity setCapacity left.
    int32_t enqueueInFront(const T& element);
    // Takes the element at the head into data; the head wraps to 0 at the end.
    void dequeue(T& data) {
        data = m_data[m_head];
        if (++m_head == m_capacity) {
            m_head = 0;
        }
        --m_elementsInUse;
    }

    T* m_data;
    int32_t m_capacity;
    int32_t m_head;
    int32_t m_tail;
    int32_t m_elementsInUse;
};

// Copies the ring's elements oldest first into the new storage and frees the old storage. Returns the capacity left.
template <typename T>
int32_t hkQueue<T>::setCapacity(int32_t n) {
    if (m_capacity < n) {
        if (2 * m_capacity >= n) {
            n = 2 * m_capacity;
        }
        const int32_t iElementSize = static_cast<int32_t>(sizeof(T));
        T* pNew = static_cast<T*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(iElementSize * n));
        if (m_data) {
            if (m_elementsInUse) {
                if (m_tail <= m_head) {
                    const int32_t iNumHead = m_capacity - m_head;
                    hkString::memCpy(pNew, m_data + m_head, iElementSize * iNumHead);
                    hkString::memCpy(pNew + iNumHead, m_data, iElementSize * m_tail);
                }
                else {
                    hkString::memCpy(pNew, m_data + m_head, iElementSize * m_elementsInUse);
                }
            }
            m_head = 0;
            m_tail = m_elementsInUse;
        }
        if (m_capacity) {
            hkMemoryRouter::getInstance().m_heap->blockFree(m_data, iElementSize * m_capacity);
        }
        m_data = pNew;
        m_capacity = n;
    }
    return m_capacity;
}

template <typename T>
void hkQueue<T>::grow(int32_t aiCapacity) {
    setCapacity(aiCapacity);
}

// The SDK's enqueue: the storage grows (increaseCapacity: 8, else twice) when full, the tail wraps to 0 at the end.
template <typename T>
int32_t hkQueue<T>::enqueue(const T& element) {
    if (m_elementsInUse >= m_capacity) {
        setCapacity(m_capacity ? 2 * m_capacity : 8);
    }
    if (m_tail == m_capacity) {
        m_tail = 0;
    }
    m_data[m_tail] = element;
    ++m_tail;
    ++m_elementsInUse;
    return 1;
}

// The SDK's enqueueInFront: as enqueue, the head wrapping to the end at 0.
template <typename T>
int32_t hkQueue<T>::enqueueInFront(const T& element) {
    int32_t iResult = m_capacity;
    if (m_elementsInUse >= iResult) {
        iResult = setCapacity(iResult ? 2 * iResult : 8);
    }
    if (!m_head) {
        m_head = m_capacity;
    }
    m_data[--m_head] = element;
    ++m_elementsInUse;
    return iResult;
}
