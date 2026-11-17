#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolve.h"

class hkpSolverResults;

// Three-dimensional friction: two linear rows and a twist row (hkpconstraintsolver.pdb; the SDK ships no header).
class hkpJacobian3dFrictionSchema {
public:
	hkpJacTriple2Bil1Ang m_jac;
	float m_invJac01;
	float m_maxImpulse;
	float m_contactRadius;
	hkpSolverResults* m_solverResults;
};

static_assert(sizeof(hkpJacobian3dFrictionSchema) == HK_SIZE_OF_JACOBIAN_3D_FRICTION_SCHEMA);
