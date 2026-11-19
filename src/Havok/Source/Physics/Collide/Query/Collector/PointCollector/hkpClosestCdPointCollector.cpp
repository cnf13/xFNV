#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpClosestCdPointCollector.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "xfnv/Compiler.h"

hkpClosestCdPointCollector::~hkpClosestCdPointCollector() {}

void hkpClosestCdPointCollector::reset() {
    m_hitPoint.m_rootCollidableA = nullptr;
    XFNV_M128_F32(m_hitPoint.m_contact.m_separatingNormal.m_quad, 3) = 0x1.ffffdc0000000p+127f;
    m_earlyOutDistance = 0x1.ffffdc0000000p+127f;
}

// Keeps the point when it is the first or closer than the kept one (hkContactPoint::getDistance), as a root point: the
// contact, each body's root collidable and shape key, and the shape keys from each body up to (not including) its root,
// leaf first, ended by HK_INVALID_SHAPE_KEY; the new distance becomes the early-out distance.
void hkpClosestCdPointCollector::addCdPoint(const hkpCdPoint& event) {
    if (!m_hitPoint.m_rootCollidableA ||
        XFNV_M128_F32(m_hitPoint.m_contact.m_separatingNormal.m_quad, 3) >
            XFNV_M128_F32(event.m_contact.m_separatingNormal.m_quad, 3)) {
        m_hitPoint.m_contact = event.m_contact;
        m_hitPoint.m_rootCollidableA = event.m_cdBodyA->getRootCollidable();
        m_hitPoint.m_shapeKeyA = event.m_cdBodyA->m_shapeKey;
        int32_t depthA = 0;
        for (const hkpCdBody* pParent = event.m_cdBodyA->m_parent; pParent; pParent = pParent->m_parent) {
            ++depthA;
        }
        const hkpCdBody* pBodyA = event.m_cdBodyA;
        m_hitPoint.m_shapeKeysA[depthA] = HK_INVALID_SHAPE_KEY;
        for (int32_t i = depthA - 1; i >= 0; --i) {
            m_hitPoint.m_shapeKeysA[i] = pBodyA->m_shapeKey;
            pBodyA = pBodyA->m_parent;
        }
        m_hitPoint.m_rootCollidableB = event.m_cdBodyB->getRootCollidable();
        m_hitPoint.m_shapeKeyB = event.m_cdBodyB->m_shapeKey;
        int32_t depthB = 0;
        for (const hkpCdBody* pParent = event.m_cdBodyB->m_parent; pParent; pParent = pParent->m_parent) {
            ++depthB;
        }
        const hkpCdBody* pBodyB = event.m_cdBodyB;
        m_hitPoint.m_shapeKeysB[depthB] = HK_INVALID_SHAPE_KEY;
        for (int32_t i = depthB - 1; i >= 0; --i) {
            m_hitPoint.m_shapeKeysB[i] = pBodyB->m_shapeKey;
            pBodyB = pBodyB->m_parent;
        }
        m_earlyOutDistance = XFNV_M128_F32(event.m_contact.m_separatingNormal.m_quad, 3);
    }
}

void hkpClosestCdPointCollector::operator delete(void* apMemory) {
    if (apMemory) {
        hkMemoryRouter::getInstance().heap().blockFree(apMemory, sizeof(hkpClosestCdPointCollector));
    }
}
