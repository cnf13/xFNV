#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"

// A one-row angular constraint (hkpconstraintsolver.pdb; the SDK ships no header). 32 bytes in the schema stream.
class hkpJacobian1dAngularSchema {
public:
	hkp2AngJacobian m_jac;
};
