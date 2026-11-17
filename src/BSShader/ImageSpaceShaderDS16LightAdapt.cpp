#include "BSShader/ImageSpaceShaderDS16LightAdapt.h"

#include "BSCore/BSStringT.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "SDK/MSVC/string.h"

ImageSpaceShaderDS16LightAdapt::ImageSpaceShaderDS16LightAdapt() {
	pTextures.SetSize(3u);
}

ImageSpaceShaderDS16LightAdapt::~ImageSpaceShaderDS16LightAdapt() {}

void ImageSpaceShaderDS16LightAdapt::ReloadShaders() {
	const char* v2;
	alignas(8) BSShaderMacro v4;
	alignas(8) BSShaderMacro v5;
	alignas(8) BSShaderMacro v6;
	v5.strPath = "imagespace\\2x\\v\\HDRbase.v.hlsl";
	std::memset(v5.kMacros, 0, sizeof(v5.kMacros));
	v4.kMacros[0].Name = "TEXTAP";
	v4.kMacros[0].Definition = "16";
	v4.kMacros[1].Name = "DOWNADAPT";
	v4.kMacros[1].Definition = "";
	v4.kMacros[2].Name = nullptr;
	if (bDoTestHDR.uValue.b) {
		v4.strPath = "imagespace\\2x\\p\\HDRBlur_TEST.p.hlsl";
		std::memset(&v4.kMacros[2].Definition, 0,
			sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
		v2 = "HDRDS16ADAPTT";
	} else {
		v4.strPath = "imagespace\\2x\\p\\HDRBlur.p.hlsl";
		std::memset(&v4.kMacros[2].Definition, 0,
			sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
		v2 = "HDRDS16ADAPT";
	}
	v6 = v4;
	LoadShaders(v5, v6, v2, ShaderTarget_vs_1_1, "ps_2_a");
	LoadStagesAndPasses();
}
