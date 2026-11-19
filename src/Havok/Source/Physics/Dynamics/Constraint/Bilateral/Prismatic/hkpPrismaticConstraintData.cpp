#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Prismatic/hkpPrismaticConstraintData.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinFrictionConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinMotorConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTransformsConstraintAtom.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <xmmintrin.h>

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpPrismaticConstraintDataAtomsTypeInfo = {
    "hkpPrismaticConstraintDataAtoms",
    finishLoadedObjecthkpPrismaticConstraintDataAtoms,
    cleanupLoadedObjecthkpPrismaticConstraintDataAtoms,
    nullptr,
    &typeid(hkpPrismaticConstraintData::Atoms),
    sizeof(hkpPrismaticConstraintData::Atoms)
};

// The reflected enum and members of hkpPrismaticConstraintData and its atoms.
using hkpPrismaticConstraintDataAtomsAxisEnumItemsArray = const hkClassEnum::Item[2];
static hkpPrismaticConstraintDataAtomsAxisEnumItemsArray hkpPrismaticConstraintDataAtomsAxisEnumItems{
    {hkpPrismaticConstraintData::Atoms::AXIS_SHAFT, "AXIS_SHAFT"},
    {hkpPrismaticConstraintData::Atoms::AXIS_PERP_TO_SHAFT, "AXIS_PERP_TO_SHAFT"}
};
using hkpPrismaticConstraintDataAtomsEnumsArray = const hkClassEnum[1];
static hkpPrismaticConstraintDataAtomsEnumsArray hkpPrismaticConstraintDataAtomsEnums{
    {"Axis", hkpPrismaticConstraintDataAtomsAxisEnumItems, 2, nullptr, {0}}
};
using hkpPrismaticConstraintData_AtomsClass_MembersArray = const hkClassMember[7];
static hkpPrismaticConstraintData_AtomsClass_MembersArray hkpPrismaticConstraintData_AtomsClass_Members{
    {"transforms",
     &hkpSetLocalTransformsConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x0,
     nullptr},
    {"motor",
     &hkpLinMotorConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x90,
     nullptr},
    {"friction",
     &hkpLinFrictionConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xA0,
     nullptr},
    {"ang",
     &hkpAngConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xA8,
     nullptr},
    {"lin0",
     &hkpLinConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xAC,
     nullptr},
    {"lin1",
     &hkpLinConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xB0,
     nullptr},
    {"linLimit",
     &hkpLinLimitConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xB4,
     nullptr}
};
using hkpPrismaticConstraintDataClass_MembersArray = const hkClassMember[1];
static hkpPrismaticConstraintDataClass_MembersArray hkpPrismaticConstraintDataClass_Members{
    {"atoms",
     &hkpPrismaticConstraintDataAtomsClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::ALIGN_16},
     0x10,
     nullptr}
};
hkClass hkpPrismaticConstraintDataAtomsClass(
    "hkpPrismaticConstraintDataAtoms",
    nullptr,
    192,
    nullptr,
    0,
    hkpPrismaticConstraintDataAtomsEnums,
    1,
    hkpPrismaticConstraintData_AtomsClass_Members,
    7,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkpPrismaticConstraintDataClass(
    "hkpPrismaticConstraintData",
    &hkpConstraintDataClass,
    208,
    nullptr,
    0,
    nullptr,
    0,
    hkpPrismaticConstraintDataClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpPrismaticConstraintDataTypeInfo = {
    "hkpPrismaticConstraintData",
    finishLoadedObjecthkpPrismaticConstraintData,
    cleanupLoadedObjecthkpPrismaticConstraintData,
    getVtablehkpPrismaticConstraintData(),
    &typeid(hkpPrismaticConstraintData),
    sizeof(hkpPrismaticConstraintData)
};

hkpPrismaticConstraintData::Atoms::Atoms() {
    m_transforms.m_type.m_storage = (uint16_t)2;
    m_motor.m_type.m_storage = (uint16_t)11;
    m_motor.m_isEnabled.m_bool = 1;
    m_motor.m_initializedOffset = 0;
    m_motor.m_previousTargetPositionOffset = 0;
    m_friction.m_isEnabled = (uint8_t)1;
    m_friction.m_type.m_storage = (uint16_t)10;
    m_ang.m_type.m_storage = (uint16_t)13;
    m_lin0.m_type.m_storage = (uint16_t)7;
    m_lin1.m_type.m_storage = (uint16_t)7;
    m_linLimit.m_type.m_storage = (uint16_t)9;
}

const hkClass& hkpPrismaticConstraintData::Atoms::staticClass() {
    return hkpPrismaticConstraintDataAtomsClass;
}

hkpPrismaticConstraintData::hkpPrismaticConstraintData() {
    m_atoms.m_transforms.m_transformA.m_rotation.m_col0.m_quad = _mm_setzero_ps();
    m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad = _mm_setzero_ps();
    m_atoms.m_transforms.m_transformA.m_rotation.m_col2.m_quad = _mm_setzero_ps();
    XFNV_M128_F32(m_atoms.m_transforms.m_transformA.m_rotation.m_col0.m_quad, 0) = 1.0f;
    XFNV_M128_F32(m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad, 1) = 1.0f;
    XFNV_M128_F32(m_atoms.m_transforms.m_transformA.m_rotation.m_col2.m_quad, 2) = 1.0f;
    m_atoms.m_transforms.m_transformA.m_translation.m_quad = _mm_setzero_ps();
    m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad = _mm_setzero_ps();
    m_atoms.m_transforms.m_transformB.m_rotation.m_col1.m_quad = _mm_setzero_ps();
    m_atoms.m_transforms.m_transformB.m_rotation.m_col2.m_quad = _mm_setzero_ps();
    XFNV_M128_F32(m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad, 0) = 1.0f;
    XFNV_M128_F32(m_atoms.m_transforms.m_transformB.m_rotation.m_col1.m_quad, 1) = 1.0f;
    XFNV_M128_F32(m_atoms.m_transforms.m_transformB.m_rotation.m_col2.m_quad, 2) = 1.0f;
    m_atoms.m_transforms.m_transformB.m_translation.m_quad = _mm_setzero_ps();
    m_atoms.m_linLimit.m_axisIndex = 0;
    m_atoms.m_linLimit.m_min = -3.40282e+38f;
    m_atoms.m_friction.m_frictionAxis = 0;
    m_atoms.m_linLimit.m_max = 3.40282e+38f;
    m_atoms.m_friction.m_maxFrictionForce = 0.0f;
    m_atoms.m_motor.m_isEnabled.m_bool = 0;
    m_atoms.m_motor.m_targetPosition = 0.0f;
    m_atoms.m_motor.m_motor = nullptr;
    m_atoms.m_motor.m_motorAxis = 0;
    m_atoms.m_ang.m_firstConstrainedAxis = 0;
    m_atoms.m_motor.m_initializedOffset = 64;
    m_atoms.m_motor.m_previousTargetPositionOffset = 68;
    m_atoms.m_ang.m_numConstrainedAxes = 3;
    m_atoms.m_lin0.m_axisIndex = 1;
    m_atoms.m_lin1.m_axisIndex = 2;
}

hkpPrismaticConstraintData::~hkpPrismaticConstraintData() {
    if (m_atoms.m_motor.m_motor) {
        m_atoms.m_motor.m_motor->removeReference();
    }
}

hkBool hkpPrismaticConstraintData::isValid() const {
    if (m_atoms.m_transforms.m_transformA.m_rotation.isOrthonormal(0.001f).m_bool &&
        (m_atoms.m_transforms.m_transformB.m_rotation.isOrthonormal(0.001f).m_bool &&
         m_atoms.m_linLimit.m_max >= static_cast<double>(m_atoms.m_linLimit.m_min))) {
        return 1;
    }

    return 0;
}

int32_t hkpPrismaticConstraintData::getType() const {
    return 6;
}

void hkpPrismaticConstraintData::getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const {
    (&infoOut)->m_numSolverResults = 8;
    (&infoOut)->m_sizeOfExternalRuntime = 72;
}

// The atoms' span up to the end of the last atom: x64's tail padding after it is no atom.
void hkpPrismaticConstraintData::getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const {
    getConstraintInfoUtil(m_atoms.getAtoms(), m_atoms.getSizeOfAllAtoms(), infoOut);
}

void hkpPrismaticConstraintData::setMotor(hkpConstraintMotor* motor) {
    if (motor) {
        motor->addReference();
    }
    if (m_atoms.m_motor.m_motor) {
        m_atoms.m_motor.m_motor->removeReference();
    }
    m_atoms.m_motor.m_motor = motor;
}

// The constraint's base in world space is the axis normalized, a perpendicular of it normalized and their cross
// product; each body's frame rotation is its rotation's transpose times that base, its translation the pivot in the
// body's space.
void hkpPrismaticConstraintData::setInWorldSpace(
    const hkTransform& bodyATransform,
    const hkTransform& bodyBTransform,
    const hkVector4& pivot,
    const hkVector4& axis
) {
    hkRotation constraintBaseInW;
    constraintBaseInW.m_col0 = axis;
    constraintBaseInW.m_col0.normalize3();
    hkVector4Util::calculatePerpendicularVector(constraintBaseInW.m_col0, constraintBaseInW.m_col1);
    constraintBaseInW.m_col1.normalize3();
    constraintBaseInW.m_col2.setCross(constraintBaseInW.m_col0, constraintBaseInW.m_col1);
    m_atoms.m_transforms.m_transformA.m_rotation.setMulInverseMul(bodyATransform.m_rotation, constraintBaseInW);
    m_atoms.m_transforms.m_transformB.m_rotation.setMulInverseMul(bodyBTransform.m_rotation, constraintBaseInW);
    m_atoms.m_transforms.m_transformA.m_translation.setTransformedInversePos(bodyATransform, pivot);
    m_atoms.m_transforms.m_transformB.m_translation.setTransformedInversePos(bodyBTransform, pivot);
}

void hkpPrismaticConstraintData::setMotorActive(hkpConstraintInstance* instance, hkBool toBeEnabled) {
    m_atoms.m_motor.m_isEnabled = toBeEnabled;
    m_atoms.m_friction.m_isEnabled = toBeEnabled.m_bool == 0;
    if (instance) {
        hkConstraintInternal* m_internal = instance->getInternal();
        if (m_internal) {
            float* m_runtime = static_cast<float*>(m_internal->m_runtime);
            if (m_runtime) {
                *m_runtime = 0.0f;
                m_runtime[1] = 0.0f;
                m_runtime[2] = 0.0f;
                m_runtime[3] = 0.0f;
            }
        }
    }
}

void hkpPrismaticConstraintData::setInBodySpace(
    const hkVector4& pivotA,
    const hkVector4& pivotB,
    const hkVector4& axisA,
    const hkVector4& axisB,
    const hkVector4& axisAPerp,
    const hkVector4& axisBPerp
) {
    __m128 v11 = _mm_undefined_ps();
    hkVector4 v17;
    hkVector4 v32;
    m_atoms.m_transforms.m_transformA.m_translation.m_quad = pivotA.m_quad;
    m_atoms.m_transforms.m_transformB.m_translation.m_quad = pivotB.m_quad;
    __m128 v7 = _mm_mul_ps(axisA.m_quad, axisA.m_quad);
    float v8 = XFNV_M128_F32(_mm_shuffle_ps(v7, v7, 85), 0) + XFNV_M128_F32(v7, 0);
    __m128 v9 = _mm_shuffle_ps(v7, v7, 170);
    XFNV_M128_F32(v9, 0) = XFNV_M128_F32(v9, 0) + v8;
    float v10 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(v9, 0))));
    XFNV_M128_F32(v11, 0) = std::bit_cast<float>(static_cast<int32_t>(0.0f));
    __m128 v12 = _mm_setzero_ps();
    XFNV_M128_F32(v12, 0) = 0.5f;
    __m128 v13 = _mm_cmpeq_ss(v9, v11);
    float v14 = 3.0f - ((XFNV_M128_F32(v9, 0) * v10) * v10);
    XFNV_M128_F32(v12, 0) = (0.5f * v10) * v14;
    __m128 v16 = _mm_andnot_ps(v13, v12);
    m_atoms.m_transforms.m_transformA.m_rotation.m_col0.m_quad = _mm_mul_ps(_mm_shuffle_ps(v16, v16, 0), axisA.m_quad);
    m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad = axisAPerp.m_quad;
    v17.m_quad = m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad;
    __m128 v18 = _mm_mul_ps(v17.m_quad, v17.m_quad);
    XFNV_M128_F32(v16, 0) = XFNV_M128_F32(_mm_shuffle_ps(v18, v18, 85), 0) + XFNV_M128_F32(v18, 0);
    __m128 v19 = _mm_shuffle_ps(v18, v18, 170);
    XFNV_M128_F32(v19, 0) = XFNV_M128_F32(v19, 0) + XFNV_M128_F32(v16, 0);
    float v20 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(v19, 0))));
    float v22 = 3.0f - ((XFNV_M128_F32(v19, 0) * v20) * v20);
    XFNV_M128_F32(v12, 0) = (0.5f * v20) * v22;
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
    __m128 v25 = _mm_mul_ps(axisB.m_quad, axisB.m_quad);
    XFNV_M128_F32(v24, 0) = XFNV_M128_F32(_mm_shuffle_ps(v25, v25, 85), 0) + XFNV_M128_F32(v25, 0);
    __m128 v26 = _mm_shuffle_ps(v25, v25, 170);
    XFNV_M128_F32(v26, 0) = XFNV_M128_F32(v26, 0) + XFNV_M128_F32(v24, 0);
    float v27 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(v26, 0))));
    __m128 v28 = _mm_cmpeq_ss(v26, v11);
    float v29 = 3.0f - ((XFNV_M128_F32(v26, 0) * v27) * v27);
    XFNV_M128_F32(v12, 0) = (0.5f * v27) * v29;
    __m128 v31 = _mm_andnot_ps(v28, v12);
    m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad = _mm_mul_ps(_mm_shuffle_ps(v31, v31, 0), axisB.m_quad);
    m_atoms.m_transforms.m_transformB.m_rotation.m_col1.m_quad = axisBPerp.m_quad;
    v32.m_quad = m_atoms.m_transforms.m_transformB.m_rotation.m_col1.m_quad;
    __m128 v33 = _mm_mul_ps(v32.m_quad, v32.m_quad);
    XFNV_M128_F32(v31, 0) = XFNV_M128_F32(_mm_shuffle_ps(v33, v33, 85), 0) + XFNV_M128_F32(v33, 0);
    __m128 v34 = _mm_shuffle_ps(v33, v33, 170);
    XFNV_M128_F32(v34, 0) = XFNV_M128_F32(v34, 0) + XFNV_M128_F32(v31, 0);
    XFNV_M128_F32(v31, 0) = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(v34, 0))));
    XFNV_M128_F32(v12, 0) = (0.5f * XFNV_M128_F32(v31, 0)) *
        (3.0f - ((XFNV_M128_F32(v34, 0) * XFNV_M128_F32(v31, 0)) * XFNV_M128_F32(v31, 0)));
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
}

const hkClass& hkpPrismaticConstraintData::staticClass() {
    return hkpPrismaticConstraintDataClass;
}
