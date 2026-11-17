#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"

// Constraint atom of a pulley: a rope through a fixed world pivot per body, of length m_ropeLength, with body B's side
// weighted by m_leverageOnBodyB.
struct hkpPulleyConstraintAtom : public hkpConstraintAtom {
	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	hkVector4 m_fixedPivotAinWorld;
	hkVector4 m_fixedPivotBinWorld;
	float m_ropeLength;
	float m_leverageOnBodyB;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpPulleyConstraintAtomTypeInfo;
