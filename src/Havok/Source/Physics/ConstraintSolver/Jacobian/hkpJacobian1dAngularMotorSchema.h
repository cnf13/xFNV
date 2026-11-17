#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverInfo.h"
#include "Reconstruction.hpp"

// An angular motor about one axis (hkpconstraintsolver.pdb; the SDK ships no header).
class hkpJacobian1dAngularMotorSchema {
public:
	hkp2AngJacobian m_jac;
	hkp1dMotorSolverInfo m_motorSolverInfo;
	float m_padding[3];
};
