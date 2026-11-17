#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkp2dAngConstraintAtom.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkp2dAngConstraintAtomTypeInfo = {"hkp2dAngConstraintAtom", finishLoadedObjecthkp2dAngConstraintAtom,
	cleanupLoadedObjecthkp2dAngConstraintAtom, nullptr, &typeid(hkp2dAngConstraintAtom),
	sizeof(hkp2dAngConstraintAtom)};

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The reflected members of
// hkp2dAngConstraintAtom, as the exe's .rdata holds them.
using hkp2dAngConstraintAtomClass_MembersArray = const hkClassMember[1];
static hkp2dAngConstraintAtomClass_MembersArray hkp2dAngConstraintAtomClass_Members{
	{"freeRotationAxis", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x2, nullptr}};

hkClass hkp2dAngConstraintAtomClass("hkp2dAngConstraintAtom", &hkpConstraintAtomClass, 4, nullptr, 0, nullptr, 0,
	hkp2dAngConstraintAtomClass_Members, 1, nullptr, nullptr, 0, 0);

const hkClass& hkp2dAngConstraintAtom::staticClass() {
	return hkp2dAngConstraintAtomClass;
}
