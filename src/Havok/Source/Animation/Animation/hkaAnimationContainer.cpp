#include "Havok/Source/Animation/Animation/hkaAnimationContainer.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Rig/hkaBoneAttachment.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnimation.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnimationBinding.h"
#include "Havok/Source/Animation/Animation/Deform/Skinning/hkaMeshBinding.h"
#include "Havok/Source/Animation/Animation/Rig/hkaSkeleton.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"

static void cleanupLoadedObjecthkaAnimationContainer(void* p);
static void finishLoadedObjecthkaAnimationContainer(void* p, int32_t finishing);
// Havok SDK signature (hkaAnimation.lib, tied by reloc,target).
static const void* getVtablehkaAnimationContainer();

// The class record's member table.
using hkaAnimationContainerClassMemberArray = const hkClassMember[5];
static hkaAnimationContainerClassMemberArray hkaAnimationContainerClass_Members{
	{"skeletons", &hkaSkeletonClass, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_POINTER}, 0, 0, 0x8,
		nullptr},
	{"animations", &hkaAnimationClass, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_POINTER}, 0, 0, 0x14,
		nullptr},
	{"bindings", &hkaAnimationBindingClass, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_POINTER}, 0, 0,
		0x20, nullptr},
	{"attachments", &hkaBoneAttachmentClass, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_POINTER}, 0, 0,
		0x2C, nullptr},
	{"skins", &hkaMeshBindingClass, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_POINTER}, 0, 0, 0x38,
		nullptr}};
hkClass hkaAnimationContainerClass("hkaAnimationContainer", &hkReferencedObjectClass, 68, nullptr, 0, nullptr, 0,
	hkaAnimationContainerClass_Members, 5, nullptr, nullptr, 0, 1);
hkTypeInfo hkaAnimationContainerTypeInfo = {"hkaAnimationContainer", finishLoadedObjecthkaAnimationContainer,
	cleanupLoadedObjecthkaAnimationContainer, getVtablehkaAnimationContainer(), &typeid(hkaAnimationContainer),
	sizeof(hkaAnimationContainer)};

static void cleanupLoadedObjecthkaAnimationContainer(void* p) {
	static_cast<hkaAnimationContainer*>(p)->~hkaAnimationContainer();
}

// The type info's finish-loaded-object callback for hkaAnimationContainer: the null check, then its finish
// constructor, which the exe inlines.
static void finishLoadedObjecthkaAnimationContainer(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag kFlag;
		kFlag.m_finishing = finishing;
		::new (p) hkaAnimationContainer(kFlag);
	}
}

static const void* getVtablehkaAnimationContainer() {
	return hkGetVtableOf<hkaAnimationContainer>();
}

// The arrays release their references and free their storage.
hkaAnimationContainer::~hkaAnimationContainer() {}

const hkClass& hkaAnimationContainer::staticClass() {
	return hkaAnimationContainerClass;
}
