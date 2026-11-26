#include "BSShader/ImageSpaceShaderWaterWadingHeightmap.h"

#include "BSShader/BSShaderManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"

ImageSpaceShaderWaterWadingHeightmap::ImageSpaceShaderWaterWadingHeightmap() {
    pTextures.SetSize(2u);
}

ImageSpaceShaderWaterWadingHeightmap::~ImageSpaceShaderWaterWadingHeightmap() {}

void ImageSpaceShaderWaterWadingHeightmap::ReloadShaders() {
    BSShaderMacro v3;
    BSShaderMacro v4;
    v4.strPath = "water\\2_ab\\v\\displace.v.hlsl";
    v4.kMacros[0].Name = "HEIGHTMAP_WADING";
    v4.kMacros[0].Definition = nullptr;
    v4.kMacros[1].Name = nullptr;
    std::memset(&v4.kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
    v3.strPath = "water\\2_ab\\p\\displace.p.hlsl";
    v3.kMacros[0].Name = "HEIGHTMAP_WADING";
    v3.kMacros[0].Definition = nullptr;
    v3.kMacros[1].Name = nullptr;
    std::memset(&v3.kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
    const char* PSTarget = BSShaderManager::GetPSTarget(true);
    LoadShaders(v4, v3, "WATERDISPLACEMENTWADINGHEIGHTMAP", ShaderTarget_vs_1_1, PSTarget);
    LoadStagesAndPasses();
}

// With the displacement water stencil on, the pass draws only where the stencil holds 1.
void ImageSpaceShaderWaterWadingHeightmap::SetState() {
    if (BSShaderManager::bUseDisplacementWaterStencil) {
        NiD3DPass* pPass = spPass.m_pObject;
        if (!pPass->m_pkRenderStateGroup) {
            pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        pPass->m_pkRenderStateGroup->SetRenderState(D3DRS_STENCILENABLE, 1u, true);
        pPass = spPass.m_pObject;
        if (!pPass->m_pkRenderStateGroup) {
            pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        pPass->m_pkRenderStateGroup->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL, true);
        pPass = spPass.m_pObject;
        if (!pPass->m_pkRenderStateGroup) {
            pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        pPass->m_pkRenderStateGroup->SetRenderState(D3DRS_STENCILREF, 1u, true);
    }
}
