#include "Havok/Source/Physics/Collide/Filter/List/hkpCollisionFilterList.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollidableCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"

#include <new>

hkpCollisionFilterList::hkpCollisionFilterList() {}

hkpCollisionFilterList::hkpCollisionFilterList(hkFinishLoadedObjectFlag aFlag)
  : hkpCollisionFilter(aFlag), m_collisionFilters(aFlag) {
    if (aFlag.m_finishing) {
        m_type.m_storage = HK_FILTER_LIST;
    }
}

// Natively: hkpCollisionFilterList(const hkArray<const hkpCollisionFilter*, hkContainerHeapAllocator>&) below.
hkpCollisionFilterList::hkpCollisionFilterList(
    const hkArray<const hkpCollisionFilter*, hkContainerHeapAllocator>& collisionFilters
) {
    m_type.m_storage = HK_FILTER_LIST;
    for (const hkpCollisionFilter* pFilter : collisionFilters) {
        addCollisionFilter(pFilter);
    }
}

hkpCollisionFilterList::~hkpCollisionFilterList() {
    for (const hkpCollisionFilter* pFilter : m_collisionFilters) {
        pFilter->removeReference();
    }
}

hkBool hkpCollisionFilterList::isCollisionEnabled(const hkpCollidable& a, const hkpCollidable& b) const {
    hkBool result;
    const hkpCollidable* v6 = &b;
    for (int32_t v5 = m_collisionFilters.m_size - 1; v5 >= 0; --v5) {
        const hkpCollisionFilter* v7 = m_collisionFilters.m_data[v5];
        if (!v7->isCollisionEnabled(a, *v6).m_bool) {
            result.m_bool = 0;
            return result;
        }
    }
    result.m_bool = 1;
    return result;
}

hkBool hkpCollisionFilterList::isCollisionEnabled(
    const hkpCollisionInput& input,
    const hkpCdBody& a,
    const hkpCdBody& b,
    const hkpShapeContainer& bContainer,
    uint32_t bKey
) const {
    hkBool result;
    for (int32_t v8 = m_collisionFilters.m_size - 1; v8 >= 0; --v8) {
        const hkpCollisionFilter* v10 = m_collisionFilters.m_data[v8];
        if (!v10->isCollisionEnabled(input, a, b, bContainer, bKey).m_bool) {
            result.m_bool = 0;
            return result;
        }
    }
    result.m_bool = 1;
    return result;
}

hkBool hkpCollisionFilterList::isCollisionEnabled(
    const hkpCollisionInput& input,
    const hkpCdBody& collectionBodyA,
    const hkpCdBody& collectionBodyB,
    const hkpShapeContainer& containerShapeA,
    const hkpShapeContainer& containerShapeB,
    uint32_t keyA,
    uint32_t keyB
) const {
    hkBool result;
    for (int32_t v10 = m_collisionFilters.m_size - 1; v10 >= 0; --v10) {
        hkBool v14;
        const hkpCollisionFilter* v12 = m_collisionFilters.m_data[v10];
        if (!(::new (&v14) hkBool(v12->isCollisionEnabled(
                  input,
                  collectionBodyA,
                  collectionBodyB,
                  containerShapeA,
                  containerShapeB,
                  keyA,
                  keyB
              )))
                 ->m_bool) {
            result.m_bool = 0;
            return result;
        }
    }
    result.m_bool = 1;
    return result;
}

hkBool hkpCollisionFilterList::isCollisionEnabled(
    const hkpShapeRayCastInput& aInput,
    const hkpShape& bShape,
    const hkpShapeContainer& bContainer,
    uint32_t bKey
) const {
    hkBool result;
    for (int32_t v7 = m_collisionFilters.m_size - 1; v7 >= 0; --v7) {
        hkBool v11;
        const hkpCollisionFilter* v9 = m_collisionFilters.m_data[v7];
        if (!(::new (&v11) hkBool(v9->isCollisionEnabled(aInput, bShape, bContainer, bKey)))->m_bool) {
            result.m_bool = 0;
            return result;
        }
    }
    result.m_bool = 1;
    return result;
}

hkBool
hkpCollisionFilterList::isCollisionEnabled(const hkpWorldRayCastInput& a, const hkpCollidable& collidableB) const {
    hkBool result;
    const hkpCollidable* v6 = &collidableB;
    for (int32_t v5 = m_collisionFilters.m_size - 1; v5 >= 0; --v5) {
        const hkpCollisionFilter* v7 = m_collisionFilters.m_data[v5];
        if (!v7->isCollisionEnabled(a, *v6).m_bool) {
            result.m_bool = 0;
            return result;
        }
    }
    result.m_bool = 1;
    return result;
}

// The filter is taken as const, but the reference count changes, so it is cast to non-const.
void hkpCollisionFilterList::addCollisionFilter(const hkpCollisionFilter* filter) {
    hkpCollisionFilter* pFilter = const_cast<hkpCollisionFilter*>(filter);
    pFilter->addReference();
    m_collisionFilters.pushBack(pFilter);
}

void hkpCollisionFilterList::removeCollisionFilter(const hkpCollisionFilter* filter) {
    int32_t m_size = m_collisionFilters.m_size;
    int32_t v3 = 0;
    if (m_size <= 0) {
        v3 = -1;
    }
    else {
        const hkpCollisionFilter** m_data = m_collisionFilters.begin();
        while (*m_data != filter) {
            ++v3;
            ++m_data;
            if (v3 >= m_size) {
                v3 = -1;
                break;
            }
        }
    }
    int32_t v5 = --m_collisionFilters.m_size;
    if (v5 != v3) {
        m_collisionFilters.m_data[v3] = m_collisionFilters.m_data[v5];
    }
    const_cast<hkpCollisionFilter*>(filter)->removeReference();
}

// Havok SDK: the list's filters.
const hkArray<const hkpCollisionFilter*, hkContainerHeapAllocator>& hkpCollisionFilterList::getCollisionFilters() {
    return m_collisionFilters;
}
