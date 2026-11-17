#include "BSHavok/bhkWaterPhantomCallbackShape.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/ahkpWorld.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkWaterListener.h"
#include "BSHavok/bhkWorld.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Bv/hkpBvShape.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/PhantomCallback/hkpPhantomCallbackShape.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

bhkWaterPhantomCallbackShape::bhkWaterPhantomCallbackShape() {}

// Tells the water listener of collidableB's world that the two collidables touch, through a contact point that holds
// only the two root collidables. The world is deliberately not tested before the listener is read, as the game does.
// KEEP: partcopy: the point's m_contact (bytes 0x00-0x1F), m_shapeKeyA/m_shapeKeysA (0x24-0x47) and
// m_shapeKeyB/m_shapeKeysB (0x4C-0x6F) are never written; TESWaterListener::contactPointAddedCallback (0x62F6B0)
// hands the point to BGSZoneTargetListener::AddZoneTargetPair (0x620340), which stores m_contact.m_position in the
// zone target (SetPositionAtHit) and reads m_shapeKeyB.
void bhkWaterPhantomCallbackShape::phantomEnterEvent(const hkpCollidable* collidableA, const hkpCollidable* collidableB,
	const hkpCollisionInput& env) {
	hkpRootCdPoint point;
	point.m_rootCollidableA = collidableA;
	point.m_rootCollidableB = collidableB;
	hkpWorldObject* pwobjb = static_cast<hkpWorldObject*>(collidableB->getOwner());
	hkpWorld* pHkWorld = pwobjb->getWorld();
	// EXE-INLINED 0x621480
	bhkWorld* pworld = pHkWorld ? static_cast<bhkWorld*>(static_cast<ahkpWorld*>(pHkWorld)->getUserData()) : nullptr;
	pworld->QWaterListener()->contactPointAddedCallback(point);
}

// The same for leaving, through the listener's removal callback (which reads only the two root collidables); an
// object with no bhkWorld is skipped. Then a rigid body leaving the water loses its flag 8 (in water).
void bhkWaterPhantomCallbackShape::phantomLeaveEvent(const hkpCollidable* collidableA,
	const hkpCollidable* collidableB) {
	hkpRootCdPoint point;
	point.m_rootCollidableA = collidableA;
	point.m_rootCollidableB = collidableB;
	hkpWorldObject* pwobjb = static_cast<hkpWorldObject*>(collidableB->getOwner());
	hkpWorld* pHkWorld = pwobjb->getWorld();
	// EXE-INLINED 0x621480
	bhkWorld* pworld = pHkWorld ? static_cast<bhkWorld*>(static_cast<ahkpWorld*>(pHkWorld)->getUserData()) : nullptr;
	if (pworld) {
		pworld->QWaterListener()->contactPointRemovedCallback(point);
	}
	// EXE-INLINED 0x4B59F0
	hkpRigidBody* prb = collidableB->m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY
		? static_cast<hkpRigidBody*>(static_cast<hkpWorldObject*>(collidableB->getOwner()))
		: nullptr;
	if (prb) {
		// EXE-INLINED 0x4B5A80
		bhkRigidBody* pbhkrba = static_cast<bhkRigidBody*>(prb->getUserData());
		if (pbhkrba) {
			// EXE-INLINED 0x628F30
			pbhkrba->iFlags &= ~8u;
		}
	}
}

// Casts the ray against the bounding volume of the parent body's bounding-volume shape and reports a hit to the
// collector with cdBody. The scope is charged to the Havok context.
// KEEP: partcopy: the ray-cast output's m_pad (bytes 0x18-0x1F) is written neither here nor by hkpShape::castRayImpl,
// and the collector's addRayHit copies it out (hkpClosestRayHitCollector::addRayHit 0xCA9440 copies [output+0x18]
// and [output+0x1C] into the collector).
void bhkWaterPhantomCallbackShape::castRayWithCollector(const hkpShapeRayCastInput& input, const hkpCdBody& cdBody,
	hkpRayHitCollector& collector) const {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	const hkpCdBody* pparentBody = cdBody.m_parent;
	if (pparentBody) {
		const hkpShape* pshape = pparentBody->m_shape;
		// EXE-INLINED 0x43B230
		if (pparentBody->m_motion.pState && pshape && pshape->m_type.m_storage == HK_SHAPE_BV) {
			const hkpShape* pboundingShape = static_cast<const hkpBvShape*>(pshape)->m_boundingVolumeShape;
			if (pboundingShape) {
				hkpShapeRayCastOutput rco;
				rco.m_hitFraction = 1.0f;
				rco.m_extraInfo = -1;
				rco.m_shapeKeys[0] = HK_INVALID_SHAPE_KEY;
				rco.m_shapeKeyIndex = 0;
				pboundingShape->castRay(input, rco);
				if (rco.hasHit()) {
					collector.addRayHit(cdBody, rco);
				}
			}
		}
	}
}
