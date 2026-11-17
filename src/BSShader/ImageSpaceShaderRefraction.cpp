#include "BSShader/ImageSpaceShaderRefraction.h"

#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

ImageSpaceShaderRefraction::ImageSpaceShaderRefraction() {
	pTextures.SetSize(3u);
}

ImageSpaceShaderRefraction::~ImageSpaceShaderRefraction() {}
