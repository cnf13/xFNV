#include "Havok/Source/Physics/Utilities/Weapons/hkpBallGun.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Queue/hkQueue.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/Attributes/hkAttributes.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkCustomAttributes.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Sphere/hkpSphereShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBodyCinfo.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Utilities/Dynamics/Inertia/hkpInertiaTensorComputer.h"
#include "Havok/Source/Physics/Utilities/Weapons/hkpFirstPersonGun.h"

#include <new>

// The reflection data of hkpBallGun's class record, as the Havok SDK's class file has it: per member its attribute
// objects, the attribute list naming them and the custom attributes record over the list, then the member table.
using hkpBallGunAttributeArray2 = const hkCustomAttributes::Attribute[2];

namespace {
    // Per member its offset into this struct (-1: no default), then the defaults.
    struct hkpBallGun_DefaultStruct {
        int32_t s_defaultOffsets[7];
        float m_bulletRadius;
        float m_bulletVelocity;
        float m_bulletMass;
        float m_damageMultiplier;
        int32_t m_maxBulletsInWorld;
        float m_bulletOffsetFromCenter[4];
    };
} // namespace

// Fires a ball from the view transform's position plus m_bulletOffsetFromCenter: a sphere shape of m_bulletRadius, a
// body info filled from the gun's fields, a sphere swept 200 units along the view transform's x axis to find a target
// (aiming at it when it hits, else flying straight along that axis), then the new body added to the world and to the
// bullet queue, the oldest one dropped once m_maxBulletsInWorld are out. The listeners hear about the shot (from a copy
// of the view transform's position), each new body and each removal.

using hkpBallGunAttributeArray3 = const hkCustomAttributes::Attribute[3];
static hkRangeRealAttribute hkpBallGun_bulletRadius_RangeReal{0.0f, 100.0f, 0.0f, 1.0f};
static hkUiAttribute hkpBallGun_bulletRadius_Ui{true, {hkUiAttribute::NONE}, "Radius", "Bullet", "", false, false,
                                                false};
static hkSemanticsAttribute hkpBallGun_bulletRadius_Semantics{{hkSemanticsAttribute::DISTANCE}};
static hkpBallGunAttributeArray3 hkpBallGun_bulletRadius_AttributeList{
    {"hk.RangeReal", {&hkpBallGun_bulletRadius_RangeReal, &hkRangeRealAttributeClass}},
    {"hk.Ui", {&hkpBallGun_bulletRadius_Ui, &hkUiAttributeClass}},
    {"hk.Semantics", {&hkpBallGun_bulletRadius_Semantics, &hkSemanticsAttributeClass}}
};
static const hkCustomAttributes hkpBallGun_bulletRadius_Attributes{hkpBallGun_bulletRadius_AttributeList, 3};
static hkRangeRealAttribute hkpBallGun_bulletVelocity_RangeReal{0.0f, 10000.0f, 0.0f, 100.0f};
static hkUiAttribute hkpBallGun_bulletVelocity_Ui{true, {hkUiAttribute::NONE}, "Velocity", "", "", false, false, false};
static hkpBallGunAttributeArray2 hkpBallGun_bulletVelocity_AttributeList{
    {"hk.RangeReal", {&hkpBallGun_bulletVelocity_RangeReal, &hkRangeRealAttributeClass}},
    {"hk.Ui", {&hkpBallGun_bulletVelocity_Ui, &hkUiAttributeClass}}
};
static const hkCustomAttributes hkpBallGun_bulletVelocity_Attributes{hkpBallGun_bulletVelocity_AttributeList, 2};
static hkRangeRealAttribute hkpBallGun_bulletMass_RangeReal{0.0f, 1000000.0f, 0.0f, 100.0f};
static hkUiAttribute hkpBallGun_bulletMass_Ui{true, {hkUiAttribute::NONE}, "Mass", "", "", false, false, false};
static hkpBallGunAttributeArray2 hkpBallGun_bulletMass_AttributeList{
    {"hk.RangeReal", {&hkpBallGun_bulletMass_RangeReal, &hkRangeRealAttributeClass}},
    {"hk.Ui", {&hkpBallGun_bulletMass_Ui, &hkUiAttributeClass}}
};
static const hkCustomAttributes hkpBallGun_bulletMass_Attributes{hkpBallGun_bulletMass_AttributeList, 2};
static hkRangeRealAttribute hkpBallGun_damageMultiplier_RangeReal{0.0f, 10000.0f, 0.0f, 10.0f};
static hkUiAttribute hkpBallGun_damageMultiplier_Ui{true, {hkUiAttribute::NONE}, "", "", "", true, false, false};
static hkpBallGunAttributeArray2 hkpBallGun_damageMultiplier_AttributeList{
    {"hk.RangeReal", {&hkpBallGun_damageMultiplier_RangeReal, &hkRangeRealAttributeClass}},
    {"hk.Ui", {&hkpBallGun_damageMultiplier_Ui, &hkUiAttributeClass}}
};
static const hkCustomAttributes hkpBallGun_damageMultiplier_Attributes{hkpBallGun_damageMultiplier_AttributeList, 2};
static hkUiAttribute hkpBallGun_maxBulletsInWorld_Ui{true, {hkUiAttribute::NONE}, "", "", "", false, false, false};
static hkRangeInt32Attribute hkpBallGun_maxBulletsInWorld_RangeInt32{0, 100, 0, 100};
static hkpBallGunAttributeArray2 hkpBallGun_maxBulletsInWorld_AttributeList{
    {"hk.Ui", {&hkpBallGun_maxBulletsInWorld_Ui, &hkUiAttributeClass}},
    {"hk.RangeInt32", {&hkpBallGun_maxBulletsInWorld_RangeInt32, &hkRangeInt32AttributeClass}}
};
static const hkCustomAttributes hkpBallGun_maxBulletsInWorld_Attributes{hkpBallGun_maxBulletsInWorld_AttributeList, 2};
static hkRangeRealAttribute hkpBallGun_bulletOffsetFromCenter_RangeReal{-10000.0f, 100000.0f, -10000.0f, 100000.0f};
static hkUiAttribute hkpBallGun_bulletOffsetFromCenter_Ui{
    true, {hkUiAttribute::NONE}, "Center Offset", "", "", false, false, false
};
static hkSemanticsAttribute hkpBallGun_bulletOffsetFromCenter_Semantics{{hkSemanticsAttribute::POSITION}};
static hkpBallGunAttributeArray3 hkpBallGun_bulletOffsetFromCenter_AttributeList{
    {"hk.RangeReal", {&hkpBallGun_bulletOffsetFromCenter_RangeReal, &hkRangeRealAttributeClass}},
    {"hk.Ui", {&hkpBallGun_bulletOffsetFromCenter_Ui, &hkUiAttributeClass}},
    {"hk.Semantics", {&hkpBallGun_bulletOffsetFromCenter_Semantics, &hkSemanticsAttributeClass}}
};
static const hkCustomAttributes hkpBallGun_bulletOffsetFromCenter_Attributes{
    hkpBallGun_bulletOffsetFromCenter_AttributeList, 3
};
using hkpBallGunClass_MembersArray = const hkClassMember[7];
static hkpBallGunClass_MembersArray hkpBallGunClass_Members{
    {"bulletRadius",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x20,
     &hkpBallGun_bulletRadius_Attributes},
    {"bulletVelocity",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x24,
     &hkpBallGun_bulletVelocity_Attributes},
    {"bulletMass",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x28,
     &hkpBallGun_bulletMass_Attributes},
    {"damageMultiplier",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x2C,
     &hkpBallGun_damageMultiplier_Attributes},
    {"maxBulletsInWorld",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x30,
     &hkpBallGun_maxBulletsInWorld_Attributes},
    {"bulletOffsetFromCenter",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x40,
     &hkpBallGun_bulletOffsetFromCenter_Attributes},
    {"addedBodies",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x50,
     nullptr}
};

// The defaults the class record names.
static const hkpBallGun_DefaultStruct hkpBallGun_Default{
    {0x1C, 0x20, 0x24, 0x28, 0x2C, 0x30, -1}, 0.2f, 40.0f, 50.0f, 50.0f, 100, {0.0f, 0.0f, 0.0f, 0.0f}
};
hkClass hkpBallGunClass(
    "hkpBallGun",
    &hkpFirstPersonGunClass,
    96,
    nullptr,
    0,
    nullptr,
    0,
    hkpBallGunClass_Members,
    7,
    &hkpBallGun_Default,
    nullptr,
    0,
    0
);
hkTypeInfo hkpBallGunTypeInfo = {"hkpBallGun",          finishLoadedObjecthkpBallGun, cleanupLoadedObjecthkpBallGun,
                                 getVtablehkpBallGun(), &typeid(hkpBallGun),          sizeof(hkpBallGun)};

hkpBallGun::hkpBallGun(int32_t numBullets)
  : m_bulletRadius(0.2f), m_bulletVelocity(40.0f), m_bulletMass(50.0f), m_damageMultiplier(50.0f),
    m_maxBulletsInWorld(numBullets) {
    m_bulletOffsetFromCenter.setZero4();
    m_type.m_storage = WEAPON_TYPE_BALLGUN;
    m_name = "BallGun";
    void* pvQueueMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkQueue<hkpRigidBody*>));
    m_addedBodies = pvQueueMemory ? new (pvQueueMemory) hkQueue<hkpRigidBody*>() : nullptr;
    m_addedBodies->grow(numBullets);
}

// Takes every queued bullet out: the listeners hear of it, it leaves its world and loses the gun's reference. Then
// the queue goes back to the heap it came from.
hkpBallGun::~hkpBallGun() {
    while (m_addedBodies->m_elementsInUse) {
        hkQueue<hkpRigidBody*>* pQueue = m_addedBodies;
        hkpRigidBody* pBody = pQueue->m_data[pQueue->m_head++];
        if (pQueue->m_head == pQueue->m_capacity) {
            pQueue->m_head = 0;
        }
        --pQueue->m_elementsInUse;
        for (int32_t i = m_listeners.m_size - 1; i >= 0; --i) {
            m_listeners.m_data[i]->bulletDeletedCallback(pBody);
        }
        if (pBody->m_world) {
            pBody->m_world->removeEntity(pBody);
        }
        pBody->removeReference();
    }
    if (m_addedBodies) {
        m_addedBodies->~hkQueue();
        hkMemoryRouter::getInstance().m_heap->blockFree(m_addedBodies, sizeof(hkQueue<hkpRigidBody*>));
    }
}

void hkpBallGun::fireGun(hkpWorld* world, const hkTransform& viewTransform) {
    alignas(16) hkVector4 kDirection;
    alignas(16) hkVector4 kFrom;
    alignas(16) hkVector4 kStart;
    alignas(16) hkVector4 kEnd;
    alignas(16) hkpRigidBodyCinfo kInfo;

    kDirection = viewTransform.m_rotation.m_col0;
    kFrom = viewTransform.m_translation;
    kStart.m_quad = _mm_add_ps(kFrom.m_quad, kDirection.m_quad);
    kEnd.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(200.0f), kDirection.m_quad), kStart.m_quad);
    kInfo.~hkpRigidBodyCinfo();
    ::new (&kInfo) hkpRigidBodyCinfo;
    void* pvShapeMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpSphereShape));
    static_cast<hkReferencedObject*>(pvShapeMemory)->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkpSphereShape));
    hkpSphereShape* pShape = ::new (pvShapeMemory) hkpSphereShape(m_bulletRadius);
    kInfo.m_mass = m_bulletMass;
    kInfo.m_allowedPenetrationDepth = 0.01f;
    kInfo.m_shape = pShape;
    kInfo.m_restitution = 0.2f;
    kInfo.m_qualityType.m_storage = HK_COLLIDABLE_QUALITY_BULLET;
    kInfo.m_motionType.m_storage = hkpMotion::MOTION_SPHERE_INERTIA;
    kInfo.m_friction = 1.0f;
    const float fDoubleMass = m_bulletMass + m_bulletMass;
    computeMassProperties(pShape, fDoubleMass, kInfo);
    kInfo.m_mass = m_bulletMass;
    kInfo.m_linearDamping = 0.0f;
    kInfo.m_angularDamping = 0.4f;
    SweepSphereOut kSweepOut;
    kInfo.m_position.m_quad = _mm_add_ps(m_bulletOffsetFromCenter.m_quad, kStart.m_quad);
    // sweepSphere fills the contact point either way: where the sweep hit, else the sweep's end.
    if (hkpFirstPersonGun::sweepSphere(world, kStart, m_bulletRadius, kEnd, kSweepOut) == HK_SUCCESS) {
        hkpFirstPersonGun::calcVelocityToTarget(
            kInfo.m_position, kSweepOut.m_contactPoint.m_position, world->m_gravity, m_bulletVelocity,
            kInfo.m_linearVelocity
        );
    }
    else {
        kInfo.m_linearVelocity.m_quad = _mm_mul_ps(_mm_set1_ps(m_bulletVelocity), kDirection.m_quad);
    }
    alignas(16) hkVector4 kHitPoint;
    kHitPoint = kSweepOut.m_contactPoint.m_position;
    for (int32_t i = 0; i < m_listeners.m_size; ++i) {
        m_listeners.m_data[i]->gunFiredCallback(&kFrom, &kHitPoint);
    }
    void* pvBodyMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpRigidBody));
    static_cast<hkReferencedObject*>(pvBodyMemory)->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkpRigidBody));
    hkpRigidBody* pBody = ::new (pvBodyMemory) hkpRigidBody(kInfo);
    kInfo.m_shape->removeReference();
    pBody->m_damageMultiplier = m_damageMultiplier;
    for (int32_t i = 0; i < m_listeners.m_size; ++i) {
        m_listeners.m_data[i]->bulletCreatedCallback(pBody);
    }
    world->addEntity(pBody, HK_ENTITY_ACTIVATION_DO_ACTIVATE);
    hkQueue<hkpRigidBody*>* pQueue = m_addedBodies;
    if (pQueue->m_elementsInUse >= pQueue->m_capacity) {
        pQueue->grow(pQueue->m_capacity ? pQueue->m_capacity * 2 : 8);
    }
    if (pQueue->m_tail == pQueue->m_capacity) {
        pQueue->m_tail = 0;
    }
    pQueue->m_data[pQueue->m_tail] = pBody;
    ++pQueue->m_tail;
    ++pQueue->m_elementsInUse;
    // Reads m_addedBodies again here.
    pQueue = m_addedBodies;
    if (pQueue->m_elementsInUse > m_maxBulletsInWorld) {
        int32_t iHead = pQueue->m_head;
        hkpRigidBody* pOldest = pQueue->m_data[iHead++];
        pQueue->m_head = iHead;
        if (iHead == pQueue->m_capacity) {
            pQueue->m_head = 0;
        }
        --pQueue->m_elementsInUse;
        for (int32_t i = m_listeners.m_size - 1; i >= 0; --i) {
            m_listeners.m_data[i]->bulletDeletedCallback(pOldest);
        }
        if (pOldest->m_world) {
            world->removeEntity(pOldest);
        }
        pOldest->removeReference();
    }
}

void hkpBallGun::reset(hkpWorld* world) {
    hkQueue<hkpRigidBody*>* m_addedBodies = this->m_addedBodies;
    if (m_addedBodies->m_elementsInUse) {
        hkpWorld* v4 = world;
        do {
            int32_t m_head = m_addedBodies->m_head;
            hkpEntity* v6 = (hkpEntity*)m_addedBodies->m_data[m_head++];
            m_addedBodies->m_head = m_head;
            if (m_head == m_addedBodies->m_capacity) {
                m_addedBodies->m_head = 0;
            }
            --m_addedBodies->m_elementsInUse;
            for (int32_t i = m_listeners.m_size - 1; i >= 0; --i) {
                hkpFirstPersonGunBulletListener* v8 = m_listeners.m_data[i];
                v8->bulletDeletedCallback(static_cast<hkpRigidBody*>(v6));
            }
            if (v6->m_world == v4) {
                v4->removeEntity(v6);
            }
            v6->removeReference();
            m_addedBodies = this->m_addedBodies;
        }
        while (m_addedBodies->m_elementsInUse);
    }
}

// Computes the shape's volume mass properties for afMass and copies the mass, centre of mass and inertia tensor into
// arInfo, which it returns.
hkpRigidBodyCinfo* hkpBallGun::computeMassProperties(const hkpShape* apShape, float afMass, hkpRigidBodyCinfo& arInfo) {
    // EXE-INLINED none: hkpMassProperties::hkpMassProperties
    alignas(16) hkpMassProperties ostr;
    ostr.m_volume = 0.0f;
    ostr.m_mass = 0.0f;
    ostr.m_centerOfMass.m_quad = {0.0f, 0.0f, 0.0f, 0.0f};
    ostr.m_inertiaTensor.m_col0.m_quad = {0.0f, 0.0f, 0.0f, 0.0f};
    ostr.m_inertiaTensor.m_col1.m_quad = {0.0f, 0.0f, 0.0f, 0.0f};
    ostr.m_inertiaTensor.m_col2.m_quad = {0.0f, 0.0f, 0.0f, 0.0f};
    hkpInertiaTensorComputer::computeShapeVolumeMassProperties(apShape, afMass, ostr);
    arInfo.m_mass = ostr.m_mass;
    arInfo.m_centerOfMass.m_quad = ostr.m_centerOfMass.m_quad;
    arInfo.m_inertiaTensor.m_col0.m_quad = ostr.m_inertiaTensor.m_col0.m_quad;
    arInfo.m_inertiaTensor.m_col1.m_quad = ostr.m_inertiaTensor.m_col1.m_quad;
    arInfo.m_inertiaTensor.m_col2.m_quad = ostr.m_inertiaTensor.m_col2.m_quad;
    return &arInfo;
}

const hkClass& hkpBallGun::staticClass() {
    return hkpBallGunClass;
}
