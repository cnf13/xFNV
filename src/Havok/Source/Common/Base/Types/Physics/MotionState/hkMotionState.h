#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

class hkClass;
class hkTypeInfo;

// The state of a body's motion: its transform, swept transform and the limits and damping the simulation applies.
class hkMotionState {
public:
	void Copy(hkMotionState* apSource);
	// Sets the swept transform and the transform from position and rotation, clears the delta
	// angle and sets the object radius to one.
	void initMotionState(const hkVector4& position, const hkQuaternion& rotation);

	// Havok SDK signature (hkBase.lib, tied by target).
	static const hkClass& staticClass();

	hkTransform m_transform;
	hkSweptTransform m_sweptTransform;
	hkVector4 m_deltaAngle;
	float m_objectRadius;
	float m_linearDamping;
	float m_angularDamping;
	hkUFloat8 m_maxLinearVelocity;
	hkUFloat8 m_maxAngularVelocity;
	uint8_t m_deactivationClass;
	uint8_t paddingAF;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkMotionStateTypeInfo;
