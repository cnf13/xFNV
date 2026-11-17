#include "BSShader/ImageSpaceShaderHDRBlur.h"

#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

ImageSpaceShaderHDRBlur::ImageSpaceShaderHDRBlur() {
    pTextures.SetSize(2u);
}

ImageSpaceShaderHDRBlur::~ImageSpaceShaderHDRBlur() {}
