#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"

// Linear friction along one direction, up to m_maxImpulse (hkpconstraintsolver.pdb; the SDK ships no header).
class hkpJacobian1dFrictionSchema {
public:
    hkp1Lin2AngJacobian m_jac;
    float m_maxImpulse;
    float m_padding[3];
};
