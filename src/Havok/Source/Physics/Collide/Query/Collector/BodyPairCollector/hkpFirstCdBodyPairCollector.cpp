#include "Havok/Source/Physics/Collide/Query/Collector/BodyPairCollector/hkpFirstCdBodyPairCollector.h"

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/BodyPairCollector/hkpRootCdBodyPair.h"

void hkpFirstCdBodyPairCollector::addCdBodyPair(const hkpCdBody& bodyA, const hkpCdBody& bodyB) {
	const hkpCdBody* i;
	const hkpCdBody* j;
	const hkpCdBody* m_parent = bodyA.m_parent;
	for (i = &bodyA; m_parent; m_parent = m_parent->m_parent) {
		i = m_parent;
	}
	m_cdBodyPair.m_rootCollidableA = static_cast<const hkpCollidable*>(i);
	m_cdBodyPair.m_shapeKeyA = bodyA.m_shapeKey;
	const hkpCdBody* v5 = bodyB.m_parent;
	for (j = &bodyB; v5; v5 = v5->m_parent) {
		j = v5;
	}
	m_cdBodyPair.m_rootCollidableB = static_cast<const hkpCollidable*>(j);
	m_cdBodyPair.m_shapeKeyB = bodyB.m_shapeKey;
	m_earlyOut.m_bool = 1;
}
