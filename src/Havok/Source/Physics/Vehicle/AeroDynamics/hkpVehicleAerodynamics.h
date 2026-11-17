#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"

class hkpVehicleInstance;

// The aerodynamic forces on a vehicle.
class hkpVehicleAerodynamics : public hkReferencedObject {
public:
	// The aerodynamic force and torque that calcAerodynamics outputs.
	struct AerodynamicsDragOutput {
		hkVector4 m_aerodynamicsForce;
		hkVector4 m_aerodynamicsTorque;
	};

	// The constructor the derived classes' own use, and the SDK's finish constructor of a loaded object (the derived
	// classes' finish constructors call it): it keeps the fields the load wrote.
	hkpVehicleAerodynamics() = default;
	hkpVehicleAerodynamics(hkFinishLoadedObjectFlag flag) : hkReferencedObject(flag) {}
	~hkpVehicleAerodynamics() override;

	// The step's time is passed as a float by value from hkpVehicleInstance::updateAerodynamics.
	virtual void calcAerodynamics(const float deltaTime, const hkpVehicleInstance* vehicle,
		hkpVehicleAerodynamics::AerodynamicsDragOutput& dragInfoOut) = 0;
};

// The class's reflection record.
extern hkClass hkpVehicleAerodynamicsClass;
