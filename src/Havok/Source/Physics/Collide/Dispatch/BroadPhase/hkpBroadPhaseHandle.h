#pragma once

#include <cstdint>

class hkClass;
class hkTypeInfo;

// The broad phase's handle on a collidable: only its id lives here, the rest is in hkpTypedBroadPhaseHandle.
class hkpBroadPhaseHandle {
public:
	// Havok SDK signature (hkpCollide.lib, tied by target).
	static const hkClass& staticClass();

	uint32_t m_id;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpBroadPhaseHandleTypeInfo;
