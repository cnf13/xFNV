#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Reconstruction.hpp"

class hkpVehicleInstance;

// The aerodynamic forces on a vehicle.
class hkpVehicleAerodynamics : public hkReferencedObject {
public:
	// The aerodynamic force and torque calcAerodynamics outputs: two 16-byte vectors (PDB members and offsets; the
	// exe's calcAerodynamics writes them at 0x0 and 0x10, 0xE44370).
	struct AerodynamicsDragOutput {
		hkVector4 m_aerodynamicsForce;
		hkVector4 m_aerodynamicsTorque;
	};

	// The constructor the derived classes' own use, and the SDK's finish constructor of a loaded object (the derived
	// classes' finish constructors call it): it keeps the fields the load wrote.
	hkpVehicleAerodynamics() = default;
	hkpVehicleAerodynamics(hkFinishLoadedObjectFlag flag) : hkReferencedObject(flag) {}
	~hkpVehicleAerodynamics() override;

	// The step's time comes as a float by value (hkpVehicleInstance::updateAerodynamics, 0xE37080, pushes it).
	virtual void calcAerodynamics(const float deltaTime, const hkpVehicleInstance* vehicle,
		hkpVehicleAerodynamics::AerodynamicsDragOutput& dragInfoOut) = 0;
};

// The class's reflection record.
extern hkClass hkpVehicleAerodynamicsClass;
