#pragma once

#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCdPoint.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/Continuous/hkpContinuousGsk.h"

class hkpConstraintOwner;

// What the agents of one collision step write: the contact points (up to 256), the owner their constraints go to and
// the time-of-impact record.
class hkpProcessCollisionData {
public:
    // The time-of-impact record: the contact point, when it happens, how fast the bodies separate there, and the
    // GJK cache and contact properties it was computed with.
    struct alignas(16) ToiInfo {
        ToiInfo() {
            m_time.m_storage = HK_REAL_MAX;
        }

        // Member-wise copy: copies each member and skips the padding, so it is not the defaulted copy (which copies the
        // padding too).
        ToiInfo& operator=(const ToiInfo& __that);

        // Swaps the bodies' sides: the normal negated and the cache's A and B swapped.
        void flip() {
            m_contactPoint.m_separatingNormal.setNeg3(m_contactPoint.m_separatingNormal);
            m_gskCache.flip();
        }

        hkContactPoint m_contactPoint;
        hkPadSpu<float> m_time;
        hkPadSpu<float> m_seperatingVelocity;
        alignas(16) hkGskCache16 m_gskCache;
        hkContactPointPropertiesWithExtendedUserData16 m_properties;
    };

    hkpProcessCollisionData() = default;
    hkpProcessCollisionData(hkpConstraintOwner* owner) {
        m_constraintOwner = owner;
    }

    hkPadSpu<hkpProcessCdPoint*> m_firstFreeContactPoint;
    hkPadSpu<hkpConstraintOwner*> m_constraintOwner;
    hkpProcessCdPoint m_contactPoints[256];
    ToiInfo m_toi;
};
