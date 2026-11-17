#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Vehicle/Brake/hkpVehicleBrake.h"

// A brake with a maximum torque per wheel, a pedal input above which the wheel blocks after a delay, and the wheels
// the handbrake acts on.
class hkpVehicleDefaultBrake : public hkpVehicleBrake {
public:
	// One wheel's braking setup.
	struct WheelBrakingProperties {
		// Havok SDK signature (hkpVehicle.lib, tied by target).
		static const hkClass& staticClass();

		float m_maxBreakingTorque;
		float m_minPedalInputToBlock;
		hkBool m_isConnectedToHandbrake;
	};

	hkpVehicleDefaultBrake();

	// The SDK's finish constructor: the loaded brake keeps its fields.
	hkpVehicleDefaultBrake(hkFinishLoadedObjectFlag aFlag) : hkpVehicleBrake(aFlag), m_wheelBrakingProperties(aFlag) {}

	~hkpVehicleDefaultBrake() override;

	void calcBreakingInfo(float deltaTime, const hkpVehicleInstance* vehicle,
		const hkpVehicleDriverInput::FilteredDriverInputOutput& FilteredDriverInputOutput,
		WheelBreakingOutput& breakingInfo) override;

	// Havok SDK signature (hkpVehicle.lib, tied by target).
	static const hkClass& staticClass();

	hkArray<WheelBrakingProperties> m_wheelBrakingProperties;
	float m_wheelsMinTimeToBlock;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpVehicleDefaultBrakeWheelBrakingPropertiesTypeInfo;
extern hkClass hkpVehicleDefaultBrakeWheelBrakingPropertiesClass;
extern hkClass hkpVehicleDefaultBrakeClass;
extern hkTypeInfo hkpVehicleDefaultBrakeTypeInfo;
