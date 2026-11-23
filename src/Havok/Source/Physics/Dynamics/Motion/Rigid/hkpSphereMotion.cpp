#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpSphereMotion.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnTrack.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpPointToPathConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Malleable/hkpMalleableConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Position/hkpPositionConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/SpringDamper/hkpSpringDamperConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Velocity/hkpVelocityConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/ThinBoxMotion/hkpThinBoxMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Internal/Collide/ConvexPieceMesh/hkpConvexPieceStreamData.h"
#include "Havok/Source/Physics/Utilities/Actions/AngularDashpot/hkpAngularDashpotAction.h"
#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"
#include "Havok/Source/Physics/Utilities/Deprecated/DisableEntity/hkpDisableEntityCollisionFilter.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpDisplayBindingData.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <xmmintrin.h>

hkClass hkpSphereMotionClass(
    "hkpSphereMotion",
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
hkTypeInfo hkpSphereMotionTypeInfo = {
    "hkpSphereMotion",
    finishLoadedObjecthkpSphereMotion,
    cleanupLoadedObjecthkpSphereMotion,
    getVtablehkpSphereMotion(),
    &typeid(hkpSphereMotion),
    sizeof(hkpSphereMotion)
};

void cleanupLoadedObjecthkpSphereMotion(void* p) {
    static_cast<hkpSphereMotion*>(p)->~hkpSphereMotion();
}

// The type info's finish-loaded-object callback for hkpSphereMotion: runs its finish constructor.
void finishLoadedObjecthkpSphereMotion(void* p, int32_t finishing) {
    if (p) {
        ::new (p) hkpSphereMotion(finishing);
    }
}

const void* getVtablehkpSphereMotion() {
    return hkGetVtableOf<hkpSphereMotion>();
}

hkpSphereMotion::hkpSphereMotion(const hkVector4& position, const hkQuaternion& rotation)
  : hkpMotion(position, rotation, false) {
    m_type.m_storage = MOTION_SPHERE_INERTIA;
}

hkpSphereMotion::hkpSphereMotion(int32_t finishing) : hkpMotion(hkFinishLoadedObjectFlag{finishing}) {}

hkpSphereMotion::~hkpSphereMotion() {}

void hkpSphereMotion::getInertiaLocal(hkMatrix3& inertia) const {
    double v2 = XFNV_M128_F32(m_inertiaAndMassInv.m_quad, 0);
    if (v2 < 1.1920928955078125e-07) {
        v2 = HK_REAL_EPSILON;
    }
    float v3 = v2;
    inertia.m_col0.m_quad = _mm_setzero_ps();
    inertia.m_col1.m_quad = _mm_setzero_ps();
    inertia.m_col2.m_quad = _mm_setzero_ps();
    float v4 = 1.0 / v3;
    XFNV_M128_F32(inertia.m_col0.m_quad, 0) = v4;
    XFNV_M128_F32(inertia.m_col1.m_quad, 1) = v4;
    XFNV_M128_F32(inertia.m_col2.m_quad, 2) = v4;
}

void hkpSphereMotion::getInertiaWorld(hkMatrix3& inertia) const {
    double v2 = XFNV_M128_F32(m_inertiaAndMassInv.m_quad, 0);
    if (v2 < 1.1920928955078125e-07) {
        v2 = HK_REAL_EPSILON;
    }
    float v3 = v2;
    inertia.m_col0.m_quad = _mm_setzero_ps();
    inertia.m_col1.m_quad = _mm_setzero_ps();
    inertia.m_col2.m_quad = _mm_setzero_ps();
    float v4 = 1.0 / v3;
    XFNV_M128_F32(inertia.m_col0.m_quad, 0) = v4;
    XFNV_M128_F32(inertia.m_col1.m_quad, 1) = v4;
    XFNV_M128_F32(inertia.m_col2.m_quad, 2) = v4;
}

// The largest diagonal element, inverted (0 when it is not positive). Each step is rounded to a float. The later
// element is kept unless it is below.
void hkpSphereMotion::setInertiaLocal(const hkMatrix3& inertia) {
    float fMax = XFNV_M128_F32(inertia.m_col1.m_quad, 1) < XFNV_M128_F32(inertia.m_col0.m_quad, 0)
        ? XFNV_M128_F32(inertia.m_col0.m_quad, 0)
        : XFNV_M128_F32(inertia.m_col1.m_quad, 1);
    fMax = XFNV_M128_F32(inertia.m_col2.m_quad, 2) < fMax ? fMax : XFNV_M128_F32(inertia.m_col2.m_quad, 2);
    float fInv = fMax > 0.0f ? 1.0f / fMax : 0.0f;
    XFNV_M128_F32(m_inertiaAndMassInv.m_quad, 0) = fInv;
    XFNV_M128_F32(m_inertiaAndMassInv.m_quad, 1) = fInv;
    XFNV_M128_F32(m_inertiaAndMassInv.m_quad, 2) = fInv;
}

// The smallest diagonal element of the inverse inertia. Each compare keeps the later element unless it is above the
// current minimum.
void hkpSphereMotion::setInertiaInvLocal(const hkMatrix3& inertiaInv) {
    float fMin = XFNV_M128_F32(inertiaInv.m_col1.m_quad, 1) > XFNV_M128_F32(inertiaInv.m_col0.m_quad, 0)
        ? XFNV_M128_F32(inertiaInv.m_col0.m_quad, 0)
        : XFNV_M128_F32(inertiaInv.m_col1.m_quad, 1);
    fMin = XFNV_M128_F32(inertiaInv.m_col2.m_quad, 2) > fMin ? fMin : XFNV_M128_F32(inertiaInv.m_col2.m_quad, 2);
    XFNV_M128_F32(m_inertiaAndMassInv.m_quad, 0) = fMin;
    XFNV_M128_F32(m_inertiaAndMassInv.m_quad, 1) = fMin;
    XFNV_M128_F32(m_inertiaAndMassInv.m_quad, 2) = fMin;
}

void hkpSphereMotion::getInertiaInvLocal(hkMatrix3& inertiaInv) const {
    double v2 = XFNV_M128_F32(m_inertiaAndMassInv.m_quad, 0);
    inertiaInv.m_col0.m_quad = _mm_setzero_ps();
    inertiaInv.m_col1.m_quad = _mm_setzero_ps();
    inertiaInv.m_col2.m_quad = _mm_setzero_ps();
    XFNV_M128_F32(inertiaInv.m_col0.m_quad, 0) = v2;
    XFNV_M128_F32(inertiaInv.m_col1.m_quad, 1) = v2;
    XFNV_M128_F32(inertiaInv.m_col2.m_quad, 2) = v2;
}

void hkpSphereMotion::getInertiaInvWorld(hkMatrix3& inertiaInv) const {
    double v2 = XFNV_M128_F32(m_inertiaAndMassInv.m_quad, 0);
    inertiaInv.m_col0.m_quad = _mm_setzero_ps();
    inertiaInv.m_col1.m_quad = _mm_setzero_ps();
    inertiaInv.m_col2.m_quad = _mm_setzero_ps();
    XFNV_M128_F32(inertiaInv.m_col0.m_quad, 0) = v2;
    XFNV_M128_F32(inertiaInv.m_col1.m_quad, 1) = v2;
    XFNV_M128_F32(inertiaInv.m_col2.m_quad, 2) = v2;
}

void hkpSphereMotion::getProjectedPointVelocity(
    const hkVector4& pos,
    const hkVector4& normal,
    float& velOut,
    float& invVirtMassOut
) const {
    __m128 v5 = _mm_sub_ps(pos.m_quad, m_motionState.m_sweptTransform.m_centerOfMass1.m_quad);
    __m128 v6 = _mm_mul_ps(_mm_shuffle_ps(normal.m_quad, normal.m_quad, 201), _mm_shuffle_ps(v5, v5, 210));
    __m128 v7 = _mm_mul_ps(_mm_shuffle_ps(normal.m_quad, normal.m_quad, 210), _mm_shuffle_ps(v5, v5, 201));
    __m128 v8 = _mm_mul_ps(m_linearVelocity.m_quad, normal.m_quad);
    __m128 v9 = _mm_sub_ps(v6, v7);
    __m128 v10 = _mm_mul_ps(m_angularVelocity.m_quad, v9);
    __m128 v11 = _mm_mul_ps(v9, v9);
    velOut = (XFNV_M128_F32(_mm_shuffle_ps(v10, v10, 170), 0) +
              (XFNV_M128_F32(_mm_shuffle_ps(v10, v10, 85), 0) + XFNV_M128_F32(v10, 0))) +
        (XFNV_M128_F32(_mm_shuffle_ps(v8, v8, 170), 0) +
         (XFNV_M128_F32(_mm_shuffle_ps(v8, v8, 85), 0) + XFNV_M128_F32(v8, 0)));
    invVirtMassOut = XFNV_M128_F32(m_inertiaAndMassInv.m_quad, 0) *
            (XFNV_M128_F32(_mm_shuffle_ps(v11, v11, 170), 0) +
             (XFNV_M128_F32(_mm_shuffle_ps(v11, v11, 85), 0) + XFNV_M128_F32(v11, 0))) +
        XFNV_M128_F32(m_inertiaAndMassInv.m_quad, 3);
}

void hkpSphereMotion::applyPointImpulse(const hkVector4& imp, const hkVector4& p) {
    __m128 v3 = _mm_shuffle_ps(m_inertiaAndMassInv.m_quad, m_inertiaAndMassInv.m_quad, 255);
    m_linearVelocity.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v3, v3, 0), imp.m_quad), m_linearVelocity.m_quad);
    __m128 v4 = _mm_sub_ps(p.m_quad, m_motionState.m_sweptTransform.m_centerOfMass1.m_quad);
    m_angularVelocity.m_quad = _mm_add_ps(
        _mm_mul_ps(
            _mm_sub_ps(
                _mm_mul_ps(_mm_shuffle_ps(imp.m_quad, imp.m_quad, 210), _mm_shuffle_ps(v4, v4, 201)),
                _mm_mul_ps(_mm_shuffle_ps(imp.m_quad, imp.m_quad, 201), _mm_shuffle_ps(v4, v4, 210))
            ),
            m_inertiaAndMassInv.m_quad
        ),
        m_angularVelocity.m_quad
    );
}

void hkpSphereMotion::applyAngularImpulse(const hkVector4& imp) {
    m_angularVelocity.m_quad = _mm_add_ps(_mm_mul_ps(m_inertiaAndMassInv.m_quad, imp.m_quad), m_angularVelocity.m_quad);
}

void hkpSphereMotion::applyForce(float deltaTime, const hkVector4& force, const hkVector4& p) {
    hkVector4 v5;
    v5.m_quad = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(deltaTime), _mm_set_ss(deltaTime), 0), force.m_quad);
    this->applyPointImpulse(v5, p);
}

void hkpSphereMotion::applyForce(float deltaTime, const hkVector4& force) {
    __m128 v3 = _mm_shuffle_ps(m_inertiaAndMassInv.m_quad, m_inertiaAndMassInv.m_quad, 255);
    m_linearVelocity.m_quad = _mm_add_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(v3, v3, 0),
            _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(deltaTime), _mm_set_ss(deltaTime), 0), force.m_quad)
        ),
        m_linearVelocity.m_quad
    );
}

void hkpSphereMotion::applyTorque(float deltaTime, const hkVector4& torque) {
    hkVector4 v4;
    v4.m_quad = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(deltaTime), _mm_set_ss(deltaTime), 0), torque.m_quad);
    this->applyAngularImpulse(v4);
}

const hkClass& hkpSphereMotion::staticClass() {
    return hkpSphereMotionClass;
}
