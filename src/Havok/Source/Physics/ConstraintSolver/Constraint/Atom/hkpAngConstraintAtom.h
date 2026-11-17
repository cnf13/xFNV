#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"

// Constraint atom that locks a range of angular axes.
class hkpAngConstraintAtom : public hkpConstraintAtom {
public:
	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	uint8_t m_firstConstrainedAxis;
	uint8_t m_numConstrainedAxes;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpAngConstraintAtomTypeInfo;
