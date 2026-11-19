#include "Havok/Source/Physics/Collide/Filter/Group/hkpGroupFilter.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/BvTree3/hkpBvTreeAgent3.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollidableCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/Modifiers/hkpRemoveTerminalsMoppModifierClass.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeBuffer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"

static void cleanupLoadedObjecthkpGroupFilter(void* p);

// The reflected members of hkpGroupFilter.
using hkpGroupFilterClass_MembersArray = const hkClassMember[3];
static hkpGroupFilterClass_MembersArray hkpGroupFilterClass_Members{
    {"nextFreeSystemGroup",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     offsetof(hkpGroupFilter, m_nextFreeSystemGroup),
     nullptr},
    {"collisionLookupTable",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT32},
     {hkClassMember::TYPE_VOID},
     32,
     {0},
     offsetof(hkpGroupFilter, m_collisionLookupTable),
     nullptr},
    {"pad256",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     4,
     {0},
     offsetof(hkpGroupFilter, m_pad256),
     nullptr}
};
hkClass hkpGroupFilterClass(
    "hkpGroupFilter",
    &hkpCollisionFilterClass,
    sizeof(hkpGroupFilter),
    nullptr,
    0,
    nullptr,
    0,
    hkpGroupFilterClass_Members,
    3,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpGroupFilterTypeInfo = {
    "hkpGroupFilter",          finishLoadedObjecthkpGroupFilter, cleanupLoadedObjecthkpGroupFilter,
    getVtablehkpGroupFilter(), &typeid(hkpGroupFilter),          sizeof(hkpGroupFilter)
};

static void cleanupLoadedObjecthkpGroupFilter(void* p) {
    static_cast<hkpGroupFilter*>(p)->~hkpGroupFilter();
}

hkpGroupFilter::hkpGroupFilter() : m_nextFreeSystemGroup(0) {
    m_type.m_storage = HK_FILTER_GROUP;
    for (uint32_t& entry : m_collisionLookupTable) {
        entry = 0xFFFFFFFFu;
    }
}

hkpGroupFilter::~hkpGroupFilter() {}

int32_t hkpGroupFilter::dummyUnused() {
    return 0;
}

hkBool hkpGroupFilter::isCollisionEnabled(const hkpCollidable& a, const hkpCollidable& b) const {
    hkBool result;
    result = isCollisionEnabled(a.m_broadPhaseHandle.m_collisionFilterInfo, b.m_broadPhaseHandle.m_collisionFilterInfo);
    return result;
}

// Havok SDK: b's child shape info (the root collidable's where its container returns USE_COLLIDABLE_FILTER_INFO)
// against a's: the info its nearest collection or bounding volume tree ancestor gives a's branch, else the root
// collidable's. A convex list ancestor enables the collision outright.
hkBool hkpGroupFilter::isCollisionEnabled(
    const hkpCollisionInput& input,
    const hkpCdBody& a,
    const hkpCdBody& b,
    const hkpShapeContainer& bContainer,
    uint32_t bKey
) const {
    uint32_t infoB = bContainer.getCollisionFilterInfo(bKey);
    if (infoB == USE_COLLIDABLE_FILTER_INFO) {
        infoB = b.getRootCollidable()->getCollisionFilterInfo();
    }
    uint32_t infoA;
    if (a.m_parent == nullptr) {
        infoA = static_cast<const hkpCollidable*>(&a)->getCollisionFilterInfo();
    }
    else {
        const hkpCollisionDispatcher* dispatcher = input.m_dispatcher;
        const hkpCdBody* parent = a.m_parent;
        const hkpCdBody* lastBody = &a;
        while (true) {
            hkpShapeType type = static_cast<hkpShapeType>(parent->m_shape->m_type.m_storage);
            if (dispatcher->hasAlternateType(type, HK_SHAPE_COLLECTION)) {
                const hkpShapeContainer* container = static_cast<const hkpShapeCollection*>(parent->m_shape);
                infoA = container->getCollisionFilterInfo(lastBody->m_shapeKey);
                break;
            }
            if (dispatcher->hasAlternateType(type, HK_SHAPE_BV_TREE)) {
                alignas(16) char shapeBuffer[HK_SHAPE_BUFFER_SIZE];
                const hkpShapeContainer* container = hkBvTreeAgent3::getShapeContainerFrom(parent, shapeBuffer);
                infoA = container->getCollisionFilterInfo(lastBody->m_shapeKey);
                break;
            }
            if (dispatcher->hasAlternateType(type, HK_SHAPE_MULTI_SPHERE)) {
                infoA = a.getRootCollidable()->getCollisionFilterInfo();
                break;
            }
            if (dispatcher->hasAlternateType(type, HK_SHAPE_CONVEX_LIST)) {
                return hkBool(true);
            }
            lastBody = parent;
            parent = parent->m_parent;
            if (parent == nullptr) {
                // lastBody is the root collidable.
                infoA = static_cast<const hkpCollidable*>(lastBody)->getCollisionFilterInfo();
                break;
            }
        }
    }
    return isCollisionEnabled(infoA, infoB);
}

// Havok SDK: the filter infos of the two child shapes, each the root collidable's where its container returns
// USE_COLLIDABLE_FILTER_INFO, tested against the layer table and the system groups.
hkBool hkpGroupFilter::isCollisionEnabled(
    const hkpCollisionInput& input,
    const hkpCdBody& collectionBodyA,
    const hkpCdBody& collectionBodyB,
    const hkpShapeContainer& containerShapeA,
    const hkpShapeContainer& containerShapeB,
    uint32_t keyA,
    uint32_t keyB
) const {
    uint32_t infoA = containerShapeA.getCollisionFilterInfo(keyA);
    if (infoA == USE_COLLIDABLE_FILTER_INFO) {
        infoA = collectionBodyA.getRootCollidable()->getCollisionFilterInfo();
    }
    uint32_t infoB = containerShapeB.getCollisionFilterInfo(keyB);
    if (infoB == USE_COLLIDABLE_FILTER_INFO) {
        infoB = collectionBodyB.getRootCollidable()->getCollisionFilterInfo();
    }
    return isCollisionEnabled(infoA, infoB);
}

hkBool hkpGroupFilter::isCollisionEnabled(
    const hkpShapeRayCastInput& aInput,
    const hkpShape& bShape,
    const hkpShapeContainer& bContainer,
    uint32_t bKey
) const {
    hkBool result;
    uint32_t v7 = bContainer.getCollisionFilterInfo(bKey);
    result = isCollisionEnabled(aInput.m_filterInfo, v7);
    return result;
}

hkBool hkpGroupFilter::isCollisionEnabled(const hkpWorldRayCastInput& a, const hkpCollidable& collidableB) const {
    hkBool result;
    result = isCollisionEnabled(a.m_filterInfo, collidableB.m_broadPhaseHandle.m_collisionFilterInfo);
    return result;
}

// Two filter infos in the same nonzero system group (bits 16..31) collide unless one's sub system id (bits 5..9) is
// the other's "don't collide with" id (bits 10..14); otherwise the layer table decides (bits 0..4).
hkBool hkpGroupFilter::isCollisionEnabled(uint32_t infoA, uint32_t infoB) const {
    hkBool result;
    if (((infoA ^ infoB) & 0xFFFF0000) == 0 && (infoA & 0xFFFF0000) != 0) {
        const uint32_t idA = (infoA >> 5) & 0x1F;
        const uint32_t dontCollideB = (infoB >> 10) & 0x1F;
        if (idA == dontCollideB) {
            result.m_bool = false;
            return result;
        }
        const uint32_t idB = (infoB >> 5) & 0x1F;
        const uint32_t dontCollideA = (infoA >> 10) & 0x1F;
        result.m_bool = idB != dontCollideA;
        return result;
    }
    result.m_bool = (m_collisionLookupTable[infoA & 0x1F] & (1u << (infoB & 0x1F))) != 0;
    return result;
}

void hkpGroupFilter::disableCollisionsBetween(int32_t layerA, int32_t layerB) {
    m_collisionLookupTable[layerA] &= (~(1 << layerB));
    m_collisionLookupTable[layerB] &= (~(1 << layerA));
}

// Havok SDK: every layer in layerBitsA collides with every layer in layerBitsB, and the reverse.
void hkpGroupFilter::enableCollisionsUsingBitfield(uint32_t layerBitsA, uint32_t layerBitsB) {
    for (int32_t i = 0; i < 32; i++) {
        const uint32_t b = 1u << i;
        if (b & layerBitsA) {
            m_collisionLookupTable[i] |= layerBitsB;
        }
        if (b & layerBitsB) {
            m_collisionLookupTable[i] |= layerBitsA;
        }
    }
}

void hkpGroupFilter::enableCollisionsBetween(int32_t layerA, int32_t layerB) {
    m_collisionLookupTable[layerA] |= 1 << layerB;
    m_collisionLookupTable[layerB] |= 1 << layerA;
}

// Havok SDK: no layer in layerBitsA collides with any layer in layerBitsB, nor the reverse.
void hkpGroupFilter::disableCollisionsUsingBitfield(uint32_t layerBitsA, uint32_t layerBitsB) {
    for (int32_t i = 0; i < 32; i++) {
        const uint32_t b = 1u << i;
        if (b & layerBitsA) {
            m_collisionLookupTable[i] &= ~layerBitsB;
        }
        if (b & layerBitsB) {
            m_collisionLookupTable[i] &= ~layerBitsA;
        }
    }
}

const hkClass& hkpGroupFilter::staticClass() {
    return hkpGroupFilterClass;
}
