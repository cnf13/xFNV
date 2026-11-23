#pragma once

#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpAllCdPointCollector.h"

class hkpWorldObject;

// Collects the contact points a character proxy's sweep reports. Overrides the destructor, addCdPoint and reset.
// 16-byte aligned.
class alignas(16) bhkCharacterPointCollector : public hkpAllCdPointCollector {
public:
    // Whether an object in m_collidingObjects is new, removed or unchanged since the previous sweep.
    enum bhkCollisionObjectState : int32_t {
        COLLISION_OBJECT_NEW = 0,
        COLLISION_OBJECT_REMOVED = 1,
        COLLISION_OBJECT_UNTOUCHED = 2,
    };

    // What a hit in m_hits touched, kept in its m_shapeKeyA: nothing known, a triangle's face (its normal within half a
    // degree of the contact's separating normal), or another feature of a triangle (set by addCdPoint).
    enum ContactFeatureType : int32_t {
        CONTACTFEATURE_INVALID = 0,
        CONTACTFEATURE_FACE = 1,
        CONTACTFEATURE_NONFACE = 2,
        CONTACTFEATURE_MAX_ID = 3,
    };

    bhkCharacterPointCollector(bhkCharacterPointCollector* apStartCollector);
    ~bhkCharacterPointCollector() override;

    void addCdPoint(const hkpCdPoint& event) override;
    // Slot 2.
    void reset() override;

    // Drops the hits behind a face hit.
    void filterOutEdgeHits();
    // Reports the sweep's trap contacts to the world's trap listener and drops the objects no longer touched.
    void handleCollisions();

    // Returns the block to the calling thread's Havok heap.
    static void operator delete(void* apMemory, size_t aSize);

    bhkCharacterPointCollector* pStartCollector;
    // The objects the sweep collided with.
    hkArray<hkpWorldObject*> m_collidingObjects;
    // The collision state of each object in m_collidingObjects.
    hkArray<bhkCollisionObjectState> m_collisionStates;
    // The points collected.
    hkArray<hkpRootCdPoint> m_collisionPoints;
};
