#include "Havok/Source/Animation/Animation/Rig/hkaSkeleton.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/LocalFrame/hkLocalFrame.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"

static void cleanupLoadedObjecthkaSkeleton(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkaSkeletonLocalFrameOnBoneTypeInfo = {"hkaSkeletonLocalFrameOnBone",
	finishLoadedObjecthkaSkeletonLocalFrameOnBone, cleanupLoadedObjecthkaSkeletonLocalFrameOnBone, nullptr,
	&typeid(hkaSkeleton::LocalFrameOnBone), sizeof(hkaSkeleton::LocalFrameOnBone)};

// The class records' member tables.
using hkaSkeletonLocalFrameOnBoneClassMemberArray = const hkClassMember[2];
static hkaSkeletonLocalFrameOnBoneClassMemberArray hkaSkeletonLocalFrameOnBoneClass_Members{
	{"localFrame", &hkLocalFrameClass, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_STRUCT}, 0, 0, 0x0,
		nullptr},
	{"boneIndex", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x4, nullptr}};
using hkaSkeletonClassMemberArray = const hkClassMember[6];
static hkaSkeletonClassMemberArray hkaSkeletonClass_Members{
	{"name", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, 0, 0x8, nullptr},
	{"parentIndices", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_INT16}, 0, 0, 0xC, nullptr},
	{"bones", &hkaBoneClass, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_STRUCT}, 0, 0, 0x18, nullptr},
	{"referencePose", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_QSTRANSFORM}, 0, 0, 0x24,
		nullptr},
	{"floatSlots", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_STRINGPTR}, 0, 0, 0x30, nullptr},
	{"localFrames", &hkaSkeletonLocalFrameOnBoneClass, nullptr, {hkClassMember::TYPE_ARRAY},
		{hkClassMember::TYPE_STRUCT}, 0, 0, 0x3C, nullptr}};
hkClass hkaSkeletonLocalFrameOnBoneClass("hkaSkeletonLocalFrameOnBone", nullptr, 8, nullptr, 0, nullptr, 0,
	hkaSkeletonLocalFrameOnBoneClass_Members, 2, nullptr, nullptr, 0, 0);
hkClass hkaSkeletonClass("hkaSkeleton", &hkReferencedObjectClass, 72, nullptr, 0, nullptr, 0, hkaSkeletonClass_Members,
	6, nullptr, nullptr, 0, 2);
hkTypeInfo hkaSkeletonTypeInfo = {"hkaSkeleton", finishLoadedObjecthkaSkeleton, cleanupLoadedObjecthkaSkeleton,
	getVtablehkaSkeleton(), &typeid(hkaSkeleton), sizeof(hkaSkeleton)};

static void cleanupLoadedObjecthkaSkeleton(void* p) {
	static_cast<hkaSkeleton*>(p)->~hkaSkeleton();
}

const hkClass& hkaSkeleton::LocalFrameOnBone::staticClass() {
	return hkaSkeletonLocalFrameOnBoneClass;
}

// The arrays release the local frames and free the bone and slot names; the name frees itself.
hkaSkeleton::~hkaSkeleton() {}
