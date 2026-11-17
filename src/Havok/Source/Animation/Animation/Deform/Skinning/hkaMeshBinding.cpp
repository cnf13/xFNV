#include "Havok/Source/Animation/Animation/Deform/Skinning/hkaMeshBinding.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"
#include "Havok/Source/Animation/Animation/Rig/hkaSkeleton.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/SceneData/Mesh/hkxMesh.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkaMeshBindingMappingTypeInfo = {"hkaMeshBindingMapping", finishLoadedObjecthkaMeshBindingMapping,
	cleanupLoadedObjecthkaMeshBindingMapping, nullptr, &typeid(hkaMeshBinding::Mapping),
	sizeof(hkaMeshBinding::Mapping)};

// The reflection's member tables (file-local data).
using hkaMeshBinding_MappingClass_MembersArray = const hkClassMember[1];
static hkaMeshBinding_MappingClass_MembersArray hkaMeshBinding_MappingClass_Members{
	{"mapping", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_INT16}, 0, {0}, 0x0, nullptr}};
using hkaMeshBindingClass_MembersArray = const hkClassMember[5];
static hkaMeshBindingClass_MembersArray hkaMeshBindingClass_Members{
	{"mesh", &hkxMeshClass, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_STRUCT}, 0, {0}, 0x8, nullptr},
	{"originalSkeletonName", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xC,
		nullptr},
	{"skeleton", &hkaSkeletonClass, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_STRUCT}, 0, {0}, 0x10,
		nullptr},
	{"mappings", &hkaMeshBindingMappingClass, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_STRUCT}, 0,
		{0}, 0x14, nullptr},
	{"boneFromSkinMeshTransforms", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_TRANSFORM}, 0,
		{0}, 0x20, nullptr}};

hkClass hkaMeshBindingMappingClass("hkaMeshBindingMapping", nullptr, 0xC, nullptr, 0, nullptr, 0,
	hkaMeshBinding_MappingClass_Members, 1, nullptr, nullptr, 0, 0);
hkClass hkaMeshBindingClass("hkaMeshBinding", &hkReferencedObjectClass, 0x2C, nullptr, 0, nullptr, 0,
	hkaMeshBindingClass_Members, 5, nullptr, nullptr, 0, 1);

hkTypeInfo hkaMeshBindingTypeInfo{"hkaMeshBinding", finishLoadedObjecthkaMeshBinding, cleanupLoadedObjecthkaMeshBinding,
	getVtablehkaMeshBinding(), &typeid(hkaMeshBinding), sizeof(hkaMeshBinding)};

const hkClass& hkaMeshBinding::Mapping::staticClass() {
	return hkaMeshBindingMappingClass;
}

const hkClass& hkaMeshBinding::Mapping::staticClass_00DFC760() {
	return hkpStiffSpringChainDataConstraintInfoClass;
}

hkaMeshBinding::hkaMeshBinding()
	: m_mesh(), m_originalSkeletonName(), m_skeleton(), m_mappings(), m_boneFromSkinMeshTransforms() {}

// The loaded object's members stay as loaded: each is built with its own finish constructor.
hkaMeshBinding::hkaMeshBinding(hkFinishLoadedObjectFlag aFlag)
	: hkReferencedObject(aFlag), m_mesh(aFlag), m_originalSkeletonName(aFlag), m_skeleton(aFlag), m_mappings(aFlag),
	  m_boneFromSkinMeshTransforms(aFlag) {}

// The transforms, the mappings, the skeleton's and the mesh's references and the skeleton name go with their
// members.
hkaMeshBinding::~hkaMeshBinding() {}

const hkClass& hkaMeshBinding::staticClass() {
	return hkaMeshBindingClass;
}

const hkClass& hkaMeshBinding::staticClass_00DFC7C0() {
	return hkpStiffSpringChainDataClass;
}
