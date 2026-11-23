#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"

// Constraint atom that removes two angular degrees of freedom, leaving rotation about one free axis.
class hkp2dAngConstraintAtom : public hkpConstraintAtom {
public:
    // Havok SDK signature (hkpDynamics.lib, tied by target).
    static const hkClass& staticClass();

    uint8_t m_freeRotationAxis;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkp2dAngConstraintAtomTypeInfo;
