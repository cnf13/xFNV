#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Ragdoll/hkpRagdollConstraintData.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngFrictionConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpBallSocketConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConeLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpRagdollMotorConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTransformsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpTwistLimitConstraintAtom.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"

#include <bit>
#include <xmmintrin.h>

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpRagdollConstraintDataAtomsTypeInfo = {
    "hkpRagdollConstraintDataAtoms",
    finishLoadedObjecthkpRagdollConstraintDataAtoms,
    cleanupLoadedObjecthkpRagdollConstraintDataAtoms,
    nullptr,
    &typeid(hkpRagdollConstraintData::Atoms),
    sizeof(hkpRagdollConstraintData::Atoms)
};

// The reflected enums and members of hkpRagdollConstraintData and its atoms.
using hkpRagdollConstraintDataMotorIndexEnumItemsArray = const hkClassEnum::Item[3];
static hkpRagdollConstraintDataMotorIndexEnumItemsArray hkpRagdollConstraintDataMotorIndexEnumItems{
    {hkpRagdollConstraintData::MOTOR_TWIST, "MOTOR_TWIST"},
    {hkpRagdollConstraintData::MOTOR_PLANE, "MOTOR_PLANE"},
    {hkpRagdollConstraintData::MOTOR_CONE, "MOTOR_CONE"}
};
using hkpRagdollConstraintDataEnumsArray = const hkClassEnum[1];
static hkpRagdollConstraintDataEnumsArray hkpRagdollConstraintDataEnums{
    {"MotorIndex", hkpRagdollConstraintDataMotorIndexEnumItems, 3, nullptr, {0}}
};
using hkpRagdollConstraintDataAtomsAxisEnumItemsArray = const hkClassEnum::Item[3];
static hkpRagdollConstraintDataAtomsAxisEnumItemsArray hkpRagdollConstraintDataAtomsAxisEnumItems{
    {hkpRagdollConstraintData::Atoms::AXIS_TWIST, "AXIS_TWIST"},
    {hkpRagdollConstraintData::Atoms::AXIS_PLANES, "AXIS_PLANES"},
    {hkpRagdollConstraintData::Atoms::AXIS_CROSS_PRODUCT, "AXIS_CROSS_PRODUCT"}
};
using hkpRagdollConstraintDataAtomsEnumsArray = const hkClassEnum[1];
static hkpRagdollConstraintDataAtomsEnumsArray hkpRagdollConstraintDataAtomsEnums{
    {"Axis", hkpRagdollConstraintDataAtomsAxisEnumItems, 3, nullptr, {0}}
};
using hkpRagdollConstraintData_AtomsClass_MembersArray = const hkClassMember[7];
static hkpRagdollConstraintData_AtomsClass_MembersArray hkpRagdollConstraintData_AtomsClass_Members{
    {"transforms",
     &hkpSetLocalTransformsConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x0,
     nullptr},
    {"ragdollMotors",
     &hkpRagdollMotorConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x90,
     nullptr},
    {"angFriction",
     &hkpAngFrictionConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xE0,
     nullptr},
    {"twistLimit",
     &hkpTwistLimitConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xEC,
     nullptr},
    {"coneLimit",
     &hkpConeLimitConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x100,
     nullptr},
    {"planesLimit",
     &hkpConeLimitConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x114,
     nullptr},
    {"ballSocket",
     &hkpBallSocketConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x128,
     nullptr}
};
using hkpRagdollConstraintDataClass_MembersArray = const hkClassMember[1];
static hkpRagdollConstraintDataClass_MembersArray hkpRagdollConstraintDataClass_Members{
    {"atoms",
     &hkpRagdollConstraintDataAtomsClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::ALIGN_16},
     0x10,
     nullptr}
};
hkClass hkpRagdollConstraintDataAtomsClass(
    "hkpRagdollConstraintDataAtoms",
    nullptr,
    304,
    nullptr,
    0,
    hkpRagdollConstraintDataAtomsEnums,
    1,
    hkpRagdollConstraintData_AtomsClass_Members,
    7,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkpRagdollConstraintDataClass(
    "hkpRagdollConstraintData",
    &hkpConstraintDataClass,
    320,
    nullptr,
    0,
    hkpRagdollConstraintDataEnums,
    1,
    hkpRagdollConstraintDataClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpRagdollConstraintDataTypeInfo = {
    "hkpRagdollConstraintData",
    finishLoadedObjecthkpRagdollConstraintData,
    cleanupLoadedObjecthkpRagdollConstraintData,
    getVtablehkpRagdollConstraintData(),
    &typeid(hkpRagdollConstraintData),
    sizeof(hkpRagdollConstraintData)
};

hkpRagdollConstraintData::Atoms::Atoms() {
    m_transforms.m_type.m_storage = 2;
    m_ragdollMotors.m_type.m_storage = 19;
    m_ragdollMotors.m_isEnabled.m_bool = 1;
    m_ragdollMotors.m_previousTargetAnglesOffset = 0;
    m_ragdollMotors.m_initializedOffset = 0;
    m_angFriction.m_isEnabled = 1;
    m_angFriction.m_numFrictionAxes = 1;
    m_angFriction.m_type.m_storage = 17;
    m_twistLimit.m_isEnabled = 1;
    m_twistLimit.m_type.m_storage = 15;
    m_coneLimit.m_memOffsetToAngleOffset = 0;
    m_coneLimit.m_isEnabled = 1;
    m_coneLimit.m_type.m_storage = 16;
    m_planesLimit.m_memOffsetToAngleOffset = 0;
    m_planesLimit.m_isEnabled = 1;
    m_planesLimit.m_type.m_storage = 16;
    m_ballSocket.m_bodiesToNotify = 0;
    m_ballSocket.m_type.m_storage = 5;
    m_ballSocket.m_stabilizationFactor = 1.0f;
    m_ballSocket.m_maxImpulse = 3.40282e+38f;
}

void hkpRagdollConstraintData::Atoms::CopyAndAddMotorRefs(const Atoms& other) {
    operator=(other);
    hkpConstraintMotor** ppMotor = m_ragdollMotors.m_motors;
    int32_t iCount = 3;
    do {
        if (*ppMotor) {
            (*ppMotor)->addReference();
        }
        ++ppMotor;
        --iCount;
    }
    while (iCount);
}

const hkClass& hkpRagdollConstraintData::Atoms::staticClass() {
    return hkpRagdollConstraintDataAtomsClass;
}

hkpRagdollConstraintData::hkpRagdollConstraintData() {
    m_atoms.m_transforms.m_transformA.m_rotation.m_col0.m_quad = _mm_setzero_ps();
    m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad = _mm_setzero_ps();
    m_atoms.m_transforms.m_transformA.m_rotation.m_col2.m_quad = _mm_setzero_ps();
    m_atoms.m_transforms.m_transformA.m_rotation.m_col0.m_quad[0] = 1.0f;
    m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad[1] = 1.0f;
    m_atoms.m_transforms.m_transformA.m_rotation.m_col2.m_quad[2] = 1.0f;
    m_atoms.m_transforms.m_transformA.m_translation.m_quad = _mm_setzero_ps();
    m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad = _mm_setzero_ps();
    m_atoms.m_transforms.m_transformB.m_rotation.m_col1.m_quad = _mm_setzero_ps();
    m_atoms.m_transforms.m_transformB.m_rotation.m_col2.m_quad = _mm_setzero_ps();
    m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad[0] = 1.0f;
    m_atoms.m_transforms.m_transformB.m_rotation.m_col1.m_quad[1] = 1.0f;
    m_atoms.m_transforms.m_transformB.m_rotation.m_col2.m_quad[2] = 1.0f;
    m_atoms.m_transforms.m_transformB.m_translation.m_quad = _mm_setzero_ps();
    m_atoms.m_angFriction.m_firstFrictionAxis = 0;
    m_atoms.m_angFriction.m_maxFrictionTorque = 0.0f;
    m_atoms.m_angFriction.m_numFrictionAxes = 3;
    m_atoms.m_ragdollMotors.m_isEnabled.m_bool = 0;
    m_atoms.m_ragdollMotors.m_initializedOffset = 96;
    m_atoms.m_ragdollMotors.m_previousTargetAnglesOffset = 100;
    m_atoms.m_ragdollMotors.m_target_bRca.m_col0.m_quad = _mm_setzero_ps();
    m_atoms.m_ragdollMotors.m_target_bRca.m_col1.m_quad = _mm_setzero_ps();
    m_atoms.m_ragdollMotors.m_target_bRca.m_col2.m_quad = _mm_setzero_ps();
    m_atoms.m_ragdollMotors.m_target_bRca.m_col0.m_quad[0] = 1.0f;
    m_atoms.m_ragdollMotors.m_target_bRca.m_col1.m_quad[1] = 1.0f;
    m_atoms.m_ragdollMotors.m_target_bRca.m_col2.m_quad[2] = 1.0f;
    m_atoms.m_ragdollMotors.m_motors[0] = nullptr;
    m_atoms.m_twistLimit.m_minAngle = -0.5236f;
    m_atoms.m_ragdollMotors.m_motors[1] = nullptr;
    m_atoms.m_ragdollMotors.m_motors[2] = nullptr;
    m_atoms.m_twistLimit.m_maxAngle = 0.5236f;
    m_atoms.m_twistLimit.m_twistAxis = 0;
    m_atoms.m_twistLimit.m_refAxis = 1;
    m_atoms.m_twistLimit.m_angularLimitsTauFactor = 0.8f;
    m_atoms.m_coneLimit.m_twistAxisInA = 0;
    m_atoms.m_coneLimit.m_refAxisInB = 0;
    m_atoms.m_coneLimit.m_minAngle = -100.0f;
    m_atoms.m_coneLimit.m_angleMeasurementMode.m_storage = 0;
    m_atoms.m_coneLimit.m_maxAngle = 1.0f;
    m_atoms.m_coneLimit.m_angularLimitsTauFactor = 0.8f;
    setConeLimitStabilization(true);
    m_atoms.m_planesLimit.m_minAngle = -0.025f;
    m_atoms.m_planesLimit.m_twistAxisInA = 0;
    m_atoms.m_planesLimit.m_refAxisInB = 1;
    m_atoms.m_planesLimit.m_maxAngle = 0.025f;
    m_atoms.m_planesLimit.m_angleMeasurementMode.m_storage = 1;
    m_atoms.m_planesLimit.m_angularLimitsTauFactor = 0.8f;
}

hkpRagdollConstraintData::~hkpRagdollConstraintData() {
    for (int32_t i = 0; i < 3; ++i) {
        if (m_atoms.m_ragdollMotors.m_motors[i]) {
            m_atoms.m_ragdollMotors.m_motors[i]->removeReference();
        }
    }
}

void hkpRagdollConstraintData::setMaxLinearImpulse(float maxImpulse) {
    m_atoms.m_ballSocket.m_maxImpulse = maxImpulse;
}

float hkpRagdollConstraintData::getMaxLinearImpulse() const {
    return m_atoms.m_ballSocket.m_maxImpulse;
}

void hkpRagdollConstraintData::setBodyToNotify(int32_t bodyIdx) {
    m_atoms.m_ballSocket.m_bodiesToNotify = (uint8_t)(1 << ((int8_t)bodyIdx));
}

uint8_t hkpRagdollConstraintData::getNotifiedBodyIndex() const {
    return (uint8_t)((int32_t)(((uint32_t)((int32_t)m_atoms.m_ballSocket.m_bodiesToNotify)) >> 1));
}

// Each partial result is stored to the result, as the game does: both frames' rotations orthonormal (to 0.001), the
// cone limit's minimum angle at its -100 marker, its maximum angle in [0, pi), the planes' and the twist's limits in
// order.
hkBool hkpRagdollConstraintData::isValid() const {
    hkBool result;
    result.m_bool = 1;
    bool bValid = m_atoms.m_transforms.m_transformA.m_rotation.isOrthonormal(0.001f).m_bool != 0;
    result.m_bool = bValid;
    bValid = bValid && m_atoms.m_transforms.m_transformB.m_rotation.isOrthonormal(0.001f).m_bool;
    result.m_bool = bValid;
    bValid = bValid && m_atoms.m_coneLimit.m_minAngle == -100.0;
    result.m_bool = bValid;
    bValid = bValid && m_atoms.m_coneLimit.m_maxAngle >= 0.0f;
    result.m_bool = bValid;
    bValid = bValid && m_atoms.m_coneLimit.m_maxAngle < 3.1415927410125732;
    result.m_bool = bValid;
    bValid = bValid && m_atoms.m_planesLimit.m_maxAngle >= m_atoms.m_planesLimit.m_minAngle;
    result.m_bool = bValid;
    if (bValid && m_atoms.m_twistLimit.m_maxAngle >= m_atoms.m_twistLimit.m_minAngle) {
        result.m_bool = 1;
        return result;
    }

    result.m_bool = 0;
    return result;
}

int32_t hkpRagdollConstraintData::getType() const {
    return 7;
}

void hkpRagdollConstraintData::getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const {
    (&infoOut)->m_numSolverResults = 12;
    (&infoOut)->m_sizeOfExternalRuntime = 128;
}

// The atoms' span is taken from getSizeOfAllAtoms() rather than a fixed 304: a literal 304 would end the walk short of
// the ball-socket atom on x64, so the solver would never hold the two bodies together.
void hkpRagdollConstraintData::getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const {
    hkpConstraintData::getConstraintInfoUtil(m_atoms.getAtoms(), m_atoms.getSizeOfAllAtoms(), infoOut);
}

// The constraint's base in world space is the twist axis normalized, the plane axis normalized and their cross product;
// each body's frame rotation is its rotation's transpose times that base, its translation the pivot in the body's
// space; the motors' target is body B's frame rotation.
void hkpRagdollConstraintData::setInWorldSpace(
    const hkTransform& bodyATransform,
    const hkTransform& bodyBTransform,
    const hkVector4& pivot,
    const hkVector4& twistAxisW,
    const hkVector4& planeAxisW
) {
    // EXE-INLINED 0x62B0A0
    // hkVector4::dot3: mulps, then y + x and z + that (addss), in the low lane.
    auto dot3 = [](__m128 a, __m128 b) {
        const __m128 p = _mm_mul_ps(a, b);
        return _mm_add_ss(_mm_shuffle_ps(p, p, 0xAA), _mm_add_ss(_mm_shuffle_ps(p, p, 0x55), p));
    };
    // EXE-INLINED 0x62AEA0
    // hkVector4::normalize3: rsqrtss refined once (0.5 r (3 - len2 r r)), zero for a zero length, times the vector.
    auto normalize3 = [&dot3](__m128 v) {
        const __m128 len2 = dot3(v, v);
        const __m128 approx = _mm_rsqrt_ss(len2);
        const __m128 lenInv = _mm_andnot_ps(
            _mm_cmpeq_ss(len2, _mm_setzero_ps()),
            _mm_mul_ss(
                _mm_mul_ss(_mm_set_ss(0.5f), approx),
                _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(len2, approx), approx))
            )
        );
        return _mm_mul_ps(_mm_shuffle_ps(lenInv, lenInv, 0), v);
    };
    // EXE-INLINED none: hkVector4::setCross
    auto cross = [](__m128 v0, __m128 v1) {
        return _mm_sub_ps(
            _mm_mul_ps(
                _mm_shuffle_ps(v0, v0, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(v1, v1, _MM_SHUFFLE(3, 1, 0, 2))
            ),
            _mm_mul_ps(_mm_shuffle_ps(v0, v0, _MM_SHUFFLE(3, 1, 0, 2)), _mm_shuffle_ps(v1, v1, _MM_SHUFFLE(3, 0, 2, 1)))
        );
    };
    // EXE-INLINED 0xCD4BF0
    // hkMatrix3::setMulInverseMul: out = a's transpose times b, a's transpose built first, column by column.
    auto setMulInverseMul = [](hkRotation& arOut, const hkRotation& a, const hkRotation& b) {
        const __m128 col1 = a.m_col1.m_quad;
        const __m128 col2 = a.m_col2.m_quad;
        const __m128 lo = _mm_unpacklo_ps(a.m_col0.m_quad, col1);
        const __m128 hi = _mm_unpackhi_ps(a.m_col0.m_quad, col1);
        const __m128 row0 = _mm_movelh_ps(lo, col2);
        const __m128 row1 = _mm_shuffle_ps(_mm_movehl_ps(row0, lo), col2, 212);
        const __m128 row2 = _mm_shuffle_ps(hi, col2, 228);
        for (int32_t i = 0; i < 3; ++i) {
            const __m128 col = (&b.m_col0)[i].m_quad;
            (&arOut.m_col0)[i].m_quad = _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(col, col, 85), row1), _mm_mul_ps(_mm_shuffle_ps(col, col, 0), row0)
                ),
                _mm_mul_ps(_mm_shuffle_ps(col, col, 170), row2)
            );
        }
    };
    hkRotation constraintBaseInW;
    constraintBaseInW.m_col0.m_quad = normalize3(twistAxisW.m_quad);
    constraintBaseInW.m_col1.m_quad = normalize3(planeAxisW.m_quad);
    constraintBaseInW.m_col2.m_quad = cross(constraintBaseInW.m_col0.m_quad, constraintBaseInW.m_col1.m_quad);
    setMulInverseMul(m_atoms.m_transforms.m_transformA.m_rotation, bodyATransform.m_rotation, constraintBaseInW);
    setMulInverseMul(m_atoms.m_transforms.m_transformB.m_rotation, bodyBTransform.m_rotation, constraintBaseInW);
    m_atoms.m_transforms.m_transformA.m_translation.setTransformedInversePos(bodyATransform, pivot);
    m_atoms.m_transforms.m_transformB.m_translation.setTransformedInversePos(bodyBTransform, pivot);
    m_atoms.m_ragdollMotors.m_target_bRca.m_col0 = m_atoms.m_transforms.m_transformB.m_rotation.m_col0;
    m_atoms.m_ragdollMotors.m_target_bRca.m_col1 = m_atoms.m_transforms.m_transformB.m_rotation.m_col1;
    m_atoms.m_ragdollMotors.m_target_bRca.m_col2 = m_atoms.m_transforms.m_transformB.m_rotation.m_col2;
}

void hkpRagdollConstraintData::setInBodySpace(
    const hkVector4& pivotA,
    const hkVector4& pivotB,
    const hkVector4& planeAxisA,
    const hkVector4& planeAxisB,
    const hkVector4& twistAxisA,
    const hkVector4& twistAxisB
) {
    __m128 v11 = _mm_undefined_ps();
    hkVector4 v17;
    hkVector4 v32;
    __m128 v7 = _mm_mul_ps(twistAxisA.m_quad, twistAxisA.m_quad);
    float v8 = _mm_shuffle_ps(v7, v7, 85)[0] + v7[0];
    __m128 v9 = _mm_shuffle_ps(v7, v7, 170);
    v9[0] = v9[0] + v8;
    float v10 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v9[0])));
    v11[0] = std::bit_cast<float>(static_cast<int32_t>(0.0f));
    __m128 v12 = _mm_setzero_ps();
    v12[0] = 0.5f;
    __m128 v13 = _mm_cmpeq_ss(v9, v11);
    float v14 = 3.0f - ((v9[0] * v10) * v10);
    v12[0] = (0.5f * v10) * v14;
    __m128 v16 = _mm_andnot_ps(v13, v12);
    m_atoms.m_transforms.m_transformA.m_rotation.m_col0.m_quad =
        _mm_mul_ps(_mm_shuffle_ps(v16, v16, 0), twistAxisA.m_quad);
    m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad = planeAxisA.m_quad;
    v17.m_quad = m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad;
    __m128 v18 = _mm_mul_ps(v17.m_quad, v17.m_quad);
    v16[0] = _mm_shuffle_ps(v18, v18, 85)[0] + v18[0];
    __m128 v19 = _mm_shuffle_ps(v18, v18, 170);
    v19[0] = v19[0] + v16[0];
    float v20 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v19[0])));
    float v22 = 3.0f - ((v19[0] * v20) * v20);
    v12[0] = (0.5f * v20) * v22;
    __m128 v24 = _mm_andnot_ps(_mm_cmpeq_ss(v19, v11), v12);
    m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad = _mm_mul_ps(_mm_shuffle_ps(v24, v24, 0), v17.m_quad);
    m_atoms.m_transforms.m_transformA.m_rotation.m_col2.m_quad = _mm_sub_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(
                m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad,
                m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad, 210
            ),
            _mm_shuffle_ps(
                m_atoms.m_transforms.m_transformA.m_rotation.m_col0.m_quad,
                m_atoms.m_transforms.m_transformA.m_rotation.m_col0.m_quad, 201
            )
        ),
        _mm_mul_ps(
            _mm_shuffle_ps(
                m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad,
                m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad, 201
            ),
            _mm_shuffle_ps(
                m_atoms.m_transforms.m_transformA.m_rotation.m_col0.m_quad,
                m_atoms.m_transforms.m_transformA.m_rotation.m_col0.m_quad, 210
            )
        )
    );
    m_atoms.m_transforms.m_transformA.m_translation.m_quad = pivotA.m_quad;
    __m128 v25 = _mm_mul_ps(twistAxisB.m_quad, twistAxisB.m_quad);
    v24[0] = _mm_shuffle_ps(v25, v25, 85)[0] + v25[0];
    __m128 v26 = _mm_shuffle_ps(v25, v25, 170);
    v26[0] = v26[0] + v24[0];
    float v27 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v26[0])));
    __m128 v28 = _mm_cmpeq_ss(v26, v11);
    float v29 = 3.0f - ((v26[0] * v27) * v27);
    v12[0] = (0.5f * v27) * v29;
    __m128 v31 = _mm_andnot_ps(v28, v12);
    m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad =
        _mm_mul_ps(_mm_shuffle_ps(v31, v31, 0), twistAxisB.m_quad);
    m_atoms.m_transforms.m_transformB.m_rotation.m_col1.m_quad = planeAxisB.m_quad;
    v32.m_quad = m_atoms.m_transforms.m_transformB.m_rotation.m_col1.m_quad;
    __m128 v33 = _mm_mul_ps(v32.m_quad, v32.m_quad);
    v31[0] = _mm_shuffle_ps(v33, v33, 85)[0] + v33[0];
    __m128 v34 = _mm_shuffle_ps(v33, v33, 170);
    v34[0] = v34[0] + v31[0];
    v31[0] = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v34[0])));
    v12[0] = (0.5f * v31[0]) * (3.0f - ((v34[0] * v31[0]) * v31[0]));
    __m128 v35 = _mm_andnot_ps(_mm_cmpeq_ss(v34, v11), v12);
    m_atoms.m_transforms.m_transformB.m_rotation.m_col1.m_quad = _mm_mul_ps(_mm_shuffle_ps(v35, v35, 0), v32.m_quad);
    m_atoms.m_transforms.m_transformB.m_rotation.m_col2.m_quad = _mm_sub_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(
                m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad,
                m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad, 201
            ),
            _mm_shuffle_ps(
                m_atoms.m_transforms.m_transformB.m_rotation.m_col1.m_quad,
                m_atoms.m_transforms.m_transformB.m_rotation.m_col1.m_quad, 210
            )
        ),
        _mm_mul_ps(
            _mm_shuffle_ps(
                m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad,
                m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad, 210
            ),
            _mm_shuffle_ps(
                m_atoms.m_transforms.m_transformB.m_rotation.m_col1.m_quad,
                m_atoms.m_transforms.m_transformB.m_rotation.m_col1.m_quad, 201
            )
        )
    );
    m_atoms.m_transforms.m_transformB.m_translation.m_quad = pivotB.m_quad;
    m_atoms.m_ragdollMotors.m_target_bRca = static_cast<hkMatrix3>(m_atoms.m_transforms.m_transformB.m_rotation);
}

void hkpRagdollConstraintData::getConstraintFrameA(hkMatrix3& constraintFrameA) const {
    constraintFrameA = static_cast<hkMatrix3>(m_atoms.m_transforms.m_transformA.m_rotation);
}

void hkpRagdollConstraintData::getConstraintFrameB(hkMatrix3& constraintFrameB) const {
    constraintFrameB = static_cast<hkMatrix3>(m_atoms.m_transforms.m_transformB.m_rotation);
}

void hkpRagdollConstraintData::setConeLimitStabilization(hkBool enable) {
    m_atoms.m_coneLimit.m_memOffsetToAngleOffset = static_cast<uint8_t>(56 * (enable.m_bool != 0));
}

void hkpRagdollConstraintData::setAsymmetricConeAngle(float cone_min, float cone_max) {
    hkQuaternion v11;
    hkVector4* p_m_col1 = &m_atoms.m_transforms.m_transformB.m_rotation.m_col1;
    float v10 = (cone_min + cone_max) * 0.5;
    v11.setAxisAngle(m_atoms.m_transforms.m_transformB.m_rotation.m_col1, -v10);
    __m128 v5 = _mm_mul_ps(m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad, v11.m_vec.m_quad);
    __m128 v6 = _mm_shuffle_ps(v5, v5, 170);
    v6[0] = v6[0] + (_mm_shuffle_ps(v5, v5, 85)[0] + v5[0]);
    __m128 v7 = _mm_shuffle_ps(v11.m_vec.m_quad, v11.m_vec.m_quad, 255);
    __m128 v8 = _mm_add_ps(
        _mm_mul_ps(
            _mm_sub_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(
                        m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad,
                        m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad, 210
                    ),
                    _mm_shuffle_ps(v11.m_vec.m_quad, v11.m_vec.m_quad, 201)
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(
                        m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad,
                        m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad, 201
                    ),
                    _mm_shuffle_ps(v11.m_vec.m_quad, v11.m_vec.m_quad, 210)
                )
            ),
            v7
        ),
        _mm_add_ps(
            _mm_mul_ps(
                _mm_add_ps(_mm_mul_ps(v7, v7), hkQuadRealMinusHalf),
                m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad
            ),
            _mm_mul_ps(_mm_shuffle_ps(v6, v6, 0), v11.m_vec.m_quad)
        )
    );
    m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad = _mm_add_ps(v8, v8);
    m_atoms.m_transforms.m_transformB.m_rotation.m_col2.m_quad = _mm_sub_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(
                m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad,
                m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad, 201
            ),
            _mm_shuffle_ps(p_m_col1->m_quad, p_m_col1->m_quad, 210)
        ),
        _mm_mul_ps(
            _mm_shuffle_ps(
                m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad,
                m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad, 210
            ),
            _mm_shuffle_ps(p_m_col1->m_quad, p_m_col1->m_quad, 201)
        )
    );
    m_atoms.m_coneLimit.m_maxAngle = (cone_max - cone_min) * 0.5;
}

void hkpRagdollConstraintData::setMaxFrictionTorque(float tmag) {
    m_atoms.m_angFriction.m_maxFrictionTorque = tmag;
}

void hkpRagdollConstraintData::setTarget(const hkMatrix3& target_cbRca) {
    m_atoms.m_ragdollMotors.m_target_bRca.setMul(m_atoms.m_transforms.m_transformB.m_rotation, target_cbRca);
}

void hkpRagdollConstraintData::getTarget(hkMatrix3& target_cbRca) {
    target_cbRca.setMulInverseMul(m_atoms.m_transforms.m_transformB.m_rotation, m_atoms.m_ragdollMotors.m_target_bRca);
}

void hkpRagdollConstraintData::setMotorsActive(hkpConstraintInstance* instance, hkBool toBeEnabled) {
    m_atoms.m_ragdollMotors.m_isEnabled = toBeEnabled;
    m_atoms.m_angFriction.m_isEnabled = toBeEnabled.m_bool == 0;
    if (instance) {
        hkConstraintInternal* m_internal = instance->getInternal();
        if (m_internal) {
            float* m_runtime = static_cast<float*>(m_internal->m_runtime);
            if (m_runtime) {
                m_runtime[6] = 0.0;
                m_runtime[7] = 0.0;
                m_runtime[8] = 0.0;
                m_runtime[9] = 0.0;
                m_runtime[10] = 0.0;
                m_runtime[11] = 0.0;
                *m_runtime = 0.0;
                m_runtime[1] = 0.0;
                m_runtime[2] = 0.0;
                m_runtime[3] = 0.0;
                m_runtime[4] = 0.0;
                m_runtime[5] = 0.0;
            }
        }
    }
}

hkpConstraintMotor* hkpRagdollConstraintData::getTwistMotor() const {
    return m_atoms.m_ragdollMotors.m_motors[0];
}

hkpConstraintMotor* hkpRagdollConstraintData::getConeMotor() const {
    return m_atoms.m_ragdollMotors.m_motors[2];
}

hkpConstraintMotor* hkpRagdollConstraintData::getPlaneMotor() const {
    return m_atoms.m_ragdollMotors.m_motors[1];
}

void hkpRagdollConstraintData::setMotor(MotorIndex index, hkpConstraintMotor* newMotor) {
    if (newMotor) {
        newMotor->addReference();
    }
    if (m_atoms.m_ragdollMotors.m_motors[index]) {
        m_atoms.m_ragdollMotors.m_motors[index]->removeReference();
    }
    m_atoms.m_ragdollMotors.m_motors[index] = newMotor;
}

void hkpRagdollConstraintData::setTwistMotor(hkpConstraintMotor* motor) {
    setMotor(MOTOR_TWIST, motor);
}

void hkpRagdollConstraintData::setConeMotor(hkpConstraintMotor* motor) {
    setMotor(MOTOR_CONE, motor);
}

void hkpRagdollConstraintData::setPlaneMotor(hkpConstraintMotor* motor) {
    setMotor(MOTOR_PLANE, motor);
}

void hkpRagdollConstraintData::setTargetRelativeOrientationOfBodies(hkMatrix3* a1) {
    m_atoms.m_ragdollMotors.m_target_bRca.setMul(*a1, m_atoms.m_transforms.m_transformA.m_rotation);
}

const hkClass& hkpRagdollConstraintData::staticClass() {
    return hkpRagdollConstraintDataClass;
}
