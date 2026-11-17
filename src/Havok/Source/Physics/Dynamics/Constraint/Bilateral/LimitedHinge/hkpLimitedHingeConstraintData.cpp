#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/LimitedHinge/hkpLimitedHingeConstraintData.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
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
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkp2dAngConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngFrictionConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngMotorConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpBallSocketConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTransformsConstraintAtom.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"

#include <bit>
#include <xmmintrin.h>

static void finishLoadedObjecthkpLimitedHingeConstraintDataAtoms(void* p, int32_t finishing);
static void cleanupLoadedObjecthkpLimitedHingeConstraintDataAtoms(void* p);
static void finishLoadedObjecthkpLimitedHingeConstraintData(void* p, int32_t finishing);
// Havok SDK signature (hkpDynamics.lib, tied by reloc,target).
static const void* getVtablehkpLimitedHingeConstraintData();
static void cleanupLoadedObjecthkpLimitedHingeConstraintData(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpLimitedHingeConstraintDataAtomsTypeInfo = {
    "hkpLimitedHingeConstraintDataAtoms",
    finishLoadedObjecthkpLimitedHingeConstraintDataAtoms,
    cleanupLoadedObjecthkpLimitedHingeConstraintDataAtoms,
    nullptr,
    &typeid(hkpLimitedHingeConstraintData::Atoms),
    sizeof(hkpLimitedHingeConstraintData::Atoms)
};

// The reflection data of hkpLimitedHingeConstraintData's and its Atoms' class records, as the Havok SDK's class file
// has it: the names of Atoms::Axis's values, the enum record and both member tables.
using hkpLimitedHingeConstraintDataAtomsAxisEnumItemsArray = const hkClassEnum::Item[3];
static hkpLimitedHingeConstraintDataAtomsAxisEnumItemsArray hkpLimitedHingeConstraintDataAtomsAxisEnumItems{
    {0, "AXIS_AXLE"},
    {1, "AXIS_PERP_TO_AXLE_1"},
    {2, "AXIS_PERP_TO_AXLE_2"}
};
using hkpLimitedHingeConstraintDataAtomsEnumsArray = const hkClassEnum[1];
static hkpLimitedHingeConstraintDataAtomsEnumsArray hkpLimitedHingeConstraintDataAtomsEnums{
    {"Axis", hkpLimitedHingeConstraintDataAtomsAxisEnumItems, 3, nullptr, {0}}
};
using hkpLimitedHingeConstraintData_AtomsClass_MembersArray = const hkClassMember[6];
static hkpLimitedHingeConstraintData_AtomsClass_MembersArray hkpLimitedHingeConstraintData_AtomsClass_Members{
    {"transforms",
     &hkpSetLocalTransformsConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x0,
     nullptr},
    {"angMotor",
     &hkpAngMotorConstraintAtomClass,
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
     0xA4,
     nullptr},
    {"angLimit",
     &hkpAngLimitConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xB0,
     nullptr},
    {"2dAng",
     &hkp2dAngConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xC0,
     nullptr},
    {"ballSocket",
     &hkpBallSocketConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xC4,
     nullptr}
};
using hkpLimitedHingeConstraintDataClass_MembersArray = const hkClassMember[1];
static hkpLimitedHingeConstraintDataClass_MembersArray hkpLimitedHingeConstraintDataClass_Members{
    {"atoms",
     &hkpLimitedHingeConstraintDataAtomsClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::ALIGN_16},
     0x10,
     nullptr}
};
hkClass hkpLimitedHingeConstraintDataAtomsClass(
    "hkpLimitedHingeConstraintDataAtoms",
    nullptr,
    208,
    nullptr,
    0,
    hkpLimitedHingeConstraintDataAtomsEnums,
    1,
    hkpLimitedHingeConstraintData_AtomsClass_Members,
    6,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkpLimitedHingeConstraintDataClass(
    "hkpLimitedHingeConstraintData",
    &hkpConstraintDataClass,
    224,
    nullptr,
    0,
    nullptr,
    0,
    hkpLimitedHingeConstraintDataClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpLimitedHingeConstraintDataTypeInfo = {
    "hkpLimitedHingeConstraintData",
    finishLoadedObjecthkpLimitedHingeConstraintData,
    cleanupLoadedObjecthkpLimitedHingeConstraintData,
    getVtablehkpLimitedHingeConstraintData(),
    &typeid(hkpLimitedHingeConstraintData),
    sizeof(hkpLimitedHingeConstraintData)
};

static void cleanupLoadedObjecthkpLimitedHingeConstraintDataAtoms(void* p) {
    ;
}

static void finishLoadedObjecthkpLimitedHingeConstraintDataAtoms(void* p, int32_t finishing) {
    ;
}

// The type info's finish-loaded-object callback for hkpLimitedHingeConstraintData: the null check, then its finish
// constructor.
static void finishLoadedObjecthkpLimitedHingeConstraintData(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpLimitedHingeConstraintData(kFlag);
    }
}

static const void* getVtablehkpLimitedHingeConstraintData() {
    return hkGetVtableOf<hkpLimitedHingeConstraintData>();
}

static void cleanupLoadedObjecthkpLimitedHingeConstraintData(void* p) {
    static_cast<hkpLimitedHingeConstraintData*>(p)->~hkpLimitedHingeConstraintData();
}

// The atom structs have no constructors: their types and defaults are set here.
hkpLimitedHingeConstraintData::Atoms::Atoms() {
    m_transforms.m_type.m_storage = 2;
    m_angMotor.m_type.m_storage = 18;
    m_angMotor.m_isEnabled.m_bool = 1;
    m_angMotor.m_initializedOffset = 0;
    m_angMotor.m_previousTargetAngleOffset = 0;
    m_angFriction.m_isEnabled = 1;
    m_angFriction.m_numFrictionAxes = 1;
    m_angFriction.m_type.m_storage = 17;
    m_angLimit.m_type.m_storage = 14;
    m_angLimit.m_isEnabled = 1;
    m_2dAng.m_type.m_storage = 12;
    m_ballSocket.m_type.m_storage = 5;
    m_ballSocket.m_bodiesToNotify = 0;
    m_ballSocket.m_stabilizationFactor = 1.0f;
    m_ballSocket.m_maxImpulse = 3.40282e+38f;
}

const hkClass& hkpLimitedHingeConstraintData::Atoms::staticClass() {
    return hkpLimitedHingeConstraintDataAtomsClass;
}

// Identity pivot transforms, a limit of plus or minus pi, no friction and the motor off and unset.
hkpLimitedHingeConstraintData::hkpLimitedHingeConstraintData() {
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
    m_atoms.m_angLimit.m_minAngle = -3.1415927f;
    m_atoms.m_angLimit.m_limitAxis = 0;
    m_atoms.m_angFriction.m_firstFrictionAxis = 0;
    m_atoms.m_angLimit.m_maxAngle = 3.1415927f;
    m_atoms.m_angMotor.m_correspondingAngLimitSolverResultOffset = 16;
    m_atoms.m_angLimit.m_angularLimitsTauFactor = 1.0f;
    m_atoms.m_angFriction.m_numFrictionAxes = 1;
    m_atoms.m_angMotor.m_initializedOffset = 64;
    m_atoms.m_angFriction.m_maxFrictionTorque = 0.0f;
    m_atoms.m_angMotor.m_previousTargetAngleOffset = 68;
    m_atoms.m_angMotor.m_isEnabled.m_bool = 0;
    m_atoms.m_angMotor.m_targetAngle = 0.0f;
    m_atoms.m_angMotor.m_motor = nullptr;
    m_atoms.m_angMotor.m_motorAxis = 0;
}

// Releases the motor.
hkpLimitedHingeConstraintData::~hkpLimitedHingeConstraintData() {
    if (m_atoms.m_angMotor.m_motor) {
        m_atoms.m_angMotor.m_motor->removeReference();
    }
}

void hkpLimitedHingeConstraintData::setMaxLinearImpulse(float maxImpulse) {
    m_atoms.m_ballSocket.m_maxImpulse = maxImpulse;
}

float hkpLimitedHingeConstraintData::getMaxLinearImpulse() const {
    return m_atoms.m_ballSocket.m_maxImpulse;
}

void hkpLimitedHingeConstraintData::setBodyToNotify(int32_t bodyIdx) {
    m_atoms.m_ballSocket.m_bodiesToNotify = (uint8_t)(1 << ((int8_t)bodyIdx));
}

uint8_t hkpLimitedHingeConstraintData::getNotifiedBodyIndex() const {
    return (uint8_t)((int32_t)(((uint32_t)((int32_t)m_atoms.m_ballSocket.m_bodiesToNotify)) >> 1));
}

hkBool* hkpLimitedHingeConstraintData::isValid(hkBool* arResult) const {
    hkBool a2;
    hkBool apResult;
    if (m_atoms.m_transforms.m_transformA.m_rotation.isOrthonormal(&a2, 0.001f)->m_bool &&
        (m_atoms.m_transforms.m_transformB.m_rotation.isOrthonormal(&apResult, 0.001f)->m_bool &&
         (!m_atoms.m_angMotor.m_isEnabled.m_bool || m_atoms.m_angLimit.m_isEnabled))) {
        arResult->m_bool = 1;
        return arResult;
    }

    arResult->m_bool = 0;
    return arResult;
}

int32_t hkpLimitedHingeConstraintData::getType() const {
    return 2;
}

void hkpLimitedHingeConstraintData::getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const {
    (&infoOut)->m_numSolverResults = 8;
    (&infoOut)->m_sizeOfExternalRuntime = 80;
}

// The atoms' span is the native one from getSizeOfAllAtoms(), not the 32-bit literal 204, which on x64 would end the
// walk short of the ball-socket atom.
void hkpLimitedHingeConstraintData::getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const {
    hkpConstraintData::getConstraintInfoUtil(m_atoms.getAtoms(), m_atoms.getSizeOfAllAtoms(), infoOut);
}

void hkpLimitedHingeConstraintData::setMotor(hkpConstraintMotor* a1) {
    hkpConstraintMotor* m_motor;
    if (a1) {
        a1->hkReferencedObject::addReference();
    }
    m_motor = m_atoms.m_angMotor.m_motor;
    if (m_motor) {
        m_motor->hkReferencedObject::removeReference();
    }
    m_atoms.m_angMotor.m_motor = a1;
}

void hkpLimitedHingeConstraintData::setMotorActive(hkpConstraintInstance* instance, hkBool toBeEnabled) {
    m_atoms.m_angMotor.m_isEnabled = toBeEnabled;
    m_atoms.m_angFriction.m_isEnabled = toBeEnabled.m_bool == 0;
    if (instance) {
        hkConstraintInternal* m_internal = instance->getInternal();
        if (m_internal) {
            float* m_runtime = static_cast<float*>(m_internal->m_runtime);
            if (m_runtime) {
                *m_runtime = 0.0;
                m_runtime[1] = 0.0;
                m_runtime[2] = 0.0;
                m_runtime[3] = 0.0;
            }
        }
    }
}

void hkpLimitedHingeConstraintData::setInWorldSpace(
    const hkTransform& bodyATransform,
    const hkTransform& bodyBTransform,
    const hkVector4& pivot,
    const hkVector4& axisIn
) {
    __m128 v11 = _mm_undefined_ps();
    hkVector4 arVector_;
    hkVector4 arVector;
    hkVector4 arVector__1;
    arVector_ = axisIn;
    __m128 v7 = _mm_mul_ps(arVector_.m_quad, arVector_.m_quad);
    float v8 = _mm_shuffle_ps(v7, v7, 85)[0] + v7[0];
    __m128 v9 = _mm_shuffle_ps(v7, v7, 170);
    v9[0] = v9[0] + v8;
    float v10 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v9[0])));
    v11[0] = 0.0f;
    int32_t n2_1 = 0;
    int32_t v13 = 1;
    __m128 v14 = _mm_setzero_ps();
    v14[0] = 0.5f;
    float v15 = 3.0f - ((v9[0] * v10) * v10);
    __m128 v16 = _mm_cmpeq_ss(v9, v11);
    v14[0] = (0.5f * v10) * v15;
    __m128 v18 = _mm_andnot_ps(v16, v14);
    __m128 m_quad = _mm_mul_ps(_mm_shuffle_ps(v18, v18, 0), arVector_.m_quad);
    arVector_.m_quad = m_quad;
    int32_t n2 = 2;
    float v27 = std::fabs((double)m_quad[0]);
    float v28 = std::fabs((double)m_quad[1]);
    float v29 = std::fabs((double)m_quad[2]);
    if (v27 > static_cast<double>(v28)) {
        v27 = v28;
        v13 = 0;
        n2_1 = 1;
    }
    if (v27 > static_cast<double>(v29)) {
        n2 = n2_1;
    }
    double v21 = arVector_.m_quad[n2];
    arVector.m_quad = _mm_setzero_ps();
    arVector.m_quad[v13] = v21;
    arVector.m_quad[n2] = -arVector_.m_quad[v13];
    __m128 v22 = _mm_mul_ps(arVector.m_quad, arVector.m_quad);
    float v23 = _mm_shuffle_ps(v22, v22, 85)[0] + v22[0];
    __m128 v24 = _mm_shuffle_ps(v22, v22, 170);
    v24[0] = v24[0] + v23;
    float v25 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v24[0])));
    v14[0] = (0.5f * v25) * (3.0f - ((v24[0] * v25) * v25));
    __m128 v26 = _mm_andnot_ps(_mm_cmpeq_ss(v24, v11), v14);
    arVector.m_quad = _mm_mul_ps(_mm_shuffle_ps(v26, v26, 0), arVector.m_quad);
    arVector__1.m_quad = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(arVector.m_quad, arVector.m_quad, 210), _mm_shuffle_ps(m_quad, m_quad, 201)),
        _mm_mul_ps(_mm_shuffle_ps(arVector.m_quad, arVector.m_quad, 201), _mm_shuffle_ps(m_quad, m_quad, 210))
    );
    m_atoms.m_transforms.m_transformA.m_rotation.m_col0.setRotatedInverseDir(bodyATransform.m_rotation, arVector_);
    m_atoms.m_transforms.m_transformA.m_rotation.m_col1.setRotatedInverseDir(bodyATransform.m_rotation, arVector);
    m_atoms.m_transforms.m_transformA.m_rotation.m_col2.setRotatedInverseDir(bodyATransform.m_rotation, arVector__1);
    m_atoms.m_transforms.m_transformA.m_translation.setTransformedInversePos(bodyATransform, pivot);
    m_atoms.m_transforms.m_transformB.m_rotation.m_col0.setRotatedInverseDir(bodyBTransform.m_rotation, arVector_);
    m_atoms.m_transforms.m_transformB.m_rotation.m_col1.setRotatedInverseDir(bodyBTransform.m_rotation, arVector);
    m_atoms.m_transforms.m_transformB.m_rotation.m_col2.setRotatedInverseDir(bodyBTransform.m_rotation, arVector__1);
    m_atoms.m_transforms.m_transformB.m_translation.setTransformedInversePos(bodyBTransform, pivot);
}

void hkpLimitedHingeConstraintData::setInBodySpace(
    const hkVector4& pivotA,
    const hkVector4& pivotB,
    const hkVector4& axisA,
    const hkVector4& axisB,
    const hkVector4& axisAPerp,
    const hkVector4& axisBPerp
) {
    hkVector4 v7;
    __m128 v12 = _mm_undefined_ps();
    hkVector4 v18;
    hkVector4 v26;
    hkVector4 v34;
    m_atoms.m_transforms.m_transformA.m_rotation.m_col0.m_quad = axisA.m_quad;
    v7.m_quad = m_atoms.m_transforms.m_transformA.m_rotation.m_col0.m_quad;
    __m128 v8 = _mm_mul_ps(v7.m_quad, v7.m_quad);
    float v9 = _mm_shuffle_ps(v8, v8, 85)[0] + v8[0];
    __m128 v10 = _mm_shuffle_ps(v8, v8, 170);
    v10[0] = v10[0] + v9;
    float v11 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v10[0])));
    v12[0] = std::bit_cast<float>(static_cast<int32_t>(0.0f));
    __m128 v13 = _mm_setzero_ps();
    v13[0] = 0.5f;
    __m128 v14 = _mm_cmpeq_ss(v10, v12);
    float v15 = 3.0f - ((v10[0] * v11) * v11);
    v13[0] = (0.5f * v11) * v15;
    __m128 v17 = _mm_andnot_ps(v14, v13);
    m_atoms.m_transforms.m_transformA.m_rotation.m_col0.m_quad = _mm_mul_ps(_mm_shuffle_ps(v17, v17, 0), v7.m_quad);
    m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad = axisAPerp.m_quad;
    v18.m_quad = m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad;
    __m128 v19 = _mm_mul_ps(v18.m_quad, v18.m_quad);
    v17[0] = _mm_shuffle_ps(v19, v19, 85)[0] + v19[0];
    __m128 v20 = _mm_shuffle_ps(v19, v19, 170);
    v20[0] = v20[0] + v17[0];
    float v21 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v20[0])));
    float v23 = 3.0f - ((v20[0] * v21) * v21);
    v13[0] = (0.5f * v21) * v23;
    __m128 v25 = _mm_andnot_ps(_mm_cmpeq_ss(v20, v12), v13);
    m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad = _mm_mul_ps(_mm_shuffle_ps(v25, v25, 0), v18.m_quad);
    m_atoms.m_transforms.m_transformA.m_rotation.m_col2.m_quad = _mm_sub_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(
                m_atoms.m_transforms.m_transformA.m_rotation.m_col0.m_quad,
                m_atoms.m_transforms.m_transformA.m_rotation.m_col0.m_quad, 201
            ),
            _mm_shuffle_ps(
                m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad,
                m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad, 210
            )
        ),
        _mm_mul_ps(
            _mm_shuffle_ps(
                m_atoms.m_transforms.m_transformA.m_rotation.m_col0.m_quad,
                m_atoms.m_transforms.m_transformA.m_rotation.m_col0.m_quad, 210
            ),
            _mm_shuffle_ps(
                m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad,
                m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad, 201
            )
        )
    );
    m_atoms.m_transforms.m_transformA.m_translation.m_quad = pivotA.m_quad;
    m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad = axisB.m_quad;
    v26.m_quad = m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad;
    __m128 v27 = _mm_mul_ps(v26.m_quad, v26.m_quad);
    v25[0] = _mm_shuffle_ps(v27, v27, 85)[0] + v27[0];
    __m128 v28 = _mm_shuffle_ps(v27, v27, 170);
    v28[0] = v28[0] + v25[0];
    float v29 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v28[0])));
    __m128 v30 = _mm_cmpeq_ss(v28, v12);
    float v31 = 3.0f - ((v28[0] * v29) * v29);
    v13[0] = (0.5f * v29) * v31;
    __m128 v33 = _mm_andnot_ps(v30, v13);
    m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad = _mm_mul_ps(_mm_shuffle_ps(v33, v33, 0), v26.m_quad);
    m_atoms.m_transforms.m_transformB.m_rotation.m_col1.m_quad = axisBPerp.m_quad;
    v34.m_quad = m_atoms.m_transforms.m_transformB.m_rotation.m_col1.m_quad;
    __m128 v35 = _mm_mul_ps(v34.m_quad, v34.m_quad);
    v33[0] = _mm_shuffle_ps(v35, v35, 85)[0] + v35[0];
    __m128 v36 = _mm_shuffle_ps(v35, v35, 170);
    v36[0] = v36[0] + v33[0];
    v33[0] = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v36[0])));
    v13[0] = (0.5f * v33[0]) * (3.0f - ((v36[0] * v33[0]) * v33[0]));
    __m128 v37 = _mm_andnot_ps(_mm_cmpeq_ss(v36, v12), v13);
    m_atoms.m_transforms.m_transformB.m_rotation.m_col1.m_quad = _mm_mul_ps(_mm_shuffle_ps(v37, v37, 0), v34.m_quad);
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
}

const hkClass& hkpLimitedHingeConstraintData::staticClass() {
    return hkpLimitedHingeConstraintDataClass;
}
