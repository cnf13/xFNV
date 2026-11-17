#pragma once

#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// An hkArray whose storage is taken from the thread's stack allocator, with the capacity it was given: it is flagged
// DONT_DEALLOCATE until it grows beyond that. The constructor (hkAllocateStack, then the capacity and the flag) and
// the destructor (clear unless reallocated, then hkDeallocateStack) are inline in the exe at each use.
template <typename T> class hkLocalArray : public hkArray<T> {
public:
	// An array with no stack memory, safe to destroy: some sites declare the local, then construct it again in place
	// (FNVD_RECONSTRUCT destroys it first) or fill its members as the exe's inlined constructor does.
	hkLocalArray() : m_localMemory(nullptr) {}

	// capacity elements from the thread's stack allocator (the byte count rounded up to 128, as hkAllocateStack
	// does), flagged DONT_DEALLOCATE.
	hkLocalArray(int32_t capacity)
		: m_localMemory(static_cast<T*>(hkMemoryRouter::getInstance()
				  .m_stack.allocateStack(static_cast<int32_t>((sizeof(T) * static_cast<size_t>(capacity) + 127) &
											 ~static_cast<size_t>(127)),
					  nullptr))) {
		this->m_data = m_localMemory;
		this->m_capacityAndFlags = capacity | hkArrayBase<T>::DONT_DEALLOCATE_FLAG;
	}

	// The elements are cleared while they are still in the stack memory, which then goes back; ~hkArray frees a
	// reallocated storage.
	~hkLocalArray() {
		if (!wasReallocated()) {
			this->clear();
		}
		// A default-constructed array took no stack memory.
		if (m_localMemory) {
			hkMemoryRouter::getInstance().m_stack.deallocateStack(m_localMemory);
		}
	}

	// Whether the array has grown out of the stack memory it was given.
	hkBool wasReallocated() const {
		hkBool kResult;
		kResult.m_bool = (m_localMemory != this->m_data);
		return kResult;
	}

	// The stack memory the array started with.
	T* m_localMemory;
};
