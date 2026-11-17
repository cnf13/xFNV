#pragma once

#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Reconstruction.hpp"

class hkClass;
class hkTransform;
class hkTypeInfo;

// The motion of a body over one step: its centre of mass and rotation at the start and the end, and the centre of
// mass in local space.
class hkSweptTransform {
public:
	// Puts position into both centres of mass with its w lane zeroed, rotation into both
	// rotations, and clears the local centre of mass.
	void initSweptTransform(const hkVector4& position, const hkQuaternion& rotation);
	void approxTransformAt(float time, hkTransform& transformOut);

	// Havok SDK signature (hkBase.lib, tied by target).
	static const hkClass& staticClass();

	hkVector4 m_centerOfMass0;
	hkVector4 m_centerOfMass1;
	hkQuaternion m_rotation0;
	hkQuaternion m_rotation1;
	hkVector4 m_centerOfMassLocal;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkSweptTransformTypeInfo;
