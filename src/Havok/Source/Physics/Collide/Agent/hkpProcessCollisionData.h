#pragma once

#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCdPoint.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/Continuous/hkpContinuousGsk.h"
#include "Reconstruction.hpp"

class hkpConstraintOwner;

// What the agents of one collision step write: the contact points (up to 256), the owner their constraints go to and
// the time-of-impact record.
class hkpProcessCollisionData {
public:
	// The time-of-impact record: the contact point, when it happens, how fast the bodies separate there, and the
	// GJK cache and contact properties it was computed with.
	struct alignas(16) ToiInfo {
		// The PDB's operator=, the member-wise copy: the exe's out-of-line copy 0xD33330 (thiscall, ret 4;
		// rollbackOutput 0xD33600 calls it), which copies each member and skips the padding at +0x28, so it is not
		// the defaulted copy (which copies the padding too).
		ToiInfo& operator=(const ToiInfo& __that);

		hkContactPoint m_contactPoint;
		hkPadSpu<float> m_time;
		hkPadSpu<float> m_seperatingVelocity;
		alignas(16) hkGskCache16 m_gskCache;
		hkContactPointPropertiesWithExtendedUserData16 m_properties;
	};

	hkPadSpu<hkpProcessCdPoint*> m_firstFreeContactPoint;
	hkPadSpu<hkpConstraintOwner*> m_constraintOwner;
	hkpProcessCdPoint m_contactPoints[256];
	ToiInfo m_toi;
};
