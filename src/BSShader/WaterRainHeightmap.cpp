#include "BSShader/WaterRainHeightmap.h"

#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "SDK/MSVC/string.h"

WaterRainHeightmap::WaterRainHeightmap() {
    pTextures.SetSize(2u);
}

WaterRainHeightmap::~WaterRainHeightmap() {}

// Each macro list ends after its last set entry: that entry's Definition is nulled (when it has a Name), the next
// entry's Name is set to null and the rest of the structure is zeroed from the next Definition on.
void WaterRainHeightmap::ReloadShaders() {
    BSShaderMacro v3;
    BSShaderMacro v4;
    v4.strPath = "water\\2_ab\\v\\displace.v.hlsl";
    v4.kMacros[0].Name = nullptr;
    std::memset(&v4.kMacros[0].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition));
    v3.strPath = "water\\2_ab\\p\\displace.p.hlsl";
    v3.kMacros[0].Name = "HEIGHTMAP_RAIN";
    v3.kMacros[0].Definition = nullptr;
    v3.kMacros[1].Name = nullptr;
    std::memset(&v3.kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
    const char* PSTarget = BSShaderManager::GetPSTarget(1);
    LoadShaders(v4, v3, "WATERDISPLACEMENTRAINHEIGHTMAP", ShaderTarget_vs_1_1, PSTarget);
    LoadStagesAndPasses();
}
