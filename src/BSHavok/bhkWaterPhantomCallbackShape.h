#pragma once

#include "Havok/Source/Physics/Collide/Shape/Misc/PhantomCallback/hkpPhantomCallbackShape.h"

class hkpCdBody;
class hkpCollidable;
class hkpCollisionInput;
class hkpRayHitCollector;
struct hkpShapeRayCastInput;

// The phantom shape of a placeable water volume: entering and leaving tell the collidable's water object, and it adds a
// ray cast against the water's own shape. It adds no data to hkpPhantomCallbackShape.
class bhkWaterPhantomCallbackShape : public hkpPhantomCallbackShape {
public:
    bhkWaterPhantomCallbackShape();

    void phantomEnterEvent(
        const hkpCollidable* collidableA,
        const hkpCollidable* collidableB,
        const hkpCollisionInput& env
    ) override;
    void phantomLeaveEvent(const hkpCollidable* collidableA, const hkpCollidable* collidableB) override;
    // A new virtual function introduced by this class.
    virtual void castRayWithCollector(
        const hkpShapeRayCastInput& input,
        const hkpCdBody& cdBody,
        hkpRayHitCollector& collector
    ) const;
};
