#include "BSShader/ImageSpaceShaderVATSScanlinesBlend.h"

#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "SDK/MSVC/string.h"

ImageSpaceShaderVATSScanlinesBlend::ImageSpaceShaderVATSScanlinesBlend() {
    pTextures.SetSize(5u);
}

ImageSpaceShaderVATSScanlinesBlend::~ImageSpaceShaderVATSScanlinesBlend() {}

void ImageSpaceShaderVATSScanlinesBlend::ReloadShaders() {
    BSShaderMacro kPixel;
    BSShaderMacro kVertex;
    kVertex.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    kVertex.kMacros[0].Name = nullptr;
    std::memset(&kVertex.kMacros[0].Definition, 0, sizeof(kVertex) - offsetof(BSShaderMacro, kMacros[0].Definition));
    kPixel.strPath = "imagespace\\2x\\p\\scanlines_blend.p.hlsl";
    kPixel.kMacros[0].Name = "BLURSRC1";
    kPixel.kMacros[0].Definition = nullptr;
    kPixel.kMacros[1].Name = "VATS";
    kPixel.kMacros[1].Definition = nullptr;
    kPixel.kMacros[2].Name = nullptr;
    std::memset(&kPixel.kMacros[2].Definition, 0, sizeof(kPixel) - offsetof(BSShaderMacro, kMacros[2].Definition));
    LoadShaders(kVertex, kPixel, "IFVATSSCANBLEND", ShaderTarget_vs_1_1, "ps_2_0");
    LoadStagesAndPasses();
    NiD3DPass* m_pObject = spPass.m_pObject;
    if (!m_pObject->m_pkRenderStateGroup) {
        m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
    NiD3DPass* v3 = spPass.m_pObject;
    if (!v3->m_pkRenderStateGroup) {
        v3->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v3->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
    if (!v3->m_pkRenderStateGroup) {
        v3->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v3->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
}
