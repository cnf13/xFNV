#include "BSShader/ImageSpaceShaderHDRBlendCinematic.h"

#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "SDK/MSVC/string.h"

ImageSpaceShaderHDRBlendCinematic::ImageSpaceShaderHDRBlendCinematic() {
	pTextures.SetSize(4u);
}

ImageSpaceShaderHDRBlendCinematic::~ImageSpaceShaderHDRBlendCinematic() {}

void ImageSpaceShaderHDRBlendCinematic::ReloadShaders() {
	const char* pName;
	alignas(8) BSShaderMacro kPixelSource;
	alignas(8) BSShaderMacro kVertex;
	alignas(8) BSShaderMacro kPixel;
	kVertex.strPath = "imagespace\\2x\\v\\HDRbase.v.hlsl";
	kVertex.kMacros[0].Name = "TEX2";
	kVertex.kMacros[0].Definition = nullptr;
	kVertex.kMacros[1].Name = nullptr;
	std::memset(&kVertex.kMacros[1].Definition, 0, sizeof(kVertex) - offsetof(BSShaderMacro, kMacros[1].Definition));
	kPixelSource.kMacros[0].Name = "CINEMATIC";
	kPixelSource.kMacros[0].Definition = nullptr;
	kPixelSource.kMacros[1].Name = "SHBLEND";
	kPixelSource.kMacros[1].Definition = nullptr;
	kPixelSource.kMacros[2].Name = "TONEMAP";
	kPixelSource.kMacros[2].Definition = nullptr;
	kPixelSource.kMacros[3].Name = nullptr;
	if (bDoTestHDR.uValue.b) {
		kPixelSource.strPath = "imagespace\\2x\\p\\HDRBlur_TEST.p.hlsl";
		std::memset(&kPixelSource.kMacros[3].Definition, 0,
			sizeof(kPixelSource) - offsetof(BSShaderMacro, kMacros[3].Definition));
		pName = "HDRBLENDINSHADERCINT";
	} else {
		kPixelSource.strPath = "imagespace\\2x\\p\\HDRBlur.p.hlsl";
		std::memset(&kPixelSource.kMacros[3].Definition, 0,
			sizeof(kPixelSource) - offsetof(BSShaderMacro, kMacros[3].Definition));
		pName = "HDRBLENDINSHADERCIN";
	}
	kPixel = kPixelSource;
	LoadShaders(kVertex, kPixel, pName, ShaderTarget_vs_1_1, "ps_2_a");
	LoadStagesAndPasses();
}
