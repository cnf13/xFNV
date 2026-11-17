#include "Havok/Source/Common/SceneData/Skin/hkxSkinBinding.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/Memory/hkMemoryMeshShape.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

static void cleanupLoadedObjecthkxSkinBinding(void* p);

hkxSkinBinding::MembersArray hkxSkinBinding::Members{
	{"mesh", &hkxMeshClass, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_STRUCT}, 0, {0}, 0x8, nullptr},
	{"nodeNames", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_STRINGPTR}, 0, {0}, 0xC, nullptr},
	{"bindPose", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_MATRIX4}, 0, {0}, 0x18, nullptr},
	{"initSkinTransform", nullptr, nullptr, {hkClassMember::TYPE_MATRIX4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x30,
		nullptr}};
hkClass hkxSkinBindingClass("hkxSkinBinding", &hkReferencedObjectClass, 0x70, nullptr, 0, nullptr, 0,
	hkxSkinBinding::Members, 4, nullptr, nullptr, 0, 2);
hkTypeInfo hkxSkinBindingTypeInfo{"hkxSkinBinding", finishLoadedObjecthkxSkinBinding, cleanupLoadedObjecthkxSkinBinding,
	getVtablehkxSkinBinding(), &typeid(hkxSkinBinding), sizeof(hkxSkinBinding)};

static void cleanupLoadedObjecthkxSkinBinding(void* p) {
	static_cast<hkxSkinBinding*>(p)->~hkxSkinBinding();
}

// The members' destructors free m_bindPose, destroy the node names and release m_mesh.
hkxSkinBinding::~hkxSkinBinding() {}

const hkClass& hkxSkinBinding::staticClass() {
	return hkxSkinBindingClass;
}
