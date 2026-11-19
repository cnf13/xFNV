#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Wheel/hkpWheelConstraintData.h"

#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkp2dAngConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinSoftConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalRotationsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTransformsConstraintAtom.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpWheelConstraintDataAtomsTypeInfo = {
    "hkpWheelConstraintDataAtoms",
    finishLoadedObjecthkpWheelConstraintDataAtoms,
    cleanupLoadedObjecthkpWheelConstraintDataAtoms,
    nullptr,
    &typeid(hkpWheelConstraintData::Atoms),
    sizeof(hkpWheelConstraintData::Atoms)
};

// The reflection of hkpWheelConstraintData::Atoms and hkpWheelConstraintData.
using hkpWheelConstraintDataAtomsAxisEnumItemsArray = const hkClassEnum::Item[4];
static hkpWheelConstraintDataAtomsAxisEnumItemsArray hkpWheelConstraintDataAtomsAxisEnumItems{
    {hkpWheelConstraintData::Atoms::AXIS_SUSPENSION, "AXIS_SUSPENSION"},
    {hkpWheelConstraintData::Atoms::AXIS_PERP_SUSPENSION, "AXIS_PERP_SUSPENSION"},
    {hkpWheelConstraintData::Atoms::AXIS_AXLE, "AXIS_AXLE"},
    {hkpWheelConstraintData::Atoms::AXIS_STEERING, "AXIS_STEERING"}
};
using hkpWheelConstraintDataAtomsEnumsArray = const hkClassEnum[1];
static hkpWheelConstraintDataAtomsEnumsArray hkpWheelConstraintDataAtomsEnums{
    {"Axis", hkpWheelConstraintDataAtomsAxisEnumItems, 4, nullptr, {0}}
};
using hkpWheelConstraintDataAtomsClass_MembersArray = const hkClassMember[7];
static hkpWheelConstraintDataAtomsClass_MembersArray hkpWheelConstraintDataAtomsClass_Members{
    {"suspensionBase", &hkpSetLocalTransformsConstraintAtomClass, nullptr, hkClassMember::TYPE_STRUCT,
     hkClassMember::TYPE_VOID, 0, 0, 0x0, nullptr},
    {"lin0Limit", &hkpLinLimitConstraintAtomClass, nullptr, hkClassMember::TYPE_STRUCT, hkClassMember::TYPE_VOID, 0, 0,
     0x90, nullptr},
    {"lin0Soft", &hkpLinSoftConstraintAtomClass, nullptr, hkClassMember::TYPE_STRUCT, hkClassMember::TYPE_VOID, 0, 0,
     0x9C, nullptr},
    {"lin1", &hkpLinConstraintAtomClass, nullptr, hkClassMember::TYPE_STRUCT, hkClassMember::TYPE_VOID, 0, 0, 0xA8,
     nullptr},
    {"lin2", &hkpLinConstraintAtomClass, nullptr, hkClassMember::TYPE_STRUCT, hkClassMember::TYPE_VOID, 0, 0, 0xAC,
     nullptr},
    {"steeringBase", &hkpSetLocalRotationsConstraintAtomClass, nullptr, hkClassMember::TYPE_STRUCT,
     hkClassMember::TYPE_VOID, 0, 0, 0xB0, nullptr},
    {"2dAng", &hkp2dAngConstraintAtomClass, nullptr, hkClassMember::TYPE_STRUCT, hkClassMember::TYPE_VOID, 0, 0, 0x120,
     nullptr}
};
hkClass hkpWheelConstraintDataAtomsClass(
    "hkpWheelConstraintDataAtoms",
    nullptr,
    304,
    nullptr,
    0,
    hkpWheelConstraintDataAtomsEnums,
    1,
    hkpWheelConstraintDataAtomsClass_Members,
    7,
    nullptr,
    nullptr,
    0,
    0
);
using hkpWheelConstraintDataClass_MembersArray = const hkClassMember[3];
static hkpWheelConstraintDataClass_MembersArray hkpWheelConstraintDataClass_Members{
    {"atoms",
     &hkpWheelConstraintDataAtomsClass,
     nullptr,
     hkClassMember::TYPE_STRUCT,
     hkClassMember::TYPE_VOID,
     0,
     {hkClassMember::ALIGN_16},
     0x10,
     nullptr},
    {"initialAxleInB", nullptr, nullptr, hkClassMember::TYPE_VECTOR4, hkClassMember::TYPE_VOID, 0, 0, 0x140, nullptr},
    {"initialSteeringAxisInB", nullptr, nullptr, hkClassMember::TYPE_VECTOR4, hkClassMember::TYPE_VOID, 0, 0, 0x150,
     nullptr}
};
hkClass hkpWheelConstraintDataClass(
    "hkpWheelConstraintData",
    &hkpConstraintDataClass,
    352,
    nullptr,
    0,
    nullptr,
    0,
    hkpWheelConstraintDataClass_Members,
    3,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpWheelConstraintDataTypeInfo = {"hkpWheelConstraintData",
                                             finishLoadedObjecthkpWheelConstraintData,
                                             cleanupLoadedObjecthkpWheelConstraintData,
                                             getVtablehkpWheelConstraintData(),
                                             &typeid(hkpWheelConstraintData),
                                             sizeof(hkpWheelConstraintData)};

hkpWheelConstraintData::Atoms::Atoms() {
    m_suspensionBase.m_type.m_storage = 2;
    m_lin0Limit.m_type.m_storage = 9;
    m_lin0Soft.m_type.m_storage = 8;
    m_lin1.m_type.m_storage = 7;
    m_lin2.m_type.m_storage = 7;
    m_steeringBase.m_type.m_storage = 4;
    m_2dAng.m_type.m_storage = 12;
}

const hkClass& hkpWheelConstraintData::Atoms::staticClass() {
    return hkpWheelConstraintDataAtomsClass;
}

hkpWheelConstraintData::hkpWheelConstraintData() {
    m_atoms.m_lin0Limit.m_min = -0.5f;
    m_atoms.m_lin0Limit.m_axisIndex = 0;
    m_atoms.m_lin0Soft.m_axisIndex = 0;
    m_atoms.m_lin0Limit.m_max = 0.5f;
    m_atoms.m_lin1.m_axisIndex = 1;
    m_atoms.m_lin2.m_axisIndex = 2;
    m_atoms.m_lin0Soft.m_tau = 0.0f;
    m_atoms.m_lin0Soft.m_damping = 0.0f;
    m_atoms.m_steeringBase.m_rotationA.m_col0.m_quad = _mm_setzero_ps();
    m_atoms.m_steeringBase.m_rotationA.m_col1.m_quad = _mm_setzero_ps();
    m_atoms.m_steeringBase.m_rotationA.m_col2.m_quad = _mm_setzero_ps();
    m_atoms.m_steeringBase.m_rotationA.m_col0.m_quad[0] = 1.0f;
    m_atoms.m_steeringBase.m_rotationA.m_col1.m_quad[1] = 1.0f;
    m_atoms.m_steeringBase.m_rotationA.m_col2.m_quad[2] = 1.0f;
    m_atoms.m_steeringBase.m_rotationB.m_col0.m_quad = _mm_setzero_ps();
    m_atoms.m_steeringBase.m_rotationB.m_col1.m_quad = _mm_setzero_ps();
    m_atoms.m_steeringBase.m_rotationB.m_col2.m_quad = _mm_setzero_ps();
    m_atoms.m_steeringBase.m_rotationB.m_col0.m_quad[0] = 1.0f;
    m_atoms.m_steeringBase.m_rotationB.m_col1.m_quad[1] = 1.0f;
    m_atoms.m_steeringBase.m_rotationB.m_col2.m_quad[2] = 1.0f;
    m_atoms.m_suspensionBase.m_transformA.m_rotation.m_col0.m_quad = _mm_setzero_ps();
    m_atoms.m_suspensionBase.m_transformA.m_rotation.m_col1.m_quad = _mm_setzero_ps();
    m_atoms.m_suspensionBase.m_transformA.m_rotation.m_col2.m_quad = _mm_setzero_ps();
    m_atoms.m_suspensionBase.m_transformA.m_rotation.m_col0.m_quad[0] = 1.0f;
    m_atoms.m_suspensionBase.m_transformA.m_rotation.m_col1.m_quad[1] = 1.0f;
    m_atoms.m_suspensionBase.m_transformA.m_rotation.m_col2.m_quad[2] = 1.0f;
    m_atoms.m_suspensionBase.m_transformA.m_translation.m_quad = _mm_setzero_ps();
    m_atoms.m_suspensionBase.m_transformB.m_rotation.m_col0.m_quad = _mm_setzero_ps();
    m_atoms.m_suspensionBase.m_transformB.m_rotation.m_col1.m_quad = _mm_setzero_ps();
    m_atoms.m_suspensionBase.m_transformB.m_rotation.m_col2.m_quad = _mm_setzero_ps();
    m_atoms.m_suspensionBase.m_transformB.m_rotation.m_col0.m_quad[0] = 1.0f;
    m_atoms.m_suspensionBase.m_transformB.m_rotation.m_col1.m_quad[1] = 1.0f;
    m_atoms.m_suspensionBase.m_transformB.m_rotation.m_col2.m_quad[2] = 1.0f;
    m_atoms.m_suspensionBase.m_transformB.m_translation.m_quad = _mm_setzero_ps();
    m_atoms.m_2dAng.m_freeRotationAxis = 0;
}

// The suspension frames and both steering rotations must be orthonormal and the suspension limits ordered. Each
// steering rotation is copied into an hkTransform whose translation is hkVector4::getZero() before it is tested; the
// tests stop at the first failure.
hkBool hkpWheelConstraintData::isValid() const {
    hkTransform kSteeringA;
    kSteeringA.m_rotation = m_atoms.m_steeringBase.m_rotationA;
    kSteeringA.m_translation = hkVector4::getZero();
    hkTransform kSteeringB;
    kSteeringB.m_rotation = m_atoms.m_steeringBase.m_rotationB;
    kSteeringB.m_translation = hkVector4::getZero();
    if (m_atoms.m_suspensionBase.m_transformA.m_rotation.isOrthonormal(0.001f).m_bool &&
        m_atoms.m_suspensionBase.m_transformB.m_rotation.isOrthonormal(0.001f).m_bool &&
        kSteeringA.m_rotation.isOrthonormal(0.001f).m_bool && kSteeringB.m_rotation.isOrthonormal(0.001f).m_bool &&
        m_atoms.m_lin0Limit.m_max >= m_atoms.m_lin0Limit.m_min) {
        return 1;
    }
    return 0;
}

int32_t hkpWheelConstraintData::getType() const {
    return 9;
}

void hkpWheelConstraintData::getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const {
    if (wantRuntime.m_bool) {
        infoOut.m_numSolverResults = 6;
        infoOut.m_sizeOfExternalRuntime = 48;
    }
    else {
        infoOut.m_numSolverResults = 0;
        infoOut.m_sizeOfExternalRuntime = 0;
    }
}

void hkpWheelConstraintData::getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const {
    hkpConstraintData::getConstraintInfoUtil(&m_atoms.m_suspensionBase, 292, infoOut);
}

// Turning the wheel rotates the axle about the steering axis: the steering base rotation in body B's space gets
// cross(axis, axle) * sin + axle * cos as its first row and cross(first row, axis) as its third, and keeps its second
// (the axis itself). The cosine is the square root of 1 - sin^2, taken in double and rounded to float.
void hkpWheelConstraintData::setSteeringAngle(float angle) {
    const float fSin = std::sin(angle);
    const float fCos = std::sqrt(static_cast<float>(1.0 - static_cast<double>(fSin) * fSin));
    const hkVector4& rAxis = m_initialSteeringAxisInB;
    const hkVector4& rAxle = m_initialAxleInB;
    hkVector4 kRow0;
    kRow0.m_quad[0] =
        (rAxis.m_quad[1] * rAxle.m_quad[2] - rAxis.m_quad[2] * rAxle.m_quad[1]) * fSin + rAxle.m_quad[0] * fCos;
    kRow0.m_quad[1] =
        (rAxis.m_quad[2] * rAxle.m_quad[0] - rAxis.m_quad[0] * rAxle.m_quad[2]) * fSin + rAxle.m_quad[1] * fCos;
    kRow0.m_quad[2] =
        (rAxis.m_quad[0] * rAxle.m_quad[1] - rAxis.m_quad[1] * rAxle.m_quad[0]) * fSin + rAxle.m_quad[2] * fCos;
    kRow0.m_quad[3] = rAxle.m_quad[3] * fCos;
    m_atoms.m_steeringBase.m_rotationB.m_col0 = kRow0;
    hkVector4 kRow2;
    kRow2.m_quad[0] = rAxis.m_quad[2] * kRow0.m_quad[1] - rAxis.m_quad[1] * kRow0.m_quad[2];
    kRow2.m_quad[1] = rAxis.m_quad[0] * kRow0.m_quad[2] - rAxis.m_quad[2] * kRow0.m_quad[0];
    kRow2.m_quad[2] = rAxis.m_quad[1] * kRow0.m_quad[0] - rAxis.m_quad[0] * kRow0.m_quad[1];
    kRow2.m_quad[3] = 0.0f;
    m_atoms.m_steeringBase.m_rotationB.m_col2 = kRow2;
}

// The axle and steering axis are normalized; the suspension frames' translations are the pivot in each body's space,
// frame A's rotation the identity, frame B's the suspension axis in B's space, a normalized perpendicular of it and
// their cross product; the steering bases are the axle, the steering axis and their cross product in each body's space,
// and B's axle and steering axis are kept as the initial ones.
void hkpWheelConstraintData::setInWorldSpace(
    const hkTransform& bodyATransform,
    const hkTransform& bodyBTransform,
    const hkVector4& pivot,
    const hkVector4& axleTmp,
    const hkVector4& suspensionAxis,
    const hkVector4& steeringAxisTmp
) {
    hkVector4 axle;
    axle.m_quad = axleTmp.m_quad;
    axle.normalize3();
    hkVector4 steeringAxis;
    steeringAxis.m_quad = steeringAxisTmp.m_quad;
    steeringAxis.normalize3();
    // The suspension: the pivot in each body's space, frame A's rotation the identity.
    hkTransform& suspensionA = m_atoms.m_suspensionBase.m_transformA;
    hkTransform& suspensionB = m_atoms.m_suspensionBase.m_transformB;
    hkVector4 pivotRelative;
    pivotRelative.m_quad = _mm_sub_ps(pivot.m_quad, bodyATransform.m_translation.m_quad);
    suspensionA.m_translation.setRotatedInverseDir(bodyATransform.m_rotation, pivotRelative);
    pivotRelative.m_quad = _mm_sub_ps(pivot.m_quad, bodyBTransform.m_translation.m_quad);
    suspensionB.m_translation.setRotatedInverseDir(bodyBTransform.m_rotation, pivotRelative);
    suspensionA.m_rotation.m_col0.m_quad = _mm_setzero_ps();
    suspensionA.m_rotation.m_col1.m_quad = _mm_setzero_ps();
    suspensionA.m_rotation.m_col2.m_quad = _mm_setzero_ps();
    suspensionA.m_rotation.m_col0(0) = 1.0f;
    suspensionA.m_rotation.m_col1(1) = 1.0f;
    suspensionA.m_rotation.m_col2(2) = 1.0f;
    suspensionB.m_rotation.m_col0.setRotatedInverseDir(bodyBTransform.m_rotation, suspensionAxis);
    hkVector4Util::calculatePerpendicularVector(suspensionB.m_rotation.m_col0, suspensionB.m_rotation.m_col1);
    suspensionB.m_rotation.m_col1.normalize3();
    suspensionB.m_rotation.m_col2.setCross(suspensionB.m_rotation.m_col0, suspensionB.m_rotation.m_col1);
    // The steering bases.
    hkRotation& steeringA = m_atoms.m_steeringBase.m_rotationA;
    hkRotation& steeringB = m_atoms.m_steeringBase.m_rotationB;
    steeringA.m_col0.setRotatedInverseDir(bodyATransform.m_rotation, axle);
    steeringB.m_col0.setRotatedInverseDir(bodyBTransform.m_rotation, axle);
    m_initialAxleInB = steeringB.m_col0;
    steeringA.m_col1.setRotatedInverseDir(bodyATransform.m_rotation, steeringAxis);
    steeringB.m_col1.setRotatedInverseDir(bodyBTransform.m_rotation, steeringAxis);
    m_initialSteeringAxisInB = steeringB.m_col1;
    steeringA.m_col2.setCross(steeringA.m_col0, steeringA.m_col1);
    steeringB.m_col2.setCross(steeringB.m_col0, steeringB.m_col1);
}

void hkpWheelConstraintData::setInBodySpace(
    const hkVector4& pivotA,
    const hkVector4& pivotB,
    const hkVector4& axleA,
    const hkVector4& axleB,
    const hkVector4& suspensionAxisB,
    const hkVector4& steeringAxisB
) {
    hkVector4 v11;
    __m128 v16 = _mm_undefined_ps();
    hkVector4 v37;
    hkVector4 v42;
    hkVector4 v51;
    m_atoms.m_suspensionBase.m_transformA.m_translation.m_quad = pivotA.m_quad;
    m_atoms.m_suspensionBase.m_transformB.m_translation.m_quad = pivotB.m_quad;
    m_atoms.m_suspensionBase.m_transformA.m_rotation.m_col0.m_quad = _mm_setzero_ps();
    m_atoms.m_suspensionBase.m_transformA.m_rotation.m_col1.m_quad = _mm_setzero_ps();
    m_atoms.m_suspensionBase.m_transformA.m_rotation.m_col2.m_quad = _mm_setzero_ps();
    m_atoms.m_suspensionBase.m_transformA.m_rotation.m_col0.m_quad[0] = 1.0f;
    m_atoms.m_suspensionBase.m_transformA.m_rotation.m_col1.m_quad[1] = 1.0f;
    m_atoms.m_suspensionBase.m_transformA.m_rotation.m_col2.m_quad[2] = 1.0f;
    m_atoms.m_suspensionBase.m_transformB.m_rotation.m_col0.m_quad = suspensionAxisB.m_quad;
    float v55 = std::fabs((double)m_atoms.m_suspensionBase.m_transformB.m_rotation.m_col0.m_quad[0]);
    int32_t v7 = 0;
    int32_t v8 = 1;
    int32_t v9 = 2;
    float v57 = std::fabs((double)m_atoms.m_suspensionBase.m_transformB.m_rotation.m_col0.m_quad[1]);
    float v58 = std::fabs((double)m_atoms.m_suspensionBase.m_transformB.m_rotation.m_col0.m_quad[2]);
    if (v55 > static_cast<double>(v57)) {
        v55 = v57;
        v8 = 0;
        v7 = 1;
    }
    if (v55 > static_cast<double>(v58)) {
        v9 = v7;
    }
    m_atoms.m_suspensionBase.m_transformB.m_rotation.m_col1.m_quad = _mm_setzero_ps();
    m_atoms.m_suspensionBase.m_transformB.m_rotation.m_col1.m_quad[v8] =
        m_atoms.m_suspensionBase.m_transformB.m_rotation.m_col0.m_quad[v9];
    m_atoms.m_suspensionBase.m_transformB.m_rotation.m_col1.m_quad[v9] =
        -m_atoms.m_suspensionBase.m_transformB.m_rotation.m_col0.m_quad[v8];
    v11.m_quad = m_atoms.m_suspensionBase.m_transformB.m_rotation.m_col1.m_quad;
    __m128 v12 = _mm_mul_ps(v11.m_quad, v11.m_quad);
    float v13 = _mm_shuffle_ps(v12, v12, 85)[0] + v12[0];
    __m128 v14 = _mm_shuffle_ps(v12, v12, 170);
    v14[0] = v14[0] + v13;
    float v15 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v14[0])));
    v16[0] = std::bit_cast<float>(static_cast<int32_t>(0.0f));
    __m128 v17 = _mm_setzero_ps();
    v17[0] = 0.5f;
    __m128 v18 = _mm_cmpeq_ss(v14, v16);
    float v19 = 3.0f - ((v14[0] * v15) * v15);
    v17[0] = (0.5f * v15) * v19;
    __m128 v21 = _mm_andnot_ps(v18, v17);
    m_atoms.m_suspensionBase.m_transformB.m_rotation.m_col1.m_quad =
        _mm_mul_ps(_mm_shuffle_ps(v21, v21, 0), v11.m_quad);
    m_atoms.m_suspensionBase.m_transformB.m_rotation.m_col2.m_quad = _mm_sub_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(
                m_atoms.m_suspensionBase.m_transformB.m_rotation.m_col0.m_quad,
                m_atoms.m_suspensionBase.m_transformB.m_rotation.m_col0.m_quad, 201
            ),
            _mm_shuffle_ps(
                m_atoms.m_suspensionBase.m_transformB.m_rotation.m_col1.m_quad,
                m_atoms.m_suspensionBase.m_transformB.m_rotation.m_col1.m_quad, 210
            )
        ),
        _mm_mul_ps(
            _mm_shuffle_ps(
                m_atoms.m_suspensionBase.m_transformB.m_rotation.m_col0.m_quad,
                m_atoms.m_suspensionBase.m_transformB.m_rotation.m_col0.m_quad, 210
            ),
            _mm_shuffle_ps(
                m_atoms.m_suspensionBase.m_transformB.m_rotation.m_col1.m_quad,
                m_atoms.m_suspensionBase.m_transformB.m_rotation.m_col1.m_quad, 201
            )
        )
    );
    __m128 v22 = _mm_mul_ps(axleA.m_quad, axleA.m_quad);
    v21[0] = _mm_shuffle_ps(v22, v22, 85)[0] + v22[0];
    __m128 v23 = _mm_shuffle_ps(v22, v22, 170);
    v23[0] = v23[0] + v21[0];
    float v24 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v23[0])));
    float v26 = 3.0f - ((v23[0] * v24) * v24);
    v17[0] = (0.5f * v24) * v26;
    __m128 v28 = _mm_andnot_ps(_mm_cmpeq_ss(v23, v16), v17);
    m_atoms.m_steeringBase.m_rotationA.m_col0.m_quad = _mm_mul_ps(_mm_shuffle_ps(v28, v28, 0), axleA.m_quad);
    __m128 m_quad = axleB.m_quad;
    __m128 v30 = _mm_mul_ps(m_quad, m_quad);
    v28[0] = _mm_shuffle_ps(v30, v30, 85)[0] + v30[0];
    __m128 v31 = _mm_shuffle_ps(v30, v30, 170);
    v31[0] = v31[0] + v28[0];
    float v32 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v31[0])));
    float v34 = 3.0f - ((v31[0] * v32) * v32);
    v17[0] = (0.5f * v32) * v34;
    m_atoms.m_steeringBase.m_rotationB.m_col0.m_quad = axleB.m_quad;
    __m128 v36 = _mm_andnot_ps(_mm_cmpeq_ss(v31, v16), v17);
    v37.m_quad = _mm_mul_ps(_mm_shuffle_ps(v36, v36, 0), m_quad);
    m_atoms.m_steeringBase.m_rotationB.m_col0.m_quad = v37.m_quad;
    m_initialAxleInB.m_quad = v37.m_quad;
    int32_t v38 = 0;
    float v61 = std::fabs((double)m_atoms.m_steeringBase.m_rotationA.m_col0.m_quad[0]);
    int32_t v39 = 1;
    int32_t v40 = 2;
    float v56 = v61;
    float v62 = std::fabs((double)m_atoms.m_steeringBase.m_rotationA.m_col0.m_quad[1]);
    float v63 = std::fabs((double)m_atoms.m_steeringBase.m_rotationA.m_col0.m_quad[2]);
    if (v56 > static_cast<double>(v62)) {
        v56 = v62;
        v39 = 0;
        v38 = 1;
    }
    if (v56 > static_cast<double>(v63)) {
        v40 = v38;
    }
    m_atoms.m_steeringBase.m_rotationA.m_col1.m_quad = _mm_setzero_ps();
    m_atoms.m_steeringBase.m_rotationA.m_col1.m_quad[v39] = m_atoms.m_steeringBase.m_rotationA.m_col0.m_quad[v40];
    m_atoms.m_steeringBase.m_rotationA.m_col1.m_quad[v40] = -m_atoms.m_steeringBase.m_rotationA.m_col0.m_quad[v39];
    v42.m_quad = m_atoms.m_steeringBase.m_rotationA.m_col1.m_quad;
    __m128 v43 = _mm_mul_ps(v42.m_quad, v42.m_quad);
    float v44 = _mm_shuffle_ps(v43, v43, 85)[0] + v43[0];
    __m128 v45 = _mm_shuffle_ps(v43, v43, 170);
    v45[0] = v45[0] + v44;
    float v46 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v45[0])));
    float v47 = 3.0f - ((v45[0] * v46) * v46);
    __m128 v48 = _mm_cmpeq_ss(v45, v16);
    v17[0] = (0.5f * v46) * v47;
    __m128 v50 = _mm_andnot_ps(v48, v17);
    m_atoms.m_steeringBase.m_rotationA.m_col1.m_quad = _mm_mul_ps(_mm_shuffle_ps(v50, v50, 0), v42.m_quad);
    m_atoms.m_steeringBase.m_rotationB.m_col1.m_quad = steeringAxisB.m_quad;
    v51.m_quad = m_atoms.m_steeringBase.m_rotationB.m_col1.m_quad;
    __m128 v52 = _mm_mul_ps(v51.m_quad, v51.m_quad);
    v50[0] = _mm_shuffle_ps(v52, v52, 85)[0] + v52[0];
    __m128 v53 = _mm_shuffle_ps(v52, v52, 170);
    v53[0] = v53[0] + v50[0];
    v50[0] = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v53[0])));
    v17[0] = (0.5f * v50[0]) * (3.0f - ((v53[0] * v50[0]) * v50[0]));
    __m128 v54 = _mm_andnot_ps(_mm_cmpeq_ss(v53, v16), v17);
    m_atoms.m_steeringBase.m_rotationB.m_col1.m_quad = _mm_mul_ps(_mm_shuffle_ps(v54, v54, 0), v51.m_quad);
    m_initialSteeringAxisInB = m_atoms.m_steeringBase.m_rotationB.m_col1;
    m_atoms.m_steeringBase.m_rotationA.m_col2.m_quad = _mm_sub_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(
                m_atoms.m_steeringBase.m_rotationA.m_col0.m_quad, m_atoms.m_steeringBase.m_rotationA.m_col0.m_quad, 201
            ),
            _mm_shuffle_ps(
                m_atoms.m_steeringBase.m_rotationA.m_col1.m_quad, m_atoms.m_steeringBase.m_rotationA.m_col1.m_quad, 210
            )
        ),
        _mm_mul_ps(
            _mm_shuffle_ps(
                m_atoms.m_steeringBase.m_rotationA.m_col0.m_quad, m_atoms.m_steeringBase.m_rotationA.m_col0.m_quad, 210
            ),
            _mm_shuffle_ps(
                m_atoms.m_steeringBase.m_rotationA.m_col1.m_quad, m_atoms.m_steeringBase.m_rotationA.m_col1.m_quad, 201
            )
        )
    );
    m_atoms.m_steeringBase.m_rotationB.m_col2.m_quad = _mm_sub_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(
                m_atoms.m_steeringBase.m_rotationB.m_col1.m_quad, m_atoms.m_steeringBase.m_rotationB.m_col1.m_quad, 210
            ),
            _mm_shuffle_ps(
                m_atoms.m_steeringBase.m_rotationB.m_col0.m_quad, m_atoms.m_steeringBase.m_rotationB.m_col0.m_quad, 201
            )
        ),
        _mm_mul_ps(
            _mm_shuffle_ps(
                m_atoms.m_steeringBase.m_rotationB.m_col1.m_quad, m_atoms.m_steeringBase.m_rotationB.m_col1.m_quad, 201
            ),
            _mm_shuffle_ps(
                m_atoms.m_steeringBase.m_rotationB.m_col0.m_quad, m_atoms.m_steeringBase.m_rotationB.m_col0.m_quad, 210
            )
        )
    );
}

const hkClass& hkpWheelConstraintData::staticClass() {
    return hkpWheelConstraintDataClass;
}
