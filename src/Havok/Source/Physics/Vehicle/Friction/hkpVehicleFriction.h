#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"

// The step the friction solver runs for: its time and the inverse.
struct hkpVehicleStepInfo {
	float m_deltaTime;
	float m_invDeltaTime;
};

// The friction solver's precomputed description of the vehicle's two axles: the friction circle table and the wheel
// inertias and ratios each axle's solver step uses. hkpVehicleFrictionStatus of the same header is declared in
// hkpVehicleInstance.h.
struct hkpVehicleFrictionDescription {
	// What hkVehicleFrictionDescriptionInitValues builds the description from: the two axles' wheel positions, the
	// chassis's centre of mass, axes and inverse friction inertia and mass, the wheels' axle inertia and radius and
	// the friction equalizer (hkpvehicle.pdb sizeof 176; its vectors and matrix fall on 16-byte offsets, so the SSE
	// layout is the same).
	struct Cinfo {
		hkVector4 m_wheelPosition[2];
		hkVector4 m_chassisCenterOfMass;
		hkVector4 m_directionRight;
		hkVector4 m_directionFront;
		hkVector4 m_directionUp;
		hkMatrix3 m_chassisFrictionInertiaInv;
		float m_chassisMassInv;
		float m_wheelAxleAngularInertia[2];
		float m_wheelRadius[2];
		float m_frictionEqualizer;
	};

	// One axle's friction parameters.
	struct AxisDescription {
		// Havok SDK signature (hkpVehicle.lib, tied by target).
		static const hkClass& staticClass();

		float m_frictionCircleYtab[16];
		float m_xStep;
		float m_xStart;
		float m_wheelSurfaceInertia;
		float m_wheelSurfaceInertiaInv;
		float m_wheelChassisMassRatio;
		float m_wheelRadius;
		float m_wheelRadiusInv;
		float m_wheelDownForceFactor;
		float m_wheelDownForceSumFactor;
	};

	// Havok SDK signature (hkpVehicle.lib, tied by target).
	static const hkClass& staticClass();

	float m_wheelDistance;
	float m_chassisMassInv;
	AxisDescription m_axleDescr[2];
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpVehicleFrictionDescriptionAxisDescriptionTypeInfo;
extern hkTypeInfo hkpVehicleFrictionDescriptionTypeInfo;
extern hkClass hkpVehicleFrictionDescriptionAxisDescriptionClass;
extern hkClass hkpVehicleFrictionDescriptionClass;
