#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"

// Constraint atom that applies angular friction around a range of the constraint's axes.
class hkpAngFrictionConstraintAtom : public hkpConstraintAtom {
public:
    // Havok SDK signature (hkpDynamics.lib, tied by target).
    static const hkClass& staticClass();

    uint8_t m_isEnabled;
    uint8_t m_firstFrictionAxis;
    uint8_t m_numFrictionAxes;
    uint8_t padding05[3];
    float m_maxFrictionTorque;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpAngFrictionConstraintAtomTypeInfo;
