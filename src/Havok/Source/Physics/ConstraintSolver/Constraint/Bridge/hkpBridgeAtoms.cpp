#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

static void cleanupLoadedObjecthkpBridgeConstraintAtom(void* p);
static void cleanupLoadedObjecthkpBridgeAtoms(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpBridgeConstraintAtomTypeInfo = {"hkpBridgeConstraintAtom", finishLoadedObjecthkpBridgeConstraintAtom,
	cleanupLoadedObjecthkpBridgeConstraintAtom, nullptr, &typeid(hkpBridgeConstraintAtom),
	sizeof(hkpBridgeConstraintAtom)};
hkTypeInfo hkpBridgeAtomsTypeInfo = {"hkpBridgeAtoms", finishLoadedObjecthkpBridgeAtoms,
	cleanupLoadedObjecthkpBridgeAtoms, nullptr, &typeid(hkpBridgeAtoms), sizeof(hkpBridgeAtoms)};

// Not named in the PDB or IDA; names chosen here (the SDK's class-file names). The reflected members of
// hkpBridgeConstraintAtom and of hkpBridgeAtoms, as the exe's .rdata holds them.
using hkpBridgeConstraintAtomClass_MembersArray = const hkClassMember[2];
static hkpBridgeConstraintAtomClass_MembersArray hkpBridgeConstraintAtomClass_Members{
	{"buildJacobianFunc", nullptr, nullptr, hkClassMember::TYPE_POINTER, hkClassMember::TYPE_VOID, 0,
		{hkClassMember::SERIALIZE_IGNORED}, 0x4, nullptr},
	{"constraintData", &hkpConstraintDataClass, nullptr, hkClassMember::TYPE_POINTER, hkClassMember::TYPE_STRUCT, 0,
		{hkClassMember::NOT_OWNED}, 0x8, nullptr}};
hkClass hkpBridgeConstraintAtomClass("hkpBridgeConstraintAtom", &hkpConstraintAtomClass, 12, nullptr, 0, nullptr, 0,
	hkpBridgeConstraintAtomClass_Members, 2, nullptr, nullptr, 0, 0);
using hkpBridgeAtomsClass_MembersArray = const hkClassMember[1];
static hkpBridgeAtomsClass_MembersArray hkpBridgeAtomsClass_Members{{"bridgeAtom", &hkpBridgeConstraintAtomClass,
	nullptr, hkClassMember::TYPE_STRUCT, hkClassMember::TYPE_VOID, 0, 0, 0x0, nullptr}};
hkClass hkpBridgeAtomsClass("hkpBridgeAtoms", nullptr, 12, nullptr, 0, nullptr, 0, hkpBridgeAtomsClass_Members, 1,
	nullptr, nullptr, 0, 0);

static void cleanupLoadedObjecthkpBridgeConstraintAtom(void* p) {
	;
}

static void cleanupLoadedObjecthkpBridgeAtoms(void* p) {
	;
}

const hkClass& hkpBridgeAtoms::staticClass() {
	return hkpBridgeAtomsClass;
}
