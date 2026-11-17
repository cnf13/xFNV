#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Reconstruction.hpp"

// Constraint atom that keeps the two pivots m_length apart. The PDB places its functions in
// hkpConstraintAtomClass.cpp.
struct hkpStiffSpringConstraintAtom : public hkpConstraintAtom {
	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	uint8_t padding02[2];
	float m_length;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpStiffSpringConstraintAtomTypeInfo;
