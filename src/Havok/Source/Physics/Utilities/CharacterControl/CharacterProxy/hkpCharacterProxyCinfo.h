#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

class hkClass;
class hkpShapePhantom;
class hkTypeInfo;

// The construction info of an hkpCharacterProxy: where it is and how it moves, rests and recovers. A plain struct.
class alignas(16) hkpCharacterProxyCinfo {
public:
	// The creation info's constructor, run on the memory the caller allocated.
	hkpCharacterProxyCinfo();

	// The class allocator's free: a block of this class's size on the Havok heap. bhkCharacterControllerCinfo inherits
	// it, though it is larger.
	static void operator delete(void* apMemory);
	// Havok SDK signature (hkpUtilities.lib, tied by target).
	static const hkClass& staticClass();

	hkVector4 m_position;
	hkVector4 m_velocity;
	float m_dynamicFriction;
	float m_staticFriction;
	float m_keepContactTolerance;
	hkVector4 m_up;
	float m_extraUpStaticFriction;
	float m_extraDownStaticFriction;
	hkpShapePhantom* m_shapePhantom;
	float m_keepDistance;
	float m_contactAngleSensitivity;
	uint32_t m_userPlanes;
	float m_maxCharacterSpeedForSolver;
	float m_characterStrength;
	float m_characterMass;
	float m_maxSlope;
	float m_penetrationRecoverySpeed;
	int32_t m_maxCastIterations;
	bool m_refreshManifoldInCheckSupport;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpCharacterProxyCinfoTypeInfo;
