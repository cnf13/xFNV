#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Reconstruction.hpp"

struct hkpBodyVelocity;
class hkpConstraintInstance;
class hkpEntity;
struct hkpSimpleConstraintInfoInitInput;
struct hkpToiEvent;
class hkpWorld;

// The base of the contact managers that turn contacts into dynamics (contact constraints) in a world. The exe has
// no vtable of its own for this abstract class; its slots 10 to 18 are the PDB's intro virtuals.
class hkpDynamicsContactMgr : public hkpContactMgr {
public:
	hkpDynamicsContactMgr() = default;
	// The SDK's constructor (inlined into its derived classes'): the manager's type; m_world is set by the caller.
	hkpDynamicsContactMgr(hkpContactMgr::Type type) : hkpContactMgr(type) {}

	// The SDK's inline defaults. The exe has no copy of them: hkpSimpleConstraintContactMgr and hkpReportContactMgr
	// override every slot. The ids are hkContactPointId (an unsigned short).
	virtual hkpContactPointProperties* getContactPointProperties(uint16_t id) { return nullptr; }
	virtual hkContactPoint* getContactPoint(uint16_t id) { return nullptr; }
	virtual void getAllContactPointIds(hkArray<uint16_t>& contactPointIds) const {}
	virtual hkpContactMgr::Type getType() const = 0;

	virtual void toiCollisionResponseBeginCallback(const hkContactPoint& cp, hkpSimpleConstraintInfoInitInput& inA,
		hkpBodyVelocity& velA, hkpSimpleConstraintInfoInitInput& inB, hkpBodyVelocity& velB) {}

	virtual void toiCollisionResponseEndCallback(const hkContactPoint& cp, float impulseApplied,
		hkpSimpleConstraintInfoInitInput& inA, hkpBodyVelocity& velA, hkpSimpleConstraintInfoInitInput& inB,
		hkpBodyVelocity& velB) {}

	virtual hkpConstraintInstance* getConstraintInstance() { return nullptr; }
	virtual void fireCallbacksForEarliestToi(hkpToiEvent& event, float& rotateNormal) {}
	// The time of impact is an hkTime (a float).
	virtual void confirmToi(hkpToiEvent& event, float rotateNormal, hkArray<hkpEntity*>& outToBeActivated) {}

	hkpWorld* m_world;
};
