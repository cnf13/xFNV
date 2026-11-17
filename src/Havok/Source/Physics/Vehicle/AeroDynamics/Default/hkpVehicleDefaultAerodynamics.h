#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Vehicle/AeroDynamics/hkpVehicleAerodynamics.h"

// Drag and lift from the air density, the frontal area and the drag and lift coefficients, plus extra gravity.
class hkpVehicleDefaultAerodynamics : public hkpVehicleAerodynamics {
public:
	hkpVehicleDefaultAerodynamics();
	hkpVehicleDefaultAerodynamics(hkFinishLoadedObjectFlag aFlag) : hkpVehicleAerodynamics(aFlag) {}
	~hkpVehicleDefaultAerodynamics() override;

	// It never reads deltaTime.
	void calcAerodynamics(const float deltaTime, const hkpVehicleInstance* vehicle,
		hkpVehicleAerodynamics::AerodynamicsDragOutput& dragInfoOut) override;
	virtual float calcAerodynamicDrag(const float forwardVelocity) const;
	virtual float calcAerodynamicLift(const float forwardVelocity) const;

	// Havok SDK signature (hkpVehicle.lib, tied by target).
	static const hkClass& staticClass();

	float m_airDensity;
	float m_frontalArea;
	float m_dragCoefficient;
	float m_liftCoefficient;
	hkVector4 m_extraGravityws;
};

extern hkClass hkpVehicleDefaultAerodynamicsClass;

extern hkTypeInfo hkpVehicleDefaultAerodynamicsTypeInfo;
