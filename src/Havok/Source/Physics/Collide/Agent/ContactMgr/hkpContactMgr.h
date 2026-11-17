#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Reconstruction.hpp"

class hkContactPoint;
class hkpCdBody;
class hkpCollidable;
class hkpConstraintOwner;
class hkpContactPointProperties;
class hkpGskCache;
class hkpProcessCollisionData;
class hkpProcessCollisionInput;
class hkpProcessCollisionOutput;

// The contact manager of a pair of bodies: the interface the collision agents report contact points to. The exe
// has no vtable of its own for this abstract class; its slots 3 to 9 are the PDB's Impl pure virtuals and cleanup.
class hkpContactMgr : public hkReferencedObject {
public:
	enum Type : int32_t {
		TYPE_SIMPLE_CONSTRAINT_CONTACT_MGR = 0x0,
		TYPE_REPORT_CONTACT_MGR = 0x1,
		TYPE_CONVEX_LIST_CONTACT_MGR = 0x2,
		TYPE_NULL_CONTACT_MGR = 0x3,
		TYPE_USER_CONTACT_MGR = 0x4,
		TYPE_MAX = 0x5,
	};

	enum ToiAccept : int32_t {
		TOI_ACCEPT = 0x0,
		TOI_REJECT = 0x1,
	};

	hkpContactMgr() = default;
	// The SDK's constructor (inlined into its derived classes').
	hkpContactMgr(Type type) : m_type(type) {}

	// The ids are hkContactPointId (an unsigned short).
	virtual uint16_t addContactPointImpl(const hkpCdBody& a, const hkpCdBody& b, const hkpProcessCollisionInput& input,
		hkpProcessCollisionOutput& output, const hkpGskCache* contactCache, hkContactPoint& cp) = 0;
	// The result is an hkResult (0 is success).
	virtual hkResult reserveContactPointsImpl(int32_t numPoints) = 0;
	// The constraint owner is an hkCollisionConstraintOwner, Havok's name for hkpConstraintOwner here.
	virtual void removeContactPointImpl(uint16_t cpId, hkpConstraintOwner& constraintOwner) = 0;
	virtual void processContactImpl(const hkpCollidable& a, const hkpCollidable& b,
		const hkpProcessCollisionInput& input, hkpProcessCollisionData& collisionData) = 0;
	// The time of impact is an hkTime (a float).
	virtual ToiAccept addToiImpl(const hkpCdBody& a, const hkpCdBody& b, const hkpProcessCollisionInput& input,
		hkpProcessCollisionOutput& output, float toi, hkContactPoint& cp, const hkpGskCache* gskCache,
		float& projectedVelocity, hkpContactPointProperties& propertiesOut) = 0;
	virtual void removeToiImpl(hkpConstraintOwner& constraintOwner, hkpContactPointProperties& properties) = 0;
	virtual void cleanup() {}

	// The SDK's inline wrappers: each calls its virtual Impl.
	uint16_t addContactPoint(const hkpCdBody& a, const hkpCdBody& b, const hkpProcessCollisionInput& input,
		hkpProcessCollisionOutput& output, const hkpGskCache* contactCache, hkContactPoint& cp) {
		return addContactPointImpl(a, b, input, output, contactCache, cp);
	}

	hkResult reserveContactPoints(int32_t numPoints) { return reserveContactPointsImpl(numPoints); }

	void removeContactPoint(uint16_t cpId, hkpConstraintOwner& constraintOwner) {
		removeContactPointImpl(cpId, constraintOwner);
	}

	void processContact(const hkpCollidable& a, const hkpCollidable& b, const hkpProcessCollisionInput& input,
		hkpProcessCollisionData& collisionData) {
		processContactImpl(a, b, input, collisionData);
	}

	ToiAccept addToi(const hkpCdBody& a, const hkpCdBody& b, const hkpProcessCollisionInput& input,
		hkpProcessCollisionOutput& output, float toi, hkContactPoint& cp, const hkpGskCache* gskCache,
		float& projectedVelocity, hkpContactPointProperties& propertiesOut) {
		return addToiImpl(a, b, input, output, toi, cp, gskCache, projectedVelocity, propertiesOut);
	}

	void removeToi(hkpConstraintOwner& constraintOwner, hkpContactPointProperties& properties) {
		removeToiImpl(constraintOwner, properties);
	}

	Type m_type;
};
