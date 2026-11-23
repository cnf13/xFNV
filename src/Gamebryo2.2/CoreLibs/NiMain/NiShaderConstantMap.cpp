#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderConstantMap.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDynamicEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDynamicEffectState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGPUProgram.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderAttributeDesc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTextureEffect.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

#include <cctype>

NiShaderConstantMap::PredefinedMapping::PredefinedMapping() : m_uiMappingID(0) {}

NiShaderConstantMap::PredefinedMapping::PredefinedMapping(uint32_t auiMappingID, const NiFixedString& akMappingName)
  : m_uiMappingID(auiMappingID) {
    m_kMappingName = akMappingName;
}

NiShaderConstantMap::NiShaderConstantMap(NiGPUProgram::ProgramType eType) : m_eProgramType(eType) {}

NiShaderConstantMap::~NiShaderConstantMap() {}

bool NiShaderConstantMap::LookUpObjectMapping(const char* pcMapping, uint32_t& uiMappingID) {
    NiShaderConstantMap::PredefinedMapping* v3;
    int32_t v2 = 0;
    if (!NiShaderConstantMap::ms_uiObjectMappingCount) {
        return false;
    }
    while (true) {
        v3 = &NiShaderConstantMap::ms_pkObjectMappings[v2];
        if (!NiStricmp(v3->m_kMappingName.GetHandle(), pcMapping)) {
            break;
        }
        if (++v2 >= NiShaderConstantMap::ms_uiObjectMappingCount) {
            return false;
        }
    }
    uiMappingID = v3->m_uiMappingID;
    return true;
}

NiShaderAttributeDesc::AttributeType
NiShaderConstantMap::LookUpObjectMappingType(uint32_t uiMappingID, uint32_t& uiRegCount, uint32_t& uiFloatCount) {
    int32_t result;
    uiRegCount = 1;
    uiFloatCount = 4;
    switch (uiMappingID) {
        case 1u:
        case 0x16u: {
            uiFloatCount = 1;
            result = 4;
            break;
        }
        case 2u:
        case 3u:
        case 4u:
        case 5u:
        case 6u:
        case 7u:
        case 8u:
        case 9u:
        case 0xAu:
        case 0xBu:
        case 0xEu:
        case 0xFu:
        case 0x12u:
        case 0x13u:
        case 0x14u:
        case 0x15u: {
            result = 7;
            break;
        }
        case 0xCu:
        case 0xDu:
        case 0x17u:
        case 0x18u: {
            uiRegCount = 4;
            uiFloatCount = 16;
            result = 9;
            break;
        }
        case 0x10u:
        case 0x11u: {
            uiRegCount = 3;
            uiFloatCount = 12;
            result = 8;
            break;
        }
        default: {
            result = 0;
            break;
        }
    }
    return static_cast<NiShaderAttributeDesc::AttributeType>(result);
}

bool NiShaderConstantMap::LookUpPredefinedMapping(
    const char* pszMapping,
    uint32_t& uiMappingID,
    uint32_t& uiNumRegisters
) {
    char pszTrueName[260];
    bool result = NiShaderConstantMap::ParseKeyName(pszMapping, pszTrueName, 260u, uiNumRegisters);
    if (result) {
        int32_t v4 = 0;
        if (NiShaderConstantMap::ms_uiPredefinedMappingCount) {
            NiShaderConstantMap::PredefinedMapping* v5;
            while (true) {
                v5 = &NiShaderConstantMap::ms_pkPredefinedMappings[v4];
                if (!NiStricmp(v5->m_kMappingName.GetHandle(), pszTrueName)) {
                    break;
                }
                if (++v4 >= NiShaderConstantMap::ms_uiPredefinedMappingCount) {
                    return false;
                }
            }
            uiMappingID = v5->m_uiMappingID;
            return true;
        }

        return false;
    }
    return result;
}

NiShaderAttributeDesc::AttributeType
NiShaderConstantMap::LookUpPredefinedMappingType(uint32_t auiMappingID, uint32_t auiNumRegisters) {
    int32_t result;
    switch (auiMappingID) {
        case 1u:
        case 2u:
        case 3u:
        case 4u:
        case 5u:
        case 6u:
        case 7u:
        case 8u:
        case 9u:
        case 0xAu:
        case 0xBu:
        case 0xCu:
        case 0xDu:
        case 0xEu:
        case 0xFu:
        case 0x10u:
        case 0x11u:
        case 0x12u:
        case 0x13u:
        case 0x14u:
        case 0x15u:
        case 0x16u:
        case 0x17u:
        case 0x18u:
        case 0x25u:
        case 0x26u:
        case 0x27u:
        case 0x28u:
        case 0x29u:
        case 0x2Au:
        case 0x2Bu:
        case 0x2Cu:
        case 0x2Du:
        case 0x2Eu:
        case 0x2Fu:
        case 0x30u:
        case 0x31u:
        case 0x32u:
        case 0x33u:
        case 0x34u:
        case 0x35u:
        case 0x36u:
        case 0x37u:
        case 0x38u:
        case 0x39u:
        case 0x3Au:
        case 0x3Bu:
        case 0x3Cu:
        case 0x3Du:
        case 0x3Eu:
        case 0x3Fu:
        case 0x40u:
        case 0x41u:
        case 0x42u:
        case 0x43u:
        case 0x44u:
        case 0x58u:
        case 0x59u:
        case 0x5Au:
        case 0x5Bu:
        case 0x5Cu:
        case 0x5Du:
        case 0x5Eu:
        case 0x5Fu: {
            result = (auiNumRegisters != 3) + 8;
            break;
        }
        case 0x19u:
        case 0x1Au:
        case 0x1Bu:
        case 0x1Cu:
        case 0x1Du:
        case 0x1Eu:
        case 0x1Fu:
        case 0x20u:
        case 0x22u:
        case 0x24u:
        case 0x60u:
        case 0x61u:
        case 0x62u:
        case 0x63u: {
            result = 9;
            break;
        }
        case 0x21u:
        case 0x23u: {
            result = 8;
            break;
        }
        case 0x45u:
        case 0x46u:
        case 0x47u:
        case 0x48u:
        case 0x49u:
        case 0x4Au:
        case 0x4Bu:
        case 0x4Cu:
        case 0x4Du:
        case 0x4Eu:
        case 0x4Fu:
        case 0x50u:
        case 0x51u:
        case 0x52u:
        case 0x53u:
        case 0x54u:
        case 0x55u:
        case 0x56u:
        case 0x57u:
        case 0x64u:
        case 0x65u:
        case 0x66u:
        case 0x67u: {
            result = 7;
            break;
        }
        case 0x68u:
        case 0x6Au:
        case 0x6Bu:
        case 0x6Cu:
        case 0x6Du:
        case 0x6Eu:
        case 0x6Fu:
        case 0x70u:
        case 0x71u:
        case 0x72u:
        case 0x73u: {
            result = 5;
            break;
        }
        case 0x69u: {
            result = 4;
            break;
        }
        default: {
            result = 0;
            break;
        }
    }
    return static_cast<NiShaderAttributeDesc::AttributeType>(result);
}

// Mapping ID 0 is also treated as no mapping.
NiShaderAttributeDesc::AttributeType NiShaderConstantMap::LookUpPredefinedMappingType(const char* apszMapping) {
    uint32_t uiMappingID;
    uint32_t uiNumRegisters;
    if (LookUpPredefinedMapping(apszMapping, uiMappingID, uiNumRegisters) && uiMappingID) {
        return LookUpPredefinedMappingType(uiMappingID, uiNumRegisters);
    }
    return NiShaderAttributeDesc::ATTRIB_TYPE_UNDEFINED;
}

NiDynamicEffect* NiShaderConstantMap::GetDynamicEffectForObject(
    const NiDynamicEffectState* pkEffectState,
    NiShaderAttributeDesc::ObjectType eObjectType,
    uint32_t uiObjectIndex
) {
    NiDynamicEffect* v14;
    NiDynamicEffectState::ListItem* m_pProjLightList;
    int32_t v17;
    int32_t v3 = 0;
    NiDynamicEffect* v20 = nullptr;
    if (!pkEffectState) {
        return v20;
    }
    switch (eObjectType) {
        case NiShaderAttributeDesc::OT_EFFECT_GENERALLIGHT:
        case NiShaderAttributeDesc::OT_EFFECT_POINTLIGHT:
        case NiShaderAttributeDesc::OT_EFFECT_DIRECTIONALLIGHT:
        case NiShaderAttributeDesc::OT_EFFECT_SPOTLIGHT: {
            NiDynamicEffect* m_pItem;
            NiDynamicEffectState::ListItem* m_pLightList = pkEffectState->m_pLightList;
            if (!m_pLightList) {
                return v20;
            }
            while (true) {
                m_pItem = m_pLightList->m_pItem;
                m_pLightList = m_pLightList->m_pNext;
                uint32_t m_ucEffectType = m_pItem->m_ucEffectType;
                bool v7 = m_ucEffectType > eObjectType && eObjectType == NiShaderAttributeDesc::OT_EFFECT_GENERALLIGHT;
                if (v7 != (m_ucEffectType == eObjectType) && m_pItem->m_bOn) {
                    int32_t v8 = v3++;
                    if (v8 == uiObjectIndex) {
                        break;
                    }
                }
                if (!m_pLightList) {
                    return nullptr;
                }
            }
            return m_pItem;
        }
        case NiShaderAttributeDesc::OT_EFFECT_SHADOWPOINTLIGHT: {
            NiTextureEffect* m_pEnvMap = pkEffectState->m_pEnvMap;
            v20 = m_pEnvMap;
            if (!m_pEnvMap) {
                return v20;
            }
            if (!m_pEnvMap->m_bOn) {
                return nullptr;
            }
            return v20;
        }
        case NiShaderAttributeDesc::OT_EFFECT_SHADOWDIRECTIONALLIGHT: {
            NiDynamicEffectState::ListItem* m_pProjShadowList = pkEffectState->m_pProjShadowList;
            int32_t v13 = 0;
            if (!m_pProjShadowList) {
                return v20;
            }
            while (true) {
                v14 = m_pProjShadowList->m_pItem;
                m_pProjShadowList = m_pProjShadowList->m_pNext;
                if (v14 && v14->m_bOn) {
                    int32_t v15 = v13++;
                    if (v15 == uiObjectIndex) {
                        break;
                    }
                }

                if (!m_pProjShadowList) {
                    return nullptr;
                }
            }
            return v14;
        }
        case NiShaderAttributeDesc::OT_EFFECT_SHADOWSPOTLIGHT: {
            m_pProjLightList = pkEffectState->m_pProjLightList;
            v17 = 0;
            if (!m_pProjLightList) {
                return v20;
            }
            break;
        }
        case NiShaderAttributeDesc::OT_EFFECT_ENVIRONMENTMAP: {
            NiTextureEffect* m_pFogMap;
            m_pFogMap = pkEffectState->m_pFogMap;
            v20 = m_pFogMap;
            if (!m_pFogMap) {
                return v20;
            }
            if (!m_pFogMap->m_bOn) {
                return nullptr;
            }
            return v20;
        }
        default: {
            return v20;
        }
    }
    while (true) {
        v14 = m_pProjLightList->m_pItem;
        m_pProjLightList = m_pProjLightList->m_pNext;
        if (v14 && v14->m_bOn) {
            int32_t v18 = v17++;
            if (v18 == uiObjectIndex) {
                break;
            }
        }

        if (!m_pProjLightList) {
            return nullptr;
        }
    }
    return v14;
}

// NiDelete[] of both tables, written out: each array's elements are walked backwards, dropping the reference on each
// name, then the block, its element-count cookie included, is handed to MemoryManager::Deallocate. The elements'
// destructor is written out so no C++ destructor runs.
void NiShaderConstantMap::_SDMShutdown() {
    PredefinedMapping* pkMappings = ms_pkObjectMappings;
    if (pkMappings != nullptr) {
        // The loop index is signed and runs down to 0.
        int32_t iCount = static_cast<int32_t>(reinterpret_cast<size_t*>(pkMappings)[-1]);
        for (int32_t i = iCount - 1; i >= 0; --i) {
            NiGlobalStringTable::GlobalStringHandle kHandle = pkMappings[i].m_kMappingName.GetHandle();
            if (kHandle != NiGlobalStringTable::NULL_STRING) {
                // A string's block is untyped memory whose first word is the reference count.
                InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kHandle) - 8));
            }
        }
        MemoryManager::Instance().Deallocate(reinterpret_cast<size_t*>(pkMappings) - 1);
    }
    ms_pkObjectMappings = nullptr;
    pkMappings = ms_pkPredefinedMappings;
    if (pkMappings != nullptr) {
        int32_t iCount = static_cast<int32_t>(reinterpret_cast<size_t*>(pkMappings)[-1]);
        for (int32_t i = iCount - 1; i >= 0; --i) {
            NiGlobalStringTable::GlobalStringHandle kHandle = pkMappings[i].m_kMappingName.GetHandle();
            if (kHandle != NiGlobalStringTable::NULL_STRING) {
                InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kHandle) - 8));
            }
        }
        MemoryManager::Instance().Deallocate(reinterpret_cast<size_t*>(pkMappings) - 1);
    }
    ms_pkPredefinedMappings = nullptr;
}

bool NiShaderConstantMap::ParseKeyName(
    const char* pszOrigName,
    char* pszTrueName,
    uint32_t uiMaxLen,
    uint32_t& uiRegisterCount
) {
    char* Context;
    char Str[260];
    if (uiMaxLen + 1 < 260) {
        return false;
    }
    strcpy_s(Str, 260, pszOrigName);
    strcpy_s(pszTrueName, 260, pszOrigName);
    char* v5 = std::strchr(Str, 95);
    if (v5 && std::isdigit(v5[1]) && (!v5[2])) {
        char* v7 = strtok_s(Str, "_", &Context);
        char* v8 = strtok_s(nullptr, "_", &Context);
        if (v8) {
            uiRegisterCount = std::atoi(v8);
            NiStrcpy(pszTrueName, uiMaxLen, v7);
        }
    }

    return true;
}

// Allocates arrays of 136 predefined and 25 object mappings with NiNew, each entry assigned a temporary
// PredefinedMapping(ID, name).
void NiShaderConstantMap::_SDMInit() {
    ms_uiPredefinedMappingCount = SCM_DEF_COUNT + 20;
    size_t* puiBlock = static_cast<size_t*>(
        MemoryManager::Instance().Allocate(sizeof(size_t) + (SCM_DEF_COUNT + 20) * sizeof(PredefinedMapping))
    );
    PredefinedMapping* pkMappings = nullptr;
    if (puiBlock) {
        // new[]'s cookie: the element count stored just before the array, written untyped.
        puiBlock[0] = SCM_DEF_COUNT + 20;
        pkMappings = reinterpret_cast<PredefinedMapping*>(puiBlock + 1);
        for (int32_t iItem = 0; iItem < static_cast<int32_t>(SCM_DEF_COUNT + 20); ++iItem) {
            ::new (static_cast<void*>(pkMappings + iItem)) PredefinedMapping;
        }
    }
    ms_pkPredefinedMappings = pkMappings;

    ms_pkPredefinedMappings[0] = PredefinedMapping(SCM_DEF_INVALID, "Invalid");
    ms_pkPredefinedMappings[1] = PredefinedMapping(SCM_DEF_PROJ, "Proj");
    ms_pkPredefinedMappings[2] = PredefinedMapping(SCM_DEF_PROJ, "Projection");
    ms_pkPredefinedMappings[3] = PredefinedMapping(SCM_DEF_INVPROJ, "InvProj");
    ms_pkPredefinedMappings[4] = PredefinedMapping(SCM_DEF_INVPROJ, "InvProjection");
    ms_pkPredefinedMappings[5] = PredefinedMapping(SCM_DEF_VIEW, "View");
    ms_pkPredefinedMappings[6] = PredefinedMapping(SCM_DEF_INVVIEW, "InvView");
    ms_pkPredefinedMappings[7] = PredefinedMapping(SCM_DEF_WORLD, "World");
    ms_pkPredefinedMappings[8] = PredefinedMapping(SCM_DEF_INVWORLD, "InvWorld");
    ms_pkPredefinedMappings[9] = PredefinedMapping(SCM_DEF_WORLDVIEW, "WorldView");
    ms_pkPredefinedMappings[10] = PredefinedMapping(SCM_DEF_INVWORLDVIEW, "InvWorldView");
    ms_pkPredefinedMappings[11] = PredefinedMapping(SCM_DEF_VIEWPROJ, "ViewProj");
    ms_pkPredefinedMappings[12] = PredefinedMapping(SCM_DEF_VIEWPROJ, "ViewProjection");
    ms_pkPredefinedMappings[13] = PredefinedMapping(SCM_DEF_INVVIEWPROJ, "InvViewProj");
    ms_pkPredefinedMappings[14] = PredefinedMapping(SCM_DEF_INVVIEWPROJ, "InvViewProjection");
    ms_pkPredefinedMappings[15] = PredefinedMapping(SCM_DEF_WORLDVIEWPROJ, "WorldViewProj");
    ms_pkPredefinedMappings[16] = PredefinedMapping(SCM_DEF_WORLDVIEWPROJ, "WorldViewProjection");
    ms_pkPredefinedMappings[17] = PredefinedMapping(SCM_DEF_INVWORLDVIEWPROJ, "InvWorldViewProj");
    ms_pkPredefinedMappings[18] = PredefinedMapping(SCM_DEF_INVWORLDVIEWPROJ, "InvWorldViewProjection");
    ms_pkPredefinedMappings[19] = PredefinedMapping(SCM_DEF_PROJ_T, "ProjTranspose");
    ms_pkPredefinedMappings[20] = PredefinedMapping(SCM_DEF_PROJ_T, "ProjectionTranspose");
    ms_pkPredefinedMappings[21] = PredefinedMapping(SCM_DEF_INVPROJ_T, "InvProjTranspose");
    ms_pkPredefinedMappings[22] = PredefinedMapping(SCM_DEF_INVPROJ_T, "InvProjectionTranspose");
    ms_pkPredefinedMappings[23] = PredefinedMapping(SCM_DEF_VIEW_T, "ViewTranspose");
    ms_pkPredefinedMappings[24] = PredefinedMapping(SCM_DEF_INVVIEW_T, "InvViewTranspose");
    ms_pkPredefinedMappings[25] = PredefinedMapping(SCM_DEF_WORLD_T, "WorldTranspose");
    ms_pkPredefinedMappings[26] = PredefinedMapping(SCM_DEF_INVWORLD_T, "InvWorldTranspose");
    ms_pkPredefinedMappings[27] = PredefinedMapping(SCM_DEF_WORLDVIEW_T, "WorldViewTranspose");
    ms_pkPredefinedMappings[28] = PredefinedMapping(SCM_DEF_INVWORLDVIEW_T, "InvWorldViewTranspose");
    ms_pkPredefinedMappings[29] = PredefinedMapping(SCM_DEF_VIEWPROJ_T, "ViewProjTranspose");
    ms_pkPredefinedMappings[30] = PredefinedMapping(SCM_DEF_VIEWPROJ_T, "ViewProjectionTranspose");
    ms_pkPredefinedMappings[31] = PredefinedMapping(SCM_DEF_INVVIEWPROJ_T, "InvViewProjTranspose");
    ms_pkPredefinedMappings[32] = PredefinedMapping(SCM_DEF_INVVIEWPROJ_T, "InvViewProjectionTranspose");
    ms_pkPredefinedMappings[33] = PredefinedMapping(SCM_DEF_WORLDVIEWPROJ_T, "WorldViewProjTranspose");
    ms_pkPredefinedMappings[34] = PredefinedMapping(SCM_DEF_WORLDVIEWPROJ_T, "WorldViewProjectionTranspose");
    ms_pkPredefinedMappings[35] = PredefinedMapping(SCM_DEF_INVWORLDVIEWPROJ_T, "InvWorldViewProjTranspose");
    ms_pkPredefinedMappings[36] = PredefinedMapping(SCM_DEF_INVWORLDVIEWPROJ_T, "InvWorldViewProjectionTranspose");
    ms_pkPredefinedMappings[37] = PredefinedMapping(SCM_DEF_SKINWORLDVIEW, "SkinWorldView");
    ms_pkPredefinedMappings[38] = PredefinedMapping(SCM_DEF_INVSKINWORLDVIEW, "InvSkinWorldView");
    ms_pkPredefinedMappings[39] = PredefinedMapping(SCM_DEF_SKINWORLDVIEW_T, "SkinWorldViewTranspose");
    ms_pkPredefinedMappings[40] = PredefinedMapping(SCM_DEF_INVSKINWORLDVIEW_T, "InvSkinWorldViewTranspose");
    ms_pkPredefinedMappings[41] = PredefinedMapping(SCM_DEF_SKINWORLDVIEWPROJ, "SkinWorldViewProj");
    ms_pkPredefinedMappings[42] = PredefinedMapping(SCM_DEF_SKINWORLDVIEWPROJ, "SkinWorldViewProjection");
    ms_pkPredefinedMappings[43] = PredefinedMapping(SCM_DEF_INVSKINWORLDVIEWPROJ, "InvSkinWorldViewProj");
    ms_pkPredefinedMappings[44] = PredefinedMapping(SCM_DEF_INVSKINWORLDVIEWPROJ, "InvSkinWorldViewProjection");
    ms_pkPredefinedMappings[45] = PredefinedMapping(SCM_DEF_SKINWORLDVIEWPROJ_T, "SkinWorldViewProjTranspose");
    ms_pkPredefinedMappings[46] = PredefinedMapping(SCM_DEF_SKINWORLDVIEWPROJ_T, "SkinWorldViewProjectionTranspose");
    ms_pkPredefinedMappings[47] = PredefinedMapping(SCM_DEF_INVSKINWORLDVIEWPROJ_T, "InvSkinWorldViewProjTranspose");
    ms_pkPredefinedMappings[48] = PredefinedMapping(
        SCM_DEF_INVSKINWORLDVIEWPROJ_T,
        "InvSkinWorldViewProjectionTranspose"
    );
    ms_pkPredefinedMappings[49] = PredefinedMapping(SCM_DEF_BONE_MATRIX_3, "BoneMatrix3");
    ms_pkPredefinedMappings[50] = PredefinedMapping(SCM_DEF_BONE_MATRIX_4, "BoneMatrix4");
    ms_pkPredefinedMappings[51] = PredefinedMapping(SCM_DEF_SKINBONE_MATRIX_3, "SkinBoneMatrix3");
    ms_pkPredefinedMappings[52] = PredefinedMapping(SCM_DEF_SKINBONE_MATRIX_4, "SkinBoneMatrix4");
    ms_pkPredefinedMappings[53] = PredefinedMapping(SCM_DEF_TEXTRANSFORMBASE, "TexTransformBase");
    ms_pkPredefinedMappings[54] = PredefinedMapping(SCM_DEF_INVTEXTRANSFORMBASE, "InvTexTransformBase");
    ms_pkPredefinedMappings[55] = PredefinedMapping(SCM_DEF_TEXTRANSFORMBASE_T, "TexTransformBaseTranspose");
    ms_pkPredefinedMappings[56] = PredefinedMapping(SCM_DEF_INVTEXTRANSFORMBASE_T, "InvTexTransformBaseTranspose");
    ms_pkPredefinedMappings[57] = PredefinedMapping(SCM_DEF_TEXTRANSFORMDARK, "TexTransformDark");
    ms_pkPredefinedMappings[58] = PredefinedMapping(SCM_DEF_INVTEXTRANSFORMDARK, "InvTexTransformDark");
    ms_pkPredefinedMappings[59] = PredefinedMapping(SCM_DEF_TEXTRANSFORMDARK_T, "TexTransformDarkTranspose");
    ms_pkPredefinedMappings[60] = PredefinedMapping(SCM_DEF_INVTEXTRANSFORMDARK_T, "InvTexTransformDarkTranspose");
    ms_pkPredefinedMappings[61] = PredefinedMapping(SCM_DEF_TEXTRANSFORMDETAIL, "TexTransformDetail");
    ms_pkPredefinedMappings[62] = PredefinedMapping(SCM_DEF_INVTEXTRANSFORMDETAIL, "InvTexTransformDetail");
    ms_pkPredefinedMappings[63] = PredefinedMapping(SCM_DEF_TEXTRANSFORMDETAIL_T, "TexTransformDetailTranspose");
    ms_pkPredefinedMappings[64] = PredefinedMapping(SCM_DEF_INVTEXTRANSFORMDETAIL_T, "InvTexTransformDetailTranspose");
    ms_pkPredefinedMappings[65] = PredefinedMapping(SCM_DEF_TEXTRANSFORMGLOSS, "TexTransformGloss");
    ms_pkPredefinedMappings[66] = PredefinedMapping(SCM_DEF_INVTEXTRANSFORMGLOSS, "InvTexTransformGloss");
    ms_pkPredefinedMappings[67] = PredefinedMapping(SCM_DEF_TEXTRANSFORMGLOSS_T, "TexTransformGlossTranspose");
    ms_pkPredefinedMappings[68] = PredefinedMapping(SCM_DEF_INVTEXTRANSFORMGLOSS_T, "InvTexTransformGlossTranspose");
    ms_pkPredefinedMappings[69] = PredefinedMapping(SCM_DEF_TEXTRANSFORMGLOW, "TexTransformGlow");
    ms_pkPredefinedMappings[70] = PredefinedMapping(SCM_DEF_INVTEXTRANSFORMGLOW, "InvTexTransformGlow");
    ms_pkPredefinedMappings[71] = PredefinedMapping(SCM_DEF_TEXTRANSFORMGLOW_T, "TexTransformGlowTranspose");
    ms_pkPredefinedMappings[72] = PredefinedMapping(SCM_DEF_INVTEXTRANSFORMGLOW_T, "InvTexTransformGlowTranspose");
    ms_pkPredefinedMappings[73] = PredefinedMapping(SCM_DEF_TEXTRANSFORMBUMP, "TexTransformBump");
    ms_pkPredefinedMappings[74] = PredefinedMapping(SCM_DEF_INVTEXTRANSFORMBUMP, "InvTexTransformBump");
    ms_pkPredefinedMappings[75] = PredefinedMapping(SCM_DEF_TEXTRANSFORMBUMP_T, "TexTransformBumpTranspose");
    ms_pkPredefinedMappings[76] = PredefinedMapping(SCM_DEF_INVTEXTRANSFORMBUMP_T, "InvTexTransformBumpTranspose");
    ms_pkPredefinedMappings[77] = PredefinedMapping(SCM_DEF_TEXTRANSFORMDECAL, "TexTransformDecal");
    ms_pkPredefinedMappings[78] = PredefinedMapping(SCM_DEF_INVTEXTRANSFORMDECAL, "InvTexTransformDecal");
    ms_pkPredefinedMappings[79] = PredefinedMapping(SCM_DEF_TEXTRANSFORMDECAL_T, "TexTransformDecalTranspose");
    ms_pkPredefinedMappings[80] = PredefinedMapping(SCM_DEF_INVTEXTRANSFORMDECAL_T, "InvTexTransformDecalTranspose");
    ms_pkPredefinedMappings[81] = PredefinedMapping(SCM_DEF_TEXTRANSFORMSHADER, "TexTransformShader");
    ms_pkPredefinedMappings[82] = PredefinedMapping(SCM_DEF_INVTEXTRANSFORMSHADER, "InvTexTransformShader");
    ms_pkPredefinedMappings[83] = PredefinedMapping(SCM_DEF_TEXTRANSFORMSHADER_T, "TexTransformShaderTranspose");
    ms_pkPredefinedMappings[84] = PredefinedMapping(SCM_DEF_INVTEXTRANSFORMSHADER_T, "InvTexTransformShaderTranspose");
    ms_pkPredefinedMappings[85] = PredefinedMapping(SCM_DEF_LIGHT_POS_WS, "LightPosWS");
    ms_pkPredefinedMappings[86] = PredefinedMapping(SCM_DEF_LIGHT_DIR_WS, "LightDirWS");
    ms_pkPredefinedMappings[87] = PredefinedMapping(SCM_DEF_LIGHT_POS_OS, "LightPosOS");
    ms_pkPredefinedMappings[88] = PredefinedMapping(SCM_DEF_LIGHT_DIR_OS, "LightDirOS");
    ms_pkPredefinedMappings[89] = PredefinedMapping(SCM_DEF_MATERIAL_DIFFUSE, "MaterialDiffuse");
    ms_pkPredefinedMappings[90] = PredefinedMapping(SCM_DEF_MATERIAL_DIFFUSE, "Diffuse");
    ms_pkPredefinedMappings[91] = PredefinedMapping(SCM_DEF_MATERIAL_AMBIENT, "MaterialAmbient");
    ms_pkPredefinedMappings[92] = PredefinedMapping(SCM_DEF_MATERIAL_AMBIENT, "Ambient");
    ms_pkPredefinedMappings[93] = PredefinedMapping(SCM_DEF_MATERIAL_SPECULAR, "MaterialSpecular");
    ms_pkPredefinedMappings[94] = PredefinedMapping(SCM_DEF_MATERIAL_SPECULAR, "Specular");
    ms_pkPredefinedMappings[95] = PredefinedMapping(SCM_DEF_MATERIAL_EMISSIVE, "MaterialEmissive");
    ms_pkPredefinedMappings[96] = PredefinedMapping(SCM_DEF_MATERIAL_POWER, "MaterialPower");
    ms_pkPredefinedMappings[97] = PredefinedMapping(SCM_DEF_MATERIAL_POWER, "SpecularPower");
    ms_pkPredefinedMappings[98] = PredefinedMapping(SCM_DEF_EYE_POS, "EyePos");
    ms_pkPredefinedMappings[99] = PredefinedMapping(SCM_DEF_EYE_DIR, "EyeDir");
    ms_pkPredefinedMappings[100] = PredefinedMapping(SCM_DEF_CONSTS_TAYLOR_SIN, "TaylorSin");
    ms_pkPredefinedMappings[101] = PredefinedMapping(SCM_DEF_CONSTS_TAYLOR_COS, "TaylorCos");
    ms_pkPredefinedMappings[102] = PredefinedMapping(SCM_DEF_CONSTS_TIME, "time");
    ms_pkPredefinedMappings[103] = PredefinedMapping(SCM_DEF_CONSTS_SINTIME, "sin_time");
    ms_pkPredefinedMappings[104] = PredefinedMapping(SCM_DEF_CONSTS_COSTIME, "cos_time");
    ms_pkPredefinedMappings[105] = PredefinedMapping(SCM_DEF_CONSTS_TANTIME, "tan_time");
    ms_pkPredefinedMappings[106] = PredefinedMapping(SCM_DEF_CONSTS_TIME_SINTIME_COSTIME_TANTIME, "time_sin_cos_tan");
    ms_pkPredefinedMappings[107] = PredefinedMapping(SCM_DEF_AMBIENTLIGHT, "AmbientLight");
    ms_pkPredefinedMappings[108] = PredefinedMapping(SCM_DEF_TEXTRANSFORMPARALLAX, "TexTransformParallax");
    ms_pkPredefinedMappings[109] = PredefinedMapping(SCM_DEF_INVTEXTRANSFORMPARALLAX, "InvTexTransformParallax");
    ms_pkPredefinedMappings[110] = PredefinedMapping(SCM_DEF_TEXTRANSFORMPARALLAX_T, "TexTransformParallaxTranspose");
    ms_pkPredefinedMappings[111] = PredefinedMapping(
        SCM_DEF_INVTEXTRANSFORMPARALLAX_T,
        "InvTexTransformParallaxTranspose"
    );
    ms_pkPredefinedMappings[112] = PredefinedMapping(SCM_DEF_TEXTRANSFORMNORMAL, "TexTransformNormal");
    ms_pkPredefinedMappings[113] = PredefinedMapping(SCM_DEF_INVTEXTRANSFORMNORMAL, "InvTexTransformNormal");
    ms_pkPredefinedMappings[114] = PredefinedMapping(SCM_DEF_TEXTRANSFORMNORMAL_T, "TexTransformNormalTranspose");
    ms_pkPredefinedMappings[115] = PredefinedMapping(SCM_DEF_INVTEXTRANSFORMNORMAL_T, "InvTexTransformNormalTranspose");
    ms_pkPredefinedMappings[116] = PredefinedMapping(SCM_DEF_SKINWORLD, "SkinWorld");
    ms_pkPredefinedMappings[117] = PredefinedMapping(SCM_DEF_INVSKINWORLD, "InvSkinWorld");
    ms_pkPredefinedMappings[118] = PredefinedMapping(SCM_DEF_SKINWORLD_T, "SkinWorldTranspose");
    ms_pkPredefinedMappings[119] = PredefinedMapping(SCM_DEF_INVSKINWORLD_T, "InvSkinWorldTranspose");
    ms_pkPredefinedMappings[120] = PredefinedMapping(SCM_DEF_FOG_DENSITY, "FogDensity");
    ms_pkPredefinedMappings[121] = PredefinedMapping(SCM_DEF_FOG_NEARFAR, "FogNearFar");
    ms_pkPredefinedMappings[122] = PredefinedMapping(SCM_DEF_FOG_COLOR, "FogColor");
    ms_pkPredefinedMappings[123] = PredefinedMapping(SCM_DEF_BUMP_MATRIX, "BumpMatrix");
    ms_pkPredefinedMappings[124] = PredefinedMapping(SCM_DEF_BUMP_LUMA_OFFSET_AND_SCALE, "BumpLumaAndScale");
    ms_pkPredefinedMappings[125] = PredefinedMapping(SCM_DEF_PARALLAX_OFFSET, "ParallaxOffset");
    ms_pkPredefinedMappings[126] = PredefinedMapping(SCM_DEF_TEXSIZEBASE, "TexSizeBase");
    ms_pkPredefinedMappings[127] = PredefinedMapping(SCM_DEF_TEXSIZEDARK, "TexSizeDark");
    ms_pkPredefinedMappings[128] = PredefinedMapping(SCM_DEF_TEXSIZEDETAIL, "TexSizeDetail");
    ms_pkPredefinedMappings[129] = PredefinedMapping(SCM_DEF_TEXSIZEGLOSS, "TexSizeGloss");
    ms_pkPredefinedMappings[130] = PredefinedMapping(SCM_DEF_TEXSIZEGLOW, "TexSizeGlow");
    ms_pkPredefinedMappings[131] = PredefinedMapping(SCM_DEF_TEXSIZEBUMP, "TexSizeBump");
    ms_pkPredefinedMappings[132] = PredefinedMapping(SCM_DEF_TEXSIZEBUMP, "TexSizeNormal");
    ms_pkPredefinedMappings[133] = PredefinedMapping(SCM_DEF_TEXSIZEBUMP, "TexSizeParallax");
    ms_pkPredefinedMappings[134] = PredefinedMapping(SCM_DEF_TEXSIZEDECAL, "TexSizeDecal");
    ms_pkPredefinedMappings[135] = PredefinedMapping(SCM_DEF_TEXSIZESHADER, "TexSizeShader");

    ms_uiObjectMappingCount = SCM_OBJ_COUNT;
    puiBlock = static_cast<size_t*>(
        MemoryManager::Instance().Allocate(sizeof(size_t) + SCM_OBJ_COUNT * sizeof(PredefinedMapping))
    );
    pkMappings = nullptr;
    if (puiBlock) {
        // new[]'s cookie, as above.
        puiBlock[0] = SCM_OBJ_COUNT;
        pkMappings = reinterpret_cast<PredefinedMapping*>(puiBlock + 1);
        for (int32_t iItem = 0; iItem < static_cast<int32_t>(SCM_OBJ_COUNT); ++iItem) {
            ::new (static_cast<void*>(pkMappings + iItem)) PredefinedMapping;
        }
    }
    ms_pkObjectMappings = pkMappings;

    ms_pkObjectMappings[0] = PredefinedMapping(SCM_OBJ_INVALID, "Invalid");
    ms_pkObjectMappings[1] = PredefinedMapping(SCM_OBJ_DIMMER, "Dimmer");
    ms_pkObjectMappings[2] = PredefinedMapping(SCM_OBJ_UNDIMMEDAMBIENT, "UndimmedAmbient");
    ms_pkObjectMappings[3] = PredefinedMapping(SCM_OBJ_UNDIMMEDDIFFUSE, "UndimmedDiffuse");
    ms_pkObjectMappings[4] = PredefinedMapping(SCM_OBJ_UNDIMMEDSPECULAR, "UndimmedSpecular");
    ms_pkObjectMappings[5] = PredefinedMapping(SCM_OBJ_AMBIENT, "Ambient");
    ms_pkObjectMappings[6] = PredefinedMapping(SCM_OBJ_DIFFUSE, "Diffuse");
    ms_pkObjectMappings[7] = PredefinedMapping(SCM_OBJ_SPECULAR, "Specular");
    ms_pkObjectMappings[8] = PredefinedMapping(SCM_OBJ_WORLDPOSITION, "WorldPosition");
    ms_pkObjectMappings[9] = PredefinedMapping(SCM_OBJ_MODELPOSITION, "ModelPosition");
    ms_pkObjectMappings[10] = PredefinedMapping(SCM_OBJ_WORLDDIRECTION, "WorldDirection");
    ms_pkObjectMappings[11] = PredefinedMapping(SCM_OBJ_MODELDIRECTION, "ModelDirection");
    ms_pkObjectMappings[12] = PredefinedMapping(SCM_OBJ_WORLDTRANSFORM, "WorldTransform");
    ms_pkObjectMappings[13] = PredefinedMapping(SCM_OBJ_MODELTRANSFORM, "ModelTransform");
    ms_pkObjectMappings[14] = PredefinedMapping(SCM_OBJ_SPOTATTENUATION, "SpotAttenuation");
    ms_pkObjectMappings[15] = PredefinedMapping(SCM_OBJ_ATTENUATION, "Attenuation");
    ms_pkObjectMappings[16] = PredefinedMapping(SCM_OBJ_WORLDPROJECTIONMATRIX, "WorldProjectionMatrix");
    ms_pkObjectMappings[17] = PredefinedMapping(SCM_OBJ_MODELPROJECTIONMATRIX, "ModelProjectionMatrix");
    ms_pkObjectMappings[18] = PredefinedMapping(SCM_OBJ_WORLDPROJECTIONTRANSLATION, "WorldProjectionTranslation");
    ms_pkObjectMappings[19] = PredefinedMapping(SCM_OBJ_MODELPROJECTIONTRANSLATION, "ModelProjectionTranslation");
    ms_pkObjectMappings[20] = PredefinedMapping(SCM_OBJ_WORLDCLIPPINGPLANE, "WorldClippingPlane");
    ms_pkObjectMappings[21] = PredefinedMapping(SCM_OBJ_MODELCLIPPINGPLANE, "ModelClippingPlane");
    ms_pkObjectMappings[22] = PredefinedMapping(SCM_OBJ_TEXCOORDGEN, "TexCoordGen");
    ms_pkObjectMappings[23] = PredefinedMapping(SCM_OBJ_WORLDPROJECTIONTRANSFORM, "WorldProjectionTransform");
    ms_pkObjectMappings[24] = PredefinedMapping(SCM_OBJ_MODELPROJECTIONTRANSFORM, "ModelProjectionTransform");
}
