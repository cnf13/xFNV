#include "Havok/Source/Common/SceneData/Attributes/hkxAnimatedQuaternion.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"

static void finishLoadedObjecthkxAnimatedQuaternion(void* p, int32_t finishing);
// Havok SDK signature (hkSceneData.lib, tied by reloc,target).
static const void* getVtablehkxAnimatedQuaternion();
static void cleanupLoadedObjecthkxAnimatedQuaternion(void* p);

// The class record's member table.
using hkxAnimatedQuaternionClassMemberArray = const hkClassMember[1];
static hkxAnimatedQuaternionClassMemberArray hkxAnimatedQuaternionClass_Members{
    {"quaternions", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_QUATERNION}, 0, 0, 0x8, nullptr}
};
hkClass hkxAnimatedQuaternionClass(
    "hkxAnimatedQuaternion",
    &hkReferencedObjectClass,
    20,
    nullptr,
    0,
    nullptr,
    0,
    hkxAnimatedQuaternionClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    1
);
hkTypeInfo hkxAnimatedQuaternionTypeInfo = {
    "hkxAnimatedQuaternion",
    finishLoadedObjecthkxAnimatedQuaternion,
    cleanupLoadedObjecthkxAnimatedQuaternion,
    getVtablehkxAnimatedQuaternion(),
    &typeid(hkxAnimatedQuaternion),
    sizeof(hkxAnimatedQuaternion)
};

// The type info's finish-loaded-object callback for hkxAnimatedQuaternion: its finish constructor, when the object
// is not null.
static void finishLoadedObjecthkxAnimatedQuaternion(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkxAnimatedQuaternion(flag);
    }
}

static const void* getVtablehkxAnimatedQuaternion() {
    return hkGetVtableOf<hkxAnimatedQuaternion>();
}

static void cleanupLoadedObjecthkxAnimatedQuaternion(void* p) {
    static_cast<hkxAnimatedQuaternion*>(p)->~hkxAnimatedQuaternion();
}

// The member frees the quaternions.
hkxAnimatedQuaternion::~hkxAnimatedQuaternion() {}

const hkClass& hkxAnimatedQuaternion::staticClass() {
    return hkxAnimatedQuaternionClass;
}
