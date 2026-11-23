#pragma once

#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"

// A point collector that keeps the closest contact point it is given; m_hitPoint's m_rootCollidableA stays null until
// there is one.
class hkpClosestCdPointCollector : public hkpCdPointCollector {
public:
    // The SDK's constructor: the collector reset.
    hkpClosestCdPointCollector() {
        reset();
    }
    ~hkpClosestCdPointCollector() override;

    void reset() override;
    // Protected in the SDK.
    void addCdPoint(const hkpCdPoint& event) override;

    // Whether a point was collected.
    hkBool hasHit() const {
        return hkBool(m_hitPoint.m_rootCollidableA != nullptr);
    }
    const hkpRootCdPoint& getHit() const {
        return m_hitPoint;
    }
    const hkContactPoint& getHitContact() const {
        return m_hitPoint.m_contact;
    }

    // DeletingDestruct's free (the thread's Havok heap, blockFree with the object's size).
    static void operator delete(void* apMemory);

    hkpRootCdPoint m_hitPoint;
};
