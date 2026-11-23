#include "BSShader/ImageSpaceShaderBlur15.h"

#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

ImageSpaceShaderBlur15::ImageSpaceShaderBlur15() {
    pTextures.SetSize(2u);
}

ImageSpaceShaderBlur15::~ImageSpaceShaderBlur15() {}
