#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"

// An angular limit between m_minRhs and m_maxRhs.
class hkpJacobian1dAngularLimitsSchema {
public:
	hkp2AngJacobian m_jac;
	float m_minRhs;
	float m_maxRhs;
	float m_tau;
	float m_padding;
};
