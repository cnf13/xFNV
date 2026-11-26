#include "BSShader/ImageSpaceShaderInterfaceScanlinesBlend.h"

#include "BSCore/BSStringT.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "SDK/MSVC/string.h"

ImageSpaceShaderInterfaceScanlinesBlend::ImageSpaceShaderInterfaceScanlinesBlend() {
    pTextures.SetSize(5u);
}

ImageSpaceShaderInterfaceScanlinesBlend::~ImageSpaceShaderInterfaceScanlinesBlend() {}

void ImageSpaceShaderInterfaceScanlinesBlend::ReloadShaders() {
    BSShaderMacro v2;
    BSShaderMacro v3;
    v3.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    std::memset(v3.kMacros, 0, sizeof(v3.kMacros));
    v2.strPath = "imagespace\\2x\\p\\scanlines_blend.p.hlsl";
    v2.kMacros[0].Name = "BLURSRC1";
    v2.kMacros[0].Definition = "";
    std::memset(&v2.kMacros[1], 0, sizeof(_D3DXMACRO) * 8);
    LoadShaders(v3, v2, "IFSCANBLEND", ShaderTarget_vs_1_1, "ps_2_0");
    LoadStagesAndPasses();
}
