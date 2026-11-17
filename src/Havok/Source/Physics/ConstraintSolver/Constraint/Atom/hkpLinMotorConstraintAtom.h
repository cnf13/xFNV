#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"

class hkpConstraintMotor;

// Constraint atom that drives one axis of a linear constraint towards a target position with a motor.
class hkpLinMotorConstraintAtom : public hkpConstraintAtom {
public:
	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	hkBool m_isEnabled;
	uint8_t m_motorAxis;
	int16_t m_initializedOffset;
	int16_t m_previousTargetPositionOffset;
	float m_targetPosition;
	hkpConstraintMotor* m_motor;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpLinMotorConstraintAtomTypeInfo;
