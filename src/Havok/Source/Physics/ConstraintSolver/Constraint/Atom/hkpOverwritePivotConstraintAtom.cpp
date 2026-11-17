#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpOverwritePivotConstraintAtom.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpOverwritePivotConstraintAtomTypeInfo = {
    "hkpOverwritePivotConstraintAtom",
    finishLoadedObjecthkpOverwritePivotConstraintAtom,
    cleanupLoadedObjecthkpOverwritePivotConstraintAtom,
    nullptr,
    &typeid(hkpOverwritePivotConstraintAtom),
    sizeof(hkpOverwritePivotConstraintAtom)
};

// The reflected member of hkpOverwritePivotConstraintAtom.
using hkpOverwritePivotConstraintAtomClass_MembersArray = const hkClassMember[1];
static hkpOverwritePivotConstraintAtomClass_MembersArray hkpOverwritePivotConstraintAtomClass_Members{
    {"copyToPivotBFromPivotA", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x2,
     nullptr}
};

hkClass hkpOverwritePivotConstraintAtomClass(
    "hkpOverwritePivotConstraintAtom",
    &hkpConstraintAtomClass,
    4,
    nullptr,
    0,
    nullptr,
    0,
    hkpOverwritePivotConstraintAtomClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);
