#pragma once

#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Dispatch/ContactMgr/hkpContactMgrFactory.h"
#include "Havok/Source/Physics/Dynamics/Collide/hkpDynamicsContactMgr.h"

#include <stdint.h>

class hkContactPoint;
class hkpCdBody;
class hkpCollidable;
class hkpCollisionInput;
class hkpConstraintOwner;
class hkpContactPointProperties;
class hkpGskCache;
class hkpProcessCollisionInput;
class hkpProcessCollisionOutput;
class hkpRigidBody;
class hkpWorld;

// Reports contacts and time-of-impact events without maintaining a contact constraint.
class hkpReportContactMgr : public hkpDynamicsContactMgr {
public:
	class Factory : public hkpContactMgrFactory {
	public:
		Factory(hkpWorld* a1);

		// Havok SDK signature (hkpDynamics.lib, tied by bytes,vtable).
		hkpContactMgr* createContactMgr(const hkpCollidable& a1, const hkpCollidable& a2,
			const hkpCollisionInput& a3) override;

		hkpWorld* m_world;
	};

	hkpReportContactMgr(hkpWorld* sm, hkpRigidBody* bodyA, hkpRigidBody* bodyB);
	~hkpReportContactMgr() override;

	// Havok SDK signature (hkpDynamics.lib, tied by vtable).
	uint16_t addContactPointImpl(const hkpCdBody& a, const hkpCdBody& b, const hkpProcessCollisionInput& input,
		hkpProcessCollisionOutput& output, const hkpGskCache* contactCache, hkContactPoint& pcp) override;
	hkResult reserveContactPointsImpl(int32_t) override;
	void removeContactPointImpl(uint16_t, hkpConstraintOwner&) override;
	void processContactImpl(const hkpCollidable&, const hkpCollidable&, const hkpProcessCollisionInput&,
		hkpProcessCollisionData&) override;
	// Havok SDK signature (hkpDynamics.lib, tied by bytes,vtable).
	hkpContactMgr::ToiAccept addToiImpl(const hkpCdBody& a1, const hkpCdBody& a2, const hkpProcessCollisionInput& a3,
		hkpProcessCollisionOutput& a4, float a5, hkContactPoint& a6, const hkpGskCache* a7, float& a8,
		hkpContactPointProperties& a9) override;
	// Havok SDK signature (hkpDynamics.lib, tied by bytes,vtable).
	void removeToiImpl(hkpConstraintOwner& constraintOwner, hkpContactPointProperties& properties) override;
	hkpContactPointProperties* getContactPointProperties(uint16_t) override;
	hkContactPoint* getContactPoint(uint16_t) override;
	void getAllContactPointIds(hkArray<uint16_t>&) const override {}
	Type getType() const override;

	void toiCollisionResponseBeginCallback(const hkContactPoint&, hkpSimpleConstraintInfoInitInput&, hkpBodyVelocity&,
		hkpSimpleConstraintInfoInitInput&, hkpBodyVelocity&) override {}

	void toiCollisionResponseEndCallback(const hkContactPoint&, float, hkpSimpleConstraintInfoInitInput&,
		hkpBodyVelocity&, hkpSimpleConstraintInfoInitInput&, hkpBodyVelocity&) override {}

	hkpConstraintInstance* getConstraintInstance() override;
	void fireCallbacksForEarliestToi(hkpToiEvent&, float&) override {}
	void confirmToi(hkpToiEvent&, float, hkArray<hkpEntity*>&) override;

	hkpRigidBody* m_bodyA;
	hkpRigidBody* m_bodyB;
	int16_t m_skipNextNprocessCallbacks;
	uint8_t padding1A[2];
};
