#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

// What a character proxy takes from a surface it touches: the plane, its velocity and the friction limits. m_priority
// is the last member; the 12 bytes after it are the hkVector4 alignment.
class hkpSurfaceConstraintInfo {
public:
	// Copy assignment: a member-wise copy of arOther into apThis, returning apThis. Declared as a static so that `a =
	// b` sites keep the implicit copy assignment.
	static hkpSurfaceConstraintInfo* Assign(hkpSurfaceConstraintInfo* apThis, const hkpSurfaceConstraintInfo& arOther);

	hkVector4 m_plane;
	hkVector4 m_velocity;
	float m_staticFriction;
	float m_extraUpStaticFriction;
	float m_extraDownStaticFriction;
	float m_dynamicFriction;
	int32_t m_priority;
};
