#include "Havok/Source/Physics/Dynamics/Constraint/ConstraintKit/hkpGenericConstraintDataScheme.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpConstraintMotor.h"

static void cleanupLoadedObjecthkpGenericConstraintDataSchemeConstraintInfo(void* p);
static void cleanupLoadedObjecthkpGenericConstraintDataScheme(void* p);
static void finishLoadedObjecthkpGenericConstraintDataScheme(void* p, int32_t finishing);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpGenericConstraintDataSchemeConstraintInfoTypeInfo = {"hkpGenericConstraintDataSchemeConstraintInfo",
	nullptr, cleanupLoadedObjecthkpGenericConstraintDataSchemeConstraintInfo, nullptr,
	&typeid(hkpGenericConstraintDataScheme::ConstraintInfo), sizeof(hkpGenericConstraintDataScheme::ConstraintInfo)};
hkTypeInfo hkpGenericConstraintDataSchemeTypeInfo = {"hkpGenericConstraintDataScheme",
	finishLoadedObjecthkpGenericConstraintDataScheme, cleanupLoadedObjecthkpGenericConstraintDataScheme, nullptr,
	&typeid(hkpGenericConstraintDataScheme), sizeof(hkpGenericConstraintDataScheme)};

// The reflected members of hkpGenericConstraintDataScheme::ConstraintInfo.
using hkpGenericConstraintDataSchemeConstraintInfoClass_MembersArray = const hkClassMember[4];
static hkpGenericConstraintDataSchemeConstraintInfoClass_MembersArray
	hkpGenericConstraintDataSchemeConstraintInfoClass_Members{
		{"maxSizeOfSchema", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0x0, nullptr},
		{"sizeOfSchemas", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0x4, nullptr},
		{"numSolverResults", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0x8, nullptr},
		{"numSolverElemTemps", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0xC,
			nullptr}};
hkClass hkpGenericConstraintDataSchemeConstraintInfoClass("hkpGenericConstraintDataSchemeConstraintInfo", nullptr, 0x10,
	nullptr, 0, nullptr, 0, hkpGenericConstraintDataSchemeConstraintInfoClass_Members, 4, nullptr, nullptr, 0, 0);

// The reflection's member table of hkpGenericConstraintDataScheme.
using hkpGenericConstraintDataSchemeClass_MembersArray = const hkClassMember[5];
static hkpGenericConstraintDataSchemeClass_MembersArray hkpGenericConstraintDataSchemeClass_Members{
	{"info", &hkpGenericConstraintDataSchemeConstraintInfoClass, nullptr, {hkClassMember::TYPE_STRUCT},
		{hkClassMember::TYPE_VOID}, 0, {hkClassMember::SERIALIZE_IGNORED},
		offsetof(hkpGenericConstraintDataScheme, m_info), nullptr},
	{"data", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_VECTOR4}, 0, {0},
		offsetof(hkpGenericConstraintDataScheme, m_data), nullptr},
	{"commands", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_INT32}, 0, {0},
		offsetof(hkpGenericConstraintDataScheme, m_commands), nullptr},
	{"modifiers", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_POINTER}, 0,
		{hkClassMember::SERIALIZE_IGNORED}, offsetof(hkpGenericConstraintDataScheme, m_modifiers), nullptr},
	{"motors", &hkpConstraintMotorClass, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_POINTER}, 0, {0},
		offsetof(hkpGenericConstraintDataScheme, m_motors), nullptr}};

hkClass hkpGenericConstraintDataSchemeClass("hkpGenericConstraintDataScheme", nullptr,
	sizeof(hkpGenericConstraintDataScheme), nullptr, 0, nullptr, 0, hkpGenericConstraintDataSchemeClass_Members, 5,
	nullptr, nullptr, 0, 0);

static void cleanupLoadedObjecthkpGenericConstraintDataSchemeConstraintInfo(void* p) {
	;
}

// The cleanup function the reflection registers for hkpGenericConstraintDataScheme: destroys the
// loaded object in place.
static void cleanupLoadedObjecthkpGenericConstraintDataScheme(void* p) {
	hkpGenericConstraintDataScheme* pScheme = static_cast<hkpGenericConstraintDataScheme*>(p);
	pScheme->~hkpGenericConstraintDataScheme();
}

static void finishLoadedObjecthkpGenericConstraintDataScheme(void* p, int32_t finishing) {
	if (p) {
		::new (p) hkpGenericConstraintDataScheme(static_cast<hkFinishLoadedObjectFlag>(finishing));
	}
}

const hkClass& hkpGenericConstraintDataScheme::ConstraintInfo::staticClass() {
	return hkpGenericConstraintDataSchemeConstraintInfoClass;
}
