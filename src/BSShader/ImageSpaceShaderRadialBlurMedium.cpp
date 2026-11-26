#include "BSShader/ImageSpaceShaderRadialBlurMedium.h"

#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

ImageSpaceShaderRadialBlurMedium::ImageSpaceShaderRadialBlurMedium() {
    pTextures.SetSize(2u);
}

ImageSpaceShaderRadialBlurMedium::~ImageSpaceShaderRadialBlurMedium() {}
