#include "Havok/Source/Common/SceneData/Material/hkxMaterialEffect.h"

#include "BSCore/RTTI.h"
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
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"

static void finishLoadedObjecthkxMaterialEffect(void* p, int32_t finishing);
// Havok SDK signature (hkSceneData.lib, tied by bytes,reloc).
static const void* getVtablehkxMaterialEffect();
static void cleanupLoadedObjecthkxMaterialEffect(void* p);

// The reflection's enum records: the names of EffectType's values, and the enum.
using hkxMaterialEffectEffectTypeEnumItemsArray = const hkClassEnum::Item[7];
static hkxMaterialEffectEffectTypeEnumItemsArray hkxMaterialEffectEffectTypeEnumItems{
    {0, "EFFECT_TYPE_INVALID"},
    {1, "EFFECT_TYPE_UNKNOWN"},
    {2, "EFFECT_TYPE_HLSL_FX_INLINE"},
    {3, "EFFECT_TYPE_CG_FX_INLINE"},
    {4, "EFFECT_TYPE_HLSL_FX_FILENAME"},
    {5, "EFFECT_TYPE_CG_FX_FILENAME"},
    {6, "EFFECT_TYPE_MAX_ID"}
};
using hkxMaterialEffectEnumsArray = const hkClassEnum[1];
static hkxMaterialEffectEnumsArray hkxMaterialEffectEnums{
    {"EffectType", hkxMaterialEffectEffectTypeEnumItems, 7, nullptr, {0}}
};

using hkxMaterialEffectClass_MembersArray = const hkClassMember[3];
static hkxMaterialEffectClass_MembersArray hkxMaterialEffectClass_Members{
    {"name", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x8, nullptr},
    {"type",
     nullptr,
     &hkxMaterialEffectEnums[0],
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_UINT8},
     0,
     {0},
     0xC,
     nullptr},
    {"data", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT8}, 0, {0}, 0x10, nullptr}
};
hkClass hkxMaterialEffectClass(
    "hkxMaterialEffect",
    &hkReferencedObjectClass,
    0x1C,
    nullptr,
    0,
    hkxMaterialEffectEnums,
    1,
    hkxMaterialEffectClass_Members,
    3,
    nullptr,
    nullptr,
    0,
    1
);
hkTypeInfo hkxMaterialEffectTypeInfo = {
    "hkxMaterialEffect",
    finishLoadedObjecthkxMaterialEffect,
    cleanupLoadedObjecthkxMaterialEffect,
    getVtablehkxMaterialEffect(),
    &typeid(hkxMaterialEffect),
    sizeof(hkxMaterialEffect)
};

static const void* getVtablehkxMaterialEffect() {
    return hkGetVtableOf<hkxMaterialEffect>();
}

// The type info's finish-loaded-object callback for hkxMaterialEffect: its finish constructor, when the object is
// not null.
static void finishLoadedObjecthkxMaterialEffect(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkxMaterialEffect(flag);
    }
}

static void cleanupLoadedObjecthkxMaterialEffect(void* p) {
    static_cast<hkxMaterialEffect*>(p)->~hkxMaterialEffect();
}

// The members' destructors free m_data and release m_name.
hkxMaterialEffect::~hkxMaterialEffect() {}

const hkClass& hkxMaterialEffect::staticClass() {
    return hkxMaterialEffectClass;
}
