#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// The constraint solver's settings for one step: stiffness (tau) and damping, the substep count and the per-class
// deactivation thresholds.
struct hkpSolverInfo {
    // A body's deactivation class: which of m_deactivationInfo's thresholds it sleeps by (the SDK's values).
    enum DeactivationClass : int32_t {
        DEACTIVATION_CLASS_INVALID = 0,
        DEACTIVATION_CLASS_OFF = 1,
        DEACTIVATION_CLASS_LOW = 2,
        DEACTIVATION_CLASS_MEDIUM = 3,
        DEACTIVATION_CLASS_HIGH = 4,
        DEACTIVATION_CLASS_AGGRESSIVE = 5,
        DEACTIVATION_CLASSES_END = 6,
    };

    // The sleep thresholds of one deactivation class.
    struct DeactivationInfo {
        float m_linearVelocityThresholdInv;
        float m_angularVelocityThresholdInv;
        float m_slowObjectVelocityMultiplier;
        float m_relativeSleepVelocityThreshold;
        float m_maxDistSqrd[2];
        hkHalf m_maxRotSqrd[2];
    };

    // Sets tau, damping and the factors derived from them.
    void setTauAndDamping(float tau, float damping);
    // Member-wise copy, which hkpContinuousSimulation::simulateToi calls on the world's solver info.
    hkpSolverInfo& operator=(const hkpSolverInfo& other);

    float m_one;
    float m_tau;
    float m_damping;
    float m_frictionTau;
    hkVector4 m_globalAccelerationPerSubStep;
    hkVector4 m_globalAccelerationPerStep;
    hkVector4 m_integrateVelocityFactor;
    hkVector4 m_invIntegrateVelocityFactor;
    float m_dampDivTau;
    float m_tauDivDamp;
    float m_dampDivFrictionTau;
    float m_frictionTauDivDamp;
    float m_contactRestingVelocity;
    DeactivationInfo m_deactivationInfo[6];
    float m_deltaTime;
    float m_invDeltaTime;
    int32_t m_numSteps;
    int32_t m_numMicroSteps;
    float m_invNumMicroSteps;
    float m_invNumSteps;
    hkBool m_forceCoherentConstraintOrderingInSolver;
    uint8_t m_deactivationNumInactiveFramesSelectFlag[2];
    uint8_t m_deactivationIntegrateCounter;
    float m_maxConstraintViolationSqrd;
};

// What the solver needs of one motorized degree of freedom: the impulse bounds per substep, the target velocity and
// the motor's stiffness and damping (SDK hkpSolverInfo.h:103).
struct hkp1dMotorSolverInfo {
    float m_maxImpulsePerSubstep;
    float m_minImpulsePerSubstep;
    float m_velocity;
    float m_tau;
    float m_damping;
};

// The three angular motors of a ragdoll joint: two bits of state per motor in m_broken, and each motor's solver
// info (SDK hkpSolverInfo.h:116; hkaragdoll.pdb sizeof 64).
struct hkp3dAngularMotorSolverInfo {
    using Status = uint8_t;

    // The two bits of m_broken that hold each motor's state.
    enum Mask : int32_t {
        ANGULAR_1ST = 0x03,
        ANGULAR_2ND = 0x0C,
        ANGULAR_3RD = 0x30,
    };

    enum MotorState : int32_t {
        MOTOR_NOT_BROKEN = 0x00,
        MOTOR_BROKEN_MAX_BREACHED = 0x01,
        MOTOR_BROKEN_MIN_BREACHED = 0x03,
        MOTOR_DISABLED = 0x02,
    };

    Status m_broken;
    hkp1dMotorSolverInfo m_motorInfos[3];
};
