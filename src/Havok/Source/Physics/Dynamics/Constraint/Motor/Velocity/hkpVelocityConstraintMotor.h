#pragma once

#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpLimitedForceConstraintMotor.h"
#include "Reconstruction.hpp"

void finishLoadedObjecthkpVelocityConstraintMotor(void* p, int32_t finishing);
// Havok SDK signature (hkpDynamics.lib, tied by reloc,target).
const void* getVtablehkpVelocityConstraintMotor();

// A motor that drives the constraint at a target velocity.
class hkpVelocityConstraintMotor : public hkpLimitedForceConstraintMotor {
public:
	hkpVelocityConstraintMotor() = default;
	// The SDK's finish constructor of a loaded object: it keeps the fields the load wrote.
	hkpVelocityConstraintMotor(hkFinishLoadedObjectFlag f) : hkpLimitedForceConstraintMotor(f) {}

	hkpConstraintMotor* clone() const override;

	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	float m_tau;
	float m_velocityTarget;
	hkBool m_useVelocityTargetFromConstraintTargets;
	uint8_t padding1D[3];
};

inline constexpr fnvd::Vtable hkpVelocityConstraintMotor_vftable{};
extern hkClass hkpVelocityConstraintMotorClass;
extern hkTypeInfo hkpVelocityConstraintMotorTypeInfo;
