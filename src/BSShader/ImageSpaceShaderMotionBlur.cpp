#include "BSShader/ImageSpaceShaderMotionBlur.h"

#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

ImageSpaceShaderMotionBlur::ImageSpaceShaderMotionBlur() {
	pTextures.SetSize(3u);
}

ImageSpaceShaderMotionBlur::~ImageSpaceShaderMotionBlur() {}
