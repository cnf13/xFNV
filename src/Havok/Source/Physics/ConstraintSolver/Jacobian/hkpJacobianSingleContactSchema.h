#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"
#include "Reconstruction.hpp"

// One contact point (hkpconstraintsolver.pdb; the SDK ships no header).
class hkpJacobianSingleContactSchema {
public:
	hkp1Lin2AngJacobian m_jac;
};
