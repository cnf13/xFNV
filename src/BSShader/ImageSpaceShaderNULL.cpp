#include "BSShader/ImageSpaceShaderNULL.h"

#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

ImageSpaceShaderNULL::ImageSpaceShaderNULL() {
    pTextures.SetSize(1u);
}

ImageSpaceShaderNULL::~ImageSpaceShaderNULL() {}
