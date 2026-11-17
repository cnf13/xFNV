#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinFrictionConstraintAtom.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpLinFrictionConstraintAtomTypeInfo = {"hkpLinFrictionConstraintAtom",
	finishLoadedObjecthkpLinFrictionConstraintAtom, cleanupLoadedObjecthkpLinFrictionConstraintAtom, nullptr,
	&typeid(hkpLinFrictionConstraintAtom), sizeof(hkpLinFrictionConstraintAtom)};

// The reflected members of hkpLinFrictionConstraintAtom.
using hkpLinFrictionConstraintAtomClass_MembersArray = const hkClassMember[3];
static hkpLinFrictionConstraintAtomClass_MembersArray hkpLinFrictionConstraintAtomClass_Members{
	{"isEnabled", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x2, nullptr},
	{"frictionAxis", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x3, nullptr},
	{"maxFrictionForce", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x4, nullptr}};

hkClass hkpLinFrictionConstraintAtomClass("hkpLinFrictionConstraintAtom", &hkpConstraintAtomClass, 8, nullptr, 0,
	nullptr, 0, hkpLinFrictionConstraintAtomClass_Members, 3, nullptr, nullptr, 0, 0);

const hkClass& hkpLinFrictionConstraintAtom::staticClass() {
	return hkpLinFrictionConstraintAtomClass;
}
