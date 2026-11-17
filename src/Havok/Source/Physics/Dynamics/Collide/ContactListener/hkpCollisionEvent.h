#pragma once

#include "Reconstruction.hpp"

class hkpDynamicsContactMgr;
class hkpRigidBody;

// A collision between two rigid bodies as the contact listeners see it: which body's listeners are being told, the
// two bodies, and the contact manager that holds the contact points. The layout is the PDB's (m_source at 0, the
// two bodies at 4, the manager at 0xC, 16 bytes); the exe inlines the constructor, the source of the enum values.
class hkpCollisionEvent {
public:
	enum CallbackSource : int32_t {
		SOURCE_A = 0,
		SOURCE_B = 1,
		SOURCE_WORLD = 2,
	};

	// For callers that fill the fields themselves (the exe inlines both forms).
	hkpCollisionEvent() = default;

	hkpCollisionEvent(CallbackSource aeSource, hkpRigidBody* apBodyA, hkpRigidBody* apBodyB,
		hkpDynamicsContactMgr* apMgr)
		: m_source(aeSource), m_contactMgr(apMgr) {
		m_bodies[0] = apBodyA;
		m_bodies[1] = apBodyB;
	}

	hkpRigidBody* getBody(int32_t bodyIdx) const { return m_bodies[bodyIdx]; }

	CallbackSource m_source;
	hkpRigidBody* m_bodies[2];
	hkpDynamicsContactMgr* m_contactMgr;
};
