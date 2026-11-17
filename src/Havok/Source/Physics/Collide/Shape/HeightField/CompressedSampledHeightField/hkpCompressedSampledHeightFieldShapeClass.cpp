#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/CompressedSampledHeightField/hkpCompressedSampledHeightFieldShape.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/SampledHeightField/hkpSampledHeightFieldShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpHavokSnapshot.h"

// Havok SDK signature (hkpCollide.lib, tied by reloc,target).
static const void* getVtablehkpCompressedSampledHeightFieldShape();
static void cleanupLoadedObjecthkpCompressedSampledHeightFieldShape(void* p);
static void finishLoadedObjecthkpCompressedSampledHeightFieldShape(void* p, int32_t finishing);

// The reflected members of hkpCompressedSampledHeightFieldShape.
using hkpCompressedSampledHeightFieldShapeClass_MembersArray = const hkClassMember[4];
static hkpCompressedSampledHeightFieldShapeClass_MembersArray hkpCompressedSampledHeightFieldShapeClass_Members{
	{"storage", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT16}, 0, {0}, 0x60, nullptr},
	{"triangleFlip", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x6C, nullptr},
	{"offset", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x70, nullptr},
	{"scale", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x74, nullptr}};
hkClass hkpCompressedSampledHeightFieldShapeClass("hkpCompressedSampledHeightFieldShape",
	&hkpSampledHeightFieldShapeClass, 0x80, nullptr, 0, nullptr, 0, hkpCompressedSampledHeightFieldShapeClass_Members,
	4, nullptr, nullptr, 0, 0);

static const void* getVtablehkpCompressedSampledHeightFieldShape() {
	return hkGetVtableOf<hkpCompressedSampledHeightFieldShape>();
}

static void cleanupLoadedObjecthkpCompressedSampledHeightFieldShape(void* p) {
	static_cast<hkpCompressedSampledHeightFieldShape*>(p)->~hkpCompressedSampledHeightFieldShape();
}

hkTypeInfo hkpCompressedSampledHeightFieldShapeTypeInfo = {"hkpCompressedSampledHeightFieldShape",
	finishLoadedObjecthkpCompressedSampledHeightFieldShape, cleanupLoadedObjecthkpCompressedSampledHeightFieldShape,
	getVtablehkpCompressedSampledHeightFieldShape(), &typeid(hkpCompressedSampledHeightFieldShape),
	sizeof(hkpCompressedSampledHeightFieldShape)};

// The type info's finish-loaded-object callback: the class's finish constructor placed on p.
static void finishLoadedObjecthkpCompressedSampledHeightFieldShape(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag aFlag;
		aFlag.m_finishing = finishing;
		::new (p) hkpCompressedSampledHeightFieldShape(aFlag);
	}
}
