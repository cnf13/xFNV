#include "BSShader/ImageSpaceEffectBloom.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSTextureManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceEffectBlur.h"
#include "BSShader/ImageSpaceEffectParam.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShaderParam.h"
#include "BSShader/ImageSpaceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "SDK/MSVC/ehvec.h"

ImageSpaceEffectBloom::ImageSpaceEffectBloom() : pfData(nullptr) {
	pTextures.SetSize(4u);
}

// The three shader parameters are the effect's: each is deleted and its slot emptied.
ImageSpaceEffectBloom::~ImageSpaceEffectBloom() {
	for (uint32_t i = 0; i < 3; ++i) {
		if (pEffectParamList.m_pBase[i].m_pObject) {
			delete pEffectParamList.m_pBase[i].m_pObject;
			pEffectParamList.SetAt(i, nullptr);
		}
	}
}

void ImageSpaceEffectBloom::Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) {
	FNVD_INLINED_LOCAL(NiPointer<ImageSpaceShaderParam>, spParam, std::nullopt, std::nullopt);
	SetEffectCount(3);
	pTextures.SetSize(4u);
	ImageSpaceShaderParam* pParam;
	void* pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam));
	if (pMemory) {
		pParam = ::new (pMemory) ImageSpaceShaderParam;
	} else {
		pParam = nullptr;
	}
	pParam->InitConstantMap(0, 1);
	pParam->SetPixelConstant(0, 0.0f, f0_6999, f0_2, 0.0f);
	spParam.m_pObject = pParam;
	pEffectParamList.SetAt(0u, spParam);
	SetEffectInputs(0, 1, NiTexturingProperty::FILTER_BILERP);
	SetEffect(0, apManager->pEffectList.m_pBase[45], pParam, 2);
	spParam.m_pObject = nullptr;
	pEffectParamList.SetAt(1u, spParam);
	SetEffectInputs(1, 2, 3, NiTexturingProperty::FILTER_NEAREST, NiTexturingProperty::FILTER_NEAREST);
	SetEffect(1, apManager->pEffectList.m_pBase[19], nullptr, 2);
	ImageSpaceShaderParam* pParam2 = nullptr;
	pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam));
	if (pMemory) {
		pParam2 = ::new (pMemory) ImageSpaceShaderParam;
	}
	pParam2->InitConstantMap(0, 1);
	pParam2->bUseTexOffset[2] = false;
	pParam2->SetPixelConstant(0, 1.0f, 1.0f, 1.0f, 1.0f);
	spParam.m_pObject = pParam2;
	pEffectParamList.SetAt(2u, spParam);
	SetEffectInputs(2, 2, 1, NiTexturingProperty::FILTER_BILERP, NiTexturingProperty::FILTER_NEAREST);
	SetEffect(2, apManager->pEffectList.m_pBase[46], pParam2, 0);
}

void ImageSpaceEffectBloom::BorrowTextures(ImageSpaceEffectParam* pParam) {
	ImageSpaceTexture* ptex = pBlurBufferA;
	pBlurBufferA->BorrowRenderedTexture(BSTextureManager::TEX_BLUR20_BUFFER);
	pTextures.SetAt(2u, ptex);
	ptex = &pBlurBufferA[1];
	pBlurBufferA[1].BorrowRenderedTexture(BSTextureManager::TEX_BLUR20_BUFFER);
	pTextures.SetAt(3u, ptex);
}

// Hands back the two rendered textures in slots 2 and 3 and clears the slots.
void ImageSpaceEffectBloom::ReturnTextures() {
	pTextures.m_pBase[2]->ReturnRenderedTexture();
	pTextures.SetAt(2, nullptr);
	pTextures.m_pBase[3]->ReturnRenderedTexture();
	pTextures.SetAt(3, nullptr);
}

bool ImageSpaceEffectBloom::IsActive() {
	return (bool)((uint8_t)(BSShaderManager::bBlurShader && pfData));
}

// The buffer the effect was given holds a point: its y and z are the pixel constant's two middle lanes.
bool ImageSpaceEffectBloom::UpdateParams(ImageSpaceEffectParam* pParam) {
	double y;
	float a5;
	ImageSpaceShaderParam* pparam = pEffectParamList.m_pBase->m_pObject;
	if (this->pfData) {
		a5 = this->pfData[2];
		y = this->pfData[1];
	} else {
		a5 = f0_2;
		y = f0_6999;
	}
	pparam->SetPixelConstant(0, 0.0f, y, a5, 0.0f);
	ImageSpaceEffectBlur::fBlurRadius = 4.0f;
	return 1;
}

void ImageSpaceEffectBloom::SetParams(float* apfData) {
	bParamsChanged = (bool)1;
	pfData = apfData;
}
