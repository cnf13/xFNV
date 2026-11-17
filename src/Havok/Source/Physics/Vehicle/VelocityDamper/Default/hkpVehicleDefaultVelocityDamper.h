#pragma once

#include "Havok/Source/Physics/Vehicle/VelocityDamper/hkpVehicleVelocityDamper.h"
#include "Reconstruction.hpp"

// Damps the chassis's spin with one factor normally and another above a spin threshold (collisions).
class hkpVehicleDefaultVelocityDamper : public hkpVehicleVelocityDamper {
public:
	hkpVehicleDefaultVelocityDamper();
	hkpVehicleDefaultVelocityDamper(hkFinishLoadedObjectFlag aFlag) : hkpVehicleVelocityDamper(aFlag) {}
	// The exe's slot 0 is hkBaseObject's shared deleting destructor body (0xDE10B0), not hkReferencedObject's.
	~hkpVehicleDefaultVelocityDamper() override;

	void applyVelocityDamping(float deltaTime, hkpVehicleInstance& vehicle) override;

	// Havok SDK signature (hkpVehicle.lib, tied by target).
	static const hkClass& staticClass();

	float m_normalSpinDamping;
	float m_collisionSpinDamping;
	float m_collisionThreshold;
};
inline constexpr fnvd::Vtable hkpVehicleDefaultVelocityDamper_vftable{};

extern hkClass hkpVehicleDefaultVelocityDamperClass;

extern hkTypeInfo hkpVehicleDefaultVelocityDamperTypeInfo;
