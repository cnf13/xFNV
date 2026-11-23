#include "Havok/Source/Animation/Animation/Animation/hkaAnimationBinding.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnimation.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"

static void finishLoadedObjecthkaAnimationBinding(void* p, int32_t finishing);
// Havok SDK signature (hkaAnimation.lib, tied by bytes,reloc).
static const void* getVtablehkaAnimationBinding();
static void cleanupLoadedObjecthkaAnimationBinding(void* p);

// The reflection's enum records: the names of BlendHint's values, and the enum, as file-local data.
using hkaAnimationBindingBlendHintEnumItemsArray = const hkClassEnum::Item[2];
static hkaAnimationBindingBlendHintEnumItemsArray hkaAnimationBindingBlendHintEnumItems{{0, "NORMAL"}, {1, "ADDITIVE"}};
using hkaAnimationBindingEnumsArray = const hkClassEnum[1];
static hkaAnimationBindingEnumsArray hkaAnimationBindingEnums{
    {"BlendHint", hkaAnimationBindingBlendHintEnumItems, 2, nullptr, {0}}
};

// The reflection's member table. The blendHint member's enum is null in the static data and is stored by an initialiser
// (below).
using hkaAnimationBindingClass_MembersArray = const hkClassMember[5];
static hkaAnimationBindingClass_MembersArray hkaAnimationBindingClass_Members{
    {"originalSkeletonName",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_STRINGPTR},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x8,
     nullptr},
    {"animation",
     &hkaAnimationClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0xC,
     nullptr},
    {"transformTrackToBoneIndices",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_INT16},
     0,
     {0},
     0x10,
     nullptr},
    {"floatTrackToFloatSlotIndices",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_INT16},
     0,
     {0},
     0x1C,
     nullptr},
    {"blendHint",
     nullptr,
     &hkaAnimationBindingEnums[0],
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_INT8},
     0,
     {0},
     0x28,
     nullptr}
};
hkClass hkaAnimationBindingClass(
    "hkaAnimationBinding",
    &hkReferencedObjectClass,
    0x2C,
    nullptr,
    0,
    hkaAnimationBindingEnums,
    1,
    hkaAnimationBindingClass_Members,
    5,
    nullptr,
    nullptr,
    0,
    1
);
hkTypeInfo hkaAnimationBindingTypeInfo{
    "hkaAnimationBinding",
    finishLoadedObjecthkaAnimationBinding,
    cleanupLoadedObjecthkaAnimationBinding,
    getVtablehkaAnimationBinding(),
    &typeid(hkaAnimationBinding),
    sizeof(hkaAnimationBinding)
};

static const void* getVtablehkaAnimationBinding() {
    return hkGetVtableOf<hkaAnimationBinding>();
}

// The type info's finish-loaded-object callback: the null check, then the binding's finish constructor.
static void finishLoadedObjecthkaAnimationBinding(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkaAnimationBinding(kFlag);
    }
}

static void cleanupLoadedObjecthkaAnimationBinding(void* p) {
    static_cast<hkaAnimationBinding*>(p)->~hkaAnimationBinding();
}

// The two index arrays, the animation's reference and the skeleton name go with their members.
hkaAnimationBinding::~hkaAnimationBinding() {}

const hkClass& hkaAnimationBinding::staticClass() {
    return hkaAnimationBindingClass;
}
