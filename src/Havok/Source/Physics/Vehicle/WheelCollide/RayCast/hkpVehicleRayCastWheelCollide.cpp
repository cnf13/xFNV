#include "Havok/Source/Physics/Vehicle/WheelCollide/RayCast/hkpVehicleRayCastWheelCollide.h"

#include "BSCore/RTTI.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/hkpRayCastQueryJobs.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastCollectorOutput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpUnaryAction.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
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
#include "xfnv/Compiler.h"

#include <xmmintrin.h>

static void cleanupLoadedObjecthkpVehicleRayCastWheelCollide(void* p);
static void finishLoadedObjecthkpVehicleRayCastWheelCollide(void* p, int32_t finishing);
// Havok SDK signature (hkpVehicle.lib, tied by reloc,target).
static const void* getVtablehkpVehicleRayCastWheelCollide();

// The reflected members of hkpVehicleRayCastWheelCollide.
using hkpVehicleRayCastWheelCollideClass_MembersArray = const hkClassMember[3];
static hkpVehicleRayCastWheelCollideClass_MembersArray hkpVehicleRayCastWheelCollideClass_Members{
    {"wheelCollisionFilterInfo",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xC,
     nullptr},
    {"phantom",
     &hkpAabbPhantomClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x10,
     nullptr},
    {"rejectRayChassisListener",
     &hkpRejectChassisListenerClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x14,
     nullptr}
};
hkClass hkpVehicleRayCastWheelCollideClass(
    "hkpVehicleRayCastWheelCollide",
    &hkpVehicleWheelCollideClass,
    36,
    nullptr,
    0,
    nullptr,
    0,
    hkpVehicleRayCastWheelCollideClass_Members,
    3,
    nullptr,
    nullptr,
    0,
    1
);
hkTypeInfo hkpVehicleRayCastWheelCollideTypeInfo = {
    "hkpVehicleRayCastWheelCollide",
    finishLoadedObjecthkpVehicleRayCastWheelCollide,
    cleanupLoadedObjecthkpVehicleRayCastWheelCollide,
    getVtablehkpVehicleRayCastWheelCollide(),
    &typeid(hkpVehicleRayCastWheelCollide),
    sizeof(hkpVehicleRayCastWheelCollide)
};

static void cleanupLoadedObjecthkpVehicleRayCastWheelCollide(void* p) {
    static_cast<hkpVehicleRayCastWheelCollide*>(p)->~hkpVehicleRayCastWheelCollide();
}

static const void* getVtablehkpVehicleRayCastWheelCollide() {
    return hkGetVtableOf<hkpVehicleRayCastWheelCollide>();
}

static void finishLoadedObjecthkpVehicleRayCastWheelCollide(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpVehicleRayCastWheelCollide(kFlag);
    }
}

hkpVehicleRayCastWheelCollide::hkpVehicleRayCastWheelCollide() : m_wheelCollisionFilterInfo(0), m_phantom(nullptr) {
    m_type.m_storage = RAY_CAST_WHEEL_COLLIDE;
}

// Takes the chassis-rejecting listener off the phantom when it has a chassis, and drops the phantom. The listener is
// destroyed by its own destructor.
hkpVehicleRayCastWheelCollide::~hkpVehicleRayCastWheelCollide() {
    if (m_phantom) {
        if (m_rejectRayChassisListener.m_chassis) {
            m_phantom->removePhantomOverlapListener(&m_rejectRayChassisListener);
        }
        m_phantom->removeReference();
    }
}

void hkpVehicleRayCastWheelCollide::init(const hkpVehicleInstance* vehicle) {
    hkAabb aabbOut_;
    calcWheelsAABB(vehicle, aabbOut_);
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpAabbPhantom* v4 = static_cast<hkpAabbPhantom*>(Instance->m_heap->blockAlloc(sizeof(hkpAabbPhantom)));
    v4->m_memSizeAndFlags = sizeof(hkpAabbPhantom);
    hkpAabbPhantom* hkpAabbPhantom_ = ::new (v4) hkpAabbPhantom(aabbOut_, m_wheelCollisionFilterInfo);
    m_phantom = hkpAabbPhantom_;
    m_rejectRayChassisListener.m_chassis = &vehicle->m_entity->m_collidable;
    // Tests the address &m_rejectRayChassisListener, which is never null.
    hkpAabbPhantom_->addPhantomOverlapListener(&static_cast<hkpPhantomOverlapListener&>(m_rejectRayChassisListener));
}

void hkpVehicleRayCastWheelCollide::collideWheels(
    float deltaTime,
    const hkpVehicleInstance* vehicle,
    hkpVehicleWheelCollide::CollisionDetectionWheelOutput* cdInfoOut
) {
    uint8_t m_numWheels = vehicle->m_data->m_numWheels;
    uint8_t v9 = 0;
    if (m_numWheels) {
        int32_t v10 = 0;
        do {
            hkpWorldRayCastOutput v11;
            v11.m_hitFraction = 1.0f;
            v11.m_shapeKeyIndex = 0;
            v11.m_rootCollidable = nullptr;
            hkpVehicleInstance::WheelInfo* m_data = vehicle->m_wheelsInfo.m_data;
            v11.m_extraInfo = -1;
            v11.m_shapeKeys[0] = -1;
            this->castSingleWheel(m_data[v10], v11);
            if (v11.m_rootCollidable) {
                getCollisionOutputFromCastResult(vehicle, v9, v11, *cdInfoOut);
            }
            else {
                getCollisionOutputWithoutHit(vehicle, v9, *cdInfoOut);
            }
            wheelCollideCallback(vehicle, v9, *cdInfoOut);
            ++v10;
            ++cdInfoOut;
            ++v9;
        }
        while (v9 < m_numWheels);
    }
}

void hkpVehicleRayCastWheelCollide::getPhantoms(hkArray<hkpPhantom*, hkContainerHeapAllocator>& phantomsOut) {
    hkpAabbPhantom* m_phantom = this->m_phantom;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (phantomsOut.m_size == (phantomsOut.m_capacityAndFlags & hkArrayBase<hkpPhantom*>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, &phantomsOut, sizeof(hkpPhantom*));
    }
    phantomsOut.m_data[phantomsOut.m_size++] = m_phantom;
}

void hkpVehicleRayCastWheelCollide::updateBeforeCollisionDetection(const hkpVehicleInstance* vehicle) {
    hkAabb a2;
    calcWheelsAABB(vehicle, a2);
    m_phantom->setAabb(a2);
}

hkpVehicleWheelCollide* hkpVehicleRayCastWheelCollide::clone(
    const hkpRigidBody& newChassis,
    const hkArray<hkpPhantom*, hkContainerHeapAllocator>& newPhantoms
) const {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpVehicleRayCastWheelCollide* v5 = static_cast<hkpVehicleRayCastWheelCollide*>(
        Instance->m_heap->blockAlloc(sizeof(hkpVehicleRayCastWheelCollide))
    );
    v5->m_memSizeAndFlags = sizeof(hkpVehicleRayCastWheelCollide);
    hkpVehicleRayCastWheelCollide* newC = ::new (v5) hkpVehicleRayCastWheelCollide;
    hkpAabbPhantom* phantom = static_cast<hkpAabbPhantom*>(newPhantoms.m_data[0]);
    newC->m_phantom = phantom;
    phantom->addReference();
    // The const of this listener is cast away because the new phantom still holds the original's.
    newC->m_phantom->removePhantomOverlapListener(const_cast<hkpRejectChassisListener*>(&m_rejectRayChassisListener));
    newC->m_phantom->addPhantomOverlapListener(&newC->m_rejectRayChassisListener);
    newC->m_rejectRayChassisListener.m_chassis = &newChassis.m_collidable;
    newC->m_wheelCollisionFilterInfo = m_wheelCollisionFilterInfo;
    return newC;
}

void hkpVehicleRayCastWheelCollide::addToWorld(hkpWorld* world) {
    world->addPhantom(m_phantom);
}

void hkpVehicleRayCastWheelCollide::removeFromWorld() {
    m_phantom->m_world->removePhantom(m_phantom);
}

void hkpVehicleRayCastWheelCollide::setCollisionFilterInfo(uint32_t filterInfo) {
    m_wheelCollisionFilterInfo = filterInfo;
    hkpAabbPhantom* m_phantom = this->m_phantom;
    if (m_phantom) {
        m_phantom->m_collidable.m_broadPhaseHandle.m_collisionFilterInfo = filterInfo;
    }
}

int32_t hkpVehicleRayCastWheelCollide::buildRayCastCommands(
    const hkpVehicleInstance* vehicle,
    const hkpCollisionFilter* collisionFilter,
    int32_t filterSize,
    hkpShapeRayCastCommand* commandStorage,
    hkpWorldRayCastOutput* outputStorage
) const {
    int32_t result;
    m_phantom->ensureDeterministicOrder();
    int32_t m_size = m_phantom->m_overlappingCollidables.m_size;
    if (m_size <= 0) {
        return 0;
    }
    result = static_cast<int32_t>(
        (static_cast<uint32_t>(result) & 0xFFFFFF00u) |
        (static_cast<uint32_t>(static_cast<uint8_t>(vehicle->m_data->m_numWheels)))
    );
    char v13 = result;
    if (static_cast<char>(result) > 0) {
        int32_t v14 = static_cast<uint8_t>(result);
        int32_t v10 = 0;
        do {
            const hkpRayShapeCollectionFilter* v12;
            hkpVehicleInstance::WheelInfo* m_data = vehicle->m_wheelsInfo.m_data;
            commandStorage->m_rayInput.m_from = m_data[v10].m_hardPointWs;
            commandStorage->m_rayInput.m_to = m_data[v10].m_rayEndPointWs;
            commandStorage->m_rayInput.m_filterInfo = m_wheelCollisionFilterInfo;
            if (collisionFilter) {
                v12 = static_cast<const hkpRayShapeCollectionFilter*>(collisionFilter);
            }
            else {
                v12 = nullptr;
            }
            commandStorage->m_rayInput.m_rayShapeCollectionFilter = v12;
            commandStorage->m_filterType = collisionFilter->m_type.m_storage;
            commandStorage->m_filterSize = filterSize;
            commandStorage->m_useCollector.m_bool = 0;
            commandStorage->m_collidables =
                const_cast<const hkpCollidable**>(m_phantom->m_overlappingCollidables.begin());
            commandStorage->m_results = outputStorage;
            commandStorage->m_numCollidables = m_size;
            commandStorage->m_resultsCapacity = 1;
            commandStorage->m_numResultsOut = 0;
            ++v10;
            ++commandStorage;
            ++outputStorage;
            --v14;
        }
        while (v14);
        result = static_cast<int32_t>(
            (static_cast<uint32_t>(result) & 0xFFFFFF00u) | (static_cast<uint32_t>(static_cast<uint8_t>(v13)))
        );
    }
    return static_cast<char>(result);
}

void hkpVehicleRayCastWheelCollide::calcWheelsAABB(const hkpVehicleInstance* vehicle, hkAabb& aabbOut) const {
    aabbOut.m_min.m_quad = _mm_min_ps(
        vehicle->m_wheelsInfo.m_data->m_hardPointWs.m_quad, vehicle->m_wheelsInfo.m_data->m_rayEndPointWs.m_quad
    );
    aabbOut.m_max.m_quad = _mm_max_ps(
        vehicle->m_wheelsInfo.m_data->m_hardPointWs.m_quad, vehicle->m_wheelsInfo.m_data->m_rayEndPointWs.m_quad
    );
    int32_t v3 = 1;
    if (vehicle->m_data->m_numWheels > 1) {
        int32_t v4 = 1;
        do {
            hkVector4 v8;
            hkVector4 v9;
            hkpVehicleInstance::WheelInfo* m_data = vehicle->m_wheelsInfo.m_data;
            __m128 m_quad = m_data[v4].m_rayEndPointWs.m_quad;
            __m128* p_m_quad = &m_data[v4].m_contactPoint.m_position.m_quad;
            v8.m_quad = _mm_min_ps(aabbOut.m_min.m_quad, m_quad);
            aabbOut.m_min.m_quad = v8.m_quad;
            aabbOut.m_min.m_quad = _mm_min_ps(v8.m_quad, p_m_quad[5]);
            v9.m_quad = _mm_max_ps(aabbOut.m_max.m_quad, p_m_quad[6]);
            aabbOut.m_max.m_quad = v9.m_quad;
            aabbOut.m_max.m_quad = _mm_max_ps(v9.m_quad, p_m_quad[5]);
            ++v3;
            ++v4;
        }
        while (v3 < vehicle->m_data->m_numWheels);
    }
}

void hkpVehicleRayCastWheelCollide::castSingleWheel(
    const hkpVehicleInstance::WheelInfo& wheelInfo,
    hkpWorldRayCastOutput& output
) const {
    // The bytes after the hkBool and some other bytes of input are deliberately left unwritten, as the game does;
    // castRay reads only the fields.
    hkpWorldRayCastInput input;
    input.m_from = wheelInfo.m_hardPointWs;
    input.m_filterInfo = m_wheelCollisionFilterInfo;
    input.m_to = wheelInfo.m_rayEndPointWs;
    input.m_enableShapeCollectionFilter.m_bool = 1;
    m_phantom->castRay(input, output);
}

void hkpVehicleRayCastWheelCollide::getCollisionOutputFromCastResult(
    const hkpVehicleInstance* vehicle,
    uint8_t wheelInfoNum,
    const hkpWorldRayCastOutput& rayCastOutput,
    hkpVehicleWheelCollide::CollisionDetectionWheelOutput& cdInfo
) const {
    float suspensionLength = vehicle->m_suspension->m_wheelParams.m_data[wheelInfoNum].m_length;
    const hkpVehicleInstance::WheelInfo* wheelInfo = &vehicle->m_wheelsInfo.m_data[wheelInfoNum];
    cdInfo.m_contactPoint.setNormal(rayCastOutput.m_normal);
    cdInfo.m_contactShapeKey[0] = rayCastOutput.m_shapeKeys[0];
    cdInfo.m_contactShapeKey[1] = rayCastOutput.m_shapeKeys[1];
    cdInfo.m_contactShapeKey[2] = rayCastOutput.m_shapeKeys[2];
    cdInfo.m_contactShapeKey[3] = rayCastOutput.m_shapeKeys[3];
    cdInfo.m_contactShapeKey[4] = rayCastOutput.m_shapeKeys[4];
    cdInfo.m_contactShapeKey[5] = rayCastOutput.m_shapeKeys[5];
    cdInfo.m_contactShapeKey[6] = rayCastOutput.m_shapeKeys[6];
    cdInfo.m_contactShapeKey[7] = rayCastOutput.m_shapeKeys[7];
    hkpRigidBody* contactBody = hkpGetRigidBody(rayCastOutput.m_rootCollidable);
    cdInfo.m_contactBody = contactBody;
    float radius = vehicle->m_data->m_wheelParams.m_data[wheelInfoNum].m_radius;
    float hitDistance = (radius + suspensionLength) * rayCastOutput.m_hitFraction;
    float currentSuspensionLength = hitDistance - radius;
    cdInfo.m_currentSuspensionLength = currentSuspensionLength;
    cdInfo.m_contactPoint.m_position.m_quad = _mm_add_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(hitDistance), _mm_set_ss(hitDistance), 0),
            wheelInfo->m_suspensionDirectionWs.m_quad
        ),
        wheelInfo->m_hardPointWs.m_quad
    );
    XFNV_M128_F32(cdInfo.m_contactPoint.m_separatingNormal.m_quad, 3) = currentSuspensionLength;
    hkVector4 normal = cdInfo.m_contactPoint.m_separatingNormal;
    cdInfo.m_contactFriction = cdInfo.m_contactBody->m_material.m_friction;
    float denominator = normal.dot3(wheelInfo->m_suspensionDirectionWs);
    if (!(-vehicle->m_data->m_normalClippingAngleCos > denominator)) {
        cdInfo.m_suspensionClosingSpeed = 0.0f;
        cdInfo.m_suspensionScalingFactor = 1.0 / vehicle->m_data->m_normalClippingAngleCos;
    }
    else {
        float inv = -1.0 / denominator;
        // EXE-INLINED none: hkpEntity::getPointVelocity, on the chassis and on the ground body
        hkpEntity* chassis = vehicle->m_entity;
        __m128 chassisArm = _mm_sub_ps(
            cdInfo.m_contactPoint.m_position.m_quad,
            chassis->m_motion.m_motionState.m_sweptTransform.m_centerOfMass1.m_quad
        );
        __m128 groundAngular = contactBody->m_motion.m_angularVelocity.m_quad;
        __m128 groundArm = _mm_sub_ps(
            cdInfo.m_contactPoint.m_position.m_quad,
            contactBody->m_motion.m_motionState.m_sweptTransform.m_centerOfMass1.m_quad
        );
        __m128 chassisAngular = chassis->m_motion.m_angularVelocity.m_quad;
        __m128 chassisVelocity = _mm_add_ps(
            _mm_sub_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(chassisArm, chassisArm, 210), _mm_shuffle_ps(chassisAngular, chassisAngular, 201)
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(chassisArm, chassisArm, 201), _mm_shuffle_ps(chassisAngular, chassisAngular, 210)
                )
            ),
            chassis->m_motion.m_linearVelocity.m_quad
        );
        __m128 groundVelocity = _mm_add_ps(
            _mm_sub_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(groundArm, groundArm, 210), _mm_shuffle_ps(groundAngular, groundAngular, 201)
                ),
                _mm_mul_ps(_mm_shuffle_ps(groundArm, groundArm, 201), _mm_shuffle_ps(groundAngular, groundAngular, 210))
            ),
            contactBody->m_motion.m_linearVelocity.m_quad
        );
        hkVector4 relVelocity;
        relVelocity.m_quad = _mm_sub_ps(chassisVelocity, groundVelocity);
        float projVel = relVelocity.dot3(normal);
        cdInfo.m_suspensionClosingSpeed = projVel * inv;
        cdInfo.m_suspensionScalingFactor = inv;
    }
}

void hkpVehicleRayCastWheelCollide::getCollisionOutputWithoutHit(
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
    XFNV_M128_F32(cdInfo.m_contactPoint.m_separatingNormal.m_quad, 3) = m_length;
}

const hkClass& hkpVehicleRayCastWheelCollide::staticClass() {
    return hkpVehicleRayCastWheelCollideClass;
}
