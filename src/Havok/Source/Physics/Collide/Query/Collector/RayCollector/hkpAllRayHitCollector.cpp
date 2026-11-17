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
    hkInplaceArray<hkpWorldRayCastOutput, 8, hkContainerHeapAllocator>* pHits = &m_hits;
    hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
    // EXE-INLINED none: hkArray<hkpWorldRayCastOutput>::expandOne
    if (pHits->m_size == (pHits->m_capacityAndFlags & hkArrayBase<hkpWorldRayCastOutput>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*pRouter->m_heap, pHits, sizeof(hkpWorldRayCastOutput));
    }
    hkpWorldRayCastOutput* pNew = &pHits->m_data[pHits->m_size];
    // EXE-INLINED none: hkpWorldRayCastOutput's constructor (the placement new)
    if (pNew) {
        pNew->m_hitFraction = 1.0f;
        pNew->m_extraInfo = -1;
        pNew->m_shapeKeyIndex = 0;
        pNew->m_shapeKeys[0] = -1;
        pNew->m_rootCollidable = nullptr;
    }
    int32_t iIndex = pHits->m_size;
    pHits->m_size = iIndex + 1;
    hkpWorldRayCastOutput& rRayHit = pHits->m_data[iIndex];
    // EXE-INLINED 0x9C1B00
    rRayHit.m_normal = hitInfo.m_normal;
    rRayHit.m_hitFraction = hitInfo.m_hitFraction;
    rRayHit.m_extraInfo = hitInfo.m_extraInfo;
    rRayHit.m_pad[0] = hitInfo.m_pad[0];
    rRayHit.m_pad[1] = hitInfo.m_pad[1];
    hkpRayHitCollector::shapeKeysFromCdBody(rRayHit.m_shapeKeys, 8, cdBody);
    // EXE-INLINED 0x624020
    const hkpCdBody* pRoot = &cdBody;
    while (pRoot->m_parent) {
        pRoot = pRoot->m_parent;
    }
    // m_rootCollidable is not const here because its game users take it non-const, hence the const_cast.
    rRayHit.m_rootCollidable = const_cast<hkpCollidable*>(static_cast<const hkpCollidable*>(pRoot));
}

hkpAllRayHitCollector::~hkpAllRayHitCollector() {}

// Sorts the hits by hit fraction when there are two or more.
void hkpAllRayHitCollector::sortHits() {
    if (m_hits.m_size > 1) {
        // The empty comparator's byte.
        hkAlgorithm::quickSortRecursive(m_hits.m_data, 0, m_hits.m_size - 1, static_cast<uint8_t>(0));
    }
}

// Hoare partition by m_hitFraction around the middle hit, recursing on the left part and looping on the right one.
void hkAlgorithm::quickSortRecursive(hkpWorldRayCastOutput* apArr, int32_t aiLow, int32_t aiHigh, uint8_t aiLess) {
    for (;;) {
        int32_t i = aiLow;
        int32_t j = aiHigh;
        const float fPivot = apArr[(aiLow + aiHigh) >> 1].m_hitFraction;
        do {
            // EXE-INLINED none: hkAlgorithm::less<hkpWorldRayCastOutput> (the hits' operator<, by m_hitFraction)
            while (apArr[i].m_hitFraction < fPivot) {
                ++i;
            }
            while (fPivot < apArr[j].m_hitFraction) {
                --j;
            }
            if (i > j) {
                break;
            }
            if (i != j) {
                hkpWorldRayCastOutput& rLeft = apArr[i];
                hkpWorldRayCastOutput& rRight = apArr[j];
                // EXE-INLINED 0x9C1A70
                const __m128 kNormal = rRight.m_normal.m_quad;
                const float fHitFraction = rRight.m_hitFraction;
                const int32_t aiPad[2] = {rRight.m_pad[0], rRight.m_pad[1]};
                uint32_t auiShapeKeys[hkpShapeRayCastOutput::MAX_HIERARCHY_DEPTH];
                for (int32_t k = 0; k < hkpShapeRayCastOutput::MAX_HIERARCHY_DEPTH; ++k) {
                    auiShapeKeys[k] = rRight.m_shapeKeys[k];
                }
                const int32_t iShapeKeyIndex = rRight.m_shapeKeyIndex;
                auto* pRootCollidable = rRight.m_rootCollidable;
                // EXE-INLINED 0x9C1A70
                rRight.m_normal.m_quad = rLeft.m_normal.m_quad;
                rRight.m_hitFraction = rLeft.m_hitFraction;
                const int32_t iExtraInfo = rRight.m_extraInfo;
                rRight.m_extraInfo = rLeft.m_extraInfo;
                rRight.m_pad[0] = rLeft.m_pad[0];
                rRight.m_pad[1] = rLeft.m_pad[1];
                for (int32_t k = 0; k < hkpShapeRayCastOutput::MAX_HIERARCHY_DEPTH; ++k) {
                    rRight.m_shapeKeys[k] = rLeft.m_shapeKeys[k];
                }
                rRight.m_shapeKeyIndex = rLeft.m_shapeKeyIndex;
                rRight.m_rootCollidable = rLeft.m_rootCollidable;
                // EXE-INLINED 0x9C1A70
                rLeft.m_hitFraction = fHitFraction;
                rLeft.m_normal.m_quad = kNormal;
                rLeft.m_extraInfo = iExtraInfo;
                rLeft.m_pad[0] = aiPad[0];
                rLeft.m_pad[1] = aiPad[1];
                for (int32_t k = 0; k < hkpShapeRayCastOutput::MAX_HIERARCHY_DEPTH; ++k) {
                    rLeft.m_shapeKeys[k] = auiShapeKeys[k];
                }
                rLeft.m_shapeKeyIndex = iShapeKeyIndex;
                rLeft.m_rootCollidable = pRootCollidable;
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
