#pragma once

#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpLimitedForceConstraintMotor.h"

void cleanupLoadedObjecthkpSpringDamperConstraintMotor(void* p);
void finishLoadedObjecthkpSpringDamperConstraintMotor(void* p, int32_t finishing);
// Havok SDK signature (hkpDynamics.lib, tied by reloc,target).
const void* getVtablehkpSpringDamperConstraintMotor();

// A motor that acts as a damped spring towards the target.
class hkpSpringDamperConstraintMotor : public hkpLimitedForceConstraintMotor {
public:
	hkpSpringDamperConstraintMotor();
	hkpSpringDamperConstraintMotor(float springConstant, float springDamping, float maxForce);
	// The SDK's finish constructor of a loaded object: it keeps the fields the load wrote.
	hkpSpringDamperConstraintMotor(hkFinishLoadedObjectFlag f) : hkpLimitedForceConstraintMotor(f) {}
	~hkpSpringDamperConstraintMotor() override;

	hkpConstraintMotor* clone() const override;

	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	float m_springConstant;
	float m_springDamping;
};

extern hkClass hkpSpringDamperConstraintMotorClass;
extern hkTypeInfo hkpSpringDamperConstraintMotorTypeInfo;
