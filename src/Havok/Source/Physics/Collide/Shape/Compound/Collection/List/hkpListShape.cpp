#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/List/hkpListShape.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/BitField/hkBitField.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"

#include <bit>
#include <xmmintrin.h>

static void cleanupLoadedObjecthkpListShape(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpListShapeChildInfoTypeInfo = {
    "hkpListShapeChildInfo",
    nullptr,
    cleanupLoadedObjecthkpListShapeChildInfo,
    nullptr,
    &typeid(hkpListShape::ChildInfo),
    sizeof(hkpListShape::ChildInfo)
};

// The reflection's member and enum tables.
using hkpListShape_ChildInfoClass_MembersArray = const hkClassMember[4];
static hkpListShape_ChildInfoClass_MembersArray hkpListShape_ChildInfoClass_Members{
    {"shape",
     &hkpShapeClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {hkClassMember::ALIGN_16},
     0x0,
     nullptr},
    {"collisionFilterInfo",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x4,
     nullptr},
    {"shapeSize",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x8,
     nullptr},
    {"numChildShapes",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0xC,
     nullptr}
};
using hkpListShapeListShapeFlagsEnumItemsArray = const hkClassEnum::Item[2];
static hkpListShapeListShapeFlagsEnumItemsArray hkpListShapeListShapeFlagsEnumItems{
    {0, "ALL_FLAGS_CLEAR"},
    {1, "DISABLE_SPU_CACHE_FOR_LIST_CHILD_INFO"}
};
using hkpListShapeEnumsArray = const hkClassEnum[1];
static hkpListShapeEnumsArray hkpListShapeEnums{
    {"ListShapeFlags", hkpListShapeListShapeFlagsEnumItems, 2, nullptr, {0}}
};
using hkpListShapeClass_MembersArray = const hkClassMember[6];
static hkpListShapeClass_MembersArray hkpListShapeClass_Members{
    {"childInfo",
     &hkpListShapeChildInfoClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x18,
     nullptr},
    {"flags", nullptr, nullptr, {hkClassMember::TYPE_UINT16}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x24, nullptr},
    {"numDisabledChildren",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT16},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x26,
     nullptr},
    {"aabbHalfExtents",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x30,
     nullptr},
    {"aabbCenter", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x40, nullptr},
    {"enabledChildren",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT32},
     {hkClassMember::TYPE_VOID},
     8,
     {0},
     0x50,
     nullptr}
};
hkClass hkpListShapeChildInfoClass(
    "hkpListShapeChildInfo",
    nullptr,
    0x10,
    nullptr,
    0,
    nullptr,
    0,
    hkpListShape_ChildInfoClass_Members,
    4,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkpListShapeClass(
    "hkpListShape",
    &hkpShapeCollectionClass,
    0x70,
    nullptr,
    0,
    hkpListShapeEnums,
    1,
    hkpListShapeClass_Members,
    6,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpListShapeTypeInfo{
    "hkpListShape",
    finishLoadedObjecthkpListShape,
    cleanupLoadedObjecthkpListShape,
    getVtablehkpListShape(),
    &typeid(hkpListShape),
    sizeof(hkpListShape)
};

static void cleanupLoadedObjecthkpListShape(void* p) {
    static_cast<hkpListShape*>(p)->~hkpListShape();
}

const hkClass& hkpListShape::ChildInfo::staticClass() {
    return hkpListShapeChildInfoClass;
}

hkpListShape::hkpListShape(
    const hkpShape* const* apShapes,
    int32_t aiNumShapes,
    hkpShapeContainer::ReferencePolicy aePolicy
)
  : hkpShapeCollection(HK_SHAPE_LIST, COLLECTION_LIST), m_flags(0), m_numDisabledChildren(0) {
    m_childInfo.reserve(4);
    setShapes(apShapes, aiNumShapes, nullptr, aePolicy);
    for (uint32_t& enabled : m_enabledChildren) {
        enabled = 0xFFFFFFFF;
    }
}

// The loaded children stay as loaded; a finished object is a list again.
hkpListShape::hkpListShape(hkFinishLoadedObjectFlag flag) : hkpShapeCollection(flag), m_childInfo(flag) {
    if (flag.m_finishing) {
        m_type = HK_SHAPE_LIST;
        m_collectionType.m_storage = COLLECTION_LIST;
    }
}

// Each child's reference goes back; the array frees its own storage.
hkpListShape::~hkpListShape() {
    for (const ChildInfo& child : m_childInfo) {
        child.m_shape->removeReference();
    }
}

void hkpListShape::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    int32_t m_capacityAndFlags = m_childInfo.m_capacityAndFlags;
    if (m_capacityAndFlags >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t AllocatedSize = hkStatisticsCollector::getAllocatedSize(
            m_childInfo.begin(),
            static_cast<int32_t>(sizeof(*m_childInfo.begin())) * m_capacityAndFlags
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY,
            "ChildPtrs",
            m_childInfo.begin(),
            static_cast<int32_t>(sizeof(*m_childInfo.begin())) * m_childInfo.m_size,
            AllocatedSize
        );
    }
    int32_t v6 = 0;
    if (m_childInfo.m_size > 0) {
        int32_t v7 = 0;
        do {
            collector->addReferencedObject("Child", m_childInfo.m_data[v7].m_shape, 0);
            ++v6;
            ++v7;
        }
        while (v6 < m_childInfo.m_size);
    }
    hkpShapeCollection::calcContentStatistics(collector, &hkpShapeCollectionClass);
}

// Each child's SPU size goes into its info, with the list's child count; a child that does not fit fails the list.
int32_t hkpListShape::calcSizeForSpu(const hkpShape::CalcSizeForSpuInput& input, int32_t spuBufferSizeLeft) const {
    if (input.m_midphaseAgent3Registered && m_childInfo.m_size > 252) {
        return -1;
    }
    if (m_childInfo.m_size >= 4095) {
        return -1;
    }
    for (int32_t i = 0; i < m_childInfo.m_size; ++i) {
        ChildInfo& child = m_childInfo.m_data[i];
        child.m_shapeSize = child.m_shape->calcSizeForSpu(input, 256);
        m_childInfo.m_data[i].m_numChildShapes = m_childInfo.m_size;
        if (m_childInfo.m_data[i].m_shapeSize == -1) {
            return -1;
        }
    }
    return 112;
}

// An empty stored box (all half extents <= 0) is recomputed first. Havok's source casts const away for that:
// getAabbImpl is const and recalcAabbExtents is not (hkpListShape.h).
void hkpListShape::getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
    hkVector4 v5;
    if (_mm_movemask_ps(_mm_cmple_ps(m_aabbHalfExtents.m_quad, hkVector4Zero.m_quad)) == 15) {
        const_cast<hkpListShape*>(this)->recalcAabbExtents();
    }
    // HK_REAL_HIGH, broadcast to all four lanes.
    v5.m_quad = _mm_set1_ps(1.70141e+38f);
    out.m_min.m_quad = v5.m_quad;
    int32_t v6 = 0;
    out.m_max.m_quad = _mm_xor_ps(v5.m_quad, hkQuadRealNegateMask);
    if (m_childInfo.m_size > 0) {
        int32_t v8 = 0;
        do {
            hkVector4 v7;
            hkAabb v9;
            m_childInfo.m_data[v8].m_shape->getAabbImpl(localToWorld, tolerance, v9);
            ++v8;
            v7.m_quad = v9.m_max.m_quad;
            out.m_min.m_quad = _mm_min_ps(out.m_min.m_quad, v9.m_min.m_quad);
            ++v6;
            out.m_max.m_quad = _mm_max_ps(out.m_max.m_quad, v7.m_quad);
        }
        while (v6 < m_childInfo.m_size);
    }
}

// Casts the ray against every enabled child (each one the filter lets through, when the input has one) one level
// down in output's shape keys; the last child that reports a hit gives the key stored at this level.
hkBool hkpListShape::castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const {
    HK_TIMER_BEGIN("rcList", nullptr);
    results.changeLevel(1);
    uint32_t hitKey = HK_INVALID_SHAPE_KEY;
    if (!input.m_rayShapeCollectionFilter) {
        for (int32_t i = 0; i < m_childInfo.m_size; ++i) {
            if (isChildEnabled(i) && m_childInfo.m_data[i].m_shape->castRayImpl(input, results).m_bool) {
                hitKey = i;
            }
        }
    }
    else {
        for (int32_t i = 0; i < m_childInfo.m_size; ++i) {
            if (isChildEnabled(i) &&
                input.m_rayShapeCollectionFilter->isCollisionEnabled(input, *this, *this, i).m_bool &&
                m_childInfo.m_data[i].m_shape->castRayImpl(input, results).m_bool) {
                hitKey = i;
            }
        }
    }
    results.changeLevel(-1);
    if (hitKey != HK_INVALID_SHAPE_KEY) {
        results.setKey(hitKey);
    }
    HK_TIMER_END();
    hkBool result;
    result.m_bool = hitKey != HK_INVALID_SHAPE_KEY;
    return result;
}

// Casts the ray against every enabled child (each one the filter lets through, when the input has one), with a child
// body below cdBody keyed by the child's index.
void hkpListShape::castRayWithCollectorImpl(
    const hkpShapeRayCastInput& input,
    const hkpCdBody& cdBody,
    hkpRayHitCollector& collector
) const {
    HK_TIMER_BEGIN("rcList", nullptr);
    hkpCdBody childBody;
    if (!input.m_rayShapeCollectionFilter) {
        for (int32_t i = 0; i < m_childInfo.m_size; ++i) {
            if (isChildEnabled(i)) {
                const hkpShape* childShape = m_childInfo.m_data[i].m_shape;
                childBody.m_parent = &cdBody;
                childBody.m_motion.pState = cdBody.m_motion.pState;
                childBody.m_shape = childShape;
                childBody.m_shapeKey = i;
                childShape->castRayWithCollectorImpl(input, childBody, collector);
            }
        }
    }
    else {
        for (int32_t i = 0; i < m_childInfo.m_size; ++i) {
            if (isChildEnabled(i) &&
                input.m_rayShapeCollectionFilter->isCollisionEnabled(input, *this, *this, i).m_bool) {
                const hkpShape* childShape = m_childInfo.m_data[i].m_shape;
                childBody.m_motion.pState = cdBody.m_motion.pState;
                childBody.m_parent = &cdBody;
                childBody.m_shape = childShape;
                childBody.m_shapeKey = i;
                childShape->castRayWithCollectorImpl(input, childBody, collector);
            }
        }
    }
    HK_TIMER_END();
}

// The children past the disabled ones' range are counted; the disabled ones are not.
int32_t hkpListShape::getNumChildShapes() const {
    return m_childInfo.m_size - m_numDisabledChildren;
}

uint32_t hkpListShape::getFirstKey() const {
    return hkpListShape::getNextKey(0xFFFFFFFF);
}

// The next enabled child after oldKey; children from MAX_DISABLED_CHILDREN (0x100) on are always enabled.
uint32_t hkpListShape::getNextKey(uint32_t oldKey) const {
    for (int32_t i = oldKey + 1; i < m_childInfo.m_size; ++i) {
        if (static_cast<uint32_t>(i) >= 0x100 || (m_enabledChildren[i >> 5] & (1 << (i & 0x1F)))) {
            return i;
        }
    }
    return 0xFFFFFFFF;
}

uint32_t hkpListShape::getCollisionFilterInfo(uint32_t key) const {
    return m_childInfo.m_data[key].m_collisionFilterInfo;
}

const hkpShape* hkpListShape::getChildShape(uint32_t key, char (&buffer)[512]) const {
    return m_childInfo.m_data[key].m_shape;
}

void hkpListShape::setCollisionFilterInfo(uint32_t index, uint32_t filterInfo) {
    m_childInfo.m_data[index].m_collisionFilterInfo = filterInfo;
}

// The first child's box seeds t; every other child's box, cast into a stack box, widens it.
void hkpListShape::recalcAabbExtents(hkAabb& t) {
    m_childInfo.m_data[0].m_shape->getAabbImpl(hkTransformIdentity, 0.0f, t);
    for (int32_t i = 1; i < m_childInfo.m_size; ++i) {
        hkAabb childAabb;
        m_childInfo.m_data[i].m_shape->getAabbImpl(hkTransformIdentity, 0.0f, childAabb);
        t.m_min.m_quad = _mm_min_ps(t.m_min.m_quad, childAabb.m_min.m_quad);
        t.m_max.m_quad = _mm_max_ps(t.m_max.m_quad, childAabb.m_max.m_quad);
    }
    m_aabbCenter.m_quad = _mm_mul_ps(_mm_add_ps(t.m_max.m_quad, t.m_min.m_quad), hkQuadRealHalf);
    m_aabbHalfExtents.m_quad = _mm_mul_ps(_mm_sub_ps(t.m_max.m_quad, t.m_min.m_quad), hkQuadRealHalf);
}

// The box is never initialised, because hkAabb's constructor is empty; recalcAabbExtents(aabb) fills it.
void hkpListShape::recalcAabbExtents() {
    hkAabb aabb;
    recalcAabbExtents(aabb);
}

const hkpShape* hkpListShape::getChildShapeInl(int32_t i) const {
    return m_childInfo[i].m_shape;
}

// Makes the list's children the numShapes shapes of shapeArray: the array is resized (reserve, then the size), each
// non-null shape is stored with its filter info (0 without filterInfo) and a zero shape info and size, the shapes get
// a reference each under REFERENCE_POLICY_INCREMENT, and the extents are recomputed. Null shapes leave their entry
// as it was. The asm stores numShapes, not the child's own count, as each child's m_numChildShapes.
void hkpListShape::setShapes(
    const hkpShape* const* shapeArray,
    int32_t numShapes,
    const uint32_t* filterInfo,
    hkpShapeContainer::ReferencePolicy ref
) {
    m_childInfo.setSize(numShapes);
    for (int32_t i = 0; i < numShapes; ++i) {
        if (shapeArray[i]) {
            m_childInfo.m_data[i].m_shape = shapeArray[i];
            m_childInfo.m_data[i].m_collisionFilterInfo = filterInfo ? filterInfo[i] : 0;
            m_childInfo.m_data[i].m_numChildShapes = numShapes;
            m_childInfo.m_data[i].m_shapeSize = 0;
        }
    }
    if (ref == hkpShapeContainer::REFERENCE_POLICY_INCREMENT) {
        hkReferencedObject::addReferences(&m_childInfo.m_data[0].m_shape, m_childInfo.m_size, sizeof(ChildInfo));
    }
    recalcAabbExtents();
}

void hkpListShape::disableChild(uint32_t index) {
    uint32_t v2 = m_enabledChildren[index >> 5];
    int32_t v3 = v2 & ~(1 << (index & 31));
    if (v2 != v3) {
        m_enabledChildren[index >> 5] = v3;
        ++m_numDisabledChildren;
    }
}

void hkpListShape::enableChild(uint32_t index) {
    uint32_t v2 = m_enabledChildren[index >> 5];
    int32_t v3 = v2 | 1 << (index & 31);
    if (v2 != v3) {
        m_enabledChildren[index >> 5] = v3;
        --m_numDisabledChildren;
    }
}

void hkpListShape::setEnabledChildren(const hkBitField& enabledChildren) {
    int32_t v2 = 0;
    if (enabledChildren.m_words.m_size > 0) {
        uint32_t* m_enabledChildren = this->m_enabledChildren;
        do {
            *m_enabledChildren++ = enabledChildren.m_words.m_data[v2++];
        }
        while (v2 < enabledChildren.m_words.m_size);
    }
    int32_t m_size = m_childInfo.m_size;
    m_numDisabledChildren = m_size -
        hkBitField::countBits(enabledChildren.m_words.m_data, enabledChildren.m_words.m_size);
}

const hkClass& hkpListShape::staticClass() {
    return hkpListShapeClass;
}
