#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Cylinder/hkpCylinderShape.h"

static void cleanupLoadedObjecthkpCylinderShape(void* p);
// This file's callbacks, defined below; the type info record names them.
void finishLoadedObjecthkpCylinderShape(void* p, int32_t finishing);
static const void* getVtablehkpCylinderShape();

// hkpCylinderShape's reflection records: VertexIdEncoding's values, the enum, the members and the defaults.
using hkpCylinderShapeVertexIdEncodingEnumItemsArray = const hkClassEnum::Item[5];
static hkpCylinderShapeVertexIdEncodingEnumItemsArray hkpCylinderShapeVertexIdEncodingEnumItems{
	{7, "VERTEX_ID_ENCODING_IS_BASE_A_SHIFT"}, {6, "VERTEX_ID_ENCODING_SIN_SIGN_SHIFT"},
	{5, "VERTEX_ID_ENCODING_COS_SIGN_SHIFT"}, {4, "VERTEX_ID_ENCODING_IS_SIN_LESSER_SHIFT"},
	{15, "VERTEX_ID_ENCODING_VALUE_MASK"}};
using hkpCylinderShapeEnumsArray = const hkClassEnum[1];
static hkpCylinderShapeEnumsArray hkpCylinderShapeEnums{
	{"VertexIdEncoding", hkpCylinderShapeVertexIdEncodingEnumItems, 5, nullptr, {0}}};
using hkpCylinderShapeClass_MembersArray = const hkClassMember[6];
static hkpCylinderShapeClass_MembersArray hkpCylinderShapeClass_Members{
	{"cylRadius", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x14, nullptr},
	{"cylBaseRadiusFactorForHeightFieldCollisions", nullptr, nullptr, {hkClassMember::TYPE_REAL},
		{hkClassMember::TYPE_VOID}, 0, {0}, 0x18, nullptr},
	{"vertexA", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x20, nullptr},
	{"vertexB", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x30, nullptr},
	{"perpendicular1", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x40,
		nullptr},
	{"perpendicular2", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x50,
		nullptr}};

static void cleanupLoadedObjecthkpCylinderShape(void* p) {
	static_cast<hkpCylinderShape*>(p)->~hkpCylinderShape();
}

namespace {
// The SDK's default struct: each member's offset into it (-1: no default), then the one default,
// cylBaseRadiusFactorForHeightFieldCollisions.
struct hkpCylinderShape_DefaultStruct {
	int32_t s_defaultOffsets[6];
	float m_cylBaseRadiusFactorForHeightFieldCollisions;
};
} // namespace
static const hkpCylinderShape_DefaultStruct hkpCylinderShape_Default{{-1, 0x18, -1, -1, -1, -1}, 0.8f};

hkClass hkpCylinderShapeClass("hkpCylinderShape", &hkpConvexShapeClass, 0x60, nullptr, 0, hkpCylinderShapeEnums, 1,
	hkpCylinderShapeClass_Members, 6, &hkpCylinderShape_Default, nullptr, 0, 0);
hkTypeInfo hkpCylinderShapeTypeInfo = {"hkpCylinderShape", finishLoadedObjecthkpCylinderShape,
	cleanupLoadedObjecthkpCylinderShape, getVtablehkpCylinderShape(), &typeid(hkpCylinderShape),
	sizeof(hkpCylinderShape)};

// The class registry's finish-loaded-object callback of hkpCylinderShape: the null check, then the class's finish
// constructor.
void finishLoadedObjecthkpCylinderShape(void* p, int32_t finishing) {
	void* apMemory = static_cast<void*>(p);
	hkFinishLoadedObjectFlag aFlag;
	aFlag.m_finishing = finishing;
	if (!apMemory) {
		return;
	}
	::new (apMemory) hkpCylinderShape(aFlag);
}

// Runs hkpCylinderShape's finish constructor (flag 0) on a 16-aligned stack object and returns the object's first word,
// the vptr that constructor stores last.
static const void* getVtablehkpCylinderShape() {
	return hkGetVtableOf<hkpCylinderShape>();
}
