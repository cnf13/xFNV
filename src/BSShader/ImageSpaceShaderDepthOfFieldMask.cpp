#include "BSShader/ImageSpaceShaderDepthOfFieldMask.h"

#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

ImageSpaceShaderDepthOfFieldMask::ImageSpaceShaderDepthOfFieldMask() {
    pTextures.SetSize(5u);
}

ImageSpaceShaderDepthOfFieldMask::~ImageSpaceShaderDepthOfFieldMask() {}
