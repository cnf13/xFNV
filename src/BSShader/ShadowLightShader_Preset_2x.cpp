#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/ShadowLightShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"

#include <bit>

void ShadowLightShader::PresetADTPasses_2x() {
    NiD3DPass* v32;
    // EXE-INLINED none: NiPointer<NiD3DPass> constructor and destructor
    FNVD_INLINED_LOCAL(NiPointer<NiD3DPass>, spD3DPass, std::nullopt, std::nullopt);
    NiD3DPass* m_pObject = 0;
    spD3DPass.m_pObject = 0;
    {
        NiPointer<NiD3DTextureStage> spstage;
        if (ShadowLightShader::spPass[251].m_pObject) {
            m_pObject = ShadowLightShader::spPass[251].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[251].m_pObject;
            ++ShadowLightShader::spPass[251].m_pObject->m_uiRefCount;
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        NiD3DVertexShader* v2 = m_pObject->m_spVertexShader.m_pObject;
        NiD3DVertexShader* v3 = ShadowLightShader::spVertexShaders[53];
        if (v2 != ShadowLightShader::spVertexShaders[53]) {
            if (v2) {
                NiDecRefCount(v2);
            }
            m_pObject->m_spVertexShader.m_pObject = v3;
            if (v3) {
                NiIncRefCount(v3);
            }
        }
        NiD3DPixelShader* v4 = ShadowLightShader::spPixelShaders[48];
        m_pObject->m_spPixelShader = v4;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[251] = 896;
        ShadowLightShader::ulPixelConstantFlags[251] = 44;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[251] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[251] = 0;
        if (m_pObject != ShadowLightShader::spPass[252].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[252].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[252].m_pObject;
            if (ShadowLightShader::spPass[252].m_pObject) {
                ++ShadowLightShader::spPass[252].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        NiD3DVertexShader* v7 = ShadowLightShader::spVertexShaders[53];
        m_pObject->m_spVertexShader = v7;
        NiD3DPixelShader* v9 = ShadowLightShader::spPixelShaders[49];
        m_pObject->m_spPixelShader = v9;
        ShadowLightShader::ulVertexConstantFlags[252] = 896;
        ShadowLightShader::ulPixelConstantFlags[252] = 44;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[252] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[252] = 36;
        if (m_pObject != ShadowLightShader::spPass[253].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[253].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[253].m_pObject;
            if (ShadowLightShader::spPass[253].m_pObject) {
                ++ShadowLightShader::spPass[253].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        NiD3DVertexShader* v11 = ShadowLightShader::spVertexShaders[54];
        m_pObject->m_spVertexShader = v11;
        NiD3DPixelShader* v13 = ShadowLightShader::spPixelShaders[50];
        m_pObject->m_spPixelShader = v13;
        ShadowLightShader::ulVertexConstantFlags[253] = 896;
        ShadowLightShader::ulPixelConstantFlags[253] = 44;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[253] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[253] = 36;
        if (m_pObject != ShadowLightShader::spPass[565].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[565].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[565].m_pObject;
            if (ShadowLightShader::spPass[565].m_pObject) {
                ++ShadowLightShader::spPass[565].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 3u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 3u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        NiD3DVertexShader* v15 = ShadowLightShader::spVertexShaders[60];
        m_pObject->m_spVertexShader = v15;
        NiD3DPixelShader* v17 = ShadowLightShader::spPixelShaders[56];
        m_pObject->m_spPixelShader = v17;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[565] = 896;
        ShadowLightShader::ulPixelConstantFlags[565] = 0x40002C;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[565] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[565] = 0x400028;
        if (m_pObject != ShadowLightShader::spPass[254].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[254].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[254].m_pObject;
            if (ShadowLightShader::spPass[254].m_pObject) {
                ++ShadowLightShader::spPass[254].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            spstage.m_pObject->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            spstage.m_pObject->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 1u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        NiD3DVertexShader* v20 = ShadowLightShader::spVertexShaders[55];
        m_pObject->m_spVertexShader = v20;
        NiD3DPixelShader* v22 = ShadowLightShader::spPixelShaders[51];
        m_pObject->m_spPixelShader = v22;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[254] = 0x420384;
        ShadowLightShader::ulPixelConstantFlags[254] = 0x20002C;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[254] = 0x20380;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[254] = 44;
        if (m_pObject != ShadowLightShader::spPass[255].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[255].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[255].m_pObject;
            if (ShadowLightShader::spPass[255].m_pObject) {
                ++ShadowLightShader::spPass[255].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        NiD3DVertexShader* v24 = ShadowLightShader::spVertexShaders[53];
        m_pObject->m_spVertexShader = v24;
        NiD3DPixelShader* v26 = ShadowLightShader::spPixelShaders[52];
        m_pObject->m_spPixelShader = v26;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[255] = 896;
        ShadowLightShader::ulPixelConstantFlags[255] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[255] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[255] = 12;
        if (m_pObject != ShadowLightShader::spPass[260].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[260].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[260].m_pObject;
            if (ShadowLightShader::spPass[260].m_pObject) {
                ++ShadowLightShader::spPass[260].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        NiD3DVertexShader* v28 = ShadowLightShader::spVertexShaders[53];
        m_pObject->m_spVertexShader = v28;
        NiD3DPixelShader* v30 = ShadowLightShader::spPixelShaders[57];
        m_pObject->m_spPixelShader = v30;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[260] = 896;
        ShadowLightShader::ulPixelConstantFlags[260] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[260] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[260] = 12;
        if (m_pObject != ShadowLightShader::spPass[261].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[261].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[261].m_pObject;
            if (ShadowLightShader::spPass[261].m_pObject) {
                ++ShadowLightShader::spPass[261].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[56]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[48]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[261] = 2944;
        ShadowLightShader::ulPixelConstantFlags[261] = 44;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[261] = 776;
        if (m_pObject != ShadowLightShader::spPass[262].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[262].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[262].m_pObject;
            if (ShadowLightShader::spPass[262].m_pObject) {
                ++ShadowLightShader::spPass[262].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[56]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[52]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[262] = 2944;
        ShadowLightShader::ulPixelConstantFlags[262] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[262] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[262] = 12;
        if (m_pObject != ShadowLightShader::spPass[265].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[265].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[265].m_pObject;
            if (ShadowLightShader::spPass[265].m_pObject) {
                ++ShadowLightShader::spPass[265].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[56]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[57]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[265] = 2944;
        ShadowLightShader::ulPixelConstantFlags[265] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[265] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[265] = 12;
        if (m_pObject != ShadowLightShader::spPass[266].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[266].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[266].m_pObject;
            if (ShadowLightShader::spPass[266].m_pObject) {
                ++ShadowLightShader::spPass[266].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[57]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[53]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[266] = 1008;
        ShadowLightShader::ulPixelConstantFlags[266] = 44;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[266] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[266] = 8;
        if (m_pObject != ShadowLightShader::spPass[267].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[267].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[267].m_pObject;
            if (ShadowLightShader::spPass[267].m_pObject) {
                ++ShadowLightShader::spPass[267].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[58]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[54]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[267] = 0x4203F0;
        ShadowLightShader::ulPixelConstantFlags[267] = 44;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[267] = 0x20360;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[267] = 8;
        if (m_pObject != ShadowLightShader::spPass[268].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[268].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[268].m_pObject;
            if (ShadowLightShader::spPass[268].m_pObject) {
                ++ShadowLightShader::spPass[268].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[57]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[55]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[268] = 1008;
        ShadowLightShader::ulPixelConstantFlags[268] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[268] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[268] = 12;
        if (m_pObject != ShadowLightShader::spPass[272].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[272].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[272].m_pObject;
            if (ShadowLightShader::spPass[272].m_pObject) {
                ++ShadowLightShader::spPass[272].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[57]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[58]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[272] = 1008;
        ShadowLightShader::ulPixelConstantFlags[272] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[272] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[272] = 12;
        if (m_pObject != ShadowLightShader::spPass[273].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[273].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[273].m_pObject;
            if (ShadowLightShader::spPass[273].m_pObject) {
                ++ShadowLightShader::spPass[273].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[59]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[53]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[273] = 3056;
        ShadowLightShader::ulPixelConstantFlags[273] = 44;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[273] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[273] = 8;
        if (m_pObject != ShadowLightShader::spPass[274].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[274].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[274].m_pObject;
            if (ShadowLightShader::spPass[274].m_pObject) {
                ++ShadowLightShader::spPass[274].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[59]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[55]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[274] = 3056;
        ShadowLightShader::ulPixelConstantFlags[274] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[274] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[274] = 12;
        if (m_pObject != ShadowLightShader::spPass[277].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[277].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[277].m_pObject;
            if (ShadowLightShader::spPass[277].m_pObject) {
                ++ShadowLightShader::spPass[277].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[59]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[58]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[277] = 3056;
        ShadowLightShader::ulPixelConstantFlags[277] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[277] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[277] = 12;
        if (m_pObject != ShadowLightShader::spPass[278].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[278].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[278].m_pObject;
            if (ShadowLightShader::spPass[278].m_pObject) {
                ++ShadowLightShader::spPass[278].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[61]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[59]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[278] = 896;
        ShadowLightShader::ulPixelConstantFlags[278] = 44;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[278] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[278] = 0;
        if (m_pObject != ShadowLightShader::spPass[279].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[279].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[279].m_pObject;
            if (ShadowLightShader::spPass[279].m_pObject) {
                ++ShadowLightShader::spPass[279].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[61]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[60]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[279] = 896;
        ShadowLightShader::ulPixelConstantFlags[279] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[279] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[279] = 4;
        if (m_pObject != ShadowLightShader::spPass[283].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[283].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[283].m_pObject;
            if (ShadowLightShader::spPass[283].m_pObject) {
                ++ShadowLightShader::spPass[283].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[61]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[61]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[283] = 896;
        ShadowLightShader::ulPixelConstantFlags[283] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[283] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[283] = 4;
        if (m_pObject != ShadowLightShader::spPass[284].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[284].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[284].m_pObject;
            if (ShadowLightShader::spPass[284].m_pObject) {
                ++ShadowLightShader::spPass[284].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[62]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[59]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[284] = 2944;
        ShadowLightShader::ulPixelConstantFlags[284] = 44;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[284] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[284] = 0;
        if (m_pObject != ShadowLightShader::spPass[285].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[285].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[285].m_pObject;
            if (ShadowLightShader::spPass[285].m_pObject) {
                ++ShadowLightShader::spPass[285].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[62]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[60]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[285] = 2944;
        ShadowLightShader::ulPixelConstantFlags[285] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[285] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[285] = 4;
        if (m_pObject != ShadowLightShader::spPass[288].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[288].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[288].m_pObject;
            if (ShadowLightShader::spPass[288].m_pObject) {
                ++ShadowLightShader::spPass[288].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[62]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[61]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[288] = 2944;
        ShadowLightShader::ulPixelConstantFlags[288] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[288] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[288] = 4;
        if (m_pObject != ShadowLightShader::spPass[289].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[289].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[289].m_pObject;
            if (ShadowLightShader::spPass[289].m_pObject) {
                ++ShadowLightShader::spPass[289].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[63]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[62]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[289] = 1008;
        ShadowLightShader::ulPixelConstantFlags[289] = 44;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[289] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[289] = 0;
        if (m_pObject != ShadowLightShader::spPass[290].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[290].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[290].m_pObject;
            if (ShadowLightShader::spPass[290].m_pObject) {
                ++ShadowLightShader::spPass[290].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[63]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[63]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[290] = 1008;
        ShadowLightShader::ulPixelConstantFlags[290] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[290] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[290] = 4;
        if (m_pObject != ShadowLightShader::spPass[294].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[294].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[294].m_pObject;
            if (ShadowLightShader::spPass[294].m_pObject) {
                ++ShadowLightShader::spPass[294].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[63]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[64]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[294] = 1008;
        ShadowLightShader::ulPixelConstantFlags[294] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[294] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[294] = 4;
        if (m_pObject != ShadowLightShader::spPass[295].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[295].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[295].m_pObject;
            if (ShadowLightShader::spPass[295].m_pObject) {
                ++ShadowLightShader::spPass[295].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[64]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[62]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[295] = 3056;
        ShadowLightShader::ulPixelConstantFlags[295] = 44;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[295] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[295] = 0;
        if (m_pObject != ShadowLightShader::spPass[296].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[296].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[296].m_pObject;
            if (ShadowLightShader::spPass[296].m_pObject) {
                ++ShadowLightShader::spPass[296].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[64]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[63]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[296] = 3056;
        ShadowLightShader::ulPixelConstantFlags[296] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[296] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[296] = 4;
        if (m_pObject != ShadowLightShader::spPass[296].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[296].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[296].m_pObject;
            if (ShadowLightShader::spPass[296].m_pObject) {
                ++ShadowLightShader::spPass[296].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[64]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[63]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[296] = 3056;
        ShadowLightShader::ulPixelConstantFlags[296] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[296] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[296] = 4;
        if (m_pObject != ShadowLightShader::spPass[16].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[16].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[16].m_pObject;
            if (ShadowLightShader::spPass[16].m_pObject) {
                ++ShadowLightShader::spPass[16].m_pObject->m_uiRefCount;
            }
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[116]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[118]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[16] = 0x4080;
        ShadowLightShader::ulPixelConstantFlags[16] = 0;
        if (m_pObject != ShadowLightShader::spPass[17].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[17].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[17].m_pObject;
            if (ShadowLightShader::spPass[17].m_pObject) {
                ++ShadowLightShader::spPass[17].m_pObject->m_uiRefCount;
            }
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[117]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[119]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[17] = 0x4080;
        ShadowLightShader::ulPixelConstantFlags[17] = 0;
        if (m_pObject != ShadowLightShader::spPass[18].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[18].m_pObject;
            spD3DPass.m_pObject = ShadowLightShader::spPass[18].m_pObject;
            if (ShadowLightShader::spPass[18].m_pObject) {
                ++ShadowLightShader::spPass[18].m_pObject->m_uiRefCount;
            }
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[118]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[120]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[18] = 0x8880;
        ShadowLightShader::ulPixelConstantFlags[18] = 0;
        spD3DPass.SetPointer_(&ShadowLightShader::spPass[19]);
        v32 = spD3DPass.m_pObject;
        if (!spD3DPass.m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            v32->SetStage(v32->m_uiCurrentStage, spstage.m_pObject);
        }
        v32->SetVertexShader(ShadowLightShader::spVertexShaders[119]);
        v32->SetPixelShader(ShadowLightShader::spPixelShaders[121]);
        if (!v32->m_pkRenderStateGroup) {
            v32->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v32->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!v32->m_pkRenderStateGroup) {
            v32->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v32->m_pkRenderStateGroup->SetRenderState(15u, 1u, 1);
        if (!v32->m_pkRenderStateGroup) {
            v32->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v32->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!v32->m_pkRenderStateGroup) {
            v32->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v32->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!v32->m_pkRenderStateGroup) {
            v32->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v32->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[19] = 0x8880;
        ShadowLightShader::ulPixelConstantFlags[19] = 0;
    }
    v32->DecRefCount();
}

void ShadowLightShader::PresetADTSPasses_2x() {
    bool v2;
    NiD3DPass* v19;
    FNVD_INLINED_LOCAL(NiPointer<NiD3DPass>, spCurrentPass, std::nullopt, std::nullopt);
    uint32_t uiValue;
    NiD3DPass* m_pObject = 0;
    spCurrentPass.m_pObject = 0;
    {
        NiPointer<NiD3DTextureStage> spstage;
        if (ShadowLightShader::spPass[149].m_pObject) {
            m_pObject = ShadowLightShader::spPass[149].m_pObject;
            spCurrentPass.m_pObject = ShadowLightShader::spPass[149].m_pObject;
            ++ShadowLightShader::spPass[149].m_pObject->m_uiRefCount;
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[65]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[65]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[149] = 1920;
        ShadowLightShader::ulPixelConstantFlags[149] = 44;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[149] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[149] = 8;
        if (m_pObject != ShadowLightShader::spPass[150].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[150].m_pObject;
            spCurrentPass.m_pObject = ShadowLightShader::spPass[150].m_pObject;
            if (ShadowLightShader::spPass[150].m_pObject) {
                ++ShadowLightShader::spPass[150].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[65]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[66]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[150] = 1920;
        ShadowLightShader::ulPixelConstantFlags[150] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[150] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[150] = 12;
        if (m_pObject != ShadowLightShader::spPass[151].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[151].m_pObject;
            spCurrentPass.m_pObject = ShadowLightShader::spPass[151].m_pObject;
            if (ShadowLightShader::spPass[151].m_pObject) {
                ++ShadowLightShader::spPass[151].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[65]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[67]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[151] = 1920;
        ShadowLightShader::ulPixelConstantFlags[151] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[151] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[151] = 12;
        if (m_pObject != ShadowLightShader::spPass[155].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[155].m_pObject;
            spCurrentPass.m_pObject = ShadowLightShader::spPass[155].m_pObject;
            if (ShadowLightShader::spPass[155].m_pObject) {
                ++ShadowLightShader::spPass[155].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[66]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[65]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[155] = 3968;
        ShadowLightShader::ulPixelConstantFlags[155] = 44;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[155] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[155] = 8;
        if (m_pObject != ShadowLightShader::spPass[156].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[156].m_pObject;
            spCurrentPass.m_pObject = ShadowLightShader::spPass[156].m_pObject;
            if (ShadowLightShader::spPass[156].m_pObject) {
                ++ShadowLightShader::spPass[156].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[66]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[66]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[156] = 3968;
        ShadowLightShader::ulPixelConstantFlags[156] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[156] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[156] = 12;
        if (m_pObject != ShadowLightShader::spPass[157].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[157].m_pObject;
            spCurrentPass.m_pObject = ShadowLightShader::spPass[157].m_pObject;
            if (ShadowLightShader::spPass[157].m_pObject) {
                ++ShadowLightShader::spPass[157].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[66]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[67]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[157] = 3968;
        ShadowLightShader::ulPixelConstantFlags[157] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[157] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[157] = 12;
        if (m_pObject != ShadowLightShader::spPass[160].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[160].m_pObject;
            spCurrentPass.m_pObject = ShadowLightShader::spPass[160].m_pObject;
            if (ShadowLightShader::spPass[160].m_pObject) {
                ++ShadowLightShader::spPass[160].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[67]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[68]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[160] = 2032;
        ShadowLightShader::ulPixelConstantFlags[160] = 44;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[160] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[160] = 8;
        if (m_pObject != ShadowLightShader::spPass[161].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[161].m_pObject;
            spCurrentPass.m_pObject = ShadowLightShader::spPass[161].m_pObject;
            if (ShadowLightShader::spPass[161].m_pObject) {
                ++ShadowLightShader::spPass[161].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[67]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[69]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[161] = 2032;
        ShadowLightShader::ulPixelConstantFlags[161] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[161] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[161] = 12;
        if (m_pObject != ShadowLightShader::spPass[162].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[162].m_pObject;
            spCurrentPass.m_pObject = ShadowLightShader::spPass[162].m_pObject;
            if (ShadowLightShader::spPass[162].m_pObject) {
                ++ShadowLightShader::spPass[162].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[67]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[70]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[162] = 2032;
        ShadowLightShader::ulPixelConstantFlags[162] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[162] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[162] = 12;
        if (m_pObject != ShadowLightShader::spPass[166].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[166].m_pObject;
            spCurrentPass.m_pObject = ShadowLightShader::spPass[166].m_pObject;
            if (ShadowLightShader::spPass[166].m_pObject) {
                ++ShadowLightShader::spPass[166].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[68]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[68]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[166] = 4080;
        ShadowLightShader::ulPixelConstantFlags[166] = 44;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[166] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[166] = 8;
        if (m_pObject != ShadowLightShader::spPass[167].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[167].m_pObject;
            spCurrentPass.m_pObject = ShadowLightShader::spPass[167].m_pObject;
            if (ShadowLightShader::spPass[167].m_pObject) {
                ++ShadowLightShader::spPass[167].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[68]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[69]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[167] = 4080;
        ShadowLightShader::ulPixelConstantFlags[167] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[167] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[167] = 12;
        if (m_pObject != ShadowLightShader::spPass[168].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[168].m_pObject;
            spCurrentPass.m_pObject = ShadowLightShader::spPass[168].m_pObject;
            if (ShadowLightShader::spPass[168].m_pObject) {
                ++ShadowLightShader::spPass[168].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[68]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[70]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[168] = 4080;
        ShadowLightShader::ulPixelConstantFlags[168] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[168] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[168] = 12;
        if (m_pObject != ShadowLightShader::spPass[171].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[171].m_pObject;
            spCurrentPass.m_pObject = ShadowLightShader::spPass[171].m_pObject;
            if (ShadowLightShader::spPass[171].m_pObject) {
                ++ShadowLightShader::spPass[171].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[69]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[71]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[171] = 1920;
        ShadowLightShader::ulPixelConstantFlags[171] = 44;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[171] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[171] = 0;
        if (m_pObject != ShadowLightShader::spPass[172].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[172].m_pObject;
            spCurrentPass.m_pObject = ShadowLightShader::spPass[172].m_pObject;
            if (ShadowLightShader::spPass[172].m_pObject) {
                ++ShadowLightShader::spPass[172].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[69]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[72]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[172] = 1920;
        ShadowLightShader::ulPixelConstantFlags[172] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[172] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[172] = 4;
        if (m_pObject != ShadowLightShader::spPass[173].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[173].m_pObject;
            spCurrentPass.m_pObject = ShadowLightShader::spPass[173].m_pObject;
            if (ShadowLightShader::spPass[173].m_pObject) {
                ++ShadowLightShader::spPass[173].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[69]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[73]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[173] = 1920;
        ShadowLightShader::ulPixelConstantFlags[173] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[173] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[173] = 4;
        if (m_pObject != ShadowLightShader::spPass[177].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[177].m_pObject;
            spCurrentPass.m_pObject = ShadowLightShader::spPass[177].m_pObject;
            if (ShadowLightShader::spPass[177].m_pObject) {
                ++ShadowLightShader::spPass[177].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[70]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[71]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[177] = 3968;
        ShadowLightShader::ulPixelConstantFlags[177] = 44;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[177] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[177] = 0;
        if (m_pObject != ShadowLightShader::spPass[178].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[178].m_pObject;
            spCurrentPass.m_pObject = ShadowLightShader::spPass[178].m_pObject;
            if (ShadowLightShader::spPass[178].m_pObject) {
                ++ShadowLightShader::spPass[178].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[70]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[72]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[178] = 3968;
        ShadowLightShader::ulPixelConstantFlags[178] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[178] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[178] = 4;
        if (m_pObject != ShadowLightShader::spPass[179].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[179].m_pObject;
            spCurrentPass.m_pObject = ShadowLightShader::spPass[179].m_pObject;
            if (ShadowLightShader::spPass[179].m_pObject) {
                ++ShadowLightShader::spPass[179].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[70]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[73]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[179] = 3968;
        ShadowLightShader::ulPixelConstantFlags[179] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[179] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[179] = 4;
        if (m_pObject != ShadowLightShader::spPass[182].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[182].m_pObject;
            spCurrentPass.m_pObject = ShadowLightShader::spPass[182].m_pObject;
            if (ShadowLightShader::spPass[182].m_pObject) {
                ++ShadowLightShader::spPass[182].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[71]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[74]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[182] = 2032;
        ShadowLightShader::ulPixelConstantFlags[182] = 44;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[182] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[182] = 8;
        if (m_pObject != ShadowLightShader::spPass[183].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[183].m_pObject;
            spCurrentPass.m_pObject = ShadowLightShader::spPass[183].m_pObject;
            if (ShadowLightShader::spPass[183].m_pObject) {
                ++ShadowLightShader::spPass[183].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[71]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[75]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[183] = 2032;
        ShadowLightShader::ulPixelConstantFlags[183] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[183] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[183] = 12;
        if (m_pObject != ShadowLightShader::spPass[184].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[184].m_pObject;
            spCurrentPass.m_pObject = ShadowLightShader::spPass[184].m_pObject;
            if (ShadowLightShader::spPass[184].m_pObject) {
                ++ShadowLightShader::spPass[184].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[71]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[76]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[184] = 2032;
        ShadowLightShader::ulPixelConstantFlags[184] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[184] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[184] = 12;
        if (m_pObject != ShadowLightShader::spPass[188].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[188].m_pObject;
            spCurrentPass.m_pObject = ShadowLightShader::spPass[188].m_pObject;
            if (ShadowLightShader::spPass[188].m_pObject) {
                ++ShadowLightShader::spPass[188].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[72]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[74]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[188] = 4080;
        ShadowLightShader::ulPixelConstantFlags[188] = 44;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[188] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[188] = 8;
        if (m_pObject != ShadowLightShader::spPass[189].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[189].m_pObject;
            spCurrentPass.m_pObject = ShadowLightShader::spPass[189].m_pObject;
            if (ShadowLightShader::spPass[189].m_pObject) {
                ++ShadowLightShader::spPass[189].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[72]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[75]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[189] = 4080;
        ShadowLightShader::ulPixelConstantFlags[189] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[189] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[189] = 12;
        if (m_pObject != ShadowLightShader::spPass[190].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[190].m_pObject;
            spCurrentPass.m_pObject = ShadowLightShader::spPass[190].m_pObject;
            if (ShadowLightShader::spPass[190].m_pObject) {
                ++ShadowLightShader::spPass[190].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[72]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[76]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[190] = 4080;
        ShadowLightShader::ulPixelConstantFlags[190] = 60;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[190] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[190] = 12;
        spCurrentPass.SetPointer_(&ShadowLightShader::spPass[195]);
        NiD3DPass* v3 = spCurrentPass.m_pObject;
        if (spCurrentPass.m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            v3->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            v3->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            v3->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            v3->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            v3->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            v3->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            v3->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            v3->SetStage(7u, spstage.m_pObject);
        }
        v3->SetVertexShader(ShadowLightShader::spVertexShaders[73]);
        v3->SetPixelShader(ShadowLightShader::spPixelShaders[77]);
        if (!v3->m_pkRenderStateGroup) {
            v3->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v3->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!v3->m_pkRenderStateGroup) {
            v3->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v3->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!v3->m_pkRenderStateGroup) {
            v3->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v3->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!v3->m_pkRenderStateGroup) {
            v3->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v3->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!v3->m_pkRenderStateGroup) {
            v3->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v3->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[195] = 0x80380;
        ShadowLightShader::ulPixelConstantFlags[195] = 316;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[195] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[195] = 0;
        spCurrentPass.SetPointer_(&ShadowLightShader::spPass[196]);
        NiD3DPass* v4 = spCurrentPass.m_pObject;
        if (spCurrentPass.m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            v4->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            v4->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            v4->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            v4->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            v4->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            v4->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            v4->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            v4->SetStage(7u, spstage.m_pObject);
        }
        v4->SetVertexShader(ShadowLightShader::spVertexShaders[73]);
        v4->SetPixelShader(ShadowLightShader::spPixelShaders[78]);
        if (!v4->m_pkRenderStateGroup) {
            v4->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v4->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!v4->m_pkRenderStateGroup) {
            v4->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v4->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!v4->m_pkRenderStateGroup) {
            v4->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v4->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!v4->m_pkRenderStateGroup) {
            v4->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v4->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!v4->m_pkRenderStateGroup) {
            v4->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v4->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[196] = 0x80380;
        ShadowLightShader::ulPixelConstantFlags[196] = 316;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[196] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[196] = 0;
        spCurrentPass.SetPointer_(&ShadowLightShader::spPass[198]);
        NiD3DPass* v5 = spCurrentPass.m_pObject;
        if (spCurrentPass.m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            v5->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            v5->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            v5->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            v5->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            v5->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            v5->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            v5->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            v5->SetStage(7u, spstage.m_pObject);
        }
        v5->SetVertexShader(ShadowLightShader::spVertexShaders[74]);
        v5->SetPixelShader(ShadowLightShader::spPixelShaders[77]);
        if (!v5->m_pkRenderStateGroup) {
            v5->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v5->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!v5->m_pkRenderStateGroup) {
            v5->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v5->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!v5->m_pkRenderStateGroup) {
            v5->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v5->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!v5->m_pkRenderStateGroup) {
            v5->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v5->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!v5->m_pkRenderStateGroup) {
            v5->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v5->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[198] = 0x80B80;
        ShadowLightShader::ulPixelConstantFlags[198] = 316;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[198] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[198] = 0;
        spCurrentPass.SetPointer_(&ShadowLightShader::spPass[199]);
        NiD3DPass* v6 = spCurrentPass.m_pObject;
        if (spCurrentPass.m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            v6->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            v6->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            v6->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            v6->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            v6->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            v6->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            v6->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            v6->SetStage(7u, spstage.m_pObject);
        }
        v6->SetVertexShader(ShadowLightShader::spVertexShaders[74]);
        v6->SetPixelShader(ShadowLightShader::spPixelShaders[78]);
        if (!v6->m_pkRenderStateGroup) {
            v6->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v6->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!v6->m_pkRenderStateGroup) {
            v6->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v6->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!v6->m_pkRenderStateGroup) {
            v6->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v6->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!v6->m_pkRenderStateGroup) {
            v6->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v6->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!v6->m_pkRenderStateGroup) {
            v6->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v6->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[199] = 0x80B80;
        ShadowLightShader::ulPixelConstantFlags[199] = 316;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[199] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[199] = 0;
        spCurrentPass.SetPointer_(&ShadowLightShader::spPass[201]);
        NiD3DPass* v7 = spCurrentPass.m_pObject;
        if (spCurrentPass.m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            v7->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            v7->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            v7->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            v7->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            v7->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            v7->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            v7->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            v7->SetStage(7u, spstage.m_pObject);
        }
        v7->SetVertexShader(ShadowLightShader::spVertexShaders[75]);
        v7->SetPixelShader(ShadowLightShader::spPixelShaders[79]);
        if (!v7->m_pkRenderStateGroup) {
            v7->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v7->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!v7->m_pkRenderStateGroup) {
            v7->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v7->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!v7->m_pkRenderStateGroup) {
            v7->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v7->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!v7->m_pkRenderStateGroup) {
            v7->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v7->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!v7->m_pkRenderStateGroup) {
            v7->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v7->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[201] = 0x80380;
        ShadowLightShader::ulPixelConstantFlags[201] = 316;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[201] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[201] = 0;
        spCurrentPass.SetPointer_(&ShadowLightShader::spPass[202]);
        NiD3DPass* v8 = spCurrentPass.m_pObject;
        if (spCurrentPass.m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            v8->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            v8->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            v8->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            v8->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            v8->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            v8->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            v8->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            v8->SetStage(7u, spstage.m_pObject);
        }
        v8->SetVertexShader(ShadowLightShader::spVertexShaders[76]);
        v8->SetPixelShader(ShadowLightShader::spPixelShaders[80]);
        ShadowLightShader::ulVertexConstantFlags[202] = 0x80380;
        ShadowLightShader::ulPixelConstantFlags[202] = 316;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[202] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[202] = 0;
        spCurrentPass.SetPointer_(&ShadowLightShader::spPass[203]);
        NiD3DPass* v9 = spCurrentPass.m_pObject;
        if (spCurrentPass.m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            v9->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            v9->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            v9->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            v9->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            v9->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            v9->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            v9->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            v9->SetStage(7u, spstage.m_pObject);
        }
        v9->SetVertexShader(ShadowLightShader::spVertexShaders[75]);
        v9->SetPixelShader(ShadowLightShader::spPixelShaders[81]);
        if (!v9->m_pkRenderStateGroup) {
            v9->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v9->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!v9->m_pkRenderStateGroup) {
            v9->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v9->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!v9->m_pkRenderStateGroup) {
            v9->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v9->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!v9->m_pkRenderStateGroup) {
            v9->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v9->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!v9->m_pkRenderStateGroup) {
            v9->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v9->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[203] = 0x80380;
        ShadowLightShader::ulPixelConstantFlags[203] = 316;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[203] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[203] = 0;
        spCurrentPass.SetPointer_(&ShadowLightShader::spPass[205]);
        NiD3DPass* v10 = spCurrentPass.m_pObject;
        if (spCurrentPass.m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            v10->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            v10->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            v10->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            v10->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            v10->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            v10->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            v10->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            v10->SetStage(7u, spstage.m_pObject);
        }
        v10->SetVertexShader(ShadowLightShader::spVertexShaders[77]);
        v10->SetPixelShader(ShadowLightShader::spPixelShaders[79]);
        if (!v10->m_pkRenderStateGroup) {
            v10->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v10->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!v10->m_pkRenderStateGroup) {
            v10->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v10->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!v10->m_pkRenderStateGroup) {
            v10->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v10->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!v10->m_pkRenderStateGroup) {
            v10->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v10->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!v10->m_pkRenderStateGroup) {
            v10->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v10->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[205] = 0x80B80;
        ShadowLightShader::ulPixelConstantFlags[205] = 316;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[205] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[205] = 0;
        spCurrentPass.SetPointer_(&ShadowLightShader::spPass[206]);
        NiD3DPass* v11 = spCurrentPass.m_pObject;
        if (spCurrentPass.m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            v11->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            v11->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            v11->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            v11->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            v11->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            v11->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            v11->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            v11->SetStage(7u, spstage.m_pObject);
        }
        v11->SetVertexShader(ShadowLightShader::spVertexShaders[77]);
        v11->SetPixelShader(ShadowLightShader::spPixelShaders[81]);
        if (!v11->m_pkRenderStateGroup) {
            v11->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v11->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!v11->m_pkRenderStateGroup) {
            v11->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v11->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!v11->m_pkRenderStateGroup) {
            v11->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v11->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!v11->m_pkRenderStateGroup) {
            v11->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v11->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!v11->m_pkRenderStateGroup) {
            v11->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v11->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[206] = 0x80B80;
        ShadowLightShader::ulPixelConstantFlags[206] = 316;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[206] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[206] = 0;
        spCurrentPass.SetPointer_(&ShadowLightShader::spPass[208]);
        NiD3DPass* v12 = spCurrentPass.m_pObject;
        if (spCurrentPass.m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            v12->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            v12->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            v12->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            v12->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            v12->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            v12->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            v12->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            v12->SetStage(7u, spstage.m_pObject);
        }
        v12->SetVertexShader(ShadowLightShader::spVertexShaders[78]);
        v12->SetPixelShader(ShadowLightShader::spPixelShaders[82]);
        if (!v12->m_pkRenderStateGroup) {
            v12->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v12->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!v12->m_pkRenderStateGroup) {
            v12->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v12->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!v12->m_pkRenderStateGroup) {
            v12->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v12->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!v12->m_pkRenderStateGroup) {
            v12->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v12->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!v12->m_pkRenderStateGroup) {
            v12->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v12->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[208] = 0x80780;
        ShadowLightShader::ulPixelConstantFlags[208] = 316;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[208] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[208] = 0;
        spCurrentPass.SetPointer_(&ShadowLightShader::spPass[209]);
        NiD3DPass* v13 = spCurrentPass.m_pObject;
        if (spCurrentPass.m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            v13->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            v13->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            v13->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            v13->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            v13->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            v13->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            v13->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            v13->SetStage(7u, spstage.m_pObject);
        }
        v13->SetVertexShader(ShadowLightShader::spVertexShaders[79]);
        v13->SetPixelShader(ShadowLightShader::spPixelShaders[83]);
        ShadowLightShader::ulVertexConstantFlags[209] = 0x80780;
        ShadowLightShader::ulPixelConstantFlags[209] = 316;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[209] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[209] = 0;
        spCurrentPass.SetPointer_(&ShadowLightShader::spPass[210]);
        NiD3DPass* v14 = spCurrentPass.m_pObject;
        if (spCurrentPass.m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            v14->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            v14->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            v14->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            v14->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            v14->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            v14->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            v14->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            v14->SetStage(7u, spstage.m_pObject);
        }
        v14->SetVertexShader(ShadowLightShader::spVertexShaders[78]);
        v14->SetPixelShader(ShadowLightShader::spPixelShaders[84]);
        if (!v14->m_pkRenderStateGroup) {
            v14->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v14->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!v14->m_pkRenderStateGroup) {
            v14->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v14->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!v14->m_pkRenderStateGroup) {
            v14->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v14->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!v14->m_pkRenderStateGroup) {
            v14->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v14->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!v14->m_pkRenderStateGroup) {
            v14->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v14->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[210] = 0x80780;
        ShadowLightShader::ulPixelConstantFlags[210] = 316;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[210] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[210] = 0;
        spCurrentPass.SetPointer_(&ShadowLightShader::spPass[212]);
        NiD3DPass* v15 = spCurrentPass.m_pObject;
        if (spCurrentPass.m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            v15->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            v15->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            v15->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            v15->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            v15->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            v15->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            v15->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            v15->SetStage(7u, spstage.m_pObject);
        }
        v15->SetVertexShader(ShadowLightShader::spVertexShaders[80]);
        v15->SetPixelShader(ShadowLightShader::spPixelShaders[82]);
        if (!v15->m_pkRenderStateGroup) {
            v15->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v15->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!v15->m_pkRenderStateGroup) {
            v15->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v15->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!v15->m_pkRenderStateGroup) {
            v15->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v15->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!v15->m_pkRenderStateGroup) {
            v15->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v15->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!v15->m_pkRenderStateGroup) {
            v15->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v15->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[212] = 0x80F80;
        ShadowLightShader::ulPixelConstantFlags[212] = 316;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[212] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[212] = 0;
        spCurrentPass.SetPointer_(&ShadowLightShader::spPass[213]);
        NiD3DPass* v16 = spCurrentPass.m_pObject;
        if (spCurrentPass.m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            v16->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            v16->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            v16->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            v16->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            v16->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            v16->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            v16->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            v16->SetStage(7u, spstage.m_pObject);
        }
        v16->SetVertexShader(ShadowLightShader::spVertexShaders[80]);
        v16->SetPixelShader(ShadowLightShader::spPixelShaders[84]);
        if (!v16->m_pkRenderStateGroup) {
            v16->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v16->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!v16->m_pkRenderStateGroup) {
            v16->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v16->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!v16->m_pkRenderStateGroup) {
            v16->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v16->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!v16->m_pkRenderStateGroup) {
            v16->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v16->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!v16->m_pkRenderStateGroup) {
            v16->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v16->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[213] = 0x80F80;
        ShadowLightShader::ulPixelConstantFlags[213] = 316;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[213] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[213] = 0;
        spCurrentPass.SetPointer_(&ShadowLightShader::spPass[561]);
        NiD3DPass* v17 = spCurrentPass.m_pObject;
        if (spCurrentPass.m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            v17->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            v17->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            v17->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            v17->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            v17->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            v17->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            v17->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            v17->SetStage(7u, spstage.m_pObject);
        }
        if (BSShaderManager::eShadowMode == BSShaderManager::BSSM_SHADOW_PCF4 ||
            BSShaderManager::eShadowMode == BSShaderManager::BSSM_SHADOW_PCF9) {
            v17->m_kStages.m_pBase[6].m_pObject->SetFilterMode(NiTexturingProperty::FILTER_NEAREST);
        }
        else {
            v17->m_kStages.m_pBase[6].m_pObject->SetFilterMode(NiTexturingProperty::FILTER_BILERP);
        }
        v17->SetVertexShader(ShadowLightShader::spVertexShaders[142]);
        if (BSShaderManager::eShadowMode == BSShaderManager::BSSM_SHADOW_LOOPFILTER) {
            v17->SetPixelShader(ShadowLightShader::spPixelShaders[2 * BSShaderManager::iShadowFilter + 200]);
        }
        else {
            v17->SetPixelShader(ShadowLightShader::spPixelShaders[200]);
        }
        if (!v17->m_pkRenderStateGroup) {
            v17->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v17->m_pkRenderStateGroup->SetRenderState(27u, 1u, 1);
        if (!v17->m_pkRenderStateGroup) {
            v17->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v17->m_pkRenderStateGroup->SetRenderState(19u, 9u, 1);
        if (!v17->m_pkRenderStateGroup) {
            v17->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v17->m_pkRenderStateGroup->SetRenderState(20u, 1u, 1);
        if (!v17->m_pkRenderStateGroup) {
            v17->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v17->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
        if (!v17->m_pkRenderStateGroup) {
            v17->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v17->m_pkRenderStateGroup->SetRenderState(152u, 63u, 1);
        *reinterpret_cast<float*>(&uiValue) = BSShaderProperty::fDepthBiasUnit;
        if (!v17->m_pkRenderStateGroup) {
            v17->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v17->m_pkRenderStateGroup->SetRenderState(195u, uiValue, 1);
        ShadowLightShader::ulVertexConstantFlags[561] = 388;
        ShadowLightShader::ulPixelConstantFlags[561] = 8;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[561] = 256;
        spCurrentPass.SetPointer_(&ShadowLightShader::spPass[562]);
        NiD3DPass* v18 = spCurrentPass.m_pObject;
        if (spCurrentPass.m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            v18->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            v18->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            v18->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            v18->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            v18->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            v18->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            v18->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            v18->SetStage(7u, spstage.m_pObject);
        }
        if (BSShaderManager::eShadowMode == BSShaderManager::BSSM_SHADOW_PCF4 ||
            BSShaderManager::eShadowMode == BSShaderManager::BSSM_SHADOW_PCF9) {
            v18->m_kStages.m_pBase[6].m_pObject->SetFilterMode(NiTexturingProperty::FILTER_NEAREST);
        }
        else {
            v18->m_kStages.m_pBase[6].m_pObject->SetFilterMode(NiTexturingProperty::FILTER_BILERP);
        }
        v18->SetVertexShader(ShadowLightShader::spVertexShaders[143]);
        if (BSShaderManager::eShadowMode == BSShaderManager::BSSM_SHADOW_LOOPFILTER) {
            v18->SetPixelShader(ShadowLightShader::spPixelShaders[2 * BSShaderManager::iShadowFilter + 200]);
        }
        else {
            v18->SetPixelShader(ShadowLightShader::spPixelShaders[200]);
        }
        if (!v18->m_pkRenderStateGroup) {
            v18->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v18->m_pkRenderStateGroup->SetRenderState(27u, 1u, 1);
        if (!v18->m_pkRenderStateGroup) {
            v18->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v18->m_pkRenderStateGroup->SetRenderState(19u, 9u, 1);
        if (!v18->m_pkRenderStateGroup) {
            v18->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v18->m_pkRenderStateGroup->SetRenderState(20u, 1u, 1);
        if (!v18->m_pkRenderStateGroup) {
            v18->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v18->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
        v2 = v18->m_pkRenderStateGroup == 0;
        *reinterpret_cast<float*>(&uiValue) = BSShaderProperty::fDepthBiasUnit;
        if (v2) {
            v18->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v18->m_pkRenderStateGroup->SetRenderState(195u, uiValue, 1);
        if (!v18->m_pkRenderStateGroup) {
            v18->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v18->m_pkRenderStateGroup->SetRenderState(152u, 63u, 1);
        ShadowLightShader::ulVertexConstantFlags[562] = 2436;
        ShadowLightShader::ulPixelConstantFlags[562] = 8;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[562] = 256;
        spCurrentPass.SetPointer_(&ShadowLightShader::spPass[563]);
        v19 = spCurrentPass.m_pObject;
        if (spCurrentPass.m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            v19->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            v19->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            v19->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            v19->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            v19->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            v19->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            spstage.m_pObject->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
            v19->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            v19->SetStage(7u, spstage.m_pObject);
        }
        if (BSShaderManager::eShadowMode == BSShaderManager::BSSM_SHADOW_PCF4 ||
            BSShaderManager::eShadowMode == BSShaderManager::BSSM_SHADOW_PCF9) {
            v19->m_kStages.m_pBase[6].m_pObject->SetFilterMode(NiTexturingProperty::FILTER_NEAREST);
        }
        else {
            v19->m_kStages.m_pBase[6].m_pObject->SetFilterMode(NiTexturingProperty::FILTER_BILERP);
        }
        v19->SetVertexShader(ShadowLightShader::spVertexShaders[144]);
        if (BSShaderManager::eShadowMode == BSShaderManager::BSSM_SHADOW_LOOPFILTER) {
            v19->SetPixelShader(ShadowLightShader::spPixelShaders[2 * BSShaderManager::iShadowFilter + 200]);
        }
        else {
            v19->SetPixelShader(ShadowLightShader::spPixelShaders[200]);
        }
        if (!v19->m_pkRenderStateGroup) {
            v19->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v19->m_pkRenderStateGroup->SetRenderState(27u, 1u, 1);
        if (!v19->m_pkRenderStateGroup) {
            v19->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v19->m_pkRenderStateGroup->SetRenderState(19u, 9u, 1);
        if (!v19->m_pkRenderStateGroup) {
            v19->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v19->m_pkRenderStateGroup->SetRenderState(20u, 1u, 1);
        if (!v19->m_pkRenderStateGroup) {
            v19->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v19->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
        v2 = v19->m_pkRenderStateGroup == 0;
        *reinterpret_cast<float*>(&uiValue) = BSShaderProperty::fDepthBiasUnit;
        if (v2) {
            v19->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v19->m_pkRenderStateGroup->SetRenderState(195u, uiValue, 1);
        if (!v19->m_pkRenderStateGroup) {
            v19->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v19->m_pkRenderStateGroup->SetRenderState(152u, 63u, 1);
        ShadowLightShader::ulVertexConstantFlags[563] = 388;
        ShadowLightShader::ulPixelConstantFlags[563] = 8;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[563] = 256;
    }
    v19->DecRefCount();
}

void ShadowLightShader::PresetADPasses_2x() {
    NiD3DPass* m_pObject = 0;
    NiD3DPass* v69 = 0;
    {
        NiPointer<NiD3DTextureStage> spstage;
        if (ShadowLightShader::spPass[215].m_pObject) {
            m_pObject = ShadowLightShader::spPass[215].m_pObject;
            v69 = ShadowLightShader::spPass[215].m_pObject;
            ++ShadowLightShader::spPass[215].m_pObject->m_uiRefCount;
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v2 = m_pObject->m_spVertexShader.m_pObject;
        NiD3DVertexShader* v3 = ShadowLightShader::spVertexShaders[81];
        if (v2 != ShadowLightShader::spVertexShaders[81]) {
            if (v2) {
                NiDecRefCount(v2);
            }
            m_pObject->m_spVertexShader.m_pObject = v3;
            if (v3) {
                NiIncRefCount(v3);
            }
        }
        NiD3DPixelShader* v4 = ShadowLightShader::spPixelShaders[85];
        m_pObject->m_spPixelShader = v4;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[215] = 1152;
        ShadowLightShader::ulPixelConstantFlags[215] = 12;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[215] = 0;
        if (m_pObject != ShadowLightShader::spPass[216].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[216].m_pObject;
            v69 = ShadowLightShader::spPass[216].m_pObject;
            if (ShadowLightShader::spPass[216].m_pObject) {
                ++ShadowLightShader::spPass[216].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v7 = ShadowLightShader::spVertexShaders[81];
        m_pObject->m_spVertexShader = v7;
        NiD3DPixelShader* v9 = ShadowLightShader::spPixelShaders[86];
        m_pObject->m_spPixelShader = v9;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[216] = 1152;
        ShadowLightShader::ulPixelConstantFlags[216] = 28;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[216] = 4;
        if (m_pObject != ShadowLightShader::spPass[220].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[220].m_pObject;
            v69 = ShadowLightShader::spPass[220].m_pObject;
            if (ShadowLightShader::spPass[220].m_pObject) {
                ++ShadowLightShader::spPass[220].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v11 = ShadowLightShader::spVertexShaders[82];
        m_pObject->m_spVertexShader = v11;
        NiD3DPixelShader* v13 = ShadowLightShader::spPixelShaders[85];
        m_pObject->m_spPixelShader = v13;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[220] = 3200;
        ShadowLightShader::ulPixelConstantFlags[220] = 12;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[220] = 0;
        if (m_pObject != ShadowLightShader::spPass[221].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[221].m_pObject;
            v69 = ShadowLightShader::spPass[221].m_pObject;
            if (ShadowLightShader::spPass[221].m_pObject) {
                ++ShadowLightShader::spPass[221].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v15 = ShadowLightShader::spVertexShaders[82];
        m_pObject->m_spVertexShader = v15;
        NiD3DPixelShader* v17 = ShadowLightShader::spPixelShaders[86];
        m_pObject->m_spPixelShader = v17;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[221] = 3200;
        ShadowLightShader::ulPixelConstantFlags[221] = 28;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[221] = 4;
        if (m_pObject != ShadowLightShader::spPass[224].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[224].m_pObject;
            v69 = ShadowLightShader::spPass[224].m_pObject;
            if (ShadowLightShader::spPass[224].m_pObject) {
                ++ShadowLightShader::spPass[224].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v19 = ShadowLightShader::spVertexShaders[83];
        m_pObject->m_spVertexShader = v19;
        NiD3DPixelShader* v21 = ShadowLightShader::spPixelShaders[87];
        m_pObject->m_spPixelShader = v21;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[224] = 1264;
        ShadowLightShader::ulPixelConstantFlags[224] = 12;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[224] = 96;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[224] = 0;
        if (m_pObject != ShadowLightShader::spPass[225].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[225].m_pObject;
            v69 = ShadowLightShader::spPass[225].m_pObject;
            if (ShadowLightShader::spPass[225].m_pObject) {
                ++ShadowLightShader::spPass[225].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v23 = ShadowLightShader::spVertexShaders[83];
        m_pObject->m_spVertexShader = v23;
        NiD3DPixelShader* v25 = ShadowLightShader::spPixelShaders[88];
        m_pObject->m_spPixelShader = v25;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[225] = 1264;
        ShadowLightShader::ulPixelConstantFlags[225] = 28;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[225] = 96;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[225] = 4;
        if (m_pObject != ShadowLightShader::spPass[229].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[229].m_pObject;
            v69 = ShadowLightShader::spPass[229].m_pObject;
            if (ShadowLightShader::spPass[229].m_pObject) {
                ++ShadowLightShader::spPass[229].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v27 = ShadowLightShader::spVertexShaders[84];
        m_pObject->m_spVertexShader = v27;
        NiD3DPixelShader* v29 = ShadowLightShader::spPixelShaders[87];
        m_pObject->m_spPixelShader = v29;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[229] = 3312;
        ShadowLightShader::ulPixelConstantFlags[225] = 12;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[225] = 96;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[225] = 0;
        if (m_pObject != ShadowLightShader::spPass[230].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[230].m_pObject;
            v69 = ShadowLightShader::spPass[230].m_pObject;
            if (ShadowLightShader::spPass[230].m_pObject) {
                ++ShadowLightShader::spPass[230].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v31 = ShadowLightShader::spVertexShaders[84];
        m_pObject->m_spVertexShader = v31;
        NiD3DPixelShader* v33 = ShadowLightShader::spPixelShaders[88];
        m_pObject->m_spPixelShader = v33;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[230] = 3312;
        ShadowLightShader::ulPixelConstantFlags[230] = 28;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[230] = 96;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[230] = 4;
        if (m_pObject != ShadowLightShader::spPass[233].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[233].m_pObject;
            v69 = ShadowLightShader::spPass[233].m_pObject;
            if (ShadowLightShader::spPass[233].m_pObject) {
                ++ShadowLightShader::spPass[233].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v35 = ShadowLightShader::spVertexShaders[85];
        m_pObject->m_spVertexShader = v35;
        NiD3DPixelShader* v37 = ShadowLightShader::spPixelShaders[89];
        m_pObject->m_spPixelShader = v37;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[233] = 1152;
        ShadowLightShader::ulPixelConstantFlags[233] = 12;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[233] = 0;
        if (m_pObject != ShadowLightShader::spPass[234].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[234].m_pObject;
            v69 = ShadowLightShader::spPass[234].m_pObject;
            if (ShadowLightShader::spPass[234].m_pObject) {
                ++ShadowLightShader::spPass[234].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v39 = ShadowLightShader::spVertexShaders[85];
        m_pObject->m_spVertexShader = v39;
        NiD3DPixelShader* v41 = ShadowLightShader::spPixelShaders[90];
        m_pObject->m_spPixelShader = v41;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[234] = 1152;
        ShadowLightShader::ulPixelConstantFlags[234] = 28;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[234] = 4;
        if (m_pObject != ShadowLightShader::spPass[238].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[238].m_pObject;
            v69 = ShadowLightShader::spPass[238].m_pObject;
            if (ShadowLightShader::spPass[238].m_pObject) {
                ++ShadowLightShader::spPass[238].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v43 = ShadowLightShader::spVertexShaders[86];
        m_pObject->m_spVertexShader = v43;
        NiD3DPixelShader* v45 = ShadowLightShader::spPixelShaders[89];
        m_pObject->m_spPixelShader = v45;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[238] = 3200;
        ShadowLightShader::ulPixelConstantFlags[238] = 12;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[238] = 0;
        if (m_pObject != ShadowLightShader::spPass[239].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[239].m_pObject;
            v69 = ShadowLightShader::spPass[239].m_pObject;
            if (ShadowLightShader::spPass[239].m_pObject) {
                ++ShadowLightShader::spPass[239].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v47 = ShadowLightShader::spVertexShaders[86];
        m_pObject->m_spVertexShader = v47;
        NiD3DPixelShader* v49 = ShadowLightShader::spPixelShaders[90];
        m_pObject->m_spPixelShader = v49;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[239] = 3200;
        ShadowLightShader::ulPixelConstantFlags[239] = 28;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[239] = 4;
        if (m_pObject != ShadowLightShader::spPass[242].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[242].m_pObject;
            v69 = ShadowLightShader::spPass[242].m_pObject;
            if (ShadowLightShader::spPass[242].m_pObject) {
                ++ShadowLightShader::spPass[242].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v51 = ShadowLightShader::spVertexShaders[87];
        m_pObject->m_spVertexShader = v51;
        NiD3DPixelShader* v53 = ShadowLightShader::spPixelShaders[91];
        m_pObject->m_spPixelShader = v53;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[242] = 1264;
        ShadowLightShader::ulPixelConstantFlags[242] = 12;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[242] = 96;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[242] = 0;
        if (m_pObject != ShadowLightShader::spPass[243].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[243].m_pObject;
            v69 = ShadowLightShader::spPass[243].m_pObject;
            if (ShadowLightShader::spPass[243].m_pObject) {
                ++ShadowLightShader::spPass[243].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v55 = ShadowLightShader::spVertexShaders[87];
        m_pObject->m_spVertexShader = v55;
        NiD3DPixelShader* v57 = ShadowLightShader::spPixelShaders[92];
        m_pObject->m_spPixelShader = v57;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[243] = 1264;
        ShadowLightShader::ulPixelConstantFlags[243] = 28;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[243] = 96;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[243] = 4;
        if (m_pObject != ShadowLightShader::spPass[247].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[247].m_pObject;
            v69 = ShadowLightShader::spPass[247].m_pObject;
            if (ShadowLightShader::spPass[247].m_pObject) {
                ++ShadowLightShader::spPass[247].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v59 = ShadowLightShader::spVertexShaders[88];
        m_pObject->m_spVertexShader = v59;
        NiD3DPixelShader* v61 = ShadowLightShader::spPixelShaders[91];
        m_pObject->m_spPixelShader = v61;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[247] = 3312;
        ShadowLightShader::ulPixelConstantFlags[247] = 12;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[247] = 96;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[247] = 0;
        if (m_pObject != ShadowLightShader::spPass[248].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[248].m_pObject;
            v69 = ShadowLightShader::spPass[248].m_pObject;
            if (ShadowLightShader::spPass[248].m_pObject) {
                ++ShadowLightShader::spPass[248].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v63 = ShadowLightShader::spVertexShaders[88];
        m_pObject->m_spVertexShader = v63;
        NiD3DPixelShader* v65 = ShadowLightShader::spPixelShaders[92];
        m_pObject->m_spPixelShader = v65;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[248] = 3312;
        ShadowLightShader::ulPixelConstantFlags[248] = 28;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[248] = 96;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[248] = 4;
    }
    m_pObject->DecRefCount();
}

void ShadowLightShader::PresetDiffusePtPasses_2x() {
    NiD3DPass* m_pObject = 0;
    NiD3DPass* v22 = 0;
    {
        NiPointer<NiD3DTextureStage> spstage;
        if (ShadowLightShader::spPass[306].m_pObject) {
            m_pObject = ShadowLightShader::spPass[306].m_pObject;
            v22 = ShadowLightShader::spPass[306].m_pObject;
            ++ShadowLightShader::spPass[306].m_pObject->m_uiRefCount;
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
        }
        NiD3DVertexShader* v2 = m_pObject->m_spVertexShader.m_pObject;
        NiD3DVertexShader* v3 = ShadowLightShader::spVertexShaders[89];
        if (v2 != v3) {
            if (v2) {
                NiDecRefCount(v2);
            }
            m_pObject->m_spVertexShader.m_pObject = v3;
            if (v3) {
                NiIncRefCount(v3);
            }
        }
        NiD3DPixelShader* v5 = ShadowLightShader::spPixelShaders[93];
        m_pObject->m_spPixelShader = v5;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        ShadowLightShader::ulVertexConstantFlags[306] = 128;
        ShadowLightShader::ulPixelConstantFlags[306] = 8;
        if (m_pObject != ShadowLightShader::spPass[309].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[309].m_pObject;
            v22 = ShadowLightShader::spPass[309].m_pObject;
            if (ShadowLightShader::spPass[309].m_pObject) {
                ++ShadowLightShader::spPass[309].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
        }
        NiD3DVertexShader* v8 = ShadowLightShader::spVertexShaders[90];
        m_pObject->m_spVertexShader = v8;
        NiD3DPixelShader* v10 = ShadowLightShader::spPixelShaders[93];
        m_pObject->m_spPixelShader = v10;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        ShadowLightShader::ulVertexConstantFlags[309] = 2176;
        ShadowLightShader::ulPixelConstantFlags[309] = 8;
        if (m_pObject != ShadowLightShader::spPass[318].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[318].m_pObject;
            v22 = ShadowLightShader::spPass[318].m_pObject;
            if (ShadowLightShader::spPass[318].m_pObject) {
                ++ShadowLightShader::spPass[318].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
        }
        NiD3DVertexShader* v12 = ShadowLightShader::spVertexShaders[91];
        m_pObject->m_spVertexShader = v12;
        NiD3DPixelShader* v14 = ShadowLightShader::spPixelShaders[94];
        m_pObject->m_spPixelShader = v14;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        ShadowLightShader::ulVertexConstantFlags[318] = 128;
        ShadowLightShader::ulPixelConstantFlags[318] = 8;
        if (m_pObject != ShadowLightShader::spPass[321].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[321].m_pObject;
            v22 = ShadowLightShader::spPass[321].m_pObject;
            if (ShadowLightShader::spPass[321].m_pObject) {
                ++ShadowLightShader::spPass[321].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
        }
        NiD3DVertexShader* v16 = ShadowLightShader::spVertexShaders[92];
        m_pObject->m_spVertexShader = v16;
        NiD3DPixelShader* v18 = ShadowLightShader::spPixelShaders[94];
        m_pObject->m_spPixelShader = v18;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        ShadowLightShader::ulVertexConstantFlags[321] = 2176;
        ShadowLightShader::ulPixelConstantFlags[321] = 8;
    }
    m_pObject->DecRefCount();
}

void ShadowLightShader::PresetSpecularPasses_2x() {
    NiD3DPass* m_pObject = 0;
    NiD3DPass* v81 = 0;
    {
        NiPointer<NiD3DTextureStage> spstage;
        if (ShadowLightShader::spPass[346].m_pObject) {
            m_pObject = ShadowLightShader::spPass[346].m_pObject;
            v81 = ShadowLightShader::spPass[346].m_pObject;
            ++ShadowLightShader::spPass[346].m_pObject->m_uiRefCount;
        }
        if (m_pObject->m_uiStageCount < 6) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
        }
        NiD3DVertexShader* v2 = m_pObject->m_spVertexShader.m_pObject;
        NiD3DVertexShader* v3 = ShadowLightShader::spVertexShaders[93];
        if (v2 != ShadowLightShader::spVertexShaders[93]) {
            if (v2) {
                NiDecRefCount(v2);
            }
            m_pObject->m_spVertexShader.m_pObject = v3;
            if (v3) {
                NiIncRefCount(v3);
            }
        }
        NiD3DPixelShader* v4 = ShadowLightShader::spPixelShaders[95];
        m_pObject->m_spPixelShader = v4;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        ShadowLightShader::ulVertexConstantFlags[346] = 1152;
        ShadowLightShader::ulPixelConstantFlags[346] = 40;
        if (m_pObject != ShadowLightShader::spPass[347].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[347].m_pObject;
            v81 = ShadowLightShader::spPass[347].m_pObject;
            if (ShadowLightShader::spPass[347].m_pObject) {
                ++ShadowLightShader::spPass[347].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 6) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
        }
        NiD3DVertexShader* v7 = ShadowLightShader::spVertexShaders[93];
        m_pObject->m_spVertexShader = v7;
        NiD3DPixelShader* v9 = ShadowLightShader::spPixelShaders[96];
        m_pObject->m_spPixelShader = v9;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        ShadowLightShader::ulVertexConstantFlags[347] = 1152;
        ShadowLightShader::ulPixelConstantFlags[347] = 40;
        if (m_pObject != ShadowLightShader::spPass[349].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[349].m_pObject;
            v81 = ShadowLightShader::spPass[349].m_pObject;
            if (ShadowLightShader::spPass[349].m_pObject) {
                ++ShadowLightShader::spPass[349].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 6) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
        }
        NiD3DVertexShader* v11 = ShadowLightShader::spVertexShaders[94];
        m_pObject->m_spVertexShader = v11;
        NiD3DPixelShader* v13 = ShadowLightShader::spPixelShaders[95];
        m_pObject->m_spPixelShader = v13;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        ShadowLightShader::ulVertexConstantFlags[349] = 3200;
        ShadowLightShader::ulPixelConstantFlags[349] = 40;
        if (m_pObject != ShadowLightShader::spPass[350].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[350].m_pObject;
            v81 = ShadowLightShader::spPass[350].m_pObject;
            if (ShadowLightShader::spPass[350].m_pObject) {
                ++ShadowLightShader::spPass[350].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 6) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
        }
        NiD3DVertexShader* v15 = ShadowLightShader::spVertexShaders[94];
        m_pObject->m_spVertexShader = v15;
        NiD3DPixelShader* v17 = ShadowLightShader::spPixelShaders[96];
        m_pObject->m_spPixelShader = v17;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        ShadowLightShader::ulVertexConstantFlags[350] = 3200;
        ShadowLightShader::ulPixelConstantFlags[350] = 40;
        if (m_pObject != ShadowLightShader::spPass[352].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[352].m_pObject;
            v81 = ShadowLightShader::spPass[352].m_pObject;
            if (ShadowLightShader::spPass[352].m_pObject) {
                ++ShadowLightShader::spPass[352].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 6) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
        }
        NiD3DVertexShader* v19 = ShadowLightShader::spVertexShaders[95];
        m_pObject->m_spVertexShader = v19;
        NiD3DPixelShader* v21 = ShadowLightShader::spPixelShaders[97];
        m_pObject->m_spPixelShader = v21;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        ShadowLightShader::ulVertexConstantFlags[352] = 1264;
        ShadowLightShader::ulPixelConstantFlags[352] = 40;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[352] = 96;
        if (m_pObject != ShadowLightShader::spPass[353].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[353].m_pObject;
            v81 = ShadowLightShader::spPass[353].m_pObject;
            if (ShadowLightShader::spPass[353].m_pObject) {
                ++ShadowLightShader::spPass[353].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 6) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
        }
        NiD3DVertexShader* v23 = ShadowLightShader::spVertexShaders[95];
        m_pObject->m_spVertexShader = v23;
        NiD3DPixelShader* v25 = ShadowLightShader::spPixelShaders[98];
        m_pObject->m_spPixelShader = v25;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        ShadowLightShader::ulVertexConstantFlags[353] = 1264;
        ShadowLightShader::ulPixelConstantFlags[353] = 40;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[352] = 96;
        if (m_pObject != ShadowLightShader::spPass[355].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[355].m_pObject;
            v81 = ShadowLightShader::spPass[355].m_pObject;
            if (ShadowLightShader::spPass[355].m_pObject) {
                ++ShadowLightShader::spPass[355].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 6) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
        }
        NiD3DVertexShader* v27 = ShadowLightShader::spVertexShaders[96];
        m_pObject->m_spVertexShader = v27;
        NiD3DPixelShader* v29 = ShadowLightShader::spPixelShaders[97];
        m_pObject->m_spPixelShader = v29;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        ShadowLightShader::ulVertexConstantFlags[355] = 3312;
        ShadowLightShader::ulPixelConstantFlags[355] = 40;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[355] = 96;
        if (m_pObject != ShadowLightShader::spPass[356].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[356].m_pObject;
            v81 = ShadowLightShader::spPass[356].m_pObject;
            if (ShadowLightShader::spPass[356].m_pObject) {
                ++ShadowLightShader::spPass[356].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 6) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
        }
        NiD3DVertexShader* v31 = ShadowLightShader::spVertexShaders[96];
        m_pObject->m_spVertexShader = v31;
        NiD3DPixelShader* v33 = ShadowLightShader::spPixelShaders[98];
        m_pObject->m_spPixelShader = v33;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        ShadowLightShader::ulVertexConstantFlags[356] = 3312;
        ShadowLightShader::ulPixelConstantFlags[356] = 40;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[355] = 96;
        if (m_pObject != ShadowLightShader::spPass[358].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[358].m_pObject;
            v81 = ShadowLightShader::spPass[358].m_pObject;
            if (ShadowLightShader::spPass[358].m_pObject) {
                ++ShadowLightShader::spPass[358].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 6) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
        }
        NiD3DVertexShader* v35 = ShadowLightShader::spVertexShaders[97];
        m_pObject->m_spVertexShader = v35;
        NiD3DPixelShader* v37 = ShadowLightShader::spPixelShaders[99];
        m_pObject->m_spPixelShader = v37;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        ShadowLightShader::ulVertexConstantFlags[358] = 1152;
        ShadowLightShader::ulPixelConstantFlags[358] = 40;
        if (m_pObject != ShadowLightShader::spPass[359].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[359].m_pObject;
            v81 = ShadowLightShader::spPass[359].m_pObject;
            if (ShadowLightShader::spPass[359].m_pObject) {
                ++ShadowLightShader::spPass[359].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 6) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
        }
        NiD3DVertexShader* v39 = ShadowLightShader::spVertexShaders[97];
        m_pObject->m_spVertexShader = v39;
        NiD3DPixelShader* v41 = ShadowLightShader::spPixelShaders[100];
        m_pObject->m_spPixelShader = v41;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        ShadowLightShader::ulVertexConstantFlags[359] = 1152;
        ShadowLightShader::ulPixelConstantFlags[359] = 40;
        if (m_pObject != ShadowLightShader::spPass[361].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[361].m_pObject;
            v81 = ShadowLightShader::spPass[361].m_pObject;
            if (ShadowLightShader::spPass[361].m_pObject) {
                ++ShadowLightShader::spPass[361].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 6) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
        }
        NiD3DVertexShader* v43 = ShadowLightShader::spVertexShaders[98];
        m_pObject->m_spVertexShader = v43;
        NiD3DPixelShader* v45 = ShadowLightShader::spPixelShaders[99];
        m_pObject->m_spPixelShader = v45;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        ShadowLightShader::ulVertexConstantFlags[361] = 3200;
        ShadowLightShader::ulPixelConstantFlags[361] = 40;
        if (m_pObject != ShadowLightShader::spPass[362].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[362].m_pObject;
            v81 = ShadowLightShader::spPass[362].m_pObject;
            if (ShadowLightShader::spPass[362].m_pObject) {
                ++ShadowLightShader::spPass[362].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 6) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
        }
        NiD3DVertexShader* v47 = ShadowLightShader::spVertexShaders[98];
        m_pObject->m_spVertexShader = v47;
        NiD3DPixelShader* v49 = ShadowLightShader::spPixelShaders[100];
        m_pObject->m_spPixelShader = v49;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        ShadowLightShader::ulVertexConstantFlags[362] = 3200;
        ShadowLightShader::ulPixelConstantFlags[362] = 40;
        if (m_pObject != ShadowLightShader::spPass[364].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[364].m_pObject;
            v81 = ShadowLightShader::spPass[364].m_pObject;
            if (ShadowLightShader::spPass[364].m_pObject) {
                ++ShadowLightShader::spPass[364].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 6) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
        }
        NiD3DVertexShader* v51 = ShadowLightShader::spVertexShaders[99];
        m_pObject->m_spVertexShader = v51;
        NiD3DPixelShader* v53 = ShadowLightShader::spPixelShaders[101];
        m_pObject->m_spPixelShader = v53;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        ShadowLightShader::ulVertexConstantFlags[364] = 1152;
        ShadowLightShader::ulPixelConstantFlags[364] = 40;
        if (m_pObject != ShadowLightShader::spPass[365].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[365].m_pObject;
            v81 = ShadowLightShader::spPass[365].m_pObject;
            if (ShadowLightShader::spPass[365].m_pObject) {
                ++ShadowLightShader::spPass[365].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 6) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
        }
        NiD3DVertexShader* v55 = ShadowLightShader::spVertexShaders[99];
        m_pObject->m_spVertexShader = v55;
        NiD3DPixelShader* v57 = ShadowLightShader::spPixelShaders[102];
        m_pObject->m_spPixelShader = v57;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        ShadowLightShader::ulVertexConstantFlags[365] = 1152;
        ShadowLightShader::ulPixelConstantFlags[365] = 40;
        if (m_pObject != ShadowLightShader::spPass[367].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[367].m_pObject;
            v81 = ShadowLightShader::spPass[367].m_pObject;
            if (ShadowLightShader::spPass[367].m_pObject) {
                ++ShadowLightShader::spPass[367].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 6) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
        }
        NiD3DVertexShader* v59 = ShadowLightShader::spVertexShaders[100];
        m_pObject->m_spVertexShader = v59;
        NiD3DPixelShader* v61 = ShadowLightShader::spPixelShaders[101];
        m_pObject->m_spPixelShader = v61;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        ShadowLightShader::ulVertexConstantFlags[367] = 3200;
        ShadowLightShader::ulPixelConstantFlags[367] = 40;
        if (m_pObject != ShadowLightShader::spPass[368].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[368].m_pObject;
            v81 = ShadowLightShader::spPass[368].m_pObject;
            if (ShadowLightShader::spPass[368].m_pObject) {
                ++ShadowLightShader::spPass[368].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 6) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
        }
        NiD3DVertexShader* v63 = ShadowLightShader::spVertexShaders[100];
        m_pObject->m_spVertexShader = v63;
        NiD3DPixelShader* v65 = ShadowLightShader::spPixelShaders[102];
        m_pObject->m_spPixelShader = v65;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        ShadowLightShader::ulVertexConstantFlags[368] = 3200;
        ShadowLightShader::ulPixelConstantFlags[368] = 40;
        if (m_pObject != ShadowLightShader::spPass[370].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[370].m_pObject;
            v81 = ShadowLightShader::spPass[370].m_pObject;
            if (ShadowLightShader::spPass[370].m_pObject) {
                ++ShadowLightShader::spPass[370].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 6) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
        }
        NiD3DVertexShader* v67 = ShadowLightShader::spVertexShaders[101];
        m_pObject->m_spVertexShader = v67;
        NiD3DPixelShader* v69 = ShadowLightShader::spPixelShaders[103];
        m_pObject->m_spPixelShader = v69;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        ShadowLightShader::ulVertexConstantFlags[370] = 1152;
        ShadowLightShader::ulPixelConstantFlags[370] = 40;
        if (m_pObject != ShadowLightShader::spPass[371].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[371].m_pObject;
            v81 = ShadowLightShader::spPass[371].m_pObject;
            if (ShadowLightShader::spPass[371].m_pObject) {
                ++ShadowLightShader::spPass[371].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 6) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
        }
        NiD3DVertexShader* v71 = ShadowLightShader::spVertexShaders[101];
        m_pObject->m_spVertexShader = v71;
        NiD3DPixelShader* v73 = ShadowLightShader::spPixelShaders[104];
        m_pObject->m_spPixelShader = v73;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        ShadowLightShader::ulVertexConstantFlags[371] = 1152;
        ShadowLightShader::ulPixelConstantFlags[371] = 40;
        if (m_pObject != ShadowLightShader::spPass[373].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[373].m_pObject;
            v81 = ShadowLightShader::spPass[373].m_pObject;
            if (ShadowLightShader::spPass[373].m_pObject) {
                ++ShadowLightShader::spPass[373].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 6) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
        }
        NiD3DVertexShader* v75 = ShadowLightShader::spVertexShaders[102];
        m_pObject->m_spVertexShader = v75;
        NiD3DPixelShader* v77 = ShadowLightShader::spPixelShaders[103];
        m_pObject->m_spPixelShader = v77;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        ShadowLightShader::ulVertexConstantFlags[373] = 3200;
        ShadowLightShader::ulPixelConstantFlags[373] = 40;
        if (m_pObject != ShadowLightShader::spPass[374].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[374].m_pObject;
            v81 = ShadowLightShader::spPass[374].m_pObject;
            if (ShadowLightShader::spPass[374].m_pObject) {
                ++ShadowLightShader::spPass[374].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 6) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[102]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[104]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        ShadowLightShader::ulVertexConstantFlags[374] = 3200;
        ShadowLightShader::ulPixelConstantFlags[374] = 40;
    }
    m_pObject->DecRefCount();
}

void ShadowLightShader::PresetEnvMapPasses_2x() {
    NiD3DPass* m_pObject = 0;
    NiD3DPass* v21 = 0;
    {
        NiPointer<NiD3DTextureStage> a1;
        if (ShadowLightShader::spPass[584].m_pObject) {
            m_pObject = ShadowLightShader::spPass[584].m_pObject;
            v21 = ShadowLightShader::spPass[BSShaderManager::BSSM_2x_ENVMAP].m_pObject;
            ++ShadowLightShader::spPass[BSShaderManager::BSSM_2x_ENVMAP].m_pObject->m_uiRefCount;
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v2 = m_pObject->m_spVertexShader.m_pObject;
        NiD3DVertexShader* v3 = ShadowLightShader::spVertexShaders[103];
        if (v2 != ShadowLightShader::spVertexShaders[103]) {
            if (v2) {
                NiDecRefCount(v2);
            }
            m_pObject->m_spVertexShader.m_pObject = v3;
            if (v3) {
                NiIncRefCount(v3);
            }
        }
        NiD3DPixelShader* v4 = ShadowLightShader::spPixelShaders[105];
        m_pObject->m_spPixelShader = v4;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 3u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
        ShadowLightShader::ulVertexConstantFlags[BSShaderManager::BSSM_2x_ENVMAP] = 1796;
        ShadowLightShader::ulPixelConstantFlags[BSShaderManager::BSSM_2x_ENVMAP] = 36;
        if (m_pObject != ShadowLightShader::spPass[585].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[585].m_pObject;
            v21 = ShadowLightShader::spPass[585].m_pObject;
            if (ShadowLightShader::spPass[585].m_pObject) {
                ++ShadowLightShader::spPass[585].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v7 = ShadowLightShader::spVertexShaders[104];
        m_pObject->m_spVertexShader = v7;
        NiD3DPixelShader* v9 = ShadowLightShader::spPixelShaders[105];
        m_pObject->m_spPixelShader = v9;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 3u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
        ShadowLightShader::ulVertexConstantFlags[BSShaderManager::BSSM_2x_ENVMAP_S] = 0x2F00;
        ShadowLightShader::ulPixelConstantFlags[BSShaderManager::BSSM_2x_ENVMAP_S] = 36;
        if (m_pObject != ShadowLightShader::spPass[586].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[586].m_pObject;
            v21 = ShadowLightShader::spPass[586].m_pObject;
            if (ShadowLightShader::spPass[586].m_pObject) {
                ++ShadowLightShader::spPass[586].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v11 = ShadowLightShader::spVertexShaders[103];
        m_pObject->m_spVertexShader = v11;
        NiD3DPixelShader* v13 = ShadowLightShader::spPixelShaders[106];
        m_pObject->m_spPixelShader = v13;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 3u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
        ShadowLightShader::ulVertexConstantFlags[BSShaderManager::BSSM_2x_ENVMAP_W] = 1796;
        ShadowLightShader::ulPixelConstantFlags[BSShaderManager::BSSM_2x_ENVMAP_W] = 36;
        if (m_pObject != ShadowLightShader::spPass[587].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[587].m_pObject;
            v21 = ShadowLightShader::spPass[587].m_pObject;
            if (ShadowLightShader::spPass[587].m_pObject) {
                ++ShadowLightShader::spPass[587].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, 1);
            a1.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::spEyeReflectionCubeMap.m_pObject;
            a1.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v15 = ShadowLightShader::spVertexShaders[105];
        m_pObject->m_spVertexShader = v15;
        NiD3DPixelShader* v17 = ShadowLightShader::spPixelShaders[107];
        m_pObject->m_spPixelShader = v17;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 3u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
        ShadowLightShader::ulVertexConstantFlags[BSShaderManager::BSSM_2x_ENVMAP_EYE] = 0x40704;
        ShadowLightShader::ulPixelConstantFlags[BSShaderManager::BSSM_2x_ENVMAP_EYE] = 44;
    }
    m_pObject->DecRefCount();
}

void ShadowLightShader::PresetLandscape_2x() {
    NiD3DPass* m_pObject = 0;
    NiD3DPass* v201 = 0;
    {
        NiPointer<NiD3DTextureStage> spstage;
        if (ShadowLightShader::spPass[147].m_pObject) {
            m_pObject = ShadowLightShader::spPass[147].m_pObject;
            v201 = ShadowLightShader::spPass[147].m_pObject;
            ++ShadowLightShader::spPass[147].m_pObject->m_uiRefCount;
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        NiD3DVertexShader* v2 = m_pObject->m_spVertexShader.m_pObject;
        NiD3DVertexShader* v3 = ShadowLightShader::spVertexShaders[131];
        if (v2 != ShadowLightShader::spVertexShaders[131]) {
            if (v2) {
                NiDecRefCount(v2);
            }
            m_pObject->m_spVertexShader.m_pObject = v3;
            if (v3) {
                NiIncRefCount(v3);
            }
        }
        NiD3DPixelShader* v4 = ShadowLightShader::spPixelShaders[128];
        m_pObject->m_spPixelShader = v4;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[BSShaderManager::BSSM_LANDAD] = 128;
        ShadowLightShader::ulPixelConstantFlags[BSShaderManager::BSSM_LANDAD] = 12;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[147] = 128;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[147] = 4;
        if (m_pObject != ShadowLightShader::spPass[498].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[498].m_pObject;
            v201 = ShadowLightShader::spPass[498].m_pObject;
            if (ShadowLightShader::spPass[498].m_pObject) {
                ++ShadowLightShader::spPass[498].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        NiD3DVertexShader* v7 = ShadowLightShader::spVertexShaders[132];
        m_pObject->m_spVertexShader = v7;
        NiD3DPixelShader* v9 = ShadowLightShader::spPixelShaders[129];
        m_pObject->m_spPixelShader = v9;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(25u, 5u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(24u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
        ShadowLightShader::ulVertexConstantFlags[498] = 128;
        ShadowLightShader::ulPixelConstantFlags[498] = 12;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[498] = 128;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[498] = 4;
        if (m_pObject != ShadowLightShader::spPass[560].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[560].m_pObject;
            v201 = ShadowLightShader::spPass[560].m_pObject;
            if (ShadowLightShader::spPass[560].m_pObject) {
                ++ShadowLightShader::spPass[560].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            spstage.m_pObject->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            spstage.m_pObject->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            NiPointer<NiSourceTexture>* v11 = &BSShaderManager::pNoiseMap;
            if (!BSShaderManager::bUseNoiseMap) {
                v11 = &BSShaderManager::pEmptyNoiseMap;
            }
            spstage.m_pObject->m_spTexture = (NiTexture*)v11->m_pObject;
            spstage.m_pObject->m_uiTextureFlags = 1;
            spstage.m_pObject->SetClampMode(NiTexturingProperty::WRAP_S_WRAP_T);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        NiD3DVertexShader* v12 = ShadowLightShader::spVertexShaders[133];
        m_pObject->m_spVertexShader = v12;
        NiD3DPixelShader* v14 = ShadowLightShader::spPixelShaders[130];
        m_pObject->m_spPixelShader = v14;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(25u, 5u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(24u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
        ShadowLightShader::ulVertexConstantFlags[560] = 0x400380;
        ShadowLightShader::ulPixelConstantFlags[560] = 12;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[560] = 128;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[560] = 772;
        if (m_pObject != ShadowLightShader::spPass[497].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[497].m_pObject;
            v201 = ShadowLightShader::spPass[497].m_pObject;
            if (ShadowLightShader::spPass[497].m_pObject) {
                ++ShadowLightShader::spPass[497].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        NiD3DVertexShader* v16 = ShadowLightShader::spVertexShaders[134];
        m_pObject->m_spVertexShader = v16;
        NiD3DPixelShader* v18 = ShadowLightShader::spPixelShaders[131];
        m_pObject->m_spPixelShader = v18;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(25u, 5u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(24u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
        ShadowLightShader::ulVertexConstantFlags[497] = 0;
        ShadowLightShader::ulPixelConstantFlags[497] = 8;
        if (m_pObject != ShadowLightShader::spPass[500].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[500].m_pObject;
            v201 = ShadowLightShader::spPass[500].m_pObject;
            if (ShadowLightShader::spPass[500].m_pObject) {
                ++ShadowLightShader::spPass[500].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        NiD3DVertexShader* v20 = ShadowLightShader::spVertexShaders[134];
        m_pObject->m_spVertexShader = v20;
        NiD3DPixelShader* v22 = ShadowLightShader::spPixelShaders[132];
        m_pObject->m_spPixelShader = v22;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(25u, 5u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(24u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
        ShadowLightShader::ulVertexConstantFlags[500] = 0;
        ShadowLightShader::ulPixelConstantFlags[500] = 8;
        if (m_pObject != ShadowLightShader::spPass[495].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[495].m_pObject;
            v201 = ShadowLightShader::spPass[495].m_pObject;
            if (ShadowLightShader::spPass[495].m_pObject) {
                ++ShadowLightShader::spPass[495].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        NiD3DVertexShader* v24 = ShadowLightShader::spVertexShaders[135];
        m_pObject->m_spVertexShader = v24;
        NiD3DPixelShader* v26 = ShadowLightShader::spPixelShaders[133];
        m_pObject->m_spPixelShader = v26;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(25u, 5u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(24u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
        ShadowLightShader::ulVertexConstantFlags[495] = 128;
        ShadowLightShader::ulPixelConstantFlags[495] = 8;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[495] = 0;
        if (m_pObject != ShadowLightShader::spPass[501].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[501].m_pObject;
            v201 = ShadowLightShader::spPass[501].m_pObject;
            if (ShadowLightShader::spPass[501].m_pObject) {
                ++ShadowLightShader::spPass[501].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        NiD3DVertexShader* v28 = ShadowLightShader::spVertexShaders[136];
        m_pObject->m_spVertexShader = v28;
        NiD3DPixelShader* v31 = ShadowLightShader::spPixelShaders[134];
        m_pObject->m_spPixelShader = v31;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(25u, 5u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(24u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
        ShadowLightShader::ulVertexConstantFlags[501] = 128;
        ShadowLightShader::ulPixelConstantFlags[501] = 28;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[501] = 0;
        if (m_pObject != ShadowLightShader::spPass[148].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[148].m_pObject;
            v201 = ShadowLightShader::spPass[148].m_pObject;
            if (ShadowLightShader::spPass[148].m_pObject) {
                ++ShadowLightShader::spPass[148].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        NiD3DVertexShader* v33 = ShadowLightShader::spVertexShaders[106];
        m_pObject->m_spVertexShader = v33;
        NiD3DPixelShader* v35 = ShadowLightShader::spPixelShaders[108];
        m_pObject->m_spPixelShader = v35;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[148] = 240;
        ShadowLightShader::ulPixelConstantFlags[148] = 12;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[148] = 224;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[148] = 4;
        if (m_pObject != ShadowLightShader::spPass[503].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[503].m_pObject;
            v201 = ShadowLightShader::spPass[503].m_pObject;
            if (ShadowLightShader::spPass[503].m_pObject) {
                ++ShadowLightShader::spPass[503].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 14) {
            for (int32_t i = 0; i < 14; ++i) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, i, 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        NiD3DVertexShader* v38 = ShadowLightShader::spVertexShaders[153];
        m_pObject->m_spVertexShader = v38;
        NiD3DPixelShader* v40 = ShadowLightShader::spPixelShaders[140];
        m_pObject->m_spPixelShader = v40;
        ShadowLightShader::ulVertexConstantFlags[503] = 768;
        ShadowLightShader::ulPixelConstantFlags[503] = 172;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[503] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[503] = 132;
        if (m_pObject != ShadowLightShader::spPass[505].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[505].m_pObject;
            v201 = ShadowLightShader::spPass[505].m_pObject;
            if (ShadowLightShader::spPass[505].m_pObject) {
                ++ShadowLightShader::spPass[505].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 14) {
            for (int32_t i_2 = 0; i_2 < 14; ++i_2) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_2), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        NiD3DVertexShader* v43 = ShadowLightShader::spVertexShaders[153];
        m_pObject->m_spVertexShader = v43;
        NiD3DPixelShader* v45 = ShadowLightShader::spPixelShaders[142];
        m_pObject->m_spPixelShader = v45;
        ShadowLightShader::ulVertexConstantFlags[505] = 768;
        ShadowLightShader::ulPixelConstantFlags[505] = 172;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[505] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[505] = 132;
        if (m_pObject != ShadowLightShader::spPass[504].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[504].m_pObject;
            v201 = ShadowLightShader::spPass[504].m_pObject;
            if (ShadowLightShader::spPass[504].m_pObject) {
                ++ShadowLightShader::spPass[504].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 16) {
            for (int32_t i_3 = 0; i_3 < 16; ++i_3) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_3), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        NiD3DVertexShader* v48 = ShadowLightShader::spVertexShaders[154];
        m_pObject->m_spVertexShader = v48;
        NiD3DPixelShader* v50 = ShadowLightShader::spPixelShaders[141];
        m_pObject->m_spPixelShader = v50;
        ShadowLightShader::ulVertexConstantFlags[504] = 880;
        ShadowLightShader::ulPixelConstantFlags[504] = 172;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[504] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[504] = 132;
        if (m_pObject != ShadowLightShader::spPass[506].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[506].m_pObject;
            v201 = ShadowLightShader::spPass[506].m_pObject;
            if (ShadowLightShader::spPass[506].m_pObject) {
                ++ShadowLightShader::spPass[506].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 16) {
            for (int32_t i_4 = 0; i_4 < 16; ++i_4) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_4), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        NiD3DVertexShader* v53 = ShadowLightShader::spVertexShaders[154];
        m_pObject->m_spVertexShader = v53;
        NiD3DPixelShader* v55 = ShadowLightShader::spPixelShaders[143];
        m_pObject->m_spPixelShader = v55;
        ShadowLightShader::ulVertexConstantFlags[506] = 880;
        ShadowLightShader::ulPixelConstantFlags[506] = 172;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[506] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[506] = 132;
        if (m_pObject != ShadowLightShader::spPass[507].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[507].m_pObject;
            v201 = ShadowLightShader::spPass[507].m_pObject;
            if (ShadowLightShader::spPass[507].m_pObject) {
                ++ShadowLightShader::spPass[507].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 14) {
            for (int32_t i_5 = 0; i_5 < 14; ++i_5) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_5), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        NiD3DVertexShader* v58 = ShadowLightShader::spVertexShaders[153];
        m_pObject->m_spVertexShader = v58;
        NiD3DPixelShader* v60 = ShadowLightShader::spPixelShaders[144];
        m_pObject->m_spPixelShader = v60;
        ShadowLightShader::ulVertexConstantFlags[507] = 768;
        ShadowLightShader::ulPixelConstantFlags[507] = 4012;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[507] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[507] = 132;
        if (m_pObject != ShadowLightShader::spPass[509].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[509].m_pObject;
            v201 = ShadowLightShader::spPass[509].m_pObject;
            if (ShadowLightShader::spPass[509].m_pObject) {
                ++ShadowLightShader::spPass[509].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 14) {
            for (int32_t i_6 = 0; i_6 < 14; ++i_6) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_6), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        NiD3DVertexShader* v63 = ShadowLightShader::spVertexShaders[153];
        m_pObject->m_spVertexShader = v63;
        NiD3DPixelShader* v65 = ShadowLightShader::spPixelShaders[146];
        m_pObject->m_spPixelShader = v65;
        ShadowLightShader::ulVertexConstantFlags[509] = 768;
        ShadowLightShader::ulPixelConstantFlags[509] = 4012;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[509] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[509] = 132;
        if (m_pObject != ShadowLightShader::spPass[508].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[508].m_pObject;
            v201 = ShadowLightShader::spPass[508].m_pObject;
            if (ShadowLightShader::spPass[508].m_pObject) {
                ++ShadowLightShader::spPass[508].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 16) {
            for (int32_t i_7 = 0; i_7 < 16; ++i_7) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_7), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        NiD3DVertexShader* v68 = ShadowLightShader::spVertexShaders[154];
        m_pObject->m_spVertexShader = v68;
        NiD3DPixelShader* v70 = ShadowLightShader::spPixelShaders[145];
        m_pObject->m_spPixelShader = v70;
        ShadowLightShader::ulVertexConstantFlags[508] = 880;
        ShadowLightShader::ulPixelConstantFlags[508] = 1964;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[508] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[508] = 132;
        if (m_pObject != ShadowLightShader::spPass[510].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[510].m_pObject;
            v201 = ShadowLightShader::spPass[510].m_pObject;
            if (ShadowLightShader::spPass[510].m_pObject) {
                ++ShadowLightShader::spPass[510].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 16) {
            for (int32_t i_8 = 0; i_8 < 16; ++i_8) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_8), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        NiD3DVertexShader* v73 = ShadowLightShader::spVertexShaders[154];
        m_pObject->m_spVertexShader = v73;
        NiD3DPixelShader* v75 = ShadowLightShader::spPixelShaders[147];
        m_pObject->m_spPixelShader = v75;
        ShadowLightShader::ulVertexConstantFlags[510] = 880;
        ShadowLightShader::ulPixelConstantFlags[510] = 1964;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[510] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[510] = 132;
        if (m_pObject != ShadowLightShader::spPass[511].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[511].m_pObject;
            v201 = ShadowLightShader::spPass[511].m_pObject;
            if (ShadowLightShader::spPass[511].m_pObject) {
                ++ShadowLightShader::spPass[511].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 14) {
            for (int32_t i_9 = 0; i_9 < 14; ++i_9) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_9), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        NiD3DVertexShader* v78 = ShadowLightShader::spVertexShaders[153];
        m_pObject->m_spVertexShader = v78;
        NiD3DPixelShader* v80 = ShadowLightShader::spPixelShaders[148];
        m_pObject->m_spPixelShader = v80;
        ShadowLightShader::ulVertexConstantFlags[511] = 768;
        ShadowLightShader::ulPixelConstantFlags[511] = 172;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[511] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[511] = 132;
        if (m_pObject != ShadowLightShader::spPass[513].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[513].m_pObject;
            v201 = ShadowLightShader::spPass[513].m_pObject;
            if (ShadowLightShader::spPass[513].m_pObject) {
                ++ShadowLightShader::spPass[513].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 14) {
            for (int32_t i_10 = 0; i_10 < 14; ++i_10) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_10), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        NiD3DVertexShader* v83 = ShadowLightShader::spVertexShaders[153];
        m_pObject->m_spVertexShader = v83;
        NiD3DPixelShader* v85 = ShadowLightShader::spPixelShaders[150];
        m_pObject->m_spPixelShader = v85;
        ShadowLightShader::ulVertexConstantFlags[513] = 768;
        ShadowLightShader::ulPixelConstantFlags[513] = 172;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[513] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[513] = 132;
        if (m_pObject != ShadowLightShader::spPass[512].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[512].m_pObject;
            v201 = ShadowLightShader::spPass[512].m_pObject;
            if (ShadowLightShader::spPass[512].m_pObject) {
                ++ShadowLightShader::spPass[512].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 16) {
            for (int32_t i_11 = 0; i_11 < 16; ++i_11) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_11), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        NiD3DVertexShader* v88 = ShadowLightShader::spVertexShaders[154];
        m_pObject->m_spVertexShader = v88;
        NiD3DPixelShader* v90 = ShadowLightShader::spPixelShaders[149];
        m_pObject->m_spPixelShader = v90;
        ShadowLightShader::ulVertexConstantFlags[512] = 880;
        ShadowLightShader::ulPixelConstantFlags[512] = 172;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[512] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[512] = 132;
        if (m_pObject != ShadowLightShader::spPass[514].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[514].m_pObject;
            v201 = ShadowLightShader::spPass[514].m_pObject;
            if (ShadowLightShader::spPass[514].m_pObject) {
                ++ShadowLightShader::spPass[514].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 16) {
            for (int32_t i_12 = 0; i_12 < 16; ++i_12) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_12), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        NiD3DVertexShader* v93 = ShadowLightShader::spVertexShaders[154];
        m_pObject->m_spVertexShader = v93;
        NiD3DPixelShader* v95 = ShadowLightShader::spPixelShaders[151];
        m_pObject->m_spPixelShader = v95;
        ShadowLightShader::ulVertexConstantFlags[514] = 880;
        ShadowLightShader::ulPixelConstantFlags[514] = 172;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[514] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[514] = 132;
        if (m_pObject != ShadowLightShader::spPass[515].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[515].m_pObject;
            v201 = ShadowLightShader::spPass[515].m_pObject;
            if (ShadowLightShader::spPass[515].m_pObject) {
                ++ShadowLightShader::spPass[515].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 14) {
            for (int32_t i_13 = 0; i_13 < 14; ++i_13) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_13), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        NiD3DVertexShader* v98 = ShadowLightShader::spVertexShaders[153];
        m_pObject->m_spVertexShader = v98;
        NiD3DPixelShader* v100 = ShadowLightShader::spPixelShaders[152];
        m_pObject->m_spPixelShader = v100;
        ShadowLightShader::ulVertexConstantFlags[515] = 768;
        ShadowLightShader::ulPixelConstantFlags[515] = 4012;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[515] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[515] = 132;
        if (m_pObject != ShadowLightShader::spPass[517].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[517].m_pObject;
            v201 = ShadowLightShader::spPass[517].m_pObject;
            if (ShadowLightShader::spPass[517].m_pObject) {
                ++ShadowLightShader::spPass[517].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 14) {
            for (int32_t i_14 = 0; i_14 < 14; ++i_14) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_14), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        NiD3DVertexShader* v103 = ShadowLightShader::spVertexShaders[153];
        m_pObject->m_spVertexShader = v103;
        NiD3DPixelShader* v105 = ShadowLightShader::spPixelShaders[154];
        m_pObject->m_spPixelShader = v105;
        ShadowLightShader::ulVertexConstantFlags[517] = 768;
        ShadowLightShader::ulPixelConstantFlags[517] = 4012;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[517] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[517] = 132;
        if (m_pObject != ShadowLightShader::spPass[516].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[516].m_pObject;
            v201 = ShadowLightShader::spPass[516].m_pObject;
            if (ShadowLightShader::spPass[516].m_pObject) {
                ++ShadowLightShader::spPass[516].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 16) {
            for (int32_t i_15 = 0; i_15 < 16; ++i_15) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_15), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        NiD3DVertexShader* v108 = ShadowLightShader::spVertexShaders[154];
        m_pObject->m_spVertexShader = v108;
        NiD3DPixelShader* v110 = ShadowLightShader::spPixelShaders[153];
        m_pObject->m_spPixelShader = v110;
        ShadowLightShader::ulVertexConstantFlags[516] = 880;
        ShadowLightShader::ulPixelConstantFlags[516] = 1964;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[516] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[516] = 132;
        if (m_pObject != ShadowLightShader::spPass[518].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[518].m_pObject;
            v201 = ShadowLightShader::spPass[518].m_pObject;
            if (ShadowLightShader::spPass[518].m_pObject) {
                ++ShadowLightShader::spPass[518].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 16) {
            for (int32_t i_16 = 0; i_16 < 16; ++i_16) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_16), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        NiD3DVertexShader* v113 = ShadowLightShader::spVertexShaders[154];
        m_pObject->m_spVertexShader = v113;
        NiD3DPixelShader* v115 = ShadowLightShader::spPixelShaders[155];
        m_pObject->m_spPixelShader = v115;
        ShadowLightShader::ulVertexConstantFlags[518] = 880;
        ShadowLightShader::ulPixelConstantFlags[518] = 1964;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[518] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[518] = 132;
        if (m_pObject != ShadowLightShader::spPass[519].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[519].m_pObject;
            v201 = ShadowLightShader::spPass[519].m_pObject;
            if (ShadowLightShader::spPass[519].m_pObject) {
                ++ShadowLightShader::spPass[519].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 14) {
            for (int32_t i_17 = 0; i_17 < 14; ++i_17) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_17), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        NiD3DVertexShader* v118 = ShadowLightShader::spVertexShaders[153];
        m_pObject->m_spVertexShader = v118;
        NiD3DPixelShader* v120 = ShadowLightShader::spPixelShaders[156];
        m_pObject->m_spPixelShader = v120;
        ShadowLightShader::ulVertexConstantFlags[519] = 768;
        ShadowLightShader::ulPixelConstantFlags[519] = 172;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[519] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[519] = 132;
        if (m_pObject != ShadowLightShader::spPass[521].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[521].m_pObject;
            v201 = ShadowLightShader::spPass[521].m_pObject;
            if (ShadowLightShader::spPass[521].m_pObject) {
                ++ShadowLightShader::spPass[521].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 14) {
            for (int32_t i_18 = 0; i_18 < 14; ++i_18) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_18), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        NiD3DVertexShader* v123 = ShadowLightShader::spVertexShaders[153];
        m_pObject->m_spVertexShader = v123;
        NiD3DPixelShader* v125 = ShadowLightShader::spPixelShaders[158];
        m_pObject->m_spPixelShader = v125;
        ShadowLightShader::ulVertexConstantFlags[521] = 768;
        ShadowLightShader::ulPixelConstantFlags[521] = 172;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[521] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[521] = 132;
        if (m_pObject != ShadowLightShader::spPass[520].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[520].m_pObject;
            v201 = ShadowLightShader::spPass[520].m_pObject;
            if (ShadowLightShader::spPass[520].m_pObject) {
                ++ShadowLightShader::spPass[520].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 16) {
            for (int32_t i_19 = 0; i_19 < 16; ++i_19) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_19), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        NiD3DVertexShader* v128 = ShadowLightShader::spVertexShaders[154];
        m_pObject->m_spVertexShader = v128;
        NiD3DPixelShader* v130 = ShadowLightShader::spPixelShaders[157];
        m_pObject->m_spPixelShader = v130;
        ShadowLightShader::ulVertexConstantFlags[520] = 880;
        ShadowLightShader::ulPixelConstantFlags[520] = 172;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[520] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[520] = 132;
        if (m_pObject != ShadowLightShader::spPass[522].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[522].m_pObject;
            v201 = ShadowLightShader::spPass[522].m_pObject;
            if (ShadowLightShader::spPass[522].m_pObject) {
                ++ShadowLightShader::spPass[522].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 16) {
            for (int32_t i_20 = 0; i_20 < 16; ++i_20) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_20), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        NiD3DVertexShader* v133 = ShadowLightShader::spVertexShaders[154];
        m_pObject->m_spVertexShader = v133;
        NiD3DPixelShader* v135 = ShadowLightShader::spPixelShaders[159];
        m_pObject->m_spPixelShader = v135;
        ShadowLightShader::ulVertexConstantFlags[522] = 880;
        ShadowLightShader::ulPixelConstantFlags[522] = 172;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[522] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[522] = 132;
        if (m_pObject != ShadowLightShader::spPass[523].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[523].m_pObject;
            v201 = ShadowLightShader::spPass[523].m_pObject;
            if (ShadowLightShader::spPass[523].m_pObject) {
                ++ShadowLightShader::spPass[523].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 14) {
            for (int32_t i_21 = 0; i_21 < 14; ++i_21) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_21), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        NiD3DVertexShader* v138 = ShadowLightShader::spVertexShaders[153];
        m_pObject->m_spVertexShader = v138;
        NiD3DPixelShader* v140 = ShadowLightShader::spPixelShaders[160];
        m_pObject->m_spPixelShader = v140;
        ShadowLightShader::ulVertexConstantFlags[523] = 768;
        ShadowLightShader::ulPixelConstantFlags[523] = 4012;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[523] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[523] = 132;
        if (m_pObject != ShadowLightShader::spPass[525].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[525].m_pObject;
            v201 = ShadowLightShader::spPass[525].m_pObject;
            if (ShadowLightShader::spPass[525].m_pObject) {
                ++ShadowLightShader::spPass[525].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 14) {
            for (int32_t i_22 = 0; i_22 < 14; ++i_22) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_22), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        NiD3DVertexShader* v143 = ShadowLightShader::spVertexShaders[153];
        m_pObject->m_spVertexShader = v143;
        NiD3DPixelShader* v145 = ShadowLightShader::spPixelShaders[162];
        m_pObject->m_spPixelShader = v145;
        ShadowLightShader::ulVertexConstantFlags[525] = 768;
        ShadowLightShader::ulPixelConstantFlags[525] = 4012;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[525] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[525] = 132;
        if (m_pObject != ShadowLightShader::spPass[524].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[524].m_pObject;
            v201 = ShadowLightShader::spPass[524].m_pObject;
            if (ShadowLightShader::spPass[524].m_pObject) {
                ++ShadowLightShader::spPass[524].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 16) {
            for (int32_t i_23 = 0; i_23 < 16; ++i_23) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_23), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        NiD3DVertexShader* v148 = ShadowLightShader::spVertexShaders[154];
        m_pObject->m_spVertexShader = v148;
        NiD3DPixelShader* v150 = ShadowLightShader::spPixelShaders[161];
        m_pObject->m_spPixelShader = v150;
        ShadowLightShader::ulVertexConstantFlags[524] = 880;
        ShadowLightShader::ulPixelConstantFlags[524] = 1964;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[524] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[524] = 132;
        if (m_pObject != ShadowLightShader::spPass[526].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[526].m_pObject;
            v201 = ShadowLightShader::spPass[526].m_pObject;
            if (ShadowLightShader::spPass[526].m_pObject) {
                ++ShadowLightShader::spPass[526].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 16) {
            for (int32_t i_24 = 0; i_24 < 16; ++i_24) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_24), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        NiD3DVertexShader* v153 = ShadowLightShader::spVertexShaders[154];
        m_pObject->m_spVertexShader = v153;
        NiD3DPixelShader* v155 = ShadowLightShader::spPixelShaders[163];
        m_pObject->m_spPixelShader = v155;
        ShadowLightShader::ulVertexConstantFlags[526] = 880;
        ShadowLightShader::ulPixelConstantFlags[526] = 1964;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[526] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[526] = 132;
        if (m_pObject != ShadowLightShader::spPass[527].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[527].m_pObject;
            v201 = ShadowLightShader::spPass[527].m_pObject;
            if (ShadowLightShader::spPass[527].m_pObject) {
                ++ShadowLightShader::spPass[527].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 14) {
            for (int32_t i_25 = 0; i_25 < 14; ++i_25) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_25), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        NiD3DVertexShader* v158 = ShadowLightShader::spVertexShaders[153];
        m_pObject->m_spVertexShader = v158;
        NiD3DPixelShader* v160 = ShadowLightShader::spPixelShaders[164];
        m_pObject->m_spPixelShader = v160;
        ShadowLightShader::ulVertexConstantFlags[527] = 768;
        ShadowLightShader::ulPixelConstantFlags[527] = 172;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[527] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[527] = 132;
        if (m_pObject != ShadowLightShader::spPass[529].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[529].m_pObject;
            v201 = ShadowLightShader::spPass[529].m_pObject;
            if (ShadowLightShader::spPass[529].m_pObject) {
                ++ShadowLightShader::spPass[529].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 14) {
            for (int32_t i_26 = 0; i_26 < 14; ++i_26) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_26), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        NiD3DVertexShader* v163 = ShadowLightShader::spVertexShaders[153];
        m_pObject->m_spVertexShader = v163;
        NiD3DPixelShader* v165 = ShadowLightShader::spPixelShaders[166];
        m_pObject->m_spPixelShader = v165;
        ShadowLightShader::ulVertexConstantFlags[529] = 768;
        ShadowLightShader::ulPixelConstantFlags[529] = 172;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[529] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[529] = 132;
        if (m_pObject != ShadowLightShader::spPass[528].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[528].m_pObject;
            v201 = ShadowLightShader::spPass[528].m_pObject;
            if (ShadowLightShader::spPass[528].m_pObject) {
                ++ShadowLightShader::spPass[528].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 16) {
            for (int32_t i_27 = 0; i_27 < 16; ++i_27) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_27), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        NiD3DVertexShader* v168 = ShadowLightShader::spVertexShaders[154];
        m_pObject->m_spVertexShader = v168;
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[165]);
        ShadowLightShader::ulVertexConstantFlags[528] = 880;
        ShadowLightShader::ulPixelConstantFlags[528] = 172;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[528] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[528] = 132;
        if (m_pObject != ShadowLightShader::spPass[530].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[530].m_pObject;
            v201 = ShadowLightShader::spPass[530].m_pObject;
            if (ShadowLightShader::spPass[530].m_pObject) {
                ++ShadowLightShader::spPass[530].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 16) {
            for (int32_t i_28 = 0; i_28 < 16; ++i_28) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_28), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[154]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[167]);
        ShadowLightShader::ulVertexConstantFlags[530] = 880;
        ShadowLightShader::ulPixelConstantFlags[530] = 172;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[530] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[530] = 132;
        if (m_pObject != ShadowLightShader::spPass[531].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[531].m_pObject;
            v201 = ShadowLightShader::spPass[531].m_pObject;
            if (ShadowLightShader::spPass[531].m_pObject) {
                ++ShadowLightShader::spPass[531].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 14) {
            for (int32_t i_29 = 0; i_29 < 14; ++i_29) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_29), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[153]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[168]);
        ShadowLightShader::ulVertexConstantFlags[531] = 768;
        ShadowLightShader::ulPixelConstantFlags[531] = 4012;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[531] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[531] = 132;
        if (m_pObject != ShadowLightShader::spPass[533].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[533].m_pObject;
            v201 = ShadowLightShader::spPass[533].m_pObject;
            if (ShadowLightShader::spPass[533].m_pObject) {
                ++ShadowLightShader::spPass[533].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 14) {
            for (int32_t i_30 = 0; i_30 < 14; ++i_30) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_30), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[153]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[170]);
        ShadowLightShader::ulVertexConstantFlags[533] = 768;
        ShadowLightShader::ulPixelConstantFlags[533] = 4012;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[533] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[533] = 132;
        if (m_pObject != ShadowLightShader::spPass[532].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[532].m_pObject;
            v201 = ShadowLightShader::spPass[532].m_pObject;
            if (ShadowLightShader::spPass[532].m_pObject) {
                ++ShadowLightShader::spPass[532].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 16) {
            for (int32_t i_31 = 0; i_31 < 16; ++i_31) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_31), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[154]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[169]);
        ShadowLightShader::ulVertexConstantFlags[532] = 880;
        ShadowLightShader::ulPixelConstantFlags[532] = 1964;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[532] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[532] = 132;
        if (m_pObject != ShadowLightShader::spPass[534].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[534].m_pObject;
            v201 = ShadowLightShader::spPass[534].m_pObject;
            if (ShadowLightShader::spPass[534].m_pObject) {
                ++ShadowLightShader::spPass[534].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 16) {
            for (int32_t i_32 = 0; i_32 < 16; ++i_32) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_32), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[154]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[171]);
        ShadowLightShader::ulVertexConstantFlags[534] = 880;
        ShadowLightShader::ulPixelConstantFlags[534] = 1964;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[534] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[534] = 132;
        if (m_pObject != ShadowLightShader::spPass[535].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[535].m_pObject;
            v201 = ShadowLightShader::spPass[535].m_pObject;
            if (ShadowLightShader::spPass[535].m_pObject) {
                ++ShadowLightShader::spPass[535].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 14) {
            for (int32_t i_33 = 0; i_33 < 14; ++i_33) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_33), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[153]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[172]);
        ShadowLightShader::ulVertexConstantFlags[535] = 768;
        ShadowLightShader::ulPixelConstantFlags[535] = 172;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[535] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[535] = 132;
        if (m_pObject != ShadowLightShader::spPass[537].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[537].m_pObject;
            v201 = ShadowLightShader::spPass[537].m_pObject;
            if (ShadowLightShader::spPass[537].m_pObject) {
                ++ShadowLightShader::spPass[537].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 14) {
            for (int32_t i_34 = 0; i_34 < 14; ++i_34) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_34), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[153]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[174]);
        ShadowLightShader::ulVertexConstantFlags[537] = 768;
        ShadowLightShader::ulPixelConstantFlags[537] = 172;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[537] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[537] = 132;
        if (m_pObject != ShadowLightShader::spPass[536].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[536].m_pObject;
            v201 = ShadowLightShader::spPass[536].m_pObject;
            if (ShadowLightShader::spPass[536].m_pObject) {
                ++ShadowLightShader::spPass[536].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 16) {
            for (int32_t i_35 = 0; i_35 < 16; ++i_35) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_35), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[154]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[173]);
        ShadowLightShader::ulVertexConstantFlags[536] = 880;
        ShadowLightShader::ulPixelConstantFlags[536] = 172;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[536] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[536] = 132;
        if (m_pObject != ShadowLightShader::spPass[538].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[538].m_pObject;
            v201 = ShadowLightShader::spPass[538].m_pObject;
            if (ShadowLightShader::spPass[538].m_pObject) {
                ++ShadowLightShader::spPass[538].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 16) {
            for (int32_t i_36 = 0; i_36 < 16; ++i_36) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_36), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[154]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[175]);
        ShadowLightShader::ulVertexConstantFlags[538] = 880;
        ShadowLightShader::ulPixelConstantFlags[538] = 172;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[538] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[538] = 132;
        if (m_pObject != ShadowLightShader::spPass[539].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[539].m_pObject;
            v201 = ShadowLightShader::spPass[539].m_pObject;
            if (ShadowLightShader::spPass[539].m_pObject) {
                ++ShadowLightShader::spPass[539].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 14) {
            for (int32_t i_37 = 0; i_37 < 14; ++i_37) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_37), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[153]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[176]);
        ShadowLightShader::ulVertexConstantFlags[539] = 768;
        ShadowLightShader::ulPixelConstantFlags[539] = 4012;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[539] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[539] = 132;
        if (m_pObject != ShadowLightShader::spPass[541].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[541].m_pObject;
            v201 = ShadowLightShader::spPass[541].m_pObject;
            if (ShadowLightShader::spPass[541].m_pObject) {
                ++ShadowLightShader::spPass[541].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 14) {
            for (int32_t i_38 = 0; i_38 < 14; ++i_38) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_38), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[153]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[178]);
        ShadowLightShader::ulVertexConstantFlags[541] = 768;
        ShadowLightShader::ulPixelConstantFlags[541] = 4012;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[541] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[541] = 132;
        if (m_pObject != ShadowLightShader::spPass[540].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[540].m_pObject;
            v201 = ShadowLightShader::spPass[540].m_pObject;
            if (ShadowLightShader::spPass[540].m_pObject) {
                ++ShadowLightShader::spPass[540].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 16) {
            for (int32_t i_39 = 0; i_39 < 16; ++i_39) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_39), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[154]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[177]);
        ShadowLightShader::ulVertexConstantFlags[540] = 880;
        ShadowLightShader::ulPixelConstantFlags[540] = 1964;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[540] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[540] = 132;
        if (m_pObject != ShadowLightShader::spPass[542].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[542].m_pObject;
            v201 = ShadowLightShader::spPass[542].m_pObject;
            if (ShadowLightShader::spPass[542].m_pObject) {
                ++ShadowLightShader::spPass[542].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 16) {
            for (int32_t i_40 = 0; i_40 < 16; ++i_40) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_40), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[154]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[179]);
        ShadowLightShader::ulVertexConstantFlags[542] = 880;
        ShadowLightShader::ulPixelConstantFlags[542] = 1964;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[542] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[542] = 132;
        if (m_pObject != ShadowLightShader::spPass[543].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[543].m_pObject;
            v201 = ShadowLightShader::spPass[543].m_pObject;
            if (ShadowLightShader::spPass[543].m_pObject) {
                ++ShadowLightShader::spPass[543].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 14) {
            for (int32_t i_41 = 0; i_41 < 14; ++i_41) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_41), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[153]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[180]);
        ShadowLightShader::ulVertexConstantFlags[543] = 768;
        ShadowLightShader::ulPixelConstantFlags[543] = 172;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[543] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[543] = 132;
        if (m_pObject != ShadowLightShader::spPass[545].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[545].m_pObject;
            v201 = ShadowLightShader::spPass[545].m_pObject;
            if (ShadowLightShader::spPass[545].m_pObject) {
                ++ShadowLightShader::spPass[545].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 14) {
            for (int32_t i_42 = 0; i_42 < 14; ++i_42) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_42), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[153]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[182]);
        ShadowLightShader::ulVertexConstantFlags[545] = 768;
        ShadowLightShader::ulPixelConstantFlags[545] = 172;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[545] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[545] = 132;
        if (m_pObject != ShadowLightShader::spPass[544].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[544].m_pObject;
            v201 = ShadowLightShader::spPass[544].m_pObject;
            if (ShadowLightShader::spPass[544].m_pObject) {
                ++ShadowLightShader::spPass[544].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 16) {
            for (int32_t i_43 = 0; i_43 < 16; ++i_43) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_43), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[154]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[181]);
        ShadowLightShader::ulVertexConstantFlags[544] = 880;
        ShadowLightShader::ulPixelConstantFlags[544] = 172;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[544] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[544] = 132;
        if (m_pObject != ShadowLightShader::spPass[546].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[546].m_pObject;
            v201 = ShadowLightShader::spPass[546].m_pObject;
            if (ShadowLightShader::spPass[546].m_pObject) {
                ++ShadowLightShader::spPass[546].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 16) {
            for (int32_t i_44 = 0; i_44 < 16; ++i_44) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_44), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[154]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[183]);
        ShadowLightShader::ulVertexConstantFlags[546] = 880;
        ShadowLightShader::ulPixelConstantFlags[546] = 172;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[546] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[546] = 132;
        if (m_pObject != ShadowLightShader::spPass[547].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[547].m_pObject;
            v201 = ShadowLightShader::spPass[547].m_pObject;
            if (ShadowLightShader::spPass[547].m_pObject) {
                ++ShadowLightShader::spPass[547].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 14) {
            for (int32_t i_45 = 0; i_45 < 14; ++i_45) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_45), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[153]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[184]);
        ShadowLightShader::ulVertexConstantFlags[547] = 768;
        ShadowLightShader::ulPixelConstantFlags[547] = 4012;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[547] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[547] = 132;
        if (m_pObject != ShadowLightShader::spPass[549].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[549].m_pObject;
            v201 = ShadowLightShader::spPass[549].m_pObject;
            if (ShadowLightShader::spPass[549].m_pObject) {
                ++ShadowLightShader::spPass[549].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 14) {
            for (int32_t i_46 = 0; i_46 < 14; ++i_46) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_46), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[153]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[186]);
        ShadowLightShader::ulVertexConstantFlags[549] = 768;
        ShadowLightShader::ulPixelConstantFlags[549] = 4012;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[549] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[549] = 132;
        if (m_pObject != ShadowLightShader::spPass[548].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[548].m_pObject;
            v201 = ShadowLightShader::spPass[548].m_pObject;
            if (ShadowLightShader::spPass[548].m_pObject) {
                ++ShadowLightShader::spPass[548].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 16) {
            for (int32_t i_47 = 0; i_47 < 16; ++i_47) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_47), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[154]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[185]);
        ShadowLightShader::ulVertexConstantFlags[548] = 880;
        ShadowLightShader::ulPixelConstantFlags[548] = 1964;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[548] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[548] = 132;
        if (m_pObject != ShadowLightShader::spPass[548].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[548].m_pObject;
            v201 = ShadowLightShader::spPass[548].m_pObject;
            if (ShadowLightShader::spPass[548].m_pObject) {
                ++ShadowLightShader::spPass[548].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 16) {
            for (int32_t i_48 = 0; i_48 < 16; ++i_48) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_48), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[154]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[187]);
        ShadowLightShader::ulVertexConstantFlags[550] = 880;
        ShadowLightShader::ulPixelConstantFlags[550] = 1964;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[550] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[550] = 132;
        if (m_pObject != ShadowLightShader::spPass[551].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[551].m_pObject;
            v201 = ShadowLightShader::spPass[551].m_pObject;
            if (ShadowLightShader::spPass[551].m_pObject) {
                ++ShadowLightShader::spPass[551].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 14) {
            for (int32_t i_49 = 0; i_49 < 14; ++i_49) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_49), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[153]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[188]);
        ShadowLightShader::ulVertexConstantFlags[551] = 768;
        ShadowLightShader::ulPixelConstantFlags[551] = 172;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[551] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[551] = 132;
        if (m_pObject != ShadowLightShader::spPass[553].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[553].m_pObject;
            v201 = ShadowLightShader::spPass[553].m_pObject;
            if (ShadowLightShader::spPass[553].m_pObject) {
                ++ShadowLightShader::spPass[553].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 14) {
            for (int32_t i_50 = 0; i_50 < 14; ++i_50) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_50), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[153]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[190]);
        ShadowLightShader::ulVertexConstantFlags[553] = 768;
        ShadowLightShader::ulPixelConstantFlags[553] = 172;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[553] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[553] = 132;
        if (m_pObject != ShadowLightShader::spPass[552].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[552].m_pObject;
            v201 = ShadowLightShader::spPass[552].m_pObject;
            if (ShadowLightShader::spPass[552].m_pObject) {
                ++ShadowLightShader::spPass[552].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 16) {
            for (int32_t i_51 = 0; i_51 < 16; ++i_51) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_51), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[154]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[189]);
        ShadowLightShader::ulVertexConstantFlags[552] = 880;
        ShadowLightShader::ulPixelConstantFlags[552] = 172;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[552] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[552] = 132;
        if (m_pObject != ShadowLightShader::spPass[554].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[554].m_pObject;
            v201 = ShadowLightShader::spPass[554].m_pObject;
            if (ShadowLightShader::spPass[554].m_pObject) {
                ++ShadowLightShader::spPass[554].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 16) {
            for (int32_t i_52 = 0; i_52 < 16; ++i_52) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_52), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[154]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[191]);
        ShadowLightShader::ulVertexConstantFlags[554] = 880;
        ShadowLightShader::ulPixelConstantFlags[554] = 172;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[554] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[554] = 132;
        if (m_pObject != ShadowLightShader::spPass[555].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[555].m_pObject;
            v201 = ShadowLightShader::spPass[555].m_pObject;
            if (ShadowLightShader::spPass[555].m_pObject) {
                ++ShadowLightShader::spPass[555].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 14) {
            for (int32_t i_53 = 0; i_53 < 14; ++i_53) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_53), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[153]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[192]);
        ShadowLightShader::ulVertexConstantFlags[555] = 768;
        ShadowLightShader::ulPixelConstantFlags[555] = 4012;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[555] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[555] = 132;
        if (m_pObject != ShadowLightShader::spPass[557].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[557].m_pObject;
            v201 = ShadowLightShader::spPass[557].m_pObject;
            if (ShadowLightShader::spPass[557].m_pObject) {
                ++ShadowLightShader::spPass[557].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 14) {
            for (int32_t i_54 = 0; i_54 < 14; ++i_54) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_54), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[153]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[194]);
        ShadowLightShader::ulVertexConstantFlags[557] = 768;
        ShadowLightShader::ulPixelConstantFlags[557] = 4012;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[557] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[557] = 132;
        if (m_pObject != ShadowLightShader::spPass[556].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[556].m_pObject;
            v201 = ShadowLightShader::spPass[556].m_pObject;
            if (ShadowLightShader::spPass[556].m_pObject) {
                ++ShadowLightShader::spPass[556].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 16) {
            for (int32_t i_55 = 0; i_55 < 16; ++i_55) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_55), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[154]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[193]);
        ShadowLightShader::ulVertexConstantFlags[556] = 880;
        ShadowLightShader::ulPixelConstantFlags[556] = 1964;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[556] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[556] = 132;
        if (m_pObject != ShadowLightShader::spPass[558].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[558].m_pObject;
            v201 = ShadowLightShader::spPass[558].m_pObject;
            if (ShadowLightShader::spPass[558].m_pObject) {
                ++ShadowLightShader::spPass[558].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 16) {
            for (int32_t i_56 = 0; i_56 < 16; ++i_56) {
                NiD3DTextureStage::CreateNewStage(spstage);
                BSShader::SetupDefaultStage(spstage.m_pObject, static_cast<uint32_t>(i_56), 1u, 2u, 1);
                m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[154]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[195]);
        ShadowLightShader::ulVertexConstantFlags[558] = 880;
        ShadowLightShader::ulPixelConstantFlags[558] = 1964;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[558] = 864;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[558] = 132;
        if (m_pObject != ShadowLightShader::spPass[559].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[559].m_pObject;
            v201 = ShadowLightShader::spPass[559].m_pObject;
            if (ShadowLightShader::spPass[559].m_pObject) {
                ++ShadowLightShader::spPass[559].m_pObject->m_uiRefCount;
            }
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[153]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[196]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[559] = 768;
        ShadowLightShader::ulPixelConstantFlags[559] = 4;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[559] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[559] = 4;
        if (m_pObject != ShadowLightShader::spPass[499].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[499].m_pObject;
            v201 = ShadowLightShader::spPass[499].m_pObject;
            if (ShadowLightShader::spPass[499].m_pObject) {
                ++ShadowLightShader::spPass[499].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[107]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[109]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(25u, 5u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(24u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
        ShadowLightShader::ulVertexConstantFlags[499] = 240;
        ShadowLightShader::ulPixelConstantFlags[499] = 12;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[499] = 224;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[499] = 4;
        if (m_pObject != ShadowLightShader::spPass[496].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[496].m_pObject;
            v201 = ShadowLightShader::spPass[496].m_pObject;
            if (ShadowLightShader::spPass[496].m_pObject) {
                ++ShadowLightShader::spPass[496].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 3) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[108]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[110]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
        ShadowLightShader::ulPixelConstantFlags[496] = 12;
        ShadowLightShader::ulVertexConstantFlags[496] = 128;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[496] = 4;
        if (m_pObject != ShadowLightShader::spPass[502].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[502].m_pObject;
            v201 = ShadowLightShader::spPass[502].m_pObject;
            if (ShadowLightShader::spPass[502].m_pObject) {
                ++ShadowLightShader::spPass[502].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 3) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[111]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[113]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(25u, 5u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(24u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
        ShadowLightShader::ulVertexConstantFlags[502] = 128;
        ShadowLightShader::ulPixelConstantFlags[502] = 28;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[502] = 0;
    }
    m_pObject->DecRefCount();
}

void ShadowLightShader::PresetTextureEffectPasses_2x() {
    NiD3DPass* m_pObject = 0;
    NiD3DPass* v13 = 0;
    {
        NiPointer<NiD3DTextureStage> a1;
        if (ShadowLightShader::spPass[592].m_pObject) {
            m_pObject = ShadowLightShader::spPass[592].m_pObject;
            v13 = ShadowLightShader::spPass[592].m_pObject;
            ++ShadowLightShader::spPass[592].m_pObject->m_uiRefCount;
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v2 = m_pObject->m_spVertexShader.m_pObject;
        NiD3DVertexShader* v3 = ShadowLightShader::spVertexShaders[140];
        if (v2 != ShadowLightShader::spVertexShaders[140]) {
            if (v2) {
                NiDecRefCount(v2);
            }
            m_pObject->m_spVertexShader.m_pObject = v3;
            if (v3) {
                NiIncRefCount(v3);
            }
        }
        NiD3DPixelShader* v4 = ShadowLightShader::spPixelShaders[137];
        m_pObject->m_spPixelShader = v4;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(171u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 3u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
        ShadowLightShader::ulVertexConstantFlags[592] = 0;
        ShadowLightShader::ulPixelConstantFlags[592] = 0;
        if (m_pObject != ShadowLightShader::spPass[593].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[593].m_pObject;
            v13 = ShadowLightShader::spPass[593].m_pObject;
            if (ShadowLightShader::spPass[593].m_pObject) {
                ++ShadowLightShader::spPass[593].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v7 = ShadowLightShader::spVertexShaders[141];
        m_pObject->m_spVertexShader = v7;
        NiD3DPixelShader* v9 = ShadowLightShader::spPixelShaders[137];
        m_pObject->m_spPixelShader = v9;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(171u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 3u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
        ShadowLightShader::ulVertexConstantFlags[593] = 0;
        ShadowLightShader::ulPixelConstantFlags[593] = 0;
    }
    m_pObject->DecRefCount();
}

void ShadowLightShader::PresetRenderNormalsPasses_2x() {
    NiD3DPass* m_pObject = 0;
    NiD3DPass* v25 = 0;
    {
        NiPointer<NiD3DTextureStage> a1;
        if (ShadowLightShader::spPass[482].m_pObject) {
            m_pObject = ShadowLightShader::spPass[482].m_pObject;
            v25 = ShadowLightShader::spPass[482].m_pObject;
            ++ShadowLightShader::spPass[482].m_pObject->m_uiRefCount;
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v2 = m_pObject->m_spVertexShader.m_pObject;
        NiD3DVertexShader* v3 = ShadowLightShader::spVertexShaders[121];
        if (v2 != ShadowLightShader::spVertexShaders[121]) {
            if (v2) {
                NiDecRefCount(v2);
            }
            m_pObject->m_spVertexShader.m_pObject = v3;
            if (v3) {
                NiIncRefCount(v3);
            }
        }
        NiD3DPixelShader* v4 = ShadowLightShader::spPixelShaders[123];
        m_pObject->m_spPixelShader = v4;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        ShadowLightShader::ulVertexConstantFlags[482] = 0x4000;
        ShadowLightShader::ulPixelConstantFlags[482] = 0x20000;
        if (m_pObject != ShadowLightShader::spPass[483].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[483].m_pObject;
            v25 = ShadowLightShader::spPass[483].m_pObject;
            if (ShadowLightShader::spPass[483].m_pObject) {
                ++ShadowLightShader::spPass[BSShaderManager::BSSM_RENDERNORMALS_S].m_pObject->m_uiRefCount;
            }
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v7 = ShadowLightShader::spVertexShaders[122];
        m_pObject->m_spVertexShader = v7;
        NiD3DPixelShader* v9 = ShadowLightShader::spPixelShaders[123];
        m_pObject->m_spPixelShader = v9;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        ShadowLightShader::ulVertexConstantFlags[483] = 0x8800;
        ShadowLightShader::ulPixelConstantFlags[483] = 0x20000;
        if (m_pObject != ShadowLightShader::spPass[484].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[484].m_pObject;
            v25 = ShadowLightShader::spPass[484].m_pObject;
            if (ShadowLightShader::spPass[484].m_pObject) {
                ++ShadowLightShader::spPass[484].m_pObject->m_uiRefCount;
            }
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v11 = ShadowLightShader::spVertexShaders[123];
        m_pObject->m_spVertexShader = v11;
        NiD3DPixelShader* v13 = ShadowLightShader::spPixelShaders[124];
        m_pObject->m_spPixelShader = v13;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        ShadowLightShader::ulVertexConstantFlags[484] = 0x14400;
        ShadowLightShader::ulPixelConstantFlags[484] = 0x20000;
        if (m_pObject != ShadowLightShader::spPass[485].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[485].m_pObject;
            v25 = ShadowLightShader::spPass[485].m_pObject;
            if (ShadowLightShader::spPass[485].m_pObject) {
                ++ShadowLightShader::spPass[485].m_pObject->m_uiRefCount;
            }
        }
        NiD3DVertexShader* v15 = ShadowLightShader::spVertexShaders[124];
        m_pObject->m_spVertexShader = v15;
        NiD3DPixelShader* v17 = ShadowLightShader::spPixelShaders[125];
        m_pObject->m_spPixelShader = v17;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        ShadowLightShader::ulVertexConstantFlags[485] = 0;
        ShadowLightShader::ulPixelConstantFlags[485] = 0;
        if (m_pObject != ShadowLightShader::spPass[486].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[486].m_pObject;
            v25 = ShadowLightShader::spPass[486].m_pObject;
            if (ShadowLightShader::spPass[486].m_pObject) {
                ++ShadowLightShader::spPass[486].m_pObject->m_uiRefCount;
            }
        }
        NiD3DVertexShader* v19 = ShadowLightShader::spVertexShaders[125];
        m_pObject->m_spVertexShader = v19;
        NiD3DPixelShader* v21 = ShadowLightShader::spPixelShaders[125];
        m_pObject->m_spPixelShader = v21;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        ShadowLightShader::ulVertexConstantFlags[486] = 2048;
        ShadowLightShader::ulPixelConstantFlags[486] = 0;
    }
    m_pObject->DecRefCount();
}

void ShadowLightShader::PresetWaterFogPasses_2x() {
    bool v6;
    uint32_t uiValue;
    NiD3DPass* m_pObject = 0;
    NiD3DPass* v37 = 0;
    {
        NiPointer<NiD3DTextureStage> a1;
        if (ShadowLightShader::spPass[728].m_pObject) {
            m_pObject = ShadowLightShader::spPass[728].m_pObject;
            v37 = ShadowLightShader::spPass[728].m_pObject;
            ++ShadowLightShader::spPass[728].m_pObject->m_uiRefCount;
        }
        NiD3DVertexShader* v2 = m_pObject->m_spVertexShader.m_pObject;
        NiD3DVertexShader* v3 = ShadowLightShader::spVertexShaders[145];
        if (v2 != ShadowLightShader::spVertexShaders[145]) {
            if (v2) {
                NiDecRefCount(v2);
            }
            m_pObject->m_spVertexShader.m_pObject = v3;
            if (v3) {
                NiIncRefCount(v3);
            }
        }
        NiD3DPixelShader* v4 = ShadowLightShader::spPixelShaders[138];
        m_pObject->m_spPixelShader = v4;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 3u, 1);
        v6 = m_pObject->m_pkRenderStateGroup == 0;
        *reinterpret_cast<float*>(&uiValue) = BSShaderProperty::fDepthBiasUnit;
        if (v6) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(195u, uiValue, 1);
        ShadowLightShader::ulVertexConstantFlags[728] = 0x4004;
        ShadowLightShader::ulPixelConstantFlags[728] = 0x140040;
        if (m_pObject != ShadowLightShader::spPass[729].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[729].m_pObject;
            v37 = ShadowLightShader::spPass[729].m_pObject;
            if (ShadowLightShader::spPass[729].m_pObject) {
                ++ShadowLightShader::spPass[729].m_pObject->m_uiRefCount;
            }
        }
        NiD3DVertexShader* v7 = ShadowLightShader::spVertexShaders[146];
        m_pObject->m_spVertexShader = v7;
        NiD3DPixelShader* v9 = ShadowLightShader::spPixelShaders[138];
        m_pObject->m_spPixelShader = v9;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 3u, 1);
        v6 = m_pObject->m_pkRenderStateGroup == 0;
        *reinterpret_cast<float*>(&uiValue) = BSShaderProperty::fDepthBiasUnit;
        if (v6) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(195u, uiValue, 1);
        ShadowLightShader::ulVertexConstantFlags[729] = 0x8800;
        ShadowLightShader::ulPixelConstantFlags[729] = 0x140040;
        if (m_pObject != ShadowLightShader::spPass[734].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[734].m_pObject;
            v37 = ShadowLightShader::spPass[734].m_pObject;
            if (ShadowLightShader::spPass[734].m_pObject) {
                ++ShadowLightShader::spPass[734].m_pObject->m_uiRefCount;
            }
        }
        NiD3DTextureStage::CreateNewStage(a1);
        BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, 1);
        m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        NiD3DVertexShader* v11 = m_pObject->m_spVertexShader.m_pObject;
        NiD3DVertexShader* v12 = ShadowLightShader::spVertexShaders[147];
        if (v11 != ShadowLightShader::spVertexShaders[147]) {
            if (v11) {
                NiDecRefCount(v11);
            }
            m_pObject->m_spVertexShader.m_pObject = v12;
            if (v12) {
                NiIncRefCount(v12);
            }
        }
        NiD3DPixelShader* v13 = ShadowLightShader::spPixelShaders[139];
        m_pObject->m_spPixelShader = v13;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 3u, 1);
        v6 = m_pObject->m_pkRenderStateGroup == 0;
        *reinterpret_cast<float*>(&uiValue) = BSShaderProperty::fDepthBiasUnit;
        if (v6) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(195u, uiValue, 1);
        ShadowLightShader::ulVertexConstantFlags[734] = 0x4004;
        ShadowLightShader::ulPixelConstantFlags[734] = 0x140040;
        if (m_pObject != ShadowLightShader::spPass[735].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[735].m_pObject;
            v37 = ShadowLightShader::spPass[735].m_pObject;
            if (ShadowLightShader::spPass[735].m_pObject) {
                ++ShadowLightShader::spPass[735].m_pObject->m_uiRefCount;
            }
        }
        NiD3DTextureStage::CreateNewStage(a1);
        BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, 1);
        m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        NiD3DVertexShader* v15 = m_pObject->m_spVertexShader.m_pObject;
        NiD3DVertexShader* v16 = ShadowLightShader::spVertexShaders[148];
        if (v15 != ShadowLightShader::spVertexShaders[148]) {
            if (v15) {
                NiDecRefCount(v15);
            }
            m_pObject->m_spVertexShader.m_pObject = v16;
            if (v16) {
                NiIncRefCount(v16);
            }
        }
        NiD3DPixelShader* v17 = ShadowLightShader::spPixelShaders[139];
        m_pObject->m_spPixelShader = v17;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 3u, 1);
        v6 = m_pObject->m_pkRenderStateGroup == 0;
        *reinterpret_cast<float*>(&uiValue) = BSShaderProperty::fDepthBiasUnit;
        if (v6) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(195u, uiValue, 1);
        ShadowLightShader::ulVertexConstantFlags[735] = 0x8800;
        ShadowLightShader::ulPixelConstantFlags[735] = 0x140040;
        if (m_pObject != ShadowLightShader::spPass[740].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[740].m_pObject;
            v37 = ShadowLightShader::spPass[740].m_pObject;
            if (ShadowLightShader::spPass[740].m_pObject) {
                ++ShadowLightShader::spPass[740].m_pObject->m_uiRefCount;
            }
        }
        NiD3DVertexShader* v19 = ShadowLightShader::spVertexShaders[149];
        m_pObject->m_spVertexShader = v19;
        NiD3DPixelShader* v21 = ShadowLightShader::spPixelShaders[138];
        m_pObject->m_spPixelShader = v21;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 3u, 1);
        v6 = m_pObject->m_pkRenderStateGroup == 0;
        *reinterpret_cast<float*>(&uiValue) = BSShaderProperty::fDepthBiasUnit;
        if (v6) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(195u, uiValue, 1);
        ShadowLightShader::ulVertexConstantFlags[740] = 0x4004;
        ShadowLightShader::ulPixelConstantFlags[740] = 0x140040;
        if (m_pObject != ShadowLightShader::spPass[741].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[741].m_pObject;
            v37 = ShadowLightShader::spPass[741].m_pObject;
            if (ShadowLightShader::spPass[741].m_pObject) {
                ++ShadowLightShader::spPass[741].m_pObject->m_uiRefCount;
            }
        }
        NiD3DVertexShader* v23 = ShadowLightShader::spVertexShaders[150];
        m_pObject->m_spVertexShader = v23;
        NiD3DPixelShader* v25 = ShadowLightShader::spPixelShaders[138];
        m_pObject->m_spPixelShader = v25;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 3u, 1);
        v6 = m_pObject->m_pkRenderStateGroup == 0;
        *reinterpret_cast<float*>(&uiValue) = BSShaderProperty::fDepthBiasUnit;
        if (v6) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(195u, uiValue, 1);
        ShadowLightShader::ulVertexConstantFlags[741] = 0x8800;
        ShadowLightShader::ulPixelConstantFlags[741] = 0x140040;
        if (m_pObject != ShadowLightShader::spPass[746].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[746].m_pObject;
            v37 = ShadowLightShader::spPass[746].m_pObject;
            if (ShadowLightShader::spPass[746].m_pObject) {
                ++ShadowLightShader::spPass[746].m_pObject->m_uiRefCount;
            }
        }
        NiD3DTextureStage::CreateNewStage(a1);
        BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, 1);
        m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        NiD3DVertexShader* v27 = m_pObject->m_spVertexShader.m_pObject;
        NiD3DVertexShader* v28 = ShadowLightShader::spVertexShaders[151];
        if (v27 != ShadowLightShader::spVertexShaders[151]) {
            if (v27) {
                NiDecRefCount(v27);
            }
            m_pObject->m_spVertexShader.m_pObject = v28;
            if (v28) {
                NiIncRefCount(v28);
            }
        }
        NiD3DPixelShader* v29 = ShadowLightShader::spPixelShaders[139];
        m_pObject->m_spPixelShader = v29;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 3u, 1);
        v6 = m_pObject->m_pkRenderStateGroup == 0;
        *reinterpret_cast<float*>(&uiValue) = BSShaderProperty::fDepthBiasUnit;
        if (v6) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(195u, uiValue, 1);
        ShadowLightShader::ulVertexConstantFlags[746] = 0x4004;
        ShadowLightShader::ulPixelConstantFlags[746] = 0x140040;
        if (m_pObject != ShadowLightShader::spPass[747].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[747].m_pObject;
            v37 = ShadowLightShader::spPass[747].m_pObject;
            if (ShadowLightShader::spPass[747].m_pObject) {
                ++ShadowLightShader::spPass[747].m_pObject->m_uiRefCount;
            }
        }
        NiD3DTextureStage::CreateNewStage(a1);
        BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, 1);
        m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        NiD3DVertexShader* v31 = m_pObject->m_spVertexShader.m_pObject;
        NiD3DVertexShader* v32 = ShadowLightShader::spVertexShaders[152];
        if (v31 != ShadowLightShader::spVertexShaders[152]) {
            if (v31) {
                NiDecRefCount(v31);
            }
            m_pObject->m_spVertexShader.m_pObject = v32;
            if (v32) {
                NiIncRefCount(v32);
            }
        }
        NiD3DPixelShader* v33 = ShadowLightShader::spPixelShaders[139];
        m_pObject->m_spPixelShader = v33;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 3u, 1);
        v6 = m_pObject->m_pkRenderStateGroup == 0;
        *reinterpret_cast<float*>(&uiValue) = BSShaderProperty::fDepthBiasUnit;
        if (v6) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(195u, uiValue, 1);
        ShadowLightShader::ulVertexConstantFlags[747] = 0x8800;
        ShadowLightShader::ulPixelConstantFlags[747] = 0x140040;
    }
    m_pObject->DecRefCount();
}

// The depth passes (487 to 489): opaque, depth-tested less-or-equal and depth-written with the depth bias unit, vertex
// shaders 126 to 128 and the shared pixel shader 126. The depth bias is fDepthBiasUnit's bits, stored to a float local
// first. The pass pointer's release stays at the end. The D3D values FALSE and TRUE are written as 0 and 1. The pass
// pointer is held as a raw pointer and released without a null check (the pass was dereferenced first), as the game
// does.
void ShadowLightShader::PresetDepthPasses_2x() {
    // EXE-INLINED none: NiPointer<NiD3DPass> constructor
    NiD3DPass* pD3DPass = spPass[487].m_pObject;
    if (pD3DPass) {
        pD3DPass->IncRefCount();
    }
    pD3DPass->m_spVertexShader = spVertexShaders[126];
    pD3DPass->m_spPixelShader = spPixelShaders[126];
    pD3DPass->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
    pD3DPass->SetRenderState(D3DRS_ALPHATESTENABLE, 0);
    pD3DPass->SetRenderState(D3DRS_ZENABLE, 1);
    pD3DPass->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    pD3DPass->SetRenderState(D3DRS_ZWRITEENABLE, 1);
    float fDepthBias = BSShaderProperty::fDepthBiasUnit;
    pD3DPass->SetRenderState(D3DRS_DEPTHBIAS, std::bit_cast<uint32_t>(fDepthBias));
    ulVertexConstantFlags[487] = 0x84000;
    ulPixelConstantFlags[487] = 0;

    // EXE-INLINED none: NiPointer<NiD3DPass>::operator=
    if (pD3DPass != spPass[488].m_pObject) {
        pD3DPass->DecRefCount();
        pD3DPass = spPass[488].m_pObject;
        if (pD3DPass) {
            pD3DPass->IncRefCount();
        }
    }
    pD3DPass->m_spVertexShader = spVertexShaders[127];
    pD3DPass->m_spPixelShader = spPixelShaders[126];
    pD3DPass->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
    pD3DPass->SetRenderState(D3DRS_ALPHATESTENABLE, 0);
    pD3DPass->SetRenderState(D3DRS_ZENABLE, 1);
    pD3DPass->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    pD3DPass->SetRenderState(D3DRS_ZWRITEENABLE, 1);
    fDepthBias = BSShaderProperty::fDepthBiasUnit;
    pD3DPass->SetRenderState(D3DRS_DEPTHBIAS, std::bit_cast<uint32_t>(fDepthBias));
    ulVertexConstantFlags[488] = 0x88800;
    ulPixelConstantFlags[488] = 0;

    // EXE-INLINED none: NiPointer<NiD3DPass>::operator=
    if (pD3DPass != spPass[489].m_pObject) {
        pD3DPass->DecRefCount();
        pD3DPass = spPass[489].m_pObject;
        if (pD3DPass) {
            pD3DPass->IncRefCount();
        }
    }
    pD3DPass->m_spVertexShader = spVertexShaders[128];
    pD3DPass->m_spPixelShader = spPixelShaders[126];
    pD3DPass->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
    pD3DPass->SetRenderState(D3DRS_ALPHATESTENABLE, 0);
    pD3DPass->SetRenderState(D3DRS_ZENABLE, 1);
    pD3DPass->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    pD3DPass->SetRenderState(D3DRS_ZWRITEENABLE, 1);
    fDepthBias = BSShaderProperty::fDepthBiasUnit;
    pD3DPass->SetRenderState(D3DRS_DEPTHBIAS, std::bit_cast<uint32_t>(fDepthBias));
    ulVertexConstantFlags[489] = 0x84000;
    ulPixelConstantFlags[489] = 0;
    // EXE-INLINED none: NiPointer<NiD3DPass> destructor
    pD3DPass->DecRefCount();
}

// The velocity passes (490 and 491): opaque, depth-tested and depth-written, no culling, each with its own vertex
// shader and the shared pixel shader 127. The D3D values FALSE and TRUE are written as 0 and 1. The pass pointer is
// released unchecked, as in PresetDepthPasses_2x.
void ShadowLightShader::PresetVelocityPasses_2x() {
    // EXE-INLINED none: NiPointer<NiD3DPass> constructor
    NiD3DPass* pD3DPass = spPass[490].m_pObject;
    if (pD3DPass) {
        pD3DPass->IncRefCount();
    }
    pD3DPass->m_spVertexShader = spVertexShaders[129];
    pD3DPass->m_spPixelShader = spPixelShaders[127];
    pD3DPass->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
    pD3DPass->SetRenderState(D3DRS_ALPHATESTENABLE, 0);
    pD3DPass->SetRenderState(D3DRS_ZENABLE, 1);
    pD3DPass->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    pD3DPass->SetRenderState(D3DRS_ZWRITEENABLE, 1);
    float fDepthBias = BSShaderProperty::fDepthBiasUnit;
    pD3DPass->SetRenderState(D3DRS_DEPTHBIAS, std::bit_cast<uint32_t>(fDepthBias));
    pD3DPass->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    ulVertexConstantFlags[490] = 0x384000;
    ulPixelConstantFlags[490] = 0;

    // EXE-INLINED none: NiPointer<NiD3DPass>::operator=
    if (pD3DPass != spPass[491].m_pObject) {
        pD3DPass->DecRefCount();
        pD3DPass = spPass[491].m_pObject;
        if (pD3DPass) {
            pD3DPass->IncRefCount();
        }
    }
    pD3DPass->m_spVertexShader = spVertexShaders[130];
    pD3DPass->m_spPixelShader = spPixelShaders[127];
    pD3DPass->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
    pD3DPass->SetRenderState(D3DRS_ALPHATESTENABLE, 0);
    pD3DPass->SetRenderState(D3DRS_ZENABLE, 1);
    pD3DPass->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    pD3DPass->SetRenderState(D3DRS_ZWRITEENABLE, 1);
    fDepthBias = BSShaderProperty::fDepthBiasUnit;
    pD3DPass->SetRenderState(D3DRS_DEPTHBIAS, std::bit_cast<uint32_t>(fDepthBias));
    pD3DPass->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    ulVertexConstantFlags[491] = 0x389800;
    ulPixelConstantFlags[491] = 0;
    // EXE-INLINED none: NiPointer<NiD3DPass> destructor
    pD3DPass->DecRefCount();
}

// The local map passes (492 to 494): opaque and depth-written; the first two test less-or-equal with pixel shader 135,
// the last always passes with pixel shader 136. The D3D values FALSE and TRUE are written as 0 and 1. The pass pointer
// is released unchecked, as in PresetDepthPasses_2x.
void ShadowLightShader::PresetLocalMapPasses_2x() {
    // EXE-INLINED none: NiPointer<NiD3DPass> constructor
    NiD3DPass* pD3DPass = spPass[492].m_pObject;
    if (pD3DPass) {
        pD3DPass->IncRefCount();
    }
    pD3DPass->m_spVertexShader = spVertexShaders[137];
    pD3DPass->m_spPixelShader = spPixelShaders[135];
    pD3DPass->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
    pD3DPass->SetRenderState(D3DRS_ALPHATESTENABLE, 0);
    pD3DPass->SetRenderState(D3DRS_ZENABLE, 1);
    pD3DPass->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    pD3DPass->SetRenderState(D3DRS_ZWRITEENABLE, 1);
    ulVertexConstantFlags[492] = 0x4000;
    ulPixelConstantFlags[492] = 0;

    // EXE-INLINED none: NiPointer<NiD3DPass>::operator=
    if (pD3DPass != spPass[493].m_pObject) {
        pD3DPass->DecRefCount();
        pD3DPass = spPass[493].m_pObject;
        if (pD3DPass) {
            pD3DPass->IncRefCount();
        }
    }
    pD3DPass->m_spVertexShader = spVertexShaders[138];
    pD3DPass->m_spPixelShader = spPixelShaders[135];
    pD3DPass->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
    pD3DPass->SetRenderState(D3DRS_ALPHATESTENABLE, 0);
    pD3DPass->SetRenderState(D3DRS_ZENABLE, 1);
    pD3DPass->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    pD3DPass->SetRenderState(D3DRS_ZWRITEENABLE, 1);
    ulVertexConstantFlags[493] = 0x8800;
    ulPixelConstantFlags[493] = 0;

    // EXE-INLINED none: NiPointer<NiD3DPass>::operator=
    if (pD3DPass != spPass[494].m_pObject) {
        pD3DPass->DecRefCount();
        pD3DPass = spPass[494].m_pObject;
        if (pD3DPass) {
            pD3DPass->IncRefCount();
        }
    }
    pD3DPass->m_spVertexShader = spVertexShaders[139];
    pD3DPass->m_spPixelShader = spPixelShaders[136];
    pD3DPass->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
    pD3DPass->SetRenderState(D3DRS_ALPHATESTENABLE, 0);
    pD3DPass->SetRenderState(D3DRS_ZENABLE, 1);
    pD3DPass->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
    pD3DPass->SetRenderState(D3DRS_ZWRITEENABLE, 1);
    ulVertexConstantFlags[494] = 0;
    ulPixelConstantFlags[494] = 0;
    // EXE-INLINED none: NiPointer<NiD3DPass> destructor
    pD3DPass->DecRefCount();
}
