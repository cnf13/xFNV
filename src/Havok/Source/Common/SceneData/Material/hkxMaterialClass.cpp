#include "Havok/Source/Common/SceneData/Material/hkxMaterialClass.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"
#include "Havok/Source/Common/SceneData/Material/hkxMaterial.h"

namespace {
    // Each member's offset in this struct (-1: texture and usageHint have no default), then tcoordChannel's default.
    struct hkxMaterialTextureStage_DefaultStruct {
        int32_t s_defaultOffsets[3];
        int32_t m_tcoordChannel;
    };
} // namespace

// KEEP: getVtablehkxMaterial is not written. It placement-constructs an hkxMaterial in a 16-byte aligned stack union
// with the finish-loading constructor, inlined: the class's vtable goes to the object's first word and a finish-loading
// constructor that only returns its object (by its use hkStringPtr's) runs with 0 on m_name. It returns that first
// word. src cannot construct hkxMaterial (an inlined constructor that stores the vtable).

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkxMaterialTextureStageTypeInfo = {
    "hkxMaterialTextureStage",
    finishLoadedObjecthkxMaterialTextureStage,
    cleanupLoadedObjecthkxMaterialTextureStage,
    nullptr,
    &typeid(hkxMaterial::TextureStage),
    sizeof(hkxMaterial::TextureStage)
};

// The reflection's enum records: the names of TextureType's values, and the enum.
using hkxMaterialTextureTypeEnumItemsArray = const hkClassEnum::Item[13];
static hkxMaterialTextureTypeEnumItemsArray hkxMaterialTextureTypeEnumItems{
    {0, "TEX_UNKNOWN"},
    {1, "TEX_DIFFUSE"},
    {2, "TEX_REFLECTION"},
    {3, "TEX_BUMP"},
    {4, "TEX_NORMAL"},
    {5, "TEX_DISPLACEMENT"},
    {6, "TEX_SPECULAR"},
    {7, "TEX_SPECULARANDGLOSS"},
    {8, "TEX_OPACITY"},
    {9, "TEX_EMISSIVE"},
    {10, "TEX_REFRACTION"},
    {11, "TEX_GLOSS"},
    {12, "TEX_NOTEXPORTED"}
};
using hkxMaterialEnumsArray = const hkClassEnum[1];
static hkxMaterialEnumsArray hkxMaterialEnums{{"TextureType", hkxMaterialTextureTypeEnumItems, 13, nullptr, {0}}};

// The reflection's member table of hkxMaterial::TextureStage. The usageHint member's enum is held by its entry in this
// table.
using hkxMaterial_TextureStageClass_MembersArray = const hkClassMember[3];
static hkxMaterial_TextureStageClass_MembersArray hkxMaterial_TextureStageClass_Members{
    {"texture",
     &hkReferencedObjectClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x0,
     nullptr},
    {"usageHint",
     nullptr,
     &hkxMaterialEnums[0],
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_INT32},
     0,
     {0},
     0x4,
     nullptr},
    {"tcoordChannel", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x8, nullptr}
};

// The defaults the class record names.
static const hkxMaterialTextureStage_DefaultStruct hkxMaterialTextureStage_Default{{-1, -1, 0xC}, -1};

using hkxMaterialClass_MembersArray = const hkClassMember[8];
static hkxMaterialClass_MembersArray hkxMaterialClass_Members{
    {"name", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x14, nullptr},
    {"stages",
     &hkxMaterialTextureStageClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x18,
     nullptr},
    {"diffuseColor",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x30,
     nullptr},
    {"ambientColor",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x40,
     nullptr},
    {"specularColor",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x50,
     nullptr},
    {"emissiveColor",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x60,
     nullptr},
    {"subMaterials",
     &hkxMaterialClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {0},
     0x70,
     nullptr},
    {"extraData",
     &hkReferencedObjectClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x7C,
     nullptr}
};
hkClass hkxMaterialTextureStageClass(
    "hkxMaterialTextureStage",
    nullptr,
    0xC,
    nullptr,
    0,
    nullptr,
    0,
    hkxMaterial_TextureStageClass_Members,
    3,
    &hkxMaterialTextureStage_Default,
    nullptr,
    0,
    0
);
hkClass hkxMaterialClass(
    "hkxMaterial",
    &hkxAttributeHolderClass,
    0x80,
    nullptr,
    0,
    hkxMaterialEnums,
    1,
    hkxMaterialClass_Members,
    8,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkxMaterialTypeInfo = {
    "hkxMaterial",
    finishLoadedObjecthkxMaterial,
    cleanupLoadedObjecthkxMaterial,
    getVtablehkxMaterial(),
    &typeid(hkxMaterial),
    sizeof(hkxMaterial)
};

void cleanupLoadedObjecthkxMaterial(void* p) {
    static_cast<hkxMaterial*>(p)->~hkxMaterial();
}

const void* getVtablehkxMaterial() {
    return hkGetVtableOf<hkxMaterial>();
}

// The type info's finish-loaded-object callback for hkxMaterial: the null check, then its finish constructor.
void finishLoadedObjecthkxMaterial(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag aFlag;
        aFlag.m_finishing = finishing;
        ::new (p) hkxMaterial(aFlag);
    }
}

const hkClass& hkxMaterial::TextureStage::staticClass() {
    return hkxMaterialTextureStageClass;
}

const hkClass& hkxMaterial::staticClass() {
    return hkxMaterialClass;
}
