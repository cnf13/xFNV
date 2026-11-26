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
    hkAlgorithm::quickSort(m_hits.m_data, m_hits.m_size);
}
