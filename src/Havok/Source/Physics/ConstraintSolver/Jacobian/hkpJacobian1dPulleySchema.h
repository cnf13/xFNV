#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"
#include "Reconstruction.hpp"

// The pulley's one row over both bodies (hkpconstraintsolver.pdb; the SDK ships no header).
class hkpJacobian1dPulleySchema {
public:
	hkp2Lin2AngJacobian m_jac;
};
