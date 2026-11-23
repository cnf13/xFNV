#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinConstraintAtom.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpLinConstraintAtomTypeInfo = {
    "hkpLinConstraintAtom",
    finishLoadedObjecthkpLinConstraintAtom,
    cleanupLoadedObjecthkpLinConstraintAtom,
    nullptr,
    &typeid(hkpLinConstraintAtom),
    sizeof(hkpLinConstraintAtom)
};

// The reflected members of hkpLinConstraintAtom.
using hkpLinConstraintAtomClass_MembersArray = const hkClassMember[1];
static hkpLinConstraintAtomClass_MembersArray hkpLinConstraintAtomClass_Members{
    {"axisIndex", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x2, nullptr}
};

hkClass hkpLinConstraintAtomClass(
    "hkpLinConstraintAtom",
    &hkpConstraintAtomClass,
    4,
    nullptr,
    0,
    nullptr,
    0,
    hkpLinConstraintAtomClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);

const hkClass& hkpLinConstraintAtom::staticClass() {
    return hkpLinConstraintAtomClass;
}
