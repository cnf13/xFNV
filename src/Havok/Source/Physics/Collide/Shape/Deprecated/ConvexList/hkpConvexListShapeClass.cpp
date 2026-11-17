#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/ConvexList/hkpConvexListShape.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"

// Havok SDK signature (hkpCollide.lib, tied by reloc,target).
static const void* getVtablehkpConvexListShape();
static void cleanupLoadedObjecthkpConvexListShape(void* p);
// This file's finish callback, defined below; the type info record names it.
static void finishLoadedObjecthkpConvexListShape(void* p, int32_t finishing);

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). hkpConvexListShape's reflected members,
// as the exe's .rdata holds them.
using hkpConvexListShapeClass_MembersArray = const hkClassMember[5];
static hkpConvexListShapeClass_MembersArray hkpConvexListShapeClass_Members{
	{"minDistanceToUseConvexHullForGetClosestPoints", nullptr, nullptr, {hkClassMember::TYPE_REAL},
		{hkClassMember::TYPE_VOID}, 0, {0}, 0x18, nullptr},
	{"aabbHalfExtents", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x20,
		nullptr},
	{"aabbCenter", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x30, nullptr},
	{"useCachedAabb", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x40, nullptr},
	{"childShapes", &hkpConvexShapeClass, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_POINTER}, 0, {0},
		0x44, nullptr}};

// The exe's record says one implemented interface with a null interface table.
hkClass hkpConvexListShapeClass("hkpConvexListShape", &hkpConvexShapeClass, 0x50, nullptr, 1, nullptr, 0,
	hkpConvexListShapeClass_Members, 5, nullptr, nullptr, 0, 0);

static const void* getVtablehkpConvexListShape() {
	return hkGetVtableOf<hkpConvexListShape>();
}

static void cleanupLoadedObjecthkpConvexListShape(void* p) {
	static_cast<hkpConvexListShape*>(p)->~hkpConvexListShape();
}

hkTypeInfo hkpConvexListShapeTypeInfo = {"hkpConvexListShape", finishLoadedObjecthkpConvexListShape,
	cleanupLoadedObjecthkpConvexListShape, getVtablehkpConvexListShape(), &typeid(hkpConvexListShape),
	sizeof(hkpConvexListShape)};

// The SDK's finish constructor placed on p (inlined in the exe).
static void finishLoadedObjecthkpConvexListShape(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag flag;
		flag.m_finishing = finishing;
		::new (p) hkpConvexListShape(flag);
	}
}
