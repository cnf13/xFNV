#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ParallaxShader.h"
#include "BSShader/ShadowLightShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"

bool ParallaxShader::LoadStagesAndPasses() {
    NiPointer<NiD3DPass>* v2 = ParallaxShader::spPass;
    int32_t v3 = 33;
    do {
        NiD3DPass::CreateNewPass(*v2++);
        --v3;
    }
    while (v3);
    PresetPasses();
    return true;
}

void ParallaxShader::PresetPasses() {
    PresetADPasses_2x();
    PresetADTPasses_2x();
    PresetADTSPasses_2x();
    PresetDiffusePtPasses_2x();
    PresetTexturePasses_2x();
    PresetSpecularPasses_2x();
}

void ParallaxShader::PresetADPasses_2x() {
    NiD3DPass* m_pObject = nullptr;
    NiD3DPass* v38 = nullptr;
    {
        NiPointer<NiD3DTextureStage> spstage;
        if (ParallaxShader::spPass[0].m_pObject) {
            m_pObject = ParallaxShader::spPass[0].m_pObject;
            v38 = ParallaxShader::spPass[0].m_pObject;
            ++ParallaxShader::spPass[0].m_pObject->m_uiRefCount;
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, true);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, true);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, true);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v2 = this->pVertexShader[8];
        m_pObject->m_spVertexShader = v2;
        NiD3DPixelShader* v4 = this->pPixelShader[13];
        m_pObject->m_spPixelShader = v4;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[217] = 1920;
        ShadowLightShader::ulPixelConstantFlags[217] = 44;
        if (m_pObject != ParallaxShader::spPass[1].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ParallaxShader::spPass[1].m_pObject;
            v38 = ParallaxShader::spPass[1].m_pObject;
            if (ParallaxShader::spPass[1].m_pObject) {
                ++ParallaxShader::spPass[1].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, true);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, true);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, true);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v7 = this->pVertexShader[8];
        m_pObject->m_spVertexShader = v7;
        NiD3DPixelShader* v9 = this->pPixelShader[14];
        m_pObject->m_spPixelShader = v9;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[218] = 1920;
        ShadowLightShader::ulPixelConstantFlags[218] = 60;
        if (m_pObject != ParallaxShader::spPass[2].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ParallaxShader::spPass[2].m_pObject;
            v38 = ParallaxShader::spPass[2].m_pObject;
            if (ParallaxShader::spPass[2].m_pObject) {
                ++ParallaxShader::spPass[2].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, true);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, true);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, true);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v11 = this->pVertexShader[9];
        m_pObject->m_spVertexShader = v11;
        NiD3DPixelShader* v13 = this->pPixelShader[15];
        m_pObject->m_spPixelShader = v13;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[226] = 2032;
        ShadowLightShader::ulPixelConstantFlags[226] = 44;
        if (m_pObject != ParallaxShader::spPass[3].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ParallaxShader::spPass[3].m_pObject;
            v38 = ParallaxShader::spPass[3].m_pObject;
            if (ParallaxShader::spPass[3].m_pObject) {
                ++ParallaxShader::spPass[3].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, true);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, true);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, true);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v15 = this->pVertexShader[9];
        m_pObject->m_spVertexShader = v15;
        NiD3DPixelShader* v17 = this->pPixelShader[16];
        m_pObject->m_spPixelShader = v17;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[227] = 2032;
        ShadowLightShader::ulPixelConstantFlags[227] = 60;
        if (m_pObject != ParallaxShader::spPass[4].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ParallaxShader::spPass[4].m_pObject;
            v38 = ParallaxShader::spPass[4].m_pObject;
            if (ParallaxShader::spPass[4].m_pObject) {
                ++ParallaxShader::spPass[4u].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, true);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, true);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, true);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v19 = this->pVertexShader[10];
        m_pObject->m_spVertexShader = v19;
        NiD3DPixelShader* v21 = this->pPixelShader[17];
        m_pObject->m_spPixelShader = v21;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[235] = 1920;
        ShadowLightShader::ulPixelConstantFlags[235] = 44;
        if (m_pObject != ParallaxShader::spPass[5].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ParallaxShader::spPass[5].m_pObject;
            v38 = ParallaxShader::spPass[5].m_pObject;
            if (ParallaxShader::spPass[5].m_pObject) {
                ++ParallaxShader::spPass[5].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, true);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, true);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, true);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v23 = this->pVertexShader[10];
        m_pObject->m_spVertexShader = v23;
        NiD3DPixelShader* v25 = this->pPixelShader[18];
        m_pObject->m_spPixelShader = v25;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[236] = 1920;
        ShadowLightShader::ulPixelConstantFlags[236] = 60;
        if (m_pObject != ParallaxShader::spPass[6].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ParallaxShader::spPass[6].m_pObject;
            v38 = ParallaxShader::spPass[6].m_pObject;
            if (ParallaxShader::spPass[6].m_pObject) {
                ++ParallaxShader::spPass[6].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, true);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, true);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, true);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v27 = this->pVertexShader[11];
        m_pObject->m_spVertexShader = v27;
        NiD3DPixelShader* v29 = this->pPixelShader[19];
        m_pObject->m_spPixelShader = v29;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[244] = 2032;
        ShadowLightShader::ulPixelConstantFlags[244] = 44;
        if (m_pObject != ParallaxShader::spPass[7].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ParallaxShader::spPass[7].m_pObject;
            v38 = ParallaxShader::spPass[7].m_pObject;
            if (ParallaxShader::spPass[7].m_pObject) {
                ++ParallaxShader::spPass[7].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, true);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, true);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, true);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v31 = this->pVertexShader[11];
        m_pObject->m_spVertexShader = v31;
        NiD3DPixelShader* v33 = this->pPixelShader[20];
        m_pObject->m_spPixelShader = v33;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[245] = 2032;
        ShadowLightShader::ulPixelConstantFlags[245] = 60;
    }
    m_pObject->DecRefCount();
}

void ParallaxShader::PresetADTPasses_2x() {
    NiD3DPass* m_pObject = nullptr;
    NiD3DPass* v42 = nullptr;
    {
        NiPointer<NiD3DTextureStage> spstage;
        if (ParallaxShader::spPass[8].m_pObject) {
            m_pObject = ParallaxShader::spPass[8].m_pObject;
            v42 = ParallaxShader::spPass[8].m_pObject;
            ++ParallaxShader::spPass[8].m_pObject->m_uiRefCount;
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, true);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, true);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, true);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, true);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        NiD3DVertexShader* v2 = this->pVertexShader[0];
        m_pObject->m_spVertexShader = v2;
        NiD3DPixelShader* v4 = this->pPixelShader[0];
        m_pObject->m_spPixelShader = v4;
        ShadowLightShader::ulVertexConstantFlags[257] = 1920;
        ShadowLightShader::ulPixelConstantFlags[257] = 44;
        if (m_pObject != ParallaxShader::spPass[9].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ParallaxShader::spPass[9].m_pObject;
            v42 = ParallaxShader::spPass[9].m_pObject;
            if (ParallaxShader::spPass[9].m_pObject) {
                ++ParallaxShader::spPass[9].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, true);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, true);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, true);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, true);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        NiD3DVertexShader* v7 = this->pVertexShader[0];
        m_pObject->m_spVertexShader = v7;
        NiD3DPixelShader* v9 = this->pPixelShader[1];
        m_pObject->m_spPixelShader = v9;
        ShadowLightShader::ulVertexConstantFlags[258] = 1920;
        ShadowLightShader::ulPixelConstantFlags[258] = 44;
        if (m_pObject != ParallaxShader::spPass[10].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ParallaxShader::spPass[10].m_pObject;
            v42 = ParallaxShader::spPass[10].m_pObject;
            if (ParallaxShader::spPass[10].m_pObject) {
                ++ParallaxShader::spPass[10].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, true);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, true);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, true);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, true);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        NiD3DVertexShader* v11 = this->pVertexShader[0];
        m_pObject->m_spVertexShader = v11;
        NiD3DPixelShader* v13 = this->pPixelShader[2];
        m_pObject->m_spPixelShader = v13;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[259] = 1920;
        ShadowLightShader::ulPixelConstantFlags[259] = 60;
        if (m_pObject != ParallaxShader::spPass[11].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ParallaxShader::spPass[11].m_pObject;
            v42 = ParallaxShader::spPass[11].m_pObject;
            if (ParallaxShader::spPass[11].m_pObject) {
                ++ParallaxShader::spPass[11].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, true);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, true);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, true);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, true);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        NiD3DVertexShader* v15 = this->pVertexShader[1];
        m_pObject->m_spVertexShader = v15;
        NiD3DPixelShader* v17 = this->pPixelShader[3];
        m_pObject->m_spPixelShader = v17;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[270] = 2032;
        ShadowLightShader::ulPixelConstantFlags[270] = 44;
        if (m_pObject != ParallaxShader::spPass[12].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ParallaxShader::spPass[12].m_pObject;
            v42 = ParallaxShader::spPass[12].m_pObject;
            if (ParallaxShader::spPass[12].m_pObject) {
                ++ParallaxShader::spPass[12].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, true);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, true);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, true);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, true);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        NiD3DVertexShader* v19 = this->pVertexShader[1];
        m_pObject->m_spVertexShader = v19;
        NiD3DPixelShader* v21 = this->pPixelShader[4];
        m_pObject->m_spPixelShader = v21;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[271] = 2032;
        ShadowLightShader::ulPixelConstantFlags[271] = 60;
        if (m_pObject != ParallaxShader::spPass[13].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ParallaxShader::spPass[13].m_pObject;
            v42 = ParallaxShader::spPass[13].m_pObject;
            if (ParallaxShader::spPass[13].m_pObject) {
                ++ParallaxShader::spPass[13].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, true);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, true);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, true);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, true);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        NiD3DVertexShader* v23 = this->pVertexShader[2];
        m_pObject->m_spVertexShader = v23;
        NiD3DPixelShader* v25 = this->pPixelShader[5];
        m_pObject->m_spPixelShader = v25;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[281] = 1920;
        ShadowLightShader::ulPixelConstantFlags[281] = 44;
        if (m_pObject != ParallaxShader::spPass[14].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ParallaxShader::spPass[14].m_pObject;
            v42 = ParallaxShader::spPass[14].m_pObject;
            if (ParallaxShader::spPass[14].m_pObject) {
                ++ParallaxShader::spPass[14].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, true);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, true);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, true);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, true);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        NiD3DVertexShader* v27 = this->pVertexShader[2];
        m_pObject->m_spVertexShader = v27;
        NiD3DPixelShader* v29 = this->pPixelShader[6];
        m_pObject->m_spPixelShader = v29;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[282] = 1920;
        ShadowLightShader::ulPixelConstantFlags[282] = 60;
        if (m_pObject != ParallaxShader::spPass[15].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ParallaxShader::spPass[15].m_pObject;
            v42 = ParallaxShader::spPass[15].m_pObject;
            if (ParallaxShader::spPass[15].m_pObject) {
                ++ParallaxShader::spPass[15].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, true);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, true);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, true);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, true);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        NiD3DVertexShader* v31 = this->pVertexShader[3];
        m_pObject->m_spVertexShader = v31;
        NiD3DPixelShader* v33 = this->pPixelShader[7];
        m_pObject->m_spPixelShader = v33;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[292] = 2032;
        ShadowLightShader::ulPixelConstantFlags[292] = 44;
        if (m_pObject != ParallaxShader::spPass[16].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ParallaxShader::spPass[16].m_pObject;
            v42 = ParallaxShader::spPass[16].m_pObject;
            if (ParallaxShader::spPass[16].m_pObject) {
                ++ParallaxShader::spPass[16].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, true);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, true);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, true);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, true);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        NiD3DVertexShader* v35 = this->pVertexShader[3];
        m_pObject->m_spVertexShader = v35;
        NiD3DPixelShader* v37 = this->pPixelShader[8];
        m_pObject->m_spPixelShader = v37;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[293] = 2032;
        ShadowLightShader::ulPixelConstantFlags[293] = 60;
    }
    m_pObject->DecRefCount();
}

void ParallaxShader::PresetADTSPasses_2x() {
    NiD3DPass* m_pObject = nullptr;
    NiD3DPass* v38 = nullptr;
    {
        NiPointer<NiD3DTextureStage> spstage;
        if (ParallaxShader::spPass[17].m_pObject) {
            m_pObject = ParallaxShader::spPass[17].m_pObject;
            v38 = ParallaxShader::spPass[17].m_pObject;
            ++ParallaxShader::spPass[17].m_pObject->m_uiRefCount;
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, true);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, true);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, true);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, true);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        NiD3DVertexShader* v2 = this->pVertexShader[4];
        m_pObject->m_spVertexShader = v2;
        NiD3DPixelShader* v4 = this->pPixelShader[9];
        m_pObject->m_spPixelShader = v4;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[153] = 1920;
        ShadowLightShader::ulPixelConstantFlags[153] = 44;
        if (m_pObject != ParallaxShader::spPass[18].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ParallaxShader::spPass[18].m_pObject;
            v38 = ParallaxShader::spPass[18].m_pObject;
            if (ParallaxShader::spPass[18].m_pObject) {
                ++ParallaxShader::spPass[18].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, true);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, true);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, true);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, true);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        NiD3DVertexShader* v7 = this->pVertexShader[4];
        m_pObject->m_spVertexShader = v7;
        NiD3DPixelShader* v9 = this->pPixelShader[10];
        m_pObject->m_spPixelShader = v9;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[154] = 1920;
        ShadowLightShader::ulPixelConstantFlags[154] = 60;
        if (m_pObject != ParallaxShader::spPass[19].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ParallaxShader::spPass[19].m_pObject;
            v38 = ParallaxShader::spPass[19].m_pObject;
            if (ParallaxShader::spPass[19].m_pObject) {
                ++ParallaxShader::spPass[19].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, true);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, true);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, true);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, true);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        NiD3DVertexShader* v11 = this->pVertexShader[5];
        m_pObject->m_spVertexShader = v11;
        NiD3DPixelShader* v13 = this->pPixelShader[11];
        m_pObject->m_spPixelShader = v13;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[164] = 2032;
        ShadowLightShader::ulPixelConstantFlags[164] = 44;
        if (m_pObject != ParallaxShader::spPass[20].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ParallaxShader::spPass[20].m_pObject;
            v38 = ParallaxShader::spPass[20].m_pObject;
            if (ParallaxShader::spPass[20].m_pObject) {
                ++ParallaxShader::spPass[20u].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, true);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, true);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, true);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, true);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        NiD3DVertexShader* v15 = this->pVertexShader[5];
        m_pObject->m_spVertexShader = v15;
        NiD3DPixelShader* v17 = this->pPixelShader[12];
        m_pObject->m_spPixelShader = v17;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[165] = 2032;
        ShadowLightShader::ulPixelConstantFlags[165] = 60;
        if (m_pObject != ParallaxShader::spPass[21].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ParallaxShader::spPass[21].m_pObject;
            v38 = ParallaxShader::spPass[21].m_pObject;
            if (ParallaxShader::spPass[21].m_pObject) {
                ++ParallaxShader::spPass[21].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, true);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, true);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, true);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, true);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        NiD3DVertexShader* v19 = this->pVertexShader[6];
        m_pObject->m_spVertexShader = v19;
        NiD3DPixelShader* v21 = this->pPixelShader[29];
        m_pObject->m_spPixelShader = v21;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[175] = 1920;
        ShadowLightShader::ulPixelConstantFlags[175] = 44;
        if (m_pObject != ParallaxShader::spPass[22].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ParallaxShader::spPass[22].m_pObject;
            v38 = ParallaxShader::spPass[22].m_pObject;
            if (ParallaxShader::spPass[22].m_pObject) {
                ++ParallaxShader::spPass[22].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, true);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, true);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, true);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, true);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        NiD3DVertexShader* v23 = this->pVertexShader[6];
        m_pObject->m_spVertexShader = v23;
        NiD3DPixelShader* v25 = this->pPixelShader[30];
        m_pObject->m_spPixelShader = v25;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[176] = 1920;
        ShadowLightShader::ulPixelConstantFlags[176] = 60;
        if (m_pObject != ParallaxShader::spPass[23].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ParallaxShader::spPass[23].m_pObject;
            v38 = ParallaxShader::spPass[23].m_pObject;
            if (ParallaxShader::spPass[23].m_pObject) {
                ++ParallaxShader::spPass[23].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, true);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, true);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, true);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, true);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        NiD3DVertexShader* v27 = this->pVertexShader[7];
        m_pObject->m_spVertexShader = v27;
        NiD3DPixelShader* v29 = this->pPixelShader[31];
        m_pObject->m_spPixelShader = v29;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[186] = 2032;
        ShadowLightShader::ulPixelConstantFlags[186] = 44;
        if (m_pObject != ParallaxShader::spPass[24].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ParallaxShader::spPass[24].m_pObject;
            v38 = ParallaxShader::spPass[24].m_pObject;
            if (ParallaxShader::spPass[24].m_pObject) {
                ++ParallaxShader::spPass[24u].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, true);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, true);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, true);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, true);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        NiD3DVertexShader* v31 = this->pVertexShader[7];
        m_pObject->m_spVertexShader = v31;
        NiD3DPixelShader* v33 = this->pPixelShader[32];
        m_pObject->m_spPixelShader = v33;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[187] = 2032;
        ShadowLightShader::ulPixelConstantFlags[187] = 60;
    }
    m_pObject->DecRefCount();
}

void ParallaxShader::PresetDiffusePtPasses_2x() {
    NiD3DPass* m_pObject = nullptr;
    NiD3DPass* v14 = nullptr;
    {
        NiPointer<NiD3DTextureStage> spstage;
        if (ParallaxShader::spPass[25].m_pObject) {
            m_pObject = ParallaxShader::spPass[25].m_pObject;
            v14 = ParallaxShader::spPass[25].m_pObject;
            ++ParallaxShader::spPass[25].m_pObject->m_uiRefCount;
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, true);
            m_pObject->SetStage(3u, spstage.m_pObject);
        }
        NiD3DVertexShader* v3 = pVertexShader[12];
        m_pObject->m_spVertexShader = v3;
        NiD3DPixelShader* v5 = pPixelShader[21];
        m_pObject->m_spPixelShader = v5;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
        ShadowLightShader::ulVertexConstantFlags[308] = 1152;
        ShadowLightShader::ulPixelConstantFlags[308] = 8;
        if (m_pObject != ParallaxShader::spPass[26].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ParallaxShader::spPass[26].m_pObject;
            v14 = ParallaxShader::spPass[26].m_pObject;
            if (ParallaxShader::spPass[26].m_pObject) {
                ++ParallaxShader::spPass[26].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, true);
            m_pObject->SetStage(3u, spstage.m_pObject);
        }
        NiD3DVertexShader* v8 = pVertexShader[13];
        m_pObject->m_spVertexShader = v8;
        NiD3DPixelShader* v10 = pPixelShader[22];
        m_pObject->m_spPixelShader = v10;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
        ShadowLightShader::ulVertexConstantFlags[320] = 1152;
        ShadowLightShader::ulPixelConstantFlags[320] = 8;
    }
    m_pObject->DecRefCount();
}

void ParallaxShader::PresetTexturePasses_2x() {
    NiD3DPass* m_pObject = nullptr;
    NiD3DPass* v10 = nullptr;
    {
        NiPointer<NiD3DTextureStage> a1;
        if (ParallaxShader::spPass[27].m_pObject) {
            m_pObject = ParallaxShader::spPass[27].m_pObject;
            v10 = ParallaxShader::spPass[27].m_pObject;
            ++ParallaxShader::spPass[27].m_pObject->m_uiRefCount;
        }
        if (m_pObject->m_uiStageCount < 3) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v3 = pVertexShader[14];
        m_pObject->m_spVertexShader = v3;
        NiD3DPixelShader* v5 = pPixelShader[23];
        m_pObject->m_spPixelShader = v5;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 9u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[339] = 1024;
        ShadowLightShader::ulPixelConstantFlags[339] = 32;
    }
    m_pObject->DecRefCount();
}

void ParallaxShader::PresetSpecularPasses_2x() {
    NiD3DPass* m_pObject = nullptr;
    NiD3DPass* v29 = nullptr;
    {
        NiPointer<NiD3DTextureStage> a1;
        if (ParallaxShader::spPass[28].m_pObject) {
            m_pObject = ParallaxShader::spPass[28].m_pObject;
            v29 = ParallaxShader::spPass[28].m_pObject;
            ++ParallaxShader::spPass[28u].m_pObject->m_uiRefCount;
        }
        if (m_pObject->m_uiStageCount < 6) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            a1.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            a1.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(a1.m_pObject, 3u, 3u, 0, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 4u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 5u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v3 = pVertexShader[15];
        m_pObject->m_spVertexShader = v3;
        NiD3DPixelShader* v5 = pPixelShader[24];
        m_pObject->m_spPixelShader = v5;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
        ShadowLightShader::ulVertexConstantFlags[348] = 1152;
        ShadowLightShader::ulPixelConstantFlags[348] = 40;
        if (m_pObject != ParallaxShader::spPass[29].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ParallaxShader::spPass[29].m_pObject;
            v29 = ParallaxShader::spPass[29].m_pObject;
            if (ParallaxShader::spPass[29].m_pObject) {
                ++ParallaxShader::spPass[29].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 6) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            a1.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            a1.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(a1.m_pObject, 3u, 3u, 0, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 4u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 5u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v8 = this->pVertexShader[16];
        m_pObject->m_spVertexShader = v8;
        NiD3DPixelShader* v10 = this->pPixelShader[25];
        m_pObject->m_spPixelShader = v10;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
        ShadowLightShader::ulVertexConstantFlags[354] = 1264;
        ShadowLightShader::ulPixelConstantFlags[354] = 40;
        if (m_pObject != ParallaxShader::spPass[30].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ParallaxShader::spPass[30].m_pObject;
            v29 = ParallaxShader::spPass[30].m_pObject;
            if (ParallaxShader::spPass[30].m_pObject) {
                ++ParallaxShader::spPass[30].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 6) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            a1.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            a1.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(a1.m_pObject, 3u, 3u, 0, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 4u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 5u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v13 = this->pVertexShader[17];
        m_pObject->m_spVertexShader = v13;
        NiD3DPixelShader* v15 = this->pPixelShader[26];
        m_pObject->m_spPixelShader = v15;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
        ShadowLightShader::ulVertexConstantFlags[360] = 1152;
        ShadowLightShader::ulPixelConstantFlags[360] = ShadowLightShader::ulPixelConstantFlags[358];
        if (m_pObject != ParallaxShader::spPass[31].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ParallaxShader::spPass[31].m_pObject;
            v29 = ParallaxShader::spPass[31].m_pObject;
            if (ParallaxShader::spPass[31].m_pObject) {
                ++ParallaxShader::spPass[31].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 6) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            a1.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            a1.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(a1.m_pObject, 3u, 3u, 0, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 4u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 5u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v17 = this->pVertexShader[18];
        m_pObject->m_spVertexShader = v17;
        NiD3DPixelShader* v19 = this->pPixelShader[27];
        m_pObject->m_spPixelShader = v19;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
        ShadowLightShader::ulVertexConstantFlags[366] = 1152;
        ShadowLightShader::ulPixelConstantFlags[366] = ShadowLightShader::ulPixelConstantFlags[364];
        if (m_pObject != ParallaxShader::spPass[32].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ParallaxShader::spPass[32].m_pObject;
            v29 = ParallaxShader::spPass[32].m_pObject;
            if (ParallaxShader::spPass[32].m_pObject) {
                ++ParallaxShader::spPass[32u].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 6) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            a1.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            a1.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(a1.m_pObject, 3u, 3u, 0, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 4u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 5u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v22 = this->pVertexShader[19];
        m_pObject->m_spVertexShader = v22;
        NiD3DPixelShader* v24 = this->pPixelShader[28];
        m_pObject->m_spPixelShader = v24;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
        ShadowLightShader::ulVertexConstantFlags[372] = 1152;
        ShadowLightShader::ulPixelConstantFlags[372] = ShadowLightShader::ulPixelConstantFlags[370];
    }
    m_pObject->DecRefCount();
}
