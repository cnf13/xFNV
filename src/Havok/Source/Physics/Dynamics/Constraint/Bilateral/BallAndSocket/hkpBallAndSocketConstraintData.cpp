#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/BallAndSocket/hkpBallAndSocketConstraintData.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpBallSocketConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTranslationsConstraintAtom.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

#include <xmmintrin.h>

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpBallAndSocketConstraintDataAtomsTypeInfo = {
    "hkpBallAndSocketConstraintDataAtoms",
    finishLoadedObjecthkpBallAndSocketConstraintDataAtoms,
    cleanupLoadedObjecthkpBallAndSocketConstraintDataAtoms,
    nullptr,
    &typeid(hkpBallAndSocketConstraintData::Atoms),
    sizeof(hkpBallAndSocketConstraintData::Atoms)
};

// The member tables of hkpBallAndSocketConstraintData's and its Atoms' class records, as the Havok SDK's class file has
// them.
using hkpBallAndSocketConstraintData_AtomsClass_MembersArray = const hkClassMember[2];
static hkpBallAndSocketConstraintData_AtomsClass_MembersArray hkpBallAndSocketConstraintData_AtomsClass_Members{
    {"pivots",
     &hkpSetLocalTranslationsConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x0,
     nullptr},
    {"ballSocket",
     &hkpBallSocketConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x30,
     nullptr}
};
using hkpBallAndSocketConstraintDataClass_MembersArray = const hkClassMember[1];
static hkpBallAndSocketConstraintDataClass_MembersArray hkpBallAndSocketConstraintDataClass_Members{
    {"atoms",
     &hkpBallAndSocketConstraintDataAtomsClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::ALIGN_16},
     0x10,
     nullptr}
};

hkClass hkpBallAndSocketConstraintDataAtomsClass(
    "hkpBallAndSocketConstraintDataAtoms",
    nullptr,
    64,
    nullptr,
    0,
    nullptr,
    0,
    hkpBallAndSocketConstraintData_AtomsClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkpBallAndSocketConstraintDataClass(
    "hkpBallAndSocketConstraintData",
    &hkpConstraintDataClass,
    80,
    nullptr,
    0,
    nullptr,
    0,
    hkpBallAndSocketConstraintDataClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);

hkTypeInfo hkpBallAndSocketConstraintDataTypeInfo = {
    "hkpBallAndSocketConstraintData",
    finishLoadedObjecthkpBallAndSocketConstraintData,
    cleanupLoadedObjecthkpBallAndSocketConstraintData,
    getVtablehkpBallAndSocketConstraintData(),
    &typeid(hkpBallAndSocketConstraintData),
    sizeof(hkpBallAndSocketConstraintData)
};

const hkClass& hkpBallAndSocketConstraintData::Atoms::staticClass() {
    return hkpBallAndSocketConstraintDataAtomsClass;
}

// The atoms have no constructors, so their types and defaults are set here.
hkpBallAndSocketConstraintData::hkpBallAndSocketConstraintData() {
    m_atoms.m_pivots.m_type.m_storage = 3;
    m_atoms.m_ballSocket.m_bodiesToNotify = 0;
    m_atoms.m_ballSocket.m_type.m_storage = 5;
    m_atoms.m_ballSocket.m_stabilizationFactor = 1.0f;
    m_atoms.m_ballSocket.m_maxImpulse = HK_REAL_MAX;
    m_atoms.m_pivots.m_translationA.m_quad = _mm_setzero_ps();
    m_atoms.m_pivots.m_translationB.m_quad = _mm_setzero_ps();
}

void hkpBallAndSocketConstraintData::setMaxLinearImpulse(float maxImpulse) {
    m_atoms.m_ballSocket.m_maxImpulse = maxImpulse;
}

float hkpBallAndSocketConstraintData::getMaxLinearImpulse() const {
    return m_atoms.m_ballSocket.m_maxImpulse;
}

void hkpBallAndSocketConstraintData::setBodyToNotify(int32_t bodyIdx) {
    m_atoms.m_ballSocket.m_bodiesToNotify = static_cast<uint8_t>(1 << bodyIdx);
}

uint8_t hkpBallAndSocketConstraintData::getNotifiedBodyIndex() const {
    return m_atoms.m_ballSocket.m_bodiesToNotify >> 1;
}

hkBool hkpBallAndSocketConstraintData::isValid() const {
    return 1;
}

int32_t hkpBallAndSocketConstraintData::getType() const {
    return 0;
}

void hkpBallAndSocketConstraintData::getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const {
    if (wantRuntime.m_bool || m_atoms.m_ballSocket.m_maxImpulse != 3.402820018375656e+38) {
        infoOut.m_numSolverResults = 3;
        infoOut.m_sizeOfExternalRuntime = 24;
    }
    else {
        infoOut.m_numSolverResults = 0;
        infoOut.m_sizeOfExternalRuntime = 0;
    }
}

void hkpBallAndSocketConstraintData::getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const {
    hkpConstraintData::getConstraintInfoUtil(&m_atoms.m_pivots, 56, infoOut);
}

void hkpBallAndSocketConstraintData::setInWorldSpace(
    const hkTransform& bodyATransform,
    const hkTransform& bodyBTransform,
    const hkVector4& pivot
) {
    m_atoms.m_pivots.m_translationA.setTransformedInversePos(bodyATransform, pivot);
    m_atoms.m_pivots.m_translationB.setTransformedInversePos(bodyBTransform, pivot);
}

void hkpBallAndSocketConstraintData::setInBodySpace(const hkVector4& pivotA, const hkVector4& pivotB) {
    m_atoms.m_pivots.m_translationA.m_quad = pivotA.m_quad;
    m_atoms.m_pivots.m_translationB.m_quad = pivotB.m_quad;
}

const hkClass& hkpBallAndSocketConstraintData::staticClass() {
    return hkpBallAndSocketConstraintDataClass;
}
