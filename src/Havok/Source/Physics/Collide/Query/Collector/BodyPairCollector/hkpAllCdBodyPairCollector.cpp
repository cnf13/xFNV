#include "Havok/Source/Physics/Collide/Query/Collector/BodyPairCollector/hkpAllCdBodyPairCollector.h"

#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"

// Appends the pair of the two bodies' root collidables and shape keys to m_hits.
void hkpAllCdBodyPairCollector::addCdBodyPair(const hkpCdBody& bodyA, const hkpCdBody& bodyB) {
    hkpRootCdBodyPair* pair = &m_hits.expandOne();
    pair->m_rootCollidableA = bodyA.getRootCollidable();
    pair->m_shapeKeyA = bodyA.m_shapeKey;
    pair->m_rootCollidableB = bodyB.getRootCollidable();
    pair->m_shapeKeyB = bodyB.m_shapeKey;
}
