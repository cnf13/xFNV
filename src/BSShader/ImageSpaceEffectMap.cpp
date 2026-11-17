#include "BSShader/ImageSpaceEffectMap.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceEffectParam.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShaderParam.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"

ImageSpaceEffectMap::ImageSpaceEffectMap() {}

// One pass with a single colour-write-all shader parameter, drawn by the manager's effect 48. The exe's cleanup-only
// EH frame around the allocation is left out.
void ImageSpaceEffectMap::Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) {
	SetEffectCount(1);
	pTextures.SetSize(2);
	void* pMemory = (&MemoryManager_Singleton)->Allocate(sizeof(ImageSpaceShaderParam));
	ImageSpaceShaderParam* pParam;
	if (pMemory)
		pParam = ::new (pMemory) ImageSpaceShaderParam;
	else
		pParam = nullptr;
	pParam->InitConstantMap(0, 1);
	pParam->SetPixelConstant(0, 1.0f, 1.0f, 1.0f, 1.0f);
	// D3DRS_COLORWRITEENABLE (0xA8): all four channels.
	pParam->SetRenderState(0xA8, 0xF);
	ImageSpaceShaderParam* pElement = pParam;
	pEffectParamList.SetAt(0, reinterpret_cast<const NiPointer<ImageSpaceShaderParam>&>(pElement));
	SetEffectInputs(0, 1, NiTexturingProperty::FILTER_NEAREST);
	// EXE-INLINED none: ImageSpaceManager::GetEffect (the effect list's base read at index 48)
	SetEffect(0, apManager->pEffectList.m_pBase[48], pParam, 0);
}

bool ImageSpaceEffectMap::IsActive() {
	return pEffectList.m_pBase->m_pObject->IsActive();
}

// The exe converts each unsigned size as a signed fild plus the float 2^32 when negative, then divides 1 by it
// before storing the float.
bool ImageSpaceEffectMap::UpdateParams(ImageSpaceEffectParam* apParam) {
	ImageSpaceShaderParam* pParam = pEffectParamList.m_pBase->m_pObject;
	NiTexture* pTexture = pTextures.m_pBase[1]->GetTexture();
	int32_t iHeight = static_cast<int32_t>(pTexture->GetHeight());
	double dHeight = iHeight;
	if (iHeight < 0) {
		dHeight += 4294967296.0f;
	}
	float fInvHeight = 1.0 / dHeight;
	int32_t iWidth = static_cast<int32_t>(pTexture->GetWidth());
	double dWidth = iWidth;
	if (iWidth < 0) {
		dWidth += 4294967296.0f;
	}
	float fInvWidth = 1.0 / dWidth;
	pParam->SetPixelConstant(0, fInvWidth, fInvHeight, 0.0f, 0.0f);
	return 1;
}

// The first shader parameter is the effect's: it is deleted and its slot emptied.
ImageSpaceEffectMap::~ImageSpaceEffectMap() {
	if (pEffectParamList.m_pBase[0].m_pObject) {
		delete pEffectParamList.m_pBase[0].m_pObject;
		pEffectParamList.SetAt(0, nullptr);
	}
}
