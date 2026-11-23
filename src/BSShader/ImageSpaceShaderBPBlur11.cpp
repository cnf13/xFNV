#include "BSShader/ImageSpaceShaderBPBlur11.h"

#include "BSCore/BSStringT.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "SDK/MSVC/string.h"

ImageSpaceShaderBPBlur11::ImageSpaceShaderBPBlur11() {
    pTextures.SetSize(3u);
}

ImageSpaceShaderBPBlur11::~ImageSpaceShaderBPBlur11() {}

void ImageSpaceShaderBPBlur11::ReloadShaders() {
    const char* pcName;
    alignas(8) BSShaderMacro kPixelShader;
    alignas(8) BSShaderMacro kVertexShader;
    alignas(8) BSShaderMacro kPixelShaderCopy;
    kVertexShader.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    kVertexShader.kMacros[0].Name = nullptr;
    std::memset(
        &kVertexShader.kMacros[0].Definition,
        0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kPixelShader.kMacros[0].Name = "BRIGHTPASS";
    kPixelShader.kMacros[0].Definition = "";
    kPixelShader.kMacros[1].Name = "TEXTAP";
    kPixelShader.kMacros[1].Definition = "11";
    kPixelShader.kMacros[2].Name = nullptr;
    if (bDoTestHDR.uValue.b) {
        kPixelShader.strPath = "imagespace\\2x\\p\\blur2_TEST.p.hlsl";
        std::memset(
            &kPixelShader.kMacros[2].Definition,
            0,
            sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
        );
        pcName = "BPBLUR11T";
    }
    else {
        kPixelShader.strPath = "imagespace\\2x\\p\\blur2.p.hlsl";
        std::memset(
            &kPixelShader.kMacros[2].Definition,
            0,
            sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
        );
        pcName = "BPBLUR11";
    }
    // Copies the pixel shader's macros to a second local and passes that copy.
    kPixelShaderCopy = kPixelShader;
    LoadShaders(kVertexShader, kPixelShaderCopy, pcName, ShaderTarget_vs_1_1, "ps_2_a");
    LoadStagesAndPasses();
}
