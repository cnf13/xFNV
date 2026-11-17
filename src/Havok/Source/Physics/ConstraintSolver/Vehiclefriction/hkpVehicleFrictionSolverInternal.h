#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"

// The vehicle friction solver's working record for one axle: the forward and side Jacobians, the slip velocities,
// virtual masses and the impulses and their limits (hkpconstraintsolver.pdb, sizeof 160; the SDK ships no header).
class hkpVehicleFrictionAxisTmp {
public:
    hkp1Lin2AngJacobian m_forwardJac;
    hkp1Lin2AngJacobian m_sideJac;
    float m_forward_vel;
    float m_forwardSlipVel;
    float m_sideSlipVel;
    float m_wheel_down_force;
    float m_sideVirtMass;
    float m_invSideVirtMass;
    float m_wheelForwardVirtMass;
    float m_invWheelForwardVirtMass;
    float m_forwardImpulse;
    float m_sideImpulse;
    float m_maxForwardImpulse;
    float m_maxSideImpulse;
    float m_invMaxForwardImpulse;
    float m_invMaxSideImpulse;
    float m_restForwardSlipImpulse;
};
