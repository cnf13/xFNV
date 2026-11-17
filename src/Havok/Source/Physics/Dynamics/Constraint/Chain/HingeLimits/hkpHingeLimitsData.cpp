#include "Havok/Source/Physics/Dynamics/Constraint/Chain/HingeLimits/hkpHingeLimitsData.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
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

#include <bit>
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
    {"rotations", &hkpSetLocalRotationsConstraintAtomClass, nullptr, hkClassMember::TYPE_STRUCT,
     hkClassMember::TYPE_VOID, 0, 0, 0x0, nullptr},
    {"angLimit", &hkpAngLimitConstraintAtomClass, nullptr, hkClassMember::TYPE_STRUCT, hkClassMember::TYPE_VOID, 0, 0,
     0x70, nullptr},
    {"2dAng", &hkp2dAngConstraintAtomClass, nullptr, hkClassMember::TYPE_STRUCT, hkClassMember::TYPE_VOID, 0, 0, 0x80,
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
    "hkpHingeLimitsData",          finishLoadedObjecthkpHingeLimitsData, cleanupLoadedObjecthkpHingeLimitsData,
    getVtablehkpHingeLimitsData(), &typeid(hkpHingeLimitsData),          sizeof(hkpHingeLimitsData)
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
    m_atoms.m_rotations.m_rotationA.m_col0.m_quad[0] = 1.0f;
    m_atoms.m_rotations.m_rotationA.m_col1.m_quad[1] = 1.0f;
    m_atoms.m_rotations.m_rotationA.m_col2.m_quad[2] = 1.0f;
    m_atoms.m_rotations.m_rotationB.m_col0.m_quad = _mm_setzero_ps();
    m_atoms.m_rotations.m_rotationB.m_col1.m_quad = _mm_setzero_ps();
    m_atoms.m_rotations.m_rotationB.m_col2.m_quad = _mm_setzero_ps();
    m_atoms.m_rotations.m_rotationB.m_col0.m_quad[0] = 1.0f;
    m_atoms.m_rotations.m_rotationB.m_col1.m_quad[1] = 1.0f;
    m_atoms.m_rotations.m_rotationB.m_col2.m_quad[2] = 1.0f;
    m_atoms.m_2dAng.m_freeRotationAxis = 0;
    m_atoms.m_angLimit.m_limitAxis = 0;
    m_atoms.m_angLimit.m_minAngle = -3.1415927f;
    m_atoms.m_angLimit.m_maxAngle = 3.1415927f;
    m_atoms.m_angLimit.m_angularLimitsTauFactor = 1.0f;
}

hkBool* hkpHingeLimitsData::isValid(hkBool* arResult) const {
    hkBool a2;
    hkBool apResult;
    if (m_atoms.m_rotations.m_rotationA.isOrthonormal(&a2, 0.001f)->m_bool &&
        m_atoms.m_rotations.m_rotationB.isOrthonormal(&apResult, 0.001f)->m_bool) {
        arResult->m_bool = 1;
        return arResult;
    }
    else {
        arResult->m_bool = 0;
        return arResult;
    }
}

int32_t hkpHingeLimitsData::getType() const {
    return 18;
}

void hkpHingeLimitsData::getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const {
    (&infoOut)->m_numSolverResults = 3;
    (&infoOut)->m_sizeOfExternalRuntime = 28;
}

void hkpHingeLimitsData::getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const {
    hkpConstraintData::getConstraintInfoUtil(&m_atoms.m_rotations, 132, infoOut);
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
    float v7 = _mm_shuffle_ps(v6, v6, 85)[0] + v6[0];
    __m128 v8 = _mm_shuffle_ps(v6, v6, 170);
    v8[0] = v8[0] + v7;
    float v9 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v8[0])));
    v10[0] = std::bit_cast<float>(static_cast<int32_t>(0.0f));
    __m128 v11 = _mm_setzero_ps();
    v11[0] = 0.5f;
    __m128 v12 = _mm_cmpeq_ss(v8, v10);
    float v13 = 3.0f - ((v8[0] * v9) * v9);
    v11[0] = (0.5f * v9) * v13;
    __m128 v15 = _mm_andnot_ps(v12, v11);
    m_atoms.m_rotations.m_rotationA.m_col0.m_quad = _mm_mul_ps(_mm_shuffle_ps(v15, v15, 0), v5.m_quad);
    m_atoms.m_rotations.m_rotationA.m_col1.m_quad = axisAPerp.m_quad;
    v16.m_quad = m_atoms.m_rotations.m_rotationA.m_col1.m_quad;
    __m128 v17 = _mm_mul_ps(v16.m_quad, v16.m_quad);
    v15[0] = _mm_shuffle_ps(v17, v17, 85)[0] + v17[0];
    __m128 v18 = _mm_shuffle_ps(v17, v17, 170);
    v18[0] = v18[0] + v15[0];
    float v19 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v18[0])));
    float v21 = 3.0f - ((v18[0] * v19) * v19);
    v11[0] = (0.5f * v19) * v21;
    __m128 v23 = _mm_andnot_ps(_mm_cmpeq_ss(v18, v10), v11);
    m_atoms.m_rotations.m_rotationA.m_col1.m_quad = _mm_mul_ps(_mm_shuffle_ps(v23, v23, 0), v16.m_quad);
    m_atoms.m_rotations.m_rotationA.m_col2.m_quad = _mm_sub_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(axisAPerp.m_quad, axisAPerp.m_quad, 210), _mm_shuffle_ps(axisA.m_quad, axisA.m_quad, 201)
        ),
        _mm_mul_ps(
            _mm_shuffle_ps(axisAPerp.m_quad, axisAPerp.m_quad, 201), _mm_shuffle_ps(axisA.m_quad, axisA.m_quad, 210)
        )
    );
    m_atoms.m_rotations.m_rotationB.m_col0.m_quad = axisB.m_quad;
    v24.m_quad = m_atoms.m_rotations.m_rotationB.m_col0.m_quad;
    __m128 v25 = _mm_mul_ps(v24.m_quad, v24.m_quad);
    v23[0] = _mm_shuffle_ps(v25, v25, 85)[0] + v25[0];
    __m128 v26 = _mm_shuffle_ps(v25, v25, 170);
    v26[0] = v26[0] + v23[0];
    float v27 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v26[0])));
    __m128 v28 = _mm_cmpeq_ss(v26, v10);
    float v29 = 3.0f - ((v26[0] * v27) * v27);
    v11[0] = (0.5f * v27) * v29;
    __m128 v31 = _mm_andnot_ps(v28, v11);
    m_atoms.m_rotations.m_rotationB.m_col0.m_quad = _mm_mul_ps(_mm_shuffle_ps(v31, v31, 0), v24.m_quad);
    m_atoms.m_rotations.m_rotationB.m_col1.m_quad = axisBPerp.m_quad;
    v32.m_quad = m_atoms.m_rotations.m_rotationB.m_col1.m_quad;
    __m128 v33 = _mm_mul_ps(v32.m_quad, v32.m_quad);
    v31[0] = _mm_shuffle_ps(v33, v33, 85)[0] + v33[0];
    __m128 v34 = _mm_shuffle_ps(v33, v33, 170);
    v34[0] = v34[0] + v31[0];
    v31[0] = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v34[0])));
    v11[0] = (0.5f * v31[0]) * (3.0f - ((v34[0] * v31[0]) * v31[0]));
    __m128 v35 = _mm_andnot_ps(_mm_cmpeq_ss(v34, v10), v11);
    m_atoms.m_rotations.m_rotationB.m_col1.m_quad = _mm_mul_ps(_mm_shuffle_ps(v35, v35, 0), v32.m_quad);
    m_atoms.m_rotations.m_rotationB.m_col2.m_quad = _mm_sub_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(axisB.m_quad, axisB.m_quad, 201), _mm_shuffle_ps(axisBPerp.m_quad, axisBPerp.m_quad, 210)
        ),
        _mm_mul_ps(
            _mm_shuffle_ps(axisB.m_quad, axisB.m_quad, 210), _mm_shuffle_ps(axisBPerp.m_quad, axisBPerp.m_quad, 201)
        )
    );
}

const hkClass& hkpHingeLimitsData::staticClass() {
    return hkpHingeLimitsDataClass;
}
