#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Reconstruction.hpp"

// Constraint atom that holds the constraint's rotation in each body's local space. The PDB places its functions in
// hkpConstraintAtomClass.cpp.
class hkpSetLocalRotationsConstraintAtom : public hkpConstraintAtom {
public:
	// The reflection class hkpSetLocalRotationsConstraintAtomClass.
	static const hkClass& staticClass();

	hkRotation m_rotationA;
	hkRotation m_rotationB;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpSetLocalRotationsConstraintAtomTypeInfo;
