#include "BSShader/ImageSpaceEffectTV.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceEffectParam.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShaderParam.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"

NiPointer<NiTexture> ImageSpaceEffectTV::spScanlinesTexture;
NiPointer<NiTexture> ImageSpaceEffectTV::spDistortTexture;

ImageSpaceEffectTV::ImageSpaceEffectTV() {
	pTextures.SetSize(4u);
}

// Releases the shared scan-line and distortion textures, frees the effect's own texture slots 1 to 3 and its
// shader parameters; the base destroys the rest. The parameter slot is cleared in place, as the object is going.
ImageSpaceEffectTV::~ImageSpaceEffectTV() {
	spScanlinesTexture = nullptr;
	spDistortTexture = nullptr;
	for (uint32_t i = 1; i <= 3; ++i) {
		ImageSpaceTexture* pTexture = pTextures.m_pBase[i];
		if (pTexture) {
			pTexture->~ImageSpaceTexture();
			MemoryManager_Singleton.Deallocate(pTexture);
			pTextures.SetAt(i, nullptr);
		}
	}
	ImageSpaceShaderParam* pParam = pEffectParamList.m_pBase[0].m_pObject;
	if (pParam) {
		delete pParam;
		pEffectParamList.m_pBase[0].m_pObject = nullptr;
	}
}

bool ImageSpaceEffectTV::IsActive() {
	return false;
}

// The exe's cleanup-only C++ EH frame is left out.
void ImageSpaceEffectTV::Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) {
	FNVD_INLINED_LOCAL(NiPointer<ImageSpaceShaderParam>, spParam, std::nullopt, std::nullopt);
	SetEffectCount(1);
	NiTPrimitiveArray<ImageSpaceTexture*>* pTextureList = &pTextures;
	pTextures.SetSize(4u);
	BSShaderManager::GetTexture("Data\\Textures\\Pipboy3000\\PipboyDistortEffectMap.dds", 1,
		ImageSpaceEffectTV::spDistortTexture, 0, 0);
	BSShaderManager::GetTexture("Data\\Textures\\Pipboy3000\\PipboyScanlines.dds", 1,
		ImageSpaceEffectTV::spScanlinesTexture, 0, 0);
	NiTexture* pDistortTexture = ImageSpaceEffectTV::spDistortTexture.m_pObject;
	NiTexture* pScanlineTexture = ImageSpaceEffectTV::spScanlinesTexture.m_pObject;
	ImageSpaceTexture* pTexture =
		static_cast<ImageSpaceTexture*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceTexture)));
	if (pTexture) {
		pTexture->bForceAniso = 0;
		pTexture->bIsRenderedTexture = 0;
		pTexture->bIsTextureBorrowed = 0;
		pTexture->spTexture = 0;
		pTexture->eFilterMode = 0;
		pTexture->eClampMode = 0;
	} else {
		pTexture = 0;
	}
	pTextureList->SetAt(1u, pTexture);
	pTextures.m_pBase[1]->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
	pTextures.m_pBase[1]->SetTexture(static_cast<NiTexture*>(0));
	pTexture = static_cast<ImageSpaceTexture*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceTexture)));
	if (pTexture) {
		pTexture->bForceAniso = 0;
		pTexture->bIsRenderedTexture = 0;
		pTexture->bIsTextureBorrowed = 0;
		pTexture->spTexture = 0;
		pTexture->eFilterMode = 0;
		pTexture->eClampMode = 0;
	} else {
		pTexture = 0;
	}
	pTextureList->SetAt(2u, pTexture);
	pTextures.m_pBase[2]->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
	pTextures.m_pBase[2]->SetTexture(pDistortTexture);
	pTexture = static_cast<ImageSpaceTexture*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceTexture)));
	if (pTexture) {
		pTexture->bForceAniso = 0;
		pTexture->bIsRenderedTexture = 0;
		pTexture->bIsTextureBorrowed = 0;
		pTexture->spTexture = 0;
		pTexture->eFilterMode = 0;
		pTexture->eClampMode = 0;
	} else {
		pTexture = 0;
	}
	pTextureList->SetAt(3u, pTexture);
	pTextures.m_pBase[3]->SetClampMode(NiTexturingProperty::WRAP_S_WRAP_T);
	pTextures.m_pBase[3]->SetTexture(pScanlineTexture);
	ImageSpaceTexture** m_pBase = pTextures.m_pBase;
	float foffsetx = 0.0f;
	float foffsety = 0.0f;
	// The exe converts each unsigned size as a signed fild plus the float 2^32 when negative; that fadd rounds at
	// the thread's precision control (24 bits under Direct3D), where a 64-bit fild of the size would not.
	if (*m_pBase) {
		NiTexture* ptex = (*m_pBase)->GetTexture();
		int32_t iWidth = static_cast<int32_t>(ptex->GetWidth());
		double dWidth = iWidth;
		if (iWidth < 0) {
			dWidth += 4294967296.0f;
		}
		foffsetx = 1.0 / dWidth;
		int32_t iHeight = static_cast<int32_t>(ptex->GetHeight());
		double dHeight = iHeight;
		if (iHeight < 0) {
			dHeight += 4294967296.0f;
		}
		foffsety = 1.0 / dHeight;
	}
	ImageSpaceShaderParam* pMemory =
		static_cast<ImageSpaceShaderParam*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam)));
	ImageSpaceShaderParam* pparam;
	if (pMemory) {
		pparam = ::new (pMemory) ImageSpaceShaderParam;
	} else {
		pparam = 0;
	}
	pparam->InitConstantMap(0, 5);
	pparam->bUseTexOffset[1] = 0;
	pparam->bUseTexOffset[2] = 0;
	pparam->SetPixelConstant(0, foffsetx, foffsety, 0.0f, 0.0f);
	pparam->SetPixelConstant(1, ImageSpaceEffectTV::fPulse, 0.0f, 0.0f, 0.0f);
	pparam->SetPixelConstant(2, ImageSpaceEffectTV::fDistortVerticalScale, ImageSpaceEffectTV::fDistortHorizontalScale,
		ImageSpaceEffectTV::fDistortVerticalPosition, 0.0f);
	pparam->SetPixelConstant(3, ImageSpaceEffectTV::fFakeDistortVerticalScale,
		ImageSpaceEffectTV::fFakeDistortHorizontalScale, ImageSpaceEffectTV::fFakeDistortVerticalPosition, 0.0f);
	pparam->SetPixelConstant(4, ImageSpaceEffectTV::Tint.r, ImageSpaceEffectTV::Tint.g, ImageSpaceEffectTV::Tint.b,
		1.0f);
	spParam.m_pObject = pparam;
	pEffectParamList.SetAt(0, spParam);
	SetEffectInputs(0, 1, 2, 3, NiTexturingProperty::FILTER_NEAREST, NiTexturingProperty::FILTER_NEAREST,
		NiTexturingProperty::FILTER_NEAREST);
	SetEffect(0, apManager->pEffectList.m_pBase[77], pparam, 0);
}

void ImageSpaceEffectTV::ReturnTextures() {
	// The exe calls the NiSourceTexture* overload (0xBA37F0) with null.
	pTextures.m_pBase[1]->SetTexture(static_cast<NiSourceTexture*>(nullptr));
}

bool ImageSpaceEffectTV::UpdateParams(ImageSpaceEffectParam* pParam) {
	double v2 = 0.0;
	float v11 = 0.0;
	float v10 = 0.0;
	ImageSpaceTexture** m_pBase = pTextures.m_pBase;
	// The exe's unsigned-to-float sequence, as in Setup.
	if (m_pBase[1]) {
		NiTexture* Texture = m_pBase[1]->GetTexture();
		int32_t iWidth = static_cast<int32_t>(Texture->GetWidth());
		double dWidth = iWidth;
		if (iWidth < 0) {
			dWidth += 4294967296.0f;
		}
		v11 = 1.0 / dWidth;
		int32_t iHeight = static_cast<int32_t>(Texture->GetHeight());
		double dHeight = iHeight;
		if (iHeight < 0) {
			dHeight += 4294967296.0f;
		}
		v10 = 1.0 / dHeight;
		v2 = 0.0;
	}
	ImageSpaceShaderParam* m_pObject = pEffectParamList.m_pBase->m_pObject;
	float a6 = v2;
	float a5 = v2;
	m_pObject->SetPixelConstant(0, v11, v10, a5, a6);
	m_pObject->SetPixelConstant(1, ImageSpaceEffectTV::fPulse, 0.0, 0.0, 0.0);
	m_pObject->SetPixelConstant(2, ImageSpaceEffectTV::fDistortVerticalScale,
		ImageSpaceEffectTV::fDistortHorizontalScale, ImageSpaceEffectTV::fDistortVerticalPosition, 0.0);
	m_pObject->SetPixelConstant(3, ImageSpaceEffectTV::fFakeDistortVerticalScale,
		ImageSpaceEffectTV::fFakeDistortHorizontalScale, ImageSpaceEffectTV::fFakeDistortVerticalPosition, 0.0);
	m_pObject->SetPixelConstant(4, ImageSpaceEffectTV::Tint.r, ImageSpaceEffectTV::Tint.g, ImageSpaceEffectTV::Tint.b,
		1.0);
	return 0;
}
