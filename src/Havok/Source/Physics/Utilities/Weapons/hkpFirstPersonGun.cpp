#include "Havok/Source/Physics/Utilities/Weapons/hkpFirstPersonGun.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Queue/hkQueue.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/Attributes/hkAttributes.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkCustomAttributes.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpLinearCastInput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpClosestCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Sphere/hkpSphereShape.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Utilities/Weapons/hkpBallGun.h"
#include "Havok/Source/Physics/Utilities/Weapons/hkpGravityGun.h"
#include "Havok/Source/Physics/Utilities/Weapons/hkpMountedBallGun.h"

#include <new>

namespace {
    // Per member its offset into this struct (-1: no default), then the defaults (name "", keyboardKey KEY_F2).
    struct hkpFirstPersonGun_DefaultStruct {
        int32_t s_defaultOffsets[4];
        const char* m_name;
        uint8_t m_keyboardKey;
    };
} // namespace

// The enums' items and the enums.
using hkpFirstPersonGunTypeEnumItemArray = const hkClassEnum::Item[11];
static hkpFirstPersonGunTypeEnumItemArray hkpFirstPersonGunTypeEnumItems{
    {hkpFirstPersonGun::WEAPON_TYPE_INVALID, "WEAPON_TYPE_INVALID"},
    {hkpFirstPersonGun::WEAPON_TYPE_BALLGUN, "WEAPON_TYPE_BALLGUN"},
    {hkpFirstPersonGun::WEAPON_TYPE_GRENADEGUN, "WEAPON_TYPE_GRENADEGUN"},
    {hkpFirstPersonGun::WEAPON_TYPE_GRAVITYGUN, "WEAPON_TYPE_GRAVITYGUN"},
    {hkpFirstPersonGun::WEAPON_TYPE_MOUNTEDBALLGUN, "WEAPON_TYPE_MOUNTEDBALLGUN"},
    {hkpFirstPersonGun::WEAPON_TYPE_TWEAKERGUN, "WEAPON_TYPE_TWEAKERGUN"},
    {hkpFirstPersonGun::WEAPON_TYPE_MISSILEGUN, "WEAPON_TYPE_MISSILEGUN"},
    {hkpFirstPersonGun::WEAPON_TYPE_RAYCASTGUN, "WEAPON_TYPE_RAYCASTGUN"},
    {hkpFirstPersonGun::WEAPON_TYPE_SPHEREGUN, "WEAPON_TYPE_SPHEREGUN"},
    {hkpFirstPersonGun::WEAPON_TYPE_STICKYGUN, "WEAPON_TYPE_STICKYGUN"},
    {hkpFirstPersonGun::WEAPON_TYPE_NUM_TYPES, "WEAPON_TYPE_NUM_TYPES"}
};
using hkpFirstPersonGunKeyboardKeyEnumItemArray = const hkClassEnum::Item[12];
static hkpFirstPersonGunKeyboardKeyEnumItemArray hkpFirstPersonGunKeyboardKeyEnumItems{
    {hkpFirstPersonGun::KEY_F1, "KEY_F1"},   {hkpFirstPersonGun::KEY_F2, "KEY_F2"},
    {hkpFirstPersonGun::KEY_F3, "KEY_F3"},   {hkpFirstPersonGun::KEY_F4, "KEY_F4"},
    {hkpFirstPersonGun::KEY_F5, "KEY_F5"},   {hkpFirstPersonGun::KEY_F6, "KEY_F6"},
    {hkpFirstPersonGun::KEY_F7, "KEY_F7"},   {hkpFirstPersonGun::KEY_F8, "KEY_F8"},
    {hkpFirstPersonGun::KEY_F9, "KEY_F9"},   {hkpFirstPersonGun::KEY_F10, "KEY_F10"},
    {hkpFirstPersonGun::KEY_F11, "KEY_F11"}, {hkpFirstPersonGun::KEY_F12, "KEY_F12"}
};
using hkpFirstPersonGunEnumArray = const hkClassEnum[2];
static hkpFirstPersonGunEnumArray hkpFirstPersonGunEnums{
    {"Type", hkpFirstPersonGunTypeEnumItems, 11, nullptr, 0},
    {"KeyboardKey", hkpFirstPersonGunKeyboardKeyEnumItems, 12, nullptr, 0}
};

static const hkClassEnum* hkpFirstPersonGunTypeEnum = &hkpFirstPersonGunEnums[0];
static const hkClassEnum* hkpFirstPersonGunKeyboardKeyEnum = &hkpFirstPersonGunEnums[1];

// The members' attribute objects, the attribute lists naming them and the custom attributes records over the lists.
using hkpFirstPersonGunAttributeArray1 = const hkCustomAttributes::Attribute[1];
static hkUiAttribute hkpFirstPersonGun_name_Ui{false, {hkUiAttribute::NONE}, "", "", "", false, false, false};
static hkpFirstPersonGunAttributeArray1 hkpFirstPersonGun_name_AttributeList{
    {"hk.Ui", {&hkpFirstPersonGun_name_Ui, &hkUiAttributeClass}}
};
static const hkCustomAttributes hkpFirstPersonGun_name_Attributes{hkpFirstPersonGun_name_AttributeList, 1};
static hkUiAttribute hkpFirstPersonGun_keyboardKey_Ui{true, {hkUiAttribute::NONE}, "", "", "", false, false, false};
static hkpFirstPersonGunAttributeArray1 hkpFirstPersonGun_keyboardKey_AttributeList{
    {"hk.Ui", {&hkpFirstPersonGun_keyboardKey_Ui, &hkUiAttributeClass}}
};
static const hkCustomAttributes hkpFirstPersonGun_keyboardKey_Attributes{
    hkpFirstPersonGun_keyboardKey_AttributeList, 1
};

// The class record's member table. The type member's m_enum is null in the image and nothing fills it; the keyboardKey
// member's is filled at start-up.
using hkpFirstPersonGunClass_MembersArray = hkClassMember[4];
static hkpFirstPersonGunClass_MembersArray hkpFirstPersonGunClass_Members{
    {"type",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_UINT8},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x8,
     nullptr},
    {"name",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_STRINGPTR},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xC,
     &hkpFirstPersonGun_name_Attributes},
    {"keyboardKey",
     nullptr,
     hkpFirstPersonGunKeyboardKeyEnum,
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_UINT8},
     0,
     {0},
     0x10,
     &hkpFirstPersonGun_keyboardKey_Attributes},
    {"listeners",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x14,
     nullptr}
};

// The defaults the class record names.
static const hkpFirstPersonGun_DefaultStruct hkpFirstPersonGun_Default{
    {-1, 0x10, 0x14, -1},
    "",
    hkpFirstPersonGun::KEY_F2
};
hkClass hkpFirstPersonGunClass(
    "hkpFirstPersonGun",
    &hkReferencedObjectClass,
    32,
    nullptr,
    0,
    hkpFirstPersonGunEnums,
    2,
    hkpFirstPersonGunClass_Members,
    4,
    &hkpFirstPersonGun_Default,
    nullptr,
    0,
    0
);
hkTypeInfo hkpFirstPersonGunTypeInfo = {
    "hkpFirstPersonGun",          finishLoadedObjecthkpFirstPersonGun, cleanupLoadedObjecthkpFirstPersonGun,
    getVtablehkpFirstPersonGun(), &typeid(hkpFirstPersonGun),          sizeof(hkpFirstPersonGun)
};

hkpFirstPersonGun::hkpFirstPersonGun() : m_type{WEAPON_TYPE_INVALID}, m_name(""), m_keyboardKey{KEY_F2} {}

// A loaded gun keeps its fields and name; its listeners start empty.
hkpFirstPersonGun::hkpFirstPersonGun(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_name(aFlag) {
    if (aFlag.m_finishing) {
        m_type.m_storage = WEAPON_TYPE_INVALID;
    }
}

hkpFirstPersonGun::~hkpFirstPersonGun() {
    for (int32_t i = 0; i < m_listeners.m_size; i++) {
        m_listeners.m_data[i]->removeReference();
    }
}

void hkpFirstPersonGun::quitGun(hkpWorld* world) {
    reset(world);
}

const char* hkpFirstPersonGun::getName() const {
    return m_name.cString();
}

// Sweeps a sphere of afRadius from arSweepStart to arSweepEnd through the world with a closest-point collector. On a
// hit, arOut gets the contact, the hit body (null unless the collidable is an entity's) and its shape key, and the
// result is HK_SUCCESS; else arOut holds the end point, a zero normal (its w kept), no body and the invalid shape key.
hkResult hkpFirstPersonGun::sweepSphere(
    const hkpWorld* world,
    const hkVector4& sweepStart,
    float radius,
    const hkVector4& sweepEnd,
    hkpFirstPersonGun::SweepSphereOut& out
) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpSphereShape* pMemory = static_cast<hkpSphereShape*>(Instance->m_heap->blockAlloc(sizeof(hkpSphereShape)));
    pMemory->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkpSphereShape));
    hkpSphereShape* pSphere = ::new (pMemory) hkpSphereShape(radius);
    alignas(16) hkTransform kTransform;
    // EXE-INLINED 0x577090
    kTransform.m_rotation.m_col0.m_quad = {0.0f, 0.0f, 0.0f, 0.0f};
    kTransform.m_rotation.m_col0.m_quad[0] = 1.0f;
    kTransform.m_rotation.m_col1.m_quad = {0.0f, 0.0f, 0.0f, 0.0f};
    kTransform.m_rotation.m_col1.m_quad[1] = 1.0f;
    kTransform.m_rotation.m_col2.m_quad = {0.0f, 0.0f, 0.0f, 0.0f};
    kTransform.m_rotation.m_col2.m_quad[2] = 1.0f;
    kTransform.m_translation.m_quad = sweepStart.m_quad;
    alignas(16) hkpCollidable kCaster(pSphere, &kTransform, 0);
    alignas(16) hkpLinearCastInput kInput;
    kInput.m_to.m_quad = sweepEnd.m_quad;
    kInput.m_maxExtraPenetration = 1.1920929e-07f;
    kInput.m_startPointTolerance = 1.1920929e-07f;
    alignas(16)
        XFNV_INLINED_LOCAL(hkpClosestCdPointCollector, kCollector, hkpClosestCdPointCollector_vftable, std::nullopt);
    // EXE-INLINED 0xC6D170
    kCollector.m_hitPoint.m_contact.m_separatingNormal.m_quad[3] = 3.40282e+38f;
    kCollector.m_earlyOutDistance = 3.40282e+38f;
    kCollector.m_hitPoint.m_rootCollidableA = nullptr;
    world->linearCast(&kCaster, kInput, kCollector, nullptr);
    pSphere->removeReference();
    if (kCollector.m_hitPoint.m_rootCollidableA) {
        out.m_contactPoint.m_position.m_quad = kCollector.m_hitPoint.m_contact.m_position.m_quad;
        out.m_contactPoint.m_separatingNormal.m_quad = kCollector.m_hitPoint.m_contact.m_separatingNormal.m_quad;
        const hkpCollidable* pHit = kCollector.m_hitPoint.m_rootCollidableB;
        // EXE-INLINED 0x4B59F0
        out.m_body = pHit->getType() == hkpWorldObject::BROAD_PHASE_ENTITY
            ? static_cast<hkpRigidBody*>(pHit->getOwner())
            : nullptr;
        out.m_shapeKey = kCollector.m_hitPoint.m_shapeKeyB;
        return HK_SUCCESS;
    }
    out.m_body = nullptr;
    out.m_contactPoint.m_position.m_quad = sweepEnd.m_quad;
    out.m_contactPoint.m_separatingNormal.setXYZ(hkVector4::getZero());
    out.m_shapeKey = HK_INVALID_SHAPE_KEY;
    return HK_FAILURE;
}

// Every lane operation and the square root round to float, so each step is a float local. The divisions and the -0.5
// scale are stored as floats. A distance not above the float epsilon (or unordered) sends the raw offset.
void hkpFirstPersonGun::calcVelocityToTarget(
    const hkVector4& position,
    const hkVector4& target,
    const hkVector4& gravity,
    float speed,
    hkVector4& velocity
) {
    hkVector4 kDir;
    kDir.m_quad[0] = target.m_quad[0] - position.m_quad[0];
    kDir.m_quad[1] = target.m_quad[1] - position.m_quad[1];
    kDir.m_quad[2] = target.m_quad[2] - position.m_quad[2];
    kDir.m_quad[3] = target.m_quad[3] - position.m_quad[3];
    float fXX = kDir.m_quad[0] * kDir.m_quad[0];
    float fYY = kDir.m_quad[1] * kDir.m_quad[1];
    float fZZ = kDir.m_quad[2] * kDir.m_quad[2];
    float fSum = fYY + fXX;
    fSum = fZZ + fSum;
    float fLength = __builtin_elementwise_sqrt(fSum);
    if (!(fLength > 1.1920929e-07f)) {
        velocity = kDir;
        return;
    }
    float fTime = (fLength / speed);
    float fScale = static_cast<float>(fTime * -0.5);
    hkVector4 kExtra;
    kExtra.m_quad[0] = fScale * gravity.m_quad[0];
    kExtra.m_quad[1] = fScale * gravity.m_quad[1];
    kExtra.m_quad[2] = fScale * gravity.m_quad[2];
    kExtra.m_quad[3] = fScale * gravity.m_quad[3];
    fXX = kExtra.m_quad[0] * kExtra.m_quad[0];
    fYY = kExtra.m_quad[1] * kExtra.m_quad[1];
    fZZ = kExtra.m_quad[2] * kExtra.m_quad[2];
    fSum = fYY + fXX;
    fSum = fZZ + fSum;
    float fExtraLength = __builtin_elementwise_sqrt(fSum);
    if (fExtraLength > speed) {
        float fShrink = (speed / fExtraLength);
        kExtra.m_quad[0] = fShrink * kExtra.m_quad[0];
        kExtra.m_quad[1] = fShrink * kExtra.m_quad[1];
        kExtra.m_quad[2] = fShrink * kExtra.m_quad[2];
        kExtra.m_quad[3] = fShrink * kExtra.m_quad[3];
    }
    float fDirScale = (speed / fLength);
    float fX = fDirScale * kDir.m_quad[0];
    float fY = fDirScale * kDir.m_quad[1];
    float fZ = fDirScale * kDir.m_quad[2];
    float fW = fDirScale * kDir.m_quad[3];
    velocity.m_quad[0] = fX + kExtra.m_quad[0];
    velocity.m_quad[1] = fY + kExtra.m_quad[1];
    velocity.m_quad[2] = fZ + kExtra.m_quad[2];
    velocity.m_quad[3] = fW + kExtra.m_quad[3];
}

const hkClass& hkpFirstPersonGun::staticClass() {
    return hkpFirstPersonGunClass;
}

// When finishing, a new bullet queue grown to m_maxBulletsInWorld, from the heap the destructor frees it to.
hkpBallGun::hkpBallGun(hkFinishLoadedObjectFlag flag) : hkpFirstPersonGun(flag) {
    if (flag.m_finishing) {
        m_type.m_storage = WEAPON_TYPE_BALLGUN;
        void* pvMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkQueue<hkpRigidBody*>));
        m_addedBodies = pvMemory ? new (pvMemory) hkQueue<hkpRigidBody*>() : nullptr;
        m_addedBodies->grow(m_maxBulletsInWorld);
    }
}

hkpGravityGun::hkpGravityGun(hkFinishLoadedObjectFlag flag) : hkpFirstPersonGun(flag) {
    if (flag.m_finishing) {
        m_type.m_storage = WEAPON_TYPE_GRAVITYGUN;
    }
}

hkpMountedBallGun::hkpMountedBallGun(hkFinishLoadedObjectFlag flag) : hkpBallGun(flag) {
    if (flag.m_finishing) {
        m_type.m_storage = WEAPON_TYPE_MOUNTEDBALLGUN;
    }
}
