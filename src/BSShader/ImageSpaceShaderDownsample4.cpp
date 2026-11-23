#include "BSShader/ImageSpaceShaderDownsample4.h"

#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "SDK/MSVC/string.h"

ImageSpaceShaderDownsample4::ImageSpaceShaderDownsample4() {
    pTextures.SetSize(2u);
}

ImageSpaceShaderDownsample4::~ImageSpaceShaderDownsample4() {}

void ImageSpaceShaderDownsample4::ReloadShaders() {
    const char* pName;
    alignas(8) BSShaderMacro kPixelSource;
    alignas(8) BSShaderMacro kVertex;
    alignas(8) BSShaderMacro kPixel;
    kVertex.strPath = "imagespace\\2x\\v\\HDRbase.v.hlsl";
    kVertex.kMacros[0].Name = nullptr;
    std::memset(&kVertex.kMacros[0].Definition, 0, sizeof(kVertex) - offsetof(BSShaderMacro, kMacros[0].Definition));
    kPixelSource.kMacros[0].Name = "TEXTAP";
    kPixelSource.kMacros[0].Definition = "4";
    kPixelSource.kMacros[1].Name = nullptr;
    if (bDoTestHDR.uValue.b) {
        kPixelSource.strPath = "imagespace\\2x\\p\\HDRBlur_TEST.p.hlsl";
        std::memset(
            &kPixelSource.kMacros[1].Definition,
            0,
            sizeof(kPixelSource) - offsetof(BSShaderMacro, kMacros[1].Definition)
        );
        pName = "HDRDOWN4T";
    }
    else {
        kPixelSource.strPath = "imagespace\\2x\\p\\HDRBlur.p.hlsl";
        std::memset(
            &kPixelSource.kMacros[1].Definition,
            0,
            sizeof(kPixelSource) - offsetof(BSShaderMacro, kMacros[1].Definition)
        );
        pName = "HDRDOWN4";
    }
    kPixel = kPixelSource;
    LoadShaders(kVertex, kPixel, pName, ShaderTarget_vs_1_1, "ps_2_a");
    LoadStagesAndPasses();
}
