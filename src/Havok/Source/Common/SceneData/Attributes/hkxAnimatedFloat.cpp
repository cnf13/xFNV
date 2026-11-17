#include "Havok/Source/Common/SceneData/Attributes/hkxAnimatedFloat.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

static void finishLoadedObjecthkxAnimatedFloat(void* p, int32_t finishing);
// Havok SDK signature (hkSceneData.lib, tied by reloc,target).
static const void* getVtablehkxAnimatedFloat();
static void cleanupLoadedObjecthkxAnimatedFloat(void* p);

// The class record's member table. Its enum member reads the enum record through hkxAttributeHintEnum, which is stored
// at start-up (it holds null until then).
using hkxAnimatedFloatClassMemberTable = hkClassMember[2];
static hkxAnimatedFloatClassMemberTable hkxAnimatedFloatClass_Members{
    {"floats", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_REAL}, 0, 0, 0x8, nullptr},
    {"hint",
     nullptr,
     hkxAttributeHintEnum,
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_UINT8},
     0,
     0,
     0x14,
     nullptr}
};
hkClass hkxAnimatedFloatClass(
    "hkxAnimatedFloat",
    &hkReferencedObjectClass,
    24,
    nullptr,
    0,
    nullptr,
    0,
    hkxAnimatedFloatClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    1
);
hkTypeInfo hkxAnimatedFloatTypeInfo{
    "hkxAnimatedFloat",          finishLoadedObjecthkxAnimatedFloat, cleanupLoadedObjecthkxAnimatedFloat,
    getVtablehkxAnimatedFloat(), &typeid(hkxAnimatedFloat),          sizeof(hkxAnimatedFloat)
};

// The type info's finish-loaded-object callback for hkxAnimatedFloat: its finish constructor, when the object is
// not null.
static void finishLoadedObjecthkxAnimatedFloat(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkxAnimatedFloat(flag);
    }
}

static const void* getVtablehkxAnimatedFloat() {
    return hkGetVtableOf<hkxAnimatedFloat>();
}

static void cleanupLoadedObjecthkxAnimatedFloat(void* p) {
    static_cast<hkxAnimatedFloat*>(p)->~hkxAnimatedFloat();
}

// The member frees the floats.
hkxAnimatedFloat::~hkxAnimatedFloat() {}

const hkClass& hkxAnimatedFloat::staticClass() {
    return hkxAnimatedFloatClass;
}
