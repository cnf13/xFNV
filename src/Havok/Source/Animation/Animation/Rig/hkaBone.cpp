#include "Havok/Source/Animation/Animation/Rig/hkaBone.h"

#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Dynamics/Constraint/ConstraintKit/hkpGenericConstraintDataScheme.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkaBoneTypeInfo = {"hkaBone", finishLoadedObjecthkaBone, cleanupLoadedObjecthkaBone, nullptr,
	&typeid(hkaBone), sizeof(hkaBone)};

// The class record's member table.
using hkaBoneClassMemberArray = const hkClassMember[2];
static hkaBoneClassMemberArray hkaBoneClass_Members{
	{"name", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, 0, 0x0, nullptr},
	{"lockTranslation", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, 0, 0x4, nullptr}};
hkClass hkaBoneClass("hkaBone", nullptr, 8, nullptr, 0, nullptr, 0, hkaBoneClass_Members, 2, nullptr, nullptr, 0, 0);

const hkClass& hkaBone::staticClass() {
	return hkaBoneClass;
}

const hkClass& hkaBone::staticClass_00DFCDA0() {
	return hkpGenericConstraintDataSchemeClass;
}
