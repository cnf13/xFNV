#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"

// Constraint atom that locks one linear axis.
class hkpLinConstraintAtom : public hkpConstraintAtom {
public:
    // Havok SDK signature (hkpDynamics.lib, tied by target).
    static const hkClass& staticClass();

    uint8_t m_axisIndex;
    uint8_t padding03;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpLinConstraintAtomTypeInfo;
