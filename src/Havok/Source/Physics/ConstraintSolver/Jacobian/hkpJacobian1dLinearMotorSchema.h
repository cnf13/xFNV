#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverInfo.h"

// A linear motor along one direction (hkpconstraintsolver.pdb; the SDK ships no header).
class hkpJacobian1dLinearMotorSchema {
public:
	hkp1Lin2AngJacobian m_jac;
	hkp1dMotorSolverInfo m_motorSolverInfo;
	float m_padding[3];
};
