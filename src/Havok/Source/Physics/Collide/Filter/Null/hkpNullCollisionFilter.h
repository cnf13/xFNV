#pragma once

#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"

// The filter that lets every pair collide (m_type HK_FILTER_NULL). It adds no data to hkpCollisionFilter.
class hkpNullCollisionFilter : public hkpCollisionFilter {
public:
    hkpNullCollisionFilter() {
        m_type = HK_FILTER_NULL;
    }
    hkpNullCollisionFilter(hkFinishLoadedObjectFlag aFlag);

    // Every overload lets the pair collide.
    hkBool isCollisionEnabled(const hkpCollidable& a, const hkpCollidable& b) const override {
        return true;
    }

    hkBool isCollisionEnabled(
        const hkpCollisionInput& input,
        const hkpCdBody& a,
        const hkpCdBody& b,
        const hkpShapeContainer& bContainer,
        uint32_t bKey
    ) const override {
        return true;
    }

    hkBool isCollisionEnabled(
        const hkpCollisionInput& input,
        const hkpCdBody& collectionBodyA,
        const hkpCdBody& collectionBodyB,
        const hkpShapeContainer& containerShapeA,
        const hkpShapeContainer& containerShapeB,
        uint32_t keyA,
        uint32_t keyB
    ) const override {
        return true;
    }

    hkBool isCollisionEnabled(
        const hkpShapeRayCastInput& aInput,
        const hkpShape& bShape,
        const hkpShapeContainer& bContainer,
        uint32_t bKey
    ) const override {
        return true;
    }

    hkBool isCollisionEnabled(const hkpWorldRayCastInput& a, const hkpCollidable& collidableB) const override {
        return true;
    }

    ~hkpNullCollisionFilter() override;

    // Havok SDK signature (hkpCollide.lib, tied by target).
    static const hkClass& staticClass();
};
