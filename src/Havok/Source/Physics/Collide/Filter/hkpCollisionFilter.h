#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollidableCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpShapeCollectionFilter.h"

// The world's collision filter: one object answering every filter interface. Its isCollisionEnabled overloads stay
// pure.
class hkpCollisionFilter : public hkReferencedObject,
                           public hkpCollidableCollidableFilter,
                           public hkpShapeCollectionFilter,
                           public hkpRayShapeCollectionFilter,
                           public hkpRayCollidableFilter {
public:
    enum hkpFilterType {
        HK_FILTER_UNKNOWN = 0,
        HK_FILTER_NULL = 1,
        HK_FILTER_GROUP = 2,
        HK_FILTER_LIST = 3,
        HK_FILTER_CUSTOM = 4,
        HK_FILTER_PAIR = 5,
        HK_FILTER_CONSTRAINT = 6,
    };

    hkpCollisionFilter();
    // The SDK's finish constructor: a loaded object keeps the fields the load wrote.
    hkpCollisionFilter(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag) {}
    ~hkpCollisionFilter() override;

    // The five filter interfaces' isCollisionEnabled overloads, all pure here: collidable pairs, shape keys of one
    // collection, shape keys of two collections, a ray against a shape key, a world ray against a collidable.
    hkBool isCollisionEnabled(const hkpCollidable& a, const hkpCollidable& b) const override = 0;
    hkBool isCollisionEnabled(
        const hkpCollisionInput& input,
        const hkpCdBody& a,
        const hkpCdBody& b,
        const hkpShapeContainer& bContainer,
        uint32_t bKey
    ) const override = 0;
    hkBool isCollisionEnabled(
        const hkpCollisionInput& input,
        const hkpCdBody& collectionBodyA,
        const hkpCdBody& collectionBodyB,
        const hkpShapeContainer& containerShapeA,
        const hkpShapeContainer& containerShapeB,
        uint32_t keyA,
        uint32_t keyB
    ) const override = 0;
    hkBool isCollisionEnabled(
        const hkpShapeRayCastInput& aInput,
        const hkpShape& bShape,
        const hkpShapeContainer& bContainer,
        uint32_t bKey
    ) const override = 0;
    hkBool isCollisionEnabled(const hkpWorldRayCastInput& a, const hkpCollidable& collidableB) const override = 0;

    int32_t numShapeKeyHitsLimitBreached(
        const hkpCollisionInput& input,
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpBvTreeShape* bvTreeShapeB,
        hkAabb& aabb,
        uint32_t* shapeKeysInOut,
        int32_t shapeKeysCapacity
    ) const override;

    // The Havok heap's free, hkReferencedObject's (the interface bases' own would make delete ambiguous).
    static void operator delete(void* apMemory) {
        hkReferencedObject::deleteObject(apMemory);
    }

    uint32_t m_prepad[2];
    hkEnum<hkpFilterType, uint32_t> m_type;
    uint32_t m_postpad[3];
};

// The class's reflection record.
extern hkClass hkpCollisionFilterClass;
