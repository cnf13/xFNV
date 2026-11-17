#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"
#include "Reconstruction.hpp"

// An angular limit between m_minRhs and m_maxRhs (hkpconstraintsolver.pdb; the SDK ships no header). The exe's
// hk1dAngularLimitBuildJacobian (0xD6CB30) writes the Jacobian at 0, the bounds at 0x20/0x24 and tau at 0x28 and
// moves the schema pointer on by 0x30.
class hkpJacobian1dAngularLimitsSchema {
public:
	hkp2AngJacobian m_jac;
	float m_minRhs;
	float m_maxRhs;
	float m_tau;
	float m_padding;
};
