#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

class hkpVehicleInstance;

// The part of a vehicle that turns the engine's torque into the wheels' torque through the gears.
class hkpVehicleTransmission : public hkReferencedObject {
public:
	// What calcTransmission produces for the step.
	struct TransmissionOutput {
		float m_transmissionRPM;
		float m_mainTransmittedTorque;
		float* m_wheelsTransmittedTorque;
		int8_t m_numWheelsTramsmittedTorque;
		hkBool m_isReversing;
		int8_t m_currentGear;
		hkBool m_delayed;
		float m_clutchDelayCountdown;
	};

	// The constructor the derived classes' own use, and the SDK's finish constructor of a loaded object (the derived
	// classes' finish constructors call it): it keeps the fields the load wrote.
	hkpVehicleTransmission() = default;
	hkpVehicleTransmission(hkFinishLoadedObjectFlag flag) : hkReferencedObject(flag) {}

	virtual void calcTransmission(float deltaTime, const hkpVehicleInstance* vehicle,
		TransmissionOutput& transmissionOut) = 0;
};

// The class's reflection record.
extern hkClass hkpVehicleTransmissionClass;
