#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"

// A one-row bilateral (linear) constraint (hkpconstraintsolver.pdb; the SDK ships no header).
class hkpJacobian1dBilateralSchema {
public:
	hkp1Lin2AngJacobian m_jac;
};

// The bilateral constraint with its own tau and damping (hkpconstraintsolver.pdb; the SDK ships no header).
class hkpJacobian1dBilateralUserTauSchema {
public:
	hkp1Lin2AngJacobian m_jac;
	float m_tau;
	float m_damping;
	float m_padding[2];
};
