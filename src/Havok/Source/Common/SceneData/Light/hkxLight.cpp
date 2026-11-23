#include "Havok/Source/Common/SceneData/Light/hkxLight.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"

static void finishLoadedObjecthkxLight(void* p, int32_t finishing);
// Havok SDK signature (hkSceneData.lib, tied by reloc,target).
static const void* getVtablehkxLight();
static void cleanupLoadedObjecthkxLight(void* p);

// The reflected enum of hkxLight.
using hkxLightLightTypeEnumItemsArray = const hkClassEnum::Item[3];
static hkxLightLightTypeEnumItemsArray hkxLightLightTypeEnumItems{
    {hkxLight::POINT_LIGHT, "POINT_LIGHT"},
    {hkxLight::DIRECTIONAL_LIGHT, "DIRECTIONAL_LIGHT"},
    {hkxLight::SPOT_LIGHT, "SPOT_LIGHT"}
};
using hkxLightEnumsArray = const hkClassEnum[1];
static hkxLightEnumsArray hkxLightEnums{{"LightType", hkxLightLightTypeEnumItems, 3, nullptr, {0}}};

// The class's enum, by pointer.
static const hkClassEnum* hkxLightLightTypeEnum = &hkxLightEnums[0];

// The class record's member table: the type member names its enum by the pointer above.
using hkxLightClass_MembersArray = hkClassMember[5];
static hkxLightClass_MembersArray hkxLightClass_Members{
    {"type",
     nullptr,
     hkxLightLightTypeEnum,
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_INT8},
     0,
     {0},
     0x8,
     nullptr},
    {"position", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x10, nullptr},
    {"direction", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x20, nullptr},
    {"color", nullptr, nullptr, {hkClassMember::TYPE_UINT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x30, nullptr},
    {"angle", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x34, nullptr}
};
hkClass hkxLightClass(
    "hkxLight",
    &hkReferencedObjectClass,
    64,
    nullptr,
    0,
    hkxLightEnums,
    1,
    hkxLightClass_Members,
    5,
    nullptr,
    nullptr,
    0,
    1
);
hkTypeInfo hkxLightTypeInfo = {
    "hkxLight",
    finishLoadedObjecthkxLight,
    cleanupLoadedObjecthkxLight,
    getVtablehkxLight(),
    &typeid(hkxLight),
    sizeof(hkxLight)
};

// The type info's finish-loaded-object callback for hkxLight: its finish constructor, when the object is not null.
static void finishLoadedObjecthkxLight(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkxLight(flag);
    }
}

static const void* getVtablehkxLight() {
    return hkGetVtableOf<hkxLight>();
}

static void cleanupLoadedObjecthkxLight(void* p) {
    static_cast<hkxLight*>(p)->~hkxLight();
}

// hkxLight's reflection data and objects.

// Nothing of its own to free.
hkxLight::~hkxLight() {}

const hkClass& hkxLight::staticClass() {
    return hkxLightClass;
}
