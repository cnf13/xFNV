#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpAllRayHitCollector.h"

#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastCollectorOutput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"

hkpAllRayHitCollector::hkpAllRayHitCollector() : m_hits(0) {
    reset();
}

// The SDK's addRayHit: m_hits.expandOne(), the collector output copied into the new hit, its shape keys and its root
// collidable.
void hkpAllRayHitCollector::addRayHit(const hkpCdBody& cdBody, const hkpShapeRayCastCollectorOutput& hitInfo) {
    hkpWorldRayCastOutput& rRayHit = m_hits.expandOne();
    static_cast<hkpShapeRayCastCollectorOutput&>(rRayHit) = hitInfo;
    hkpRayHitCollector::shapeKeysFromCdBody(rRayHit.m_shapeKeys, 8, cdBody);
    // m_rootCollidable is not const here because its game users take it non-const, hence the const_cast.
    rRayHit.m_rootCollidable = const_cast<hkpCollidable*>(cdBody.getRootCollidable());
}

hkpAllRayHitCollector::~hkpAllRayHitCollector() {}

// Sorts the hits by hit fraction when there are two or more.
void hkpAllRayHitCollector::sortHits() {
    if (m_hits.m_size > 1) {
        // The empty comparator's byte.
        hkAlgorithm::quickSortRecursive(m_hits.m_data, 0, m_hits.m_size - 1, static_cast<uint8_t>(0));
    }
}

// Hoare partition by hit fraction around a copy of the middle hit, recursing on the left part and looping on the right
// one.
void hkAlgorithm::quickSortRecursive(hkpWorldRayCastOutput* apArr, int32_t aiLow, int32_t aiHigh, uint8_t aiLess) {
    for (;;) {
        int32_t i = aiLow;
        int32_t j = aiHigh;
        const hkpWorldRayCastOutput kPivot(apArr[(aiLow + aiHigh) >> 1]);
        hkAlgorithm::less<hkpWorldRayCastOutput> cmpLess;
        do {
            while (cmpLess(apArr[i], kPivot)) {
                ++i;
            }
            while (cmpLess(kPivot, apArr[j])) {
                --j;
            }
            if (i > j) {
                break;
            }
            if (i != j) {
                hkpWorldRayCastOutput& rLeft = apArr[i];
                hkpWorldRayCastOutput& rRight = apArr[j];
                const hkpWorldRayCastOutput kRight(rRight);
                rRight = rLeft;
                rLeft = kRight;
            }
            ++i;
            --j;
        }
        while (i <= j);
        if (aiLow < j) {
            quickSortRecursive(apArr, aiLow, j, aiLess);
        }
        if (i >= aiHigh) {
            return;
        }
        aiLow = i;
    }
}
