#pragma once

#include "Reconstruction.hpp"

class hkContactPoint;
class hkVector4;
class hkpRigidBody;

// Helpers over a contact constraint's points (calculateSeparatingVelocity). Statics only: sizeof 1 in Havok's
// release PDBs; the game PDB has calculateSeparatingVelocity.
class hkpSimpleContactConstraintUtil {
public:
	// The bodies' relative velocity at cp along its normal; the centres of mass in world are the accumulators'
	// m_scratch0 at the exe's one call (cdecl, ret 0, ST0, 0xDF2510). The SDK's signature.
	static float calculateSeparatingVelocity(const hkpRigidBody* bodyA, const hkpRigidBody* bodyB,
		const hkVector4& centerOfMassInWorldA, const hkVector4& centerOfMassInWorldB, const hkContactPoint* cp);
};
