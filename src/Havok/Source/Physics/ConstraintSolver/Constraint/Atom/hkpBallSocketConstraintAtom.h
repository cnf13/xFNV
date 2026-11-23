#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"

// Constraint atom that holds the two pivots together.
struct hkpBallSocketConstraintAtom : public hkpConstraintAtom {
    // Havok SDK signature (hkpDynamics.lib, tied by target).
    static const hkClass& staticClass();

    uint8_t m_bodiesToNotify;
    // The setters store it through hkUFloat8::operator=.
    hkUFloat8 m_stabilizationFactor;
    float m_maxImpulse;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpBallSocketConstraintAtomTypeInfo;
