#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinLimitConstraintAtom.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpLinLimitConstraintAtomTypeInfo = {"hkpLinLimitConstraintAtom",
	finishLoadedObjecthkpLinLimitConstraintAtom, cleanupLoadedObjecthkpLinLimitConstraintAtom, nullptr,
	&typeid(hkpLinLimitConstraintAtom), sizeof(hkpLinLimitConstraintAtom)};

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The reflected members of
// hkpLinLimitConstraintAtom, as the exe's .rdata holds them.
using hkpLinLimitConstraintAtomClass_MembersArray = const hkClassMember[3];
static hkpLinLimitConstraintAtomClass_MembersArray hkpLinLimitConstraintAtomClass_Members{
	{"axisIndex", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x2, nullptr},
	{"min", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x4, nullptr},
	{"max", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x8, nullptr}};

hkClass hkpLinLimitConstraintAtomClass("hkpLinLimitConstraintAtom", &hkpConstraintAtomClass, 12, nullptr, 0, nullptr, 0,
	hkpLinLimitConstraintAtomClass_Members, 3, nullptr, nullptr, 0, 0);

const hkClass& hkpLinLimitConstraintAtom::staticClass() {
	return hkpLinLimitConstraintAtomClass;
}
