#include "BSShader/WaterDisplacementNormals.h"

#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "SDK/MSVC/string.h"

WaterDisplacementNormals::WaterDisplacementNormals() {
    pTextures.SetSize(2u);
}

WaterDisplacementNormals::~WaterDisplacementNormals() {}

// Each macro list ends after its last set entry: that entry's Definition is nulled (when it has a Name), the next
// entry's null Name is stored and the rest of the structure is cleared from the next Definition on.
void WaterDisplacementNormals::ReloadShaders() {
    BSShaderMacro v3;
    BSShaderMacro v4;
    v4.strPath = "water\\2_ab\\v\\displace.v.hlsl";
    v4.kMacros[0].Name = nullptr;
    std::memset(&v4.kMacros[0].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition));
    v3.strPath = "water\\2_ab\\p\\displace.p.hlsl";
    v3.kMacros[0].Name = "NORMALS";
    v3.kMacros[0].Definition = nullptr;
    v3.kMacros[1].Name = nullptr;
    std::memset(&v3.kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
    const char* PSTarget = BSShaderManager::GetPSTarget(true);
    LoadShaders(v4, v3, "WATERDISPLACEMENTNORMALS", ShaderTarget_vs_1_1, PSTarget);
    LoadStagesAndPasses();
}
