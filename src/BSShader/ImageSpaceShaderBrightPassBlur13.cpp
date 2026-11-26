#include "BSShader/ImageSpaceShaderBrightPassBlur13.h"

#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

ImageSpaceShaderBrightPassBlur13::ImageSpaceShaderBrightPassBlur13() {
    pTextures.SetSize(3u);
}

ImageSpaceShaderBrightPassBlur13::~ImageSpaceShaderBrightPassBlur13() {}

void ImageSpaceShaderBrightPassBlur13::ReloadShaders() {
    const char* pName;
    alignas(8) BSShaderMacro kPixelSource;
    alignas(8) BSShaderMacro kVertex;
    alignas(8) BSShaderMacro kPixel;
    kVertex.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    kVertex.kMacros[0].Name = nullptr;
    std::memset(&kVertex.kMacros[0].Definition, 0, sizeof(kVertex) - offsetof(BSShaderMacro, kMacros[0].Definition));
    kPixelSource.kMacros[0].Name = "BRIGHTPASS";
    kPixelSource.kMacros[0].Definition = "";
    kPixelSource.kMacros[1].Name = "TEXTAP";
    kPixelSource.kMacros[1].Definition = "13";
    kPixelSource.kMacros[2].Name = nullptr;
    if (bDoTestHDR.uValue.b) {
        kPixelSource.strPath = "imagespace\\2x\\p\\blur2_TEST.p.hlsl";
        std::memset(
            &kPixelSource.kMacros[2].Definition,
            0,
            sizeof(kPixelSource) - offsetof(BSShaderMacro, kMacros[2].Definition)
        );
        pName = "BPBLUR13T";
    }
    else {
        kPixelSource.strPath = "imagespace\\2x\\p\\blur2.p.hlsl";
        std::memset(
            &kPixelSource.kMacros[2].Definition,
            0,
            sizeof(kPixelSource) - offsetof(BSShaderMacro, kMacros[2].Definition)
        );
        pName = "BPBLUR13";
    }
    kPixel = kPixelSource;
    LoadShaders(kVertex, kPixel, pName, ShaderTarget_vs_1_1, "ps_2_a");
    LoadStagesAndPasses();
}
