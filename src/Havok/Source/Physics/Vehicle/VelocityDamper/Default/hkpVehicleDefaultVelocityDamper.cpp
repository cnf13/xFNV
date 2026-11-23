#include "Havok/Source/Physics/Vehicle/VelocityDamper/Default/hkpVehicleDefaultVelocityDamper.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpUnaryAction.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleInstance.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <xmmintrin.h>

static void finishLoadedObjecthkpVehicleDefaultVelocityDamper(void* p, int32_t finishing);
// Havok SDK signature (hkpVehicle.lib, tied by reloc,target).
static const void* getVtablehkpVehicleDefaultVelocityDamper();
static void cleanupLoadedObjecthkpVehicleDefaultVelocityDamper(void* p);

// The reflected members of hkpVehicleDefaultVelocityDamper.
using hkpVehicleDefaultVelocityDamperClass_MembersArray = const hkClassMember[3];
static hkpVehicleDefaultVelocityDamperClass_MembersArray hkpVehicleDefaultVelocityDamperClass_Members{
    {"normalSpinDamping",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x8,
     nullptr},
    {"collisionSpinDamping",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xC,
     nullptr},
    {"collisionThreshold",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x10,
     nullptr}
};
hkClass hkpVehicleDefaultVelocityDamperClass(
    "hkpVehicleDefaultVelocityDamper",
    &hkpVehicleVelocityDamperClass,
    20,
    nullptr,
    0,
    nullptr,
    0,
    hkpVehicleDefaultVelocityDamperClass_Members,
    3,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpVehicleDefaultVelocityDamperTypeInfo = {
    "hkpVehicleDefaultVelocityDamper",
    finishLoadedObjecthkpVehicleDefaultVelocityDamper,
    cleanupLoadedObjecthkpVehicleDefaultVelocityDamper,
    getVtablehkpVehicleDefaultVelocityDamper(),
    &typeid(hkpVehicleDefaultVelocityDamper),
    sizeof(hkpVehicleDefaultVelocityDamper)
};

// The type info's finish-loaded-object callback for hkpVehicleDefaultVelocityDamper: the null check, then its finish
// constructor.
static void finishLoadedObjecthkpVehicleDefaultVelocityDamper(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpVehicleDefaultVelocityDamper(kFlag);
    }
}

static const void* getVtablehkpVehicleDefaultVelocityDamper() {
    return hkGetVtableOf<hkpVehicleDefaultVelocityDamper>();
}

static void cleanupLoadedObjecthkpVehicleDefaultVelocityDamper(void* p) {
    static_cast<hkpVehicleDefaultVelocityDamper*>(p)->~hkpVehicleDefaultVelocityDamper();
}

hkpVehicleDefaultVelocityDamper::hkpVehicleDefaultVelocityDamper()
  : m_normalSpinDamping(0.0f), m_collisionSpinDamping(0.0f), m_collisionThreshold(1.0f) {}

// The destructor is empty because this class owns nothing of its own to release.
hkpVehicleDefaultVelocityDamper::~hkpVehicleDefaultVelocityDamper() {}

void hkpVehicleDefaultVelocityDamper::applyVelocityDamping(float deltaTime, hkpVehicleInstance& vehicle) {
    double m_normalSpinDamping;
    uint32_t v11;
    hkVector4 arVelocity;
    hkpRigidBody* m_entity = static_cast<hkpRigidBody*>(vehicle.m_entity);
    double m_collisionThreshold = this->m_collisionThreshold;
    arVelocity.m_quad = m_entity->m_motion.m_angularVelocity.m_quad;
    __m128 v6 = _mm_mul_ps(arVelocity.m_quad, arVelocity.m_quad);
    float v9 = m_collisionThreshold * m_collisionThreshold;
    if (!(v9 < static_cast<double>(
                   (XFNV_M128_F32(_mm_shuffle_ps(v6, v6, 170), 0) +
                    (XFNV_M128_F32(_mm_shuffle_ps(v6, v6, 85), 0) + XFNV_M128_F32(v6, 0)))
               ))) {
        m_normalSpinDamping = this->m_normalSpinDamping;
    }
    else {
        m_normalSpinDamping = m_collisionSpinDamping;
    }
    float v10 = 1.0 - m_normalSpinDamping * deltaTime;
    double v8 = v10;
    if (v10 < 0.0) {
        v8 = 0.0;
    }
    *reinterpret_cast<float*>(&v11) = v8;
    arVelocity.m_quad = _mm_mul_ps(
        _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v11)), _mm_set_ss(std::bit_cast<float>(v11)), 0),
        arVelocity.m_quad
    );
    m_entity->setAngularVelocity(arVelocity);
}

const hkClass& hkpVehicleDefaultVelocityDamper::staticClass() {
    return hkpVehicleDefaultVelocityDamperClass;
}
