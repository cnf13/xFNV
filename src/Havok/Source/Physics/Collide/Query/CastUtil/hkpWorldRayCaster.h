#pragma once

#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhase.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhaseCastCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkpCollisionFilter;
class hkpRayCollidableFilter;
class hkpRayHitCollector;
struct hkpWorldRayCastInput;

// Casts a world ray through the cached broad phase and hands every shape hit to a ray hit collector (the collector form
// of hkpWorld::castRay). Only the destructor and addBroadPhaseHandle are declared; castRayGroup,
// castRaysFromSinglePoint and the two constructors are left out.
class hkpWorldRayCaster : public hkpBroadPhaseCastCollector {
public:
    ~hkpWorldRayCaster() override;

    float addBroadPhaseHandle(const hkpBroadPhaseHandle* broadPhaseHandle, int32_t castIndex) override;

    // Casts input through broadphase, narrowing each collidable the broad phase reports
    // against filter's ray collidable filter and handing the hits to collector; cache is the broad phase's AABB cache.
    void castRay(
        const hkpBroadPhase& broadphase,
        const hkpWorldRayCastInput& input,
        const hkpCollisionFilter* filter,
        const hkpBroadPhaseAabbCache* cache,
        hkpRayHitCollector& collector
    );
    // Havok SDK signature (hkpCollide.lib, tied by string).
    void castRayGroup(
        const hkpBroadPhase& broadphase,
        const hkpWorldRayCastInput* inputArray,
        int32_t numRays,
        const hkpCollisionFilter* filter,
        hkpRayHitCollector* collectorBase,
        int32_t collectorStriding
    );
    void castRaysFromSinglePoint(
        const hkpBroadPhase& broadphase,
        const hkpWorldRayCastInput* inputArray,
        int32_t numRays,
        const hkpCollisionFilter* filter,
        const char* cache,
        hkpRayHitCollector* collectorBase,
        int32_t collectorStriding
    );

    const hkpWorldRayCastInput* m_input;
    const hkpRayCollidableFilter* m_filter;
    hkpRayHitCollector* m_collectorBase;
    int32_t m_collectorStriding;
    hkpShapeRayCastInput m_shapeInput;
};
inline constexpr xfnv::Vtable hkpWorldRayCaster_vftable{};
