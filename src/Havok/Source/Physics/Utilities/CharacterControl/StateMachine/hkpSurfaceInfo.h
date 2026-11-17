#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// What a character controller found under it: how well the surface supports it, and how the surface looks and moves.
class hkpSurfaceInfo {
public:
	enum SupportedState : int32_t {
		UNSUPPORTED = 0x0,
		SLIDING = 0x1,
		SUPPORTED = 0x2,
	};

	SupportedState m_supportedState;
	hkVector4 m_surfaceNormal;
	hkVector4 m_surfaceVelocity;
	float m_surfaceDistanceExcess;
	hkBool m_surfaceIsDynamic;
};
