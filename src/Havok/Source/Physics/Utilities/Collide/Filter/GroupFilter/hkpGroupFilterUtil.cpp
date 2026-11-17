#include "Havok/Source/Physics/Utilities/Collide/Filter/GroupFilter/hkpGroupFilterUtil.h"

#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

void hkpGroupFilterUtil::disableCollisionsBetweenConstraintBodies(const hkpConstraintInstance* const* constraints,
	int32_t numConstraints, int32_t groupFilterSystemGroup) {
	int32_t v3 = 0;
	int32_t v13 = 0;
	for (int32_t i = 0; v3 < numConstraints; i = v3) {
		const hkpConstraintInstance* v4 = constraints[v3];
		hkpEntity* v5 = v4->m_entities[0];
		hkpEntity* v6 = v4->m_entities[1];
		hkpEntity* v15 = v6;
		if (v5) {
			if (v5->m_collidable.m_shape) {
				if (v6) {
					if (v6->m_collidable.m_shape) {
						uint32_t m_collisionFilterInfo = v5->m_collidable.m_broadPhaseHandle.m_collisionFilterInfo;
						uint32_t v8 = v6->m_collidable.m_broadPhaseHandle.m_collisionFilterInfo;
						int32_t v9 = m_collisionFilterInfo >> 5 & 31;
						int32_t v10 = v8 >> 5 & 31;
						int32_t v11 = m_collisionFilterInfo >> 10 & 31;
						int32_t v12 = v8 >> 10 & 31;
						if (!v9) {
							v9 = v13++;
						}
						if (!v10) {
							v10 = v13++;
						}
						if (v11) {
							v12 = v9;
						} else {
							v11 = v10;
						}
						v5->m_collidable.m_broadPhaseHandle.m_collisionFilterInfo =
							(m_collisionFilterInfo & 31) | 32 * (v9 | 32 * (v11 | groupFilterSystemGroup << 6));
						v15->m_collidable.m_broadPhaseHandle.m_collisionFilterInfo =
							(v8 & 31) | 32 * (v10 | 32 * (v12 | groupFilterSystemGroup << 6));
						v3 = i;
					}
				}
			}
		}
		++v3;
	}
}
