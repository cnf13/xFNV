#pragma once

#include "Reconstruction.hpp"

class hkContactPoint;
class hkpDynamicsContactMgr;
class hkpMotion;
class hkpRigidBody;
struct hkpSimpleConstraintUtilCollideParams;
class hkpSimpleContactConstraintData;
class hkpVelocityAccumulator;

// Solves a single contact between two bodies outside the solver, for times of impact.
class hkpSimpleCollisionResponse {
public:
	struct SolveSingleOutput {
		float m_impulse;
		float m_velocityKeyframedA;
		float m_velocityKeyframedB;
	};

	// The second solver's output: the impulse applied (hkpdynamics.pdb, sizeof 4).
	struct SolveSingleOutput2 {
		float m_impulse;
	};

	// Applies the impulse that stops the bodies approaching at cp at time, and reports the keyframed bodies'
	// velocities.
	static void solveSingleContact(const hkContactPoint& cp, float time, hkpSimpleConstraintUtilCollideParams& params,
		hkpMotion* bodyA, hkpMotion* bodyB, hkpDynamicsContactMgr* contactMgr, SolveSingleOutput& output);
	// Solves cp against the two accumulators (their motion fields +0x10..+0x70) and calls the constraint data's
	// collision response callbacks (PDB hkpSimpleCollisionResponse::solveSingleContact2; cdecl, ret 0, 0xD9EE50; its
	// one caller is hkSimpleContactConstraintData_fireCallbacks 0xD92900).
	static void solveSingleContact2(hkpSimpleContactConstraintData* constraintData, const hkContactPoint& cp,
		hkpSimpleConstraintUtilCollideParams& params, hkpRigidBody* rigidBodyA, hkpRigidBody* rigidBodyB,
		hkpVelocityAccumulator* bodyA, hkpVelocityAccumulator* bodyB, SolveSingleOutput2& output);
};
