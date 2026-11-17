#include "BSShader/ImageSpaceShaderBlur7.h"

#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

ImageSpaceShaderBlur7::ImageSpaceShaderBlur7() {
    pTextures.SetSize(2u);
}

ImageSpaceShaderBlur7::~ImageSpaceShaderBlur7() {}
