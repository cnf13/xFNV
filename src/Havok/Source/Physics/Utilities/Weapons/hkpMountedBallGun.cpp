#include "Havok/Source/Physics/Utilities/Weapons/hkpMountedBallGun.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
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
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBodyCinfo.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Utilities/Weapons/hkpFirstPersonGun.h"

#include <xmmintrin.h>

namespace {
    // Per member its offset into this struct (-1: no default), then the defaults.
    struct hkpMountedBallGun_DefaultStruct {
        int32_t s_defaultOffsets[1];
        float m_position[4];
    };
} // namespace

// Fires a ball from the gun's m_position: a sphere shape of m_bulletRadius, a body info filled from the gun's fields, a
// sphere swept 200 units along the view transform's x axis to find a target (when it hits, a ballistic velocity aimed
// at the contact point against the world's gravity, else flying straight along that axis), then the new body added to
// the world and to the bullet queue, the oldest one dropped once m_maxBulletsInWorld are out. The listeners hear about
// the shot, each new body and each removal. The vectors and the body info are aligned locals.

// The reflection data of hkpMountedBallGun's class record, as the Havok SDK's class file has it: the position member's
// attribute objects, the attribute list naming them and the custom attributes record over the list, then the member
// table.
using hkpMountedBallGunAttributeArray3 = const hkCustomAttributes::Attribute[3];
static hkRangeRealAttribute hkpMountedBallGun_position_RangeReal{-10000.0f, 100000.0f, -10000.0f, 100000.0f};
static hkUiAttribute hkpMountedBallGun_position_Ui{true, {hkUiAttribute::NONE}, "World Position", "", "", false, false,
                                                   false};
static hkSemanticsAttribute hkpMountedBallGun_position_Semantics{{hkSemanticsAttribute::POSITION}};
static hkpMountedBallGunAttributeArray3 hkpMountedBallGun_position_AttributeList{
    {"hk.RangeReal", {&hkpMountedBallGun_position_RangeReal, &hkRangeRealAttributeClass}},
    {"hk.Ui", {&hkpMountedBallGun_position_Ui, &hkUiAttributeClass}},
    {"hk.Semantics", {&hkpMountedBallGun_position_Semantics, &hkSemanticsAttributeClass}}
};
static const hkCustomAttributes hkpMountedBallGun_position_Attributes{hkpMountedBallGun_position_AttributeList, 3};
using hkpMountedBallGunClass_MembersArray = const hkClassMember[1];
static hkpMountedBallGunClass_MembersArray hkpMountedBallGunClass_Members{
    {"position",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x60,
     &hkpMountedBallGun_position_Attributes}
};

// The defaults the class record names.
static const hkpMountedBallGun_DefaultStruct hkpMountedBallGun_Default{{0x4}, {0.0f, 100.0f, 0.0f, 0.0f}};
hkClass hkpMountedBallGunClass(
    "hkpMountedBallGun",
    &hkpBallGunClass,
    112,
    nullptr,
    0,
    nullptr,
    0,
    hkpMountedBallGunClass_Members,
    1,
    &hkpMountedBallGun_Default,
    nullptr,
    0,
    0
);
hkTypeInfo hkpMountedBallGunTypeInfo = {
    "hkpMountedBallGun",          finishLoadedObjecthkpMountedBallGun, cleanupLoadedObjecthkpMountedBallGun,
    getVtablehkpMountedBallGun(), &typeid(hkpMountedBallGun),          sizeof(hkpMountedBallGun)
};

// The default position is (0, 100, 0, 0).
hkpMountedBallGun::hkpMountedBallGun(int32_t numBullets) : hkpBallGun(numBullets) {
    m_type.m_storage = WEAPON_TYPE_MOUNTEDBALLGUN;
    m_name = "MountedBallGun";
    m_position.m_quad[0] = 0.0f;
    m_position.m_quad[1] = 100.0f;
    m_position.m_quad[2] = 0.0f;
    m_position.m_quad[3] = 0.0f;
}

void hkpMountedBallGun::fireGun(hkpWorld* world, const hkTransform& viewTransform) {
    alignas(16) hkVector4 kDirection;
    alignas(16) hkVector4 kStart;
    alignas(16) hkVector4 kEnd;
    alignas(16) hkpRigidBodyCinfo kInfo;

    kDirection = viewTransform.m_rotation.m_col0;
    kStart.m_quad = _mm_add_ps(viewTransform.m_translation.m_quad, kDirection.m_quad);
    const __m128 kRange = _mm_set_ss(200.0f);
    kEnd.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kRange, kRange, 0), kDirection.m_quad), kStart.m_quad);
    kInfo.~hkpRigidBodyCinfo();
    ::new (&kInfo) hkpRigidBodyCinfo;
    void* pvShapeMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpSphereShape));
    static_cast<hkReferencedObject*>(pvShapeMemory)->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkpSphereShape));
    hkpSphereShape* pShape = ::new (pvShapeMemory) hkpSphereShape(m_bulletRadius);
    kInfo.m_mass = m_bulletMass;
    kInfo.m_allowedPenetrationDepth = 0.01f;
    kInfo.m_restitution = 0.2f;
    kInfo.m_shape = pShape;
    kInfo.m_friction = 1.0f;
    kInfo.m_qualityType.m_storage = HK_COLLIDABLE_QUALITY_BULLET;
    kInfo.m_position = m_position;
    kInfo.m_motionType.m_storage = hkpMotion::MOTION_SPHERE_INERTIA;
    computeMassProperties(pShape, m_bulletMass + m_bulletMass, kInfo);
    kInfo.m_mass = m_bulletMass;
    kInfo.m_linearDamping = 0.0f;
    kInfo.m_angularDamping = 0.4f;
    SweepSphereOut kSweepOut;
    if (hkpFirstPersonGun::sweepSphere(world, kStart, 0.05f, kEnd, kSweepOut)) {
        const __m128 kSpeed = _mm_set_ss(m_bulletVelocity);
        kInfo.m_linearVelocity.m_quad = _mm_mul_ps(_mm_shuffle_ps(kSpeed, kSpeed, 0), kDirection.m_quad);
    }
    else {
        // Aims at the sweep's contact point whether or not the sweep filled it, as the game does.
        hkpFirstPersonGun::calcVelocityToTarget(
            kInfo.m_position, kSweepOut.m_contactPoint.m_position, world->m_gravity, m_bulletVelocity,
            kInfo.m_linearVelocity
        );
    }
    alignas(16) hkVector4 kHitPoint;
    kHitPoint = kSweepOut.m_contactPoint.m_position;
    for (int32_t i = 0; i < m_listeners.m_size; ++i) {
        m_listeners.m_data[i]->gunFiredCallback(&m_position, &kHitPoint);
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
    // Reads m_addedBodies again here, as the game does.
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
            pOldest->m_world->removeEntity(pOldest);
        }
        pOldest->removeReference();
    }
}

const hkClass& hkpMountedBallGun::staticClass() {
    return hkpMountedBallGunClass;
}
