#include "Havok/Source/Physics/Dynamics/Constraint/Chain/RagdollLimits/hkpRagdollLimitsData.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConeLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalRotationsConstraintAtom.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <xmmintrin.h>

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpRagdollLimitsDataAtomsTypeInfo = {
    "hkpRagdollLimitsDataAtoms",
    finishLoadedObjecthkpRagdollLimitsDataAtoms,
    cleanupLoadedObjecthkpRagdollLimitsDataAtoms,
    nullptr,
    &typeid(hkpRagdollLimitsData::Atoms),
    sizeof(hkpRagdollLimitsData::Atoms)
};

using hkpRagdollLimitsDataAtomsAxisEnumItemsArray = const hkClassEnum::Item[3];
static hkpRagdollLimitsDataAtomsAxisEnumItemsArray hkpRagdollLimitsDataAtomsAxisEnumItems{
    {hkpRagdollLimitsData::Atoms::AXIS_TWIST, "AXIS_TWIST"},
    {hkpRagdollLimitsData::Atoms::AXIS_PLANES, "AXIS_PLANES"},
    {hkpRagdollLimitsData::Atoms::AXIS_CROSS_PRODUCT, "AXIS_CROSS_PRODUCT"}
};
using hkpRagdollLimitsDataAtomsEnumsArray = const hkClassEnum[1];
static hkpRagdollLimitsDataAtomsEnumsArray hkpRagdollLimitsDataAtomsEnums{
    {"Axis", hkpRagdollLimitsDataAtomsAxisEnumItems, 3, nullptr, {0}}
};
using hkpRagdollLimitsData_AtomsClass_MembersArray = const hkClassMember[4];
static hkpRagdollLimitsData_AtomsClass_MembersArray hkpRagdollLimitsData_AtomsClass_Members{
    {"rotations",
     &hkpSetLocalRotationsConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x0,
     nullptr},
    {"twistLimit",
     &hkpTwistLimitConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x70,
     nullptr},
    {"coneLimit",
     &hkpConeLimitConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x84,
     nullptr},
    {"planesLimit",
     &hkpConeLimitConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x98,
     nullptr}
};
using hkpRagdollLimitsDataClass_MembersArray = const hkClassMember[1];
static hkpRagdollLimitsDataClass_MembersArray hkpRagdollLimitsDataClass_Members{
    {"atoms",
     &hkpRagdollLimitsDataAtomsClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::ALIGN_16},
     0x10,
     nullptr}
};

hkClass hkpRagdollLimitsDataAtomsClass(
    "hkpRagdollLimitsDataAtoms",
    nullptr,
    176,
    nullptr,
    0,
    hkpRagdollLimitsDataAtomsEnums,
    1,
    hkpRagdollLimitsData_AtomsClass_Members,
    4,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkpRagdollLimitsDataClass(
    "hkpRagdollLimitsData",
    &hkpConstraintDataClass,
    192,
    nullptr,
    0,
    nullptr,
    0,
    hkpRagdollLimitsDataClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpRagdollLimitsDataTypeInfo = {
    "hkpRagdollLimitsData",
    finishLoadedObjecthkpRagdollLimitsData,
    cleanupLoadedObjecthkpRagdollLimitsData,
    getVtablehkpRagdollLimitsData(),
    &typeid(hkpRagdollLimitsData),
    sizeof(hkpRagdollLimitsData)
};

const hkClass& hkpRagdollLimitsData::Atoms::staticClass() {
    return hkpRagdollLimitsDataAtomsClass;
}

// Sets identity rotations, the twist, cone and planes limits' defaults, and turns cone limit stabilization on.
hkpRagdollLimitsData::hkpRagdollLimitsData() {
    m_atoms.m_rotations.m_type.m_storage = 4;
    m_atoms.m_twistLimit.m_isEnabled = 1;
    m_atoms.m_twistLimit.m_type.m_storage = 15;
    m_atoms.m_coneLimit.m_isEnabled = 1;
    m_atoms.m_coneLimit.m_memOffsetToAngleOffset = 0;
    m_atoms.m_coneLimit.m_type.m_storage = 16;
    m_atoms.m_planesLimit.m_type.m_storage = 16;
    m_atoms.m_planesLimit.m_isEnabled = 1;
    m_atoms.m_planesLimit.m_memOffsetToAngleOffset = 0;
    m_atoms.m_rotations.m_rotationA.m_col0.m_quad = _mm_setzero_ps();
    m_atoms.m_rotations.m_rotationA.m_col1.m_quad = _mm_setzero_ps();
    m_atoms.m_rotations.m_rotationA.m_col2.m_quad = _mm_setzero_ps();
    XFNV_M128_F32(m_atoms.m_rotations.m_rotationA.m_col0.m_quad, 0) = 1.0f;
    XFNV_M128_F32(m_atoms.m_rotations.m_rotationA.m_col1.m_quad, 1) = 1.0f;
    XFNV_M128_F32(m_atoms.m_rotations.m_rotationA.m_col2.m_quad, 2) = 1.0f;
    m_atoms.m_rotations.m_rotationB.m_col0.m_quad = _mm_setzero_ps();
    m_atoms.m_rotations.m_rotationB.m_col1.m_quad = _mm_setzero_ps();
    m_atoms.m_rotations.m_rotationB.m_col2.m_quad = _mm_setzero_ps();
    XFNV_M128_F32(m_atoms.m_rotations.m_rotationB.m_col0.m_quad, 0) = 1.0f;
    XFNV_M128_F32(m_atoms.m_rotations.m_rotationB.m_col1.m_quad, 1) = 1.0f;
    XFNV_M128_F32(m_atoms.m_rotations.m_rotationB.m_col2.m_quad, 2) = 1.0f;
    m_atoms.m_twistLimit.m_twistAxis = 0;
    m_atoms.m_twistLimit.m_refAxis = 1;
    m_atoms.m_twistLimit.m_minAngle = -0.5236f;
    m_atoms.m_coneLimit.m_twistAxisInA = 0;
    m_atoms.m_coneLimit.m_refAxisInB = 0;
    m_atoms.m_twistLimit.m_maxAngle = 0.5236f;
    m_atoms.m_coneLimit.m_angleMeasurementMode.m_storage = 0;
    m_atoms.m_twistLimit.m_angularLimitsTauFactor = 0.8f;
    m_atoms.m_coneLimit.m_minAngle = -100.0f;
    m_atoms.m_coneLimit.m_maxAngle = 1.0f;
    m_atoms.m_coneLimit.m_angularLimitsTauFactor = 0.8f;
    setConeLimitStabilization(1);
    m_atoms.m_planesLimit.m_minAngle = -0.025f;
    m_atoms.m_planesLimit.m_refAxisInB = 1;
    m_atoms.m_planesLimit.m_angleMeasurementMode.m_storage = 1;
    m_atoms.m_planesLimit.m_maxAngle = 0.025f;
    m_atoms.m_planesLimit.m_twistAxisInA = 0;
    m_atoms.m_planesLimit.m_angularLimitsTauFactor = 0.8f;
}

// Whether both rotations are orthonormal, the cone limit's minimum is the unused -100, its maximum in [0, pi) and the
// planes and twist limits' maximum at least their minimum.
hkBool hkpRagdollLimitsData::isValid() const {
    hkBool result;
    result.m_bool = 1;
    bool m_bool = m_atoms.m_rotations.m_rotationA.isOrthonormal(0.001f).m_bool != 0;
    result.m_bool = m_bool;
    bool m_bool_1 = m_bool && m_atoms.m_rotations.m_rotationB.isOrthonormal(0.001f).m_bool;
    result.m_bool = m_bool_1;
    bool m_bool_2 = m_bool_1 && m_atoms.m_coneLimit.m_minAngle == -100.0;
    result.m_bool = m_bool_2;
    bool m_bool_3 = m_bool_2 && m_atoms.m_coneLimit.m_maxAngle >= 0.0f;
    result.m_bool = m_bool_3;
    bool m_bool_4 = m_bool_3 && m_atoms.m_coneLimit.m_maxAngle < 3.1415927410125732;
    result.m_bool = m_bool_4;
    bool m_bool_5 = m_bool_4 && m_atoms.m_planesLimit.m_maxAngle >= m_atoms.m_planesLimit.m_minAngle;
    result.m_bool = m_bool_5;
    if (m_bool_5 && m_atoms.m_twistLimit.m_maxAngle >= m_atoms.m_twistLimit.m_minAngle) {
        result.m_bool = 1;
        return result;
    }

    result.m_bool = 0;
    return result;
}

int32_t hkpRagdollLimitsData::getType() const {
    return 19;
}

void hkpRagdollLimitsData::getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const {
    infoOut.m_numSolverResults = 3;
    infoOut.m_sizeOfExternalRuntime = 28;
}

void hkpRagdollLimitsData::getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const {
    hkpConstraintData::getConstraintInfoUtil(&m_atoms.m_rotations, 172, infoOut);
}

// KEEP: its callers (the constructor, for one) store values after the call that they loaded before it, relying on this
// function changing nothing but its return value, which clang does not guarantee.
char hkpRagdollLimitsData::setConeLimitStabilization(char enable) {
    char result = 16 * (enable != 0);
    m_atoms.m_coneLimit.m_memOffsetToAngleOffset = result;
    return result;
}

// The constraint's base in world space is the twist axis normalized, the plane axis normalized and their cross product;
// each body's rotation is its rotation's transpose times that base.
void hkpRagdollLimitsData::setInWorldSpace(
    const hkTransform& bodyATransform,
    const hkTransform& bodyBTransform,
    const hkVector4& twistAxisW,
    const hkVector4& planeAxisW
) {
    hkRotation constraintBaseInW;
    constraintBaseInW.m_col0 = twistAxisW;
    constraintBaseInW.m_col0.normalize3();
    constraintBaseInW.m_col1 = planeAxisW;
    constraintBaseInW.m_col1.normalize3();
    constraintBaseInW.m_col2.setCross(constraintBaseInW.m_col0, constraintBaseInW.m_col1);
    m_atoms.m_rotations.m_rotationA.setMulInverseMul(bodyATransform.m_rotation, constraintBaseInW);
    m_atoms.m_rotations.m_rotationB.setMulInverseMul(bodyBTransform.m_rotation, constraintBaseInW);
}

void hkpRagdollLimitsData::setInBodySpace(
    const hkVector4& planeAxisA,
    const hkVector4& planeAxisB,
    const hkVector4& twistAxisA,
    const hkVector4& twistAxisB
) {
    __m128 v9 = _mm_undefined_ps();
    hkVector4 v15;
    hkVector4 v30;
    __m128 v5 = _mm_mul_ps(twistAxisA.m_quad, twistAxisA.m_quad);
    float v6 = XFNV_M128_F32(_mm_shuffle_ps(v5, v5, 85), 0) + XFNV_M128_F32(v5, 0);
    __m128 v7 = _mm_shuffle_ps(v5, v5, 170);
    XFNV_M128_F32(v7, 0) = XFNV_M128_F32(v7, 0) + v6;
    float v8 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(v7, 0))));
    XFNV_M128_F32(v9, 0) = std::bit_cast<float>(static_cast<int32_t>(0.0f));
    __m128 v10 = _mm_setzero_ps();
    XFNV_M128_F32(v10, 0) = 0.5f;
    __m128 v11 = _mm_cmpeq_ss(v7, v9);
    float v12 = 3.0f - ((XFNV_M128_F32(v7, 0) * v8) * v8);
    XFNV_M128_F32(v10, 0) = (0.5f * v8) * v12;
    __m128 v14 = _mm_andnot_ps(v11, v10);
    m_atoms.m_rotations.m_rotationA.m_col0.m_quad = _mm_mul_ps(_mm_shuffle_ps(v14, v14, 0), twistAxisA.m_quad);
    m_atoms.m_rotations.m_rotationA.m_col1.m_quad = planeAxisA.m_quad;
    v15.m_quad = m_atoms.m_rotations.m_rotationA.m_col1.m_quad;
    __m128 v16 = _mm_mul_ps(v15.m_quad, v15.m_quad);
    XFNV_M128_F32(v14, 0) = XFNV_M128_F32(_mm_shuffle_ps(v16, v16, 85), 0) + XFNV_M128_F32(v16, 0);
    __m128 v17 = _mm_shuffle_ps(v16, v16, 170);
    XFNV_M128_F32(v17, 0) = XFNV_M128_F32(v17, 0) + XFNV_M128_F32(v14, 0);
    float v18 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(v17, 0))));
    float v20 = 3.0f - ((XFNV_M128_F32(v17, 0) * v18) * v18);
    XFNV_M128_F32(v10, 0) = (0.5f * v18) * v20;
    __m128 v22 = _mm_andnot_ps(_mm_cmpeq_ss(v17, v9), v10);
    m_atoms.m_rotations.m_rotationA.m_col1.m_quad = _mm_mul_ps(_mm_shuffle_ps(v22, v22, 0), v15.m_quad);
    m_atoms.m_rotations.m_rotationA.m_col2.m_quad = _mm_sub_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(
                m_atoms.m_rotations.m_rotationA.m_col1.m_quad,
                m_atoms.m_rotations.m_rotationA.m_col1.m_quad,
                210
            ),
            _mm_shuffle_ps(
                m_atoms.m_rotations.m_rotationA.m_col0.m_quad,
                m_atoms.m_rotations.m_rotationA.m_col0.m_quad,
                201
            )
        ),
        _mm_mul_ps(
            _mm_shuffle_ps(
                m_atoms.m_rotations.m_rotationA.m_col1.m_quad,
                m_atoms.m_rotations.m_rotationA.m_col1.m_quad,
                201
            ),
            _mm_shuffle_ps(
                m_atoms.m_rotations.m_rotationA.m_col0.m_quad,
                m_atoms.m_rotations.m_rotationA.m_col0.m_quad,
                210
            )
        )
    );
    __m128 v23 = _mm_mul_ps(twistAxisB.m_quad, twistAxisB.m_quad);
    XFNV_M128_F32(v22, 0) = XFNV_M128_F32(_mm_shuffle_ps(v23, v23, 85), 0) + XFNV_M128_F32(v23, 0);
    __m128 v24 = _mm_shuffle_ps(v23, v23, 170);
    XFNV_M128_F32(v24, 0) = XFNV_M128_F32(v24, 0) + XFNV_M128_F32(v22, 0);
    float v25 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(v24, 0))));
    __m128 v26 = _mm_cmpeq_ss(v24, v9);
    float v27 = 3.0f - ((XFNV_M128_F32(v24, 0) * v25) * v25);
    XFNV_M128_F32(v10, 0) = (0.5f * v25) * v27;
    __m128 v29 = _mm_andnot_ps(v26, v10);
    m_atoms.m_rotations.m_rotationB.m_col0.m_quad = _mm_mul_ps(_mm_shuffle_ps(v29, v29, 0), twistAxisB.m_quad);
    m_atoms.m_rotations.m_rotationB.m_col1.m_quad = planeAxisB.m_quad;
    v30.m_quad = m_atoms.m_rotations.m_rotationB.m_col1.m_quad;
    __m128 v31 = _mm_mul_ps(v30.m_quad, v30.m_quad);
    XFNV_M128_F32(v29, 0) = XFNV_M128_F32(_mm_shuffle_ps(v31, v31, 85), 0) + XFNV_M128_F32(v31, 0);
    __m128 v32 = _mm_shuffle_ps(v31, v31, 170);
    XFNV_M128_F32(v32, 0) = XFNV_M128_F32(v32, 0) + XFNV_M128_F32(v29, 0);
    XFNV_M128_F32(v29, 0) = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(v32, 0))));
    XFNV_M128_F32(v10, 0) = (0.5f * XFNV_M128_F32(v29, 0)) *
        (3.0f - ((XFNV_M128_F32(v32, 0) * XFNV_M128_F32(v29, 0)) * XFNV_M128_F32(v29, 0)));
    __m128 v33 = _mm_andnot_ps(_mm_cmpeq_ss(v32, v9), v10);
    m_atoms.m_rotations.m_rotationB.m_col1.m_quad = _mm_mul_ps(_mm_shuffle_ps(v33, v33, 0), v30.m_quad);
    m_atoms.m_rotations.m_rotationB.m_col2.m_quad = _mm_sub_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(
                m_atoms.m_rotations.m_rotationB.m_col1.m_quad,
                m_atoms.m_rotations.m_rotationB.m_col1.m_quad,
                210
            ),
            _mm_shuffle_ps(
                m_atoms.m_rotations.m_rotationB.m_col0.m_quad,
                m_atoms.m_rotations.m_rotationB.m_col0.m_quad,
                201
            )
        ),
        _mm_mul_ps(
            _mm_shuffle_ps(
                m_atoms.m_rotations.m_rotationB.m_col1.m_quad,
                m_atoms.m_rotations.m_rotationB.m_col1.m_quad,
                201
            ),
            _mm_shuffle_ps(
                m_atoms.m_rotations.m_rotationB.m_col0.m_quad,
                m_atoms.m_rotations.m_rotationB.m_col0.m_quad,
                210
            )
        )
    );
}

void hkpRagdollLimitsData::getConstraintFrameA(hkMatrix3& constraintFrameA) {
    constraintFrameA = static_cast<hkMatrix3>(m_atoms.m_rotations.m_rotationA);
}

void hkpRagdollLimitsData::getConstraintFrameB(hkMatrix3& constraintFrameB) {
    constraintFrameB = static_cast<hkMatrix3>(m_atoms.m_rotations.m_rotationB);
}

const hkClass& hkpRagdollLimitsData::staticClass() {
    return hkpRagdollLimitsDataClass;
}
