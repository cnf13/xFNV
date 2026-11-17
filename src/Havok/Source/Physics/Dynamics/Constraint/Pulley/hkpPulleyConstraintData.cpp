#include "Havok/Source/Physics/Dynamics/Constraint/Pulley/hkpPulleyConstraintData.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpPulleyConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTranslationsConstraintAtom.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

#include <xmmintrin.h>

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpPulleyConstraintDataAtomsTypeInfo = {
    "hkpPulleyConstraintDataAtoms",
    finishLoadedObjecthkpPulleyConstraintDataAtoms,
    cleanupLoadedObjecthkpPulleyConstraintDataAtoms,
    nullptr,
    &typeid(hkpPulleyConstraintData::Atoms),
    sizeof(hkpPulleyConstraintData::Atoms)
};

// The reflected members of hkpPulleyConstraintData::Atoms.
using hkpPulleyConstraintDataAtomsClass_MembersArray = const hkClassMember[2];
static hkpPulleyConstraintDataAtomsClass_MembersArray hkpPulleyConstraintDataAtomsClass_Members{
    {"translations",
     &hkpSetLocalTranslationsConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x0,
     nullptr},
    {"pulley",
     &hkpPulleyConstraintAtomClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x30,
     nullptr}
};

hkClass hkpPulleyConstraintDataAtomsClass(
    "hkpPulleyConstraintDataAtoms",
    nullptr,
    112,
    nullptr,
    0,
    nullptr,
    0,
    hkpPulleyConstraintDataAtomsClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);

// The reflected members of hkpPulleyConstraintData: the atoms are 16-aligned (ALIGN_16).
using hkpPulleyConstraintDataClass_MembersArray = const hkClassMember[1];
static hkpPulleyConstraintDataClass_MembersArray hkpPulleyConstraintDataClass_Members{
    {"atoms",
     &hkpPulleyConstraintDataAtomsClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0x100},
     0x10,
     nullptr}
};

hkClass hkpPulleyConstraintDataClass(
    "hkpPulleyConstraintData",
    &hkpConstraintDataClass,
    128,
    nullptr,
    0,
    nullptr,
    0,
    hkpPulleyConstraintDataClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpPulleyConstraintDataTypeInfo = {
    "hkpPulleyConstraintData",
    finishLoadedObjecthkpPulleyConstraintData,
    cleanupLoadedObjecthkpPulleyConstraintData,
    getVtablehkpPulleyConstraintData(),
    &typeid(hkpPulleyConstraintData),
    sizeof(hkpPulleyConstraintData)
};

const hkClass& hkpPulleyConstraintData::Atoms::staticClass() {
    return hkpPulleyConstraintDataAtomsClass;
}

hkpPulleyConstraintData::hkpPulleyConstraintData() {
    m_atoms.m_translations.m_type.m_storage = 3;
    m_atoms.m_pulley.m_type.m_storage = 20;
    m_atoms.m_pulley.m_ropeLength = 0.0f;
    m_atoms.m_pulley.m_leverageOnBodyB = 1.0f;
    m_atoms.m_translations.m_translationA.m_quad = _mm_setzero_ps();
    m_atoms.m_translations.m_translationB.m_quad = _mm_setzero_ps();
    m_atoms.m_pulley.m_fixedPivotAinWorld.m_quad = _mm_setzero_ps();
    m_atoms.m_pulley.m_fixedPivotBinWorld.m_quad = _mm_setzero_ps();
}

hkBool* hkpPulleyConstraintData::isValid(hkBool* arResult) const {
    if (!(m_atoms.m_pulley.m_ropeLength > 0.0f) || !(m_atoms.m_pulley.m_leverageOnBodyB > 0.0f)) {
        arResult->m_bool = 0;
        return arResult;
    }

    arResult->m_bool = 1;
    return arResult;
}

int32_t hkpPulleyConstraintData::getType() const {
    return CONSTRAINT_TYPE_PULLEY;
}

void hkpPulleyConstraintData::getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const {
    if (wantRuntime.m_bool) {
        infoOut.m_numSolverResults = 1;
        infoOut.m_sizeOfExternalRuntime = 8;
    }
    else {
        infoOut.m_numSolverResults = 0;
        infoOut.m_sizeOfExternalRuntime = 0;
    }
}

void hkpPulleyConstraintData::getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const {
    hkpConstraintData::getConstraintInfoUtil(&m_atoms.m_translations, 112, infoOut);
}

const hkClass& hkpPulleyConstraintData::staticClass() {
    return hkpPulleyConstraintDataClass;
}
