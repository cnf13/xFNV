#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Reconstruction.hpp"

// Constraint atom that limits the distance along one linear axis to a range.
// The PDB places its functions in hkpConstraintAtomClass.cpp.
struct hkpLinLimitConstraintAtom : public hkpConstraintAtom {
	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	uint8_t m_axisIndex;
	float m_min;
	float m_max;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpLinLimitConstraintAtomTypeInfo;
