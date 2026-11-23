#include "BSHavok/bhkWaterPhantomCallbackShape.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/ahkpWorld.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkWaterListener.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "Fallout Shared/GlobalFunc.h"
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
// only the two root collidables. The world is deliberately not tested before the listener is read.
void bhkWaterPhantomCallbackShape::phantomEnterEvent(
    const hkpCollidable* collidableA,
    const hkpCollidable* collidableB,
    const hkpCollisionInput& env
) {
    hkpRootCdPoint point;
    point.m_rootCollidableA = collidableA;
    point.m_rootCollidableB = collidableB;
    // At the origin with no sub-shape key (a departure): AddZoneTargetPair stores the position in the zone target and
    // looks up the material of shape key B, so with no key it takes the water shape's own material; the exe leaves
    // both unset.
    point.m_contact.m_position.setZero4();
    point.m_shapeKeyA = HK_INVALID_SHAPE_KEY;
    point.m_shapeKeyB = HK_INVALID_SHAPE_KEY;
    hkpWorldObject* pwobjb = static_cast<hkpWorldObject*>(collidableB->getOwner());
    hkpWorld* pHkWorld = pwobjb->getWorld();
    bhkWorld* pworld = bhkWorld::Getbhk(pHkWorld);
    pworld->QWaterListener()->contactPointAddedCallback(point);
}

// The same for leaving, through the listener's removal callback (which reads only the two root collidables); an
// object with no bhkWorld is skipped. Then a rigid body leaving the water loses its flag 8 (in water).
void bhkWaterPhantomCallbackShape::phantomLeaveEvent(
    const hkpCollidable* collidableA,
    const hkpCollidable* collidableB
) {
    hkpRootCdPoint point;
    point.m_rootCollidableA = collidableA;
    point.m_rootCollidableB = collidableB;
    hkpWorldObject* pwobjb = static_cast<hkpWorldObject*>(collidableB->getOwner());
    hkpWorld* pHkWorld = pwobjb->getWorld();
    bhkWorld* pworld = bhkWorld::Getbhk(pHkWorld);
    if (pworld) {
        pworld->QWaterListener()->contactPointRemovedCallback(point);
    }
    hkpRigidBody* prb = hkpGetRigidBody(collidableB);
    if (prb) {
        bhkRigidBody* pbhkrba = static_cast<bhkRigidBody*>(bhkEntity::Getbhk(prb));
        if (pbhkrba) {
            pbhkrba->SetFlags(bhkWorldObject::UNDERWATER, false);
        }
    }
}

// Casts the ray against the bounding volume of the parent body's bounding-volume shape and reports a hit to the
// collector with cdBody. The scope is charged to the Havok context.
// KEEP: partcopy: the ray-cast output's m_pad is written neither here nor by hkpShape::castRayImpl, and the
// collector's addRayHit copies it out (hkpClosestRayHitCollector::addRayHit copies the whole of m_pad into the
// collector).
void bhkWaterPhantomCallbackShape::castRayWithCollector(
    const hkpShapeRayCastInput& input,
    const hkpCdBody& cdBody,
    hkpRayHitCollector& collector
) const {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    const hkpCdBody* pparentBody = cdBody.m_parent;
    if (pparentBody) {
        const hkpShape* pshape = pparentBody->m_shape;
        if (pparentBody->getMotionState() && pshape && pshape->m_type.m_storage == HK_SHAPE_BV) {
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
