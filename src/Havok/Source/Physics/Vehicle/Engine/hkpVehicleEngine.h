#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Vehicle/DriverInput/hkpVehicleDriverInput.h"
#include "Havok/Source/Physics/Vehicle/Transmission/hkpVehicleTransmission.h"
#include "Reconstruction.hpp"

class hkpVehicleInstance;

// The part of a vehicle that produces the engine's torque and RPM from the accelerator and the transmission. The exe
// has no vtable of its own for it (abstract; its constructor is inlined into the derived classes').
class hkpVehicleEngine : public hkReferencedObject {
public:
	// What calcEngineInfo produces for the step.
	struct EngineOutput {
		float m_torque;
		float m_rpm;
	};

	// The constructor the derived classes' own use, and the SDK's finish constructor of a loaded object (the derived
	// classes' finish constructors call it): it keeps the fields the load wrote.
	hkpVehicleEngine() = default;
	hkpVehicleEngine(hkFinishLoadedObjectFlag flag) : hkReferencedObject(flag) {}

	virtual void calcEngineInfo(float deltaTime, const hkpVehicleInstance* vehicle,
		const hkpVehicleDriverInput::FilteredDriverInputOutput& FilteredDriverInputOutput,
		const hkpVehicleTransmission::TransmissionOutput& TransmissionOutput, EngineOutput& engineOutput) = 0;
};

// The class's reflection record.
extern hkClass hkpVehicleEngineClass;
