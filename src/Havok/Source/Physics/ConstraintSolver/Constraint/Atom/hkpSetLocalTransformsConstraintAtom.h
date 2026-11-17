#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"

// Constraint atom that holds the constraint's frame in each body's local space.
class hkpSetLocalTransformsConstraintAtom : public hkpConstraintAtom {
public:
    // Copy assignment: copies the atom's type and the two transforms.
    hkpSetLocalTransformsConstraintAtom& operator=(const hkpSetLocalTransformsConstraintAtom& other);

    // Havok SDK signature (hkpDynamics.lib, tied by target).
    static const hkClass& staticClass();

    hkTransform m_transformA;
    hkTransform m_transformB;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpSetLocalTransformsConstraintAtomTypeInfo;
