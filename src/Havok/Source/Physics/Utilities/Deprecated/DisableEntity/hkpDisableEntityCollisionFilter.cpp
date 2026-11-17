#include "Havok/Source/Physics/Utilities/Deprecated/DisableEntity/hkpDisableEntityCollisionFilter.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/Array/hkSmallArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollidableCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpShapeCollectionFilter.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityActivationListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"

hkpDisableEntityCollisionFilter::hkpDisableEntityCollisionFilter() {}

// A loaded filter keeps the fields the load wrote.
hkpDisableEntityCollisionFilter::hkpDisableEntityCollisionFilter(hkFinishLoadedObjectFlag aFlag)
  : hkpCollisionFilter(aFlag) {}

// The filter stops listening to each entity it still holds; the array's storage goes with m_disabledEntities. An entity
// with no listener lists has nothing to remove.
hkpDisableEntityCollisionFilter::~hkpDisableEntityCollisionFilter() {
    for (int32_t i = 0; i < m_disabledEntities.m_size; i++) {
        hkpEntity* pEntity = m_disabledEntities.m_data[i];
        hkpEntity::ExtendedListeners* pListeners = pEntity->m_extendedListeners;
        if (!pListeners) {
            continue;
        }
        for (int32_t j = 0; j < pListeners->m_entityListeners.getSize(); j++) {
            if (pListeners->m_entityListeners.begin()[j] == static_cast<hkpEntityListener*>(this)) {
                pEntity->removeEntityListener(this);
                break;
            }
        }
    }
}

// A removed entity leaves the filter, and the filter stops listening to it. The entity's own argument is handed to
// removeEntityFromFilter as the result buffer, so the entity is copied first and the result's byte overwrites the
// argument after the entity is read.
void hkpDisableEntityCollisionFilter::entityRemovedCallback(hkpEntity* entity) {
    hkpEntity* pEntity = entity;
    if (pEntity) {
        removeEntityFromFilter(reinterpret_cast<hkBool*>(&entity), pEntity);
        pEntity->removeEntityListener(this);
    }
}

// Nothing to do: the entity's removal already dropped it.
void hkpDisableEntityCollisionFilter::entityDeletedCallback(hkpEntity* entity) {}

// Two collidables may collide unless either belongs to a held entity.
hkBool hkpDisableEntityCollisionFilter::isCollisionEnabled(const hkpCollidable& a, const hkpCollidable& b) const {
    hkBool result;
    for (int32_t i = 0; i < m_disabledEntities.m_size; i++) {
        const hkpCollidable* collidable = &m_disabledEntities.m_data[i]->m_collidable;
        if (collidable == &a || collidable == &b) {
            result.m_bool = 0;
            return result;
        }
    }
    result.m_bool = 1;
    return result;
}

// Shape collection children always collide.
hkBool hkpDisableEntityCollisionFilter::isCollisionEnabled(
    const hkpCollisionInput& input,
    const hkpCdBody& a,
    const hkpCdBody& b,
    const hkpShapeContainer& bContainer,
    uint32_t bKey
) const {
    hkBool result;
    result.m_bool = 1;
    return result;
}

// Pairs of shape collection children always collide.
hkBool hkpDisableEntityCollisionFilter::isCollisionEnabled(
    const hkpCollisionInput& input,
    const hkpCdBody& collectionBodyA,
    const hkpCdBody& collectionBodyB,
    const hkpShapeContainer& containerShapeA,
    const hkpShapeContainer& containerShapeB,
    uint32_t keyA,
    uint32_t keyB
) const {
    hkBool result;
    result.m_bool = 1;
    return result;
}

// Rays always hit shape collection children.
hkBool hkpDisableEntityCollisionFilter::isCollisionEnabled(
    const hkpShapeRayCastInput& aInput,
    const hkpShape& bShape,
    const hkpShapeContainer& bContainer,
    uint32_t bKey
) const {
    hkBool result;
    result.m_bool = 1;
    return result;
}

// World rays always hit.
hkBool hkpDisableEntityCollisionFilter::isCollisionEnabled(
    const hkpWorldRayCastInput& a,
    const hkpCollidable& collidableB
) const {
    hkBool result;
    result.m_bool = 1;
    return result;
}

hkBool* hkpDisableEntityCollisionFilter::removeEntityFromFilter(hkBool* arResult, hkpEntity* entity) {
    if (entity) {
        int32_t m_size = m_disabledEntities.m_size;
        int32_t v5 = 0;
        if (m_size <= 0) {
        LABEL_7:;
            arResult->m_bool = 0;
            return arResult;
        }
        else {
            hkpEntity** m_data = static_cast<hkArrayBase<hkpEntity*>&>(m_disabledEntities).begin();
            while (*m_data != entity) {
                ++v5;
                ++m_data;
                if (v5 >= m_size) {
                    goto LABEL_7;
                }
            }
            uint32_t v7 = --m_disabledEntities.m_size;
            if (v7 != v5) {
                m_disabledEntities.m_data[v5] = m_disabledEntities.m_data[v7];
            }
            arResult->m_bool = 1;
            return arResult;
        }
    }
    else {
        arResult->m_bool = 0;
        return arResult;
    }
}

hkBool* hkpDisableEntityCollisionFilter::addEntityToFilter(hkBool* apResult, hkpEntity* new_entity) {
    if (!new_entity) {
    LABEL_2:;
        apResult->m_bool = 0;
        return apResult;
    }
    int32_t m_size = m_disabledEntities.m_size;
    int32_t v6 = 0;
    if (m_size > 0) {
        hkpEntity** m_data = m_disabledEntities.begin();
        while (*m_data != new_entity) {
            ++v6;
            ++m_data;
            if (v6 >= m_size) {
                goto LABEL_7;
            }
        }
        goto LABEL_2;
    }
LABEL_7:;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (m_disabledEntities.m_size == (m_disabledEntities.m_capacityAndFlags & hkArrayBase<hkpEntity*>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, &m_disabledEntities, sizeof(hkpEntity*));
    }
    m_disabledEntities.m_data[m_disabledEntities.m_size++] = new_entity;
    hkpEntityListener* v9 = static_cast<hkpEntityListener*>(this);
    if (!new_entity->m_extendedListeners) {
        hkMemoryRouter* v10 = &hkMemoryRouter::getInstance();
        hkpEntity::ExtendedListeners* v11 =
            static_cast<hkpEntity::ExtendedListeners*>(v10->m_heap->blockAlloc(sizeof(hkpEntity::ExtendedListeners)));
        if (v11) {
            v11->m_activationListeners.m_data = nullptr;
            v11->m_activationListeners.m_size = 0;
            v11->m_activationListeners.m_capacityAndFlags =
                hkSmallArray<hkpEntityActivationListener*>::DONT_DEALLOCATE_FLAG;
            v11->m_entityListeners.m_data = nullptr;
            v11->m_entityListeners.m_size = 0;
            v11->m_entityListeners.m_capacityAndFlags = hkSmallArray<hkpEntityListener*>::DONT_DEALLOCATE_FLAG;
        }
        else {
            v11 = nullptr;
        }
        new_entity->m_extendedListeners = v11;
    }
    hkpEntity::ExtendedListeners* m_extendedListeners = new_entity->m_extendedListeners;
    int32_t v13 = m_extendedListeners->m_entityListeners.m_size;
    hkSmallArray<hkpEntityListener*>* p_m_entityListeners = &m_extendedListeners->m_entityListeners;
    int32_t v15 = 0;
    if (!v13) {
        goto LABEL_20;
    }
    for (hkpEntityListener** i = p_m_entityListeners->begin(); *i != v9; ++i) {
        if (++v15 >= v13) {
            new_entity->addEntityListener(v9);
            apResult->m_bool = 1;
            return apResult;
        }
    }
    if (v15 < 0) {
    LABEL_20:;
        new_entity->addEntityListener(v9);
    }
    apResult->m_bool = 1;
    return apResult;
}

const hkClass& hkpDisableEntityCollisionFilter::staticClass() {
    return hkpDisableEntityCollisionFilterClass;
}
