#include "BSShader/ImageSpaceShaderBlur13.h"

#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

ImageSpaceShaderBlur13::ImageSpaceShaderBlur13() {
    pTextures.SetSize(2u);
}

ImageSpaceShaderBlur13::~ImageSpaceShaderBlur13() {}
