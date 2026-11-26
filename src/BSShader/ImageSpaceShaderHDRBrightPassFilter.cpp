#include "BSShader/ImageSpaceShaderHDRBrightPassFilter.h"

#include "BSMain/Setting.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "SDK/MSVC/string.h"

ImageSpaceShaderHDRBrightPassFilter::ImageSpaceShaderHDRBrightPassFilter() {
    pTextures.SetSize(2u);
}

ImageSpaceShaderHDRBrightPassFilter::~ImageSpaceShaderHDRBrightPassFilter() {}

void ImageSpaceShaderHDRBrightPassFilter::ReloadShaders() {
    const char* v2;
    alignas(8) BSShaderMacro v4;
    alignas(8) BSShaderMacro v5;
    alignas(8) BSShaderMacro v6;
    v5.strPath = "imagespace\\2x\\v\\HDRbase.v.hlsl";
    std::memset(v5.kMacros, 0, sizeof(v5.kMacros));
    v4.kMacros[0].Name = "BRIGHTFILTER";
    v4.kMacros[0].Definition = nullptr;
    v4.kMacros[1].Name = nullptr;
    if (bDoTestHDR.uValue.b) {
        v4.strPath = "imagespace\\2x\\p\\HDRBlur_TEST.p.hlsl";
        std::memset(
            &v4.kMacros[1].Definition,
            0,
            sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
        );
        v2 = "HDRBRIGHTT";
    }
    else {
        v4.strPath = "imagespace\\2x\\p\\HDRBlur.p.hlsl";
        std::memset(
            &v4.kMacros[1].Definition,
            0,
            sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
        );
        v2 = "HDRBRIGHT";
    }
    v6 = v4;
    LoadShaders(v5, v6, v2, ShaderTarget_vs_1_1, "ps_2_a");
    LoadStagesAndPasses();
}
