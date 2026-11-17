#pragma once

#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpLimitedForceConstraintMotor.h"
#include "Reconstruction.hpp"

void cleanupLoadedObjecthkpPositionConstraintMotor(void* p);
void finishLoadedObjecthkpPositionConstraintMotor(void* p, int32_t finishing);
// Havok SDK signature (hkpDynamics.lib, tied by reloc,target).
const void* getVtablehkpPositionConstraintMotor();

// A motor that drives the constraint towards a target position, with recovery velocities.
class hkpPositionConstraintMotor : public hkpLimitedForceConstraintMotor {
public:
	// The exe's constructor ignores currentPosition and sets the defaults.
	hkpPositionConstraintMotor(float currentPosition = 0.0f);
	// The SDK's finish constructor of a loaded object: it keeps the fields the load wrote.
	hkpPositionConstraintMotor(hkFinishLoadedObjectFlag f) : hkpLimitedForceConstraintMotor(f) {}
	~hkpPositionConstraintMotor() override;

	hkpConstraintMotor* clone() const override;

	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	float m_tau;
	float m_damping;
	float m_proportionalRecoveryVelocity;
	float m_constantRecoveryVelocity;
};

inline constexpr fnvd::Vtable hkpPositionConstraintMotor_vftable{};
extern hkClass hkpPositionConstraintMotorClass;
extern hkTypeInfo hkpPositionConstraintMotorTypeInfo;
