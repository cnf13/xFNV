#include "BSShader/ImageSpaceShaderDS4LumClamp.h"

#include "BSCore/BSStringT.h"
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

#include <cstddef>

ImageSpaceShaderDS4LumClamp::ImageSpaceShaderDS4LumClamp() {
    pTextures.SetSize(2u);
}

ImageSpaceShaderDS4LumClamp::~ImageSpaceShaderDS4LumClamp() {}

void ImageSpaceShaderDS4LumClamp::ReloadShaders() {
    const char* pName;
    alignas(8) BSShaderMacro kPixelSource;
    alignas(8) BSShaderMacro kVertex;
    alignas(8) BSShaderMacro kPixel;
    kVertex.strPath = "imagespace\\2x\\v\\HDRbase.v.hlsl";
    kVertex.kMacros[0].Name = nullptr;
    std::memset(&kVertex.kMacros[0].Definition, 0, sizeof(kVertex) - offsetof(BSShaderMacro, kMacros[0].Definition));
    kPixelSource.kMacros[0].Name = "TEXTAP";
    kPixelSource.kMacros[0].Definition = "4";
    kPixelSource.kMacros[1].Name = "LUMCLAMP";
    kPixelSource.kMacros[1].Definition = "";
    kPixelSource.kMacros[2].Name = nullptr;
    if (bDoTestHDR.uValue.b) {
        kPixelSource.strPath = "imagespace\\2x\\p\\HDRBlur_TEST.p.hlsl";
        std::memset(
            &kPixelSource.kMacros[2].Definition,
            0,
            sizeof(kPixelSource) - offsetof(BSShaderMacro, kMacros[2].Definition)
        );
        pName = "HDRDS4LUMCLAMPT";
    }
    else {
        kPixelSource.strPath = "imagespace\\2x\\p\\HDRBlur.p.hlsl";
        std::memset(
            &kPixelSource.kMacros[2].Definition,
            0,
            sizeof(kPixelSource) - offsetof(BSShaderMacro, kMacros[2].Definition)
        );
        pName = "HDRDS4LUMCLAMP";
    }
    kPixel = kPixelSource;
    LoadShaders(kVertex, kPixel, pName, ShaderTarget_vs_1_1, "ps_2_a");
    LoadStagesAndPasses();
}
