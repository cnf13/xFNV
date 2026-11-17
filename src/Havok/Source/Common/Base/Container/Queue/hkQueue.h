#pragma once

#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkpRigidBody;

// Havok's ring buffer of T: the storage, its capacity, the head and tail indices and the number of elements in use.
// IDA names the members; the PDB has no class of this name.
template <typename T> class hkQueue {
public:
	hkQueue() : m_data(nullptr), m_capacity(0), m_head(0), m_tail(0), m_elementsInUse(0) {}

	~hkQueue() {
		if (m_capacity) {
			hkMemoryRouter::getInstance().m_heap->blockFree(m_data, static_cast<int32_t>(sizeof(T)) * m_capacity);
		}
	}

	// Grows the storage to at least aiCapacity elements, keeping the ring's order. The exe has one out-of-line copy
	// per element type; the instantiations src uses define it next to the element type's owner.
	void grow(int32_t aiCapacity);
	// Grows the storage to aiCapacity elements (at least twice the old capacity), keeping the ring's order. The exe
	// leaves the new or old capacity in eax, which enqueueInFront returns.
	int32_t setCapacity(int32_t n);
	// Appends element at the tail, growing the storage when full; the exe returns 1.
	int32_t enqueue(const T& element);
	// Inserts element at the head, growing the storage when full; the exe returns the capacity setCapacity left.
	int32_t enqueueInFront(const T& element);

	T* m_data;
	int32_t m_capacity;
	int32_t m_head;
	int32_t m_tail;
	int32_t m_elementsInUse;
};

// The SDK's setCapacity (hkQueue.inl): the ring's elements copied oldest first into the new storage, the old storage
// freed. Returns the capacity left, as the exe's copy 0xD056D0 leaves it in eax.
template <typename T> int32_t hkQueue<T>::setCapacity(int32_t n) {
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
				} else {
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

// The exe's copy for hkpRigidBody* (0xE45460) is the SDK's setCapacity with no result.
template <typename T> void hkQueue<T>::grow(int32_t aiCapacity) {
	setCapacity(aiCapacity);
}

// The SDK's enqueue: the storage grows (increaseCapacity: 8, else twice) when full, the tail wraps to 0 at the end.
template <typename T> int32_t hkQueue<T>::enqueue(const T& element) {
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
template <typename T> int32_t hkQueue<T>::enqueueInFront(const T& element) {
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
