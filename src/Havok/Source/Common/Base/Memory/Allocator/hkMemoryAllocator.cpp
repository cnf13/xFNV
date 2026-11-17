#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Util/hkMemUtil.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"

void HK_ASSERT_OBJECT_SIZE_OK_FUNC(uint32_t nbytes) {
	;
}

// The original reads the request through the pointer and tail-jumps to the concrete allocator's blockAlloc
// (vtable slot 1), leaving the dereferenced size in the argument slot.
void* hkMemoryAllocator::bufAlloc(int32_t& reqBytesInOut) {
	return blockAlloc(reqBytesInOut);
}

// The original tail-jumps to the concrete allocator's blockFree (vtable slot 2) with the arguments untouched.
void hkMemoryAllocator::bufFree(void* p, int32_t numBytes) {
	blockFree(p, numBytes);
}

void* hkMemoryAllocator::bufRealloc(void* pold, int32_t oldNumBytes, int32_t& reqBytesInOut) {
	void* v5 = bufAlloc(reqBytesInOut);
	int32_t v6 = reqBytesInOut;
	if (oldNumBytes < reqBytesInOut) {
		v6 = oldNumBytes;
	}
	hkMemUtil::memCpy(v5, pold, v6);
	bufFree(pold, oldNumBytes);
	return v5;
}

void hkMemoryAllocator::blockAllocBatch(void** ptrsOut, int32_t numPtrs, int32_t blockSize) {
	for (int32_t i = 0; i < numPtrs; ++i) {
		ptrsOut[i] = blockAlloc(blockSize);
	}
}

void hkMemoryAllocator::blockFreeBatch(void** ptrsIn, int32_t numPtrs, int32_t blockSize) {
	for (int32_t i = 0; i < numPtrs; ++i) {
		void* v6 = ptrsIn[i];
		if (v6) {
			blockFree(v6, blockSize);
		}
	}
}

// The original tail-jumps to the concrete allocator's canAllocTotal.
bool hkMemoryAllocator::canAllocSingleBlock(int32_t numBytes) {
	return canAllocTotal(numBytes);
}

// An instance of the SDK's _bufFree<T> for T = void* (hkArrayBase<void*>::_clearAndDeallocate, 0x62A0B0).
void hkMemoryAllocator::bufFreePointers(void* p, int32_t numElem) {
	bufFree(p, sizeof(void*) * numElem);
}
