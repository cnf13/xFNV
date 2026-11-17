#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Reconstruction.hpp"

// Constraint atom that applies friction along one linear axis.
// The PDB places its functions in hkpConstraintAtomClass.cpp.
struct hkpLinFrictionConstraintAtom : public hkpConstraintAtom {
	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	uint8_t m_isEnabled;
	uint8_t m_frictionAxis;
	float m_maxFrictionForce;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpLinFrictionConstraintAtomTypeInfo;
