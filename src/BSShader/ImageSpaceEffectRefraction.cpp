#include "BSShader/ImageSpaceEffectRefraction.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSTextureManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceEffectParam.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "BSShader/ImageSpaceShaderParam.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDepthStencilBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElements.h"

ImageSpaceEffectRefraction::ImageSpaceEffectRefraction() : spRefractionNormalBuffer() {}

// The first shader parameter is the effect's; the normals texture goes back to the texture manager.
ImageSpaceEffectRefraction::~ImageSpaceEffectRefraction() {
	if (pEffectParamList.m_pBase[0].m_pObject) {
		delete pEffectParamList.m_pBase[0].m_pObject;
		pEffectParamList.SetAt(0, nullptr);
	}
	if (spRefractionNormalBuffer.GetTexture()) {
		BSShaderManager::pTextureManager->ReturnRenderedTexture(spRefractionNormalBuffer.GetRenderedTexture());
		spRefractionNormalBuffer.ClearTexture();
	}
}

// The first effect is an ImageSpaceShader: the exe downcasts it (null-checked) and calls its ImageSpaceEffect part.
void ImageSpaceEffectRefraction::Render(NiScreenElements* apGeometry, NiDX9Renderer* apRenderer,
	ImageSpaceEffectParam* apParam, int32_t abEndFrame) {
	if (bParamsChanged) {
		bParamsChanged = UpdateParams(apParam);
	}
	ImageSpaceShader* pShader = static_cast<ImageSpaceShader*>(pEffectList.m_pBase->m_pObject);
	ImageSpaceShaderParam* v8 = pEffectParamList.m_pBase->m_pObject;
	pShader->SetTexture(0, *pTextures.m_pBase, NiTexturingProperty::FILTER_NEAREST);
	pShader->SetTexture(1, pTextures.m_pBase[1], NiTexturingProperty::FILTER_NEAREST);
	ImageSpaceTexture* p_spRefractionNormalBuffer = &spRefractionNormalBuffer;
	pShader->SetTexture(2, p_spRefractionNormalBuffer, NiTexturingProperty::FILTER_NEAREST);
	pShader->Render(apGeometry, apRenderer, v8, 1);
	pShader->SetTexture(0, 0, NiTexturingProperty::FILTER_NEAREST);
	pShader->SetTexture(1, 0, NiTexturingProperty::FILTER_NEAREST);
	pShader->SetTexture(2, 0, NiTexturingProperty::FILTER_NEAREST);
	BSTextureManager* v11 = BSShaderManager::pTextureManager;
	BSRenderedTexture* RenderedTexture = p_spRefractionNormalBuffer->GetRenderedTexture();
	v11->ReturnRenderedTexture(RenderedTexture);
	p_spRefractionNormalBuffer->ClearTexture();
}

// The exe's cleanup-only C++ EH frame is left out. The NiPointer temporary handed to SetAt has its constructor and
// destructor inlined; its stack slot first holds the allocation, then the constructed parameter.
void ImageSpaceEffectRefraction::Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) {
	FNVD_INLINED_LOCAL(NiPointer<ImageSpaceShaderParam>, spParam, std::nullopt, std::nullopt);
	ImageSpaceShaderParam* pparam;
	SetEffectCount(1);
	pTextures.SetSize(3u);
	ImageSpaceShaderParam* pMemory =
		static_cast<ImageSpaceShaderParam*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam)));
	spParam.m_pObject = pMemory;
	if (pMemory) {
		pparam = ::new (pMemory) ImageSpaceShaderParam;
	} else {
		pparam = 0;
	}
	pparam->InitConstantMap(0, 1);
	// The exe copies the constant to the stack and passes its components from there.
	NiPoint4 kTint = BSShaderLightingProperty::pShaderLightConstants[0];
	pparam->SetPixelConstant(0, kTint.X(), kTint.Y(), kTint.Z(), kTint.W());
	spParam.m_pObject = pparam;
	pEffectParamList.SetAt(0, spParam);
	SetEffectInputs(0, 1, 2, NiTexturingProperty::FILTER_NEAREST, NiTexturingProperty::FILTER_NEAREST);
	SetEffect(0, apManager->pEffectList.m_pBase[41], pparam, 0);
}

bool ImageSpaceEffectRefraction::IsActive() {
	return ((&spRefractionNormalBuffer)->GetTexture() != 0);
}

bool ImageSpaceEffectRefraction::UpdateParams(ImageSpaceEffectParam* apParam) {
	pEffectParamList.m_pBase->m_pObject->SetPixelConstant(0, BSShaderManager::pImageSpaceManager->kRefractionTint.r,
		BSShaderManager::pImageSpaceManager->kRefractionTint.g, BSShaderManager::pImageSpaceManager->kRefractionTint.b,
		BSShaderManager::pImageSpaceManager->kRefractionTint.a);
	return (bool)1;
}

BSRenderedTexture* ImageSpaceEffectRefraction::GetRefractionNormalsTexture(bool abCreate) {
	BSRenderedTexture* v3;
	ImageSpaceTexture* p_spRefractionNormalBuffer = &spRefractionNormalBuffer;
	if (!(&spRefractionNormalBuffer)->GetTexture()) {
		if (abCreate) {
			bCreateDiscardable = (uint32_t)0;
			v3 = BSShaderManager::pTextureManager
					 ->BorrowRenderedTexture(static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer),
						 BSTextureManager::TEX_REFRACTION_NORMALS, (uint32_t)0, nullptr, (uint32_t)0);
			p_spRefractionNormalBuffer->SetTexture(v3);
			bCreateDiscardable = (uint32_t)1;
		}
	}
	return p_spRefractionNormalBuffer->GetRenderedTexture();
}
