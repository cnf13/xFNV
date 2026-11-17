#include "Havok/Source/Physics/Dynamics/Collide/hkpSimpleConstraintContactMgr.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPointMaterial.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionQualityInfo.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionData.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Contact/hkpContactPointProperties.h"
#include "Havok/Source/Physics/ConstraintSolver/SimpleConstraints/hkpSimpleConstraintUtil.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolve.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactPointEvent.h"
#include "Havok/Source/Physics/Dynamics/Collide/Deprecated/hkpCollisionListener.h"
#include "Havok/Source/Physics/Dynamics/Collide/hkpDynamicsContactMgr.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Atom/hkpConstraintAtomUtil.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Contact/hkpDynamicsCpIdMgr.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Contact/hkpSimpleContactConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityCallbackUtil.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCallbackUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Continuous/hkpContinuousSimulation.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldConstraintUtil.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationUtil.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/hkpGskCache.h"

#include <cmath>
#include <xmmintrin.h>

hkClass hkpSimpleConstraintContactMgrClass("hkpSimpleConstraintContactMgr", &hkReferencedObjectClass, 128, nullptr, 0,
	nullptr, 0, nullptr, 0, nullptr, nullptr, 0, 0);

void sub_CFDB40() {
	;
}

hkpSimpleConstraintContactMgr::Factory::Factory(hkpWorld* mgr) : m_world(mgr) {}

// The SDK: a contact manager for the two collidables' rigid bodies, in this factory's world.
hkpContactMgr* hkpSimpleConstraintContactMgr::Factory::createContactMgr(const hkpCollidable& a, const hkpCollidable& b,
	const hkpCollisionInput& input) {
	hkpRigidBody* bodyA = static_cast<hkpRigidBody*>(a.getOwner());
	hkpRigidBody* bodyB = static_cast<hkpRigidBody*>(b.getOwner());
	// EXE-INLINED none: hkpSimpleConstraintContactMgr::operator new (HK_DECLARE_CLASS_ALLOCATOR: no null test)
	hkpSimpleConstraintContactMgr* pMgr = static_cast<hkpSimpleConstraintContactMgr*>(hkMemoryRouter::getInstance()
			.heap()
			.blockAlloc(sizeof(hkpSimpleConstraintContactMgr)));
	pMgr->m_memSizeAndFlags = sizeof(hkpSimpleConstraintContactMgr);
	return ::new (pMgr) hkpSimpleConstraintContactMgr(m_world, bodyA, bodyB);
}

// The SDK's constructor: the contact constraint between the two bodies, embedded (no size, so never freed on its
// own), with the priority the world's collision quality table gives the two bodies' quality types.
hkpSimpleConstraintContactMgr::hkpSimpleConstraintContactMgr(hkpWorld* sm, hkpRigidBody* bodyA, hkpRigidBody* bodyB)
	: hkpDynamicsContactMgr(hkpContactMgr::TYPE_SIMPLE_CONSTRAINT_CONTACT_MGR), m_reservedContactPoints(0),
	  m_contactPointCallbackDelay(0), m_contactConstraintData(&m_constraint, bodyA, bodyB),
	  m_constraint(hkpConstraintInstance::PRIORITY_PSI) {
	m_constraint.m_entities[0] = bodyA;
	m_contactConstraintData.m_memSizeAndFlags = 0;
	m_constraint.m_memSizeAndFlags = 0;
	m_constraint.m_data = &m_contactConstraintData;
	m_constraint.m_entities[1] = bodyB;
	m_constraint.m_uid = -1;
	m_world = sm;
	m_constraint.setPriority(static_cast<hkpConstraintInstance::ConstraintPriority>(sm->m_collisionDispatcher
			->m_collisionQualityInfo[sm->m_collisionDispatcher
					->m_collisionQualityTable[bodyA->m_collidable.m_broadPhaseHandle.m_objectQualityType]
											 [bodyB->m_collidable.m_broadPhaseHandle.m_objectQualityType]]
			.m_constraintPriority));
}

// Takes the constraint out of the world while it still has contact points; the constraint and its data go with their
// members, without the entities and data the constraint does not own.
hkpSimpleConstraintContactMgr::~hkpSimpleConstraintContactMgr() {
	if (m_contactConstraintData.m_atom->m_numContactPoints) {
		hkpWorldOperationUtil::removeConstraintImmediately(m_world, &m_constraint,
			hkpWorldOperationUtil::DO_FIRE_CALLBACKS);
	}
	m_constraint.m_entities[0] = nullptr;
	m_constraint.m_entities[1] = nullptr;
	m_constraint.m_data = nullptr;
}

const hkClass* hkpSimpleConstraintContactMgr::getClassType() const {
	return &hkpSimpleConstraintContactMgrClass;
}

void hkpSimpleConstraintContactMgr::calcContentStatistics(hkStatisticsCollector* a1, const hkClass* a2) const {
	a1->pushDir("hkpSimpleConstraintContactMgr");
	a1->addNormalChunk("ContactAtom", m_contactConstraintData.m_atom, m_contactConstraintData.m_atom->m_sizeOfAllAtoms,
		0);
	int32_t m_capacityAndFlags = m_contactConstraintData.m_idMgrA.m_values.m_capacityAndFlags;
	if (m_capacityAndFlags >= 0 || a1->m_includeUnmanagedArrayMemory) {
		int32_t AllocatedSize =
			hkStatisticsCollector::getAllocatedSize(m_contactConstraintData.m_idMgrA.m_values.begin(),
				m_capacityAndFlags & 0x3FFFFFFF);
		a1->addChunk(hkStatisticsCollector::MEM_ARRAY, "ContactIds", m_contactConstraintData.m_idMgrA.m_values.begin(),
			m_contactConstraintData.m_idMgrA.m_values.m_size, AllocatedSize);
	}
	a1->popDir();
}

// The SDK: unless the atom is full, a contact point is allocated (the constraint joins the critical locked island
// with its first one), its properties are set and the manifold point added event goes to the world's and both
// entities' listeners. A rejected point is removed again. An accepted one asks for the new contact point callback,
// unless confirmed callbacks may be skipped, nothing restitutes and nobody listens: then its impulse is estimated
// from the bodies' inverse masses and the projected velocity.
uint16_t hkpSimpleConstraintContactMgr::addContactPointImpl(const hkpCdBody& a, const hkpCdBody& b,
	const hkpProcessCollisionInput& input, hkpProcessCollisionOutput& output, const hkpGskCache* contactCache,
	hkContactPoint& cp) {
	hkpSimpleContactConstraintAtom* localAtom = m_contactConstraintData.m_atom;
	if (localAtom->m_numContactPoints + m_reservedContactPoints > localAtom->m_maxNumContactPoints - 2) {
		return 0xFFFF;
	}
	if (!localAtom->m_numContactPoints) {
		output.m_constraintOwner.m_storage->addConstraintToCriticalLockedIsland(&m_constraint);
	}
	hkContactPoint* dcp;
	hkpContactPointProperties* cpi;
	uint16_t id = m_contactConstraintData.allocateContactPoint(*output.m_constraintOwner.m_storage, &dcp, &cpi);
	dcp->m_position.m_quad = cp.m_position.m_quad;
	dcp->m_separatingNormal = cp.m_separatingNormal;
	hkpRigidBody* rbA = setContactPointProperties(a, b, 10000, cpi);
	hkpRigidBody* rbB = hkSelectOther(rbA, m_constraint.getRigidBodyA(), m_constraint.getRigidBodyB());
	hkVector4 velA;
	hkVector4 velB;
	rbA->getPointVelocity(cp.m_position, velA);
	rbB->getPointVelocity(cp.m_position, velB);
	__m128 deltaVel = _mm_sub_ps(velA.m_quad, velB.m_quad);
	cpi->m_impulseApplied = 0.0f;
	__m128 dot = _mm_mul_ps(deltaVel, cp.m_separatingNormal.m_quad);
	cpi->m_userData = 0;
	float projectedVel =
		_mm_shuffle_ps(dot, dot, 170)[0] + static_cast<float>(_mm_shuffle_ps(dot, dot, 85)[0] + dot[0]);
	hkpManifoldPointAddedEvent event(id, this, &input, &output, &a, &b, &cp, contactCache, cpi, projectedVel);
	hkpWorldCallbackUtil::fireContactPointAdded(m_world, event);
	if (rbA->m_contactListeners.m_size) {
		hkpEntityCallbackUtil::fireContactPointAddedInternal(rbA, event);
	}
	if (rbB->m_contactListeners.m_size) {
		hkpEntityCallbackUtil::fireContactPointAddedInternal(rbB, event);
	}
	if (event.m_status == HK_CONTACT_POINT_REJECT) {
		removeContactPointImpl(id, *output.m_constraintOwner.m_storage);
		return 0xFFFF;
	}
	m_contactPointCallbackDelay = event.m_nextProcessCallbackDelay;
	if (input.m_allowToSkipConfirmedCallbacks.m_bool) {
		// EXE-INLINED none: hkContactPointMaterial::getRestitution
		float restitution = cpi->m_restitution * 0.0078125;
		if (restitution == 0.0f && !rbA->m_contactListeners.m_size && !rbB->m_contactListeners.m_size) {
			float invMassB = rbB->getMotionMassInv();
			float sumInvMass = rbA->getMotionMassInv() + invMassB;
			float invSum = 1.0 / (sumInvMass + 1.000000013351432e-10);
			cpi->m_impulseApplied = invSum * -0.20000000298023224 * projectedVel;
			cpi->m_internalSolverData = 0.0f;
			cpi->m_internalDataA = cp.m_separatingNormal.m_quad[3];
			return id;
		}
	}
	output.m_constraintOwner.m_storage->addCallbackRequest(m_contactConstraintData.m_constraint,
		hkpConstraintAtom::CALLBACK_REQUEST_NEW_CONTACT_POINT);
	return id;
}

hkResult hkpSimpleConstraintContactMgr::reserveContactPointsImpl(int32_t a1) {
	if (a1 + m_contactConstraintData.m_atom->m_numContactPoints + m_reservedContactPoints >
		m_contactConstraintData.m_atom->m_maxNumContactPoints - 2) {
		return HK_FAILURE;
	}
	m_reservedContactPoints = static_cast<uint8_t>(a1 + static_cast<uint8_t>(m_reservedContactPoints));
	return HK_SUCCESS;
}

// Fires the contact point removed event (world, then each entity with contact listeners) with the point's properties,
// then frees the point, dropping the constraint from its island when no point is left.
void hkpSimpleConstraintContactMgr::removeContactPointImpl(uint16_t cpId, hkpConstraintOwner& constraintOwner) {
	hkpContactPointProperties* v6;
	hkpContactPointRemovedEvent arEvent;
	hkpEntity* v4 = m_constraint.m_entities[1];
	hkpEntity* v5 = m_constraint.m_entities[0];
	if (m_contactConstraintData.m_idMgrA.m_values.m_data[cpId] == 255) {
		v6 = 0;
	} else {
		// EXE-INLINED none: hkpSimpleContactConstraintAtom::getContactPointPropertiesStream(i). The contact points
		// and then their property blocks follow the atom as untyped memory: block i starts i strides after the
		// reserved contact points.
		hkpSimpleContactConstraintAtom* pAtom = m_contactConstraintData.m_atom;
		const char* pPropsStart = reinterpret_cast<const char*>(reinterpret_cast<const hkContactPoint*>(pAtom + 1) +
			pAtom->m_numReservedContactPoints);
		v6 = reinterpret_cast<hkpContactPointProperties*>(const_cast<char*>(pPropsStart +
			m_contactConstraintData.m_idMgrA.m_values.m_data[cpId] * pAtom->m_contactPointPropertiesStriding));
	}
	arEvent.m_contactPointProperties = v6;
	arEvent.m_contactPointId = cpId;
	hkpWorld* m_world = this->m_world;
	arEvent.m_entityA = v5;
	arEvent.m_entityB = v4;
	arEvent.m_internalContactMgr = (hkpDynamicsContactMgr*)this;
	arEvent.m_constraintOwner = &constraintOwner;
	hkpWorldCallbackUtil::fireContactPointRemoved(m_world, arEvent);
	if (v5->m_contactListeners.m_size) {
		hkpEntityCallbackUtil::fireContactPointRemovedInternal(v5, arEvent);
	}
	if (v4->m_contactListeners.m_size) {
		hkpEntityCallbackUtil::fireContactPointRemovedInternal(v4, arEvent);
	}
	if (!m_contactConstraintData.freeContactPoint(constraintOwner, cpId)) {
		constraintOwner.removeConstraintFromCriticalLockedIsland(&m_constraint);
	}
}

// The SDK: every m_contactPointCallbackDelay calls, the contact process event (with each point's properties) goes
// to the world's and both entities' listeners and the constraint asks for the contact point callback; the delay
// starts again from the smaller of the entities' delays. Then the processed points are copied into the atom's.
void hkpSimpleConstraintContactMgr::processContactImpl(const hkpCollidable& a, const hkpCollidable& b,
	const hkpProcessCollisionInput& input, hkpProcessCollisionData& collisionData) {
	uint16_t delay = m_contactPointCallbackDelay;
	uint16_t newDelay = delay - 1;
	if (!delay) {
		hkpEntity* entityA = static_cast<hkpEntity*>(a.getOwner());
		hkpEntity* entityB = static_cast<hkpEntity*>(b.getOwner());
		// EXE-INLINED none: hkMath::min2
		newDelay = entityA->getContactPointCallbackDelay() < entityB->getContactPointCallbackDelay()
			? entityA->getContactPointCallbackDelay()
			: entityB->getContactPointCallbackDelay();
		hkpContactProcessEvent event;
		event.m_collidableA = &a;
		event.m_collidableB = &b;
		event.m_collisionData = &collisionData;
		event.m_internalContactMgr = this;
		hkpContactPointProperties** cpp = event.m_contactPointProperties;
		hkpProcessCdPoint* end = collisionData.m_firstFreeContactPoint.m_storage;
		for (hkpProcessCdPoint* ccp = collisionData.m_contactPoints; ccp < end; ccp++) {
			// EXE-INLINED 0xCFD410
			uint8_t index = m_contactConstraintData.m_idMgrA.m_values.m_data[ccp->m_contactPointId.m_storage];
			if (index == 0xFF) {
				*cpp = nullptr;
			} else {
				*cpp = m_contactConstraintData.m_atom->getContactPointPropertiesStream(index)->asProperties();
			}
			cpp++;
		}
		hkpWorldCallbackUtil::fireContactProcess(m_world, event);
		if (entityA->m_contactListeners.m_size) {
			hkpEntityCallbackUtil::fireContactProcessInternal(entityA, event);
		}
		if (entityB->m_contactListeners.m_size) {
			hkpEntityCallbackUtil::fireContactProcessInternal(entityB, event);
		}
		collisionData.m_constraintOwner.m_storage->addCallbackRequest(m_contactConstraintData.m_constraint,
			hkpConstraintAtom::CALLBACK_REQUEST_CONTACT_POINT_CALLBACK);
	}
	m_contactPointCallbackDelay = newDelay;
	for (hkpProcessCdPoint* ccp = collisionData.m_contactPoints; ccp < collisionData.m_firstFreeContactPoint.m_storage;
		ccp++) {
		hkContactPoint& dcp =
			m_contactConstraintData.m_atom
				->getContactPoints()[m_contactConstraintData.m_idMgrA.m_values.m_data[ccp->m_contactPointId.m_storage]];
		ccp->m_contact.m_position.storeUncached(&dcp.m_position);
		ccp->m_contact.m_separatingNormal.storeUncached(&dcp.m_separatingNormal);
	}
}

// The SDK: the TOI point's properties are set, the TOI point added event goes to the world's and both entities'
// listeners; a rejected point's TOI is removed again, an accepted one takes the listeners' projected velocity.
hkpContactMgr::ToiAccept hkpSimpleConstraintContactMgr::addToiImpl(const hkpCdBody& a, const hkpCdBody& b,
	const hkpProcessCollisionInput& input, hkpProcessCollisionOutput& output, float toi, hkContactPoint& cp,
	const hkpGskCache* gskCache, float& projectedVelocity, hkpContactPointProperties& properties) {
	hkpRigidBody* rbA = setContactPointProperties(a, b, 7, &properties);
	hkpRigidBody* rbB = hkSelectOther(rbA, m_constraint.getRigidBodyA(), m_constraint.getRigidBodyB());
	hkpToiPointAddedEvent event;
	event.m_bodyA = &a;
	event.m_bodyB = &b;
	event.m_type.m_storage = hkpContactPointAddedEvent::TYPE_TOI;
	event.m_contactPoint = &cp;
	event.m_gskCache = gskCache;
	event.m_contactPointProperties = &properties;
	event.m_projectedVelocity = projectedVelocity;
	event.m_status = HK_CONTACT_POINT_ACCEPT;
	event.m_internalContactMgr = this;
	event.m_collisionInput = &input;
	event.m_collisionOutput = &output;
	event.m_toi = toi;
	hkpWorldCallbackUtil::fireContactPointAdded(m_world, event);
	if (rbA->m_contactListeners.m_size) {
		hkpEntityCallbackUtil::fireContactPointAddedInternal(rbA, event);
	}
	if (rbB->m_contactListeners.m_size) {
		hkpEntityCallbackUtil::fireContactPointAddedInternal(rbB, event);
	}
	if (event.m_status == HK_CONTACT_POINT_REJECT) {
		removeToiImpl(*output.m_constraintOwner.m_storage, properties);
		return static_cast<hkpContactMgr::ToiAccept>(event.m_status);
	}
	projectedVelocity = event.m_projectedVelocity;
	return static_cast<hkpContactMgr::ToiAccept>(event.m_status);
}

// Drops the time of impact's contact point: fires the contact point removed event at the world and at both entities.
// The event's m_callbackFiredFrom is never written (the callback util sets it per entity it tells), so the local is
// deliberately left uninitialised, as the game does. The two entities are the constraint's.
void hkpSimpleConstraintContactMgr::removeToiImpl(hkpConstraintOwner& constraintOwner,
	hkpContactPointProperties& properties) {
	hkpContactPointRemovedEvent event;
	event.m_contactPointId = 0xFFFF;
	event.m_contactPointProperties = &properties;
	hkpEntity* pEntityA = m_constraint.m_entities[0];
	hkpEntity* pEntityB = m_constraint.m_entities[1];
	event.m_entityA = pEntityA;
	event.m_entityB = pEntityB;
	event.m_internalContactMgr = this;
	event.m_constraintOwner = &constraintOwner;
	hkpWorldCallbackUtil::fireContactPointRemoved(m_world, event);
	// Tests the listeners array's size before each fire.
	if (pEntityA->m_contactListeners.getSize() > 0) {
		hkpEntityCallbackUtil::fireContactPointRemovedInternal(pEntityA, event);
	}
	if (pEntityB->m_contactListeners.getSize() > 0) {
		hkpEntityCallbackUtil::fireContactPointRemovedInternal(pEntityB, event);
	}
}

hkpContactPointProperties* hkpSimpleConstraintContactMgr::getContactPointProperties(uint16_t id) {
	if (m_contactConstraintData.m_idMgrA.m_values.m_data[id] == 255) {
		return 0;
	} else {
		hkpSimpleContactConstraintAtom* pAtom = m_contactConstraintData.m_atom;
		return pAtom->getContactPointPropertiesStream(m_contactConstraintData.m_idMgrA.m_values.m_data[id])
			->asProperties();
	}
}

hkContactPoint* hkpSimpleConstraintContactMgr::getContactPoint(uint16_t id) {
	return &m_contactConstraintData.m_atom->getContactPoints()[m_contactConstraintData.m_idMgrA.m_values.m_data[id]];
}

// The SDK: the ids the id manager has in use.
void hkpSimpleConstraintContactMgr::getAllContactPointIds(hkArray<uint16_t, hkContainerHeapAllocator>& contactPointIds)
	const {
	m_contactConstraintData.m_idMgrA.getAllUsedIds(contactPointIds);
}

hkpContactMgr::Type hkpSimpleConstraintContactMgr::getType() const {
	return TYPE_SIMPLE_CONSTRAINT_CONTACT_MGR;
}

void hkpSimpleConstraintContactMgr::toiCollisionResponseBeginCallback(const hkContactPoint& a1,
	hkpSimpleConstraintInfoInitInput& a2, hkpBodyVelocity& a3, hkpSimpleConstraintInfoInitInput& a4,
	hkpBodyVelocity& a5) {
	m_contactConstraintData.collisionResponseBeginCallback(a1, a2, a3, a4, a5);
}

void hkpSimpleConstraintContactMgr::toiCollisionResponseEndCallback(const hkContactPoint& a1, float a2,
	hkpSimpleConstraintInfoInitInput& a3, hkpBodyVelocity& a4, hkpSimpleConstraintInfoInitInput& a5,
	hkpBodyVelocity& a6) {
	m_contactConstraintData.collisionResponseEndCallback(a1, a2, a3, a4, a5, a6);
}

hkpConstraintInstance* hkpSimpleConstraintContactMgr::getConstraintInstance() {
	return &m_constraint;
}

// Reports the earliest toi's contact point to the newer contact listeners (hkpContactListener::contactPointCallback):
// one hkpContactPointEvent of type TYPE_TOI, fired at the world first and then at each of the two entities, m_source
// saying which side is being told. The world is the first entity's, and the event's entity pointers are read from the
// event again after each fire. The event's shape key storage is the address of the toi event's m_extendedUserDatas
// member, and the contact point is the one embedded in the toi event. The padding byte after the three manifold flags
// is left unwritten.
void hkpSimpleConstraintContactMgr::fireCallbacksForEarliestToi(hkpToiEvent& event, float& rotateNormal) {
	hkpContactPointEvent cpEvent(hkpCollisionEvent::SOURCE_WORLD, static_cast<hkpRigidBody*>(event.m_entities[0]),
		static_cast<hkpRigidBody*>(event.m_entities[1]), this);
	cpEvent.m_type = hkpContactPointEvent::TYPE_TOI;
	cpEvent.m_contactPoint = &event.m_contactPoint;
	cpEvent.m_contactPointProperties = &event.m_properties;
	cpEvent.m_firingCallbacksForFullManifold.m_bool = 0;
	cpEvent.m_firstCallbackForFullManifold.m_bool = 0;
	cpEvent.m_lastCallbackForFullManifold.m_bool = 0;
	cpEvent.m_separatingVelocity = &event.m_seperatingVelocity;
	cpEvent.m_rotateNormal = &rotateNormal;
	cpEvent.m_shapeKeyStorage = event.m_extendedUserDatas;
	cpEvent.m_accumulators[0] = nullptr;
	cpEvent.m_accumulators[1] = nullptr;
	hkpWorldCallbackUtil::fireContactPointCallback(static_cast<hkpWorld*>(event.m_entities[0]->m_world), cpEvent);
	cpEvent.m_source = hkpCollisionEvent::SOURCE_A;
	// Tests the listeners array's size before each fire.
	if (event.m_entities[0]->m_contactListeners.getSize() > 0) {
		hkpEntityCallbackUtil::fireContactPointCallbackInternal(event.m_entities[0], cpEvent);
	}
	cpEvent.m_source = hkpCollisionEvent::SOURCE_B;
	if (event.m_entities[1]->m_contactListeners.getSize() > 0) {
		hkpEntityCallbackUtil::fireContactPointCallbackInternal(event.m_entities[1], cpEvent);
	}
}

void hkpSimpleConstraintContactMgr::confirmToi(hkpToiEvent& a1, float a2,
	hkArray<hkpEntity*, hkContainerHeapAllocator>& a3) {
	hkpEntity* v4 = a1.m_entities[0u];
	hkpWorld* m_world = v4->m_world;
	if ((a1.m_properties.m_flags & 8) == 0) {
		uint8_t v7;
		uint8_t m_type = v4->m_motion.m_type.m_storage;
		if ((m_type != 5 && m_type != 4) || ((v7 = a1.m_entities[1]->m_motion.m_type.m_storage, v7 != 5) && v7 != 4)) {
			hkLs_doSimpleCollisionResponse(m_world, a1, a2, a3);
		}
	}
}

// The SDK: the point's friction and restitution are the geometric means of the two entities' materials, its maximum
// impulse is cleared, and the shape keys of the two bodies' hierarchies (up to the atom's counts, together at most
// maxNumExtraDataInEvent) go into the extended user data after the properties. Returns the rigid body of a's root
// collidable, or the constraint's entity B when it has user data and that body is not its entity A.
hkpRigidBody* hkpSimpleConstraintContactMgr::setContactPointProperties(const hkpCdBody& a, const hkpCdBody& b,
	int32_t maxNumExtraDataInEvent, hkpContactPointProperties* cpi) {
	hkpRigidBody* rbA = m_constraint.getRigidBodyA();
	hkpRigidBody* rbB = m_constraint.getRigidBodyB();
	float frictionSq = rbB->m_material.m_friction * rbA->m_material.m_friction;
	float friction = std::sqrt(frictionSq);
	cpi->m_friction = friction;
	float restitutionSq = rbB->m_material.m_restitution * rbA->m_material.m_restitution;
	float restitution = std::sqrt(restitutionSq);
	// EXE-INLINED none: hkContactPointMaterial::setRestitution
	float restitutionScaled = restitution * 128.0;
	cpi->m_restitution = static_cast<uint8_t>(hkMath::hkToIntFast(restitutionScaled));
	cpi->m_maxImpulse.m_value = 0;
	// EXE-INLINED 0x624020
	const hkpCdBody* root = &a;
	while (root->m_parent) {
		root = root->m_parent;
	}
	hkpRigidBody* rbOfA = static_cast<hkpRigidBody*>(static_cast<const hkpCollidable*>(root)->getOwner());
	hkpSimpleContactConstraintAtom* atom = m_contactConstraintData.m_atom;
	int32_t numA = atom->m_numUserDatasForBodyA;
	int32_t numB = atom->m_numUserDatasForBodyB;
	if (numA + numB) {
		const hkpCdBody* cdBodyA;
		const hkpCdBody* cdBodyB;
		if (rbOfA == rbA) {
			cdBodyA = &a;
			cdBodyB = &b;
		} else {
			cdBodyA = &b;
			cdBodyB = &a;
			rbOfA = rbB;
		}
		if (numA >= maxNumExtraDataInEvent) {
			numA = maxNumExtraDataInEvent;
		}
		if (numB >= maxNumExtraDataInEvent - numA) {
			numB = maxNumExtraDataInEvent - numA;
		}
		hkpContactPointProperties::UserData* data = cpi->getStartOfExtendedUserData(atom);
		if (cdBodyA->m_shapeKey == HK_INVALID_SHAPE_KEY && cdBodyA->m_parent) {
			cdBodyA = cdBodyA->m_parent;
		}
		for (int32_t i = 0; cdBodyA && i < numA; i++) {
			data[i] = cdBodyA->m_shapeKey;
			cdBodyA = cdBodyA->m_parent;
		}
		if (cdBodyB->m_shapeKey == HK_INVALID_SHAPE_KEY && cdBodyB->m_parent) {
			cdBodyB = cdBodyB->m_parent;
		}
		for (int32_t i = 0; cdBodyB && i < numB; i++) {
			data[numA + i] = cdBodyB->m_shapeKey;
			cdBodyB = cdBodyB->m_parent;
		}
	}
	return rbOfA;
}

// Removes contact point index and its properties, moving the later ones down by one. The full index is compared with
// the decremented count, which is kept as an int.
void hkpSimpleContactConstraintAtomUtil::removeAtAndCopy(hkpSimpleContactConstraintAtom* atom, int32_t index) {
	const int32_t striding = atom->getContactPointPropertiesStriding();
	const int32_t numContactPoints = atom->m_numContactPoints - 1;
	atom->m_numContactPoints = static_cast<uint16_t>(numContactPoints);
	hkContactPoint* cp = &atom->getContactPoints()[index];
	hkpContactPointPropertiesStream* cpp = atom->getContactPointPropertiesStream(static_cast<int16_t>(index));
	for (int32_t i = index; i < numContactPoints; i++) {
		cp[0] = cp[1];
		// The properties are the atom's untyped stream memory, striding bytes apart.
		// EXE-INLINED none: hkAddByteOffset<hkpContactPointPropertiesStream>
		hkpContactPointPropertiesStream* next =
			reinterpret_cast<hkpContactPointPropertiesStream*>(reinterpret_cast<char*>(cpp) + striding);
		hkString::memCpy4(cpp, next, striding >> 2);
		cp++;
		cpp = next;
	}
}

// The value is compared with relIndex as a signed int after zero extension.
void hkpDynamicsCpIdMgr::decrementValuesGreater(int32_t relIndex) {
	for (int32_t i = m_values.m_size - 1; i >= 0; i--) {
		uint8_t value = m_values.m_data[i];
		if (value != FREE_VALUE && value > relIndex) {
			m_values.m_data[i]--;
		}
	}
}

void hkpDynamicsCpIdMgr::getAllUsedIds(hkArray<uint16_t, hkContainerHeapAllocator>& ids) const {
	for (int32_t i = 0; i < m_values.m_size; ++i) {
		if (m_values.m_data[i] != 0xFF) {
			hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
			if (ids.m_size == (ids.m_capacityAndFlags & hkArrayBase<uint16_t>::CAPACITY_MASK)) {
				hkArrayUtil::_reserveMore(*Instance->m_heap, &ids, sizeof(uint16_t));
			}
			ids.m_data[ids.m_size++] = i;
		}
	}
}

// The point's properties are found as a byte stream: after the reserved points (32 bytes each) at a stride the atom
// stores, so their address is untyped arithmetic.
int32_t hkpSimpleContactConstraintData::freeContactPoint(hkpConstraintOwner& constraintOwner, uint16_t id) {
	// EXE-INLINED none: hkpDynamicsCpIdMgr::indexOf
	int32_t iIndex = m_idMgrA.m_values.m_data[id];
	// EXE-INLINED none: hkpDynamicsCpIdMgr::freeId
	m_idMgrA.m_values.m_data[id] = hkpDynamicsCpIdMgr::FREE_VALUE;
	hkpSimpleContactConstraintAtom* pAtom = m_atom;
	hkpSimpleContactConstraintAtom* pOldAtom = m_atom;
	hkBool atomReallocated;
	atomReallocated.m_bool = false;
	int32_t iNumContactPoints = pAtom->m_numContactPoints;

	// EXE-INLINED none: the solver info of the point removed
	int32_t iSizeOfSchemas = 0;
	int32_t iNumSolverResults = 0;
	int32_t iNumSolverElemTemps = 0;
	if (iNumContactPoints == 2) {
		// The second point's 3D friction schema goes back to the 2D one.
		iSizeOfSchemas = HK_SIZE_OF_JACOBIAN_3D_FRICTION_SCHEMA - HK_SIZE_OF_JACOBIAN_2D_FRICTION_SCHEMA;
		iNumSolverResults = 1;
		iNumSolverElemTemps = 1;
	}
	iNumSolverResults += 1;
	if (iNumContactPoints & 1) {
		iSizeOfSchemas += 0x30;
	} else {
		iSizeOfSchemas += 0x40;
	}
	iNumSolverElemTemps += 1;

	hkpSimpleContactConstraintAtomUtil::removeAtAndCopy(pAtom, iIndex);
	uint8_t* pucProperties = reinterpret_cast<uint8_t*>(pAtom) + sizeof(hkpSimpleContactConstraintAtom) +
		pAtom->m_numReservedContactPoints * 0x20 +
		static_cast<int16_t>(iIndex) * pAtom->m_contactPointPropertiesStriding;
	reinterpret_cast<hkpContactPointProperties*>(pucProperties)->m_flags &=
		~hkContactPointMaterial::CONTACT_USES_SOLVER_PATH2;

	hkpSimpleContactConstraintAtom* pNewAtom =
		hkpSimpleContactConstraintAtomUtil::optimizeCapacity(m_atom, 1, atomReallocated);
	m_atom = pNewAtom;
	m_idMgrA.decrementValuesGreater(iIndex);

	// EXE-INLINED none: hkpConstraintOwner::subConstraintInfo
	if (m_constraint->m_internal) {
		constraintOwner.m_constraintInfo.m_sizeOfSchemas -= iSizeOfSchemas;
		constraintOwner.m_constraintInfo.m_numSolverResults -= iNumSolverResults;
		constraintOwner.m_constraintInfo.m_numSolverElemTemps -= iNumSolverElemTemps;
		hkConstraintInternal* pInternal = m_constraint->m_internal;
		pInternal->m_numSolverResults -= iNumSolverResults;
		pInternal->m_numSolverElemTemps -= iNumSolverElemTemps;
		pInternal->m_sizeOfSchemas -= iSizeOfSchemas;
	}
	if (atomReallocated.m_bool) {
		hkpWorldConstraintUtil::updateFatherOfMovedAtom(m_constraint, pOldAtom, m_atom, m_atom->m_sizeOfAllAtoms);
		m_atomSize = m_atom->m_sizeOfAllAtoms;
	}
	pNewAtom->m_info.m_flags |= hkpSimpleContactConstraintDataInfo::HK_FLAG_POINT_REMOVED |
		hkpSimpleContactConstraintDataInfo::HK_FLAG_AREA_CHANGED;
	return iNumContactPoints - 1;
}
