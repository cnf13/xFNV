#include "BSShader/ImageSpaceEffectRadialBlur.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSTextureManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceEffectParam.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShaderParam.h"
#include "BSShader/ImageSpaceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/string.h"

#include <cmath>

// The game settings this file defines (PDB), with the IDs and values the exe's initializers pass, in its initialization order.
SettingT<INISettingCollection> bDoRadialBlur("bDoRadialBlur:Imagespace", true);
SettingT<RendererSettingCollection> iRadialBlurLevel("iRadialBlurLevel:Imagespace", 0);

// Params.a starts at the fRenderDepthMaxDepth setting; fMaskConst is left unset, as in the exe.
ImageSpaceEffectDepthRadialBlur::ImageSpaceEffectDepthRadialBlur()
	: Mode(DOF_FRONT_BACK), Params3(0.0f, 0.0f, 0.0f, 0.0f), Params2(0.0f, 1.0f, 1.0f, 0.0f),
	  Params(1.0f, 1000.0f, 0.0f, fRenderDepthMaxDepth.uValue.f), RadialParams(0.0f, 0.0f, 0.0f, 0.0f), fRange(600.0f),
	  MaskBuffer() {
	pTextures.SetSize(4u);
}

// The blur texture (slot 2) and the shader parameters in slots 1 and 0 are the effect's.
ImageSpaceEffectDepthRadialBlur::~ImageSpaceEffectDepthRadialBlur() {
	if (ImageSpaceTexture* pTexture = pTextures.m_pBase[2]) {
		pTexture->~ImageSpaceTexture();
		MemoryManager_Singleton.Deallocate(pTexture);
		pTextures.SetAt(2u, nullptr);
	}
	for (int32_t i = 1; i >= 0; --i) {
		if (pEffectParamList.m_pBase[i].m_pObject) {
			delete pEffectParamList.m_pBase[i].m_pObject;
			pEffectParamList.SetAt(static_cast<uint32_t>(i), nullptr);
		}
	}
}

// Two effects with new shader parameters: the radial blur shader of the configured level (2 pixel constants) and
// the depth of field shader (3 pixel constants, no offset on texture 2). The exe passes each parameters object to
// SetAt in one stack slot as the array's element, its NiPointer structors inlined (the parameter class is not a
// NiRefObject), and also keeps the second fresh allocation there. The exe's cleanup-only C++ EH frame is left out.
void ImageSpaceEffectDepthRadialBlur::Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) {
	ImageSpaceShaderParam* pparam;
	ImageSpaceShaderParam* pparam_2;
	FNVD_INLINED_LOCAL(NiPointer<ImageSpaceShaderParam>, spParam, std::nullopt, std::nullopt);
	SetEffectCount(2);
	ImageSpaceShaderParam* v4 =
		static_cast<ImageSpaceShaderParam*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam)));
	if (v4) {
		pparam = ::new (v4) ImageSpaceShaderParam;
	} else {
		pparam = 0;
	}
	pparam->InitConstantMap(0, 2);
	pparam->SetPixelConstant(0, 0.1f, 0.0f, 0.0f, 0.0f);
	SetEffect(0,
		apManager->pEffectList.m_pBase[iRadialBlurLevel.uValue.uint + ImageSpaceManager::EFFECT_SHADER_RADIAL_BLUR],
		pparam, 2);
	spParam.m_pObject = pparam;
	pEffectParamList.SetAt(0, spParam);
	SetEffectInputs(0, 1, NiTexturingProperty::FILTER_BILERP);
	ImageSpaceShaderParam* v7 =
		static_cast<ImageSpaceShaderParam*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam)));
	spParam.m_pObject = v7;
	if (v7) {
		pparam_2 = ::new (v7) ImageSpaceShaderParam;
	} else {
		pparam_2 = 0;
	}
	pparam_2->InitConstantMap(0, 3);
	pparam_2->SetPixelConstant(0, 1.0f, 1000.0f, 500.0f, fRenderDepthMaxDepth.uValue.f);
	pparam_2->bUseTexOffset[2] = 0;
	spParam.m_pObject = pparam_2;
	pEffectParamList.SetAt(1u, spParam);
	SetEffectInputs(1, 1, 2, 3, NiTexturingProperty::FILTER_NEAREST, NiTexturingProperty::FILTER_BILERP,
		NiTexturingProperty::FILTER_NEAREST);
	SetEffect(1, apManager->pEffectList.m_pBase[ImageSpaceManager::EFFECT_SHADER_DEPTH_OF_FIELD], pparam_2, 0);
}

// Borrows the blur buffer as texture 2 and binds the scene depth texture to the second buffer as texture 3.
void ImageSpaceEffectDepthRadialBlur::BorrowTextures(ImageSpaceEffectParam* pParam) {
	ImageSpaceTexture* ptex = &pBufferA[0];
	ptex->BorrowRenderedTexture(BSTextureManager::TEX_DEPTH_RADIALBLUR_BLUR);
	pTextures.SetAt(2u, ptex);
	ptex = &pBufferA[1];
	ptex->SetTexture(BSShaderManager::GetSceneDepthTexture());
	pTextures.SetAt(3u, ptex);
}

// Hands back the blur buffer and clears texture slots 2 and 3.
void ImageSpaceEffectDepthRadialBlur::ReturnTextures() {
	pTextures.m_pBase[2]->ReturnRenderedTexture();
	pTextures.SetAt(2u, nullptr);
	pTextures.SetAt(3u, nullptr);
}

// The exe reads the setting's value as a raw byte (cmp byte, 0) and returns 0 or 1, never the byte itself.
bool ImageSpaceEffectDepthRadialBlur::IsActive() {
	return Params2.r > 0.0f && reinterpret_cast<const uint8_t&>(bDoRadialBlur.uValue.b) != 0;
}

bool ImageSpaceEffectDepthRadialBlur::UpdateParams(ImageSpaceEffectParam* apParam) {
	ImageSpaceShaderParam* pparam = pEffectParamList.m_pBase[1].m_pObject;
	float fnear = BSShaderManager::spCamera->m_kViewFrustum.m_fNear;
	float ffar = BSShaderManager::spCamera->m_kViewFrustum.m_fFar;
	Params3.g = fnear;
	Params3.b = ffar - fnear;
	Params3.a = fnear * ffar;
	pparam->SetPixelConstant(0, Params.r, Params.g, Params.b, Params.a);
	pparam->SetPixelConstant(1, Params2.r, Params2.g, Params2.b, Params2.a);
	pparam->SetPixelConstant(2, Params3.r, Params3.g, Params3.b, Params3.a);
	ImageSpaceShaderParam* pparam_2 = pEffectParamList.m_pBase->m_pObject;
	pparam_2->SetPixelConstant(0, RadialParams.r, RadialParams.g, RadialParams.b, 0.0f);
	pparam_2->SetPixelConstant(1, 0.5f, 0.5f, 0.0f, 0.0f);
	return 1;
}

ImageSpaceEffectRadialBlur::ImageSpaceEffectRadialBlur() : Center(0.5f, 0.5f) {
	pTextures.SetSize(2u);
}

// The first shader parameter is the effect's: it is deleted and its slot emptied.
ImageSpaceEffectRadialBlur::~ImageSpaceEffectRadialBlur() {
	if (pEffectParamList.m_pBase[0].m_pObject) {
		delete pEffectParamList.m_pBase[0].m_pObject;
		pEffectParamList.SetAt(0, nullptr);
	}
}

// One effect with new shader parameters (2 pixel constants): the manager's radial blur shader of the configured
// level. The exe passes the parameters to SetAt in a stack slot as the array's element, its NiPointer structors
// inlined (the parameter class is not a NiRefObject). The exe's cleanup-only C++ EH frame is left out.
void ImageSpaceEffectRadialBlur::Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) {
	ImageSpaceShaderParam* pparam;
	FNVD_INLINED_LOCAL(NiPointer<ImageSpaceShaderParam>, spParam, std::nullopt, std::nullopt);
	SetEffectCount(1);
	ImageSpaceShaderParam* v4 =
		static_cast<ImageSpaceShaderParam*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam)));
	if (v4) {
		pparam = ::new (v4) ImageSpaceShaderParam;
	} else {
		pparam = 0;
	}
	pparam->InitConstantMap(0, 2);
	pparam->SetPixelConstant(0, 0.1f, 0.0f, 0.0f, 0.0f);
	SetEffect(0,
		apManager->pEffectList.m_pBase[iRadialBlurLevel.uValue.uint + ImageSpaceManager::EFFECT_SHADER_RADIAL_BLUR],
		pparam, 0);
	spParam.m_pObject = pparam;
	pEffectParamList.SetAt(0, spParam);
	SetEffectInputs(0, 1, NiTexturingProperty::FILTER_NEAREST);
}

// The exe reads the setting's value as a raw byte (cmp byte, 0) and returns 0 or 1, never the byte itself.
bool ImageSpaceEffectRadialBlur::IsActive() {
	return fStrength > 0.0f && reinterpret_cast<const uint8_t&>(bDoRadialBlur.uValue.b) != 0;
}

bool ImageSpaceEffectRadialBlur::UpdateParams(ImageSpaceEffectParam* apParam) {
	ImageSpaceShaderParam* v3 = pEffectParamList.m_pBase->m_pObject;
	v3->SetPixelConstant(0, fStrength, fRampup, fStart, fRampdown);
	v3->SetPixelConstant(1, Center.x, Center.y, fDownstart, 0.0);
	return (bool)1;
}

void ImageSpaceEffectRadialBlur::GetBoundParams(const NiBound& aBound, NiPoint2& aCenter, float& afStartBias) {
	double v9;
	double v10;
	double v11;
	double v12;
	double v14;
	double v16;
	double v17;
	NiPoint2 v31;
	float pvDest[4][4];
	NiPoint3 v49 = aBound.m_kCenter;
	NiRect<float> m_kPort = ImageSpaceEffectRadialBlur::pWorldCamera->m_kPort;
	rt::memcpy(pvDest, ImageSpaceEffectRadialBlur::pWorldCamera->m_aafWorldToCam, sizeof(pvDest));
	double v45 = pvDest[3][0] * v49.x + pvDest[3][1] * v49.y;
	float v24 = v45 + pvDest[3][2] * v49.z + pvDest[3][3];
	bool v3 = v24 < 0.0f;
	float v25 = std::fabs((double)v24);
	float v22 = 1.0 / v25;
	double v4 = pvDest[0][1] * v49.y + pvDest[0][0] * v49.x;
	float v26 = pvDest[0][2] * v49.z + v4 + pvDest[0][3];
	double v5 = v22;
	float v23 = v26 * v22;
	double v6 = v49.x * pvDest[1][0] + v49.y * pvDest[1][1];
	float v27 = pvDest[1][2] * v49.z + v6 + pvDest[1][3];
	float v28 = v5 * v27;
	double v7 = v28;
	if ((!(v23 >= -1.0f) || (!(v23 <= 1.0f) || (!(v7 >= -1.0f) || !(v7 <= 1.0)))) && v3) {
		v11 = v28;
		afStartBias = 0.3f;
		v9 = 0.5;
		v12 = 1.0;
		v10 = v23;
	} else {
		float v44 = v49.z + aBound.m_fRadius;
		float v33 = pvDest[3][2] * v44 + v45 + pvDest[3][3];
		float v46 = 1.0 - v7;
		float v29 = v4 + pvDest[0][2] * v44 + pvDest[0][3];
		float v30 = v29 / v33;
		double v8 = v33;
		float v34 = v6 + v44 * pvDest[1][2] + pvDest[1][3];
		float v35 = v34 / v8;
		float v32 = 1.0 - v35;
		float v36 = v23 - v30;
		float v43 = v46 - v32;
		float v37 = v43 * v43 + v36 * v36;
		float v38 = std::sqrt((double)v37);
		v9 = 0.5;
		float v39 = v38 * 0.5;
		afStartBias = v39;
		if (v39 > 0.5f) {
			afStartBias = 0.5f;
		}
		v10 = v23;
		v11 = v28;
		v12 = 1.0;
	}
	float v40 = v10 * ((m_kPort.m_right - m_kPort.m_left) * v9);
	v31.x = v40 + (m_kPort.m_right + m_kPort.m_left) * v9;
	float v41 = v11 * ((m_kPort.m_top - m_kPort.m_bottom) * v9);
	float v42 = v9 * (m_kPort.m_top + m_kPort.m_bottom) + v41;
	v31.y = v12 - v42;
	double x = v31.x;
	if (!(v31.x > 1.0f)) {
		v14 = 0.0;
		if (x < 0.0) {
			v31.x = 0.0f;
			x = 0.0f;
		}
	} else {
		v31.x = 1.0f;
		x = 1.0f;
		v14 = 0.0;
	}
	if (!(v31.y > 1.0f)) {
		if (v31.y < v14) {
			v31.y = v14;
		}
		v16 = 1.0;
		v17 = v14;
	} else {
		v16 = 1.0;
		v17 = v14;
		v31.y = 1.0f;
	}
	// Behind the camera x snaps to the nearer edge; a NaN x fails both of the exe's compares (0xB5A0A8, 0xB5A0D0) and
	// is kept.
	if (v3) {
		if (x <= v17) {
			v31.x = v17;
		} else if (x > v17) {
			v31.x = v16;
		}
	}
	aCenter = v31;
}
