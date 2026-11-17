#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

class hkpVehicleInstance;

// What the device (keyboard, pad, ...) reports to a vehicle's driver input. Each kind of device derives from it.
class hkpVehicleDriverInputStatus : public hkReferencedObject {
public:
	// The constructor the derived classes' own use, and the SDK's finish constructor of a loaded object (the derived
	// classes' finish constructors call it): it keeps the fields the load wrote.
	hkpVehicleDriverInputStatus() = default;
	hkpVehicleDriverInputStatus(hkFinishLoadedObjectFlag f) : hkReferencedObject(f) {}
	~hkpVehicleDriverInputStatus() override;

	virtual hkpVehicleDriverInputStatus* clone() const = 0;
};

// The part of a vehicle that turns the device's status into the pedal and wheel inputs the other components use.
class hkpVehicleDriverInput : public hkReferencedObject {
public:
	// The filtered inputs calcDriverInput produces.
	struct FilteredDriverInputOutput {
		float m_acceleratorPedalInput;
		float m_brakePedalInput;
		float m_steeringWheelInput;
		hkBool m_handbrakeOn;
		hkBool m_tryingToReverse;
	};

	// The constructor the derived classes' own use, and the SDK's finish constructor of a loaded object (the derived
	// classes' finish constructors call it): it keeps the fields the load wrote.
	hkpVehicleDriverInput() = default;
	hkpVehicleDriverInput(hkFinishLoadedObjectFlag flag) : hkReferencedObject(flag) {}

	virtual void calcDriverInput(float deltaTime, const hkpVehicleInstance* vehicle,
		const hkpVehicleDriverInputStatus* deviceStatus, FilteredDriverInputOutput& filteredInputOut) = 0;
};

// The class's reflection record.
extern hkClass hkpVehicleDriverInputStatusClass;

// The class's reflection record.
extern hkClass hkpVehicleDriverInputClass;
