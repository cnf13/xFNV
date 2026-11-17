#include "Havok/Source/Physics/Collide/Shape/Deprecated/ConvexPieceMesh/hkpConvexPieceMeshShapeClass.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/ConvexPieceMesh/hkpConvexPieceMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Internal/Collide/ConvexPieceMesh/hkpConvexPieceStreamData.h"

// This file's finish callback, defined below; the type info record names it.
void finishLoadedObjecthkpConvexPieceMeshShape(void* p, int32_t finishing);

// hkpConvexPieceMeshShape's reflected members.
using hkpConvexPieceMeshShapeClass_MembersArray = const hkClassMember[3];
static hkpConvexPieceMeshShapeClass_MembersArray hkpConvexPieceMeshShapeClass_Members{
	{"convexPieceStream", &hkpConvexPieceStreamDataClass, nullptr, {hkClassMember::TYPE_POINTER},
		{hkClassMember::TYPE_STRUCT}, 0, {0}, 0x18, nullptr},
	{"displayMesh", &hkpShapeCollectionClass, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_STRUCT}, 0,
		{0}, 0x1C, nullptr},
	{"radius", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x20, nullptr}};

hkClass hkpConvexPieceMeshShapeClass("hkpConvexPieceMeshShape", &hkpShapeCollectionClass, 0x24, nullptr, 0, nullptr, 0,
	hkpConvexPieceMeshShapeClass_Members, 3, nullptr, nullptr, 0, 0);
hkTypeInfo hkpConvexPieceMeshShapeTypeInfo = {"hkpConvexPieceMeshShape", finishLoadedObjecthkpConvexPieceMeshShape,
	cleanupLoadedObjecthkpConvexPieceMeshShape, getVtablehkpConvexPieceMeshShape(), &typeid(hkpConvexPieceMeshShape),
	sizeof(hkpConvexPieceMeshShape)};

// The class registry's finish-loaded-object callback of hkpConvexPieceMeshShape: the null check, then its finish
// constructor.
void finishLoadedObjecthkpConvexPieceMeshShape(void* p, int32_t finishing) {
	if (!p) {
		return;
	}
	hkFinishLoadedObjectFlag kFlag;
	kFlag.m_finishing = finishing;
	::new (p) hkpConvexPieceMeshShape(kFlag);
}

const void* getVtablehkpConvexPieceMeshShape() {
	return hkGetVtableOf<hkpConvexPieceMeshShape>();
}

void cleanupLoadedObjecthkpConvexPieceMeshShape(void* p) {
	static_cast<hkpConvexPieceMeshShape*>(p)->~hkpConvexPieceMeshShape();
}

const hkClass& hkpConvexPieceMeshShape::staticClass() {
	return hkpConvexPieceMeshShapeClass;
}
