#include "Havok/Source/Physics/Dynamics/Phantom/hkpSimpleShapePhantom.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"
#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpLinearCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpAabbPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpShapePhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

#include <xmmintrin.h>

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpSimpleShapePhantomCollisionDetailTypeInfo = {
    "hkpSimpleShapePhantomCollisionDetail",
    nullptr,
    cleanupLoadedObjecthkpSimpleShapePhantomCollisionDetail,
    nullptr,
    &typeid(hkpSimpleShapePhantom::CollisionDetail),
    sizeof(hkpSimpleShapePhantom::CollisionDetail)
};

// The reflected members of hkpSimpleShapePhantom::CollisionDetail.
using hkpSimpleShapePhantomCollisionDetailClass_MembersArray = const hkClassMember[1];
static hkpSimpleShapePhantomCollisionDetailClass_MembersArray hkpSimpleShapePhantomCollisionDetailClass_Members{
    {"collidable",
     &hkpCollidableClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x0,
     nullptr}
};
hkpSimpleShapePhantom::MembersArray hkpSimpleShapePhantom::Members{
    {"collisionDetails",
     &hkpSimpleShapePhantomCollisionDetailClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x160,
     nullptr},
    {"orderDirty",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_BOOL},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x16C,
     nullptr}
};

hkClass hkpSimpleShapePhantomCollisionDetailClass(
    "hkpSimpleShapePhantomCollisionDetail",
    nullptr,
    4,
    nullptr,
    0,
    nullptr,
    0,
    hkpSimpleShapePhantomCollisionDetailClass_Members,
    1,
    nullptr,
    nullptr,
    1,
    0
);
hkClass hkpSimpleShapePhantomClass(
    "hkpSimpleShapePhantom",
    &hkpShapePhantomClass,
    368,
    nullptr,
    0,
    nullptr,
    0,
    hkpSimpleShapePhantom::Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpSimpleShapePhantomTypeInfo = {
    "hkpSimpleShapePhantom",
    finishLoadedObjecthkpSimpleShapePhantom,
    cleanupLoadedObjecthkpSimpleShapePhantom,
    getVtablehkpSimpleShapePhantom(),
    &typeid(hkpSimpleShapePhantom),
    sizeof(hkpSimpleShapePhantom)
};

const hkClass& hkpSimpleShapePhantom::CollisionDetail::staticClass() {
    return hkpSimpleShapePhantomCollisionDetailClass;
}

hkpSimpleShapePhantom::hkpSimpleShapePhantom(
    const hkpShape* shape,
    const hkTransform& transform,
    uint32_t collisionFilterInfo
)
  : hkpShapePhantom(const_cast<hkpShape*>(shape), const_cast<hkTransform*>(&transform)) {
    m_orderDirty.m_bool = 0;
    m_collidable.m_broadPhaseHandle.m_collisionFilterInfo = collisionFilterInfo;
}

hkpSimpleShapePhantom::~hkpSimpleShapePhantom() {}

void hkpSimpleShapePhantom::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    int32_t v5;
    hkpPhantom::calcContentStatistics(collector, &hkpPhantomClass);
    int32_t result = m_collisionDetails.m_capacityAndFlags;
    if ((result >= 0) || collector->m_includeUnmanagedArrayMemory) {
        v5 = hkStatisticsCollector::getAllocatedSize(
            m_collisionDetails.m_data,
            static_cast<int32_t>(sizeof(*m_collisionDetails.m_data)) * result
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY,
            "OverlapPtr",
            m_collisionDetails.m_data,
            (static_cast<int32_t>(sizeof(*m_collisionDetails.m_data)) * m_collisionDetails.m_size),
            v5
        );
        return;
    }
}

hkpPhantomType hkpSimpleShapePhantom::getType() const {
    return HK_PHANTOM_SIMPLE_SHAPE;
}

void hkpSimpleShapePhantom::addOverlappingCollidable(hkpCollidable* collidable) {
    if (collidable->m_shape && (fireCollidableAdded(collidable) == HK_COLLIDABLE_ACCEPT)) {
        hkpSimpleShapePhantom::CollisionDetail* v5 = &m_collisionDetails.expandOne();
        v5->m_collidable = collidable;
        m_orderDirty.m_bool = 1;
    }
}

hkBool hkpSimpleShapePhantom::isOverlappingCollidableAdded(const hkpCollidable* apCollidable) {
    int32_t m_size = m_collisionDetails.m_size;
    int32_t v4 = 0;
    if (m_size <= 0) {
        return 0;
    }

    hkpSimpleShapePhantom::CollisionDetail*
        m_data = static_cast<hkArrayBase<hkpSimpleShapePhantom::CollisionDetail>&>(m_collisionDetails).begin();
    while (m_data->m_collidable != apCollidable) {
        ++v4;
        ++m_data;
        if (v4 >= m_size) {
            return 0;
        }
    }
    return 1;
}

void hkpSimpleShapePhantom::removeOverlappingCollidable(hkpCollidable* collidable) {
    if (collidable->m_shape) {
        int32_t v3 = m_collisionDetails.m_size - 1;
        if (v3 >= 0) {
            hkpSimpleShapePhantom::CollisionDetail* v4 = &m_collisionDetails.m_data[v3];
            while (true) {
                if (v4->m_collidable == collidable) {
                    fireCollidableRemoved(collidable, hkBool(true));
                    int32_t v5 = --m_collisionDetails.m_size;
                    if (v5 != v3) {
                        m_collisionDetails.m_data[v3].m_collidable = m_collisionDetails.m_data[v5].m_collidable;
                    }
                    m_orderDirty.m_bool = 1;
                    return;
                }
                --v3;
                --v4;
                if (v3 < 0) {
                    break;
                }
            }
        }
        fireCollidableRemoved(collidable, hkBool(false));
    }
}

// Sorts the collision details once a query has changed their order, then clears the dirty flag. Sorts only when more
// than one detail overlaps (a signed compare of the array's size), over indices 0..size-1, ordering the collidables by
// their broadphase id as unsigned.
void hkpSimpleShapePhantom::ensureDeterministicOrder() {
    if (!m_orderDirty.m_bool) {
        return;
    }
    const int32_t iSize = m_collisionDetails.m_size;
    if (iSize > 1) {
        hkAlgorithm::quickSort(m_collisionDetails.m_data, 0, iSize - 1, 0);
    }
    m_orderDirty.m_bool = 0;
}

// A new phantom of the same shape, transform and filter info, given copies of both listener arrays and the world
// object's properties.
hkpPhantom* hkpSimpleShapePhantom::clone() const {
    hkpSimpleShapePhantom* pClone = new hkpSimpleShapePhantom(
        m_collidable.m_shape,
        m_motionState.m_transform,
        m_collidable.m_broadPhaseHandle.m_collisionFilterInfo
    );
    pClone->m_overlapListeners = m_overlapListeners;
    pClone->m_phantomListeners = m_phantomListeners;
    pClone->copyProperties(this);
    return pClone;
}

void hkpSimpleShapePhantom::deallocateInternalArrays() {
    hkMemoryAllocator* m_heap;
    int32_t m_capacityAndFlags;
    if (!m_collisionDetails.m_size) {
        m_heap = hkMemoryRouter::getInstance().m_heap;
        m_capacityAndFlags = m_collisionDetails.m_capacityAndFlags;
        m_collisionDetails.m_size = 0;
        if (m_capacityAndFlags >= 0) {
            m_heap->bufFree(
                m_collisionDetails.m_data,
                sizeof(hkpSimpleShapePhantom::CollisionDetail) * m_capacityAndFlags
            );
        }
        m_collisionDetails.m_data = nullptr;
        m_collisionDetails.m_capacityAndFlags = 0x80000000u;
    }
    hkpShapePhantom::deallocateInternalArrays();
}

// Moves the phantom to position, widens its AABB by the cast path and casts the shape along the path against every
// collidable it overlaps, each through the dispatcher's linear cast function for the pair of shape types.
void hkpSimpleShapePhantom::setPositionAndLinearCast(
    const hkVector4& position,
    const hkpLinearCastInput& input,
    hkpCdPointCollector& castCollector,
    hkpCdPointCollector* startCollector
) {
    m_motionState.m_transform.setTranslation(position);
    const hkpShape* shape = m_collidable.m_shape;
    // The half tolerance is rounded to a float before the start point tolerance is added; 0.5 is a double constant.
    float halfTolerance = m_world->m_collisionInput->m_tolerance * 0.5;
    // The box is not default-constructed: it is uninitialised stack memory until getAabbImpl fills it.
    alignas(16) hkAabb aabb;
    shape->getAabbImpl(m_motionState.m_transform, halfTolerance + input.m_startPointTolerance, aabb);
    hkVector4 path;
    path.setSub4(input.m_to, position);
    hkVector4 zero;
    zero.setZero4();
    hkVector4 pathMin;
    pathMin.setMin4(zero, path);
    hkVector4 pathMax;
    pathMax.setMax4(zero, path);
    aabb.m_min.add4(pathMin);
    aabb.m_max.add4(pathMax);
    updateBroadPhase(aabb);

    hkpWorld* world = m_world;
    const hkpProcessCollisionInput* collisionInput = world->m_collisionInput;
    hkpLinearCastCollisionInput linearCastInput;
    linearCastInput.m_weldClosestPoints.m_storage = 0;
    linearCastInput.set(*collisionInput);
    linearCastInput.setPathAndTolerance(path, input.m_startPointTolerance);
    linearCastInput.m_maxExtraPenetration = input.m_maxExtraPenetration;

    hkpCollisionDispatcher* dispatcher = world->m_collisionDispatcher;
    for (int32_t i = m_collisionDetails.m_size - 1; i >= 0; --i) {
        hkpCollidable* collidable = m_collisionDetails.m_data[i].m_collidable;
        hkpShapeType typeA = m_collidable.m_shape->getType();
        hkpShapeType typeB = collidable->m_shape->getType();
        dispatcher->m_agent2Func[dispatcher->m_agent2Types[typeA][typeB]]
            .m_linearCastFunc(m_collidable, *collidable, linearCastInput, castCollector, startCollector);
    }
}

// The rotation goes straight into the motion state; the translation goes through setPositionAndLinearCast, which is
// called directly.
void hkpSimpleShapePhantom::setTransformAndLinearCast(
    const hkTransform& transform,
    const hkpLinearCastInput& input,
    hkpCdPointCollector& castCollector,
    hkpCdPointCollector* startCollector
) {
    m_motionState.m_transform.m_rotation = transform.m_rotation;
    hkpSimpleShapePhantom::setPositionAndLinearCast(transform.m_translation, input, castCollector, startCollector);
}

void hkpSimpleShapePhantom::getClosestPoints(hkpCdPointCollector& collector, const hkpCollisionInput* input) {
    if (!input) {
        input = m_world->m_collisionInput;
    }
    int32_t v4 = m_collisionDetails.m_size - 1;
    for (hkpCollisionDispatcher* i = input->m_dispatcher; v4 >= 0; --v4) {
        i->m_agent2Func[i->m_agent2Types[m_collidable.m_shape->m_type]
                                        [m_collisionDetails.m_data[v4].m_collidable->m_shape->m_type]]
            .m_getClosestPointFunc(m_collidable, *m_collisionDetails.m_data[v4].m_collidable, *input, collector);
    }
}

void hkpSimpleShapePhantom::getPenetrations(hkpCdBodyPairCollector& collector, const hkpCollisionInput* input) {
    hkpSimpleShapePhantom* pThis = this;
    hkpSimpleShapePhantom* v7 = pThis;
    if (!input) {
        input = pThis->m_world->m_collisionInput;
    }
    int32_t v3 = pThis->m_collisionDetails.m_size - 1;
    hkpCollisionDispatcher* m_dispatcher = input->m_dispatcher;
    if (v3 >= 0) {
        hkpLinkedCollidable* p_m_collidable = &pThis->m_collidable;
        while (true) {
            hkpCollidable* m_collidable = pThis->m_collisionDetails.m_data[v3].m_collidable;
            m_dispatcher
                ->m_agent2Func[m_dispatcher
                                   ->m_agent2Types[p_m_collidable->m_shape->m_type][m_collidable->m_shape->m_type]]
                .m_getPenetrationsFunc(*p_m_collidable, *m_collidable, *input, collector);
            if (collector.m_earlyOut.m_bool) {
                break;
            }
            if (--v3 < 0) {
                break;
            }
            pThis = v7;
        }
    }
}

const hkClass& hkpSimpleShapePhantom::staticClass() {
    return hkpSimpleShapePhantomClass;
}

int32_t
hkAlgorithm::quickSort(hkpSimpleShapePhantom::CollisionDetail* apArr, int32_t aiLow, int32_t aiHigh, uint8_t aiLess) {
    hkpSimpleShapePhantom::OrderByUid cmpLess;
    int32_t d = aiLow;
    for (;;) {
        int32_t i = d;
        int32_t j = aiHigh;
        hkpSimpleShapePhantom::CollisionDetail pivot = apArr[(d + j) >> 1];
        do {
            while (cmpLess(apArr[i], pivot)) {
                i++;
            }
            while (cmpLess(pivot, apArr[j])) {
                j--;
            }
            if (j < i) {
                break;
            }
            if (j != i) {
                hkpSimpleShapePhantom::CollisionDetail tmp = apArr[j];
                apArr[j] = apArr[i];
                apArr[i] = tmp;
            }
            --j;
            ++i;
        }
        while (i <= j);
        if (d < j) {
            hkAlgorithm::quickSort(apArr, d, j, aiLess);
        }
        if (i >= aiHigh) {
            return aiHigh;
        }
        d = i;
    }
}
