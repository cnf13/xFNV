#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngConstraintAtom.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpAngConstraintAtomTypeInfo = {"hkpAngConstraintAtom", finishLoadedObjecthkpAngConstraintAtom,
	cleanupLoadedObjecthkpAngConstraintAtom, nullptr, &typeid(hkpAngConstraintAtom), sizeof(hkpAngConstraintAtom)};

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The reflected members of
// hkpAngConstraintAtom, as the exe's .rdata holds them.
using hkpAngConstraintAtomClass_MembersArray = const hkClassMember[2];
static hkpAngConstraintAtomClass_MembersArray hkpAngConstraintAtomClass_Members{
	{"firstConstrainedAxis", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x2, nullptr},
	{"numConstrainedAxes", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x3, nullptr}};

hkClass hkpAngConstraintAtomClass("hkpAngConstraintAtom", &hkpConstraintAtomClass, 4, nullptr, 0, nullptr, 0,
	hkpAngConstraintAtomClass_Members, 2, nullptr, nullptr, 0, 0);

const hkClass& hkpAngConstraintAtom::staticClass() {
	return hkpAngConstraintAtomClass;
}
