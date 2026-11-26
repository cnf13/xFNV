#include "BSShader/ImageSpaceShaderMap.h"

#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

ImageSpaceShaderMap::ImageSpaceShaderMap() {
    pTextures.SetSize(2u);
}

ImageSpaceShaderMap::~ImageSpaceShaderMap() {}
