#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"

class hkpCollidable;
class hkpCollisionInput;

// A shape with no geometry that tells its owner when other collidables enter or leave it (a trigger volume); the
// derived shapes implement the two events. It adds no data to hkpShape. The events' parameters are collidableA,
// collidableB and the collision input. phantomLeaveEvent takes no collision input.
class hkpPhantomCallbackShape : public hkpShape {
public:
    hkpPhantomCallbackShape();
    ~hkpPhantomCallbackShape() override;

    void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
    void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
    hkBool castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const override;
    void castRayWithCollectorImpl(
        const hkpShapeRayCastInput& input,
        const hkpCdBody& cdBody,
        hkpRayHitCollector& collector
    ) const override;
    virtual void phantomEnterEvent(
        const hkpCollidable* collidableA,
        const hkpCollidable* collidableB,
        const hkpCollisionInput& env
    ) = 0;
    virtual void phantomLeaveEvent(const hkpCollidable* collidableA, const hkpCollidable* collidableB) = 0;
};

// The class's reflection record.
extern hkClass hkpPhantomCallbackShapeClass;

inline hkpPhantomCallbackShape::hkpPhantomCallbackShape() : hkpShape(HK_SHAPE_PHANTOM_CALLBACK) {}
