#include "BSShader/ImageSpaceEffectVATSScan.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceEffectParam.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "BSShader/ImageSpaceShaderParam.h"
#include "BSShader/ImageSpaceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"

NiPointer<NiTexture> ImageSpaceEffectVATSScan::spScanlinesTexture;
NiPointer<NiTexture> ImageSpaceEffectVATSScan::spMaskTexture;
NiPointer<NiSourceTexture> ImageSpaceEffectVATSScan::PipboyDistortEffectMap;

ImageSpaceEffectVATSScan::ImageSpaceEffectVATSScan() {
	pTextures.SetSize(5u);
	bIsActive = false;
	kTint.r = 1.0f;
	kTint.a = 1.0f;
	kTint.g = 1.0f;
	kTint.b = 1.0f;
}

// Releases the shared scan-line, mask and distortion textures, frees the shader parameters and the effect's own
// texture slots 3, 4 and 2; the base destroys the rest. The parameter slot is cleared in place, as the object is
// going.
ImageSpaceEffectVATSScan::~ImageSpaceEffectVATSScan() {
	spScanlinesTexture = nullptr;
	spMaskTexture = nullptr;
	PipboyDistortEffectMap = nullptr;
	ImageSpaceShaderParam* pParam = pEffectParamList.m_pBase[0].m_pObject;
	if (pParam) {
		delete pParam;
		pEffectParamList.m_pBase[0].m_pObject = nullptr;
	}
	const uint32_t auiSlots[] = {3u, 4u, 2u};
	for (uint32_t uiSlot : auiSlots) {
		ImageSpaceTexture* pTexture = pTextures.m_pBase[uiSlot];
		if (pTexture) {
			pTexture->~ImageSpaceTexture();
			MemoryManager_Singleton.Deallocate(pTexture);
			pTextures.SetAt(uiSlot, nullptr);
		}
	}
}

// The exe's C++ EH (unwind) frame is left out: it only matters when a call in here throws.
void ImageSpaceEffectVATSScan::Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* /*apParam*/) {
	SetEffectCount(1u);
	pTextures.SetSize(5u);
	// GetTexture fills the distortion map's NiPointer<NiSourceTexture> through its NiPointer<NiTexture>& parameter,
	// as the exe does.
	BSShaderManager::GetTexture("Data\\Textures\\Pipboy3000\\PipboyScanlines.dds", true, spScanlinesTexture, false,
		false);
	NiTexture* pScanlines = spScanlinesTexture;
	BSShaderManager::GetTexture("Data\\Textures\\Pipboy3000\\PipboyDistortEffectMap.dds", true,
		reinterpret_cast<NiPointer<NiTexture>&>(PipboyDistortEffectMap), false, false);
	NiSourceTexture* pDistort = PipboyDistortEffectMap;

	// EXE-INLINED none: ImageSpaceTexture constructor
	ImageSpaceTexture* pTexture =
		static_cast<ImageSpaceTexture*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceTexture)));
	if (pTexture) {
		pTexture->bForceAniso = false;
		pTexture->bIsRenderedTexture = false;
		pTexture->bIsTextureBorrowed = false;
		pTexture->spTexture = nullptr;
		pTexture->eFilterMode = 0;
		pTexture->eClampMode = 0;
	} else {
		pTexture = nullptr;
	}
	pTextures.SetAt(TEXTURE_SCANLINES, pTexture);
	pTextures.m_pBase[TEXTURE_SCANLINES]->SetTexture(pScanlines);
	pTextures.m_pBase[TEXTURE_SCANLINES]->SetClampMode(NiTexturingProperty::WRAP_S_WRAP_T);

	// EXE-INLINED none: ImageSpaceTexture constructor
	pTexture = static_cast<ImageSpaceTexture*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceTexture)));
	if (pTexture) {
		pTexture->bForceAniso = false;
		pTexture->bIsRenderedTexture = false;
		pTexture->bIsTextureBorrowed = false;
		pTexture->spTexture = nullptr;
		pTexture->eFilterMode = 0;
		pTexture->eClampMode = 0;
	} else {
		pTexture = nullptr;
	}
	pTextures.SetAt(TEXTURE_SCANMASK, pTexture);
	pTextures.m_pBase[TEXTURE_SCANMASK]->SetTexture(pDistort);

	// EXE-INLINED none: ImageSpaceTexture constructor
	pTexture = static_cast<ImageSpaceTexture*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceTexture)));
	if (pTexture) {
		pTexture->bForceAniso = false;
		pTexture->bIsRenderedTexture = false;
		pTexture->bIsTextureBorrowed = false;
		pTexture->spTexture = nullptr;
		pTexture->eFilterMode = 0;
		pTexture->eClampMode = 0;
	} else {
		pTexture = nullptr;
	}
	pTextures.SetAt(TEXTURE_SRC_MASK, pTexture);
	pTextures.m_pBase[TEXTURE_SRC_MASK]->SetTexture(static_cast<NiSourceTexture*>(nullptr));

	// The effect list holds the shader's ImageSpaceEffect base (at +0x6C); the exe casts it down and back up.
	ImageSpaceShader* pShader =
		static_cast<ImageSpaceShader*>(apManager->pEffectList.m_pBase[ImageSpaceManager::EFFECT_VATS_SCAN_SHADER]);
	void* pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam));
	ImageSpaceShaderParam* pParam;
	if (pMemory) {
		pParam = ::new (pMemory) ImageSpaceShaderParam;
	} else {
		pParam = nullptr;
	}
	pParam->InitConstantMap(0, 3);
	pParam->iClearFlags = NiRenderer::CLEAR_BACKBUFFER;
	pParam->SetPixelConstant(1, fScanVerticalScale, fScanVerticalPosition, fScanlineFrequency, 0.0f);
	pParam->SetPixelConstant(2, kTint.r, kTint.g, kTint.b, kTint.a);
	pEffectParamList.SetAt(EFFECT_VATS_SCAN, reinterpret_cast<const NiPointer<ImageSpaceShaderParam>&>(pParam));
	SetEffectInputs(EFFECT_VATS_SCAN, TEXTURE_SRC_COLORS, TEXTURE_SRC_MASK, TEXTURE_SCANLINES, TEXTURE_SCANMASK,
		NiTexturingProperty::FILTER_NEAREST, NiTexturingProperty::FILTER_NEAREST, NiTexturingProperty::FILTER_NEAREST,
		NiTexturingProperty::FILTER_NEAREST);
	SetEffect(EFFECT_VATS_SCAN, pShader, pParam, 0);
}

void ImageSpaceEffectVATSScan::ReturnTextures() {
	ImageSpaceEffectVATSScan::spMaskTexture = nullptr;
	for (int32_t i = 0; i < 5; ++i) {
		ImageSpaceTexture* ptex = pTextures.m_pBase[i];
		if (ptex) {
			ptex->SetTexture(static_cast<NiTexture*>(0));
		}
	}
}

bool ImageSpaceEffectVATSScan::UpdateParams(ImageSpaceEffectParam* pParam) {
	pTextures.m_pBase[3]->SetTexture(ImageSpaceEffectVATSScan::spScanlinesTexture.m_pObject);
	pTextures.m_pBase[4]->SetTexture(ImageSpaceEffectVATSScan::PipboyDistortEffectMap.m_pObject);
	pTextures.m_pBase[2]->SetTexture(ImageSpaceEffectVATSScan::spMaskTexture.m_pObject);
	ImageSpaceShaderParam* pparam = pEffectParamList.m_pBase->m_pObject;
	pparam->SetPixelConstant(1, ImageSpaceEffectVATSScan::fScanVerticalScale,
		ImageSpaceEffectVATSScan::fScanVerticalPosition, ImageSpaceEffectVATSScan::fScanlineFrequency, 0.0f);
	pparam->SetPixelConstant(2, ImageSpaceEffectVATSScan::kTint.r, ImageSpaceEffectVATSScan::kTint.g,
		ImageSpaceEffectVATSScan::kTint.b, ImageSpaceEffectVATSScan::kTint.a);
	return 0;
}
