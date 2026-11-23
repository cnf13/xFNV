#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/StiffSpring/hkpStiffSpringConstraintData.h"

#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTranslationsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpStiffSpringConstraintAtom.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

#include <xmmintrin.h>

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpStiffSpringConstraintDataAtomsTypeInfo = {
    "hkpStiffSpringConstraintDataAtoms",
    finishLoadedObjecthkpStiffSpringConstraintDataAtoms,
    cleanupLoadedObjecthkpStiffSpringConstraintDataAtoms,
    nullptr,
    &typeid(hkpStiffSpringConstraintData::Atoms),
    sizeof(hkpStiffSpringConstraintData::Atoms)
};

// The reflected members of hkpStiffSpringConstraintData::Atoms.
using hkpStiffSpringConstraintDataAtomsClass_MembersArray = const hkClassMember[2];
static hkpStiffSpringConstraintDataAtomsClass_MembersArray hkpStiffSpringConstraintDataAtomsClass_Members{
    {"pivots",
     &hkpSetLocalTranslationsConstraintAtomClass,
     nullptr,
     hkClassMember::TYPE_STRUCT,
     hkClassMember::TYPE_VOID,
     0,
     0,
     0x0,
     nullptr},
    {"spring",
     &hkpStiffSpringConstraintAtomClass,
     nullptr,
     hkClassMember::TYPE_STRUCT,
     hkClassMember::TYPE_VOID,
     0,
     0,
     0x30,
     nullptr}
};

hkClass hkpStiffSpringConstraintDataAtomsClass(
    "hkpStiffSpringConstraintDataAtoms",
    nullptr,
    64,
    nullptr,
    0,
    nullptr,
    0,
    hkpStiffSpringConstraintDataAtomsClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);

// The reflected members of hkpStiffSpringConstraintData.
using hkpStiffSpringConstraintDataClass_MembersArray = const hkClassMember[1];
static hkpStiffSpringConstraintDataClass_MembersArray hkpStiffSpringConstraintDataClass_Members{
    {"atoms",
     &hkpStiffSpringConstraintDataAtomsClass,
     nullptr,
     hkClassMember::TYPE_STRUCT,
     hkClassMember::TYPE_VOID,
     0,
     {hkClassMember::ALIGN_16},
     0x10,
     nullptr}
};

hkClass hkpStiffSpringConstraintDataClass(
    "hkpStiffSpringConstraintData",
    &hkpConstraintDataClass,
    80,
    nullptr,
    0,
    nullptr,
    0,
    hkpStiffSpringConstraintDataClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);

hkTypeInfo hkpStiffSpringConstraintDataTypeInfo = {
    "hkpStiffSpringConstraintData",
    finishLoadedObjecthkpStiffSpringConstraintData,
    cleanupLoadedObjecthkpStiffSpringConstraintData,
    getVtablehkpStiffSpringConstraintData(),
    &typeid(hkpStiffSpringConstraintData),
    sizeof(hkpStiffSpringConstraintData)
};

const hkClass& hkpStiffSpringConstraintData::Atoms::staticClass() {
    return hkpStiffSpringConstraintDataAtomsClass;
}

hkpStiffSpringConstraintData::hkpStiffSpringConstraintData() {
    m_atoms.m_pivots.m_type.m_storage = 3;
    m_atoms.m_spring.m_type.m_storage = 6;
    m_atoms.m_spring.m_length = 0.0f;
    m_atoms.m_pivots.m_translationA.m_quad = _mm_setzero_ps();
    m_atoms.m_pivots.m_translationB.m_quad = _mm_setzero_ps();
}

hkBool hkpStiffSpringConstraintData::isValid() const {
    return m_atoms.m_spring.m_length > 0.0;
}

int32_t hkpStiffSpringConstraintData::getType() const {
    return 8;
}

void hkpStiffSpringConstraintData::getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const {
    if (wantRuntime.m_bool) {
        infoOut.m_numSolverResults = 1;
        infoOut.m_sizeOfExternalRuntime = 8;
    }
    else {
        infoOut.m_numSolverResults = 0;
        infoOut.m_sizeOfExternalRuntime = 0;
    }
}

void hkpStiffSpringConstraintData::getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const {
    hkpConstraintData::getConstraintInfoUtil(&m_atoms.m_pivots, 56, infoOut);
}

const hkClass& hkpStiffSpringConstraintData::staticClass() {
    return hkpStiffSpringConstraintDataClass;
}
