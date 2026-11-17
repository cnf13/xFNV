#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShapeClass.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h"
#include "Havok/Source/Physics/Collide/Util/Welding/hkpWeldingUtility.h"

static void finishLoadedObjecthkpExtendedMeshShapeTrianglesSubpart(void* p, int32_t finishing);
static void finishLoadedObjecthkpExtendedMeshShapeSubpart(void* p, int32_t finishing);
static void cleanupLoadedObjecthkpExtendedMeshShapeSubpart(void* p);
static void cleanupLoadedObjecthkpExtendedMeshShapeTrianglesSubpart(void* p);
static void cleanupLoadedObjecthkpExtendedMeshShapeShapesSubpart(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpExtendedMeshShapeSubpartTypeInfo = {"hkpExtendedMeshShapeSubpart",
	finishLoadedObjecthkpExtendedMeshShapeSubpart, cleanupLoadedObjecthkpExtendedMeshShapeSubpart, nullptr,
	&typeid(hkpExtendedMeshShape::Subpart), sizeof(hkpExtendedMeshShape::Subpart)};
hkTypeInfo hkpExtendedMeshShapeTrianglesSubpartTypeInfo = {"hkpExtendedMeshShapeTrianglesSubpart",
	finishLoadedObjecthkpExtendedMeshShapeTrianglesSubpart, cleanupLoadedObjecthkpExtendedMeshShapeTrianglesSubpart,
	nullptr, &typeid(hkpExtendedMeshShape::TrianglesSubpart), sizeof(hkpExtendedMeshShape::TrianglesSubpart)};
hkTypeInfo hkpExtendedMeshShapeShapesSubpartTypeInfo = {"hkpExtendedMeshShapeShapesSubpart",
	finishLoadedObjecthkpExtendedMeshShapeShapesSubpart, cleanupLoadedObjecthkpExtendedMeshShapeShapesSubpart, nullptr,
	&typeid(hkpExtendedMeshShape::ShapesSubpart), sizeof(hkpExtendedMeshShape::ShapesSubpart)};

// The enums' items and the enums.
using hkpExtendedMeshShapeIndexStridingTypeEnumItemsArray = const hkClassEnum::Item[5];
static hkpExtendedMeshShapeIndexStridingTypeEnumItemsArray hkpExtendedMeshShapeIndexStridingTypeEnumItems{
	{hkpExtendedMeshShape::INDICES_INVALID, "INDICES_INVALID"}, {hkpExtendedMeshShape::INDICES_INT8, "INDICES_INT8"},
	{hkpExtendedMeshShape::INDICES_INT16, "INDICES_INT16"}, {hkpExtendedMeshShape::INDICES_INT32, "INDICES_INT32"},
	{hkpExtendedMeshShape::INDICES_MAX_ID, "INDICES_MAX_ID"}};
using hkpExtendedMeshShapeMaterialIndexStridingTypeEnumItemsArray = const hkClassEnum::Item[4];
static hkpExtendedMeshShapeMaterialIndexStridingTypeEnumItemsArray
	hkpExtendedMeshShapeMaterialIndexStridingTypeEnumItems{
		{hkpExtendedMeshShape::MATERIAL_INDICES_INVALID, "MATERIAL_INDICES_INVALID"},
		{hkpExtendedMeshShape::MATERIAL_INDICES_INT8, "MATERIAL_INDICES_INT8"},
		{hkpExtendedMeshShape::MATERIAL_INDICES_INT16, "MATERIAL_INDICES_INT16"},
		{hkpExtendedMeshShape::MATERIAL_INDICES_MAX_ID, "MATERIAL_INDICES_MAX_ID"}};
using hkpExtendedMeshShapeSubpartTypeEnumItemsArray = const hkClassEnum::Item[2];
static hkpExtendedMeshShapeSubpartTypeEnumItemsArray hkpExtendedMeshShapeSubpartTypeEnumItems{
	{hkpExtendedMeshShape::SUBPART_TRIANGLES, "SUBPART_TRIANGLES"},
	{hkpExtendedMeshShape::SUBPART_SHAPE, "SUBPART_SHAPE"}};
using hkpExtendedMeshShapeEnumsArray = const hkClassEnum[3];
static hkpExtendedMeshShapeEnumsArray hkpExtendedMeshShapeEnums{
	{"IndexStridingType", hkpExtendedMeshShapeIndexStridingTypeEnumItems, 5, nullptr, 0},
	{"MaterialIndexStridingType", hkpExtendedMeshShapeMaterialIndexStridingTypeEnumItems, 4, nullptr, 0},
	{"SubpartType", hkpExtendedMeshShapeSubpartTypeEnumItems, 2, nullptr, 0}};

// The PDB's names: the enums, by pointer (.data).
static const hkClassEnum* hkpExtendedMeshShapeIndexStridingTypeEnum = &hkpExtendedMeshShapeEnums[0];
static const hkClassEnum* hkpExtendedMeshShapeMaterialIndexStridingTypeEnum = &hkpExtendedMeshShapeEnums[1];
static const hkClassEnum* hkpExtendedMeshShapeSubpartTypeEnum = &hkpExtendedMeshShapeEnums[2];

// The PDB's name. The subpart record's member table (.data); its two enum members' enums are stored at start-up
// (0xFBEF60, below).
using hkpExtendedMeshShape_SubpartClass_MembersArray = hkClassMember[8];
static hkpExtendedMeshShape_SubpartClass_MembersArray hkpExtendedMeshShape_SubpartClass_Members{
	{"type", nullptr, &hkpExtendedMeshShapeEnums[2], {hkClassMember::TYPE_ENUM}, {hkClassMember::TYPE_INT8}, 0, 0, 0x0,
		nullptr},
	{"materialIndexStridingType", nullptr, &hkpExtendedMeshShapeEnums[1], {hkClassMember::TYPE_ENUM},
		{hkClassMember::TYPE_INT8}, 0, 0, 0x1, nullptr},
	{"materialStriding", nullptr, nullptr, {hkClassMember::TYPE_INT16}, {hkClassMember::TYPE_VOID}, 0, 0, 0x2, nullptr},
	{"materialIndexBase", nullptr, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_VOID}, 0,
		{hkClassMember::SERIALIZE_IGNORED}, 0x4, nullptr},
	{"materialIndexStriding", nullptr, nullptr, {hkClassMember::TYPE_UINT16}, {hkClassMember::TYPE_VOID}, 0, 0, 0x8,
		nullptr},
	{"numMaterials", nullptr, nullptr, {hkClassMember::TYPE_UINT16}, {hkClassMember::TYPE_VOID}, 0, 0, 0xA, nullptr},
	{"materialBase", nullptr, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_VOID}, 0,
		{hkClassMember::SERIALIZE_IGNORED}, 0xC, nullptr},
	{"materialClass", nullptr, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_VOID}, 0,
		{hkClassMember::SERIALIZE_IGNORED}, 0x10, nullptr}};

// The PDB's name. The triangles subpart record's member table (.data); its stridingType member's enum is stored at
// start-up.
using hkpExtendedMeshShape_TrianglesSubpartClass_MembersArray = hkClassMember[11];
static hkpExtendedMeshShape_TrianglesSubpartClass_MembersArray hkpExtendedMeshShape_TrianglesSubpartClass_Members{
	{"numTriangleShapes", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x14,
		nullptr},
	{"vertexBase", nullptr, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_VOID}, 0,
		{hkClassMember::SERIALIZE_IGNORED}, 0x18, nullptr},
	{"vertexStriding", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x1C, nullptr},
	{"numVertices", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x20, nullptr},
	{"extrusion", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, 0, 0x30, nullptr},
	{"indexBase", nullptr, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_VOID}, 0,
		{hkClassMember::SERIALIZE_IGNORED}, 0x40, nullptr},
	{"indexStriding", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x44, nullptr},
	{"stridingType", nullptr, &hkpExtendedMeshShapeEnums[0], {hkClassMember::TYPE_ENUM}, {hkClassMember::TYPE_INT8}, 0,
		0, 0x48, nullptr},
	{"flipAlternateTriangles", nullptr, nullptr, {hkClassMember::TYPE_INT8}, {hkClassMember::TYPE_VOID}, 0, 0, 0x49,
		nullptr},
	{"triangleOffset", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x4C, nullptr},
	{"userData", nullptr, nullptr, {hkClassMember::TYPE_ULONG}, {hkClassMember::TYPE_VOID}, 0, 0, 0x50, nullptr}};
hkpExtendedMeshShape::ShapesSubpart::MembersArray hkpExtendedMeshShape::ShapesSubpart::Members{
	{"childShapes", &hkpConvexShapeClass, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_POINTER}, 0, 0,
		0x14, nullptr},
	{"rotation", nullptr, nullptr, {hkClassMember::TYPE_QUATERNION}, {hkClassMember::TYPE_VOID}, 0, 0, 0x20, nullptr},
	{"translation", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, 0, 0x30, nullptr}};
hkpExtendedMeshShape::MembersArray hkpExtendedMeshShape::Members{
	{"embeddedTrianglesSubpart", &hkpExtendedMeshShapeTrianglesSubpartClass, nullptr, {hkClassMember::TYPE_STRUCT},
		{hkClassMember::TYPE_VOID}, 0, 0, 0x20, nullptr},
	{"scaling", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, 0, 0x80, nullptr},
	{"aabbHalfExtents", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, 0, 0x90,
		nullptr},
	{"aabbCenter", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, 0, 0xA0, nullptr},
	{"numBitsForSubpartIndex", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0xB0,
		nullptr},
	{"trianglesSubparts", &hkpExtendedMeshShapeTrianglesSubpartClass, nullptr, {hkClassMember::TYPE_ARRAY},
		{hkClassMember::TYPE_STRUCT}, 0, 0, 0xB4, nullptr},
	{"shapesSubparts", &hkpExtendedMeshShapeShapesSubpartClass, nullptr, {hkClassMember::TYPE_ARRAY},
		{hkClassMember::TYPE_STRUCT}, 0, 0, 0xC0, nullptr},
	{"weldingInfo", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT16}, 0, 0, 0xCC, nullptr},
	{"weldingType", nullptr, &hkpWeldingUtilityEnums[0], {hkClassMember::TYPE_ENUM}, {hkClassMember::TYPE_UINT8}, 0, 0,
		0xD8, nullptr},
	{"defaultCollisionFilterInfo", nullptr, nullptr, {hkClassMember::TYPE_UINT32}, {hkClassMember::TYPE_VOID}, 0, 0,
		0xDC, nullptr},
	{"cachedNumChildShapes", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0xE0,
		nullptr},
	{"triangleRadius", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, 0, 0xE4, nullptr},
	{"padding", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 2,
		{hkClassMember::SERIALIZE_IGNORED}, 0xE8, nullptr}};
hkClass hkpExtendedMeshShapeSubpartClass("hkpExtendedMeshShapeSubpart", nullptr, 20, nullptr, 0, nullptr, 0,
	hkpExtendedMeshShape_SubpartClass_Members, 8, nullptr, nullptr, 0, 0);

static void finishLoadedObjecthkpExtendedMeshShapeTrianglesSubpart(void* p, int32_t finishing) {
	;
}

static void finishLoadedObjecthkpExtendedMeshShapeSubpart(void* p, int32_t finishing) {
	;
}

static void cleanupLoadedObjecthkpExtendedMeshShapeSubpart(void* p) {
	;
}

static void cleanupLoadedObjecthkpExtendedMeshShapeTrianglesSubpart(void* p) {
	;
}

// Destroys one ShapesSubpart: a tail jump to its destructor, which leaves no result a caller reads.
static void cleanupLoadedObjecthkpExtendedMeshShapeShapesSubpart(void* p) {
	static_cast<hkpExtendedMeshShape::ShapesSubpart*>(p)->~ShapesSubpart();
}

// The class records of hkpExtendedMeshShape and its subparts, as the SDK's generated file has them. A member table
// that names an enum is in .data: the exe's `dynamic initializer for '<table>'' stores the enum's pointer into the
// member, which the image holds null; each data line holds the enum itself. Names of tables the PDB does not name
// are the Havok SDK's.

namespace {
// The PDB's `anonymous-namespace'::hkpExtendedMeshShape_DefaultStruct: each member's offset in this struct (-1: no
// default), then the defaults of weldingType and cachedNumChildShapes.
struct hkpExtendedMeshShape_DefaultStruct {
	int32_t s_defaultOffsets[13];
	hkEnum<hkpWeldingUtility::WeldingType, uint8_t> m_weldingType;
	int32_t m_cachedNumChildShapes;
};
// The PDB's `anonymous-namespace'::hkpExtendedMeshShapeTrianglesSubpart_DefaultStruct: as above, the default of
// triangleOffset.
struct hkpExtendedMeshShapeTrianglesSubpart_DefaultStruct {
	int32_t s_defaultOffsets[11];
	int32_t m_triangleOffset;
};
} // namespace
// The defaults the class records name. Not named in the PDB or IDA; names chosen here (the Havok SDK's). The
// shape's are in .data, the triangles subpart's in .rdata.
static hkpExtendedMeshShape_DefaultStruct hkpExtendedMeshShape_Default{
	{-1, -1, -1, -1, -1, -1, -1, -1, 0x34, -1, 0x38, -1, -1}, {hkpWeldingUtility::WELDING_TYPE_NONE}, -1};
static const hkpExtendedMeshShapeTrianglesSubpart_DefaultStruct hkpExtendedMeshShapeTrianglesSubpart_Default{
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, 0x2C, -1}, -1};

hkClass hkpExtendedMeshShapeTrianglesSubpartClass("hkpExtendedMeshShapeTrianglesSubpart",
	&hkpExtendedMeshShapeSubpartClass, 96, nullptr, 0, nullptr, 0, hkpExtendedMeshShape_TrianglesSubpartClass_Members,
	11, &hkpExtendedMeshShapeTrianglesSubpart_Default, nullptr, 0, 2);
hkClass hkpExtendedMeshShapeShapesSubpartClass("hkpExtendedMeshShapeShapesSubpart", &hkpExtendedMeshShapeSubpartClass,
	64, nullptr, 0, nullptr, 0, hkpExtendedMeshShape::ShapesSubpart::Members, 3, nullptr, nullptr, 0, 1);
hkClass hkpExtendedMeshShapeClass("hkpExtendedMeshShape", &hkpShapeCollectionClass, 240, nullptr, 0,
	hkpExtendedMeshShapeEnums, 3, hkpExtendedMeshShape::Members, 13, &hkpExtendedMeshShape_Default, nullptr, 0, 2);
hkTypeInfo hkpExtendedMeshShapeTypeInfo = {"hkpExtendedMeshShape", finishLoadedObjecthkpExtendedMeshShape,
	cleanupLoadedObjecthkpExtendedMeshShape, getVtablehkpExtendedMeshShape(), &typeid(hkpExtendedMeshShape),
	sizeof(hkpExtendedMeshShape)};

// The SDK's getVtable: hkpExtendedMeshShape's finish constructor (flag 0), out of line, on a 16-aligned stack object;
// returns the object's first word, the vptr that constructor stores.
const void* getVtablehkpExtendedMeshShape() {
	return hkGetVtableOf<hkpExtendedMeshShape>();
}

// As above, for the shape itself (cdecl, 2 stack arguments, 0xD4B300).
void finishLoadedObjecthkpExtendedMeshShape(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag flag;
		flag.m_finishing = finishing;
		::new (p) hkpExtendedMeshShape(flag);
	}
}

void cleanupLoadedObjecthkpExtendedMeshShape(void* p) {
	static_cast<hkpExtendedMeshShape*>(p)->~hkpExtendedMeshShape();
}

// The type info's finish-loaded-object callback for a shapes subpart: the null check, then the subpart's out-of-line
// finish constructor (cdecl, 2 stack arguments, 0xD4B2D0).
void finishLoadedObjecthkpExtendedMeshShapeShapesSubpart(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag flag;
		flag.m_finishing = finishing;
		::new (p) hkpExtendedMeshShape::ShapesSubpart(flag);
	}
}

const hkClass& hkpExtendedMeshShape::Subpart::staticClass() {
	return hkpExtendedMeshShapeSubpartClass;
}

const hkClass& hkpExtendedMeshShape::TrianglesSubpart::staticClass() {
	return hkpExtendedMeshShapeTrianglesSubpartClass;
}

const hkClass& hkpExtendedMeshShape::ShapesSubpart::staticClass() {
	return hkpExtendedMeshShapeShapesSubpartClass;
}

const hkClass& hkpExtendedMeshShape::staticClass() {
	return hkpExtendedMeshShapeClass;
}
