#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Vehicle/Friction/hkpVehicleFriction.h"
#include "Havok/Source/Physics/Vehicle/Suspension/hkpVehicleSuspension.h"

class hkpRigidBody;

// A vehicle's static description: the chassis axes and inertia factors, the per-wheel component parameters, the wheels
// per axle and the friction solver's description that init derives from them.
class hkpVehicleData : public hkReferencedObject {
public:
	// One wheel's physical parameters and the axle it is on.
	struct WheelComponentParams {
		// Havok SDK signature (hkpVehicle.lib, tied by target).
		static const hkClass& staticClass();

		float m_radius;
		float m_mass;
		float m_width;
		float m_friction;
		float m_viscosityFriction;
		float m_maxFriction;
		float m_slipAngle;
		float m_forceFeedbackMultiplier;
		float m_maxContactBodyAcceleration;
		int8_t m_axle;
	};

	hkpVehicleData();

	// The SDK's finish constructor: the loaded data keeps its fields.
	hkpVehicleData(hkFinishLoadedObjectFlag aFlag)
		: hkReferencedObject(aFlag), m_wheelParams(aFlag), m_numWheelsPerAxle(aFlag) {}

	~hkpVehicleData() override;

	virtual void init(const hkArray<hkpVehicleSuspension::SuspensionWheelParameters>& suspensionWheelParams,
		hkpRigidBody* chassis);

	// Havok SDK signature (hkpVehicle.lib, tied by target).
	static const hkClass& staticClass();

	hkVector4 m_gravity;
	int8_t m_numWheels;
	hkRotation m_chassisOrientation;
	float m_torqueRollFactor;
	float m_torquePitchFactor;
	float m_torqueYawFactor;
	float m_extraTorqueFactor;
	float m_maxVelocityForPositionalFriction;
	float m_chassisUnitInertiaYaw;
	float m_chassisUnitInertiaRoll;
	float m_chassisUnitInertiaPitch;
	float m_frictionEqualizer;
	float m_normalClippingAngleCos;
	float m_maxFrictionSolverMassRatio;
	hkArray<WheelComponentParams> m_wheelParams;
	hkArray<int8_t> m_numWheelsPerAxle;
	hkpVehicleFrictionDescription m_frictionDescription;
	hkVector4 m_chassisFrictionInertiaInvDiag;
	hkBool m_alreadyInitialised;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpVehicleDataWheelComponentParamsTypeInfo;
extern hkClass hkpVehicleDataWheelComponentParamsClass;
extern hkClass hkpVehicleDataClass;
extern hkTypeInfo hkpVehicleDataTypeInfo;
