#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Hinge/hkpHingeConstraintData.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
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
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpBallSocketConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTransformsConstraintAtom.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

static void cleanupLoadedObjecthkpHingeConstraintDataAtoms(void* p);
static void finishLoadedObjecthkpHingeConstraintDataAtoms(void* p, int32_t finishing);
static void finishLoadedObjecthkpHingeConstraintData(void* p, int32_t finishing);
// Havok SDK signature (hkpDynamics.lib, tied by reloc,target).
static const void* getVtablehkpHingeConstraintData();
static void cleanupLoadedObjecthkpHingeConstraintData(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpHingeConstraintDataAtomsTypeInfo = {
    "hkpHingeConstraintDataAtoms",
    finishLoadedObjecthkpHingeConstraintDataAtoms,
    cleanupLoadedObjecthkpHingeConstraintDataAtoms,
    nullptr,
    &typeid(hkpHingeConstraintData::Atoms),
    sizeof(hkpHingeConstraintData::Atoms)
};

static void cleanupLoadedObjecthkpHingeConstraintDataAtoms(void* p) {
    ;
}

static void finishLoadedObjecthkpHingeConstraintDataAtoms(void* p, int32_t finishing) {
    ;
}

// The type info's finish-loaded-object callback for hkpHingeConstraintData: the null check, then its finish
// constructor.
static void finishLoadedObjecthkpHingeConstraintData(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag aFlag;
        aFlag.m_finishing = finishing;
        ::new (p) hkpHingeConstraintData(aFlag);
    }
}

static const void* getVtablehkpHingeConstraintData() {
    return hkGetVtableOf<hkpHingeConstraintData>();
}

static void cleanupLoadedObjecthkpHingeConstraintData(void* p) {
    static_cast<hkpHingeConstraintData*>(p)->~hkpHingeConstraintData();
}

// The reflection data of hkpHingeConstraintData's and its Atoms' class records, as the Havok SDK's class file has it:
// the names of Atoms::Axis's values, the enum record and both member tables.
using hkpHingeConstraintDataAtomsAxisEnumItemsArray = const hkClassEnum::Item[1];
static hkpHingeConstraintDataAtomsAxisEnumItemsArray hkpHingeConstraintDataAtomsAxisEnumItems{{0, "AXIS_AXLE"}};
using hkpHingeConstraintDataAtomsEnumsArray = const hkClassEnum[1];
static hkpHingeConstraintDataAtomsEnumsArray hkpHingeConstraintDataAtomsEnums{
    {"Axis", hkpHingeConstraintDataAtomsAxisEnumItems, 1, nullptr, {0}}
};
using hkpHingeConstraintData_AtomsClass_MembersArray = const hkClassMember[3];
static hkpHingeConstraintData_AtomsClass_MembersArray hkpHingeConstraintData_AtomsClass_Members{
    {"transforms",
     &hkpSetLocalTransformsConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x0,
     nullptr},
    {"2dAng",
     &hkp2dAngConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x90,
     nullptr},
    {"ballSocket",
     &hkpBallSocketConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x94,
     nullptr}
};
using hkpHingeConstraintDataClass_MembersArray = const hkClassMember[1];
static hkpHingeConstraintDataClass_MembersArray hkpHingeConstraintDataClass_Members{
    {"atoms",
     &hkpHingeConstraintDataAtomsClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::ALIGN_16},
     0x10,
     nullptr}
};

hkClass hkpHingeConstraintDataAtomsClass(
    "hkpHingeConstraintDataAtoms",
    nullptr,
    160,
    nullptr,
    0,
    hkpHingeConstraintDataAtomsEnums,
    1,
    hkpHingeConstraintData_AtomsClass_Members,
    3,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkpHingeConstraintDataClass(
    "hkpHingeConstraintData",
    &hkpConstraintDataClass,
    176,
    nullptr,
    0,
    nullptr,
    0,
    hkpHingeConstraintDataClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);

hkTypeInfo hkpHingeConstraintDataTypeInfo = {"hkpHingeConstraintData",
                                             finishLoadedObjecthkpHingeConstraintData,
                                             cleanupLoadedObjecthkpHingeConstraintData,
                                             getVtablehkpHingeConstraintData(),
                                             &typeid(hkpHingeConstraintData),
                                             sizeof(hkpHingeConstraintData)};

// The atom structs have no constructors: their types and the ball socket's defaults are set here.
hkpHingeConstraintData::Atoms::Atoms() {
    m_transforms.m_type.m_storage = 2;
    m_2dAng.m_type.m_storage = 12;
    m_ballSocket.m_type.m_storage = 5;
    m_ballSocket.m_bodiesToNotify = 0;
    m_ballSocket.m_stabilizationFactor = 1.0f;
    m_ballSocket.m_maxImpulse = 3.40282e+38f;
}

const hkClass& hkpHingeConstraintData::Atoms::staticClass() {
    return hkpHingeConstraintDataAtomsClass;
}

// Identity pivot transforms and the first free rotation axis; the atoms' types come from Atoms().
hkpHingeConstraintData::hkpHingeConstraintData() {
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
    m_atoms.m_2dAng.m_freeRotationAxis = 0;
}

void hkpHingeConstraintData::setMaxLinearImpulse(float maxImpulse) {
    m_atoms.m_ballSocket.m_maxImpulse = maxImpulse;
}

float hkpHingeConstraintData::getMaxLinearImpulse() const {
    return m_atoms.m_ballSocket.m_maxImpulse;
}

void hkpHingeConstraintData::setBodyToNotify(int32_t bodyIdx) {
    m_atoms.m_ballSocket.m_bodiesToNotify = (uint8_t)(1 << ((int8_t)bodyIdx));
}

uint8_t hkpHingeConstraintData::getNotifiedBodyIndex() const {
    return (uint8_t)((int32_t)(((uint32_t)((int32_t)m_atoms.m_ballSocket.m_bodiesToNotify)) >> 1));
}

hkBool hkpHingeConstraintData::isValid() const {
    if (m_atoms.m_transforms.m_transformA.m_rotation.isOrthonormal(0.001f).m_bool &&
        m_atoms.m_transforms.m_transformB.m_rotation.isOrthonormal(0.001f).m_bool) {
        return 1;
    }

    return 0;
}

int32_t hkpHingeConstraintData::getType() const {
    return 1;
}

void hkpHingeConstraintData::getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const {
    if (wantRuntime.m_bool) {
        infoOut.m_numSolverResults = 5;
        infoOut.m_sizeOfExternalRuntime = 40;
    }
    else {
        infoOut.m_numSolverResults = 0;
        infoOut.m_sizeOfExternalRuntime = 0;
    }
}

void hkpHingeConstraintData::getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const {
    hkpConstraintData::getConstraintInfoUtil(&m_atoms.m_transforms, 156, infoOut);
}

void hkpHingeConstraintData::setInWorldSpace(
    const hkTransform& bodyATransform,
    const hkTransform& bodyBTransform,
    const hkVector4& pivot,
    const hkVector4& axis
) {
    __m128 v14 = _mm_undefined_ps();
    hkVector4 arVector;
    hkVector4 v25;
    int32_t v5 = 0;
    float v20 = std::fabs((double)axis.m_quad[0]);
    int32_t v7 = 1;
    int32_t v8 = 2;
    float v21 = std::fabs((double)axis.m_quad[1]);
    float v22 = std::fabs((double)axis.m_quad[2]);
    if (v20 > static_cast<double>(v21)) {
        v20 = v21;
        v7 = 0;
        v5 = 1;
    }
    if (v20 > static_cast<double>(v22)) {
        v8 = v5;
    }
    double v9 = axis.m_quad[v8];
    arVector.m_quad = _mm_setzero_ps();
    arVector.m_quad[v7] = v9;
    arVector.m_quad[v8] = -axis.m_quad[v7];
    __m128 v10 = _mm_mul_ps(arVector.m_quad, arVector.m_quad);
    float v11 = _mm_shuffle_ps(v10, v10, 85)[0] + v10[0];
    __m128 v12 = _mm_shuffle_ps(v10, v10, 170);
    v12[0] = v12[0] + v11;
    float v13 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v12[0])));
    v14[0] = std::bit_cast<float>(static_cast<int32_t>(0.0f));
    __m128 v15 = _mm_cmpeq_ss(v12, v14);
    v14[0] = 3.0f - ((v12[0] * v13) * v13);
    __m128 v16 = _mm_setzero_ps();
    v16[0] = (0.5f * v13) * v14[0];
    __m128 v17 = _mm_andnot_ps(v15, v16);
    __m128 v18 = _mm_shuffle_ps(v17, v17, 0);
    __m128 m_quad = axis.m_quad;
    arVector.m_quad = _mm_mul_ps(v18, arVector.m_quad);
    v25.m_quad = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(m_quad, m_quad, 201), _mm_shuffle_ps(arVector.m_quad, arVector.m_quad, 210)),
        _mm_mul_ps(_mm_shuffle_ps(m_quad, m_quad, 210), _mm_shuffle_ps(arVector.m_quad, arVector.m_quad, 201))
    );
    m_atoms.m_transforms.m_transformA.m_rotation.m_col0.setRotatedInverseDir(bodyATransform.m_rotation, axis);
    m_atoms.m_transforms.m_transformA.m_rotation.m_col1.setRotatedInverseDir(bodyATransform.m_rotation, arVector);
    m_atoms.m_transforms.m_transformA.m_rotation.m_col2.setRotatedInverseDir(bodyATransform.m_rotation, v25);
    m_atoms.m_transforms.m_transformA.m_translation.setTransformedInversePos(bodyATransform, pivot);
    m_atoms.m_transforms.m_transformB.m_rotation.m_col0.setRotatedInverseDir(bodyBTransform.m_rotation, axis);
    m_atoms.m_transforms.m_transformB.m_rotation.m_col1.setRotatedInverseDir(bodyBTransform.m_rotation, arVector);
    m_atoms.m_transforms.m_transformB.m_rotation.m_col2.setRotatedInverseDir(bodyBTransform.m_rotation, v25);
    m_atoms.m_transforms.m_transformB.m_translation.setTransformedInversePos(bodyBTransform, pivot);
}

// The pivots are the frames' translations; each body's axis, normalized, is its frame's first column, a perpendicular
// of it, normalized, the second, and their cross product the third.
void hkpHingeConstraintData::setInBodySpace(
    const hkVector4& pivotA,
    const hkVector4& pivotB,
    const hkVector4& axisA,
    const hkVector4& axisB
) {
    m_atoms.m_transforms.m_transformA.m_translation.m_quad = pivotA.m_quad;
    m_atoms.m_transforms.m_transformB.m_translation.m_quad = pivotB.m_quad;
    hkRotation& rotationA = m_atoms.m_transforms.m_transformA.m_rotation;
    rotationA.m_col0 = axisA;
    rotationA.m_col0.normalize3();
    hkVector4Util::calculatePerpendicularVector(rotationA.m_col0, rotationA.m_col1);
    rotationA.m_col1.normalize3();
    rotationA.m_col2.setCross(rotationA.m_col0, rotationA.m_col1);
    hkRotation& rotationB = m_atoms.m_transforms.m_transformB.m_rotation;
    rotationB.m_col0 = axisB;
    rotationB.m_col0.normalize3();
    hkVector4Util::calculatePerpendicularVector(rotationB.m_col0, rotationB.m_col1);
    rotationB.m_col1.normalize3();
    rotationB.m_col2.setCross(rotationB.m_col0, rotationB.m_col1);
}

const hkClass& hkpHingeConstraintData::staticClass() {
    return hkpHingeConstraintDataClass;
}
