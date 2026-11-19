#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpAllCdPointCollector.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"

hkpAllCdPointCollector::hkpAllCdPointCollector() : m_hits(0) {
    hkpAllCdPointCollector::reset();
}

hkpAllCdPointCollector::~hkpAllCdPointCollector() {}

// Havok SDK: appends the point as a root point: the contact, each body's root collidable and shape key, and the
// shape keys from each body up to (not including) its root, leaf first, ended by HK_INVALID_SHAPE_KEY.
void hkpAllCdPointCollector::addCdPoint(const hkpCdPoint& event) {
    hkInplaceArray<hkpRootCdPoint, 8, hkContainerHeapAllocator>* p_m_hits = &m_hits;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_hits->m_size == (p_m_hits->m_capacityAndFlags & hkArrayBase<hkpRootCdPoint>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_hits, sizeof(hkpRootCdPoint));
    }
    hkpRootCdPoint& hit = p_m_hits->m_data[p_m_hits->m_size++];
    hit.m_contact = event.m_contact;
    hit.m_rootCollidableA = event.m_cdBodyA->getRootCollidable();
    hit.m_shapeKeyA = event.m_cdBodyA->m_shapeKey;
    int32_t depthA = 0;
    for (const hkpCdBody* pParent = event.m_cdBodyA->m_parent; pParent; pParent = pParent->m_parent) {
        ++depthA;
    }
    const hkpCdBody* pBodyA = event.m_cdBodyA;
    hit.m_shapeKeysA[depthA] = HK_INVALID_SHAPE_KEY;
    for (int32_t i = depthA - 1; i >= 0; --i) {
        hit.m_shapeKeysA[i] = pBodyA->m_shapeKey;
        pBodyA = pBodyA->m_parent;
    }
    hit.m_rootCollidableB = event.m_cdBodyB->getRootCollidable();
    hit.m_shapeKeyB = event.m_cdBodyB->m_shapeKey;
    int32_t depthB = 0;
    for (const hkpCdBody* pParent = event.m_cdBodyB->m_parent; pParent; pParent = pParent->m_parent) {
        ++depthB;
    }
    const hkpCdBody* pBodyB = event.m_cdBodyB;
    hit.m_shapeKeysB[depthB] = HK_INVALID_SHAPE_KEY;
    for (int32_t i = depthB - 1; i >= 0; --i) {
        hit.m_shapeKeysB[i] = pBodyB->m_shapeKey;
        pBodyB = pBodyB->m_parent;
    }
}

void hkpAllCdPointCollector::addRootCdPoint(const hkpRootCdPoint& a1) {
    hkInplaceArray<hkpRootCdPoint, 8, hkContainerHeapAllocator>* p_m_hits = &m_hits;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_hits->m_size == (p_m_hits->m_capacityAndFlags & hkArrayBase<hkpRootCdPoint>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_hits, sizeof(hkpRootCdPoint));
    }
    hkpRootCdPoint* v4 = &p_m_hits->m_data[p_m_hits->m_size++];
    v4->Assign(a1);
}

hkBool hkpAllCdPointCollector::hasHit() {
    int32_t v2 = (&m_hits)->m_size;
    return v2 > 0;
}

// Sorts the hits by distance (hkpRootCdPoint's operator<) with a quick sort for root points.
void hkpAllCdPointCollector::sortHits() {
    int32_t m_size = m_hits.m_size;
    if (m_size > 1) {
        hkAlgorithm::quickSortRecursive<hkpRootCdPoint, hkAlgorithm::less<hkpRootCdPoint>>(
            m_hits.begin(), 0, m_size - 1, hkAlgorithm::less<hkpRootCdPoint>()
        );
    }
}
