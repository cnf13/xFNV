#include "BSShader/ImageSpaceShaderBlur9.h"

#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

ImageSpaceShaderBlur9::ImageSpaceShaderBlur9() {
	pTextures.SetSize(2u);
}

ImageSpaceShaderBlur9::~ImageSpaceShaderBlur9() {}
