#include "BSShader/ImageSpaceShaderBrightPassBlur7.h"

#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

ImageSpaceShaderBrightPassBlur7::ImageSpaceShaderBrightPassBlur7() {
	pTextures.SetSize(3u);
}

ImageSpaceShaderBrightPassBlur7::~ImageSpaceShaderBrightPassBlur7() {}
