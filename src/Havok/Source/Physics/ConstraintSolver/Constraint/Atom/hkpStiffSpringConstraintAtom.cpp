#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpStiffSpringConstraintAtom.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpStiffSpringConstraintAtomTypeInfo = {"hkpStiffSpringConstraintAtom",
	finishLoadedObjecthkpStiffSpringConstraintAtom, cleanupLoadedObjecthkpStiffSpringConstraintAtom, nullptr,
	&typeid(hkpStiffSpringConstraintAtom), sizeof(hkpStiffSpringConstraintAtom)};

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The reflected members of
// hkpStiffSpringConstraintAtom, as the exe's .rdata holds them.
using hkpStiffSpringConstraintAtomClass_MembersArray = const hkClassMember[1];
static hkpStiffSpringConstraintAtomClass_MembersArray hkpStiffSpringConstraintAtomClass_Members{
	{"length", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x4, nullptr}};

hkClass hkpStiffSpringConstraintAtomClass("hkpStiffSpringConstraintAtom", &hkpConstraintAtomClass, 8, nullptr, 0,
	nullptr, 0, hkpStiffSpringConstraintAtomClass_Members, 1, nullptr, nullptr, 0, 0);

const hkClass& hkpStiffSpringConstraintAtom::staticClass() {
	return hkpStiffSpringConstraintAtomClass;
}
