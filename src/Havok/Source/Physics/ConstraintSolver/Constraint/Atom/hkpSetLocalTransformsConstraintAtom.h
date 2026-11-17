#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Reconstruction.hpp"

// Constraint atom that holds the constraint's frame in each body's local space. The PDB places its functions in
// hkpConstraintAtomClass.cpp.
class hkpSetLocalTransformsConstraintAtom : public hkpConstraintAtom {
public:
	// The exe's out-of-line copy assignment: copies the atom's type and the two transforms (thiscall, ret 4,
	// returns this, 0xD85300); the compiler emitted it from hkpConstraintDataCloningUtil.cpp.
	hkpSetLocalTransformsConstraintAtom& operator=(const hkpSetLocalTransformsConstraintAtom& other);

	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	hkTransform m_transformA;
	hkTransform m_transformB;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpSetLocalTransformsConstraintAtomTypeInfo;
