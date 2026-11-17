#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngMotorConstraintAtom.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpConstraintMotor.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpAngMotorConstraintAtomTypeInfo = {
    "hkpAngMotorConstraintAtom",
    finishLoadedObjecthkpAngMotorConstraintAtom,
    cleanupLoadedObjecthkpAngMotorConstraintAtom,
    nullptr,
    &typeid(hkpAngMotorConstraintAtom),
    sizeof(hkpAngMotorConstraintAtom)
};

// The reflected members of hkpAngMotorConstraintAtom.
using hkpAngMotorConstraintAtomClass_MembersArray = const hkClassMember[7];
static hkpAngMotorConstraintAtomClass_MembersArray hkpAngMotorConstraintAtomClass_Members{
    {"isEnabled", nullptr, nullptr, hkClassMember::TYPE_BOOL, hkClassMember::TYPE_VOID, 0, 0, 0x2, nullptr},
    {"motorAxis", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x3, nullptr},
    {"initializedOffset", nullptr, nullptr, hkClassMember::TYPE_INT16, hkClassMember::TYPE_VOID, 0, 0, 0x4, nullptr},
    {"previousTargetAngleOffset", nullptr, nullptr, hkClassMember::TYPE_INT16, hkClassMember::TYPE_VOID, 0, 0, 0x6,
     nullptr},
    {"correspondingAngLimitSolverResultOffset", nullptr, nullptr, hkClassMember::TYPE_INT16, hkClassMember::TYPE_VOID,
     0, 0, 0x8, nullptr},
    {"targetAngle", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0xC, nullptr},
    {"motor", &hkpConstraintMotorClass, nullptr, hkClassMember::TYPE_POINTER, hkClassMember::TYPE_STRUCT, 0, 0, 0x10,
     nullptr}
};

hkClass hkpAngMotorConstraintAtomClass(
    "hkpAngMotorConstraintAtom",
    &hkpConstraintAtomClass,
    20,
    nullptr,
    0,
    nullptr,
    0,
    hkpAngMotorConstraintAtomClass_Members,
    7,
    nullptr,
    nullptr,
    0,
    0
);

const hkClass& hkpAngMotorConstraintAtom::staticClass() {
    return hkpAngMotorConstraintAtomClass;
}
