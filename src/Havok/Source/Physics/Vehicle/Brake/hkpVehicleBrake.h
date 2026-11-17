#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Vehicle/DriverInput/hkpVehicleDriverInput.h"
#include "Reconstruction.hpp"

class hkpVehicleInstance;

// The part of a vehicle that turns the driver's brake and handbrake input into braking torque per wheel. The exe has
// no vtable of its own for it (abstract; its constructor is inlined into the derived classes').
class hkpVehicleBrake : public hkReferencedObject {
public:
	// What calcBreakingInfo produces for the step.
	struct WheelBreakingOutput {
		hkArray<float> m_brakingTorque;
		hkArray<hkBool> m_isFixed;
		float m_wheelsTimeSinceMaxPedalInput;
	};

	hkpVehicleBrake() = default;
	// The SDK's finish constructor: the loaded brake keeps its fields.
	hkpVehicleBrake(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag) {}

	virtual void calcBreakingInfo(float deltaTime, const hkpVehicleInstance* vehicle,
		const hkpVehicleDriverInput::FilteredDriverInputOutput& FilteredDriverInputOutput,
		WheelBreakingOutput& breakingInfo) = 0;
};

// The class's reflection record.
extern hkClass hkpVehicleBrakeClass;
