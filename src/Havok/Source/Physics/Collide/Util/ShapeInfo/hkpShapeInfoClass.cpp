#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/Modifiers/hkpRemoveTerminalsMoppModifierClass.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Util/ShapeInfo/hkpShapeInfo.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The reflected members of hkpShapeInfo,
// as the exe's .rdata holds them.
using hkpShapeInfoClass_MembersArray = const hkClassMember[6];
static hkpShapeInfoClass_MembersArray hkpShapeInfoClass_Members{{"shape", &hkpShapeClass, nullptr,
																	{hkClassMember::TYPE_POINTER},
																	{hkClassMember::TYPE_STRUCT}, 0, {0}, 0x8, nullptr},
	{"isHierarchicalCompound", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xC,
		nullptr},
	{"hkdShapesCollected", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xD,
		nullptr},
	{"childShapeNames", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_STRINGPTR}, 0, {0}, 0x10,
		nullptr},
	{"childTransforms", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_TRANSFORM}, 0, {0}, 0x1C,
		nullptr},
	{"transform", nullptr, nullptr, {hkClassMember::TYPE_TRANSFORM}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x30,
		nullptr}};

hkClass hkpShapeInfoClass("hkpShapeInfo", &hkReferencedObjectClass, 112, nullptr, 0, nullptr, 0,
	hkpShapeInfoClass_Members, 6, nullptr, nullptr, 0, 0);
hkTypeInfo hkpShapeInfoTypeInfo = {"hkpShapeInfo", finishLoadedObjecthkpShapeInfo, cleanupLoadedObjecthkpShapeInfo,
	getVtablehkpShapeInfo(), &typeid(hkpShapeInfo), sizeof(hkpShapeInfo)};

// KEEP: getVtablehkpShapeInfo (0xE00820, cdecl) is not written. It builds an hkpShapeInfo in place with the
// inlined finish-loading constructor and returns its vtable, the exe's 0x10E8824; the compiler drops the dead
// stores, leaving `mov eax, 0x10E8824`. src cannot construct hkpShapeInfo (an inlined constructor that stores the
// exe's vtable).
void cleanupLoadedObjecthkpShapeInfo(void* p) {
	static_cast<hkpShapeInfo*>(p)->~hkpShapeInfo();
}

const void* getVtablehkpShapeInfo() {
	return hkGetVtableOf<hkpShapeInfo>();
}

// The arrays, the names and the shape's reference go with their members.
hkpShapeInfo::~hkpShapeInfo() {}

const hkClass& hkpShapeInfo::staticClass() {
	return hkpShapeInfoClass;
}
