#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpPulleyConstraintAtom.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpPulleyConstraintAtomTypeInfo = {"hkpPulleyConstraintAtom", finishLoadedObjecthkpPulleyConstraintAtom,
	cleanupLoadedObjecthkpPulleyConstraintAtom, nullptr, &typeid(hkpPulleyConstraintAtom),
	sizeof(hkpPulleyConstraintAtom)};

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The reflected members of
// hkpPulleyConstraintAtom, as the exe's .rdata holds them.
using hkpPulleyConstraintAtomClass_MembersArray = const hkClassMember[4];
static hkpPulleyConstraintAtomClass_MembersArray hkpPulleyConstraintAtomClass_Members{
	{"fixedPivotAinWorld", nullptr, nullptr, hkClassMember::TYPE_VECTOR4, hkClassMember::TYPE_VOID, 0, 0, 0x10,
		nullptr},
	{"fixedPivotBinWorld", nullptr, nullptr, hkClassMember::TYPE_VECTOR4, hkClassMember::TYPE_VOID, 0, 0, 0x20,
		nullptr},
	{"ropeLength", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x30, nullptr},
	{"leverageOnBodyB", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x34, nullptr}};

hkClass hkpPulleyConstraintAtomClass("hkpPulleyConstraintAtom", &hkpConstraintAtomClass, 64, nullptr, 0, nullptr, 0,
	hkpPulleyConstraintAtomClass_Members, 4, nullptr, nullptr, 0, 0);

const hkClass& hkpPulleyConstraintAtom::staticClass() {
	return hkpPulleyConstraintAtomClass;
}
