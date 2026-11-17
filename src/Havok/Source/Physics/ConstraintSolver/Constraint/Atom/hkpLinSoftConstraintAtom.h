#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Reconstruction.hpp"

// Constraint atom that holds one linear axis through a spring of the given stiffness and damping.
// The PDB places its functions in hkpConstraintAtomClass.cpp.
class hkpLinSoftConstraintAtom : public hkpConstraintAtom {
public:
	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	uint8_t m_axisIndex;
	float m_tau;
	float m_damping;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpLinSoftConstraintAtomTypeInfo;
