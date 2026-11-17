#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

// The interface of the large-block allocator a free-list allocator takes its big blocks from: alloc/free, validity
// and size queries, and the critical variants that may dip into reserved memory.
class hkFreeListAllocatorServer {
public:
	virtual ~hkFreeListAllocatorServer();

	virtual void* alloc(uint32_t auiReqSize) = 0;
	virtual void ourFree(void* apPtr) = 0;
	virtual hkBool isValidAlloc(const void* apPtr) = 0;
	virtual uint32_t getAllocatedSize(const void* apPtr, uint32_t) = 0;
	virtual uint32_t getAllocatedSizeWithOverhead(const void* apPtr, uint32_t) = 0;
	virtual void* criticalAlloc(uint32_t auiReqSize) = 0;
	virtual void criticalFree(void* apPtr, uint32_t auiNumBytes) = 0;

	// DeletingDestruct's free (ourFree).
	static void operator delete(void* apMemory);
};
inline constexpr fnvd::Vtable hkFreeListAllocatorServer_vftable{};
