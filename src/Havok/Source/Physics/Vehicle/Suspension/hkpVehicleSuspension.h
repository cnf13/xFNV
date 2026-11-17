#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/Vehicle/WheelCollide/hkpVehicleWheelCollide.h"

class hkpVehicleInstance;

// The part of a vehicle that computes each wheel's suspension force from the wheel collision results; holds each
// wheel's hard point, direction and length. It is abstract.
class hkpVehicleSuspension : public hkReferencedObject {
public:
	// One wheel's suspension geometry, in chassis space.
	struct SuspensionWheelParameters {
		// Havok SDK signature (hkpVehicle.lib, tied by target).
		static const hkClass& staticClass();

		hkVector4 m_hardpointChassisSpace;
		hkVector4 m_directionChassisSpace;
		float m_length;
	};

	// The constructor the derived classes' own use, and the SDK's finish constructor of a loaded object (the derived
	// classes' finish constructors call it): it keeps the fields the load wrote.
	hkpVehicleSuspension() = default;
	hkpVehicleSuspension(hkFinishLoadedObjectFlag f) : hkReferencedObject(f), m_wheelParams(f) {}

	virtual void calcSuspension(float deltaTime, const hkpVehicleInstance* vehicle,
		const hkpVehicleWheelCollide::CollisionDetectionWheelOutput* cdInfo, float* suspensionForceOut) = 0;

	hkArray<SuspensionWheelParameters> m_wheelParams;
};

// The class's reflection record.
extern hkClass hkpVehicleSuspensionClass;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpVehicleSuspensionSuspensionWheelParametersTypeInfo;
extern hkClass hkpVehicleSuspensionSuspensionWheelParametersClass;
