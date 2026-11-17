#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPlane/hkpPointToPlaneConstraintData.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTransformsConstraintAtom.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

static void cleanupLoadedObjecthkpPointToPlaneConstraintDataAtoms(void* p);
static void finishLoadedObjecthkpPointToPlaneConstraintDataAtoms(void* p, int32_t finishing);
static void finishLoadedObjecthkpPointToPlaneConstraintData(void* p, int32_t finishing);
// Havok SDK signature (hkpDynamics.lib, tied by reloc,target).
static const void* getVtablehkpPointToPlaneConstraintData();
static void cleanupLoadedObjecthkpPointToPlaneConstraintData(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpPointToPlaneConstraintDataAtomsTypeInfo = {
    "hkpPointToPlaneConstraintDataAtoms",
    finishLoadedObjecthkpPointToPlaneConstraintDataAtoms,
    cleanupLoadedObjecthkpPointToPlaneConstraintDataAtoms,
    nullptr,
    &typeid(hkpPointToPlaneConstraintData::Atoms),
    sizeof(hkpPointToPlaneConstraintData::Atoms)
};

// The reflected members of hkpPointToPlaneConstraintData and its atoms.
using hkpPointToPlaneConstraintData_AtomsClass_MembersArray = const hkClassMember[2];
static hkpPointToPlaneConstraintData_AtomsClass_MembersArray hkpPointToPlaneConstraintData_AtomsClass_Members{
    {"transforms",
     &hkpSetLocalTransformsConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x0,
     nullptr},
    {"lin",
     &hkpLinConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x90,
     nullptr}
};
using hkpPointToPlaneConstraintDataClass_MembersArray = const hkClassMember[1];
static hkpPointToPlaneConstraintDataClass_MembersArray hkpPointToPlaneConstraintDataClass_Members{
    {"atoms",
     &hkpPointToPlaneConstraintDataAtomsClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::ALIGN_16},
     0x10,
     nullptr}
};
hkClass hkpPointToPlaneConstraintDataAtomsClass(
    "hkpPointToPlaneConstraintDataAtoms",
    nullptr,
    160,
    nullptr,
    0,
    nullptr,
    0,
    hkpPointToPlaneConstraintData_AtomsClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkpPointToPlaneConstraintDataClass(
    "hkpPointToPlaneConstraintData",
    &hkpConstraintDataClass,
    176,
    nullptr,
    0,
    nullptr,
    0,
    hkpPointToPlaneConstraintDataClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpPointToPlaneConstraintDataTypeInfo = {
    "hkpPointToPlaneConstraintData",
    finishLoadedObjecthkpPointToPlaneConstraintData,
    cleanupLoadedObjecthkpPointToPlaneConstraintData,
    getVtablehkpPointToPlaneConstraintData(),
    &typeid(hkpPointToPlaneConstraintData),
    sizeof(hkpPointToPlaneConstraintData)
};

static void cleanupLoadedObjecthkpPointToPlaneConstraintDataAtoms(void* p) {
    ;
}

static void finishLoadedObjecthkpPointToPlaneConstraintDataAtoms(void* p, int32_t finishing) {
    ;
}

// The type info's finish-loaded-object callback for hkpPointToPlaneConstraintData: the null check, then its finish
// constructor.
static void finishLoadedObjecthkpPointToPlaneConstraintData(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpPointToPlaneConstraintData(kFlag);
    }
}

static const void* getVtablehkpPointToPlaneConstraintData() {
    return hkGetVtableOf<hkpPointToPlaneConstraintData>();
}

static void cleanupLoadedObjecthkpPointToPlaneConstraintData(void* p) {
    static_cast<hkpPointToPlaneConstraintData*>(p)->~hkpPointToPlaneConstraintData();
}

const hkClass& hkpPointToPlaneConstraintData::Atoms::staticClass() {
    return hkpPointToPlaneConstraintDataAtomsClass;
}

// The atoms have no constructors, so their types are set here; then identity pivot transforms and the plane's axis
// index 0.
hkpPointToPlaneConstraintData::hkpPointToPlaneConstraintData() {
    m_atoms.m_transforms.m_type.m_storage = 2;
    m_atoms.m_lin.m_type.m_storage = 7;
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
    m_atoms.m_lin.m_axisIndex = 0;
}

hkBool* hkpPointToPlaneConstraintData::isValid(hkBool* arResult) const {
    hkBool a2;
    hkBool apResult;
    if (m_atoms.m_transforms.m_transformA.m_rotation.isOrthonormal(&a2, 0.001f)->m_bool &&
        m_atoms.m_transforms.m_transformB.m_rotation.isOrthonormal(&apResult, 0.001f)->m_bool) {
        arResult->m_bool = 1;
        return arResult;
    }
    else {
        arResult->m_bool = 0;
        return arResult;
    }
}

int32_t hkpPointToPlaneConstraintData::getType() const {
    return 14;
}

void hkpPointToPlaneConstraintData::getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const {
    if (wantRuntime.m_bool) {
        infoOut.m_numSolverResults = 1;
        infoOut.m_sizeOfExternalRuntime = 8;
    }
    else {
        infoOut.m_numSolverResults = 0;
        infoOut.m_sizeOfExternalRuntime = 0;
    }
}

void hkpPointToPlaneConstraintData::getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const {
    hkpConstraintData::getConstraintInfoUtil(&m_atoms.m_transforms, 148, infoOut);
}

void hkpPointToPlaneConstraintData::setInBodySpace(
    const hkVector4& pivotA,
    const hkVector4& pivotB,
    const hkVector4& planeNormalB
) {
    __m128 v13 = _mm_undefined_ps();
    m_atoms.m_transforms.m_transformA.m_translation.m_quad = pivotA.m_quad;
    m_atoms.m_transforms.m_transformB.m_translation.m_quad = pivotB.m_quad;
    m_atoms.m_transforms.m_transformA.m_rotation.m_col0.m_quad = _mm_setzero_ps();
    m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad = _mm_setzero_ps();
    m_atoms.m_transforms.m_transformA.m_rotation.m_col2.m_quad = _mm_setzero_ps();
    m_atoms.m_transforms.m_transformA.m_rotation.m_col0.m_quad[0] = 1.0f;
    m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad[1] = 1.0f;
    m_atoms.m_transforms.m_transformA.m_rotation.m_col2.m_quad[2] = 1.0f;
    int32_t m_axisIndex = m_atoms.m_lin.m_axisIndex;
    __m128* v5 = &m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad + m_axisIndex;
    *v5 = planeNormalB.m_quad;
    int32_t v18 = 0;
    int32_t v6 = 1;
    float v17 = std::fabs((double)(*v5)[0]);
    float v19 = std::fabs((double)m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad[4 * m_axisIndex + 1]);
    float v20 = std::fabs((double)m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad[4 * m_axisIndex + 2]);
    __m128* v7 = &m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad + (m_axisIndex + 1) % 3;
    if (v17 > static_cast<double>(v19)) {
        v17 = v19;
        v6 = 0;
        v18 = 1;
    }
    int32_t v8 = v18;
    if (v17 <= static_cast<double>(v20)) {
        v8 = 2;
    }
    *v7 = _mm_setzero_ps();
    (*v7)[v6] = (*v5)[v8];
    (*v7)[v8] = -(*v5)[v6];
    __m128 v9 = _mm_mul_ps(*v7, *v7);
    float v10 = _mm_shuffle_ps(v9, v9, 85)[0] + v9[0];
    __m128 v11 = _mm_shuffle_ps(v9, v9, 170);
    v11[0] = v11[0] + v10;
    float v12 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v11[0])));
    v13[0] = std::bit_cast<float>(static_cast<int32_t>(0.0f));
    __m128 v14 = _mm_cmpeq_ss(v11, v13);
    v13[0] = 3.0f - static_cast<float>(static_cast<float>(v11[0] * v12) * v12);
    __m128 v15 = _mm_setzero_ps();
    v15[0] = static_cast<float>(0.5f * v12) * v13[0];
    __m128 v16 = _mm_andnot_ps(v14, v15);
    *v7 = _mm_mul_ps(_mm_shuffle_ps(v16, v16, 0), *v7);
    *(&m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad + (m_axisIndex + 2) % 3) = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(*v5, *v5, 201), _mm_shuffle_ps(*v7, *v7, 210)),
        _mm_mul_ps(_mm_shuffle_ps(*v5, *v5, 210), _mm_shuffle_ps(*v7, *v7, 201))
    );
}

void hkpPointToPlaneConstraintData::setInWorldSpace(
    const hkTransform& bodyATransform,
    const hkTransform& bodyBTransform,
    const hkVector4& pivotW,
    const hkVector4& planeNormalW
) {
    __m128 v15 = _mm_undefined_ps();
    m_atoms.m_transforms.m_transformA.m_translation.setTransformedInversePos(bodyATransform, pivotW);
    m_atoms.m_transforms.m_transformB.m_translation.setTransformedInversePos(bodyBTransform, pivotW);
    m_atoms.m_transforms.m_transformA.m_rotation.m_col0.m_quad = _mm_setzero_ps();
    m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad = _mm_setzero_ps();
    m_atoms.m_transforms.m_transformA.m_rotation.m_col2.m_quad = _mm_setzero_ps();
    m_atoms.m_transforms.m_transformA.m_rotation.m_col0.m_quad[0] = 1.0f;
    m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad[1] = 1.0f;
    m_atoms.m_transforms.m_transformA.m_rotation.m_col2.m_quad[2] = 1.0f;
    int32_t m_axisIndex = m_atoms.m_lin.m_axisIndex;
    hkVector4* v7 = &m_atoms.m_transforms.m_transformB.m_rotation.m_col0 + m_axisIndex;
    v7->setRotatedInverseDir(bodyBTransform.m_rotation, planeNormalW);
    float v19 = std::fabs((double)v7->m_quad[0]);
    float v21 = std::fabs((double)m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad[4 * m_axisIndex + 1]);
    float v22 = std::fabs((double)m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad[4 * m_axisIndex + 2]);
    int32_t v20 = 0;
    __m128* v8 = &m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad + (m_axisIndex + 1) % 3;
    int32_t v9 = 1;
    if (v19 > static_cast<double>(v21)) {
        v19 = v21;
        v9 = 0;
        v20 = 1;
    }
    int32_t v10 = v20;
    if (v19 <= static_cast<double>(v22)) {
        v10 = 2;
    }
    *v8 = _mm_setzero_ps();
    (*v8)[v9] = v7->m_quad[v10];
    (*v8)[v10] = -v7->m_quad[v9];
    __m128 v11 = _mm_mul_ps(*v8, *v8);
    float v12 = _mm_shuffle_ps(v11, v11, 85)[0] + v11[0];
    __m128 v13 = _mm_shuffle_ps(v11, v11, 170);
    v13[0] = v13[0] + v12;
    float v14 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v13[0])));
    v15[0] = std::bit_cast<float>(static_cast<int32_t>(0.0f));
    __m128 v16 = _mm_cmpeq_ss(v13, v15);
    v15[0] = 3.0f - static_cast<float>(static_cast<float>(v13[0] * v14) * v14);
    __m128 v17 = _mm_setzero_ps();
    v17[0] = static_cast<float>(0.5f * v14) * v15[0];
    __m128 v18 = _mm_andnot_ps(v16, v17);
    *v8 = _mm_mul_ps(_mm_shuffle_ps(v18, v18, 0), *v8);
    *(&m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad + (m_axisIndex + 2) % 3) = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(v7->m_quad, v7->m_quad, 201), _mm_shuffle_ps(*v8, *v8, 210)),
        _mm_mul_ps(_mm_shuffle_ps(v7->m_quad, v7->m_quad, 210), _mm_shuffle_ps(*v8, *v8, 201))
    );
}

const hkClass& hkpPointToPlaneConstraintData::staticClass() {
    return hkpPointToPlaneConstraintDataClass;
}
