#include "Havok/Source/Common/SceneData/Attributes/hkxAnimatedVector.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"

static void finishLoadedObjecthkxAnimatedVector(void* p, int32_t finishing);
// Havok SDK signature (hkSceneData.lib, tied by reloc,target).
static const void* getVtablehkxAnimatedVector();
static void cleanupLoadedObjecthkxAnimatedVector(void* p);

// The class record's member table. Its enum member reads the enum record through hkxAttributeHintEnum, which is stored
// at start-up.
using hkxAnimatedVectorClassMemberArray = hkClassMember[2];
static hkxAnimatedVectorClassMemberArray hkxAnimatedVectorClass_Members{
	{"vectors", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_VECTOR4}, 0, 0, 0x8, nullptr},
	{"hint", nullptr, hkxAttributeHintEnum, {hkClassMember::TYPE_ENUM}, {hkClassMember::TYPE_UINT8}, 0, 0, 0x14,
		nullptr}};
hkClass hkxAnimatedVectorClass("hkxAnimatedVector", &hkReferencedObjectClass, 24, nullptr, 0, nullptr, 0,
	hkxAnimatedVectorClass_Members, 2, nullptr, nullptr, 0, 1);
hkTypeInfo hkxAnimatedVectorTypeInfo = {"hkxAnimatedVector", finishLoadedObjecthkxAnimatedVector,
	cleanupLoadedObjecthkxAnimatedVector, getVtablehkxAnimatedVector(), &typeid(hkxAnimatedVector),
	sizeof(hkxAnimatedVector)};

// The type info's finish-loaded-object callback for hkxAnimatedVector: its finish constructor, when the object is
// not null.
static void finishLoadedObjecthkxAnimatedVector(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag flag;
		flag.m_finishing = finishing;
		::new (p) hkxAnimatedVector(flag);
	}
}

static const void* getVtablehkxAnimatedVector() {
	return hkGetVtableOf<hkxAnimatedVector>();
}

static void cleanupLoadedObjecthkxAnimatedVector(void* p) {
	static_cast<hkxAnimatedVector*>(p)->~hkxAnimatedVector();
}

// hkxAnimatedVector's reflection data and objects.

// The member frees the vectors.
hkxAnimatedVector::~hkxAnimatedVector() {}

const hkClass& hkxAnimatedVector::staticClass() {
	return hkxAnimatedVectorClass;
}
