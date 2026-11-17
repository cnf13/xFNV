#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"
#include "Reconstruction.hpp"

// A linear limit between m_minRhs and m_maxRhs (hkpconstraintsolver.pdb; the SDK ships no header).
class hkpJacobian1dLinearLimitsSchema {
public:
	hkp1Lin2AngJacobian m_jac;
	float m_minRhs;
	float m_maxRhs;
	float m_padding[2];
};
