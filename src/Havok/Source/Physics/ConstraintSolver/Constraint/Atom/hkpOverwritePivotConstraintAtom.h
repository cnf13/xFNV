#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Reconstruction.hpp"

// Constraint atom that tells the solver to copy pivot A over pivot B (the SDK's; the exe's hkClass
// hkpOverwritePivotConstraintAtomClass has the one member at +0x2 and an object size of 4).
struct hkpOverwritePivotConstraintAtom : public hkpConstraintAtom {
	// The reflection class hkpOverwritePivotConstraintAtomClass.
	static const hkClass& staticClass();

	uint8_t m_copyToPivotBFromPivotA;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpOverwritePivotConstraintAtomTypeInfo;
