#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"

// Constraint atom that limits the angle about one axis of the first body's constraint base to a range.
struct hkpAngLimitConstraintAtom : public hkpConstraintAtom {
    // Havok SDK signature (hkpDynamics.lib, tied by target).
    static const hkClass& staticClass();

    uint8_t m_isEnabled;
    // The index of the axis in the first body's constraint base that is limited.
    uint8_t m_limitAxis;
    // The minimum angle in radians (may be negative).
    float m_minAngle;
    // The maximum angle in radians (may be negative).
    float m_maxAngle;
    // A stiffness factor [0..1] the solver uses; defaults to 1.0.
    float m_angularLimitsTauFactor;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpAngLimitConstraintAtomTypeInfo;
