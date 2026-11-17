#include "BSShader/ImageSpaceShaderFFTWaterSpectrum.h"

#include "BSShader/BSShader.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"

ImageSpaceShaderFFTWaterSpectrum::ImageSpaceShaderFFTWaterSpectrum() {
	pTextures.SetSize(3u);
}

ImageSpaceShaderFFTWaterSpectrum::~ImageSpaceShaderFFTWaterSpectrum() {}

void ImageSpaceShaderFFTWaterSpectrum::ReloadShaders() {
	BSShaderMacro kPixel;
	BSShaderMacro kVertex;
	kVertex.strPath = "imagespace\\1x\\v\\base.v.hlsl";
	std::memset(kVertex.kMacros, 0, sizeof(kVertex.kMacros));
	kPixel.strPath = "water\\2_ab\\p\\waterhmap.p.hlsl";
	kPixel.kMacros[0].Name = "WATER_SPECTRUM";
	std::memset(&kPixel.kMacros[0].Definition, 0, sizeof(kPixel) - offsetof(BSShaderMacro, kMacros[0].Definition));
	const char* PSTarget = BSShaderManager::GetPSTarget(1);
	LoadShaders(kVertex, kPixel, "FFTWATERSPECTRUM", ShaderTarget_vs_1_1, PSTarget);
	LoadStagesAndPasses();
}

// One pass with a wrapped stage per input, no depth test or write, no blending or alpha test, all colour channels
// written.
bool ImageSpaceShaderFFTWaterSpectrum::LoadStagesAndPasses() {
	NiPointer<NiD3DTextureStage> spStage;
	NiD3DPass::CreateNewPass(spPass);
	for (uint32_t i = 0; i < pTextures.m_usMaxSize; ++i) {
		NiD3DTextureStage::CreateNewStage(spStage);
		BSShader::SetupDefaultStage(spStage.m_pObject, i, D3DTADDRESS_WRAP, D3DTEXF_LINEAR, true);
		spPass.m_pObject->SetStage(spPass.m_pObject->m_uiCurrentStage, spStage.m_pObject);
	}
	spPass.m_pObject->SetRenderState(D3DRS_ZENABLE, 0, true);
	spPass.m_pObject->SetRenderState(D3DRS_ZWRITEENABLE, 0, true);
	spPass.m_pObject->SetRenderState(D3DRS_ALPHABLENDENABLE, 0, true);
	spPass.m_pObject->SetRenderState(D3DRS_ALPHATESTENABLE, 0, true);
	spPass.m_pObject->SetRenderState(D3DRS_COLORWRITEENABLE,
		D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA,
		true);
	SetState();
	spPass.m_pObject->m_spVertexShader = spVertexShader.m_pObject;
	spPass.m_pObject->m_spPixelShader = spPixelShader.m_pObject;
	return true;
}
