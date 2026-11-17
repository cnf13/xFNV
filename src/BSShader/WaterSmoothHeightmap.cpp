#include "BSShader/WaterSmoothHeightmap.h"

#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

WaterSmoothHeightmap::WaterSmoothHeightmap() {
    pTextures.SetSize(2u);
}

WaterSmoothHeightmap::~WaterSmoothHeightmap() {}

void WaterSmoothHeightmap::ReloadShaders() {
    BSShaderMacro v3;
    BSShaderMacro v4;
    v4.strPath = "water\\2_ab\\v\\displace.v.hlsl";
    v4.kMacros[0].Name = "HEIGHTMAP_SMOOTH";
    v4.kMacros[0].Definition = nullptr;
    v4.kMacros[1].Name = nullptr;
    std::memset(&v4.kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
    v3.strPath = "water\\2_ab\\p\\displace.p.hlsl";
    v3.kMacros[0].Name = "HEIGHTMAP_SMOOTH";
    v3.kMacros[0].Definition = nullptr;
    v3.kMacros[1].Name = nullptr;
    std::memset(&v3.kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
    const char* PSTarget = BSShaderManager::GetPSTarget(1);
    LoadShaders(v4, v3, "WATERSMOOTHHEIGHTMAP", ShaderTarget_vs_1_1, PSTarget);
    LoadStagesAndPasses();
}
