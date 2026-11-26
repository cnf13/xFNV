#include "Havok/Source/Physics/Collide/Shape/Misc/Bv/hkpBvShape.h"

#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"

static void finishLoadedObjecthkpSingleShapeContainer(void* p, int32_t finishing);
// Havok SDK signature (hkpCollide.lib, tied by reloc,target).
static const void* getVtablehkpSingleShapeContainer();
static void cleanupLoadedObjecthkpSingleShapeContainer(void* p);
static void finishLoadedObjecthkpBvShape(void* p, int32_t finishing);
// Havok SDK signature (hkpCollide.lib, tied by reloc,target).
static const void* getVtablehkpBvShape();
static void cleanupLoadedObjecthkpBvShape(void* p);

hkpSingleShapeContainer::MembersArray hkpSingleShapeContainer::Members{
    {"childShape",
     &hkpShapeClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x4,
     nullptr}
};
hkClass hkpSingleShapeContainerClass(
    "hkpSingleShapeContainer",
    &hkpShapeContainerClass,
    8,
    nullptr,
    0,
    nullptr,
    0,
    hkpSingleShapeContainer::Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpSingleShapeContainerTypeInfo = {
    "hkpSingleShapeContainer",
    finishLoadedObjecthkpSingleShapeContainer,
    cleanupLoadedObjecthkpSingleShapeContainer,
    getVtablehkpSingleShapeContainer(),
    &typeid(hkpSingleShapeContainer),
    sizeof(hkpSingleShapeContainer)
};

// The reflected members of hkpBvShape.
using hkpBvShapeClass_MembersArray = const hkClassMember[2];
static hkpBvShapeClass_MembersArray hkpBvShapeClass_Members{
    {"boundingVolumeShape",
     &hkpShapeClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x10,
     nullptr},
    {"childShape",
     &hkpSingleShapeContainerClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x14,
     nullptr}
};
hkClass hkpBvShapeClass(
    "hkpBvShape",
    &hkpShapeClass,
    28,
    nullptr,
    0,
    nullptr,
    0,
    hkpBvShapeClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpBvShapeTypeInfo = {
    "hkpBvShape",
    finishLoadedObjecthkpBvShape,
    cleanupLoadedObjecthkpBvShape,
    getVtablehkpBvShape(),
    &typeid(hkpBvShape),
    sizeof(hkpBvShape)
};

// The type info's finish-loaded-object callback for hkpSingleShapeContainer: its finish constructor.
static void finishLoadedObjecthkpSingleShapeContainer(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpSingleShapeContainer(kFlag);
    }
}

static const void* getVtablehkpSingleShapeContainer() {
    return hkGetVtableOf<hkpSingleShapeContainer>();
}

static void cleanupLoadedObjecthkpSingleShapeContainer(void* p) {
    static_cast<hkpSingleShapeContainer*>(p)->~hkpSingleShapeContainer();
}

static const void* getVtablehkpBvShape() {
    return hkGetVtableOf<hkpBvShape>();
}

static void cleanupLoadedObjecthkpBvShape(void* p) {
    static_cast<hkpBvShape*>(p)->~hkpBvShape();
}

// Runs the finish constructor hkpBvShape(flag) in place.
static void finishLoadedObjecthkpBvShape(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpBvShape(kFlag);
    }
}

hkpSingleShapeContainer::hkpSingleShapeContainer(const hkpShape* s, ReferencePolicy ref) : m_childShape(s) {
    if (ref == REFERENCE_POLICY_INCREMENT) {
        m_childShape->addReference();
    }
}

hkpSingleShapeContainer::hkpSingleShapeContainer(hkFinishLoadedObjectFlag) {}

hkpSingleShapeContainer::~hkpSingleShapeContainer() {
    if (m_childShape) {
        m_childShape->removeReference();
    }
}

// The container holds exactly one child.
int32_t hkpSingleShapeContainer::getNumChildShapes() const {
    return 1;
}

// The one child's key is 0.
uint32_t hkpSingleShapeContainer::getFirstKey() const {
    return 0;
}

uint32_t hkpSingleShapeContainer::getNextKey(uint32_t oldKey) const {
    return HK_INVALID_SHAPE_KEY;
}

const hkpShape* hkpSingleShapeContainer::getChildShape(uint32_t key, char (&buffer)[512]) const {
    return m_childShape;
}

const hkClass& hkpSingleShapeContainer::staticClass() {
    return hkpSingleShapeContainerClass;
}

hkpBvShape::hkpBvShape(const hkpShape* boundingVolumeShape, const hkpShape* childShape)
  : hkpShape(HK_SHAPE_BV), m_boundingVolumeShape(boundingVolumeShape), m_childShape(childShape) {
    m_boundingVolumeShape->addReference();
}

hkpBvShape::~hkpBvShape() {
    m_boundingVolumeShape->removeReference();
}

void hkpBvShape::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    collector->addReferencedObject("Child", m_childShape.m_childShape, 0);
    hkReferencedObject::calcContentStatistics(collector, &hkpShapeClass);
}

const hkpShapeContainer* hkpBvShape::getContainer() const {
    return &m_childShape;
}

void hkpBvShape::getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
    m_boundingVolumeShape->getAabbImpl(localToWorld, tolerance, out);
}

// The SDK's form: the ray goes to the child one shape-key level down, and a hit gets this level's key 0; the child's
// hkBool is written straight into this function's result.
hkBool hkpBvShape::castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const {
    HK_TIMER_BEGIN("rcBvShape", nullptr);
    ++results.m_shapeKeyIndex;
    hkBool result = m_childShape.m_childShape->castRayImpl(input, results);
    --results.m_shapeKeyIndex;
    if (result.m_bool) {
        results.m_shapeKeys[results.m_shapeKeyIndex] = 0;
    }
    HK_TIMER_END();
    return result;
}

// The SDK's form: the child body takes the parent's motion and the child shape with key 0.
void hkpBvShape::castRayWithCollectorImpl(
    const hkpShapeRayCastInput& input,
    const hkpCdBody& cdBody,
    hkpRayHitCollector& collector
) const {
    HK_TIMER_BEGIN("rcBvShape", nullptr);
    hkpCdBody childBody;
    const hkpShape* pChild = m_childShape.m_childShape;
    childBody.m_parent = &cdBody;
    childBody.m_motion.pState = cdBody.m_motion.pState;
    childBody.m_shape = pChild;
    childBody.m_shapeKey = 0;
    pChild->castRayWithCollectorImpl(input, childBody, collector);
    HK_TIMER_END();
}

const hkClass& hkpBvShape::staticClass() {
    return hkpBvShapeClass;
}

hkpShapeContainer::~hkpShapeContainer() {}

void hkpShapeContainer::operator delete(void* apMemory) {
    ourFree(apMemory);
}
