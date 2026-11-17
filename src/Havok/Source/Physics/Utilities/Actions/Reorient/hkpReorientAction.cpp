#include "Havok/Source/Physics/Utilities/Actions/Reorient/hkpReorientAction.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpUnaryAction.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"

#include <cmath>
#include <xmmintrin.h>

// The member table of hkpReorientAction's class record, as the Havok SDK's class file has it.
using hkpReorientActionClass_MembersArray = const hkClassMember[4];
static hkpReorientActionClass_MembersArray hkpReorientActionClass_Members{
    {"rotationAxis",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x20,
     nullptr},
    {"upAxis", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x30, nullptr},
    {"strength", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x40, nullptr},
    {"damping", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x44, nullptr}
};

hkClass hkpReorientActionClass(
    "hkpReorientAction",
    &hkpUnaryActionClass,
    80,
    nullptr,
    0,
    nullptr,
    0,
    hkpReorientActionClass_Members,
    4,
    nullptr,
    nullptr,
    0,
    0
);

hkTypeInfo hkpReorientActionTypeInfo = {
    "hkpReorientAction",          finishLoadedObjecthkpReorientAction, cleanupLoadedObjecthkpReorientAction,
    getVtablehkpReorientAction(), &typeid(hkpReorientAction),          sizeof(hkpReorientAction)
};

hkpReorientAction::hkpReorientAction() : hkpUnaryAction(nullptr, 0), m_strength(0.0f), m_damping(0.0f) {
    m_rotationAxis.setZero4();
    m_upAxis.setZero4();
}

hkpReorientAction::hkpReorientAction(
    hkpRigidBody* body,
    const hkVector4& rotationAxis,
    const hkVector4& upAxis,
    float strength,
    float damping
)
  : hkpUnaryAction(body, 0), m_rotationAxis(rotationAxis), m_upAxis(upAxis), m_strength(strength), m_damping(damping) {}

// Turns the entity so that its up axis, seen in the plane normal to its rotation axis, points along the action's up
// axis: the angle between the two (signed by the cross product's major axis against the rotation axis's), times
// m_strength, less m_damping times the angular velocity, both per second, is applied as an angular impulse through the
// motion. A zero-length vector normalizes to zero. If |cos| is below 1.0f or NaN the angle is acos of it; otherwise it
// is 0 for a positive cosine and pi (3.1415927f) otherwise. When finding the major axes, only a strictly greater
// magnitude moves the index.
void hkpReorientAction::applyAction(const hkStepInfo& stepInfo) {
    hkpEntity* pEntity = m_entity;
    const hkQuaternion& kRotation = pEntity->m_motion.m_motionState.m_sweptTransform.m_rotation1;
    const __m128 kQ = _mm_load_ps(&reinterpret_cast<const float*>(&kRotation.m_vec.m_quad)[0]);
    const __m128 kQw = _mm_shuffle_ps(kQ, kQ, 0xFF);
    // The quaternion's w squared less 0.5.
    const __m128 kMinusHalf = _mm_set1_ps(-0.5f);
    const __m128 kTarget = _mm_load_ps(&reinterpret_cast<float*>(&m_upAxis.m_quad)[0]);

    // EXE-INLINED 0x630BB0
    __m128 kUp;
    {
        const __m128 p = _mm_mul_ps(kTarget, kQ);
        const __m128 d = _mm_add_ss(_mm_shuffle_ps(p, p, 0xAA), _mm_add_ss(_mm_shuffle_ps(p, p, 0x55), p));
        const __m128 c = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(kTarget, kTarget, 0xD2), _mm_shuffle_ps(kQ, kQ, 0xC9)),
            _mm_mul_ps(_mm_shuffle_ps(kTarget, kTarget, 0xC9), _mm_shuffle_ps(kQ, kQ, 0xD2))
        );
        const __m128 s = _mm_add_ps(
            _mm_mul_ps(_mm_add_ps(_mm_mul_ps(kQw, kQw), kMinusHalf), kTarget),
            _mm_mul_ps(_mm_shuffle_ps(d, d, 0x00), kQ)
        );
        const __m128 r = _mm_add_ps(_mm_mul_ps(c, kQw), s);
        kUp = _mm_add_ps(r, r);
    }
    // EXE-INLINED 0x630BB0
    __m128 kAxis;
    {
        const __m128 v = _mm_load_ps(&reinterpret_cast<float*>(&m_rotationAxis.m_quad)[0]);
        const __m128 p = _mm_mul_ps(v, kQ);
        const __m128 d = _mm_add_ss(_mm_shuffle_ps(p, p, 0xAA), _mm_add_ss(_mm_shuffle_ps(p, p, 0x55), p));
        const __m128 c = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(v, v, 0xD2), _mm_shuffle_ps(kQ, kQ, 0xC9)),
            _mm_mul_ps(_mm_shuffle_ps(v, v, 0xC9), _mm_shuffle_ps(kQ, kQ, 0xD2))
        );
        const __m128 s = _mm_add_ps(
            _mm_mul_ps(_mm_add_ps(_mm_mul_ps(kQw, kQw), kMinusHalf), v), _mm_mul_ps(_mm_shuffle_ps(d, d, 0x00), kQ)
        );
        const __m128 r = _mm_add_ps(_mm_mul_ps(c, kQw), s);
        kAxis = _mm_add_ps(r, r);
    }

    // The action's up axis without its component along the rotated axis, normalized.
    const __m128 kAlong = _mm_mul_ps(kAxis, kTarget);
    const float fAlong = _mm_cvtss_f32(
        _mm_add_ss(_mm_shuffle_ps(kAlong, kAlong, 0xAA), _mm_add_ss(_mm_shuffle_ps(kAlong, kAlong, 0x55), kAlong))
    );
    const float fNegAlong = -fAlong;
    const __m128 kProjected = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(fNegAlong), kAxis), kTarget);
    // EXE-INLINED none: hkVector4::normalize3
    const __m128 kSq = _mm_mul_ps(kProjected, kProjected);
    const __m128 kLengthSq =
        _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq));
    const __m128 kRsqrt = _mm_rsqrt_ss(kLengthSq);
    const __m128 kError = _mm_mul_ss(_mm_mul_ss(kLengthSq, kRsqrt), kRsqrt);
    const __m128 kZero = _mm_cmpeq_ss(kLengthSq, _mm_setzero_ps());
    const __m128 kStep = _mm_sub_ss(_mm_set_ss(3.0f), kError);
    const __m128 kScale = _mm_andnot_ps(kZero, _mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), kRsqrt), kStep));
    const __m128 kNormal = _mm_mul_ps(_mm_shuffle_ps(kScale, kScale, 0x00), kProjected);

    const __m128 kCosP = _mm_mul_ps(kNormal, kUp);
    const float fCos = _mm_cvtss_f32(
        _mm_add_ss(_mm_shuffle_ps(kCosP, kCosP, 0xAA), _mm_add_ss(_mm_shuffle_ps(kCosP, kCosP, 0x55), kCosP))
    );
    const float fAbsCos = __builtin_fabsf(fCos);
    float fAngle;
    if (!(fAbsCos >= 1.0f)) {
        fAngle = std::acos(fCos);
    }
    else if (0.0f < fCos) {
        fAngle = 0.0f;
    }
    else {
        fAngle = 3.1415927f;
    }

    // EXE-INLINED none: hkVector4::setCross (kNormal x kUp)
    alignas(16) hkVector4 kCross;
    _mm_store_ps(
        &reinterpret_cast<float*>(&kCross.m_quad)[0],
        _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(kNormal, kNormal, 0xD2), _mm_shuffle_ps(kUp, kUp, 0xC9)),
            _mm_mul_ps(_mm_shuffle_ps(kNormal, kNormal, 0xC9), _mm_shuffle_ps(kUp, kUp, 0xD2))
        )
    );
    alignas(16) hkVector4 kAxisV;
    _mm_store_ps(&reinterpret_cast<float*>(&kAxisV.m_quad)[0], kAxis);
    // EXE-INLINED none: hkVector4::getMajorAxis (the magnitudes through the andps mask at 0x1017D10)
    float fAbs0 = __builtin_fabsf(kCross.m_quad[0]);
    float fAbs1 = __builtin_fabsf(kCross.m_quad[1]);
    float fAbs2 = __builtin_fabsf(kCross.m_quad[2]);
    int32_t iCrossMajor;
    if (fAbs1 > fAbs0) {
        iCrossMajor = fAbs2 > fAbs1 ? 2 : 1;
    }
    else {
        iCrossMajor = fAbs2 > fAbs0 ? 2 : 0;
    }
    const float fCrossMajor = (&reinterpret_cast<float*>(&kCross.m_quad)[0])[iCrossMajor];
    // EXE-INLINED none: hkVector4::getMajorAxis
    fAbs0 = __builtin_fabsf(kAxisV.m_quad[0]);
    fAbs1 = __builtin_fabsf(kAxisV.m_quad[1]);
    fAbs2 = __builtin_fabsf(kAxisV.m_quad[2]);
    int32_t iAxisMajor;
    if (fAbs1 > fAbs0) {
        iAxisMajor = fAbs2 > fAbs1 ? 2 : 1;
    }
    else {
        iAxisMajor = fAbs2 > fAbs0 ? 2 : 0;
    }
    const float fAxisMajor = (&reinterpret_cast<float*>(&kAxisV.m_quad)[0])[iAxisMajor];
    if ((0.0f > fCrossMajor) != (0.0f > fAxisMajor)) {
        fAngle = -fAngle;
    }

    const float fDamping = m_damping * stepInfo.m_invDeltaTime;
    const __m128 kDamped = _mm_mul_ps(
        _mm_set1_ps(fDamping), _mm_load_ps(&reinterpret_cast<float*>(&pEntity->m_motion.m_angularVelocity.m_quad)[0])
    );
    const float fStrength = m_strength * fAngle * stepInfo.m_invDeltaTime;
    alignas(16) hkVector4 kImpulse;
    _mm_store_ps(
        &reinterpret_cast<float*>(&kImpulse.m_quad)[0], _mm_sub_ps(_mm_mul_ps(_mm_set1_ps(fStrength), kAxis), kDamped)
    );
    pEntity->activate();
    hkpMotion* pMotion = &pEntity->m_motion;
    pMotion->applyAngularImpulse(kImpulse);
}

hkpAction* hkpReorientAction::clone(
    const hkArray<hkpEntity*, hkContainerHeapAllocator>& newEntities,
    const hkArray<hkpPhantom*, hkContainerHeapAllocator>& newPhantoms
) const {
    if (newEntities.m_size != 1 || newPhantoms.m_size) {
        return nullptr;
    }
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpReorientAction* v6 = static_cast<hkpReorientAction*>(Instance->m_heap->blockAlloc(sizeof(hkpReorientAction)));
    v6->m_memSizeAndFlags = sizeof(hkpReorientAction);
    hkpAction* result = ::new (v6) hkpReorientAction(
        static_cast<hkpRigidBody*>(*(&newEntities)->begin()), m_rotationAxis, m_upAxis, m_strength, m_damping
    );
    result->m_userData = m_userData;
    return result;
}

const hkClass& hkpReorientAction::staticClass() {
    return hkpReorientActionClass;
}
