#include "BSShader/WaterDisplaceTexOffset.h"

#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

WaterDisplaceTexOffset::WaterDisplaceTexOffset() {
    pTextures.SetSize(2u);
}

WaterDisplaceTexOffset::~WaterDisplaceTexOffset() {}

void WaterDisplaceTexOffset::ReloadShaders() {
    BSShaderMacro v3;
    BSShaderMacro v4;
    v4.strPath = "water\\2_ab\\v\\displace.v.hlsl";
    std::memset(v4.kMacros, 0, sizeof(v4.kMacros));
    v3.strPath = "water\\2_ab\\p\\displace.p.hlsl";
    v3.kMacros[0].Name = "TEXCOORD_OFFSET";
    std::memset(&v3.kMacros[0].Definition, 0, sizeof(v3.kMacros) - sizeof(v3.kMacros[0].Name));
    const char* PSTarget = BSShaderManager::GetPSTarget(1);
    LoadShaders(v4, v3, "WATERDISPLACEMENTTEXOFFSET", ShaderTarget_vs_1_1, PSTarget);
    LoadStagesAndPasses();
}
