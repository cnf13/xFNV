#include "Havok/Source/Common/Serialize/Packfile/Binary/hkPackfileHeader.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkPackfileHeaderTypeInfo = {"hkPackfileHeader", nullptr, cleanupLoadedObjecthkPackfileHeader, nullptr,
	&typeid(hkPackfileHeader), sizeof(hkPackfileHeader)};

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The reflected members of
// hkPackfileHeader, as the exe's .rdata holds them.
using hkPackfileHeaderClass_MembersArray = const hkClassMember[11];
static hkPackfileHeaderClass_MembersArray hkPackfileHeaderClass_Members{
	{"magic", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 2, 0, 0x0, nullptr},
	{"userTag", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0x8, nullptr},
	{"fileVersion", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0xC, nullptr},
	{"layoutRules", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 4, 0, 0x10, nullptr},
	{"numSections", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0x14, nullptr},
	{"contentsSectionIndex", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0x18,
		nullptr},
	{"contentsSectionOffset", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0x1C,
		nullptr},
	{"contentsClassNameSectionIndex", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0x20,
		nullptr},
	{"contentsClassNameSectionOffset", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0,
		0x24, nullptr},
	{"contentsVersion", nullptr, nullptr, hkClassMember::TYPE_CHAR, hkClassMember::TYPE_VOID, 16, 0, 0x28, nullptr},
	{"pad", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 2, 0, 0x38, nullptr}};

hkClass hkPackfileHeaderClass("hkPackfileHeader", nullptr, 0x40, nullptr, 0, nullptr, 0, hkPackfileHeaderClass_Members,
	11, nullptr, nullptr, 0, 0);
