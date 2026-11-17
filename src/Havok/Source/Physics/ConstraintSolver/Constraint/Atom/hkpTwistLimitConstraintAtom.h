#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Reconstruction.hpp"

// Constraint atom that limits the twist angle about an axis to a range.
// The PDB places its functions in hkpConstraintAtomClass.cpp.
class hkpTwistLimitConstraintAtom : public hkpConstraintAtom {
public:
	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	uint8_t m_isEnabled;
	uint8_t m_twistAxis;
	uint8_t m_refAxis;
	uint8_t padding05[3];
	float m_minAngle;
	float m_maxAngle;
	float m_angularLimitsTauFactor;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpTwistLimitConstraintAtomTypeInfo;
