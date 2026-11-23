#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"

// Constraint atom that holds the constraint's pivot in each body's local space.
struct hkpSetLocalTranslationsConstraintAtom : public hkpConstraintAtom {
    // Havok SDK signature (hkpDynamics.lib, tied by target).
    static const hkClass& staticClass();

    hkVector4 m_translationA;
    hkVector4 m_translationB;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpSetLocalTranslationsConstraintAtomTypeInfo;
