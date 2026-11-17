#include "BSShader/ImageSpaceEffectDepthOfField.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/RendererSettingCollection.h"
#include "BSMain/Setting.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSTextureManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceEffectBlur.h"
#include "BSShader/ImageSpaceEffectHDR.h"
#include "BSShader/ImageSpaceEffectParam.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShaderParam.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElements.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"

// The game settings this file defines (PDB), with the IDs and values the exe's initializers pass, in its initialization order.
SettingT<RendererSettingCollection> bDoDepthOfField("bDoDepthOfField:Imagespace", true);
SettingT<INISettingCollection> fRenderDepthMaxDepth("fRenderDepthMaxDepth:Imagespace", 10000.0f);

ImageSpaceEffectOption::ImageSpaceEffectOption() : pEffectOn(0, 1) {}

// The switch array frees its own buffer.
ImageSpaceEffectOption::~ImageSpaceEffectOption() {}

ImageSpaceEffectDepthOfField::ImageSpaceEffectDepthOfField()
	: fDepth(0.0f), fRange(0.0f), fStrength(0.0f), Mode(DOF_FRONT_BACK), MaskBuffer() {
	pTextures.SetSize(6u);
	pEffectOn.SetSize(4u);
	DoStandard();
}

// The four shader parameters are the effect's: each is deleted and its slot emptied, in the exe's order.
ImageSpaceEffectDepthOfField::~ImageSpaceEffectDepthOfField() {
	static const uint32_t kOrder[] = {2u, 1u, 0u, 3u};
	for (uint32_t uiSlot : kOrder) {
		if (pEffectParamList.m_pBase[uiSlot].m_pObject) {
			delete pEffectParamList.m_pBase[uiSlot].m_pObject;
			pEffectParamList.SetAt(uiSlot, nullptr);
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out. Each SetAt gets the pointer through one stack slot, a NiPointer
// temporary whose constructor and destructor the exe inlines down to the pointer store (the parameter class is not
// a NiRefObject, so src's NiPointer structors do not compile for it); the exe also keeps each fresh allocation there.
void ImageSpaceEffectDepthOfField::Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) {
	ImageSpaceShaderParam* pparam;
	ImageSpaceShaderParam* pparam_3;
	FNVD_INLINED_LOCAL(NiPointer<ImageSpaceShaderParam>, spParam, std::nullopt, std::nullopt);
	SetEffectCount(4);
	// The exe converts each unsigned size as a signed fild plus the float 2^32 when negative; that fadd rounds at
	// the thread's precision control (24 bits under Direct3D), where a 64-bit fild of the size would not.
	NiRenderTargetGroup* v4 = BSShaderManager::pRenderer.m_pObject->GetDefaultRenderTargetGroup();
	int32_t iWidth = static_cast<int32_t>(v4->GetWidth(0));
	double dWidth = iWidth;
	if (iWidth < 0) {
		dWidth += 4294967296.0f;
	}
	float fblurwidth = dWidth;
	NiRenderTargetGroup* v5 = BSShaderManager::pRenderer.m_pObject->GetDefaultRenderTargetGroup();
	int32_t iHeight = static_cast<int32_t>(v5->GetHeight(0));
	double dHeight = iHeight;
	if (iHeight < 0) {
		dHeight += 4294967296.0f;
	}
	float fblurheight = dHeight;
	ImageSpaceShaderParam* v6 =
		static_cast<ImageSpaceShaderParam*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam)));
	if (v6) {
		pparam = ::new (v6) ImageSpaceShaderParam;
	} else {
		pparam = 0;
	}
	pparam->InitConstantMap(0, 24);
	pparam->CopyConstantGroup(3, kHDRConstants11ADD48, 64u);
	float v15 = 1.0 / fblurheight;
	float afVal0 = 1.0 / fblurwidth;
	pparam->SetPixelConstant(2, afVal0, v15, 0.0f, 0.0f);
	pparam->bUseTexOffset[1] = 0;
	SetEffect(0, apManager->pEffectList.m_pBase[65], pparam, 2);
	spParam.m_pObject = pparam;
	NiTPrimitiveArray<NiPointer<ImageSpaceShaderParam>>* p_pEffectParamList = &pEffectParamList;
	ImageSpaceShaderParam* pparam_2 = 0;
	pEffectParamList.SetAt(0, spParam);
	SetEffectInputs(0, 1, NiTexturingProperty::FILTER_BILERP);
	spParam.m_pObject = nullptr;
	pEffectParamList.SetAt(1u, spParam);
	SetEffectInputs(1, 2, 3, NiTexturingProperty::FILTER_NEAREST, NiTexturingProperty::FILTER_NEAREST);
	SetEffect(1, apManager->pEffectList.m_pBase[18], 0, 2);
	ImageSpaceShaderParam* v11 =
		static_cast<ImageSpaceShaderParam*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam)));
	spParam.m_pObject = v11;
	if (v11) {
		pparam_2 = ::new (v11) ImageSpaceShaderParam;
	}
	pparam_2->InitConstantMap(0, 3);
	pparam_2->SetPixelConstant(0, 1.0f, 1000.0f, 500.0f, fRenderDepthMaxDepth.uValue.f);
	pparam_2->bUseTexOffset[2] = 0;
	spParam.m_pObject = pparam_2;
	p_pEffectParamList->SetAt(2u, spParam);
	SetEffectInputs(2, 1, 2, 4, NiTexturingProperty::FILTER_NEAREST, NiTexturingProperty::FILTER_BILERP,
		NiTexturingProperty::FILTER_NEAREST);
	SetEffect(2, apManager->pEffectList.m_pBase[51], pparam_2, 0);
	ImageSpaceShaderParam* v12 =
		static_cast<ImageSpaceShaderParam*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam)));
	spParam.m_pObject = v12;
	if (v12) {
		pparam_3 = ::new (v12) ImageSpaceShaderParam;
	} else {
		pparam_3 = 0;
	}
	pparam_3->InitConstantMap(0, 4);
	pparam_3->SetPixelConstant(0, 1.0f, 1000.0f, 500.0f, fRenderDepthMaxDepth.uValue.f);
	pparam_3->bUseTexOffset[2] = 0;
	spParam.m_pObject = pparam_3;
	p_pEffectParamList->SetAt(3u, spParam);
	SetEffectInputs(3, 1, 2, 4, 5, NiTexturingProperty::FILTER_NEAREST, NiTexturingProperty::FILTER_BILERP,
		NiTexturingProperty::FILTER_NEAREST, NiTexturingProperty::FILTER_NEAREST);
	SetEffect(3, apManager->pEffectList.m_pBase[52], pparam_3, 0);
}

// Each SetAt gets the texture through one stack slot. With the mask effect on, slot 5 gets the mask buffer as it
// is; with it off, the mask is returned and slot 5 cleared.
void ImageSpaceEffectDepthOfField::BorrowTextures(ImageSpaceEffectParam* pParam) {
	ImageSpaceTexture* ptex = pBufferA;
	pBufferA->BorrowRenderedTexture(BSTextureManager::TEX_DOF_BLUR);
	pTextures.SetAt(2u, ptex);
	ptex = &pBufferA[1];
	pBufferA[1].BorrowRenderedTexture(BSTextureManager::TEX_DOF_BLUR_SWAP);
	pTextures.SetAt(3u, ptex);
	if (pEffectOn.m_pBase[3]) {
		ptex = &MaskBuffer;
	} else {
		MaskBuffer.ReturnRenderedTexture();
		ptex = nullptr;
	}
	pTextures.SetAt(5u, ptex);
	ImageSpaceTexture* ptex_2 = &pBufferA[2];
	ptex = ptex_2;
	NiTexture* SceneDepthTexture = BSShaderManager::GetSceneDepthTexture();
	ptex_2->SetTexture(SceneDepthTexture);
	pTextures.SetAt(4u, ptex);
}

void ImageSpaceEffectDepthOfField::ReturnTextures() {
	pTextures.m_pBase[2]->ReturnRenderedTexture();
	ImageSpaceTexture* a3 = 0;
	pTextures.SetAt(2u, a3);
	pTextures.m_pBase[3]->ReturnRenderedTexture();
	a3 = 0;
	pTextures.SetAt(3u, a3);
	a3 = 0;
	pTextures.SetAt(4u, a3);
	if (pTextures.m_pBase[5]) {
		MaskBuffer.ReturnRenderedTexture();
		a3 = 0;
		pTextures.SetAt(5u, a3);
	}
}

bool ImageSpaceEffectDepthOfField::IsActive() {
	return fStrength > 0.0f && (bDoDepthOfField.uValue.b && BSShaderManager::GetSceneDepthTexture());
}

bool ImageSpaceEffectDepthOfField::UpdateParams(ImageSpaceEffectParam* apParam) {
	float fblurfront;
	float fnear = BSShaderManager::spCamera->m_kViewFrustum.m_fNear;
	ImageSpaceEffectDepthOfField::DepthOfFieldMode Mode = this->Mode;
	double ffar = BSShaderManager::spCamera->m_kViewFrustum.m_fFar;
	float ffar_near = ffar - fnear;
	float ffarnear = ffar * fnear;
	double fblurback = 1.0;
	if (Mode == ImageSpaceEffectDepthOfField::DOF_FRONT_BACK || Mode == ImageSpaceEffectDepthOfField::DOF_FRONT) {
		fblurfront = 1.0f;
	} else {
		fblurfront = 0.0f;
	}
	if (Mode != ImageSpaceEffectDepthOfField::DOF_BACK) {
		if (Mode) {
			fblurback = 0.0;
		}
	}
	float fblurback_2 = fblurback;
	ImageSpaceShaderParam* pparam = pEffectParamList.m_pBase[2].m_pObject;
	float fmindistance = fDepth - fRange;
	float fmaxdistance = fRange + fDepth;
	pparam->SetPixelConstant(0, fmindistance, fmaxdistance, fDepth, fRenderDepthMaxDepth.uValue.f);
	pparam->SetPixelConstant(1, fStrength, fblurfront, fblurback_2, 0.0f);
	pparam->SetPixelConstant(2, -1e+08f, fnear, ffar_near, ffarnear);
	ImageSpaceShaderParam* pparam_2 = pEffectParamList.m_pBase[3].m_pObject;
	pparam_2->SetPixelConstant(0, fmindistance, fmaxdistance, fDepth, fRenderDepthMaxDepth.uValue.f);
	pparam_2->SetPixelConstant(1, fStrength, fblurfront, fblurback_2, 0.0f);
	pparam_2->SetPixelConstant(2, -1e+08f, fnear, ffar_near, ffarnear);
	if (BSShaderManager::fTimerValues[1] > 0.0f) {
		float fblurconst = fMaskConst / (BSShaderManager::fTimerValues[1] * 30.0);
		float v21 = -BSShaderManager::spCamera->m_kWorld.m_Rotate.m_pEntry[0][1];
		float v22 = -BSShaderManager::spCamera->m_kWorld.m_Rotate.m_pEntry[1][1];
		float v23 = -BSShaderManager::spCamera->m_kWorld.m_Rotate.m_pEntry[2][1];
		float v19 = v21 * BSShaderProperty::PreviousTransform::vecPrevCameraDir.x +
			BSShaderProperty::PreviousTransform::vecPrevCameraDir.y * v22 +
			BSShaderProperty::PreviousTransform::vecPrevCameraDir.z * v23;
		float v20 = BSShaderProperty::PreviousTransform::vecPrevCameraDir.z *
				BSShaderManager::spCamera->m_kWorld.m_Rotate.m_pEntry[2][2] +
			(BSShaderProperty::PreviousTransform::vecPrevCameraDir.x *
					BSShaderManager::spCamera->m_kWorld.m_Rotate.m_pEntry[0][2] +
				BSShaderProperty::PreviousTransform::vecPrevCameraDir.y *
					BSShaderManager::spCamera->m_kWorld.m_Rotate.m_pEntry[1][2]);
		pparam_2->SetPixelConstant(3, v20, v19, fblurconst, 0.0f);
	}
	ImageSpaceEffectBlur::fBlurRadius = 3.0f;
	return 1;
}

BSRenderedTexture* ImageSpaceEffectDepthOfField::BorrowMaskTexture() {
	(&MaskBuffer)->BorrowRenderedTexture(BSTextureManager::TEX_DOF_MASK);
	return (&MaskBuffer)->GetRenderedTexture();
}

void ImageSpaceEffectDepthOfField::ReturnMaskTexture() {
	(&MaskBuffer)->ReturnRenderedTexture();
}

bool ImageSpaceEffectDepthOfField::DoDepth() const {
	return (bool)((uint8_t)(fStrength > 0.0));
}
