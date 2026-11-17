#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Vehicle/Steering/hkpVehicleSteering.h"
#include "Reconstruction.hpp"

// Steering with a maximum angle that shrinks above a speed, applied to the wheels marked as steering.
class hkpVehicleDefaultSteering : public hkpVehicleSteering {
public:
	hkpVehicleDefaultSteering();
	hkpVehicleDefaultSteering(hkFinishLoadedObjectFlag aFlag) : hkpVehicleSteering(aFlag), m_doesWheelSteer(aFlag) {}
	~hkpVehicleDefaultSteering() override;

	void calcSteering(float deltaTime, const hkpVehicleInstance* vehicle,
		const hkpVehicleDriverInput::FilteredDriverInputOutput& filteredInfoOutput,
		SteeringAnglesOutput& steeringOutput) override;
	virtual void calcMainSteeringAngle(float deltaTime, const hkpVehicleInstance* vehicle,
		const hkpVehicleDriverInput::FilteredDriverInputOutput& filteredInfoOutput,
		SteeringAnglesOutput& steeringOutput);

	// Havok SDK signature (hkpVehicle.lib, tied by target).
	static const hkClass& staticClass();

	float m_maxSteeringAngle;
	float m_maxSpeedFullSteeringAngle;
	hkArray<hkBool> m_doesWheelSteer;
};
inline constexpr fnvd::Vtable hkpVehicleDefaultSteering_vftable{};

extern hkClass hkpVehicleDefaultSteeringClass;

extern hkTypeInfo hkpVehicleDefaultSteeringTypeInfo;
