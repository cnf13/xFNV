#include "Havok/Source/Physics/Dynamics/Phantom/hkpShapePhantom.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpAabbPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCallbackUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationQueue.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationUtil.h"

// The reflected members of hkpShapePhantom.
using hkpShapePhantomClass_MembersArray = const hkClassMember[1];
static hkpShapePhantomClass_MembersArray hkpShapePhantomClass_Members{{"motionState", &hkMotionStateClass, nullptr,
	{hkClassMember::TYPE_STRUCT}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xB0, nullptr}};

hkClass hkpShapePhantomClass("hkpShapePhantom", &hkpPhantomClass, 352, nullptr, 0, nullptr, 0,
	hkpShapePhantomClass_Members, 1, nullptr, nullptr, 0, 0);

void sub_C9E3E0() {
	;
}

hkpShapePhantom::hkpShapePhantom(hkpShape* a1, hkTransform* a2) : hkpPhantom(a1) {
	hkQuaternion rotation;
	rotation.set(a2->m_rotation);
	m_motionState.initMotionState(a2->m_translation, rotation);
	m_collidable.m_motion.pState = &m_motionState;
}

// Havok SDK: while the world's critical operations are locked, a SetWorldObjectShape operation is queued (POSTPONED);
// otherwise the phantom leaves the broad phase, takes the new shape (its reference swapped), the world's and the
// phantom's listeners are told, and it enters the broad phase again under the world's lock (DONE).
// KEEP (partcopy): hkpWorldOperationQueue::queueOperation copies the whole 20 bytes of a
// BiggestOperation from the 12-byte SetWorldObjectShape local, whose bytes 1-3 (the padding after m_type) nothing
// wrote.
hkWorldOperation::Result hkpShapePhantom::setShape(const hkpShape* shape) {
	hkpWorld* pWorld = m_world;
	if (pWorld) {
		if (pWorld->m_criticalOperationsLockCount) {
			hkWorldOperation::SetWorldObjectShape op;
			// EXE-INLINED none: hkEnum<hkWorldOperation::Type, uint8_t>::operator=
			op.m_type.m_storage = hkWorldOperation::WORLD_OBJECT_SET_SHAPE;
			op.m_worldObject = this;
			op.m_shape = shape;
			pWorld->queueOperation(op);
			return hkWorldOperation::POSTPONED;
		}
		// EXE-INLINED none: hkpWorld::lockCriticalOperations
		++pWorld->m_criticalOperationsLockCount;
		hkpWorldOperationUtil::removePhantomBP(m_world, this);
	}
	shape->addReference();
	const hkpShape* pOldShape = m_collidable.m_shape;
	if (pOldShape) {
		pOldShape->removeReference();
	}
	hkpWorld* pShapeWorld = m_world;
	m_collidable.m_shape = shape;
	if (pShapeWorld) {
		hkpWorldCallbackUtil::firePhantomShapeSet(pShapeWorld, this);
	}
	firePhantomShapeSet();
	hkpWorld* pAddWorld = m_world;
	if (pAddWorld) {
		hkpWorldOperationUtil::addPhantomBP(pAddWorld, this);
		// EXE-INLINED none: hkpWorld::unlockAndAttemptToExecutePendingOperations
		hkpWorld* pUnlockWorld = m_world;
		--pUnlockWorld->m_criticalOperationsLockCount;
		pUnlockWorld->attemptToExecutePendingOperations();
	}
	return hkWorldOperation::DONE;
}

hkMotionState* hkpShapePhantom::getMotionState() {
	return &m_motionState;
}

void hkpShapePhantom::calcAabb(hkAabb& aAabb) {
	float aabb = m_world->m_collisionInput->m_tolerance * 0.5;
	m_collidable.m_shape->getAabbImpl(m_motionState.m_transform, aabb, aAabb);
}

void hkpShapePhantom::deallocateInternalArrays() {
	hkpPhantom::deallocateInternalArrays();
}

void hkpShapePhantom::setPosition(const hkVector4& position, float extraTolerance) {
	hkAabb thisObj;
	hkpWorld* m_world = this->m_world;
	m_motionState.m_transform.m_translation.m_quad = position.m_quad;
	if (m_world) {
		float v5 = m_world->m_collisionInput->m_tolerance * 0.5;
		float v6 = v5 + extraTolerance;
		m_collidable.m_shape->getAabbImpl(m_motionState.m_transform, v6, thisObj);
		updateBroadPhase(thisObj);
	}
}

void hkpShapePhantom::setTransform(const hkTransform& transform) {
	hkAabb thisObj;
	m_motionState.m_transform = transform;
	hkpWorld* m_world = this->m_world;
	if (m_world) {
		float v4 = m_world->m_collisionInput->m_tolerance * 0.5;
		m_collidable.m_shape->getAabbImpl(transform, v4, thisObj);
		updateBroadPhase(thisObj);
	}
}

// The listeners are told from the back, so that one may remove itself while it is told; the null entries left behind
// are then dropped by hkpPhantom::removeNullPhantomListeners.
void hkpShapePhantom::firePhantomShapeSet() {
	for (int32_t i = m_phantomListeners.m_size - 1; i >= 0; --i) {
		if (m_phantomListeners.m_data[i] != nullptr) {
			m_phantomListeners.m_data[i]->phantomShapeSetCallback(this);
		}
	}
	removeNullPhantomListeners();
}

hkpPhantom::hkpPhantom(const hkpShape* shape) : hkpWorldObject(shape, BROAD_PHASE_PHANTOM) {
	m_collidable.setOwner(static_cast<hkpWorldObject*>(this));
}
