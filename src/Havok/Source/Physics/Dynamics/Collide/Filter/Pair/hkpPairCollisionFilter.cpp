#include "Havok/Source/Physics/Dynamics/Collide/Filter/Pair/hkpPairCollisionFilter.h"

#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollidableCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

#include <new>

float* sub_DEF530(float a1, float* r_eax) {
    r_eax[1] = a1;
    return r_eax;
}

float* sub_DEF540(float a1, float a2, float* r_eax) {
    float fA1 = a1;
    r_eax[0] = fA1;
    float fA2 = a2;
    r_eax[1] = fA2;
    return r_eax;
}

float* sub_DEF550(float a1, float a2, float a3, float a4, float* r_eax) {
    float fA1 = a1;
    r_eax[0] = fA1;
    float fA2 = a2;
    r_eax[1] = fA2;
    float fA3 = a3;
    r_eax[2] = fA3;
    float fA4 = a4;
    r_eax[3] = fA4;
    return r_eax;
}

// Releases the child filter; the map goes with its member.
hkpPairCollisionFilter::~hkpPairCollisionFilter() {
    if (m_childFilter) {
        m_childFilter->removeReference();
    }
}

// The SDK's isCollisionEnabled(collidableA, collidableB): the child filter first, then the pair of the collidables'
// entities (null for a collidable whose owner is no entity).
hkBool
hkpPairCollisionFilter::isCollisionEnabled(const hkpCollidable& collidableA, const hkpCollidable& collidableB) const {
    hkBool result;
    if (m_childFilter && !m_childFilter->isCollisionEnabled(collidableA, collidableB).m_bool) {
        result.m_bool = 0;
        return result;
    }
    const hkpEntity* entityA = nullptr;
    // EXE-INLINED 0x4B59F0
    if (collidableA.m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY) {
        entityA = static_cast<const hkpEntity*>(collidableA.getOwner());
    }
    const hkpEntity* entityB = nullptr;
    // EXE-INLINED 0x4B59F0
    if (collidableB.m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY) {
        entityB = static_cast<const hkpEntity*>(collidableB.getOwner());
    }
    result = isCollisionEnabled(entityA, entityB);
    return result;
}

hkBool hkpPairCollisionFilter::isCollisionEnabled(
    const hkpCollisionInput& input,
    const hkpCdBody& a,
    const hkpCdBody& b,
    const hkpShapeContainer& bContainer,
    uint32_t bKey
) const {
    hkBool result;
    if (!m_childFilter || m_childFilter->isCollisionEnabled(input, a, b, bContainer, bKey).m_bool) {
        result.m_bool = 1;
        return result;
    }
    else {
        result.m_bool = 0;
        return result;
    }
}

hkBool hkpPairCollisionFilter::isCollisionEnabled(
    const hkpCollisionInput& input,
    const hkpCdBody& collectionBodyA,
    const hkpCdBody& collectionBodyB,
    const hkpShapeContainer& containerShapeA,
    const hkpShapeContainer& containerShapeB,
    uint32_t keyA,
    uint32_t keyB
) const {
    hkBool result;
    hkBool v10;
    if (!m_childFilter ||
        (::new (&v10) hkBool(m_childFilter->isCollisionEnabled(
             input, collectionBodyA, collectionBodyB, containerShapeA, containerShapeB, keyA, keyB
         )))
            ->m_bool) {
        result.m_bool = 1;
        return result;
    }
    else {
        result.m_bool = 0;
        return result;
    }
}

hkBool hkpPairCollisionFilter::isCollisionEnabled(
    const hkpShapeRayCastInput& aInput,
    const hkpShape& bShape,
    const hkpShapeContainer& bContainer,
    uint32_t bKey
) const {
    hkBool result;
    hkBool v7;
    if (!m_childFilter ||
        (::new (&v7) hkBool(m_childFilter->isCollisionEnabled(aInput, bShape, bContainer, bKey)))->m_bool) {
        result.m_bool = 1;
        return result;
    }
    else {
        result.m_bool = 0;
        return result;
    }
}

hkBool
hkpPairCollisionFilter::isCollisionEnabled(const hkpWorldRayCastInput& inputA, const hkpCollidable& collidableB) const {
    hkBool result;
    if (!m_childFilter || m_childFilter->isCollisionEnabled(inputA, collidableB).m_bool) {
        result.m_bool = 1;
        return result;
    }
    else {
        result.m_bool = 0;
        return result;
    }
}

// The SDK's constructor: the map starts empty through its own constructor; the child filter is referenced.
hkpPairCollisionFilter::hkpPairCollisionFilter(const hkpCollisionFilter* a1) : m_childFilter(a1) {
    m_type = HK_FILTER_PAIR;
    if (a1) {
        a1->addReference();
    }
}

// The SDK's non-virtual isCollisionEnabled(entityA, entityB): a pair with a null entity collides; any other pair
// collides unless its key is in m_disabledPairs.
hkBool hkpPairCollisionFilter::isCollisionEnabled(const hkpEntity* entityA, const hkpEntity* entityB) const {
    hkBool result;
    if (entityA && entityB) {
        PairFilterKey key;
        calcKey(entityA, entityB, key);
        // A slot index up to m_hashMod (signed compare): the pair is disabled.
        if (m_disabledPairs.findKey(key) <= m_disabledPairs.m_hashMod) {
            result.m_bool = 0;
            return result;
        }
    }
    result.m_bool = 1;
    return result;
}

// The SDK's enableCollisionsBetween: one disable of the pair undone; the pair leaves the map at zero. Returns the
// count left.
int32_t hkpPairCollisionFilter::enableCollisionsBetween(const hkpEntity* entityA, const hkpEntity* entityB) {
    PairFilterKey key;
    calcKey(entityA, entityB, key);
    uint64_t count = m_disabledPairs.getWithDefault(key, 0);
    if (count) {
        count--;
        if (count) {
            m_disabledPairs.insert(*hkMemoryRouter::getInstance().m_heap, key, count);
        }
        else {
            m_disabledPairs.remove(key);
        }
    }
    return static_cast<int32_t>(count);
}

// The SDK's disableCollisionsBetween: the pair's count raised by one. Returns the new count.
int32_t hkpPairCollisionFilter::disableCollisionsBetween(const hkpEntity* entityA, const hkpEntity* entityB) {
    PairFilterKey key;
    calcKey(entityA, entityB, key);
    uint64_t count = m_disabledPairs.getWithDefault(key, 0) + 1;
    m_disabledPairs.insert(*hkMemoryRouter::getInstance().m_heap, key, count);
    return static_cast<int32_t>(count);
}
