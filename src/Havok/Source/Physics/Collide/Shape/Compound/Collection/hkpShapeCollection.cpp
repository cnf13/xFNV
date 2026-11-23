#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeBuffer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"

namespace {
    // Each member's offset in this struct (-1: disableWelding has no default), then the default of collectionType.
    struct hkpShapeCollection_DefaultStruct {
        int32_t s_defaultOffsets[2];
        hkEnum<hkpShapeCollection::CollectionType, uint8_t> m_collectionType;
    };
} // namespace

// The enum's items and the enum.
using hkpShapeCollectionCollectionTypeEnumItemArray = const hkClassEnum::Item[8];
static hkpShapeCollectionCollectionTypeEnumItemArray hkpShapeCollectionCollectionTypeEnumItems{
    {hkpShapeCollection::COLLECTION_LIST, "COLLECTION_LIST"},
    {hkpShapeCollection::COLLECTION_EXTENDED_MESH, "COLLECTION_EXTENDED_MESH"},
    {hkpShapeCollection::COLLECTION_TRISAMPLED_HEIGHTFIELD, "COLLECTION_TRISAMPLED_HEIGHTFIELD"},
    {hkpShapeCollection::COLLECTION_USER, "COLLECTION_USER"},
    {hkpShapeCollection::COLLECTION_SIMPLE_MESH, "COLLECTION_SIMPLE_MESH"},
    {hkpShapeCollection::COLLECTION_MESH_SHAPE, "COLLECTION_MESH_SHAPE"},
    {hkpShapeCollection::COLLECTION_COMPRESSED_MESH, "COLLECTION_COMPRESSED_MESH"},
    {hkpShapeCollection::COLLECTION_MAX, "COLLECTION_MAX"}
};
using hkpShapeCollectionEnumArray = const hkClassEnum[1];
static hkpShapeCollectionEnumArray hkpShapeCollectionEnums{
    {"CollectionType", hkpShapeCollectionCollectionTypeEnumItems, 8, nullptr, 0}
};

static const hkClassEnum* hkpShapeCollectionCollectionTypeEnum = &hkpShapeCollectionEnums[0];

// The class record's member table.
using hkpShapeCollectionClassMemberArray = hkClassMember[2];
static hkpShapeCollectionClassMemberArray hkpShapeCollectionClass_Members{
    {"disableWelding", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, 0, 0x14, nullptr},
    {"collectionType",
     nullptr,
     hkpShapeCollectionCollectionTypeEnum,
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_UINT8},
     0,
     0,
     0x15,
     nullptr}
};

// The defaults the class record names.
static const hkpShapeCollection_DefaultStruct hkpShapeCollection_Default{
    {-1, 0x8},
    {hkpShapeCollection::COLLECTION_USER}
};

// Passes one implemented interface with a null table.
hkClass hkpShapeCollectionClass(
    "hkpShapeCollection",
    &hkpShapeClass,
    0x18,
    nullptr,
    1,
    hkpShapeCollectionEnums,
    1,
    hkpShapeCollectionClass_Members,
    2,
    &hkpShapeCollection_Default,
    nullptr,
    0,
    0
);

hkpShapeCollection::hkpShapeCollection(hkpShapeType type, hkpShapeCollection::CollectionType collectionType)
  : hkpShape(type), m_disableWelding(false) {
    m_collectionType.m_storage = static_cast<uint8_t>(collectionType);
}

// The loaded object keeps what was loaded; a finished one is a user collection until a derived class says otherwise.
hkpShapeCollection::hkpShapeCollection(hkFinishLoadedObjectFlag flag) : hkpShape(flag) {
    if (flag.m_finishing) {
        m_type = HK_SHAPE_COLLECTION;
        m_collectionType.m_storage = COLLECTION_USER;
    }
}

// The base's statistics, always for this class's record; cls is ignored.
void hkpShapeCollection::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    hkReferencedObject::calcContentStatistics(collector, &hkpShapeClass);
}

float hkpShapeCollection::getMaximumProjection(const hkVector4& direction) const {
    HK_TIMER_BEGIN("hkpShapeCollection::getMaximumProjection", nullptr);
    float result = -HK_REAL_MAX;
    alignas(16) char buffer[HK_SHAPE_BUFFER_SIZE];
    // The keys and children are read through the hkpShapeContainer subobject.
    const hkpShapeContainer* pContainer = static_cast<const hkpShapeContainer*>(this);
    for (uint32_t key = pContainer->getFirstKey(); key != HK_INVALID_SHAPE_KEY; key = pContainer->getNextKey(key)) {
        const hkpShape* pChildShape = pContainer->getChildShape(key, buffer);
        float projection = pChildShape->getMaximumProjection(direction);
        if (!(projection < result)) {
            result = projection;
        }
    }
    HK_TIMER_END();
    return result;
}

// The collection's container base, or null for a null this.
const hkpShapeContainer* hkpShapeCollection::getContainer() const {
    return const_cast<hkpShapeCollection*>(this);
}

// The union of the children's boxes, starting from the inverted largest box (+max as min, -max as max).
void hkpShapeCollection::getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
    HK_TIMER_BEGIN("hkpShapeCollection::getAabb", nullptr);
    out.m_min.m_quad = _mm_shuffle_ps(_mm_set_ss(HK_REAL_MAX), _mm_set_ss(HK_REAL_MAX), 0);
    out.m_max.m_quad = _mm_shuffle_ps(_mm_set_ss(-HK_REAL_MAX), _mm_set_ss(-HK_REAL_MAX), 0);
    hkAabb childAabb;
    alignas(16) char buffer[HK_SHAPE_BUFFER_SIZE];
    // The keys and children are read through the hkpShapeContainer subobject.
    const hkpShapeContainer* pContainer = static_cast<const hkpShapeContainer*>(this);
    for (uint32_t key = pContainer->getFirstKey(); key != HK_INVALID_SHAPE_KEY; key = pContainer->getNextKey(key)) {
        const hkpShape* pChildShape = pContainer->getChildShape(key, buffer);
        pChildShape->getAabbImpl(localToWorld, tolerance, childAabb);
        out.m_min.m_quad = _mm_min_ps(out.m_min.m_quad, childAabb.m_min.m_quad);
        out.m_max.m_quad = _mm_max_ps(out.m_max.m_quad, childAabb.m_max.m_quad);
    }
    HK_TIMER_END();
}

hkBool hkpShapeCollection::castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const {
    HK_TIMER_BEGIN("rcShpCollect", nullptr);
    ++results.m_shapeKeyIndex;
    alignas(16) char buffer[HK_SHAPE_BUFFER_SIZE];
    // The keys and children are read through the hkpShapeContainer subobject.
    const hkpShapeContainer* pContainer = static_cast<const hkpShapeContainer*>(this);
    uint32_t bestKey = HK_INVALID_SHAPE_KEY;
    if (!input.m_rayShapeCollectionFilter) {
        for (uint32_t key = pContainer->getFirstKey(); key != HK_INVALID_SHAPE_KEY; key = pContainer->getNextKey(key)) {
            const hkpShape* pChildShape = pContainer->getChildShape(key, buffer);
            if (pChildShape->castRayImpl(input, results).m_bool) {
                bestKey = key;
            }
        }
    }
    else {
        for (uint32_t key = pContainer->getFirstKey(); key != HK_INVALID_SHAPE_KEY; key = pContainer->getNextKey(key)) {
            if (input.m_rayShapeCollectionFilter->isCollisionEnabled(input, *this, *pContainer, key).m_bool) {
                const hkpShape* pChildShape = pContainer->getChildShape(key, buffer);
                if (pChildShape->castRayImpl(input, results).m_bool) {
                    bestKey = key;
                }
            }
        }
    }
    --results.m_shapeKeyIndex;
    if (bestKey != HK_INVALID_SHAPE_KEY) {
        results.m_shapeKeys[results.m_shapeKeyIndex] = bestKey;
    }
    HK_TIMER_END();
    hkBool result;
    result.m_bool = bestKey != HK_INVALID_SHAPE_KEY;
    return result;
}

void hkpShapeCollection::castRayWithCollectorImpl(
    const hkpShapeRayCastInput& input,
    const hkpCdBody& cdBody,
    hkpRayHitCollector& collector
) const {
    HK_TIMER_BEGIN("rcShpCollect", nullptr);
    alignas(16) char buffer[HK_SHAPE_BUFFER_SIZE];
    // The keys and children are read through the hkpShapeContainer subobject.
    const hkpShapeContainer* pContainer = static_cast<const hkpShapeContainer*>(this);
    if (input.m_rayShapeCollectionFilter) {
        for (uint32_t key = pContainer->getFirstKey(); key != HK_INVALID_SHAPE_KEY; key = pContainer->getNextKey(key)) {
            if (input.m_rayShapeCollectionFilter->isCollisionEnabled(input, *this, *pContainer, key).m_bool) {
                const hkpShape* pChildShape = pContainer->getChildShape(key, buffer);
                hkpCdBody childBody;
                childBody.m_parent = &cdBody;
                childBody.m_motion.pState = cdBody.m_motion.pState;
                childBody.m_shape = pChildShape;
                childBody.m_shapeKey = key;
                pChildShape->castRayWithCollectorImpl(input, childBody, collector);
            }
        }
    }
    else {
        for (uint32_t key = pContainer->getFirstKey(); key != HK_INVALID_SHAPE_KEY; key = pContainer->getNextKey(key)) {
            const hkpShape* pChildShape = pContainer->getChildShape(key, buffer);
            hkpCdBody childBody;
            childBody.m_parent = &cdBody;
            childBody.m_motion.pState = cdBody.m_motion.pState;
            childBody.m_shape = pChildShape;
            childBody.m_shapeKey = key;
            pChildShape->castRayWithCollectorImpl(input, childBody, collector);
        }
    }
    HK_TIMER_END();
}

void hkpShapeCollection::initWeldingInfo(hkpWeldingUtility::WeldingType weldingType) {
    ;
}

void hkpShapeCollection::setWeldingInfo(uint32_t key, int16_t weldingInfo) {
    ;
}
