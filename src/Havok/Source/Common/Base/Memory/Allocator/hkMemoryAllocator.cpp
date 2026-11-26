#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Util/hkMemUtil.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"

void HK_ASSERT_OBJECT_SIZE_OK_FUNC(uint32_t nbytes) {
    ;
}

// Reads the request through the reference and forwards it to the concrete allocator's blockAlloc, leaving reqBytesInOut
// unchanged.
void* hkMemoryAllocator::bufAlloc(int32_t& reqBytesInOut) {
    return blockAlloc(reqBytesInOut);
}

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

bool hkMemoryAllocator::canAllocSingleBlock(int32_t numBytes) {
    return canAllocTotal(numBytes);
}

void hkMemoryAllocator::bufFreePointers(void* p, int32_t numElem) {
    bufFree(p, sizeof(void*) * numElem);
}

hkMemoryAllocator::~hkMemoryAllocator() {}

void hkMemoryAllocator::bufFreeRayCastOutputs(void* p, int32_t numElem) {
    bufFree(p, sizeof(hkpWorldRayCastOutput) * numElem);
}

void hkMemoryAllocator::bufFreeArray(void* p, int32_t numElem) {
    bufFree(p, numElem);
}

void hkMemoryAllocator::freeHandlePairBuffer(void* p, int32_t numElem) {
    bufFree(p, sizeof(hkpBroadPhaseHandlePair) * numElem);
}
