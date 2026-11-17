#pragma once

class hkContactPoint;
class hkpRigidBody;
class hkVector4;

// Helpers over a contact constraint's points (calculateSeparatingVelocity). Statics only.
class hkpSimpleContactConstraintUtil {
public:
	// The bodies' relative velocity at cp along its normal.
	static float calculateSeparatingVelocity(const hkpRigidBody* bodyA, const hkpRigidBody* bodyB,
		const hkVector4& centerOfMassInWorldA, const hkVector4& centerOfMassInWorldB, const hkContactPoint* cp);
};
