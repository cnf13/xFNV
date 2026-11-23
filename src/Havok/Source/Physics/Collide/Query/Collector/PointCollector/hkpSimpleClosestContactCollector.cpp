#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpSimpleClosestContactCollector.h"

#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdPoint.h"
#include "xfnv/Compiler.h"

void hkpSimpleClosestContactCollector::addCdPoint(const hkpCdPoint& event) {
    if (!m_hasHit.m_bool ||
        XFNV_M128_F32(m_hitPoint.m_separatingNormal.m_quad, 3) >
            static_cast<double>(XFNV_M128_F32(event.m_contact.m_separatingNormal.m_quad, 3))) {
        m_hasHit.m_bool = 1;
        m_hitPoint = event.m_contact;
        m_earlyOutDistance = XFNV_M128_F32(event.m_contact.m_separatingNormal.m_quad, 3);
    }
}

void hkpSimpleClosestContactCollector::reset() {
    m_hasHit.m_bool = 0;
    XFNV_M128_F32(m_hitPoint.m_separatingNormal.m_quad, 3) = 0x1.ffffdc0000000p+127f;
    m_earlyOutDistance = 0x1.ffffdc0000000p+127f;
}
