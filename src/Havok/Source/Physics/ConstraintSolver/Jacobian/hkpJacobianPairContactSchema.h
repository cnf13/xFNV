#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"
#include "Reconstruction.hpp"

// Two contact points solved together (hkpconstraintsolver.pdb; the SDK ships no header).
class hkpJacobianPairContactSchema {
public:
	hkpJacDouble2Bil m_jac;
	float m_invJac01;
	float m_padding[3];
};
