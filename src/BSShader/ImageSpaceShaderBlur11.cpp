#include "BSShader/ImageSpaceShaderBlur11.h"

#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

ImageSpaceShaderBlur11::ImageSpaceShaderBlur11() {
    pTextures.SetSize(2u);
}

ImageSpaceShaderBlur11::~ImageSpaceShaderBlur11() {}
