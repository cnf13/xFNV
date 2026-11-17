#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTranslationsConstraintAtom.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpSetLocalTranslationsConstraintAtomTypeInfo = {"hkpSetLocalTranslationsConstraintAtom",
	finishLoadedObjecthkpSetLocalTranslationsConstraintAtom, cleanupLoadedObjecthkpSetLocalTranslationsConstraintAtom,
	nullptr, &typeid(hkpSetLocalTranslationsConstraintAtom), sizeof(hkpSetLocalTranslationsConstraintAtom)};

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The reflected members of
// hkpSetLocalTranslationsConstraintAtom, as the exe's .rdata holds them.
using hkpSetLocalTranslationsConstraintAtomClass_MembersArray = const hkClassMember[2];
static hkpSetLocalTranslationsConstraintAtomClass_MembersArray hkpSetLocalTranslationsConstraintAtomClass_Members{
	{"translationA", nullptr, nullptr, hkClassMember::TYPE_VECTOR4, hkClassMember::TYPE_VOID, 0, 0, 0x10, nullptr},
	{"translationB", nullptr, nullptr, hkClassMember::TYPE_VECTOR4, hkClassMember::TYPE_VOID, 0, 0, 0x20, nullptr}};

hkClass hkpSetLocalTranslationsConstraintAtomClass("hkpSetLocalTranslationsConstraintAtom", &hkpConstraintAtomClass, 48,
	nullptr, 0, nullptr, 0, hkpSetLocalTranslationsConstraintAtomClass_Members, 2, nullptr, nullptr, 0, 0);

const hkClass& hkpSetLocalTranslationsConstraintAtom::staticClass() {
	return hkpSetLocalTranslationsConstraintAtomClass;
}
