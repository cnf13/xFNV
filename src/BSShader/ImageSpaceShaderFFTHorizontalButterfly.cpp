#include "BSShader/ImageSpaceShaderFFTHorizontalButterfly.h"

#include "BSShader/BSShader.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "BSShader/ImageSpaceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"

ImageSpaceShaderFFTHorizontalButterfly::ImageSpaceShaderFFTHorizontalButterfly() {
    pTextures.SetSize(4u);
}

ImageSpaceShaderFFTHorizontalButterfly::~ImageSpaceShaderFFTHorizontalButterfly() {}

void ImageSpaceShaderFFTHorizontalButterfly::ReloadShaders() {
    BSShaderMacro kPixel;
    BSShaderMacro kVertex;
    kVertex.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    std::memset(kVertex.kMacros, 0, sizeof(kVertex.kMacros));
    kPixel.strPath = "water\\2_ab\\p\\waterhmap.p.hlsl";
    kPixel.kMacros[0].Name = "HORIZONTAL_BUTTERFLY";
    std::memset(&kPixel.kMacros[0].Definition, 0, sizeof(kPixel) - offsetof(BSShaderMacro, kMacros[0].Definition));
    const char* PSTarget = BSShaderManager::GetPSTarget(true);
    LoadShaders(kVertex, kPixel, "FFTHORIZONTALBUTTERFLY", ShaderTarget_vs_1_1, PSTarget);
    LoadStagesAndPasses();
}

bool ImageSpaceShaderFFTHorizontalButterfly::LoadStagesAndPasses() {
    NiPointer<NiD3DTextureStage> a1;
    NiPointer<NiD3DPass>* p_spPass = &spPass;
    NiD3DPass::CreateNewPass(spPass);
    for (uint32_t i = 0; i < pTextures.m_usMaxSize; ++i) {
        NiD3DTextureStage::CreateNewStage(a1);
        if (i >= static_cast<uint32_t>(pTextures.m_usMaxSize) - 1) {
            BSShader::SetupDefaultStage(a1.m_pObject, i, 1u, 2u, true);
        }
        else {
            BSShader::SetupDefaultStage(a1.m_pObject, i, 3u, 2u, true);
        }
        p_spPass->m_pObject->SetStage(p_spPass->m_pObject->m_uiCurrentStage, a1.m_pObject);
    }
    NiD3DPass* m_pObject = p_spPass->m_pObject;
    if (!p_spPass->m_pObject->m_pkRenderStateGroup) {
        m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 0, true);
    NiD3DPass* v5 = p_spPass->m_pObject;
    if (!p_spPass->m_pObject->m_pkRenderStateGroup) {
        v5->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v5->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
    NiD3DPass* v6 = p_spPass->m_pObject;
    if (!p_spPass->m_pObject->m_pkRenderStateGroup) {
        v6->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v6->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
    NiD3DPass* v7 = p_spPass->m_pObject;
    if (!p_spPass->m_pObject->m_pkRenderStateGroup) {
        v7->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v7->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
    NiD3DPass* v8 = p_spPass->m_pObject;
    if (!p_spPass->m_pObject->m_pkRenderStateGroup) {
        v8->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v8->m_pkRenderStateGroup->SetRenderState(168u, 15u, true);
    SetState();
    p_spPass->m_pObject->m_spVertexShader = spVertexShader.m_pObject;
    NiD3DPass* v11 = p_spPass->m_pObject;
    NiD3DPixelShader* v12 = spPixelShader.m_pObject;
    v11->m_spPixelShader = v12;
    return true;
}
