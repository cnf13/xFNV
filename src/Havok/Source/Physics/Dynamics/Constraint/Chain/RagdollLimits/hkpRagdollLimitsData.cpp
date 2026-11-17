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
    "hkpRagdollLimitsData",          finishLoadedObjecthkpRagdollLimitsData, cleanupLoadedObjecthkpRagdollLimitsData,
    getVtablehkpRagdollLimitsData(), &typeid(hkpRagdollLimitsData),          sizeof(hkpRagdollLimitsData)
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
    m_atoms.m_rotations.m_rotationA.m_col0.m_quad[0] = 1.0f;
    m_atoms.m_rotations.m_rotationA.m_col1.m_quad[1] = 1.0f;
    m_atoms.m_rotations.m_rotationA.m_col2.m_quad[2] = 1.0f;
    m_atoms.m_rotations.m_rotationB.m_col0.m_quad = _mm_setzero_ps();
    m_atoms.m_rotations.m_rotationB.m_col1.m_quad = _mm_setzero_ps();
    m_atoms.m_rotations.m_rotationB.m_col2.m_quad = _mm_setzero_ps();
    m_atoms.m_rotations.m_rotationB.m_col0.m_quad[0] = 1.0f;
    m_atoms.m_rotations.m_rotationB.m_col1.m_quad[1] = 1.0f;
    m_atoms.m_rotations.m_rotationB.m_col2.m_quad[2] = 1.0f;
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
// planes and twist limits' maximum at least their minimum. Each isOrthonormal result goes to a local the callee fills.
hkBool* hkpRagdollLimitsData::isValid(hkBool* arResult) const {
    hkBool kOrthonormal;
    hkBool* arResult_1 = arResult;
    arResult->m_bool = 1;
    bool m_bool = m_atoms.m_rotations.m_rotationA.isOrthonormal(&kOrthonormal, 0.001f)->m_bool != 0;
    arResult_1->m_bool = m_bool;
    bool m_bool_1 = m_bool && m_atoms.m_rotations.m_rotationB.isOrthonormal(&kOrthonormal, 0.001f)->m_bool;
    arResult_1->m_bool = m_bool_1;
    bool m_bool_2 = m_bool_1 && m_atoms.m_coneLimit.m_minAngle == -100.0;
    arResult_1->m_bool = m_bool_2;
    bool m_bool_3 = m_bool_2 && m_atoms.m_coneLimit.m_maxAngle >= 0.0f;
    arResult_1->m_bool = m_bool_3;
    bool m_bool_4 = m_bool_3 && m_atoms.m_coneLimit.m_maxAngle < 3.1415927410125732;
    arResult_1->m_bool = m_bool_4;
    bool m_bool_5 = m_bool_4 && m_atoms.m_planesLimit.m_maxAngle >= m_atoms.m_planesLimit.m_minAngle;
    arResult_1->m_bool = m_bool_5;
    if (m_bool_5 && m_atoms.m_twistLimit.m_maxAngle >= m_atoms.m_twistLimit.m_minAngle) {
        arResult_1->m_bool = 1;
        return arResult_1;
    }

    arResult_1->m_bool = 0;
    return arResult_1;
}

int32_t hkpRagdollLimitsData::getType() const {
    return 19;
}

void hkpRagdollLimitsData::getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const {
    (&infoOut)->m_numSolverResults = 3;
    (&infoOut)->m_sizeOfExternalRuntime = 28;
}

void hkpRagdollLimitsData::getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const {
    hkpConstraintData::getConstraintInfoUtil(&m_atoms.m_rotations, 172, infoOut);
}

// KEEP: the exe's callers rely on ecx and edx the exe body leaves intact (the constructor 0xD4F2A0 stores through
// ecx at 0xD4F398 and dl at 0xD4F3B6 after the call), which clang does not guarantee.
char hkpRagdollLimitsData::setConeLimitStabilization(char a1) {
    char result = 16 * (a1 != 0);
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
    setMulInverseMul(m_atoms.m_rotations.m_rotationA, bodyATransform.m_rotation, constraintBaseInW);
    setMulInverseMul(m_atoms.m_rotations.m_rotationB, bodyBTransform.m_rotation, constraintBaseInW);
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
    float v6 = _mm_shuffle_ps(v5, v5, 85)[0] + v5[0];
    __m128 v7 = _mm_shuffle_ps(v5, v5, 170);
    v7[0] = v7[0] + v6;
    float v8 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v7[0])));
    v9[0] = std::bit_cast<float>(static_cast<int32_t>(0.0f));
    __m128 v10 = _mm_setzero_ps();
    v10[0] = 0.5f;
    __m128 v11 = _mm_cmpeq_ss(v7, v9);
    float v12 = 3.0f - ((v7[0] * v8) * v8);
    v10[0] = (0.5f * v8) * v12;
    __m128 v14 = _mm_andnot_ps(v11, v10);
    m_atoms.m_rotations.m_rotationA.m_col0.m_quad = _mm_mul_ps(_mm_shuffle_ps(v14, v14, 0), twistAxisA.m_quad);
    m_atoms.m_rotations.m_rotationA.m_col1.m_quad = planeAxisA.m_quad;
    v15.m_quad = m_atoms.m_rotations.m_rotationA.m_col1.m_quad;
    __m128 v16 = _mm_mul_ps(v15.m_quad, v15.m_quad);
    v14[0] = _mm_shuffle_ps(v16, v16, 85)[0] + v16[0];
    __m128 v17 = _mm_shuffle_ps(v16, v16, 170);
    v17[0] = v17[0] + v14[0];
    float v18 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v17[0])));
    float v20 = 3.0f - ((v17[0] * v18) * v18);
    v10[0] = (0.5f * v18) * v20;
    __m128 v22 = _mm_andnot_ps(_mm_cmpeq_ss(v17, v9), v10);
    m_atoms.m_rotations.m_rotationA.m_col1.m_quad = _mm_mul_ps(_mm_shuffle_ps(v22, v22, 0), v15.m_quad);
    m_atoms.m_rotations.m_rotationA.m_col2.m_quad = _mm_sub_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(
                m_atoms.m_rotations.m_rotationA.m_col1.m_quad, m_atoms.m_rotations.m_rotationA.m_col1.m_quad, 210
            ),
            _mm_shuffle_ps(
                m_atoms.m_rotations.m_rotationA.m_col0.m_quad, m_atoms.m_rotations.m_rotationA.m_col0.m_quad, 201
            )
        ),
        _mm_mul_ps(
            _mm_shuffle_ps(
                m_atoms.m_rotations.m_rotationA.m_col1.m_quad, m_atoms.m_rotations.m_rotationA.m_col1.m_quad, 201
            ),
            _mm_shuffle_ps(
                m_atoms.m_rotations.m_rotationA.m_col0.m_quad, m_atoms.m_rotations.m_rotationA.m_col0.m_quad, 210
            )
        )
    );
    __m128 v23 = _mm_mul_ps(twistAxisB.m_quad, twistAxisB.m_quad);
    v22[0] = _mm_shuffle_ps(v23, v23, 85)[0] + v23[0];
    __m128 v24 = _mm_shuffle_ps(v23, v23, 170);
    v24[0] = v24[0] + v22[0];
    float v25 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v24[0])));
    __m128 v26 = _mm_cmpeq_ss(v24, v9);
    float v27 = 3.0f - ((v24[0] * v25) * v25);
    v10[0] = (0.5f * v25) * v27;
    __m128 v29 = _mm_andnot_ps(v26, v10);
    m_atoms.m_rotations.m_rotationB.m_col0.m_quad = _mm_mul_ps(_mm_shuffle_ps(v29, v29, 0), twistAxisB.m_quad);
    m_atoms.m_rotations.m_rotationB.m_col1.m_quad = planeAxisB.m_quad;
    v30.m_quad = m_atoms.m_rotations.m_rotationB.m_col1.m_quad;
    __m128 v31 = _mm_mul_ps(v30.m_quad, v30.m_quad);
    v29[0] = _mm_shuffle_ps(v31, v31, 85)[0] + v31[0];
    __m128 v32 = _mm_shuffle_ps(v31, v31, 170);
    v32[0] = v32[0] + v29[0];
    v29[0] = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v32[0])));
    v10[0] = (0.5f * v29[0]) * (3.0f - ((v32[0] * v29[0]) * v29[0]));
    __m128 v33 = _mm_andnot_ps(_mm_cmpeq_ss(v32, v9), v10);
    m_atoms.m_rotations.m_rotationB.m_col1.m_quad = _mm_mul_ps(_mm_shuffle_ps(v33, v33, 0), v30.m_quad);
    m_atoms.m_rotations.m_rotationB.m_col2.m_quad = _mm_sub_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(
                m_atoms.m_rotations.m_rotationB.m_col1.m_quad, m_atoms.m_rotations.m_rotationB.m_col1.m_quad, 210
            ),
            _mm_shuffle_ps(
                m_atoms.m_rotations.m_rotationB.m_col0.m_quad, m_atoms.m_rotations.m_rotationB.m_col0.m_quad, 201
            )
        ),
        _mm_mul_ps(
            _mm_shuffle_ps(
                m_atoms.m_rotations.m_rotationB.m_col1.m_quad, m_atoms.m_rotations.m_rotationB.m_col1.m_quad, 201
            ),
            _mm_shuffle_ps(
                m_atoms.m_rotations.m_rotationB.m_col0.m_quad, m_atoms.m_rotations.m_rotationB.m_col0.m_quad, 210
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
