#include "Havok/Source/Common/SceneData/Mesh/hkxVertexDescription.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkxVertexDescriptionElementDeclTypeInfo = {
    "hkxVertexDescriptionElementDecl",
    nullptr,
    cleanupLoadedObjecthkxVertexDescriptionElementDecl,
    nullptr,
    &typeid(hkxVertexDescription::ElementDecl),
    sizeof(hkxVertexDescription::ElementDecl)
};
hkTypeInfo hkxVertexDescriptionTypeInfo = {
    "hkxVertexDescription",
    finishLoadedObjecthkxVertexDescription,
    cleanupLoadedObjecthkxVertexDescription,
    nullptr,
    &typeid(hkxVertexDescription),
    sizeof(hkxVertexDescription)
};

// The reflection's enum records: the names of DataType's and DataUsage's values, and the enums.
using hkxVertexDescriptionDataTypeEnumItemsArray = const hkClassEnum::Item[8];
static hkxVertexDescriptionDataTypeEnumItemsArray hkxVertexDescriptionDataTypeEnumItems{
    {hkxVertexDescription::HKX_DT_NONE, "HKX_DT_NONE"},
    {hkxVertexDescription::HKX_DT_UINT8, "HKX_DT_UINT8"},
    {hkxVertexDescription::HKX_DT_INT16, "HKX_DT_INT16"},
    {hkxVertexDescription::HKX_DT_UINT32, "HKX_DT_UINT32"},
    {hkxVertexDescription::HKX_DT_FLOAT, "HKX_DT_FLOAT"},
    {hkxVertexDescription::HKX_DT_FLOAT2, "HKX_DT_FLOAT2"},
    {hkxVertexDescription::HKX_DT_FLOAT3, "HKX_DT_FLOAT3"},
    {hkxVertexDescription::HKX_DT_FLOAT4, "HKX_DT_FLOAT4"}
};
using hkxVertexDescriptionDataUsageEnumItemsArray = const hkClassEnum::Item[10];
static hkxVertexDescriptionDataUsageEnumItemsArray hkxVertexDescriptionDataUsageEnumItems{
    {hkxVertexDescription::HKX_DU_NONE, "HKX_DU_NONE"},
    {hkxVertexDescription::HKX_DU_POSITION, "HKX_DU_POSITION"},
    {hkxVertexDescription::HKX_DU_COLOR, "HKX_DU_COLOR"},
    {hkxVertexDescription::HKX_DU_NORMAL, "HKX_DU_NORMAL"},
    {hkxVertexDescription::HKX_DU_TANGENT, "HKX_DU_TANGENT"},
    {hkxVertexDescription::HKX_DU_BINORMAL, "HKX_DU_BINORMAL"},
    {hkxVertexDescription::HKX_DU_TEXCOORD, "HKX_DU_TEXCOORD"},
    {hkxVertexDescription::HKX_DU_BLENDWEIGHTS, "HKX_DU_BLENDWEIGHTS"},
    {hkxVertexDescription::HKX_DU_BLENDINDICES, "HKX_DU_BLENDINDICES"},
    {hkxVertexDescription::HKX_DU_USERDATA, "HKX_DU_USERDATA"}
};
using hkxVertexDescriptionEnumsArray = const hkClassEnum[2];
static hkxVertexDescriptionEnumsArray hkxVertexDescriptionEnums{
    {"DataType", hkxVertexDescriptionDataTypeEnumItems, 8, nullptr, {0}},
    {"DataUsage", hkxVertexDescriptionDataUsageEnumItems, 10, nullptr, {0}}
};
// ElementDecl's member table. The type and usage members hold their enums in the initializer itself.
using hkxVertexDescription_ElementDeclClass_MembersArray = hkClassMember[4];
static hkxVertexDescription_ElementDeclClass_MembersArray hkxVertexDescription_ElementDeclClass_Members{
    {"byteOffset", nullptr, nullptr, {hkClassMember::TYPE_UINT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x0, nullptr},
    {"type",
     nullptr,
     &hkxVertexDescriptionEnums[0],
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_UINT16},
     0,
     {0},
     0x4,
     nullptr},
    {"usage",
     nullptr,
     &hkxVertexDescriptionEnums[1],
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_UINT16},
     0,
     {0},
     0x6,
     nullptr},
    {"byteStride", nullptr, nullptr, {hkClassMember::TYPE_UINT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x8, nullptr}
};
hkClass hkxVertexDescriptionElementDeclClass(
    "hkxVertexDescriptionElementDecl",
    nullptr,
    0xC,
    nullptr,
    0,
    nullptr,
    0,
    hkxVertexDescription_ElementDeclClass_Members,
    4,
    nullptr,
    nullptr,
    0,
    1
);

hkxVertexDescription::MembersArray hkxVertexDescription::Members{
    {"decls",
     &hkxVertexDescriptionElementDeclClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x0,
     nullptr}
};
hkClass hkxVertexDescriptionClass(
    "hkxVertexDescription",
    nullptr,
    0xC,
    nullptr,
    0,
    hkxVertexDescriptionEnums,
    2,
    hkxVertexDescription::Members,
    1,
    nullptr,
    nullptr,
    0,
    1
);

const hkClass& hkxVertexDescription::ElementDecl::staticClass() {
    return hkxVertexDescriptionElementDeclClass;
}

// Compares each element's type, usage, byte offset and stride, in that order.
bool hkxVertexDescription::operator==(const hkxVertexDescription& other) const {
    int32_t numDecls = m_decls.m_size;
    if (numDecls != other.m_decls.m_size) {
        return false;
    }
    const ElementDecl* pDecl = m_decls.m_data;
    const ElementDecl* pOther = other.m_decls.m_data;
    for (int32_t i = 0; i < numDecls; ++i, ++pDecl, ++pOther) {
        if (pDecl->m_type.m_storage != pOther->m_type.m_storage ||
            pDecl->m_usage.m_storage != pOther->m_usage.m_storage ||
            pDecl->m_byteOffset != pOther->m_byteOffset ||
            pDecl->m_byteStride != pOther->m_byteStride) {
            return false;
        }
    }
    return true;
}

hkxVertexDescription::ElementDecl*
hkxVertexDescription::getElementDecl(hkxVertexDescription::DataUsage usage, int32_t index) {
    hkEnum<hkxVertexDescription::DataUsage, uint16_t>* i;
    int32_t m_size = m_decls.m_size;
    int32_t v4 = 0;
    int32_t v5 = 0;
    if (m_size <= 0) {
        return nullptr;
    }
    hkxVertexDescription::ElementDecl* m_data = m_decls.begin();
    for (i = &m_data->m_usage;; i += 6) {
        if (i->m_storage == static_cast<uint16_t>(usage)) {
            if (v4 == index) {
                return &m_data[v5];
            }
            ++v4;
        }
        if (++v5 >= m_size) {
            return nullptr;
        }
    }
}

const hkClass& hkxVertexDescription::staticClass() {
    return hkxVertexDescriptionClass;
}
