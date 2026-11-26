#include "BSShader/ImageSpaceShaderBlur3.h"

#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

ImageSpaceShaderBlur3::ImageSpaceShaderBlur3() {
    pTextures.SetSize(2u);
}

ImageSpaceShaderBlur3::~ImageSpaceShaderBlur3() {}
