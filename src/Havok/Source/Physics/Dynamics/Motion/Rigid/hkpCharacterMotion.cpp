#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpCharacterMotion.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <xmmintrin.h>

static void finishLoadedObjecthkpCharacterMotion(void* p, int32_t finishing);
// Havok SDK signature (hkpDynamics.lib, tied by reloc,target).
static const void* getVtablehkpCharacterMotion();
static void cleanupLoadedObjecthkpCharacterMotion(void* p);

hkClass hkpCharacterMotionClass(
    "hkpCharacterMotion",
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
hkTypeInfo hkpCharacterMotionTypeInfo = {
    "hkpCharacterMotion",          finishLoadedObjecthkpCharacterMotion, cleanupLoadedObjecthkpCharacterMotion,
    getVtablehkpCharacterMotion(), &typeid(hkpCharacterMotion),          sizeof(hkpCharacterMotion)
};

static const void* getVtablehkpCharacterMotion() {
    return hkGetVtableOf<hkpCharacterMotion>();
}

static void cleanupLoadedObjecthkpCharacterMotion(void* p) {
    static_cast<hkpCharacterMotion*>(p)->~hkpCharacterMotion();
}

// The type info's finish-loaded-object callback for hkpCharacterMotion: runs its finish constructor.
static void finishLoadedObjecthkpCharacterMotion(void* p, int32_t finishing) {
    if (p) {
        ::new (p) hkpCharacterMotion(finishing);
    }
}

hkpCharacterMotion::hkpCharacterMotion(const hkVector4& position, const hkQuaternion& rotation)
  : hkpMotion(position, rotation, false) {
    m_type.m_storage = MOTION_CHARACTER;
}

hkpCharacterMotion::hkpCharacterMotion(int32_t a1) : hkpMotion(hkFinishLoadedObjectFlag{a1}) {}

void hkpCharacterMotion::getInertiaLocal(hkMatrix3& a1) const {
    double v2 = 1.0 / XFNV_M128_F32(m_inertiaAndMassInv.m_quad, 0);
    a1.m_col0.m_quad = _mm_setzero_ps();
    a1.m_col1.m_quad = _mm_setzero_ps();
    a1.m_col2.m_quad = _mm_setzero_ps();
    float v3 = v2;
    XFNV_M128_F32(a1.m_col0.m_quad, 0) = v3;
    XFNV_M128_F32(a1.m_col1.m_quad, 1) = v3;
    XFNV_M128_F32(a1.m_col2.m_quad, 2) = v3;
}

void hkpCharacterMotion::getInertiaWorld(hkMatrix3& a1) const {
    double v2 = 1.0 / XFNV_M128_F32(m_inertiaAndMassInv.m_quad, 0);
    a1.m_col0.m_quad = _mm_setzero_ps();
    a1.m_col1.m_quad = _mm_setzero_ps();
    a1.m_col2.m_quad = _mm_setzero_ps();
    float v3 = v2;
    XFNV_M128_F32(a1.m_col0.m_quad, 0) = v3;
    XFNV_M128_F32(a1.m_col1.m_quad, 1) = v3;
    XFNV_M128_F32(a1.m_col2.m_quad, 2) = v3;
}

void hkpCharacterMotion::setInertiaLocal(const hkMatrix3& a2) {
    float v6 = XFNV_M128_F32((&a2)->m_col1.m_quad, 1);
    double v3 = (double)XFNV_M128_F32((&a2)->m_col0.m_quad, 0);
    if (!(v6 < v3)) {
        v3 = (double)v6;
    }
    float v8 = (float)v3;
    float v7 = XFNV_M128_F32((&a2)->m_col2.m_quad, 2);
    double v4 = (double)v8;
    if (!(v7 < ((double)v8))) {
        v4 = (double)v7;
    }
    float v9 = (float)v4;
    double v5 = 0.0;
    if (v9 > 0.0) {
        v5 = 1.0 / v9;
    }
    float v10 = (float)v5;
    XFNV_M128_F32(m_inertiaAndMassInv.m_quad, 0) = v10;
    XFNV_M128_F32(m_inertiaAndMassInv.m_quad, 1) = v10;
    XFNV_M128_F32(m_inertiaAndMassInv.m_quad, 2) = v10;
}

void hkpCharacterMotion::setInertiaInvLocal(const hkMatrix3& a2) {
    float v5 = XFNV_M128_F32((&a2)->m_col1.m_quad, 1);
    double v3 = (double)XFNV_M128_F32((&a2)->m_col0.m_quad, 0);
    if (!(v5 > v3)) {
        v3 = (double)v5;
    }
    float v7 = (float)v3;
    float v6 = XFNV_M128_F32((&a2)->m_col2.m_quad, 2);
    double v4 = (double)v7;
    if (!(v6 > ((double)v7))) {
        v4 = (double)v6;
    }
    float v8 = (float)v4;
    XFNV_M128_F32(m_inertiaAndMassInv.m_quad, 0) = v8;
    XFNV_M128_F32(m_inertiaAndMassInv.m_quad, 1) = v8;
    XFNV_M128_F32(m_inertiaAndMassInv.m_quad, 2) = v8;
}

void hkpCharacterMotion::getInertiaInvLocal(hkMatrix3& a1) const {
    double v2 = XFNV_M128_F32(m_inertiaAndMassInv.m_quad, 0);
    a1.m_col0.m_quad = _mm_setzero_ps();
    a1.m_col1.m_quad = _mm_setzero_ps();
    a1.m_col2.m_quad = _mm_setzero_ps();
    XFNV_M128_F32(a1.m_col0.m_quad, 0) = v2;
    XFNV_M128_F32(a1.m_col1.m_quad, 1) = v2;
    XFNV_M128_F32(a1.m_col2.m_quad, 2) = v2;
}

void hkpCharacterMotion::getInertiaInvWorld(hkMatrix3& a1) const {
    double v2 = XFNV_M128_F32(m_inertiaAndMassInv.m_quad, 0);
    a1.m_col0.m_quad = _mm_setzero_ps();
    a1.m_col1.m_quad = _mm_setzero_ps();
    a1.m_col2.m_quad = _mm_setzero_ps();
    XFNV_M128_F32(a1.m_col0.m_quad, 0) = v2;
    XFNV_M128_F32(a1.m_col1.m_quad, 1) = v2;
    XFNV_M128_F32(a1.m_col2.m_quad, 2) = v2;
}

void hkpCharacterMotion::getProjectedPointVelocity(
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
    a3 = (XFNV_M128_F32(_mm_shuffle_ps(v7, v7, 170), 0) +
          (XFNV_M128_F32(_mm_shuffle_ps(v7, v7, 85), 0) + XFNV_M128_F32(v7, 0))) +
        (XFNV_M128_F32(_mm_shuffle_ps(v6, v6, 170), 0) +
         (XFNV_M128_F32(_mm_shuffle_ps(v6, v6, 85), 0) + XFNV_M128_F32(v6, 0)));
    a4 = XFNV_M128_F32(m_inertiaAndMassInv.m_quad, 3);
}

void hkpCharacterMotion::applyPointImpulse(const hkVector4& a1, const hkVector4& a2) {
    __m128 v3 = _mm_shuffle_ps(m_inertiaAndMassInv.m_quad, m_inertiaAndMassInv.m_quad, 255);
    m_linearVelocity.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v3, v3, 0), a1.m_quad), m_linearVelocity.m_quad);
    __m128 v4 = _mm_sub_ps(a2.m_quad, m_motionState.m_sweptTransform.m_centerOfMass1.m_quad);
    m_angularVelocity.m_quad = _mm_add_ps(
        _mm_mul_ps(
            _mm_sub_ps(
                _mm_mul_ps(_mm_shuffle_ps(a1.m_quad, a1.m_quad, 210), _mm_shuffle_ps(v4, v4, 201)),
                _mm_mul_ps(_mm_shuffle_ps(a1.m_quad, a1.m_quad, 201), _mm_shuffle_ps(v4, v4, 210))
            ),
            m_inertiaAndMassInv.m_quad
        ),
        m_angularVelocity.m_quad
    );
}

void hkpCharacterMotion::applyAngularImpulse(const hkVector4& a1) {
    m_angularVelocity.m_quad = _mm_add_ps(_mm_mul_ps(m_inertiaAndMassInv.m_quad, a1.m_quad), m_angularVelocity.m_quad);
}

void hkpCharacterMotion::applyForce(float a1, const hkVector4& a2) {
    __m128 v3 = _mm_shuffle_ps(m_inertiaAndMassInv.m_quad, m_inertiaAndMassInv.m_quad, 255);
    m_linearVelocity.m_quad = _mm_add_ps(
        _mm_mul_ps(_mm_shuffle_ps(v3, v3, 0), _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(a1), _mm_set_ss(a1), 0), a2.m_quad)),
        m_linearVelocity.m_quad
    );
}

void hkpCharacterMotion::applyForce(float deltaTime, const hkVector4& force, const hkVector4& p) {
    hkVector4 v5;
    v5.m_quad = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(deltaTime), _mm_set_ss(deltaTime), 0), force.m_quad);
    this->applyPointImpulse(v5, p);
}

void hkpCharacterMotion::applyTorque(float deltaTime, const hkVector4& torque) {
    hkVector4 v4;
    v4.m_quad = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(deltaTime), _mm_set_ss(deltaTime), 0), torque.m_quad);
    this->applyAngularImpulse(v4);
}

const hkClass& hkpCharacterMotion::staticClass() {
    return hkpCharacterMotionClass;
}
