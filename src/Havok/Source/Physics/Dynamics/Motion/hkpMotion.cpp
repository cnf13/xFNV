#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransformUtil.h"
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
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"

#include <bit>
#include <xmmintrin.h>

static void finishLoadedObjecthkpKeyframedRigidMotion(void* p, int32_t finishing);
// Havok SDK signature (hkpDynamics.lib, tied by reloc,target).
static const void* getVtablehkpKeyframedRigidMotion();
static void finishLoadedObjecthkpMaxSizeMotion(void* p, int32_t finishing);
// Havok SDK signature (hkpDynamics.lib, tied by reloc,target).
static const void* getVtablehkpMaxSizeMotion();
static void cleanupLoadedObjecthkpKeyframedRigidMotion(void* p);

hkClass hkpKeyframedRigidMotionClass(
    "hkpKeyframedRigidMotion",
    &hkpMotionClass,
    288,
    nullptr,
    0,
    nullptr,
    0,
    nullptr,
    0,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpKeyframedRigidMotionTypeInfo = {
    "hkpKeyframedRigidMotion",
    finishLoadedObjecthkpKeyframedRigidMotion,
    cleanupLoadedObjecthkpKeyframedRigidMotion,
    getVtablehkpKeyframedRigidMotion(),
    &typeid(hkpKeyframedRigidMotion),
    sizeof(hkpKeyframedRigidMotion)
};
hkClass hkpMaxSizeMotionClass(
    "hkpMaxSizeMotion",
    &hkpKeyframedRigidMotionClass,
    288,
    nullptr,
    0,
    nullptr,
    0,
    nullptr,
    0,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpMaxSizeMotionTypeInfo = {
    "hkpMaxSizeMotion",          finishLoadedObjecthkpMaxSizeMotion, cleanupLoadedObjecthkpMaxSizeMotion,
    getVtablehkpMaxSizeMotion(), &typeid(hkpMaxSizeMotion),          sizeof(hkpMaxSizeMotion)
};

// The reflection data of hkpMotion's class record: the names of MotionType's values, the enum record and savedMotion's
// hk.DataObjectType attribute, the enum by pointer and the member table.
using hkpMotionMotionTypeEnumItemsArray = const hkClassEnum::Item[9];
static hkpMotionMotionTypeEnumItemsArray hkpMotionMotionTypeEnumItems{
    {0, "MOTION_INVALID"},          {1, "MOTION_DYNAMIC"},   {2, "MOTION_SPHERE_INERTIA"},
    {3, "MOTION_BOX_INERTIA"},      {4, "MOTION_KEYFRAMED"}, {5, "MOTION_FIXED"},
    {6, "MOTION_THIN_BOX_INERTIA"}, {7, "MOTION_CHARACTER"}, {8, "MOTION_MAX_ID"}
};
using hkpMotionEnumsArray = const hkClassEnum[1];
static hkpMotionEnumsArray hkpMotionEnums{{"MotionType", hkpMotionMotionTypeEnumItems, 9, nullptr, {0}}};

static const hkClassEnum* hkpMotionMotionTypeEnum = &hkpMotionEnums[0];

// The SDK's m_typeName is a char*, and hkVariant's object pointer is not const.
static hkDataObjectTypeAttribute hkpMotion_SavedMotionDataObjectType{const_cast<char*>("hkpMotion")};
using hkpMotion_SavedMotionAttributeArray = const hkCustomAttributes::Attribute[1];
static hkpMotion_SavedMotionAttributeArray hkpMotion_SavedMotionAttributeList{
    {"hk.DataObjectType", {&hkpMotion_SavedMotionDataObjectType, &hkDataObjectTypeAttributeClass}}
};
static const hkCustomAttributes hkpMotion_SavedMotionAttributes{hkpMotion_SavedMotionAttributeList, 1};
using hkpMotionClass_MembersArray = hkClassMember[12];
static hkpMotionClass_MembersArray hkpMotionClass_Members{
    {"type",
     nullptr,
     hkpMotionMotionTypeEnum,
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_UINT8},
     0,
     {0},
     0x8,
     nullptr},
    {"deactivationIntegrateCounter",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT8},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x9,
     nullptr},
    {"deactivationNumInactiveFrames",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT16},
     {hkClassMember::TYPE_VOID},
     2,
     {0},
     0xA,
     nullptr},
    {"motionState",
     &hkMotionStateClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x10,
     nullptr},
    {"inertiaAndMassInv",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xC0,
     nullptr},
    {"linearVelocity",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xD0,
     nullptr},
    {"angularVelocity",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xE0,
     nullptr},
    {"deactivationRefPosition",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     2,
     {0},
     0xF0,
     nullptr},
    {"deactivationRefOrientation",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT32},
     {hkClassMember::TYPE_VOID},
     2,
     {0},
     0x110,
     nullptr},
    {"savedMotion",
     &hkpMaxSizeMotionClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x118,
     &hkpMotion_SavedMotionAttributes},
    {"savedQualityTypeIndex",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT16},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x11C,
     nullptr},
    {"gravityFactor", nullptr, nullptr, {hkClassMember::TYPE_HALF}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x11E, nullptr}
};
hkClass hkpMotionClass(
    "hkpMotion",
    &hkReferencedObjectClass,
    288,
    nullptr,
    0,
    hkpMotionEnums,
    1,
    hkpMotionClass_Members,
    12,
    nullptr,
    nullptr,
    0,
    3
);

static const void* getVtablehkpKeyframedRigidMotion() {
    return hkGetVtableOf<hkpKeyframedRigidMotion>();
}

static const void* getVtablehkpMaxSizeMotion() {
    return hkGetVtableOf<hkpMaxSizeMotion>();
}

static void cleanupLoadedObjecthkpKeyframedRigidMotion(void* p) {
    static_cast<hkpKeyframedRigidMotion*>(p)->~hkpKeyframedRigidMotion();
}

static void finishLoadedObjecthkpKeyframedRigidMotion(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpKeyframedRigidMotion(kFlag);
    }
}

// As finishLoadedObjecthkpKeyframedRigidMotion, for hkpMaxSizeMotion.
static void finishLoadedObjecthkpMaxSizeMotion(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpMaxSizeMotion(kFlag);
    }
}

void cleanupLoadedObjecthkpMaxSizeMotion(void* p) {
    static_cast<hkpMaxSizeMotion*>(p)->~hkpMaxSizeMotion();
}

hkpMotion::hkpMotion(const hkVector4& position, const hkQuaternion& rotation, bool wantDeactivation)
  : m_deactivationIntegrateCounter(
        wantDeactivation ? static_cast<uint8_t>(static_cast<int32_t>(position.m_quad[0]) & 15) : uint8_t{0xFF}
    ),
    m_deactivationNumInactiveFrames{0, 0}, m_deactivationRefOrientation{0, 0} {
    m_linearVelocity.m_quad = _mm_setzero_ps();
    m_angularVelocity.m_quad = _mm_setzero_ps();
    m_motionState.initMotionState(position, rotation);
    m_motionState.m_linearDamping = 0.0f;
    m_motionState.m_angularDamping = 0.0f;
    m_type.m_storage = MOTION_INVALID;
    m_deactivationRefPosition[0].m_quad = _mm_setzero_ps();
    m_deactivationRefPosition[1].m_quad = _mm_setzero_ps();
    m_gravityFactor.m_value = std::bit_cast<int32_t>(1.0f) >> 16;
}

hkpMotion::~hkpMotion() {}

void hkpMotion::setMass(float mass) {
    float v2;
    if (mass == 0.0) {
        setMassInv((float)0.0);
    }
    else {
        v2 = (float)(1.0 / mass);
        setMassInv(v2);
    }
}

void hkpMotion::setMassInv(float massInv) {
    m_inertiaAndMassInv.m_quad[3] = massInv;
}

void hkpMotion::setCenterOfMassInLocal(const hkVector4& centerOfMass) {
    hkSweptTransformUtil::setCentreOfRotationLocal(centerOfMass, m_motionState);
}

void hkpMotion::setPosition(const hkVector4& position) {
    hkSweptTransformUtil::warpToPosition(position, m_motionState);
}

void hkpMotion::setRotation(const hkQuaternion& rotation) {
    hkSweptTransformUtil::warpToRotation(rotation, m_motionState);
}

void hkpMotion::setPositionAndRotation(const hkVector4& position, const hkQuaternion& rotation) {
    hkSweptTransformUtil::warpTo(position, rotation, m_motionState);
}

void hkpMotion::setTransform(const hkTransform& transform) {
    hkSweptTransformUtil::warpTo(transform, m_motionState);
}

void hkpMotion::setLinearVelocity(const hkVector4& assertBuf) {
    m_linearVelocity = assertBuf;
}

void hkpMotion::setAngularVelocity(const hkVector4& assertBuf) {
    m_angularVelocity = assertBuf;
}

void hkpMotion::applyLinearImpulse(const hkVector4& a1) {
    __m128 v2 = _mm_shuffle_ps(m_inertiaAndMassInv.m_quad, m_inertiaAndMassInv.m_quad, 255);
    m_linearVelocity.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v2, v2, 0), a1.m_quad), m_linearVelocity.m_quad);
}

void hkpMotion::getMotionStateAndVelocitiesAndDeactivationType(hkpMotion* a2) {
    (&a2->m_motionState)->Copy(&m_motionState);
    a2->m_linearVelocity = m_linearVelocity;
    a2->m_angularVelocity = m_angularVelocity;
    a2->m_deactivationIntegrateCounter = m_deactivationIntegrateCounter;
}

float hkpMotion::getMass() const {
    double v1 = 0.0;
    float v2 = _mm_shuffle_ps(m_inertiaAndMassInv.m_quad, m_inertiaAndMassInv.m_quad, 255)[0];
    if (v2 != 0.0f) {
        return 1.0 / v2;
    }
    return v1;
}

hkVector4* hkpMotion::getTranslation() {
    return &this->getMotionState()->m_transform.m_translation;
}

void hkpMotion::setDeactivationClass(int32_t deactivationClass) {
    m_motionState.m_deactivationClass = (uint8_t)deactivationClass;
}

void hkpMotion::approxTransformAt(float time, hkTransform& transformOut) {
    m_motionState.m_sweptTransform.approxTransformAt(time, transformOut);
}

hkpKeyframedRigidMotion::hkpKeyframedRigidMotion(const hkVector4& position, const hkQuaternion& rotation)
  : hkpMotion(position, rotation, false) {
    m_savedMotion = nullptr;
    m_savedQualityTypeIndex = 0;
    m_inertiaAndMassInv.m_quad = _mm_setzero_ps();
    m_type.m_storage = MOTION_KEYFRAMED;
}

// Releases the saved motion's reference; hkpMotion and the max size motion add nothing to the destructor.
hkpKeyframedRigidMotion::~hkpKeyframedRigidMotion() {
    if (m_savedMotion) {
        m_savedMotion->removeReference();
    }
}

void hkpKeyframedRigidMotion::setMass(float a1) {
    ;
}

void hkpKeyframedRigidMotion::setMassInv(float a1) {
    ;
}

void hkpKeyframedRigidMotion::getInertiaLocal(hkMatrix3& a1) const {
    a1.m_col0.m_quad = _mm_setzero_ps();
    a1.m_col1.m_quad = _mm_setzero_ps();
    a1.m_col2.m_quad = _mm_setzero_ps();
}

void hkpKeyframedRigidMotion::getInertiaWorld(hkMatrix3& inertia) const {
    inertia.m_col0.m_quad = _mm_setzero_ps();
    inertia.m_col1.m_quad = _mm_setzero_ps();
    inertia.m_col2.m_quad = _mm_setzero_ps();
}

void hkpKeyframedRigidMotion::setInertiaLocal(const hkMatrix3& a1) {
    ;
}

void hkpKeyframedRigidMotion::setInertiaInvLocal(const hkMatrix3& a1) {
    ;
}

void hkpKeyframedRigidMotion::getInertiaInvLocal(hkMatrix3& a1) const {
    a1.m_col0.m_quad = _mm_setzero_ps();
    a1.m_col1.m_quad = _mm_setzero_ps();
    a1.m_col2.m_quad = _mm_setzero_ps();
}

void hkpKeyframedRigidMotion::getInertiaInvWorld(hkMatrix3& a1) const {
    a1.m_col0.m_quad = _mm_setzero_ps();
    a1.m_col1.m_quad = _mm_setzero_ps();
    a1.m_col2.m_quad = _mm_setzero_ps();
}

void hkpKeyframedRigidMotion::getProjectedPointVelocity(
    const hkVector4& pos,
    const hkVector4& a2,
    float& a3,
    float& a4
) const {
    __m128 v5 = _mm_sub_ps(pos.m_quad, m_motionState.m_sweptTransform.m_centerOfMass1.m_quad);
    __m128 v6 = _mm_mul_ps(m_linearVelocity.m_quad, a2.m_quad);
    __m128 v7 = _mm_mul_ps(
        _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(a2.m_quad, a2.m_quad, 201), _mm_shuffle_ps(v5, v5, 210)),
            _mm_mul_ps(_mm_shuffle_ps(a2.m_quad, a2.m_quad, 210), _mm_shuffle_ps(v5, v5, 201))
        ),
        m_angularVelocity.m_quad
    );
    a3 = (_mm_shuffle_ps(v7, v7, 170)[0] + (_mm_shuffle_ps(v7, v7, 85)[0] + v7[0])) +
        (_mm_shuffle_ps(v6, v6, 170)[0] + (_mm_shuffle_ps(v6, v6, 85)[0] + v6[0]));
    a4 = 0.0f;
}

void hkpKeyframedRigidMotion::applyLinearImpulse(const hkVector4& a1) {
    ;
}

void hkpKeyframedRigidMotion::applyPointImpulse(const hkVector4& a1, const hkVector4& a2) {
    ;
}

void hkpKeyframedRigidMotion::applyAngularImpulse(const hkVector4& a1) {
    ;
}

void hkpKeyframedRigidMotion::applyForce(float a1, const hkVector4& a2) {
    ;
}

void hkpKeyframedRigidMotion::applyForce(float a1, const hkVector4& a2, const hkVector4& a3) {
    ;
}

void hkpKeyframedRigidMotion::applyTorque(float a1, const hkVector4& a2) {
    ;
}

void hkpKeyframedRigidMotion::setStepPosition(float a1, float a2) {
    ;
}

void hkpKeyframedRigidMotion::setStoredMotion(hkpMaxSizeMotion* a1) {
    if (a1) {
        a1->addReference();
    }
    hkpMaxSizeMotion* m_savedMotion = this->m_savedMotion;
    if (m_savedMotion) {
        m_savedMotion->removeReference();
    }
    this->m_savedMotion = a1;
}

const hkClass& hkpKeyframedRigidMotion::staticClass() {
    return hkpKeyframedRigidMotionClass;
}

hkpMaxSizeMotion::~hkpMaxSizeMotion() {}

const hkClass& hkpMaxSizeMotion::staticClass() {
    return hkpMaxSizeMotionClass;
}
