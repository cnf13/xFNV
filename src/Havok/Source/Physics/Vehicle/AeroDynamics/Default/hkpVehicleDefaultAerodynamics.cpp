#include "Havok/Source/Physics/Vehicle/AeroDynamics/Default/hkpVehicleDefaultAerodynamics.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpUnaryAction.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Vehicle/AeroDynamics/hkpVehicleAerodynamics.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleData.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleInstance.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

static void finishLoadedObjecthkpVehicleDefaultAerodynamics(void* p, int32_t finishing);
// Havok SDK signature (hkpVehicle.lib, tied by reloc,target).
static const void* getVtablehkpVehicleDefaultAerodynamics();
static void cleanupLoadedObjecthkpVehicleDefaultAerodynamics(void* p);

// The reflected members of hkpVehicleDefaultAerodynamics.
using hkpVehicleDefaultAerodynamicsClass_MembersArray = const hkClassMember[5];
static hkpVehicleDefaultAerodynamicsClass_MembersArray hkpVehicleDefaultAerodynamicsClass_Members{
    {"airDensity", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x8, nullptr},
    {"frontalArea", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xC, nullptr},
    {"dragCoefficient",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x10,
     nullptr},
    {"liftCoefficient",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x14,
     nullptr},
    {"extraGravityws",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x20,
     nullptr}
};
hkClass hkpVehicleDefaultAerodynamicsClass(
    "hkpVehicleDefaultAerodynamics",
    &hkpVehicleAerodynamicsClass,
    48,
    nullptr,
    0,
    nullptr,
    0,
    hkpVehicleDefaultAerodynamicsClass_Members,
    5,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpVehicleDefaultAerodynamicsTypeInfo = {
    "hkpVehicleDefaultAerodynamics",
    finishLoadedObjecthkpVehicleDefaultAerodynamics,
    cleanupLoadedObjecthkpVehicleDefaultAerodynamics,
    getVtablehkpVehicleDefaultAerodynamics(),
    &typeid(hkpVehicleDefaultAerodynamics),
    sizeof(hkpVehicleDefaultAerodynamics)
};

// The type info's finish-loaded-object callback for hkpVehicleDefaultAerodynamics: the null check, then its finish
// constructor.
static void finishLoadedObjecthkpVehicleDefaultAerodynamics(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpVehicleDefaultAerodynamics(kFlag);
    }
}

static const void* getVtablehkpVehicleDefaultAerodynamics() {
    return hkGetVtableOf<hkpVehicleDefaultAerodynamics>();
}

static void cleanupLoadedObjecthkpVehicleDefaultAerodynamics(void* p) {
    static_cast<hkpVehicleDefaultAerodynamics*>(p)->~hkpVehicleDefaultAerodynamics();
}

hkpVehicleDefaultAerodynamics::hkpVehicleDefaultAerodynamics()
  : m_airDensity(0.0f), m_frontalArea(0.0f), m_dragCoefficient(0.0f), m_liftCoefficient(0.0f) {
    m_extraGravityws.setZero4();
}

hkpVehicleDefaultAerodynamics::~hkpVehicleDefaultAerodynamics() {}

void hkpVehicleDefaultAerodynamics::calcAerodynamics(
    const float deltaTime,
    const hkpVehicleInstance* vehicle,
    hkpVehicleAerodynamics::AerodynamicsDragOutput& dragInfoOut
) {
    hkVector4 v18;
    uint32_t v20;
    uint32_t v21;
    hkpEntity* m_entity = vehicle->m_entity;
    hkpVehicleData* m_data = vehicle->m_data;
    __m128 m_quad = m_data->m_chassisOrientation.m_col1.m_quad;
    __m128 v8 = m_entity->m_motion.m_motionState.m_transform.m_rotation.m_col0.m_quad;
    __m128 v9 = m_entity->m_motion.m_motionState.m_transform.m_rotation.m_col1.m_quad;
    __m128 v10 = m_entity->m_motion.m_motionState.m_transform.m_rotation.m_col2.m_quad;
    __m128 v11 = _mm_add_ps(
        _mm_mul_ps(_mm_shuffle_ps(m_quad, m_quad, 0), v8), _mm_mul_ps(_mm_shuffle_ps(m_quad, m_quad, 85), v9)
    );
    __m128 v12 = _mm_shuffle_ps(m_quad, m_quad, 170);
    __m128 v13 = m_data->m_chassisOrientation.m_col0.m_quad;
    __m128 v14 = _mm_add_ps(v11, _mm_mul_ps(v12, v10));
    __m128 v23 = _mm_mul_ps(_mm_shuffle_ps(v13, v13, 170), v10);
    __m128 v15 = _mm_mul_ps(_mm_shuffle_ps(v13, v13, 0), v8);
    __m128 v16 = _mm_shuffle_ps(v13, v13, 85);
    __m128 v17 = _mm_mul_ps(m_entity->m_motion.m_linearVelocity.m_quad, v14);
    float v19 = XFNV_M128_F32(_mm_shuffle_ps(v17, v17, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v17, v17, 85), 0) + XFNV_M128_F32(v17, 0));
    *reinterpret_cast<float*>(&v21) = calcAerodynamicDrag(v19);
    *reinterpret_cast<float*>(&v20) = calcAerodynamicLift(v19);
    v18.m_quad = _mm_mul_ps(
        _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v21)), _mm_set_ss(std::bit_cast<float>(v21)), 0), v14
    );
    dragInfoOut.m_aerodynamicsForce.m_quad = v18.m_quad;
    dragInfoOut.m_aerodynamicsForce.m_quad = _mm_add_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v20)), _mm_set_ss(std::bit_cast<float>(v20)), 0),
            _mm_add_ps(_mm_add_ps(v15, _mm_mul_ps(v16, v9)), v23)
        ),
        v18.m_quad
    );
    float Mass = m_entity->m_motion.getMass();
    dragInfoOut.m_aerodynamicsForce.m_quad = _mm_add_ps(
        _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(Mass), _mm_set_ss(Mass), 0), m_extraGravityws.m_quad),
        dragInfoOut.m_aerodynamicsForce.m_quad
    );
    dragInfoOut.m_aerodynamicsTorque.m_quad = _mm_setzero_ps();
}

// The drag opposes the motion and grows with the square of the speed. The speed is rounded to float, the product is
// kept in double and then rounded to float.
float hkpVehicleDefaultAerodynamics::calcAerodynamicDrag(const float forwardVelocity) const {
    float fSpeed = std::fabsf(forwardVelocity);
    float fDrag = m_airDensity * -0.5 * m_dragCoefficient * m_frontalArea * fSpeed * forwardVelocity;
    return fDrag;
}

// The lift grows with the square of the speed; the product is kept in double and rounded to float.
float hkpVehicleDefaultAerodynamics::calcAerodynamicLift(const float forwardVelocity) const {
    float fLift = m_airDensity * 0.5 * m_liftCoefficient * m_frontalArea * forwardVelocity * forwardVelocity;
    return fLift;
}

const hkClass& hkpVehicleDefaultAerodynamics::staticClass() {
    return hkpVehicleDefaultAerodynamicsClass;
}
