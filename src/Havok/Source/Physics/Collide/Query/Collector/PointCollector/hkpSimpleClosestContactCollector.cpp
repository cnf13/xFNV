#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpSimpleClosestContactCollector.h"

#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdPoint.h"

void hkpSimpleClosestContactCollector::addCdPoint(const hkpCdPoint& event) {
    if (!m_hasHit.m_bool ||
        m_hitPoint.m_separatingNormal.m_quad[3] > static_cast<double>(event.m_contact.m_separatingNormal.m_quad[3])) {
        m_hasHit.m_bool = 1;
        m_hitPoint = event.m_contact;
        m_earlyOutDistance = event.m_contact.m_separatingNormal.m_quad[3];
    }
}

void hkpSimpleClosestContactCollector::reset() {
    m_hasHit.m_bool = 0;
    m_hitPoint.m_separatingNormal.m_quad[3] = 0x1.ffffdc0000000p+127f;
    m_earlyOutDistance = 0x1.ffffdc0000000p+127f;
}
