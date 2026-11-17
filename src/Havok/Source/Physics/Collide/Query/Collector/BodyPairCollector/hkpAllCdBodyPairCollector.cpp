#include "Havok/Source/Physics/Collide/Query/Collector/BodyPairCollector/hkpAllCdBodyPairCollector.h"

#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"

// Appends the pair of the two bodies' root collidables and shape keys to m_hits.
void hkpAllCdBodyPairCollector::addCdBodyPair(const hkpCdBody& bodyA, const hkpCdBody& bodyB) {
    // EXE-INLINED none: hkArray<hkpRootCdBodyPair>::expandOne
    hkMemoryRouter& router = hkMemoryRouter::getInstance();
    if (m_hits.m_size == (m_hits.m_capacityAndFlags & hkArrayBase<hkpRootCdBodyPair>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(router.heap(), &m_hits, sizeof(hkpRootCdBodyPair));
    }
    hkpRootCdBodyPair* pair = m_hits.m_data + m_hits.m_size;
    ++m_hits.m_size;
    // EXE-INLINED 0x624020
    const hkpCdBody* rootA = &bodyA;
    while (rootA->m_parent) {
        rootA = rootA->m_parent;
    }
    pair->m_rootCollidableA = static_cast<const hkpCollidable*>(rootA);
    pair->m_shapeKeyA = bodyA.m_shapeKey;
    // EXE-INLINED 0x624020
    const hkpCdBody* rootB = &bodyB;
    while (rootB->m_parent) {
        rootB = rootB->m_parent;
    }
    pair->m_rootCollidableB = static_cast<const hkpCollidable*>(rootB);
    pair->m_shapeKeyB = bodyB.m_shapeKey;
}
