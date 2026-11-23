#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpFixedBufferRayHitCollector.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastCollectorOutput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"

void hkpFixedBufferRayHitCollector::addRayHit(const hkpCdBody& cdBody, const hkpShapeRayCastCollectorOutput& hitInfo) {
    g_FixedBufferRayHitCollectorAddRayHitCallbackFunc(cdBody, hitInfo, this);
}

hkpFixedBufferRayHitCollector::~hkpFixedBufferRayHitCollector() {}

void hkpFixedBufferRayHitCollector::operator delete(void* apMemory) {
    if (apMemory) {
        hkMemoryRouter::getInstance().heap().blockFree(apMemory, sizeof(hkpFixedBufferRayHitCollector));
    }
}

// Takes the next free output while the buffer has room, else the output with the largest hit fraction when the new hit
// is nearer; fills it with the hit, its root collidable and its shape keys, with no bound on the depth.
void hkpFixedBufferRayHitCollector::addRayHitImplementation(
    const hkpCdBody& cdBody,
    const hkpShapeRayCastCollectorOutput& hitInfo,
    hkpFixedBufferRayHitCollector* collector
) {
    hkpWorldRayCastOutput* m_storage_1;
    int32_t m_storage = collector->m_numOutputs.m_storage;
    if (m_storage < collector->m_capacity.m_storage) {
        m_storage_1 = collector->m_nextFreeOutput.m_storage;
        collector->m_numOutputs.m_storage = m_storage + 1;
        collector->m_nextFreeOutput.m_storage = m_storage_1 + 1;
    }
    else {
        hkpWorldRayCastOutput* m_storage_3 = collector->m_rayCastOutputBase.m_storage + 1;
        m_storage_1 = collector->m_rayCastOutputBase.m_storage;
        if (m_storage > 1) {
            int32_t v6 = m_storage - 1;
            do {
                if (m_storage_1->m_hitFraction < static_cast<double>(m_storage_3->m_hitFraction)) {
                    m_storage_1 = m_storage_3;
                }
                ++m_storage_3;
                --v6;
            }
            while (v6);
        }
        if (!(m_storage_1->m_hitFraction > static_cast<double>(hitInfo.m_hitFraction))) {
            return;
        }
    }
    int32_t j;
    m_storage_1->m_hitFraction = hitInfo.m_hitFraction;
    m_storage_1->m_normal = hitInfo.m_normal;
    m_storage_1->m_extraInfo = hitInfo.m_extraInfo;
    const hkpCollidable* m_storage_2 = collector->m_collidableOnPpu.m_storage;
    const hkpCdBody* cdBody_1 = &cdBody;
    if (m_storage_2) {
        // m_rootCollidable is not const because its game users take it non-const, hence the const_cast.
        m_storage_1->m_rootCollidable = const_cast<hkpCollidable*>(m_storage_2);
    }
    else {
        const hkpCdBody* i;
        const hkpCdBody* m_parent = cdBody.m_parent;
        for (i = &cdBody; m_parent; m_parent = m_parent->m_parent) {
            i = m_parent;
        }
        m_storage_1->m_rootCollidable = const_cast<hkpCollidable*>(static_cast<const hkpCollidable*>(i));
    }
    const hkpCdBody* m_parent_1 = cdBody.m_parent;
    for (j = 0; m_parent_1; ++j) {
        m_parent_1 = m_parent_1->m_parent;
    }
    int32_t v13 = j - 1;
    m_storage_1->m_shapeKeys[j] = -1;
    if (j - 1 >= 0) {
        uint32_t* v14 = &m_storage_1->m_shapeKeys[v13];
        do {
            *v14 = cdBody_1->m_shapeKey;
            cdBody_1 = cdBody_1->m_parent;
            --v13;
            --v14;
        }
        while (v13 >= 0);
    }
}
