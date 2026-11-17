#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpFixedBufferCdPointCollector.h"

#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"

// The contact, both root collidables and shape keys, and the shape key path of body B
// (hkpRayHitCollector::shapeKeysFromCdBody's order, with no bound on the depth) go into the hit. No key path is written
// for body A.
void _setHit(hkpRootCdPoint* insertAt, const hkpCdPoint* event) {
    insertAt->m_contact.m_position = event->m_contact.m_position;
    insertAt->m_contact.m_separatingNormal = event->m_contact.m_separatingNormal;
    // EXE-INLINED 0x624020
    const hkpCdBody* rootA = event->m_cdBodyA;
    while (rootA->m_parent) {
        rootA = rootA->m_parent;
    }
    insertAt->m_rootCollidableA = static_cast<const hkpCollidable*>(rootA);
    // EXE-INLINED 0x624020
    const hkpCdBody* rootB = event->m_cdBodyB;
    while (rootB->m_parent) {
        rootB = rootB->m_parent;
    }
    insertAt->m_rootCollidableB = static_cast<const hkpCollidable*>(rootB);
    insertAt->m_shapeKeyA = event->m_cdBodyA->m_shapeKey;
    insertAt->m_shapeKeyB = event->m_cdBodyB->m_shapeKey;
    int32_t depth = 0;
    for (const hkpCdBody* parent = event->m_cdBodyB->m_parent; parent; parent = parent->m_parent) {
        ++depth;
    }
    const hkpCdBody* body = event->m_cdBodyB;
    insertAt->m_shapeKeysB[depth] = HK_INVALID_SHAPE_KEY;
    for (int32_t i = depth - 1; i >= 0; --i) {
        insertAt->m_shapeKeysB[i] = body->m_shapeKey;
        body = body->m_parent;
    }
}

void hkpFixedBufferCdPointCollector::registerDefaultAddCdPointFunction() {
    g_FixedBufferCdPointCollectorAddCdPointCallbackFunc = &hkpFixedBufferCdPointCollector::addCdPointImplementation;
}

// While the buffer has room, the point goes into the next free slot. Once it is full, the furthest kept point
// (hkContactPoint::getDistance) is replaced when the new point is closer, the furthest is searched again, and its
// distance becomes the early-out distance. Each search is written out as an unrolled loop: its trip count and its
// four-at-a-time part compare the addresses signed, the rest of the loop unsigned, which differ from a plain `for (p =
// base + 1; p < end; ++p)` loop where an array wraps the address space.
void hkpFixedBufferCdPointCollector::addCdPointImplementation(
    const hkpCdPoint& event,
    hkpFixedBufferCdPointCollector* collector
) {
    if (collector->m_capacity.m_storage == 0) {
        return;
    }
    if (collector->m_numPoints.m_storage < collector->m_capacity.m_storage) {
        _setHit(collector->m_nextFreePoint.m_storage, &event);
        ++collector->m_numPoints.m_storage;
        ++collector->m_nextFreePoint.m_storage;
        return;
    }
    hkpRootCdPoint* pEnd = collector->m_pointsArrayBase.m_storage + collector->m_numPoints.m_storage;
    hkpRootCdPoint* pFurthest = collector->m_pointsArrayBase.m_storage;
    hkpRootCdPoint* pPoint = pFurthest + 1;
    if (static_cast<intptr_t>(
            reinterpret_cast<uintptr_t>(pEnd) - reinterpret_cast<uintptr_t>(pPoint) + (sizeof(hkpRootCdPoint) - 1)
        ) / static_cast<intptr_t>(sizeof(hkpRootCdPoint)) >=
        4) {
        do {
            for (int32_t i = 0; i < 4; i++) {
                if (pFurthest->m_contact.m_separatingNormal.m_quad[3] <
                    pPoint[i].m_contact.m_separatingNormal.m_quad[3]) {
                    pFurthest = &pPoint[i];
                }
            }
            pPoint += 4;
        }
        while (reinterpret_cast<intptr_t>(pPoint) < reinterpret_cast<intptr_t>(pEnd - 3));
    }
    for (; pPoint < pEnd; ++pPoint) {
        if (pFurthest->m_contact.m_separatingNormal.m_quad[3] < pPoint->m_contact.m_separatingNormal.m_quad[3]) {
            pFurthest = pPoint;
        }
    }
    if (pFurthest->m_contact.m_separatingNormal.m_quad[3] > event.m_contact.m_separatingNormal.m_quad[3]) {
        _setHit(pFurthest, &event);
        pFurthest = collector->m_pointsArrayBase.m_storage;
        pPoint = pFurthest + 1;
        if (static_cast<intptr_t>(
                reinterpret_cast<uintptr_t>(pEnd) - reinterpret_cast<uintptr_t>(pPoint) + (sizeof(hkpRootCdPoint) - 1)
            ) / static_cast<intptr_t>(sizeof(hkpRootCdPoint)) >=
            4) {
            do {
                for (int32_t i = 0; i < 4; i++) {
                    if (pFurthest->m_contact.m_separatingNormal.m_quad[3] <
                        pPoint[i].m_contact.m_separatingNormal.m_quad[3]) {
                        pFurthest = &pPoint[i];
                    }
                }
                pPoint += 4;
            }
            while (reinterpret_cast<intptr_t>(pPoint) < reinterpret_cast<intptr_t>(pEnd - 3));
        }
        for (; pPoint < pEnd; ++pPoint) {
            if (pFurthest->m_contact.m_separatingNormal.m_quad[3] < pPoint->m_contact.m_separatingNormal.m_quad[3]) {
                pFurthest = pPoint;
            }
        }
    }
    float fDistance = pFurthest->m_contact.m_separatingNormal.m_quad[3];
    collector->m_earlyOutDistance = fDistance;
}
