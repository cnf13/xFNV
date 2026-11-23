#include "Havok/Source/Physics/Utilities/Collide/Filter/ConstrainedSystem/hkpConstrainedSystemFilter.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnEntry.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnMachine.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollidableCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldAgentUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

#include <new>

static void cleanupLoadedObjecthkpConstrainedSystemFilter(void* p);

// The member table of hkpConstrainedSystemFilter's class record, as the Havok SDK's class file has it.
using hkpConstrainedSystemFilterClass_MembersArray = const hkClassMember[1];
static hkpConstrainedSystemFilterClass_MembersArray hkpConstrainedSystemFilterClass_Members{
    {"otherFilter",
     &hkpCollisionFilterClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x34,
     nullptr}
};
hkClass hkpConstrainedSystemFilterClass(
    "hkpConstrainedSystemFilter",
    &hkpCollisionFilterClass,
    56,
    nullptr,
    1,
    nullptr,
    0,
    hkpConstrainedSystemFilterClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpConstrainedSystemFilterTypeInfo = {
    "hkpConstrainedSystemFilter",
    finishLoadedObjecthkpConstrainedSystemFilter,
    cleanupLoadedObjecthkpConstrainedSystemFilter,
    getVtablehkpConstrainedSystemFilter(),
    &typeid(hkpConstrainedSystemFilter),
    sizeof(hkpConstrainedSystemFilter)
};

static void cleanupLoadedObjecthkpConstrainedSystemFilter(void* p) {
    static_cast<hkpConstrainedSystemFilter*>(p)->~hkpConstrainedSystemFilter();
}

// A loaded filter keeps the fields the load wrote.
hkpConstrainedSystemFilter::hkpConstrainedSystemFilter(hkFinishLoadedObjectFlag aFlag) : hkpCollisionFilter(aFlag) {}

// The other filter, with a reference held to it.
hkpConstrainedSystemFilter::hkpConstrainedSystemFilter(const hkpCollisionFilter* otherFilter)
  : m_otherFilter(otherFilter) {
    if (otherFilter) {
        otherFilter->addReference();
    }
}

// The reference to the other filter is dropped.
hkpConstrainedSystemFilter::~hkpConstrainedSystemFilter() {
    if (m_otherFilter) {
        m_otherFilter->removeReference();
    }
}

hkBool hkpConstrainedSystemFilter::isCollisionEnabled(const hkpCollidable& a, const hkpCollidable& b) const {
    hkBool result;
    hkpEntity* entityA;
    hkpEntity* entityB;
    if (a.m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY) {
        entityA = static_cast<hkpEntity*>(a.getOwner());
    }
    else {
        entityA = nullptr;
    }
    if (b.m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY) {
        entityB = static_cast<hkpEntity*>(b.getOwner());
    }
    else {
        entityB = nullptr;
    }
    if (m_otherFilter && !m_otherFilter->isCollisionEnabled(a, b).m_bool) {
        result.m_bool = 0;
        return result;
    }
    if (!entityA || !entityB) {
        result.m_bool = 1;
        return result;
    }
    // Walk the constraints of the entity with fewer of them, looking for one to the other entity.
    const hkpEntity* entity;
    const hkpEntity* other;
    int32_t numB = entityB->getNumConstraints();
    if (entityA->getNumConstraints() > numB) {
        entity = entityB;
        other = entityA;
    }
    else {
        entity = entityA;
        other = entityB;
    }
    int32_t numConstraints = entity->getNumConstraints();
    for (int32_t i = 0; i < numConstraints; i++) {
        const hkpConstraintInstance* constraint = entity->getConstraint(i);
        if (constraint &&
            constraint->getDataRw()->getType() != hkpConstraintData::CONSTRAINT_TYPE_CONTACT &&
            (constraint->m_entities[0] == other || constraint->m_entities[1] == other)) {
            result.m_bool = 0;
            return result;
        }
    }
    result.m_bool = 1;
    return result;
}

hkBool hkpConstrainedSystemFilter::isCollisionEnabled(
    const hkpCollisionInput& input,
    const hkpCdBody& a,
    const hkpCdBody& b,
    const hkpShapeContainer& bContainer,
    uint32_t bKey
) const {
    hkBool result;
    if (!m_otherFilter || m_otherFilter->isCollisionEnabled(input, a, b, bContainer, bKey).m_bool) {
        result.m_bool = 1;
        return result;
    }

    result.m_bool = 0;
    return result;
}

hkBool hkpConstrainedSystemFilter::isCollisionEnabled(
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
    if (!m_otherFilter ||
        (::new (&v10) hkBool(m_otherFilter->isCollisionEnabled(
             input,
             collectionBodyA,
             collectionBodyB,
             containerShapeA,
             containerShapeB,
             keyA,
             keyB
         )))
            ->m_bool) {
        result.m_bool = 1;
        return result;
    }

    result.m_bool = 0;
    return result;
}

hkBool hkpConstrainedSystemFilter::isCollisionEnabled(
    const hkpShapeRayCastInput& aInput,
    const hkpShape& bShape,
    const hkpShapeContainer& bContainer,
    uint32_t bKey
) const {
    hkBool result;
    hkBool v7;
    if (!m_otherFilter ||
        (::new (&v7) hkBool(m_otherFilter->isCollisionEnabled(aInput, bShape, bContainer, bKey)))->m_bool) {
        result.m_bool = 1;
        return result;
    }

    result.m_bool = 0;
    return result;
}

hkBool
hkpConstrainedSystemFilter::isCollisionEnabled(const hkpWorldRayCastInput& a, const hkpCollidable& collidableB) const {
    hkBool result;
    if (!m_otherFilter || m_otherFilter->isCollisionEnabled(a, collidableB).m_bool) {
        result.m_bool = 1;
        return result;
    }

    result.m_bool = 0;
    return result;
}

void hkpConstrainedSystemFilter::constraintAddedCallback(hkpConstraintInstance* constraint) {
    if (constraint->getDataRw()->getType() != 11) {
        hkpAgentNnEntry* Agent = hkAgentNnMachine_FindAgent(
            &constraint->m_entities[0]->m_collidable,
            &constraint->m_entities[1]->m_collidable
        );
        if (Agent) {
            hkpWorldAgentUtil::removeAgentAndItsToiEvents(Agent);
        }
    }
}

void hkpConstrainedSystemFilter::constraintRemovedCallback(hkpConstraintInstance* constraint) {
    ;
}

const hkClass* hkpConstrainedSystemFilter::getClassType() const {
    return &hkpConstrainedSystemFilterClass;
}
