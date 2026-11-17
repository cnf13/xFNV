#include "BSShader/ImageSpaceShaderDS16LumClamp.h"

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

ImageSpaceShaderDS16LumClamp::ImageSpaceShaderDS16LumClamp() {
	pTextures.SetSize(2u);
}

ImageSpaceShaderDS16LumClamp::~ImageSpaceShaderDS16LumClamp() {}

void ImageSpaceShaderDS16LumClamp::ReloadShaders() {
	const char* v2;
	alignas(8) BSShaderMacro v4;
	alignas(8) BSShaderMacro v5;
	alignas(8) BSShaderMacro v7;
	v5.strPath = "imagespace\\2x\\v\\HDRbase.v.hlsl";
	v5.kMacros[0].Name = nullptr;
	std::memset(&v5.kMacros[0].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition));
	v4.kMacros[0].Name = "TEXTAP";
	v4.kMacros[0].Definition = "16";
	v4.kMacros[1].Name = "LUMCLAMP";
	v4.kMacros[1].Definition = "";
	v4.kMacros[2].Name = nullptr;
	if (bDoTestHDR.uValue.b) {
		v4.strPath = "imagespace\\2x\\p\\HDRBlur_TEST.p.hlsl";
		std::memset(&v4.kMacros[2].Definition, 0,
			sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
		v2 = "HDRDS16LUMCLAMPT";
	} else {
		v4.strPath = "imagespace\\2x\\p\\HDRBlur.p.hlsl";
		std::memset(&v4.kMacros[2].Definition, 0,
			sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
		v2 = "HDRDS16LUMCLAMP";
	}
	// The exe copies the pixel shader's macros inline (rep movsd), no memcpy call.
	v7 = v4;
	LoadShaders(v5, v7, v2, ShaderTarget_vs_1_1, "ps_2_a");
	LoadStagesAndPasses();
}
