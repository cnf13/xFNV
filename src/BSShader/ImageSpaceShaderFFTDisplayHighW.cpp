#include "BSShader/ImageSpaceShaderFFTDisplayHighW.h"

#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "SDK/MSVC/string.h"

ImageSpaceShaderFFTDisplayHighW::ImageSpaceShaderFFTDisplayHighW() {
	pTextures.SetSize(2u);
}

ImageSpaceShaderFFTDisplayHighW::~ImageSpaceShaderFFTDisplayHighW() {}

void ImageSpaceShaderFFTDisplayHighW::ReloadShaders() {
	BSShaderMacro v3;
	BSShaderMacro v4;
	v4.strPath = "imagespace\\1x\\v\\base.v.hlsl";
	std::memset(v4.kMacros, 0, sizeof(v4.kMacros));
	v3.strPath = "water\\2_ab\\p\\waterhmap.p.hlsl";
	v3.kMacros[0].Name = "DISPLAY_HIGHW";
	std::memset(&v3.kMacros[0].Definition, 0, sizeof(v3) - offsetof(BSShaderMacro, kMacros[0].Definition));
	const char* PSTarget = BSShaderManager::GetPSTarget(1);
	LoadShaders(v4, v3, "FFTDISPLAYHIGHW", ShaderTarget_vs_1_1, PSTarget);
	LoadStagesAndPasses();
}
