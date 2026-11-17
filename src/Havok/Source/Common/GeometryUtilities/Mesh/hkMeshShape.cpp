#include "Havok/Source/Common/GeometryUtilities/Mesh/hkMeshShape.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/hkMeshVertexBuffer.h"

static void cleanupLoadedObjecthkMeshSectionCinfo(void* p);
static void cleanupLoadedObjecthkMeshSection(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkMeshSectionTypeInfo = {"hkMeshSection", nullptr, cleanupLoadedObjecthkMeshSection, nullptr,
	&typeid(hkMeshSection), sizeof(hkMeshSection)};
hkTypeInfo hkMeshSectionCinfoTypeInfo = {"hkMeshSectionCinfo", nullptr, cleanupLoadedObjecthkMeshSectionCinfo, nullptr,
	&typeid(hkMeshSectionCinfo), sizeof(hkMeshSectionCinfo)};

// The enum and member tables of hkMeshShapeClass.cpp's class records, in the SDK's file-local form. Not named in the
// PDB or IDA (the PDB names only hkMeshSectionClass_Members and hkMeshSectionCinfoClass_Members); names chosen here,
// after the SDK's.
using hkMeshSectionMeshSectionIndexTypeEnumItemTable = const hkClassEnum::Item[3];
static hkMeshSectionMeshSectionIndexTypeEnumItemTable hkMeshSectionMeshSectionIndexTypeEnumItems{{0, "INDEX_TYPE_NONE"},
	{1, "INDEX_TYPE_UINT16"}, {2, "INDEX_TYPE_UINT32"}};
using hkMeshSectionPrimitiveTypeEnumItemTable = const hkClassEnum::Item[5];
static hkMeshSectionPrimitiveTypeEnumItemTable hkMeshSectionPrimitiveTypeEnumItems{{0, "PRIMITIVE_TYPE_UNKNOWN"},
	{1, "PRIMITIVE_TYPE_POINT_LIST"}, {2, "PRIMITIVE_TYPE_LINE_LIST"}, {3, "PRIMITIVE_TYPE_TRIANGLE_LIST"},
	{4, "PRIMITIVE_TYPE_TRIANGLE_STRIP"}};
using hkMeshSectionEnumTable = const hkClassEnum[2];
static hkMeshSectionEnumTable hkMeshSectionEnums{
	{"MeshSectionIndexType", hkMeshSectionMeshSectionIndexTypeEnumItems, 3, nullptr, 0},
	{"PrimitiveType", hkMeshSectionPrimitiveTypeEnumItems, 5, nullptr, 0}};
using hkMeshShapeAccessFlagsEnumItemTable = const hkClassEnum::Item[2];
static hkMeshShapeAccessFlagsEnumItemTable hkMeshShapeAccessFlagsEnumItems{{1, "ACCESS_INDICES"},
	{2, "ACCESS_VERTEX_BUFFER"}};
using hkMeshShapeEnumTable = const hkClassEnum[1];
static hkMeshShapeEnumTable hkMeshShapeEnums{{"AccessFlags", hkMeshShapeAccessFlagsEnumItems, 2, nullptr, 0}};

// The PDB's names. The enum records of hkMeshSection and hkMeshShape, by pointer (.data).
const hkClassEnum* hkMeshSectionMeshSectionIndexTypeEnum = &hkMeshSectionEnums[0];
const hkClassEnum* hkMeshSectionPrimitiveTypeEnum = &hkMeshSectionEnums[1];
const hkClassEnum* hkMeshShapeAccessFlagsEnum = &hkMeshShapeEnums[0];

// The two enum members name the enum records the pointers above hold; the exe's image holds null there and its
// initialisers below store the pointers at start-up.
using hkMeshSectionClassMemberTable = hkClassMember[10];
static hkMeshSectionClassMemberTable hkMeshSectionClass_Members{{"primitiveType", nullptr, &hkMeshSectionEnums[1],
																	{hkClassMember::TYPE_ENUM},
																	{hkClassMember::TYPE_UINT8}, 0, 0, 0x0, nullptr},
	{"numPrimitives", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x4, nullptr},
	{"numIndices", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x8, nullptr},
	{"vertexStartIndex", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0xC, nullptr},
	{"transformIndex", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x10, nullptr},
	{"indexType", nullptr, &hkMeshSectionEnums[0], {hkClassMember::TYPE_ENUM}, {hkClassMember::TYPE_UINT8}, 0, 0, 0x14,
		nullptr},
	{"indices", nullptr, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_VOID}, 0,
		hkClassMember::SERIALIZE_IGNORED, 0x18, nullptr},
	{"vertexBuffer", &hkMeshVertexBufferClass, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_STRUCT}, 0,
		0, 0x1C, nullptr},
	{"material", &hkMeshMaterialClass, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_STRUCT}, 0, 0, 0x20,
		nullptr},
	{"sectionIndex", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x24, nullptr}};
using hkMeshSectionCinfoClassMemberTable = hkClassMember[8];
static hkMeshSectionCinfoClassMemberTable hkMeshSectionCinfoClass_Members{
	{"vertexBuffer", &hkMeshVertexBufferClass, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_STRUCT}, 0,
		0, 0x0, nullptr},
	{"material", &hkMeshMaterialClass, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_STRUCT}, 0, 0, 0x4,
		nullptr},
	{"primitiveType", nullptr, &hkMeshSectionEnums[1], {hkClassMember::TYPE_ENUM}, {hkClassMember::TYPE_UINT8}, 0, 0,
		0x8, nullptr},
	{"numPrimitives", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0xC, nullptr},
	{"indexType", nullptr, &hkMeshSectionEnums[0], {hkClassMember::TYPE_ENUM}, {hkClassMember::TYPE_UINT8}, 0, 0, 0x10,
		nullptr},
	{"indices", nullptr, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_VOID}, 0,
		hkClassMember::SERIALIZE_IGNORED, 0x14, nullptr},
	{"vertexStartIndex", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x18,
		nullptr},
	{"transformIndex", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x1C, nullptr}};
hkClass hkMeshMaterialClass("hkMeshMaterial", &hkReferencedObjectClass, 8, nullptr, 0, nullptr, 0, nullptr, 0, nullptr,
	nullptr, 0, 0);
hkClass hkMeshSectionClass("hkMeshSection", nullptr, 40, nullptr, 0, hkMeshSectionEnums, 2, hkMeshSectionClass_Members,
	10, nullptr, nullptr, 0, 1);
hkClass hkMeshSectionCinfoClass("hkMeshSectionCinfo", nullptr, 32, nullptr, 0, nullptr, 0,
	hkMeshSectionCinfoClass_Members, 8, nullptr, nullptr, 0, 1);
hkClass hkMeshShapeClass("hkMeshShape", &hkReferencedObjectClass, 8, nullptr, 0, hkMeshShapeEnums, 1, nullptr, 0,
	nullptr, nullptr, 0, 0);

static void cleanupLoadedObjecthkMeshSection(void* p) {
	;
}

static void cleanupLoadedObjecthkMeshSectionCinfo(void* p) {
	;
}

const hkClass& hkMeshSection::staticClass() {
	return hkMeshSectionClass;
}

const hkClass& hkMeshSectionCinfo::staticClass() {
	return hkMeshSectionCinfoClass;
}
