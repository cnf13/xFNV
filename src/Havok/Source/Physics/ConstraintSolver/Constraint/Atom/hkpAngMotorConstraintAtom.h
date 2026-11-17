#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"

class hkpConstraintMotor;

// Constraint atom that drives one axis of an angular constraint towards a target angle with a motor.
class hkpAngMotorConstraintAtom : public hkpConstraintAtom {
public:
    // Havok SDK signature (hkpDynamics.lib, tied by target).
    static const hkClass& staticClass();

    hkBool m_isEnabled;
    uint8_t m_motorAxis;
    int16_t m_initializedOffset;
    int16_t m_previousTargetAngleOffset;
    int16_t m_correspondingAngLimitSolverResultOffset;
    uint8_t padding0A[2];
    float m_targetAngle;
    hkpConstraintMotor* m_motor;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpAngMotorConstraintAtomTypeInfo;
