#include "BSShader/ImageSpaceShaderAlphaMult.h"

#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

ImageSpaceShaderAlphaMult::ImageSpaceShaderAlphaMult() {
	pTextures.SetSize(2u);
}

ImageSpaceShaderAlphaMult::~ImageSpaceShaderAlphaMult() {}
