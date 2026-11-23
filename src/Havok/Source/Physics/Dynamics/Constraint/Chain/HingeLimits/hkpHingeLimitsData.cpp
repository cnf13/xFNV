#include "Havok/Source/Physics/Dynamics/Constraint/Chain/HingeLimits/hkpHingeLimitsData.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkp2dAngConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalRotationsConstraintAtom.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

static void cleanupLoadedObjecthkpHingeLimitsDataAtoms(void* p);
static void cleanupLoadedObjecthkpHingeLimitsData(void* p);
static void finishLoadedObjecthkpHingeLimitsDataAtoms(void* p, int32_t finishing);
static void finishLoadedObjecthkpHingeLimitsData(void* p, int32_t finishing);
// Havok SDK signature (hkpDynamics.lib, tied by reloc,target).
static const void* getVtablehkpHingeLimitsData();

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpHingeLimitsDataAtomsTypeInfo = {
    "hkpHingeLimitsDataAtoms",
    finishLoadedObjecthkpHingeLimitsDataAtoms,
    cleanupLoadedObjecthkpHingeLimitsDataAtoms,
    nullptr,
    &typeid(hkpHingeLimitsData::Atoms),
    sizeof(hkpHingeLimitsData::Atoms)
};

// The reflection of hkpHingeLimitsData::Atoms and hkpHingeLimitsData.
using hkpHingeLimitsDataAtomsAxisEnumItemsArray = const hkClassEnum::Item[3];
static hkpHingeLimitsDataAtomsAxisEnumItemsArray hkpHingeLimitsDataAtomsAxisEnumItems{
    {hkpHingeLimitsData::Atoms::AXIS_AXLE, "AXIS_AXLE"},
    {hkpHingeLimitsData::Atoms::AXIS_PERP_TO_AXLE_1, "AXIS_PERP_TO_AXLE_1"},
    {hkpHingeLimitsData::Atoms::AXIS_PERP_TO_AXLE_2, "AXIS_PERP_TO_AXLE_2"}
};
using hkpHingeLimitsDataAtomsEnumsArray = const hkClassEnum[1];
static hkpHingeLimitsDataAtomsEnumsArray hkpHingeLimitsDataAtomsEnums{
    {"Axis", hkpHingeLimitsDataAtomsAxisEnumItems, 3, nullptr, {0}}
};
using hkpHingeLimitsDataAtomsClass_MembersArray = const hkClassMember[3];
static hkpHingeLimitsDataAtomsClass_MembersArray hkpHingeLimitsDataAtomsClass_Members{
    {"rotations",
     &hkpSetLocalRotationsConstraintAtomClass,
     nullptr,
     hkClassMember::TYPE_STRUCT,
     hkClassMember::TYPE_VOID,
     0,
     0,
     0x0,
     nullptr},
    {"angLimit",
     &hkpAngLimitConstraintAtomClass,
     nullptr,
     hkClassMember::TYPE_STRUCT,
     hkClassMember::TYPE_VOID,
     0,
     0,
     0x70,
     nullptr},
    {"2dAng",
     &hkp2dAngConstraintAtomClass,
     nullptr,
     hkClassMember::TYPE_STRUCT,
     hkClassMember::TYPE_VOID,
     0,
     0,
     0x80,
     nullptr}
};
hkClass hkpHingeLimitsDataAtomsClass(
    "hkpHingeLimitsDataAtoms",
    nullptr,
    144,
    nullptr,
    0,
    hkpHingeLimitsDataAtomsEnums,
    1,
    hkpHingeLimitsDataAtomsClass_Members,
    3,
    nullptr,
    nullptr,
    0,
    0
);
using hkpHingeLimitsDataClass_MembersArray = const hkClassMember[1];
static hkpHingeLimitsDataClass_MembersArray hkpHingeLimitsDataClass_Members{
    {"atoms",
     &hkpHingeLimitsDataAtomsClass,
     nullptr,
     hkClassMember::TYPE_STRUCT,
     hkClassMember::TYPE_VOID,
     0,
     {hkClassMember::ALIGN_16},
     0x10,
     nullptr}
};
hkClass hkpHingeLimitsDataClass(
    "hkpHingeLimitsData",
    &hkpConstraintDataClass,
    160,
    nullptr,
    0,
    nullptr,
    0,
    hkpHingeLimitsDataClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpHingeLimitsDataTypeInfo = {
    "hkpHingeLimitsData",
    finishLoadedObjecthkpHingeLimitsData,
    cleanupLoadedObjecthkpHingeLimitsData,
    getVtablehkpHingeLimitsData(),
    &typeid(hkpHingeLimitsData),
    sizeof(hkpHingeLimitsData)
};

static void cleanupLoadedObjecthkpHingeLimitsDataAtoms(void* p) {
    ;
}

static void cleanupLoadedObjecthkpHingeLimitsData(void* p) {
    static_cast<hkpHingeLimitsData*>(p)->~hkpHingeLimitsData();
}

static void finishLoadedObjecthkpHingeLimitsDataAtoms(void* p, int32_t finishing) {
    ;
}

// The type info's finish-loaded-object callback for hkpHingeLimitsData: the null check, then its finish constructor.
static void finishLoadedObjecthkpHingeLimitsData(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag aFlag;
        aFlag.m_finishing = finishing;
        ::new (p) hkpHingeLimitsData(aFlag);
    }
}

static const void* getVtablehkpHingeLimitsData() {
    return hkGetVtableOf<hkpHingeLimitsData>();
}

const hkClass& hkpHingeLimitsData::Atoms::staticClass() {
    return hkpHingeLimitsDataAtomsClass;
}

hkpHingeLimitsData::hkpHingeLimitsData() {
    m_atoms.m_rotations.m_type.m_storage = 4;
    m_atoms.m_angLimit.m_isEnabled = 1;
    m_atoms.m_angLimit.m_type.m_storage = 14;
    m_atoms.m_2dAng.m_type.m_storage = 12;
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
    m_atoms.m_2dAng.m_freeRotationAxis = 0;
    m_atoms.m_angLimit.m_limitAxis = 0;
    m_atoms.m_angLimit.m_minAngle = -3.1415927f;
    m_atoms.m_angLimit.m_maxAngle = 3.1415927f;
    m_atoms.m_angLimit.m_angularLimitsTauFactor = 1.0f;
}

hkBool hkpHingeLimitsData::isValid() const {
    if (m_atoms.m_rotations.m_rotationA.isOrthonormal(0.001f).m_bool &&
        m_atoms.m_rotations.m_rotationB.isOrthonormal(0.001f).m_bool) {
        return 1;
    }

    return 0;
}

int32_t hkpHingeLimitsData::getType() const {
    return 18;
}

void hkpHingeLimitsData::getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const {
    infoOut.m_numSolverResults = 3;
    infoOut.m_sizeOfExternalRuntime = 28;
}

void hkpHingeLimitsData::getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const {
    hkpConstraintData::getConstraintInfoUtil(&m_atoms.m_rotations, 132, infoOut);
}

void hkpHingeLimitsData::setInWorldSpace(
    const hkTransform& bodyATransform,
    const hkTransform& bodyBTransform,
    const hkVector4& axis
) {
    __m128 v13 = _mm_undefined_ps();
    hkVector4 arVector;
    hkVector4 v24;
    int32_t v4 = 0;
    float v19 = std::fabs(static_cast<double>(XFNV_M128_F32(axis.m_quad, 0)));
    int32_t v6 = 1;
    int32_t v7 = 2;
    float v20 = std::fabs(static_cast<double>(XFNV_M128_F32(axis.m_quad, 1)));
    float v21 = std::fabs(static_cast<double>(XFNV_M128_F32(axis.m_quad, 2)));
    if (v19 > static_cast<double>(v20)) {
        v19 = v20;
        v6 = 0;
        v4 = 1;
    }
    if (v19 > static_cast<double>(v21)) {
        v7 = v4;
    }
    double v8 = XFNV_M128_F32(axis.m_quad, v7);
    arVector.m_quad = _mm_setzero_ps();
    XFNV_M128_F32(arVector.m_quad, v6) = v8;
    XFNV_M128_F32(arVector.m_quad, v7) = -XFNV_M128_F32(axis.m_quad, v6);
    __m128 v9 = _mm_mul_ps(arVector.m_quad, arVector.m_quad);
    float v10 = XFNV_M128_F32(_mm_shuffle_ps(v9, v9, 85), 0) + XFNV_M128_F32(v9, 0);
    __m128 v11 = _mm_shuffle_ps(v9, v9, 170);
    XFNV_M128_F32(v11, 0) = XFNV_M128_F32(v11, 0) + v10;
    float v12 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(v11, 0))));
    XFNV_M128_F32(v13, 0) = std::bit_cast<float>(static_cast<int32_t>(0.0f));
    __m128 v14 = _mm_cmpeq_ss(v11, v13);
    XFNV_M128_F32(v13, 0) = 3.0f - ((XFNV_M128_F32(v11, 0) * v12) * v12);
    __m128 v15 = _mm_setzero_ps();
    XFNV_M128_F32(v15, 0) = (0.5f * v12) * XFNV_M128_F32(v13, 0);
    __m128 v16 = _mm_andnot_ps(v14, v15);
    __m128 v17 = _mm_shuffle_ps(v16, v16, 0);
    __m128 m_quad = axis.m_quad;
    arVector.m_quad = _mm_mul_ps(v17, arVector.m_quad);
    v24.m_quad = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(m_quad, m_quad, 201), _mm_shuffle_ps(arVector.m_quad, arVector.m_quad, 210)),
        _mm_mul_ps(_mm_shuffle_ps(m_quad, m_quad, 210), _mm_shuffle_ps(arVector.m_quad, arVector.m_quad, 201))
    );
    m_atoms.m_rotations.m_rotationA.m_col0.setRotatedInverseDir(bodyATransform.m_rotation, axis);
    m_atoms.m_rotations.m_rotationA.m_col1.setRotatedInverseDir(bodyATransform.m_rotation, arVector);
    m_atoms.m_rotations.m_rotationA.m_col2.setRotatedInverseDir(bodyATransform.m_rotation, v24);
    m_atoms.m_rotations.m_rotationB.m_col0.setRotatedInverseDir(bodyBTransform.m_rotation, axis);
    m_atoms.m_rotations.m_rotationB.m_col1.setRotatedInverseDir(bodyBTransform.m_rotation, arVector);
    m_atoms.m_rotations.m_rotationB.m_col2.setRotatedInverseDir(bodyBTransform.m_rotation, v24);
}

void hkpHingeLimitsData::setInBodySpace(
    const hkVector4& axisA,
    const hkVector4& axisB,
    const hkVector4& axisAPerp,
    const hkVector4& axisBPerp
) {
    hkVector4 v5;
    __m128 v10 = _mm_undefined_ps();
    hkVector4 v16;
    hkVector4 v24;
    hkVector4 v32;
    m_atoms.m_rotations.m_rotationA.m_col0.m_quad = axisA.m_quad;
    v5.m_quad = m_atoms.m_rotations.m_rotationA.m_col0.m_quad;
    __m128 v6 = _mm_mul_ps(v5.m_quad, v5.m_quad);
    float v7 = XFNV_M128_F32(_mm_shuffle_ps(v6, v6, 85), 0) + XFNV_M128_F32(v6, 0);
    __m128 v8 = _mm_shuffle_ps(v6, v6, 170);
    XFNV_M128_F32(v8, 0) = XFNV_M128_F32(v8, 0) + v7;
    float v9 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(v8, 0))));
    XFNV_M128_F32(v10, 0) = std::bit_cast<float>(static_cast<int32_t>(0.0f));
    __m128 v11 = _mm_setzero_ps();
    XFNV_M128_F32(v11, 0) = 0.5f;
    __m128 v12 = _mm_cmpeq_ss(v8, v10);
    float v13 = 3.0f - ((XFNV_M128_F32(v8, 0) * v9) * v9);
    XFNV_M128_F32(v11, 0) = (0.5f * v9) * v13;
    __m128 v15 = _mm_andnot_ps(v12, v11);
    m_atoms.m_rotations.m_rotationA.m_col0.m_quad = _mm_mul_ps(_mm_shuffle_ps(v15, v15, 0), v5.m_quad);
    m_atoms.m_rotations.m_rotationA.m_col1.m_quad = axisAPerp.m_quad;
    v16.m_quad = m_atoms.m_rotations.m_rotationA.m_col1.m_quad;
    __m128 v17 = _mm_mul_ps(v16.m_quad, v16.m_quad);
    XFNV_M128_F32(v15, 0) = XFNV_M128_F32(_mm_shuffle_ps(v17, v17, 85), 0) + XFNV_M128_F32(v17, 0);
    __m128 v18 = _mm_shuffle_ps(v17, v17, 170);
    XFNV_M128_F32(v18, 0) = XFNV_M128_F32(v18, 0) + XFNV_M128_F32(v15, 0);
    float v19 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(v18, 0))));
    float v21 = 3.0f - ((XFNV_M128_F32(v18, 0) * v19) * v19);
    XFNV_M128_F32(v11, 0) = (0.5f * v19) * v21;
    __m128 v23 = _mm_andnot_ps(_mm_cmpeq_ss(v18, v10), v11);
    m_atoms.m_rotations.m_rotationA.m_col1.m_quad = _mm_mul_ps(_mm_shuffle_ps(v23, v23, 0), v16.m_quad);
    m_atoms.m_rotations.m_rotationA.m_col2.m_quad = _mm_sub_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(axisAPerp.m_quad, axisAPerp.m_quad, 210),
            _mm_shuffle_ps(axisA.m_quad, axisA.m_quad, 201)
        ),
        _mm_mul_ps(
            _mm_shuffle_ps(axisAPerp.m_quad, axisAPerp.m_quad, 201),
            _mm_shuffle_ps(axisA.m_quad, axisA.m_quad, 210)
        )
    );
    m_atoms.m_rotations.m_rotationB.m_col0.m_quad = axisB.m_quad;
    v24.m_quad = m_atoms.m_rotations.m_rotationB.m_col0.m_quad;
    __m128 v25 = _mm_mul_ps(v24.m_quad, v24.m_quad);
    XFNV_M128_F32(v23, 0) = XFNV_M128_F32(_mm_shuffle_ps(v25, v25, 85), 0) + XFNV_M128_F32(v25, 0);
    __m128 v26 = _mm_shuffle_ps(v25, v25, 170);
    XFNV_M128_F32(v26, 0) = XFNV_M128_F32(v26, 0) + XFNV_M128_F32(v23, 0);
    float v27 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(v26, 0))));
    __m128 v28 = _mm_cmpeq_ss(v26, v10);
    float v29 = 3.0f - ((XFNV_M128_F32(v26, 0) * v27) * v27);
    XFNV_M128_F32(v11, 0) = (0.5f * v27) * v29;
    __m128 v31 = _mm_andnot_ps(v28, v11);
    m_atoms.m_rotations.m_rotationB.m_col0.m_quad = _mm_mul_ps(_mm_shuffle_ps(v31, v31, 0), v24.m_quad);
    m_atoms.m_rotations.m_rotationB.m_col1.m_quad = axisBPerp.m_quad;
    v32.m_quad = m_atoms.m_rotations.m_rotationB.m_col1.m_quad;
    __m128 v33 = _mm_mul_ps(v32.m_quad, v32.m_quad);
    XFNV_M128_F32(v31, 0) = XFNV_M128_F32(_mm_shuffle_ps(v33, v33, 85), 0) + XFNV_M128_F32(v33, 0);
    __m128 v34 = _mm_shuffle_ps(v33, v33, 170);
    XFNV_M128_F32(v34, 0) = XFNV_M128_F32(v34, 0) + XFNV_M128_F32(v31, 0);
    XFNV_M128_F32(v31, 0) = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(v34, 0))));
    XFNV_M128_F32(v11, 0) = (0.5f * XFNV_M128_F32(v31, 0)) *
        (3.0f - ((XFNV_M128_F32(v34, 0) * XFNV_M128_F32(v31, 0)) * XFNV_M128_F32(v31, 0)));
    __m128 v35 = _mm_andnot_ps(_mm_cmpeq_ss(v34, v10), v11);
    m_atoms.m_rotations.m_rotationB.m_col1.m_quad = _mm_mul_ps(_mm_shuffle_ps(v35, v35, 0), v32.m_quad);
    m_atoms.m_rotations.m_rotationB.m_col2.m_quad = _mm_sub_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(axisB.m_quad, axisB.m_quad, 201),
            _mm_shuffle_ps(axisBPerp.m_quad, axisBPerp.m_quad, 210)
        ),
        _mm_mul_ps(
            _mm_shuffle_ps(axisB.m_quad, axisB.m_quad, 210),
            _mm_shuffle_ps(axisBPerp.m_quad, axisBPerp.m_quad, 201)
        )
    );
}

const hkClass& hkpHingeLimitsData::staticClass() {
    return hkpHingeLimitsDataClass;
}
