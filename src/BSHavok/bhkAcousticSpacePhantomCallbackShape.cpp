#include "BSHavok/bhkAcousticSpacePhantomCallbackShape.h"

#include "BSHavok/ahkpWorld.h"
#include "BSHavok/bhkEntityListener.h"
#include "BSHavok/bhkWorld.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

// Tells the acoustic space listener of collidableB's world that the two collidables touch, through a contact point that
// holds only the two root collidables (the listener reads nothing else of it). The world is not tested before the
// listener is read, as the game does.
void bhkAcousticSpacePhantomCallbackShape::phantomEnterEvent(const hkpCollidable* collidableA,
	const hkpCollidable* collidableB, const hkpCollisionInput& env) {
	hkpRootCdPoint point;
	point.m_rootCollidableA = collidableA;
	point.m_rootCollidableB = collidableB;
	hkpWorldObject* pwobjb = static_cast<hkpWorldObject*>(collidableB->getOwner());
	hkpWorld* pHkWorld = pwobjb->getWorld();
	// EXE-INLINED 0x621480
	bhkWorld* pworld = pHkWorld ? static_cast<bhkWorld*>(static_cast<ahkpWorld*>(pHkWorld)->getUserData()) : nullptr;
	pworld->QAcousticSpaceListener()->contactPointAddedCallback(point);
}

// The same for leaving, through the listener's removal callback; here an object with no bhkWorld is skipped.
void bhkAcousticSpacePhantomCallbackShape::phantomLeaveEvent(const hkpCollidable* collidableA,
	const hkpCollidable* collidableB) {
	hkpRootCdPoint point;
	point.m_rootCollidableA = collidableA;
	point.m_rootCollidableB = collidableB;
	hkpWorldObject* pwobjb = static_cast<hkpWorldObject*>(collidableB->getOwner());
	hkpWorld* pHkWorld = pwobjb->getWorld();
	// EXE-INLINED 0x621480
	bhkWorld* pworld = pHkWorld ? static_cast<bhkWorld*>(static_cast<ahkpWorld*>(pHkWorld)->getUserData()) : nullptr;
	if (pworld)
		pworld->QAcousticSpaceListener()->contactPointRemovedCallback(point);
}
