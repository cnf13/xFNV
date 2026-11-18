#include "Havok/Source/Physics/Vehicle/WheelCollide/LinearCast/hkpVehicleLinearCastWheelCollide.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabbUtil.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpLinearCastInput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpClosestCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpPairLinearCastJob.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Cylinder/hkpCylinderShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpUnaryAction.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpAabbPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomOverlapListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Vehicle/Suspension/hkpVehicleSuspension.h"
#include "Havok/Source/Physics/Vehicle/WheelCollide/RejectChassisListener/hkpRejectChassisListener.h"
#include "Havok/Source/Physics/Vehicle/WheelCollide/hkpVehicleWheelCollide.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleData.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleInstance.h"

#include <bit>
#include <new>
#include <xmmintrin.h>

static void cleanupLoadedObjecthkpVehicleLinearCastWheelCollideWheelState(void* p);
static void finishLoadedObjecthkpVehicleLinearCastWheelCollide(void* p, int32_t finishing);
// Havok SDK signature (hkpVehicle.lib, tied by reloc,target).
static const void* getVtablehkpVehicleLinearCastWheelCollide();

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpVehicleLinearCastWheelCollideWheelStateTypeInfo = {
    "hkpVehicleLinearCastWheelCollideWheelState",
    nullptr,
    cleanupLoadedObjecthkpVehicleLinearCastWheelCollideWheelState,
    nullptr,
    &typeid(hkpVehicleLinearCastWheelCollide::WheelState),
    sizeof(hkpVehicleLinearCastWheelCollide::WheelState)
};

// The reflected members of hkpVehicleLinearCastWheelCollide.
using hkpVehicleLinearCastWheelCollideClass_MembersArray = const hkClassMember[5];
static hkpVehicleLinearCastWheelCollideClass_MembersArray hkpVehicleLinearCastWheelCollideClass_Members{
    {"wheelCollisionFilterInfo",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xC,
     nullptr},
    {"wheelStates",
     &hkpVehicleLinearCastWheelCollideWheelStateClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x10,
     nullptr},
    {"rejectChassisListener",
     &hkpRejectChassisListenerClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x1C,
     nullptr},
    {"maxExtraPenetration",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x2C,
     nullptr},
    {"startPointTolerance",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x30,
     nullptr}
};

static void cleanupLoadedObjecthkpVehicleLinearCastWheelCollideWheelState(void* p) {
    ;
}

static const void* getVtablehkpVehicleLinearCastWheelCollide() {
    return hkGetVtableOf<hkpVehicleLinearCastWheelCollide>();
}

// The type info's finish-loaded-object callback for hkpVehicleLinearCastWheelCollide: the null check, then its finish
// constructor.
static void finishLoadedObjecthkpVehicleLinearCastWheelCollide(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpVehicleLinearCastWheelCollide(kFlag);
    }
}

// The reflected members of hkpVehicleLinearCastWheelCollide::WheelState.
using hkpVehicleLinearCastWheelCollideWheelStateClass_MembersArray = const hkClassMember[4];
static hkpVehicleLinearCastWheelCollideWheelStateClass_MembersArray
    hkpVehicleLinearCastWheelCollideWheelStateClass_Members{
        {"phantom",
         &hkpAabbPhantomClass,
         nullptr,
         {hkClassMember::TYPE_POINTER},
         {hkClassMember::TYPE_STRUCT},
         0,
         {0},
         0x0,
         nullptr},
        {"shape",
         &hkpShapeClass,
         nullptr,
         {hkClassMember::TYPE_POINTER},
         {hkClassMember::TYPE_STRUCT},
         0,
         {0},
         0x4,
         nullptr},
        {"transform",
         nullptr,
         nullptr,
         {hkClassMember::TYPE_TRANSFORM},
         {hkClassMember::TYPE_VOID},
         0,
         {0},
         0x10,
         nullptr},
        {"to", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x50, nullptr}
};

hkClass hkpVehicleLinearCastWheelCollideWheelStateClass(
    "hkpVehicleLinearCastWheelCollideWheelState",
    nullptr,
    96,
    nullptr,
    0,
    nullptr,
    0,
    hkpVehicleLinearCastWheelCollideWheelStateClass_Members,
    4,
    nullptr,
    nullptr,
    0,
    0
);

hkClass hkpVehicleLinearCastWheelCollideClass(
    "hkpVehicleLinearCastWheelCollide",
    &hkpVehicleWheelCollideClass,
    52,
    nullptr,
    0,
    nullptr,
    0,
    hkpVehicleLinearCastWheelCollideClass_Members,
    5,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpVehicleLinearCastWheelCollideTypeInfo = {
    "hkpVehicleLinearCastWheelCollide",
    finishLoadedObjecthkpVehicleLinearCastWheelCollide,
    cleanupLoadedObjecthkpVehicleLinearCastWheelCollide,
    getVtablehkpVehicleLinearCastWheelCollide(),
    &typeid(hkpVehicleLinearCastWheelCollide),
    sizeof(hkpVehicleLinearCastWheelCollide)
};

void cleanupLoadedObjecthkpVehicleLinearCastWheelCollide(void* p) {
    static_cast<hkpVehicleLinearCastWheelCollide*>(p)->~hkpVehicleLinearCastWheelCollide();
}

const hkClass& hkpVehicleLinearCastWheelCollide::WheelState::staticClass() {
    return hkpVehicleLinearCastWheelCollideWheelStateClass;
}

hkpVehicleLinearCastWheelCollide::hkpVehicleLinearCastWheelCollide()
  : m_maxExtraPenetration(1.1920929e-07f), m_startPointTolerance(1.1920929e-07f) {
    m_type.m_storage = LINEAR_CAST_WHEEL_COLLIDE;
}

// Per wheel: drops the shape, takes the chassis-rejecting listener off the phantom when both this and the phantom
// carry a size (loaded objects do not), and drops the phantom. The listener and the array are destroyed by their own
// destructors.
hkpVehicleLinearCastWheelCollide::~hkpVehicleLinearCastWheelCollide() {
    for (int32_t i = 0; i < m_wheelStates.m_size; ++i) {
        WheelState& state = m_wheelStates.m_data[i];
        state.m_shape->removeReference();
        hkpPhantom* pPhantom = state.m_phantom;
        if (m_memSizeAndFlags && pPhantom->m_memSizeAndFlags) {
            pPhantom->removePhantomOverlapListener(&m_rejectChassisListener);
        }
        pPhantom->removeReference();
    }
}

// One cylinder shape per wheel, created only while the wheel states are still empty, then per wheel its state, its AABB
// and an AABB phantom carrying the chassis-rejecting listener. The wheel count is read unsigned.
void hkpVehicleLinearCastWheelCollide::init(const hkpVehicleInstance* vehicle) {
    const int32_t numWheels = static_cast<uint8_t>(vehicle->m_data->m_numWheels);
    if (m_wheelStates.m_size == 0) {
        m_wheelStates.reserve(numWheels);
        // EXE-INLINED none: hkArray<WheelState>::setSize (the elements' construction is empty)
        m_wheelStates.m_size = numWheels;
        for (uint8_t i = 0; i < numWheels; ++i) {
            m_wheelStates.m_data[i].m_shape = createWheelShape(
                vehicle->m_data->m_wheelParams.m_data[i].m_width, vehicle->m_data->m_wheelParams.m_data[i].m_radius
            );
        }
    }
    m_rejectChassisListener.m_chassis = &vehicle->m_entity->m_collidable;
    for (uint8_t wheelNum = 0; wheelNum < numWheels; ++wheelNum) {
        hkAabb aabb;
        updateWheelState(vehicle, wheelNum);
        calcAabbOfWheel(vehicle, wheelNum, aabb);
        void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpAabbPhantom));
        static_cast<hkReferencedObject*>(pMemory)->m_memSizeAndFlags = sizeof(hkpAabbPhantom);
        m_wheelStates.m_data[wheelNum].m_phantom = ::new (pMemory) hkpAabbPhantom(aabb, m_wheelCollisionFilterInfo);
        m_wheelStates.m_data[wheelNum].m_phantom->addPhantomOverlapListener(&m_rejectChassisListener);
    }
}

void hkpVehicleLinearCastWheelCollide::collideWheels(
    float deltaTime,
    const hkpVehicleInstance* vehicle,
    hkpVehicleWheelCollide::CollisionDetectionWheelOutput* cdInfoOut
) {
    int32_t v8;
    uint8_t m_numWheels = vehicle->m_data->m_numWheels;
    v8 = static_cast<int32_t>(
        (static_cast<uint32_t>(v8) & 0xFFFFFF00u) | (static_cast<uint32_t>(static_cast<uint8_t>(0)))
    );
    if (m_numWheels) {
        do {
            hkpRootCdPoint v9;
            if (castSingleWheel(vehicle, v8, v9).m_bool) {
                getCollisionOutputFromCastResult(vehicle, v8, v9, *cdInfoOut);
            }
            else {
                getCollisionOutputWithoutHit(vehicle, v8, *cdInfoOut);
            }
            wheelCollideCallback(vehicle, v8, *cdInfoOut++);
            v8 = static_cast<int32_t>(
                (static_cast<uint32_t>(v8) & 0xFFFFFF00u) | (static_cast<uint32_t>(static_cast<uint8_t>((v8 + 1))))
            );
        }
        while (static_cast<uint8_t>(v8) < m_numWheels);
    }
}

void hkpVehicleLinearCastWheelCollide::getPhantoms(hkArray<hkpPhantom*, hkContainerHeapAllocator>& phantomsOut) {
    hkpVehicleLinearCastWheelCollide* pThis = this;
    hkpVehicleLinearCastWheelCollide* v6 = pThis;
    if (pThis->m_wheelStates.m_size > 0) {
        int32_t v2 = 0;
        int32_t m_size = pThis->m_wheelStates.m_size;
        while (true) {
            hkpAabbPhantom* m_phantom = pThis->m_wheelStates.m_data[v2].m_phantom;
            hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
            if (phantomsOut.m_size == (phantomsOut.m_capacityAndFlags & hkArrayBase<hkpPhantom*>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(*Instance->m_heap, &phantomsOut, sizeof(hkpPhantom*));
            }
            phantomsOut.m_data[phantomsOut.m_size++] = m_phantom;
            ++v2;
            if (!--m_size) {
                break;
            }
            pThis = v6;
        }
    }
}

// Per wheel: its cast state, its AABB, and its phantom moved to that AABB.
void hkpVehicleLinearCastWheelCollide::updateBeforeCollisionDetection(const hkpVehicleInstance* vehicle) {
    const int32_t numWheels = m_wheelStates.m_size;
    for (uint8_t wheelNum = 0; wheelNum < numWheels; ++wheelNum) {
        hkAabb aabb;
        updateWheelState(vehicle, wheelNum);
        calcAabbOfWheel(vehicle, wheelNum, aabb);
        m_wheelStates.m_data[wheelNum].m_phantom->setAabb(aabb);
    }
}

// A copy over newChassis and newPhantoms: the same wheel shapes (referenced again), the new phantoms (referenced), each
// moved from this collide's chassis listener to the copy's, and the same filter info and cast tolerances.
hkpVehicleWheelCollide* hkpVehicleLinearCastWheelCollide::clone(
    const hkpRigidBody& newChassis,
    const hkArray<hkpPhantom*, hkContainerHeapAllocator>& newPhantoms
) const {
    void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpVehicleLinearCastWheelCollide));
    static_cast<hkReferencedObject*>(pMemory)->m_memSizeAndFlags = sizeof(hkpVehicleLinearCastWheelCollide);
    hkpVehicleLinearCastWheelCollide* pClone = ::new (pMemory) hkpVehicleLinearCastWheelCollide;
    const int32_t numWheels = m_wheelStates.m_size;
    pClone->m_wheelStates.reserve(numWheels);
    // EXE-INLINED none: hkArray<WheelState>::setSize (the elements' construction is empty)
    pClone->m_wheelStates.m_size = numWheels;
    for (uint8_t i = 0; i < numWheels; ++i) {
        WheelState& wheelState = pClone->m_wheelStates.m_data[i];
        const hkpShape* shape = m_wheelStates.m_data[i].m_shape;
        wheelState.m_shape = shape;
        shape->addReference();
        hkpPhantom* phantom = newPhantoms.m_data[i];
        wheelState.m_phantom = static_cast<hkpAabbPhantom*>(phantom);
        phantom->addReference();
        // The SDK's clone is const and removes this collide's own listener: its const_cast.
        phantom->removePhantomOverlapListener(const_cast<hkpRejectChassisListener*>(&m_rejectChassisListener));
        phantom->addPhantomOverlapListener(&pClone->m_rejectChassisListener);
        pClone->m_rejectChassisListener.m_chassis = &newChassis.m_collidable;
    }
    pClone->m_wheelCollisionFilterInfo = m_wheelCollisionFilterInfo;
    float fMaxExtraPenetration = m_maxExtraPenetration;
    pClone->m_maxExtraPenetration = fMaxExtraPenetration;
    float fStartPointTolerance = m_startPointTolerance;
    pClone->m_startPointTolerance = fStartPointTolerance;
    return pClone;
}

void hkpVehicleLinearCastWheelCollide::addToWorld(hkpWorld* world) {
    int32_t m_size = m_wheelStates.m_size;
    if (m_size > 0) {
        int32_t v4 = 0;
        do {
            world->addPhantom(m_wheelStates.m_data[v4++].m_phantom);
            --m_size;
        }
        while (m_size);
    }
}

void hkpVehicleLinearCastWheelCollide::removeFromWorld() {
    int32_t m_size = m_wheelStates.m_size;
    if (m_size > 0) {
        int32_t v3 = 0;
        do {
            m_wheelStates.m_data[v3].m_phantom->m_world->removePhantom(m_wheelStates.m_data[v3].m_phantom);
            ++v3;
            --m_size;
        }
        while (m_size);
    }
}

void hkpVehicleLinearCastWheelCollide::setCollisionFilterInfo(uint32_t filterInfo) {
    int32_t m_size = m_wheelStates.m_size;
    m_wheelCollisionFilterInfo = filterInfo;
    if (m_size > 0) {
        int32_t v3 = 0;
        int32_t v4 = m_size;
        do {
            hkpAabbPhantom* m_phantom = m_wheelStates.m_data[v3].m_phantom;
            if (m_phantom) {
                m_phantom->m_collidable.m_broadPhaseHandle.m_collisionFilterInfo = filterInfo;
            }
            ++v3;
            --v4;
        }
        while (v4);
    }
}

void hkpVehicleLinearCastWheelCollide::wheelCollideCallback(
    const hkpVehicleInstance* vehicle,
    uint8_t wheelIndex,
    hkpVehicleWheelCollide::CollisionDetectionWheelOutput& cdInfo
) {
    centerWheelContactPoint(vehicle, wheelIndex, cdInfo);
}

// The sum of the wheels' command counts.
int32_t hkpVehicleLinearCastWheelCollide::getTotalNumCommands() const {
    const int32_t numWheels = m_wheelStates.m_size;
    int32_t totalNumCommands = 0;
    for (uint8_t i = 0; i < numWheels; ++i) {
        totalNumCommands += getNumCommands(i);
    }
    return totalNumCommands;
}

int32_t hkpVehicleLinearCastWheelCollide::getNumCommands(uint8_t numWheel) const {
    hkpAabbPhantom* m_phantom;
    m_phantom = m_wheelStates.m_data[numWheel].m_phantom;
    m_phantom->ensureDeterministicOrder();
    return m_phantom->m_overlappingCollidables.m_size;
}

// One collidable per wheel, built in collidableStorage over the wheel's shape and transform (when there is storage),
// and one command per collidable overlapping the wheel's phantom, each with one result slot of outputStorage. Returns
// the number of commands. The wheel and overlap counters are signed bytes, the counts read once.
int32_t hkpVehicleLinearCastWheelCollide::buildLinearCastCommands(
    const hkpVehicleInstance* vehicle,
    const hkpCollisionFilter* collisionFilter,
    hkpCollidable* collidableStorage,
    hkpPairLinearCastCommand* commandStorage,
    hkpRootCdPoint* outputStorage
) const {
    int32_t numCommands = 0;
    const int32_t numWheels = m_wheelStates.m_size;
    for (int8_t i = 0; i < numWheels; ++i) {
        const WheelState& wheelState = m_wheelStates.m_data[i];
        // The SDK's placement new, with its null test.
        if (collidableStorage) {
            // EXE-INLINED 0xCD2D80
            hkpCollidable* pCollidable = collidableStorage;
            pCollidable->m_shape = wheelState.m_shape;
            pCollidable->m_parent = nullptr;
            pCollidable->m_ownerOffset = 0;
            pCollidable->m_shapeKey = 0xFFFFFFFF;
            // The transform is kept in the body's motion pointer; the motion union is not const.
            pCollidable->m_motion.pMotion = const_cast<hkTransform*>(&wheelState.m_transform);
            pCollidable->m_broadPhaseHandle.m_id = 0;
            pCollidable->m_broadPhaseHandle.m_type = 0;
            pCollidable->m_broadPhaseHandle.m_collisionFilterInfo = 0;
            pCollidable->m_broadPhaseHandle.m_objectQualityType = -1;
            pCollidable->m_broadPhaseHandle.m_ownerOffset =
                static_cast<int8_t>(hkpTypedBroadPhaseHandle::OFFSET_INVALID);
            new (&pCollidable->m_boundingVolumeData) hkpCollidable::BoundingVolumeData();
            pCollidable->m_allowedPenetrationDepth = -1.0f;
            // The handle sits inside the collidable, so its owner offset is the distance back to the collidable.
            pCollidable->m_broadPhaseHandle.m_ownerOffset =
                static_cast<int8_t>(-static_cast<int32_t>(offsetof(hkpCollidable, m_broadPhaseHandle)));
            pCollidable->m_forceCollideOntoPpu = 0;
            pCollidable->m_shapeSizeOnSpu = 0;
        }
        wheelState.m_phantom->ensureDeterministicOrder();
        const hkArray<hkpCollidable*>& overlaps = wheelState.m_phantom->m_overlappingCollidables;
        const int32_t numOverlaps = overlaps.m_size;
        for (int8_t j = 0; j < numOverlaps; ++j) {
            ++numCommands;
            commandStorage->m_collidableA = collidableStorage;
            commandStorage->m_collidableB = overlaps.m_data[j];
            commandStorage->m_from = wheelState.m_transform.m_translation;
            commandStorage->m_results = outputStorage;
            ++outputStorage;
            commandStorage->m_to = wheelState.m_to;
            commandStorage->m_resultsCapacity = 1;
            commandStorage->m_numResultsOut = 0;
            commandStorage->m_startPointResultsCapacity = 0;
            ++commandStorage;
        }
        ++collidableStorage;
    }
    return numCommands;
}

// The nearest hit of the wheel's commands: the first result of each command that has one, the smallest distance (the
// separating normal's w) winning; a later hit replaces the nearest only when strictly nearer, so a NaN never replaces
// it.
const hkpRootCdPoint* hkpVehicleLinearCastWheelCollide::determineNearestHit(
    uint8_t numWheel,
    const hkpPairLinearCastCommand* commandStorageOutput
) const {
    const int32_t numCommands = getNumCommands(numWheel);
    const hkpRootCdPoint* nearest = nullptr;
    for (int32_t i = 0; i < numCommands; ++i) {
        const hkpPairLinearCastCommand& command = commandStorageOutput[i];
        if (command.m_numResultsOut != 0) {
            const hkpRootCdPoint* hit = command.m_results;
            if (!nearest ||
                nearest->m_contact.m_separatingNormal.m_quad[3] > hit->m_contact.m_separatingNormal.m_quad[3]) {
                nearest = hit;
            }
        }
    }
    return nearest;
}

hkBool hkpVehicleLinearCastWheelCollide::castSingleWheel(
    const hkpVehicleInstance* vehicle,
    uint8_t wheelNum,
    hkpRootCdPoint& linearCastOutput
) const {
    hkVector4 v6;
    hkpLinearCastInput v8;
    XFNV_INLINED_LOCAL(hkpClosestCdPointCollector, a3, hkpClosestCdPointCollector_vftable, std::nullopt);
    hkpVehicleLinearCastWheelCollide::WheelState* v5 = &m_wheelStates.m_data[wheelNum];
    // The collidable over the wheel's transform.
    alignas(16) hkpCollidable v10(v5->m_shape, &v5->m_transform, 0);
    v10.m_broadPhaseHandle.m_collisionFilterInfo = m_wheelCollisionFilterInfo;
    v8.m_maxExtraPenetration = 1.1920929e-07f;
    v8.m_startPointTolerance = 1.1920929e-07f;
    v6.m_quad = v5->m_to.m_quad;
    v8.m_maxExtraPenetration = m_maxExtraPenetration;
    v8.m_startPointTolerance = m_startPointTolerance;
    // MAYBE-INLINED 0xE36A46: before the vptr store of hkpClosestCdPointCollector's inlined constructor
    a3.m_hitPoint.m_contact.m_separatingNormal.m_quad[3] = 3.40282e+38f;
    v8.m_to.m_quad = v6.m_quad;
    // MAYBE-INLINED 0xE36A4F: before the vptr store of hkpClosestCdPointCollector's inlined constructor
    a3.m_earlyOutDistance = 3.40282e+38f;
    // MAYBE-INLINED 0xE36A5B: after the vptr store of hkpClosestCdPointCollector's inlined constructor
    a3.m_hitPoint.m_rootCollidableA = nullptr;
    v5->m_phantom->linearCast(&v10, v8, a3, nullptr);
    if (a3.m_hitPoint.m_rootCollidableA) {
        linearCastOutput.Assign(a3.m_hitPoint);
        return 1;
    }

    return 0;
}

// The wheel's contact from the cast hit: the contact point, the body hit (when it is an entity) and its friction, the
// suspension length at the hit, and the closing speed and scaling factor from the angle between the suspension and
// the contact normal (clipped at the vehicle data's normalClippingAngleCos).
void hkpVehicleLinearCastWheelCollide::getCollisionOutputFromCastResult(
    const hkpVehicleInstance* vehicle,
    uint8_t wheelNum,
    const hkpRootCdPoint& linearCastOutput,
    CollisionDetectionWheelOutput& output
) const {
    const hkpVehicleInstance::WheelInfo& wheelInfo = vehicle->m_wheelsInfo.m_data[wheelNum];
    const float suspensionLength = vehicle->m_suspension->m_wheelParams.m_data[wheelNum].m_length;
    // SWEEP-INLINE 0x4A3F10
    output.m_contactPoint = linearCastOutput.m_contact;
    const hkpCollidable* collidable = linearCastOutput.m_rootCollidableB;
    // EXE-INLINED none: hkGetRigidBody (the collidable's owner when its broad phase type is an entity)
    hkpRigidBody* body = nullptr;
    if (collidable->m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY) {
        // The owner sits m_ownerOffset raw bytes from the collidable.
        body = reinterpret_cast<hkpRigidBody*>(
            const_cast<char*>(reinterpret_cast<const char*>(collidable)) + collidable->m_ownerOffset
        );
    }
    output.m_contactBody = body;
    output.m_contactFriction = body->m_material.m_friction;
    output.m_contactShapeKey[0] = linearCastOutput.m_shapeKeyB;
    // HK_INVALID_SHAPE_KEY ends the key list.
    output.m_contactShapeKey[1] = 0xFFFFFFFF;
    output.m_currentSuspensionLength = linearCastOutput.m_contact.m_separatingNormal.m_quad[3] * suspensionLength;

    const hkQuadReal& direction = wheelInfo.m_suspensionDirectionWs.m_quad;
    const hkQuadReal& normal = output.m_contactPoint.m_separatingNormal.m_quad;
    const float denominator = (direction[1] * normal[1] + direction[0] * normal[0]) + direction[2] * normal[2];
    if (denominator < -vehicle->m_data->m_normalClippingAngleCos) {
        hkVector4 chassisVelocity;
        hkVector4 groundVelocity;
        static_cast<hkpRigidBody*>(vehicle->m_entity)
            ->getPointVelocity(output.m_contactPoint.m_position, chassisVelocity);
        output.m_contactBody->getPointVelocity(output.m_contactPoint.m_position, groundVelocity);
        // Divides the double constant -1.0 by the float and rounds the result to float.
        const float inverse = static_cast<float>(-1.0 / denominator);
        const hkQuadReal& normalAfter = output.m_contactPoint.m_separatingNormal.m_quad;
        const float dx = (chassisVelocity.m_quad[0] - groundVelocity.m_quad[0]) * normalAfter[0];
        const float dy = (chassisVelocity.m_quad[1] - groundVelocity.m_quad[1]) * normalAfter[1];
        const float dz = (chassisVelocity.m_quad[2] - groundVelocity.m_quad[2]) * normalAfter[2];
        const float projectedVelocity = (dy + dx) + dz;
        output.m_suspensionClosingSpeed = projectedVelocity * inverse;
        output.m_suspensionScalingFactor = inverse;
    }
    else {
        output.m_suspensionClosingSpeed = 0.0f;
        output.m_suspensionScalingFactor = 1.0f / vehicle->m_data->m_normalClippingAngleCos;
    }
}

void hkpVehicleLinearCastWheelCollide::getCollisionOutputWithoutHit(
    const hkpVehicleInstance* vehicle,
    uint8_t wheelNum,
    hkpVehicleWheelCollide::CollisionDetectionWheelOutput& cdInfo
) const {
    hkpVehicleInstance::WheelInfo* v4 = &vehicle->m_wheelsInfo.m_data[wheelNum];
    double m_length = vehicle->m_suspension->m_wheelParams.m_data[wheelNum].m_length;
    cdInfo.m_contactBody = nullptr;
    cdInfo.m_currentSuspensionLength = m_length;
    cdInfo.m_suspensionClosingSpeed = 0.0f;
    cdInfo.m_contactPoint.m_position = v4->m_rayEndPointWs;
    __m128 v6 = _mm_xor_ps(v4->m_suspensionDirectionWs.m_quad, hkQuadRealNegateMask);
    cdInfo.m_contactPoint.m_separatingNormal.m_quad =
        _mm_shuffle_ps(v6, _mm_unpackhi_ps(v6, cdInfo.m_contactPoint.m_separatingNormal.m_quad), 196);
    cdInfo.m_contactFriction = 0.0f;
    cdInfo.m_suspensionScalingFactor = 1.0f;
    cdInfo.m_contactPoint.m_separatingNormal.m_quad[3] = m_length;
}

// The wheel's cast: its transform from the chassis's rotation times the wheel's steering orientation and the hard
// point, its end the hard point plus the suspension direction times the suspension length.
void hkpVehicleLinearCastWheelCollide::updateWheelState(const hkpVehicleInstance* vehicle, uint8_t wheelNum) {
    const hkpVehicleInstance::WheelInfo& wheelInfo = vehicle->m_wheelsInfo.m_data[wheelNum];
    WheelState& wheelState = m_wheelStates.m_data[wheelNum];
    hkQuaternion rotation;
    rotation.setMul(
        vehicle->m_entity->m_motion.m_motionState.m_sweptTransform.m_rotation1,
        wheelInfo.m_steeringOrientationChassisSpace
    );
    wheelState.m_transform.m_rotation.set(rotation);
    wheelState.m_transform.m_translation = wheelInfo.m_hardPointWs;
    const __m128 length = _mm_set_ss(vehicle->m_suspension->m_wheelParams.m_data[wheelNum].m_length);
    wheelState.m_to.m_quad = _mm_add_ps(
        _mm_mul_ps(_mm_shuffle_ps(length, length, 0), wheelInfo.m_suspensionDirectionWs.m_quad),
        wheelInfo.m_hardPointWs.m_quad
    );
}

void hkpVehicleLinearCastWheelCollide::centerWheelContactPoint(
    const hkpVehicleInstance* vehicle,
    uint8_t wheelIndex,
    hkpVehicleWheelCollide::CollisionDetectionWheelOutput& cdInfo
) const {
    hkVector4 a3;
    hkVector4 v6;
    hkTransform* p_m_transform = &m_wheelStates.m_data[wheelIndex].m_transform;
    a3.setTransformedInversePos(*p_m_transform, cdInfo.m_contactPoint.m_position);
    a3.m_quad[2] = 0.0f;
    v6.setTransformedPos(*p_m_transform, a3);
    cdInfo.m_contactPoint.m_position.m_quad = v6.m_quad;
}

// Sizes the wheel states to the vehicle's wheels and takes a reference to each wheel's shape. The wheel count is read
// as an unsigned byte.
void hkpVehicleLinearCastWheelCollide::setWheelShapes(
    const hkpVehicleInstance* vehicle,
    const hkArray<hkpShape*, hkContainerHeapAllocator>& shapes
) {
    const int32_t numWheels = static_cast<uint8_t>(vehicle->m_data->m_numWheels);
    // EXE-INLINED none: hkArray<hkpVehicleLinearCastWheelCollide::WheelState>::setSize
    hkMemoryAllocator& heap = *hkMemoryRouter::getInstance().m_heap;
    const int32_t capacity = m_wheelStates.m_capacityAndFlags & hkArrayBase<WheelState>::CAPACITY_MASK;
    if (capacity < numWheels) {
        const int32_t newCapacity = numWheels < 2 * capacity ? 2 * capacity : numWheels;
        hkArrayUtil::_reserve(heap, &m_wheelStates, newCapacity, sizeof(WheelState));
    }
    m_wheelStates.m_size = numWheels;
    for (uint8_t i = 0; i < numWheels; ++i) {
        m_wheelStates.m_data[i].m_shape = shapes.m_data[i];
        shapes.m_data[i]->addReference();
    }
}

hkpCylinderShape* hkpVehicleLinearCastWheelCollide::createWheelShape(float width, float radius) {
    __m128 v3 = _mm_undefined_ps();
    __m128 v4 = _mm_undefined_ps();
    uint32_t v8;
    hkVector4 vertexB;
    hkVector4 vertexA;
    v3[0] = 0.0f;
    v4[0] = 0.0f;
    *reinterpret_cast<float*>(&v8) = width * 0.5;
    vertexA.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero))), _mm_set_ss(std::bit_cast<float>(v8))
        ),
        _mm_unpacklo_ps(v3, v4)
    );
    vertexB.m_quad = _mm_xor_ps(rdata_10C4C00, vertexA.m_quad);
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpCylinderShape* v6 = static_cast<hkpCylinderShape*>(Instance->m_heap->blockAlloc(sizeof(hkpCylinderShape)));
    v6->m_memSizeAndFlags = sizeof(hkpCylinderShape);
    return ::new (v6) hkpCylinderShape(vertexA, vertexB, radius, 0.0f);
}

void hkpVehicleLinearCastWheelCollide::calcAabbOfWheel(
    const hkpVehicleInstance* vehicle,
    uint8_t wheelNum,
    hkAabb& aabbOut
) const {
    __m128 v4 = _mm_undefined_ps();
    __m128 v5 = _mm_undefined_ps();
    uint32_t v13;
    uint32_t v14;
    hkVector4 center;
    hkVector4 halfExtents;
    hkpVehicleData::WheelComponentParams* m_data = vehicle->m_data->m_wheelParams.begin();
    __m128 m_radius_low = _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_data[wheelNum].m_radius)));
    float* p_m_radius = &m_data[wheelNum].m_radius;
    v5[0] = 0.0f;
    *reinterpret_cast<float*>(&v13) =
        (vehicle->m_suspension->m_wheelParams.m_data[wheelNum].m_length + *p_m_radius) * 0.5;
    __m128 v9 = _mm_set_ss(std::bit_cast<float>(v13));
    v4[0] = *reinterpret_cast<float*>(&v13);
    *reinterpret_cast<float*>(&v14) = 0.5 * p_m_radius[2];
    hkpVehicleLinearCastWheelCollide::WheelState* v10 = m_wheelStates.m_data;
    __m128 v11 =
        _mm_unpacklo_ps(_mm_unpacklo_ps(m_radius_low, _mm_set_ss(std::bit_cast<float>(v14))), _mm_unpacklo_ps(v4, v5));
    __m128 v12 = _mm_set_ss(std::bit_cast<float>(0x80000000u));
    halfExtents.m_quad = v11;
    v12[0] = -0.0f - *reinterpret_cast<float*>(&v13);
    v11[0] = 0.0f;
    v9[0] = 0.0f;
    center.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(v11, v9), _mm_unpacklo_ps(v12, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero))))
    );
    hkAabbUtil::calcAabb(v10[wheelNum].m_transform, halfExtents, center, 0.0f, aabbOut);
}

const hkClass& hkpVehicleLinearCastWheelCollide::staticClass() {
    return hkpVehicleLinearCastWheelCollideClass;
}
