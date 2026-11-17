#pragma once

#include "Havok/Source/Physics/Vehicle/VelocityDamper/hkpVehicleVelocityDamper.h"

// Damps the chassis's spin with one factor normally and another above a spin threshold (collisions).
class hkpVehicleDefaultVelocityDamper : public hkpVehicleVelocityDamper {
public:
	hkpVehicleDefaultVelocityDamper();
	hkpVehicleDefaultVelocityDamper(hkFinishLoadedObjectFlag aFlag) : hkpVehicleVelocityDamper(aFlag) {}
	~hkpVehicleDefaultVelocityDamper() override;

	void applyVelocityDamping(float deltaTime, hkpVehicleInstance& vehicle) override;

	// Havok SDK signature (hkpVehicle.lib, tied by target).
	static const hkClass& staticClass();

	float m_normalSpinDamping;
	float m_collisionSpinDamping;
	float m_collisionThreshold;
};

extern hkClass hkpVehicleDefaultVelocityDamperClass;

extern hkTypeInfo hkpVehicleDefaultVelocityDamperTypeInfo;
