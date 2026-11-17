#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

class hkpCollisionDispatcher;
class hkpCollisionFilter;
class hkpConvexListFilter;

// What the collision agents are given besides the bodies: the dispatcher, the filters, the tolerance and the AABB
// compression. Not polymorphic.
class hkpCollisionInput {
public:
	// The offsets and scale that pack an AABB into 32 bits.
	struct Aabb32Info {
		Aabb32Info();

		hkVector4 m_bitOffsetLow;
		hkVector4 m_bitOffsetHigh;
		hkVector4 m_bitScale;
	};

	hkpCollisionInput();

	// The SDK's inline setter. The exe's out-of-line copy (thiscall, ret 4, 0x6218D0) stores through the hkPadSpu
	// assignment 0x52CE20.
	void setTolerance(float t) { m_tolerance = t; }

	hkpCollisionDispatcher* m_dispatcher;
	hkPadSpu<int32_t> m_weldClosestPoints;
	float m_tolerance;
	const hkpCollisionFilter* m_filter;
	hkpConvexListFilter* m_convexListFilter;
	// Mutable as in the SDK header: hkpSimulation::processAgentCollideDiscrete 0xCF92C0 sets it through its const
	// input.
	mutable hkPadSpu<int32_t> m_createPredictiveAgents;
	Aabb32Info m_aabb32Info;
};
