#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"

#include <stdint.h>

void finishLoadedObjecthkpGroupCollisionFilter(void* p, int32_t finishing);
// Havok SDK signature (hkpUtilities.lib, tied by reloc).
const void* getVtablehkpGroupCollisionFilter();

// The deprecated collision filter of Havok 1's 32 collision groups: bit i of group j's mask says whether group i
// collides with j.
class hkpGroupCollisionFilter : public hkpCollisionFilter {
public:
    hkpGroupCollisionFilter();
    hkpGroupCollisionFilter(hkFinishLoadedObjectFlag aFlag) : hkpCollisionFilter(aFlag) {}
    ~hkpGroupCollisionFilter() override;

    // The five filter interfaces of hkpCollisionFilter, answered from the group masks.
    hkBool isCollisionEnabled(const hkpCollidable& a, const hkpCollidable& b) const override;
    hkBool isCollisionEnabled(
        const hkpCollisionInput& input,
        const hkpCdBody& a,
        const hkpCdBody& b,
        const hkpShapeContainer& bContainer,
        uint32_t bKey
    ) const override;
    hkBool isCollisionEnabled(
        const hkpCollisionInput& input,
        const hkpCdBody& collectionBodyA,
        const hkpCdBody& collectionBodyB,
        const hkpShapeContainer& containerShapeA,
        const hkpShapeContainer& containerShapeB,
        uint32_t keyA,
        uint32_t keyB
    ) const override;
    hkBool isCollisionEnabled(
        const hkpShapeRayCastInput& aInput,
        const hkpShape& bShape,
        const hkpShapeContainer& bContainer,
        uint32_t bKey
    ) const override;
    hkBool isCollisionEnabled(const hkpWorldRayCastInput& a, const hkpCollidable& collidableB) const override;

    // Sets (enable) or clears (disable) the collision bits between the two group sets; with both sets empty, sets
    // m_noGroupCollisionEnabled instead.
    void enableCollisionGroups(uint32_t groupBitsA, uint32_t groupBitsB);
    void disableCollisionGroups(uint32_t groupBitsA, uint32_t groupBitsB);
    // Whether the groups of two filter infos collide; the five filter interfaces call it with this as the filter's
    // primary base and read m_noGroupCollisionEnabled and m_collisionGroups.
    hkBool isCollisionEnabled(uint32_t infoA, uint32_t infoB) const;

    static const hkClass& staticClass();

    hkBool m_noGroupCollisionEnabled;
    uint32_t m_collisionGroups[32];
};

extern hkClass hkpGroupCollisionFilterClass;
extern hkTypeInfo hkpGroupCollisionFilterTypeInfo;
