#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalRotationsConstraintAtom.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpSetLocalRotationsConstraintAtomTypeInfo = {
    "hkpSetLocalRotationsConstraintAtom",
    finishLoadedObjecthkpSetLocalRotationsConstraintAtom,
    cleanupLoadedObjecthkpSetLocalRotationsConstraintAtom,
    nullptr,
    &typeid(hkpSetLocalRotationsConstraintAtom),
    sizeof(hkpSetLocalRotationsConstraintAtom)
};

// The reflected members of hkpSetLocalRotationsConstraintAtom.
using hkpSetLocalRotationsConstraintAtomClass_MembersArray = const hkClassMember[2];
static hkpSetLocalRotationsConstraintAtomClass_MembersArray hkpSetLocalRotationsConstraintAtomClass_Members{
    {"rotationA", nullptr, nullptr, hkClassMember::TYPE_ROTATION, hkClassMember::TYPE_VOID, 0, 0, 0x10, nullptr},
    {"rotationB", nullptr, nullptr, hkClassMember::TYPE_ROTATION, hkClassMember::TYPE_VOID, 0, 0, 0x40, nullptr}
};

hkClass hkpSetLocalRotationsConstraintAtomClass(
    "hkpSetLocalRotationsConstraintAtom",
    &hkpConstraintAtomClass,
    0x70,
    nullptr,
    0,
    nullptr,
    0,
    hkpSetLocalRotationsConstraintAtomClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
