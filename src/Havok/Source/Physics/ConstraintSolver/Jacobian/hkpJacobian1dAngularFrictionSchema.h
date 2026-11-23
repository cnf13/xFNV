#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"

// Angular friction about one axis, up to m_maxImpulse (hkpconstraintsolver.pdb; the SDK ships no header).
class hkpJacobian1dAngularFrictionSchema {
public:
    hkp2AngJacobian m_jac;
    float m_maxImpulse;
    float m_padding[3];
};
