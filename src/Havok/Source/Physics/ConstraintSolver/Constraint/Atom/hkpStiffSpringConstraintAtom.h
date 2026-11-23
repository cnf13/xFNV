#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"

// Constraint atom that keeps the two pivots m_length apart.
struct hkpStiffSpringConstraintAtom : public hkpConstraintAtom {
    // Havok SDK signature (hkpDynamics.lib, tied by target).
    static const hkClass& staticClass();

    float m_length;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpStiffSpringConstraintAtomTypeInfo;
