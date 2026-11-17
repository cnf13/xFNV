#include "Havok/Source/Animation/Animation/Rig/hkaBoneAttachment.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkRefVariant.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Animation/Animation/Motion/Default/hkaDefaultAnimatedReferenceFrame.h"

static void finishLoadedObjecthkaBoneAttachment(void* p, int32_t finishing);
// Havok SDK signature (hkaAnimation.lib, tied by bytes,reloc).
static const void* getVtablehkaBoneAttachment();
static void cleanupLoadedObjecthkaBoneAttachment(void* p);

// The class record's member table.
using hkaBoneAttachmentClassMemberArray = const hkClassMember[5];
static hkaBoneAttachmentClassMemberArray hkaBoneAttachmentClass_Members{
	{"originalSkeletonName", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, 0, 0x8,
		nullptr},
	{"boneFromAttachment", nullptr, nullptr, {hkClassMember::TYPE_MATRIX4}, {hkClassMember::TYPE_VOID}, 0, 0, 0x10,
		nullptr},
	{"attachment", &hkReferencedObjectClass, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_STRUCT}, 0, 0,
		0x50, nullptr},
	{"name", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, 0, 0x54, nullptr},
	{"boneIndex", nullptr, nullptr, {hkClassMember::TYPE_INT16}, {hkClassMember::TYPE_VOID}, 0, 0, 0x58, nullptr}};
hkClass hkaBoneAttachmentClass("hkaBoneAttachment", &hkReferencedObjectClass, 96, nullptr, 0, nullptr, 0,
	hkaBoneAttachmentClass_Members, 5, nullptr, nullptr, 0, 1);
hkTypeInfo hkaBoneAttachmentTypeInfo = {"hkaBoneAttachment", finishLoadedObjecthkaBoneAttachment,
	cleanupLoadedObjecthkaBoneAttachment, getVtablehkaBoneAttachment(), &typeid(hkaBoneAttachment),
	sizeof(hkaBoneAttachment)};

// Havok SDK: finishes a loaded hkaBoneAttachment in place, when there is one: its finish constructor, which the exe
// inlines.
static void finishLoadedObjecthkaBoneAttachment(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag kFlag;
		kFlag.m_finishing = finishing;
		::new (p) hkaBoneAttachment(kFlag);
	}
}

static const void* getVtablehkaBoneAttachment() {
	return hkGetVtableOf<hkaBoneAttachment>();
}

static void cleanupLoadedObjecthkaBoneAttachment(void* p) {
	static_cast<hkaBoneAttachment*>(p)->~hkaBoneAttachment();
}

// The members release the attachment and free the two names.
hkaBoneAttachment::~hkaBoneAttachment() {}

const hkClass& hkaBoneAttachment::staticClass() {
	return hkaBoneAttachmentClass;
}
