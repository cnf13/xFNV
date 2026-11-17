#include "Havok/Source/Common/SceneData/Camera/hkxCamera.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"

static void finishLoadedObjecthkxCamera(void* p, int32_t finishing);
// Havok SDK signature (hkSceneData.lib, tied by reloc,target).
static const void* getVtablehkxCamera();
static void cleanupLoadedObjecthkxCamera(void* p);

// The class record's member table.
using hkxCameraClassMemberArray = const hkClassMember[7];
static hkxCameraClassMemberArray hkxCameraClass_Members{
	{"from", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, 0, 0x10, nullptr},
	{"focus", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, 0, 0x20, nullptr},
	{"up", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, 0, 0x30, nullptr},
	{"fov", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, 0, 0x40, nullptr},
	{"far", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, 0, 0x44, nullptr},
	{"near", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, 0, 0x48, nullptr},
	{"leftHanded", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, 0, 0x4C, nullptr}};
hkClass hkxCameraClass("hkxCamera", &hkReferencedObjectClass, 80, nullptr, 0, nullptr, 0, hkxCameraClass_Members, 7,
	nullptr, nullptr, 0, 1);
hkTypeInfo hkxCameraTypeInfo = {"hkxCamera", finishLoadedObjecthkxCamera, cleanupLoadedObjecthkxCamera,
	getVtablehkxCamera(), &typeid(hkxCamera), sizeof(hkxCamera)};

// The type info's finish-loaded-object callback for hkxCamera: its finish constructor, when the object is not null.
static void finishLoadedObjecthkxCamera(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag flag;
		flag.m_finishing = finishing;
		::new (p) hkxCamera(flag);
	}
}

static const void* getVtablehkxCamera() {
	return hkGetVtableOf<hkxCamera>();
}

static void cleanupLoadedObjecthkxCamera(void* p) {
	static_cast<hkxCamera*>(p)->~hkxCamera();
}

// Nothing of its own to free.
hkxCamera::~hkxCamera() {}

const hkClass& hkxCamera::staticClass() {
	return hkxCameraClass;
}
