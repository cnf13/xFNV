#include "BSShader/ImageSpaceShaderNoiseScrollAndBlend.h"

#include "BSShader/BSShader.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"

ImageSpaceShaderNoiseScrollAndBlend::ImageSpaceShaderNoiseScrollAndBlend() {
    pTextures.SetSize(2u);
}

ImageSpaceShaderNoiseScrollAndBlend::~ImageSpaceShaderNoiseScrollAndBlend() {}

// One pass with a wrapped stage per input, no depth test or write, no blending or alpha test, all colour channels
// written; the pass becomes the current one.
bool ImageSpaceShaderNoiseScrollAndBlend::LoadStagesAndPasses() {
    NiPointer<NiD3DTextureStage> spStage;
    NiD3DPass::CreateNewPass(spPass);
    for (uint32_t i = 0; i < pTextures.m_usMaxSize; ++i) {
        NiD3DTextureStage::CreateNewStage(spStage);
        BSShader::SetupDefaultStage(spStage.m_pObject, i, D3DTADDRESS_WRAP, D3DTEXF_LINEAR, true);
        spPass->SetStage(spPass->m_uiCurrentStage, spStage.m_pObject);
    }
    spPass->SetRenderState(D3DRS_ZENABLE, 0, true);
    spPass->SetRenderState(D3DRS_ZWRITEENABLE, 0, true);
    spPass->SetRenderState(D3DRS_ALPHABLENDENABLE, 0, true);
    spPass->SetRenderState(D3DRS_ALPHATESTENABLE, 0, true);
    spPass->SetRenderState(
        D3DRS_COLORWRITEENABLE,
        D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA,
        true
    );
    SetState();
    spPass->m_spVertexShader = spVertexShader.m_pObject;
    spPass->m_spPixelShader = spPixelShader.m_pObject;
    NiD3DShader::m_spCurrentPass = spPass.m_pObject;
    return true;
}
