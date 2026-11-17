#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolve.h"
#include "Reconstruction.hpp"

class hkpSolverResults;

// Two-dimensional friction: two rows solved together, up to m_maxImpulse, with the results it writes back
// (hkpconstraintsolver.pdb; the SDK ships no header).
class hkpJacobian2dFrictionSchema {
public:
	hkpJacDouble2Bil m_jac;
	float m_invJac01;
	float m_maxImpulse;
	hkpSolverResults* m_solverResults;
	// The 4-byte pointer's padding up to HK_SIZE_OF_JACOBIAN_2D_FRICTION_SCHEMA (112 bytes for both pointer sizes,
	// hkpSolve.h): an 8-byte pointer fills the 16-byte lane alone.
#if !FNVD_64BIT
	float m_padding55;
#endif
};

static_assert(sizeof(hkpJacobian2dFrictionSchema) == HK_SIZE_OF_JACOBIAN_2D_FRICTION_SCHEMA);
