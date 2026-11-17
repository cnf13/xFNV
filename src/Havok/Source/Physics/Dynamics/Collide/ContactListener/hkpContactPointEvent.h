#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpCollisionEvent.h"
#include "Reconstruction.hpp"

class hkContactPoint;
class hkpContactPointProperties;
class hkpDynamicsContactMgr;
class hkpVelocityAccumulator;

// A contact point of a collision as hkpContactListener::contactPointCallback sees it: the collision it belongs to,
// which kind of callback this is, the point and its properties, and the pointers a listener may write back (the
// separating velocity, the rotated normal, the shape keys). Layout from the exe (IDA's types).
class hkpContactPointEvent : public hkpCollisionEvent {
public:
	enum Type : int32_t {
		TYPE_TOI = 0,
		TYPE_EXPAND_MANIFOLD = 1,
		TYPE_MANIFOLD = 2,
		TYPE_MANIFOLD_AT_END_OF_STEP = 3,
		TYPE_MANIFOLD_FROM_SAVED_CONTACT_POINT = 4,
	};

	hkpContactPointEvent() = default;

	// The collision the event is about; the caller fills the rest. The exe inlines this (the same stores the
	// manager's own events start with).
	hkpContactPointEvent(CallbackSource aeSource, hkpRigidBody* apBodyA, hkpRigidBody* apBodyB,
		hkpDynamicsContactMgr* apMgr)
		: hkpCollisionEvent(aeSource, apBodyA, apBodyB, apMgr) {}

	Type m_type;
	hkContactPoint* m_contactPoint;
	hkpContactPointProperties* m_contactPointProperties;
	hkBool m_firingCallbacksForFullManifold;
	hkBool m_firstCallbackForFullManifold;
	hkBool m_lastCallbackForFullManifold;
	float* m_separatingVelocity;
	float* m_rotateNormal;
	// The PDB's hkpShapeKey* (a uint32_t per key).
	uint32_t* m_shapeKeyStorage;
	hkpVelocityAccumulator* m_accumulators[2];
};
