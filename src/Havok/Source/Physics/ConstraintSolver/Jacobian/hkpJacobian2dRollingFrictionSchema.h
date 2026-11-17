#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolve.h"

class hkpSolverResults;

// Rolling friction about two axes (hkpconstraintsolver.pdb; the SDK ships no header).
class hkpJacobian2dRollingFrictionSchema {
public:
	hkp2AngJacobian m_jac0;
	hkp2AngJacobian m_jac1;
	float m_maxImpulse;
	hkpSolverResults* m_solverResults;
	hkHalf m_originalRollingFrictionMultiplier;
	hkHalf m_padding1;
	uint32_t m_padding2;
};

static_assert(sizeof(hkpJacobian2dRollingFrictionSchema) == HK_SIZE_OF_JACOBIAN_2D_ROLLING_FRICTION_SCHEMA);
