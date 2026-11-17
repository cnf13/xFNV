#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTransformsConstraintAtom.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

static void finishLoadedObjecthkpSetLocalTransformsConstraintAtom(void* p, int32_t finishing);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpSetLocalTransformsConstraintAtomTypeInfo = {"hkpSetLocalTransformsConstraintAtom",
	finishLoadedObjecthkpSetLocalTransformsConstraintAtom, cleanupLoadedObjecthkpSetLocalTransformsConstraintAtom,
	nullptr, &typeid(hkpSetLocalTransformsConstraintAtom), sizeof(hkpSetLocalTransformsConstraintAtom)};

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The reflected members of
// hkpSetLocalTransformsConstraintAtom, as the exe's .rdata holds them.
using hkpSetLocalTransformsConstraintAtomClass_MembersArray = const hkClassMember[2];
static hkpSetLocalTransformsConstraintAtomClass_MembersArray hkpSetLocalTransformsConstraintAtomClass_Members{
	{"transformA", nullptr, nullptr, hkClassMember::TYPE_TRANSFORM, hkClassMember::TYPE_VOID, 0, 0, 0x10, nullptr},
	{"transformB", nullptr, nullptr, hkClassMember::TYPE_TRANSFORM, hkClassMember::TYPE_VOID, 0, 0, 0x50, nullptr}};
hkClass hkpSetLocalTransformsConstraintAtomClass("hkpSetLocalTransformsConstraintAtom", &hkpConstraintAtomClass, 0x90,
	nullptr, 0, nullptr, 0, hkpSetLocalTransformsConstraintAtomClass_Members, 2, nullptr, nullptr, 0, 0);

static void finishLoadedObjecthkpSetLocalTransformsConstraintAtom(void* p, int32_t finishing) {
	;
}
