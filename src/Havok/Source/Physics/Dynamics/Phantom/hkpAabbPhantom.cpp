#include "Havok/Source/Physics/Dynamics/Phantom/hkpAabbPhantom.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpLinearCastInput.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/Modifiers/hkpRemoveTerminalsMoppModifierClass.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomOverlapListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

#include <xmmintrin.h>

static void cleanupLoadedObjecthkpAabbPhantom(void* p);

// The reflected members of hkpAabbPhantom.
using hkpAabbPhantomClass_MembersArray = const hkClassMember[3];
static hkpAabbPhantomClass_MembersArray hkpAabbPhantomClass_Members{
    {"aabb", &hkAabbClass, nullptr, {hkClassMember::TYPE_STRUCT}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xB0, nullptr},
    {"overlappingCollidables",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0xD0,
     nullptr},
    {"orderDirty",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_BOOL},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0xDC,
     nullptr}
};
hkClass hkpAabbPhantomClass(
    "hkpAabbPhantom",
    &hkpPhantomClass,
    224,
    nullptr,
    0,
    nullptr,
    0,
    hkpAabbPhantomClass_Members,
    3,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpAabbPhantomTypeInfo = {
    "hkpAabbPhantom",          finishLoadedObjecthkpAabbPhantom, cleanupLoadedObjecthkpAabbPhantom,
    getVtablehkpAabbPhantom(), &typeid(hkpAabbPhantom),          sizeof(hkpAabbPhantom)
};

static void cleanupLoadedObjecthkpAabbPhantom(void* p) {
    static_cast<hkpAabbPhantom*>(p)->~hkpAabbPhantom();
}

hkpAabbPhantom::hkpAabbPhantom(const hkAabb& a1, uint32_t a2) : hkpPhantom(nullptr) {
    m_orderDirty.m_bool = 0;
    m_aabb = a1;
    m_collidable.m_broadPhaseHandle.m_collisionFilterInfo = a2;
}

hkpAabbPhantom::~hkpAabbPhantom() {}

void hkpAabbPhantom::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    int32_t m_capacityAndFlags = m_overlappingCollidables.m_capacityAndFlags;
    if (m_capacityAndFlags >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t AllocatedSize = hkStatisticsCollector::getAllocatedSize(
            m_overlappingCollidables.begin(),
            static_cast<int32_t>(sizeof(*m_overlappingCollidables.begin())) * m_capacityAndFlags
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY, "OvrlpCollPtr", m_overlappingCollidables.begin(),
            static_cast<int32_t>(sizeof(*m_overlappingCollidables.begin())) * m_overlappingCollidables.m_size,
            AllocatedSize
        );
    }
    hkpPhantom::calcContentStatistics(collector, &hkpPhantomClass);
}

hkMotionState* hkpAabbPhantom::getMotionState() {
    return nullptr;
}

hkpPhantomType hkpAabbPhantom::getType() const {
    return HK_PHANTOM_AABB;
}

void hkpAabbPhantom::calcAabb(hkAabb& aabb) {
    aabb = m_aabb;
}

void hkpAabbPhantom::addOverlappingCollidable(hkpCollidable* collidable) {
    if (fireCollidableAdded(collidable) == HK_COLLIDABLE_ACCEPT) {
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        if (m_overlappingCollidables.m_size ==
            (m_overlappingCollidables.m_capacityAndFlags & hkArrayBase<hkpCollidable*>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*Instance->m_heap, &m_overlappingCollidables, sizeof(hkpCollidable*));
        }
        m_overlappingCollidables.m_data[m_overlappingCollidables.m_size++] = collidable;
        m_orderDirty.m_bool = 1;
    }
}

hkBool hkpAabbPhantom::isOverlappingCollidableAdded(const hkpCollidable* apCollidable) {
    int32_t iSize = m_overlappingCollidables.m_size;
    int32_t v4 = 0;
    if (iSize <= 0) {
    LABEL_5:;
        return 0;
    }

    hkpCollidable** pCollidables = m_overlappingCollidables.m_data;
    while (*pCollidables != apCollidable) {
        ++v4;
        ++pCollidables;
        if (v4 >= iSize) {
            goto LABEL_5;
        }
    }
    return 1;
}

void hkpAabbPhantom::removeOverlappingCollidable(hkpCollidable* collidable) {
    int32_t v6;
    int32_t m_size = m_overlappingCollidables.m_size;
    int32_t v4 = 0;
    if (m_size <= 0) {
    LABEL_5:;
        v6 = -1;
    }
    else {
        hkpCollidable** m_data = m_overlappingCollidables.begin();
        while (*m_data != collidable) {
            ++v4;
            ++m_data;
            if (v4 >= m_size) {
                goto LABEL_5;
            }
        }
        v6 = v4;
    }
    fireCollidableRemoved(collidable, static_cast<hkBool>(v6 >= 0));
    if (v6 >= 0) {
        int32_t v7 = --m_overlappingCollidables.m_size;
        if (v7 != v6) {
            m_overlappingCollidables.m_data[v6] = m_overlappingCollidables.m_data[v7];
        }
        m_orderDirty.m_bool = 1;
    }
}

// Sorts the overlapping collidables once something changed their order and clears the flag. Only sorts when more than
// one collidable overlaps (a signed compare of the array's size).
void hkpAabbPhantom::ensureDeterministicOrder() {
    if (!m_orderDirty.m_bool) {
        return;
    }
    const int32_t iSize = m_overlappingCollidables.m_size;
    if (iSize > 1) {
        hkAlgorithm::quickSort(m_overlappingCollidables.m_data, 0, iSize - 1, 0);
    }
    m_orderDirty.m_bool = 0;
}

// The SDK's clone: a new phantom with this one's AABB and filter info, then copies of both listener arrays and the
// world object's properties.
hkpPhantom* hkpAabbPhantom::clone() const {
    hkMemoryRouter& router = hkMemoryRouter::getInstance();
    void* pMemory = router.m_heap->blockAlloc(static_cast<int32_t>(sizeof(hkpAabbPhantom)));
    static_cast<hkpAabbPhantom*>(pMemory)->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkpAabbPhantom));
    hkpAabbPhantom* pPhantom =
        ::new (pMemory) hkpAabbPhantom(m_aabb, m_collidable.m_broadPhaseHandle.m_collisionFilterInfo);
    pPhantom->m_overlapListeners = m_overlapListeners;
    pPhantom->m_phantomListeners = m_phantomListeners;
    pPhantom->copyProperties(this);
    return pPhantom;
}

void hkpAabbPhantom::deallocateInternalArrays() {
    hkMemoryAllocator* m_heap;
    int32_t v3;
    if (!m_overlappingCollidables.m_size) {
        m_heap = (&hkMemoryRouter::getInstance())->m_heap;
        v3 = m_overlappingCollidables.m_capacityAndFlags;
        m_overlappingCollidables.m_size = (uint32_t)0;
        if (v3 >= 0) {
            m_heap->bufFree(m_overlappingCollidables.m_data, sizeof(hkpCollidable*) * v3);
        }
        m_overlappingCollidables.m_data = nullptr;
        m_overlappingCollidables.m_capacityAndFlags = 0x80000000u;
    }
    hkpPhantom::deallocateInternalArrays();
}

void hkpAabbPhantom::setAabb(const hkAabb& a1) {
    m_aabb = a1;
    updateBroadPhase(m_aabb);
}

// castRay with a collector: the ray, taken into each overlapping collidable's space, is cast against the collidable's
// shape, which reports its hits to the collector; the world's filter is used as a ray shape collection filter when the
// input enables it.
void hkpAabbPhantom::castRay(const hkpWorldRayCastInput& input, hkpRayHitCollector& collector) const {
    HK_TIMER_BEGIN("rcPhantom", nullptr);
    hkpShapeRayCastInput sinput;
    sinput.m_filterInfo = input.m_filterInfo;
    sinput.m_rayShapeCollectionFilter = nullptr;
    hkpCollisionFilter* pFilter;
    if (input.m_enableShapeCollectionFilter.m_bool && (pFilter = m_world->m_collisionFilter) != nullptr) {
        sinput.m_rayShapeCollectionFilter = static_cast<const hkpRayShapeCollectionFilter*>(pFilter);
    }
    else {
        sinput.m_rayShapeCollectionFilter = nullptr;
    }
    hkpCollidable* const* ppCollidable = m_overlappingCollidables.m_data;
    for (int32_t i = m_overlappingCollidables.m_size - 1; i >= 0; --i, ++ppCollidable) {
        const hkpShape* pShape = (*ppCollidable)->m_shape;
        if (pShape) {
            const hkTransform& t = *(*ppCollidable)->m_motion.pMotion;
            sinput.m_from.setTransformedInversePos(t, input.m_from);
            sinput.m_to.setTransformedInversePos(t, input.m_to);
            pShape->castRayWithCollectorImpl(sinput, **ppCollidable, collector);
        }
    }
    HK_TIMER_END();
}

// Runs the world's collision input with the cast's path, tolerance and penetration against every overlapping collidable
// that has a shape, passes the world's filter and is not the cast one. The filter is called before the shape is tested.
// The input's padding is never written; the agents' linear-cast functions get it by reference.
void hkpAabbPhantom::linearCast(
    const hkpCollidable* const toBeCast,
    const hkpLinearCastInput& input,
    hkpCdPointCollector& castCollector,
    hkpCdPointCollector* startCollector
) const {
    HK_TIMER_BEGIN("hkpAabbPhantom::linearCast", nullptr);
    const hkpProcessCollisionInput* collisionInput = m_world->m_collisionInput;
    // EXE-INLINED none: hkpLinearCastCollisionInput::hkpLinearCastCollisionInput
    hkpLinearCastCollisionInput lcInput;
    // FLT_EPSILON.
    lcInput.m_maxExtraPenetration = 1.1920929e-07f;
    lcInput.m_weldClosestPoints.m_storage = 0;
    // EXE-INLINED none: hkpLinearCastCollisionInput::set
    lcInput.m_dispatcher = collisionInput->m_dispatcher;
    lcInput.m_weldClosestPoints = collisionInput->m_weldClosestPoints;
    lcInput.m_tolerance = collisionInput->m_tolerance;
    lcInput.m_filter = collisionInput->m_filter;
    lcInput.m_convexListFilter = collisionInput->m_convexListFilter;
    lcInput.m_createPredictiveAgents = collisionInput->m_createPredictiveAgents;
    lcInput.m_aabb32Info.m_bitOffsetLow.m_quad = collisionInput->m_aabb32Info.m_bitOffsetLow.m_quad;
    lcInput.m_aabb32Info.m_bitOffsetHigh.m_quad = collisionInput->m_aabb32Info.m_bitOffsetHigh.m_quad;
    lcInput.m_aabb32Info.m_bitScale.m_quad = collisionInput->m_aabb32Info.m_bitScale.m_quad;
    lcInput.m_config = collisionInput->m_config;
    // EXE-INLINED none: hkpLinearCastCollisionInput::setPathAndTolerance
    __m128 path = _mm_sub_ps(input.m_to.m_quad, toBeCast->m_motion.pState->m_transform.m_translation.m_quad);
    lcInput.m_path.m_quad = path;
    lcInput.m_cachedPathLength = lcInput.m_path.length3();
    lcInput.m_tolerance = input.m_startPointTolerance;
    lcInput.m_maxExtraPenetration = input.m_maxExtraPenetration;

    hkpCollisionDispatcher* dispatcher = m_world->m_collisionDispatcher;
    const hkpCollisionFilter* filter = m_world->m_collisionFilter;
    const int32_t numCollidables = static_cast<int32_t>(m_overlappingCollidables.m_size);
    for (int32_t i = 0; i < numCollidables; ++i) {
        hkpCollidable* collidable = m_overlappingCollidables.m_data[i];
        hkBool enabled = filter->isCollisionEnabled(*toBeCast, *collidable);
        if (collidable->m_shape && enabled.m_bool && collidable != toBeCast) {
            // EXE-INLINED 0x43B230
            hkpShapeType typeA = toBeCast->m_shape->m_type;
            // EXE-INLINED 0x43B230
            hkpShapeType typeB = collidable->m_shape->m_type;
            dispatcher->m_agent2Func[dispatcher->m_agent2Types[typeA][typeB]].m_linearCastFunc(
                *toBeCast, *collidable, lcInput, castCollector, startCollector
            );
        }
    }
    HK_TIMER_END();
}

// castRay with an output: the ray, taken into each overlapping collidable's space as above, is cast against each shape
// into the caller's output, which keeps the closest hit; a shape that reports a hit makes its collidable the output's
// root collidable. The hit normal is then turned into world space by that collidable's rotation, through setRotatedDir.
void hkpAabbPhantom::castRay(const hkpWorldRayCastInput& input, hkpWorldRayCastOutput& output) const {
    HK_TIMER_BEGIN("rcPhantom", nullptr);
    hkpShapeRayCastInput sinput;
    sinput.m_filterInfo = input.m_filterInfo;
    sinput.m_rayShapeCollectionFilter = nullptr;
    hkpCollisionFilter* pFilter;
    if (input.m_enableShapeCollectionFilter.m_bool && (pFilter = m_world->m_collisionFilter) != nullptr) {
        sinput.m_rayShapeCollectionFilter = static_cast<const hkpRayShapeCollectionFilter*>(pFilter);
    }
    else {
        sinput.m_rayShapeCollectionFilter = nullptr;
    }
    hkpCollidable* const* ppCollidable = m_overlappingCollidables.m_data;
    for (int32_t i = m_overlappingCollidables.m_size - 1; i >= 0; --i, ++ppCollidable) {
        const hkpShape* pShape = (*ppCollidable)->m_shape;
        if (pShape) {
            const hkTransform& t = *(*ppCollidable)->m_motion.pMotion;
            sinput.m_from.setTransformedInversePos(t, input.m_from);
            sinput.m_to.setTransformedInversePos(t, input.m_to);
            if (pShape->castRay(sinput, output).m_bool) {
                output.m_rootCollidable = *ppCollidable;
            }
        }
    }
    if (output.m_rootCollidable) {
        output.m_normal.setRotatedDir(output.m_rootCollidable->m_motion.pMotion->m_rotation, output.m_normal);
    }
    HK_TIMER_END();
}

const hkClass& hkpAabbPhantom::staticClass() {
    return hkpAabbPhantomClass;
}
