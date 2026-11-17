#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinMotorConstraintAtom.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpConstraintMotor.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpLinMotorConstraintAtomTypeInfo = {"hkpLinMotorConstraintAtom",
	finishLoadedObjecthkpLinMotorConstraintAtom, cleanupLoadedObjecthkpLinMotorConstraintAtom, nullptr,
	&typeid(hkpLinMotorConstraintAtom), sizeof(hkpLinMotorConstraintAtom)};

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The reflected members of
// hkpLinMotorConstraintAtom, as the exe's .rdata holds them.
using hkpLinMotorConstraintAtomClass_MembersArray = const hkClassMember[6];
static hkpLinMotorConstraintAtomClass_MembersArray hkpLinMotorConstraintAtomClass_Members{
	{"isEnabled", nullptr, nullptr, hkClassMember::TYPE_BOOL, hkClassMember::TYPE_VOID, 0, 0, 0x2, nullptr},
	{"motorAxis", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x3, nullptr},
	{"initializedOffset", nullptr, nullptr, hkClassMember::TYPE_INT16, hkClassMember::TYPE_VOID, 0, 0, 0x4, nullptr},
	{"previousTargetPositionOffset", nullptr, nullptr, hkClassMember::TYPE_INT16, hkClassMember::TYPE_VOID, 0, 0, 0x6,
		nullptr},
	{"targetPosition", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x8, nullptr},
	{"motor", &hkpConstraintMotorClass, nullptr, hkClassMember::TYPE_POINTER, hkClassMember::TYPE_STRUCT, 0, 0, 0xC,
		nullptr}};

hkClass hkpLinMotorConstraintAtomClass("hkpLinMotorConstraintAtom", &hkpConstraintAtomClass, 16, nullptr, 0, nullptr, 0,
	hkpLinMotorConstraintAtomClass_Members, 6, nullptr, nullptr, 0, 0);

const hkClass& hkpLinMotorConstraintAtom::staticClass() {
	return hkpLinMotorConstraintAtomClass;
}
