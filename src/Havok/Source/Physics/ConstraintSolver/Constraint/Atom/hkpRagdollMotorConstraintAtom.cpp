#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpRagdollMotorConstraintAtom.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpConstraintMotor.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpRagdollMotorConstraintAtomTypeInfo = {
    "hkpRagdollMotorConstraintAtom",
    finishLoadedObjecthkpRagdollMotorConstraintAtom,
    cleanupLoadedObjecthkpRagdollMotorConstraintAtom,
    nullptr,
    &typeid(hkpRagdollMotorConstraintAtom),
    sizeof(hkpRagdollMotorConstraintAtom)
};

// The reflected members of hkpRagdollMotorConstraintAtom.
using hkpRagdollMotorConstraintAtomClass_MembersArray = const hkClassMember[5];
static hkpRagdollMotorConstraintAtomClass_MembersArray hkpRagdollMotorConstraintAtomClass_Members{
    {"isEnabled", nullptr, nullptr, hkClassMember::TYPE_BOOL, hkClassMember::TYPE_VOID, 0, 0, 0x2, nullptr},
    {"initializedOffset", nullptr, nullptr, hkClassMember::TYPE_INT16, hkClassMember::TYPE_VOID, 0, 0, 0x4, nullptr},
    {"previousTargetAnglesOffset",
     nullptr,
     nullptr,
     hkClassMember::TYPE_INT16,
     hkClassMember::TYPE_VOID,
     0,
     0,
     0x6,
     nullptr},
    {"target_bRca", nullptr, nullptr, hkClassMember::TYPE_MATRIX3, hkClassMember::TYPE_VOID, 0, 0, 0x10, nullptr},
    {"motors",
     &hkpConstraintMotorClass,
     nullptr,
     hkClassMember::TYPE_POINTER,
     hkClassMember::TYPE_STRUCT,
     3,
     0,
     0x40,
     nullptr}
};

hkClass hkpRagdollMotorConstraintAtomClass(
    "hkpRagdollMotorConstraintAtom",
    &hkpConstraintAtomClass,
    80,
    nullptr,
    0,
    nullptr,
    0,
    hkpRagdollMotorConstraintAtomClass_Members,
    5,
    nullptr,
    nullptr,
    0,
    0
);

const hkClass& hkpRagdollMotorConstraintAtom::staticClass() {
    return hkpRagdollMotorConstraintAtomClass;
}
