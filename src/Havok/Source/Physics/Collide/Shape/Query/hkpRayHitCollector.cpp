#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"

#include "BSCore/MemoryManager.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"

void hkpRayHitCollector::reset() {
    m_earlyOutHitFraction = 1.0f;
}

// The bodies below the root are collected on the stack (the SDK's 8-deep array), then read back root first.
int32_t hkpRayHitCollector::shapeKeysFromCdBody(uint32_t* buf, int32_t maxKeys, const hkpCdBody& body) {
    const hkpCdBody* bodies[8];
    int32_t i = 0;
    const hkpCdBody* pBody = &body;
    for (const hkpCdBody* pParent = body.m_parent; pParent; pParent = pParent->m_parent) {
        bodies[i++] = pBody;
        pBody = pParent;
    }
    int32_t j = 0;
    for (; i > 0 && j < maxKeys - 1; j++) {
        buf[j] = bodies[--i]->m_shapeKey;
    }
    buf[j] = 0xFFFFFFFF;
    return j + 1;
}

void hkpRayHitCollector::operator delete(void* apMemory) {
    ourFree(apMemory);
}
