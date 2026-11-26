#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

class hkpCdPoint;

// The visitor collision detection reports the contact points it found to; its owner lowers m_earlyOutDistance to
// stop the search once it has enough.
class hkpCdPointCollector {
public:
    hkpCdPointCollector();
    virtual ~hkpCdPointCollector();

    virtual void addCdPoint(const hkpCdPoint& aPoint) = 0;
    // The distance every contact point is compared against before it is reported: no limit until the owner lowers it.
    virtual void reset() {
        m_earlyOutDistance = HK_REAL_MAX;
    }

    // Frees the block to the calling thread's router heap allocator (blockFree) with the object's size, 8; nothing for
    // null. The deleting destructors of this class and of hkpCdBodyPairCollector call it.
    static void operator delete(void* p);

    float m_earlyOutDistance;
};

// No early out until the owner lowers the distance (reset()'s HK_REAL_MAX).
inline hkpCdPointCollector::hkpCdPointCollector() : m_earlyOutDistance(HK_REAL_MAX) {}

inline hkpCdPointCollector::~hkpCdPointCollector() {}
