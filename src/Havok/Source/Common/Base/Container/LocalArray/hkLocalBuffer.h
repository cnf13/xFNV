#pragma once

#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

// A pointer and an element count: the block hkLocalBuffer takes from the stack allocator. The SDK's m_debugSize is
// in the exe's layout too, though the release build does not set it.
template <typename T> class hkFixedArray {
public:
	T& operator[](int32_t aiIndex) { return m_data.m_storage[aiIndex]; }
	const T& operator[](int32_t aiIndex) const { return m_data.m_storage[aiIndex]; }
	T* begin() { return m_data.m_storage; }
	const T* begin() const { return m_data.m_storage; }

	hkPadSpu<T*> m_data;
	int32_t m_debugSize;
};

// A fixed array over memory from the stack allocator, which the constructor takes (hkAllocateStack) and the
// destructor gives back (hkDeallocateStack): both inline in the exe at each use, so they are not declared here.
template <typename T> class hkLocalBuffer : public hkFixedArray<T> {
public:
	// A buffer with no stack memory, safe to destroy: some sites declare the local, then construct it again in place
	// (FNVD_RECONSTRUCT destroys it first) or take the stack memory as the exe's inlined constructor does.
	hkLocalBuffer() { this->m_data.m_storage = nullptr; }

	// aiCapacity elements from the thread's stack allocator (the byte count rounded up to 128, as hkAllocateStack
	// does); apWhat names the use.
	hkLocalBuffer(int32_t aiCapacity, const char* apWhat) {
		this->m_data.m_storage = static_cast<T*>(hkMemoryRouter::getInstance()
				.m_stack.allocateStack(static_cast<int32_t>((sizeof(T) * static_cast<size_t>(aiCapacity) + 127) &
										   ~static_cast<size_t>(127)),
					apWhat));
	}

	~hkLocalBuffer() {
		// A default-constructed buffer took no stack memory.
		if (this->m_data.m_storage) {
			hkMemoryRouter::getInstance().m_stack.deallocateStack(this->m_data.m_storage);
		}
	}
};
