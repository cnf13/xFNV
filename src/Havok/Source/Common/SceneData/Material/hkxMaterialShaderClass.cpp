#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/Memory/hkMemoryMeshShape.h"
#include "Havok/Source/Common/SceneData/Material/hkxMaterialShader.h"

// Havok SDK signature (hkSceneData.lib, tied by bytes,reloc).
static const void* getVtablehkxMaterialShader();

// The reflection's enum records: the names of ShaderType's values, and the enum.
using hkxMaterialShaderShaderTypeEnumItemsArray = const hkClassEnum::Item[7];
static hkxMaterialShaderShaderTypeEnumItemsArray hkxMaterialShaderShaderTypeEnumItems{
    {0, "EFFECT_TYPE_INVALID"},
    {1, "EFFECT_TYPE_UNKNOWN"},
    {2, "EFFECT_TYPE_HLSL_INLINE"},
    {3, "EFFECT_TYPE_CG_INLINE"},
    {4, "EFFECT_TYPE_HLSL_FILENAME"},
    {5, "EFFECT_TYPE_CG_FILENAME"},
    {6, "EFFECT_TYPE_MAX_ID"}
};
using hkxMaterialShaderEnumsArray = const hkClassEnum[1];
static hkxMaterialShaderEnumsArray hkxMaterialShaderEnums{
    {"ShaderType", hkxMaterialShaderShaderTypeEnumItems, 7, nullptr, {0}}
};

// The reflection's member table. The type member's enum is held directly in its data line.
using hkxMaterialShaderClass_MembersArray = const hkClassMember[6];
static hkxMaterialShaderClass_MembersArray hkxMaterialShaderClass_Members{
    {"name", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x8, nullptr},
    {"type",
     nullptr,
     &hkxMaterialShaderEnums[0],
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_UINT8},
     0,
     {0},
     0xC,
     nullptr},
    {"vertexEntryName",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_STRINGPTR},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x10,
     nullptr},
    {"geomEntryName",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_STRINGPTR},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x14,
     nullptr},
    {"pixelEntryName",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_STRINGPTR},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x18,
     nullptr},
    {"data", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT8}, 0, {0}, 0x1C, nullptr}
};
hkClass hkxMaterialShaderClass(
    "hkxMaterialShader",
    &hkReferencedObjectClass,
    0x28,
    nullptr,
    0,
    hkxMaterialShaderEnums,
    1,
    hkxMaterialShaderClass_Members,
    6,
    nullptr,
    nullptr,
    0,
    1
);
hkTypeInfo hkxMaterialShaderTypeInfo = {
    "hkxMaterialShader",
    finishLoadedObjecthkxMaterialShader,
    cleanupLoadedObjecthkxMaterialShader,
    getVtablehkxMaterialShader(),
    &typeid(hkxMaterialShader),
    sizeof(hkxMaterialShader)
};

static const void* getVtablehkxMaterialShader() {
    return hkGetVtableOf<hkxMaterialShader>();
}

void cleanupLoadedObjecthkxMaterialShader(void* p) {
    static_cast<hkxMaterialShader*>(p)->~hkxMaterialShader();
}

// KEEP: getVtablehkxMaterialShader is not written. It placement-constructs an hkxMaterialShader in a 16-byte aligned
// stack union with the finish-loading constructor, inlined: the class's vtable goes to the object's first word and a
// finish-loading constructor that only returns its object (by its use a member's, likely hkStringPtr's) runs with 0
// on m_name, m_vertexEntryName, m_geomEntryName and m_pixelEntryName. It returns that first word. src cannot
// construct hkxMaterialShader (an inlined constructor that stores the vtable).
const hkClass& hkxMaterialShader::staticClass() {
    return hkxMaterialShaderClass;
}
