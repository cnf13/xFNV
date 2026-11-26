#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpClosestRayHitCollector.h"

#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"

// Keeps the hit when it is closer than the one held: the hit's output, its shape keys and its root collidable, and
// the hit's fraction as the new early-out fraction.
void hkpClosestRayHitCollector::addRayHit(const hkpCdBody& cdBody, const hkpShapeRayCastCollectorOutput& hitInfo) {
    if (m_rayHit.m_hitFraction > static_cast<double>(hitInfo.m_hitFraction)) {
        static_cast<hkpShapeRayCastCollectorOutput&>(m_rayHit) = hitInfo;
        hkpRayHitCollector::shapeKeysFromCdBody(m_rayHit.m_shapeKeys, 8, cdBody);
        // m_rootCollidable is not const because the game code that uses it takes it non-const, hence the const_cast.
        m_rayHit.m_rootCollidable = const_cast<hkpCollidable*>(cdBody.getRootCollidable());
        m_earlyOutHitFraction = hitInfo.m_hitFraction;
    }
}
