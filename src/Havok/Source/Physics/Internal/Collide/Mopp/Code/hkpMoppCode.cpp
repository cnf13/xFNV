#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppCompilerChunkInfo.h"

namespace {
// Per member its offset into this struct (-1: no default), then the defaults (buildType
// BUILT_WITHOUT_CHUNK_SUBDIVISION).
struct hkpMoppCode_DefaultStruct {
	int32_t s_defaultOffsets[3];
	int8_t m_buildType;
};
} // namespace

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpMoppCodeReindexedTerminalTypeInfo = {"hkpMoppCodeReindexedTerminal", nullptr,
	cleanupLoadedObjecthkpMoppCodeReindexedTerminal, nullptr, &typeid(hkpMoppCodeReindexedTerminal),
	sizeof(hkpMoppCodeReindexedTerminal)};
hkTypeInfo hkpMoppCodeCodeInfoTypeInfo = {"hkpMoppCodeCodeInfo", nullptr, cleanupLoadedObjecthkpMoppCodeCodeInfo,
	nullptr, &typeid(hkpMoppCode::CodeInfo), sizeof(hkpMoppCode::CodeInfo)};

// The reflected members and enums of hkpMoppCodeReindexedTerminal, hkpMoppCode::CodeInfo and hkpMoppCode.
using hkpMoppCodeReindexedTerminalClass_MembersArray = const hkClassMember[2];
static hkpMoppCodeReindexedTerminalClass_MembersArray hkpMoppCodeReindexedTerminalClass_Members{
	{"origShapeKey", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x0, nullptr},
	{"reindexedShapeKey", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x4, nullptr}};
using hkpMoppCodeBuildTypeEnumItemsArray = const hkClassEnum::Item[3];
static hkpMoppCodeBuildTypeEnumItemsArray hkpMoppCodeBuildTypeEnumItems{
	{hkpMoppCode::BUILT_WITH_CHUNK_SUBDIVISION, "BUILT_WITH_CHUNK_SUBDIVISION"},
	{hkpMoppCode::BUILT_WITHOUT_CHUNK_SUBDIVISION, "BUILT_WITHOUT_CHUNK_SUBDIVISION"},
	{hkpMoppCode::BUILD_NOT_SET, "BUILD_NOT_SET"}};
using hkpMoppCodeEnumsArray = const hkClassEnum[1];
static hkpMoppCodeEnumsArray hkpMoppCodeEnums{{"BuildType", hkpMoppCodeBuildTypeEnumItems, 3, nullptr, {0}}};
using hkpMoppCodeCodeInfoClass_MembersArray = const hkClassMember[1];
static hkpMoppCodeCodeInfoClass_MembersArray hkpMoppCodeCodeInfoClass_Members{
	{"offset", nullptr, nullptr, hkClassMember::TYPE_VECTOR4, hkClassMember::TYPE_VOID, 0, 0, 0x0, nullptr}};

// The defaults the class record names.
static const hkpMoppCode_DefaultStruct hkpMoppCode_Default{{-1, -1, 0xC}, hkpMoppCode::BUILT_WITHOUT_CHUNK_SUBDIVISION};

static const hkClassEnum* hkpMoppCodeBuildTypeEnum = &hkpMoppCodeEnums[0];

using hkpMoppCodeClass_MembersArray = hkClassMember[3];
static hkpMoppCodeClass_MembersArray hkpMoppCodeClass_Members{{"info", &hkpMoppCodeCodeInfoClass, nullptr,
																  hkClassMember::TYPE_STRUCT, hkClassMember::TYPE_VOID,
																  0, 0, 0x10, nullptr},
	{"data", nullptr, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_UINT8, 0, 0, 0x20, nullptr},
	{"buildType", nullptr, hkpMoppCodeBuildTypeEnum, hkClassMember::TYPE_ENUM, hkClassMember::TYPE_INT8, 0, 0, 0x2C,
		nullptr}};
hkClass hkpMoppCodeReindexedTerminalClass("hkpMoppCodeReindexedTerminal", nullptr, 8, nullptr, 0, nullptr, 0,
	hkpMoppCodeReindexedTerminalClass_Members, 2, nullptr, nullptr, 0, 0);
hkClass hkpMoppCodeCodeInfoClass("hkpMoppCodeCodeInfo", nullptr, 16, nullptr, 0, nullptr, 0,
	hkpMoppCodeCodeInfoClass_Members, 1, nullptr, nullptr, 0, 0);
hkClass hkpMoppCodeClass("hkpMoppCode", &hkReferencedObjectClass, 48, nullptr, 0, hkpMoppCodeEnums, 1,
	hkpMoppCodeClass_Members, 3, &hkpMoppCode_Default, nullptr, 0, 0);
hkTypeInfo hkpMoppCodeTypeInfo = {"hkpMoppCode", finishLoadedObjecthkpMoppCode, cleanupLoadedObjecthkpMoppCode,
	getVtablehkpMoppCode(), &typeid(hkpMoppCode), sizeof(hkpMoppCode)};

const hkClass& hkpMoppCodeReindexedTerminal::staticClass() {
	return hkpMoppCodeReindexedTerminalClass;
}

const hkClass& hkpMoppCode::CodeInfo::staticClass() {
	return hkpMoppCodeCodeInfoClass;
}
