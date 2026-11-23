#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngFrictionConstraintAtom.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpAngFrictionConstraintAtomTypeInfo = {
    "hkpAngFrictionConstraintAtom",
    finishLoadedObjecthkpAngFrictionConstraintAtom,
    cleanupLoadedObjecthkpAngFrictionConstraintAtom,
    nullptr,
    &typeid(hkpAngFrictionConstraintAtom),
    sizeof(hkpAngFrictionConstraintAtom)
};

// The reflected members of hkpAngFrictionConstraintAtom.
using hkpAngFrictionConstraintAtomClass_MembersArray = const hkClassMember[4];
static hkpAngFrictionConstraintAtomClass_MembersArray hkpAngFrictionConstraintAtomClass_Members{
    {"isEnabled", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x2, nullptr},
    {"firstFrictionAxis", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x3, nullptr},
    {"numFrictionAxes", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x4, nullptr},
    {"maxFrictionTorque", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x8, nullptr}
};

hkClass hkpAngFrictionConstraintAtomClass(
    "hkpAngFrictionConstraintAtom",
    &hkpConstraintAtomClass,
    12,
    nullptr,
    0,
    nullptr,
    0,
    hkpAngFrictionConstraintAtomClass_Members,
    4,
    nullptr,
    nullptr,
    0,
    0
);

const hkClass& hkpAngFrictionConstraintAtom::staticClass() {
    return hkpAngFrictionConstraintAtomClass;
}
